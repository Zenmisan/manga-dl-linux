// JsWorker.cpp - QuickJS-ng runtime for the extension engine. See JsWorker.h for the design.
//
// Globals installed in every extension context:
//   console.log/info/debug/warn/error   forwarded to the application log
//   btoa(str) / atob(b64)               Base64 over the UTF-8 bytes of the string (same as the Windows app)
//   encodeURIComponent & friends        the engine's own built-ins (correct for all of Unicode)
//   URL                                 new URL(href[, base]) with .href .origin .host .pathname .search .hash
//   DOMParser                           parseFromString(html, mime) -> live Document (see DOM section)
//   apiFetch(url[, opts])               Promise<{status,text,html,url,json,...keys of json}>
//   setTimeout(fn, ms, ...args)         runs fn on the next microtask; the delay is NOT honoured
//   Blob, Image, XMLHttpRequest         inert stubs so browser-only code paths fail soft

#include "extensions/JsWorker.h"

#include "dom/HtmlParser.h"
#include "dom/Selector.h"
#include "http/HttpService.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QPointer>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVector>

#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

#include "quickjs.h"

namespace {

// ===============================================================================================================
// String / variant helpers
// ===============================================================================================================

qint64 nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

QString jsValueToString(JSContext *ctx, JSValueConst v)
{
    if (JS_IsUndefined(v) || JS_IsNull(v))
        return {};
    JSValue strVal = JS_ToString(ctx, v);
    size_t len = 0;
    const char *cstr = JS_ToCStringLen(ctx, &len, strVal);
    const QString result = cstr ? QString::fromUtf8(cstr, int(len)) : QString();
    if (cstr)
        JS_FreeCString(ctx, cstr);
    JS_FreeValue(ctx, strVal);
    return result;
}

JSValue jsString(JSContext *ctx, const QString &s)
{
    const QByteArray utf8 = s.toUtf8();
    return JS_NewStringLen(ctx, utf8.constData(), size_t(utf8.size()));
}

// "Name: message" for Error objects (the plain "Error" name is omitted), the string form for anything else.
QString describeError(JSContext *ctx, JSValueConst err)
{
    if (JS_IsObject(err)) {
        JSValue msg = JS_GetPropertyStr(ctx, err, "message");
        if (!JS_IsUndefined(msg)) {
            JSValue name = JS_GetPropertyStr(ctx, err, "name");
            const QString text = jsValueToString(ctx, msg);
            const QString nameText = JS_IsUndefined(name) ? QString() : jsValueToString(ctx, name);
            JS_FreeValue(ctx, name);
            JS_FreeValue(ctx, msg);
            if (nameText.isEmpty() || nameText == QLatin1String("Error"))
                return text;
            return nameText + QLatin1String(": ") + text;
        }
        JS_FreeValue(ctx, msg);
    }
    return jsValueToString(ctx, err);
}

QString takeExceptionText(JSContext *ctx)
{
    JSValue exc = JS_GetException(ctx);
    const QString text = describeError(ctx, exc);
    JS_FreeValue(ctx, exc);
    return text;
}

QVariant jsValueToVariant(JSContext *ctx, JSValueConst v, int depth = 0);

QVariantMap jsObjectToMap(JSContext *ctx, JSValueConst obj, int depth)
{
    QVariantMap map;
    JSPropertyEnum *props = nullptr;
    uint32_t count = 0;
    if (JS_GetOwnPropertyNames(ctx, &props, &count, obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0)
        return map;
    for (uint32_t k = 0; k < count; ++k) {
        const char *key = JS_AtomToCString(ctx, props[k].atom);
        JSValue val = JS_GetProperty(ctx, obj, props[k].atom);
        map.insert(QString::fromUtf8(key), jsValueToVariant(ctx, val, depth + 1));
        JS_FreeValue(ctx, val);
        JS_FreeCString(ctx, key);
        JS_FreeAtom(ctx, props[k].atom);
    }
    js_free(ctx, props);
    return map;
}

QVariant jsValueToVariant(JSContext *ctx, JSValueConst v, int depth)
{
    if (depth > 20 || JS_IsUndefined(v) || JS_IsNull(v))
        return {};
    if (JS_IsBool(v))
        return bool(JS_ToBool(ctx, v));
    if (JS_IsNumber(v)) {
        double d = 0;
        JS_ToFloat64(ctx, &d, v);
        if (!std::isfinite(d))
            return {};
        if (std::fabs(d) < 9.0e15 && d == std::floor(d))
            return QVariant(qint64(d));
        return QVariant(d);
    }
    if (JS_IsString(v))
        return jsValueToString(ctx, v);
    if (JS_IsArray(v)) {
        QVariantList list;
        JSValue lenV = JS_GetPropertyStr(ctx, v, "length");
        int32_t n = 0;
        JS_ToInt32(ctx, &n, lenV);
        JS_FreeValue(ctx, lenV);
        for (int32_t k = 0; k < n; ++k) {
            JSValue item = JS_GetPropertyUint32(ctx, v, uint32_t(k));
            list.append(jsValueToVariant(ctx, item, depth + 1));
            JS_FreeValue(ctx, item);
        }
        return list;
    }
    if (JS_IsObject(v))
        return jsObjectToMap(ctx, v, depth);
    return {};
}

// ===============================================================================================================
// DOM bindings
//
// Elements and documents are thin JS objects: all they hold is a shared_ptr to the C++ document plus a node id.
// Every property is a getter on the class prototype that reads the live tree, so `el.remove()` followed by
// `parent.innerHTML` sees the removal, and an element keeps its document alive for as long as the script holds it.
// ===============================================================================================================

struct NodeRef {
    std::shared_ptr<dom::Document> doc;
    dom::NodeId id = dom::kNone;
};

JSClassID gDocClass = 0;
JSClassID gNodeClass = 0;
JSClassID gListClass = 0;

void docFinalizer(JSRuntime *, JSValue v) { delete static_cast<NodeRef *>(JS_GetOpaque(v, gDocClass)); }
void nodeFinalizer(JSRuntime *, JSValue v) { delete static_cast<NodeRef *>(JS_GetOpaque(v, gNodeClass)); }
void listFinalizer(JSRuntime *, JSValue v) { delete static_cast<NodeRef *>(JS_GetOpaque(v, gListClass)); }

void registerDomClasses(JSRuntime *rt)
{
    // The class ids are process-wide statics shared by every runtime; allocate them once, under a lock.
    static QMutex mutex;
    QMutexLocker lock(&mutex);
    JS_NewClassID(rt, &gDocClass);
    JS_NewClassID(rt, &gNodeClass);
    JS_NewClassID(rt, &gListClass);

    JSClassDef def{};
    def.class_name = "DOMDocument";
    def.finalizer = docFinalizer;
    JS_NewClass(rt, gDocClass, &def);

    JSClassDef nodeDef{};
    nodeDef.class_name = "DOMNode";
    nodeDef.finalizer = nodeFinalizer;
    JS_NewClass(rt, gNodeClass, &nodeDef);

    JSClassDef listDef{};
    listDef.class_name = "DOMTokenList";
    listDef.finalizer = listFinalizer;
    JS_NewClass(rt, gListClass, &listDef);
}

JSValue wrapNode(JSContext *ctx, const std::shared_ptr<dom::Document> &doc, dom::NodeId id)
{
    if (id == dom::kNone)
        return JS_NULL;
    JSValue obj = JS_NewObjectClass(ctx, int(gNodeClass));
    if (JS_IsException(obj))
        return obj;
    JS_SetOpaque(obj, new NodeRef{doc, id});
    return obj;
}

JSValue wrapDocument(JSContext *ctx, const std::shared_ptr<dom::Document> &doc)
{
    JSValue obj = JS_NewObjectClass(ctx, int(gDocClass));
    if (JS_IsException(obj))
        return obj;
    JS_SetOpaque(obj, new NodeRef{doc, doc->root()});
    return obj;
}

JSValue wrapList(JSContext *ctx, const NodeRef &ref)
{
    JSValue obj = JS_NewObjectClass(ctx, int(gListClass));
    if (JS_IsException(obj))
        return obj;
    JS_SetOpaque(obj, new NodeRef{ref.doc, ref.id});
    return obj;
}

// Nodes and documents share most behaviour, so both prototypes use the same functions.
NodeRef *refOf(JSValueConst v)
{
    void *p = JS_GetOpaque(v, gNodeClass);
    if (!p)
        p = JS_GetOpaque(v, gDocClass);
    return static_cast<NodeRef *>(p);
}

NodeRef *listRefOf(JSValueConst v) { return static_cast<NodeRef *>(JS_GetOpaque(v, gListClass)); }

JSValue nodeArray(JSContext *ctx, const std::shared_ptr<dom::Document> &doc, const QVector<dom::NodeId> &ids)
{
    JSValue arr = JS_NewArray(ctx);
    for (int k = 0; k < ids.size(); ++k)
        JS_SetPropertyUint32(ctx, arr, uint32_t(k), wrapNode(ctx, doc, ids[k]));
    return arr;
}

bool hasClassToken(const QString &classAttr, const QString &token)
{
    const QStringList tokens = classAttr.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    return tokens.contains(token);
}

// --- getters ------------------------------------------------------------------------------------------------

JSValue gNodeType(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (!r)
        return JS_UNDEFINED;
    switch (r->doc->node(r->id).type) {
    case dom::NodeType::Element: return JS_NewInt32(ctx, 1);
    case dom::NodeType::Text: return JS_NewInt32(ctx, 3);
    case dom::NodeType::Comment: return JS_NewInt32(ctx, 8);
    case dom::NodeType::Document: return JS_NewInt32(ctx, 9);
    }
    return JS_UNDEFINED;
}

JSValue gNodeName(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (!r)
        return JS_UNDEFINED;
    const dom::Node &n = r->doc->node(r->id);
    switch (n.type) {
    case dom::NodeType::Element: return jsString(ctx, n.name.toUpper());
    case dom::NodeType::Text: return jsString(ctx, QStringLiteral("#text"));
    case dom::NodeType::Comment: return jsString(ctx, QStringLiteral("#comment"));
    case dom::NodeType::Document: return jsString(ctx, QStringLiteral("#document"));
    }
    return JS_UNDEFINED;
}

JSValue gTagName(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (!r || r->doc->node(r->id).type != dom::NodeType::Element)
        return JS_UNDEFINED;
    return jsString(ctx, r->doc->node(r->id).name.toUpper());
}

JSValue gNodeValue(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (!r)
        return JS_UNDEFINED;
    const dom::Node &n = r->doc->node(r->id);
    if (n.type == dom::NodeType::Text || n.type == dom::NodeType::Comment)
        return jsString(ctx, n.text);
    return JS_NULL;
}

JSValue gTextContent(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? jsString(ctx, r->doc->textContent(r->id)) : JS_UNDEFINED;
}

JSValue gInnerHtml(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? jsString(ctx, r->doc->innerHtml(r->id)) : JS_UNDEFINED;
}

JSValue gOuterHtml(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? jsString(ctx, r->doc->outerHtml(r->id)) : JS_UNDEFINED;
}

JSValue gId(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? jsString(ctx, r->doc->attr(r->id, QStringLiteral("id"))) : JS_UNDEFINED;
}

JSValue gClassName(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? jsString(ctx, r->doc->attr(r->id, QStringLiteral("class"))) : JS_UNDEFINED;
}

JSValue gHref(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? jsString(ctx, r->doc->attr(r->id, QStringLiteral("href"))) : JS_UNDEFINED;
}

// Same fallback the Windows app has: lazy-loaded images keep their address in data-src.
JSValue gSrc(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (!r)
        return JS_UNDEFINED;
    for (const char *name : {"src", "data-src", "data-lazy-src"}) {
        const QString v = r->doc->attr(r->id, QString::fromLatin1(name));
        if (!v.isEmpty())
            return jsString(ctx, v);
    }
    return jsString(ctx, QString());
}

JSValue gAttributes(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (!r)
        return JS_UNDEFINED;
    JSValue arr = JS_NewArray(ctx);
    const dom::Node &n = r->doc->node(r->id);
    for (int k = 0; k < n.attrs.size(); ++k) {
        JSValue a = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, a, "name", jsString(ctx, n.attrs[k].name));
        JS_SetPropertyStr(ctx, a, "value", jsString(ctx, n.attrs[k].value));
        JS_SetPropertyUint32(ctx, arr, uint32_t(k), a);
    }
    return arr;
}

JSValue gParentElement(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->parentElement(r->id)) : JS_UNDEFINED;
}

JSValue gParentNode(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->node(r->id).parent) : JS_UNDEFINED;
}

JSValue gChildren(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? nodeArray(ctx, r->doc, r->doc->elementChildren(r->id)) : JS_UNDEFINED;
}

JSValue gChildNodes(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? nodeArray(ctx, r->doc, r->doc->children(r->id)) : JS_UNDEFINED;
}

JSValue gFirstElementChild(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->firstElementChild(r->id)) : JS_UNDEFINED;
}

JSValue gLastElementChild(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->lastElementChild(r->id)) : JS_UNDEFINED;
}

JSValue gFirstChild(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->node(r->id).firstChild) : JS_UNDEFINED;
}

JSValue gLastChild(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->node(r->id).lastChild) : JS_UNDEFINED;
}

JSValue gNextElementSibling(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->nextElementSibling(r->id)) : JS_UNDEFINED;
}

JSValue gPrevElementSibling(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->prevElementSibling(r->id)) : JS_UNDEFINED;
}

JSValue gNextSibling(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->node(r->id).next) : JS_UNDEFINED;
}

JSValue gPrevSibling(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->node(r->id).prev) : JS_UNDEFINED;
}

JSValue gClassList(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapList(ctx, *r) : JS_UNDEFINED;
}

// Document-only
JSValue gBody(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->body()) : JS_UNDEFINED;
}

JSValue gHead(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->head()) : JS_UNDEFINED;
}

JSValue gDocumentElement(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    return r ? wrapNode(ctx, r->doc, r->doc->documentElement()) : JS_UNDEFINED;
}

JSValue gTitle(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (!r)
        return JS_UNDEFINED;
    const QVector<dom::NodeId> titles = r->doc->elementsByTagName(r->doc->root(), QStringLiteral("title"));
    return jsString(ctx, titles.isEmpty() ? QString() : r->doc->textContent(titles.first()).simplified());
}

// --- methods ------------------------------------------------------------------------------------------------

JSValue mGetAttribute(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NULL;
    const QString *v = r->doc->attrPtr(r->id, jsValueToString(ctx, argv[0]).toLower());
    return v ? jsString(ctx, *v) : JS_NULL;
}

JSValue mSetAttribute(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (r && argc >= 2)
        r->doc->setAttr(r->id, jsValueToString(ctx, argv[0]).toLower(), jsValueToString(ctx, argv[1]));
    return JS_UNDEFINED;
}

JSValue mHasAttribute(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_FALSE;
    return JS_NewBool(ctx, r->doc->hasAttr(r->id, jsValueToString(ctx, argv[0]).toLower()));
}

JSValue mRemoveAttribute(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (r && argc >= 1)
        r->doc->removeAttr(r->id, jsValueToString(ctx, argv[0]).toLower());
    return JS_UNDEFINED;
}

JSValue mGetAttributeNames(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (!r)
        return JS_UNDEFINED;
    JSValue arr = JS_NewArray(ctx);
    const dom::Node &n = r->doc->node(r->id);
    for (int k = 0; k < n.attrs.size(); ++k)
        JS_SetPropertyUint32(ctx, arr, uint32_t(k), jsString(ctx, n.attrs[k].name));
    return arr;
}

JSValue mRemove(JSContext *, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = refOf(t);
    if (r)
        r->doc->detach(r->id);
    return JS_UNDEFINED;
}

JSValue mQuerySelector(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NULL;
    return wrapNode(ctx, r->doc, dom::querySelector(*r->doc, r->id, jsValueToString(ctx, argv[0])));
}

JSValue mQuerySelectorAll(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NewArray(ctx);
    return nodeArray(ctx, r->doc, dom::querySelectorAll(*r->doc, r->id, jsValueToString(ctx, argv[0])));
}

JSValue mClosest(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NULL;
    return wrapNode(ctx, r->doc, dom::closest(*r->doc, r->id, jsValueToString(ctx, argv[0])));
}

// closest() tests the element itself first, so it returns `id` exactly when the element matches.
JSValue mMatches(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_FALSE;
    return JS_NewBool(ctx, dom::closest(*r->doc, r->id, jsValueToString(ctx, argv[0])) == r->id);
}

JSValue mContains(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    NodeRef *other = argc >= 1 ? refOf(argv[0]) : nullptr;
    if (!r || !other || other->doc != r->doc)
        return JS_FALSE;
    for (dom::NodeId n = other->id; n != dom::kNone; n = r->doc->node(n).parent) {
        if (n == r->id)
            return JS_TRUE;
    }
    Q_UNUSED(ctx)
    return JS_FALSE;
}

JSValue mGetElementsByTagName(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NewArray(ctx);
    return nodeArray(ctx, r->doc, r->doc->elementsByTagName(r->id, jsValueToString(ctx, argv[0]).toLower()));
}

JSValue mGetElementsByClassName(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NewArray(ctx);
    const QStringList wanted = jsValueToString(ctx, argv[0]).simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QVector<dom::NodeId> hits;
    if (!wanted.isEmpty()) {
        const QString classAttr = QStringLiteral("class");
        for (dom::NodeId c = r->doc->nextPreorder(r->id, r->id); c != dom::kNone; c = r->doc->nextPreorder(r->id, c)) {
            if (r->doc->node(c).type != dom::NodeType::Element)
                continue;
            const QString classes = r->doc->attr(c, classAttr);
            bool all = true;
            for (const QString &w : wanted) {
                if (!hasClassToken(classes, w)) {
                    all = false;
                    break;
                }
            }
            if (all)
                hits.append(c);
        }
    }
    return nodeArray(ctx, r->doc, hits);
}

JSValue mGetElementById(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NULL;
    const QString wanted = jsValueToString(ctx, argv[0]);
    const QString idAttr = QStringLiteral("id");
    for (dom::NodeId c = r->doc->nextPreorder(r->id, r->id); c != dom::kNone; c = r->doc->nextPreorder(r->id, c)) {
        if (r->doc->node(c).type == dom::NodeType::Element && r->doc->attr(c, idAttr) == wanted)
            return wrapNode(ctx, r->doc, c);
    }
    return JS_NULL;
}

JSValue mCreateElement(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NULL;
    return wrapNode(ctx, r->doc, r->doc->createElement(jsValueToString(ctx, argv[0]).toLower()));
}

JSValue mCreateTextNode(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = refOf(t);
    if (!r || argc < 1)
        return JS_NULL;
    return wrapNode(ctx, r->doc, r->doc->createText(jsValueToString(ctx, argv[0])));
}

// --- classList (DOMTokenList) -------------------------------------------------------------------------------

QStringList classTokens(const NodeRef *r)
{
    return r->doc->attr(r->id, QStringLiteral("class")).simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

void setClassTokens(const NodeRef *r, const QStringList &tokens)
{
    r->doc->setAttr(r->id, QStringLiteral("class"), tokens.join(QLatin1Char(' ')));
}

JSValue lContains(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = listRefOf(t);
    if (!r || argc < 1)
        return JS_FALSE;
    return JS_NewBool(ctx, classTokens(r).contains(jsValueToString(ctx, argv[0])));
}

JSValue lAdd(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = listRefOf(t);
    if (!r)
        return JS_UNDEFINED;
    QStringList tokens = classTokens(r);
    for (int k = 0; k < argc; ++k) {
        const QString token = jsValueToString(ctx, argv[k]);
        if (!token.isEmpty() && !tokens.contains(token))
            tokens.append(token);
    }
    setClassTokens(r, tokens);
    return JS_UNDEFINED;
}

JSValue lRemove(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = listRefOf(t);
    if (!r)
        return JS_UNDEFINED;
    QStringList tokens = classTokens(r);
    for (int k = 0; k < argc; ++k)
        tokens.removeAll(jsValueToString(ctx, argv[k]));
    setClassTokens(r, tokens);
    return JS_UNDEFINED;
}

JSValue lToggle(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = listRefOf(t);
    if (!r || argc < 1)
        return JS_FALSE;
    const QString token = jsValueToString(ctx, argv[0]);
    QStringList tokens = classTokens(r);
    const bool present = tokens.contains(token);
    const bool want = argc >= 2 && !JS_IsUndefined(argv[1]) ? bool(JS_ToBool(ctx, argv[1])) : !present;
    if (want && !present)
        tokens.append(token);
    else if (!want && present)
        tokens.removeAll(token);
    setClassTokens(r, tokens);
    return JS_NewBool(ctx, want);
}

JSValue lItem(JSContext *ctx, JSValueConst t, int argc, JSValueConst *argv)
{
    NodeRef *r = listRefOf(t);
    int32_t index = 0;
    if (!r || argc < 1 || JS_ToInt32(ctx, &index, argv[0]) < 0)
        return JS_NULL;
    const QStringList tokens = classTokens(r);
    return index >= 0 && index < tokens.size() ? jsString(ctx, tokens[index]) : JS_NULL;
}

JSValue lToString(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = listRefOf(t);
    return r ? jsString(ctx, classTokens(r).join(QLatin1Char(' '))) : JS_UNDEFINED;
}

JSValue lLength(JSContext *ctx, JSValueConst t, int, JSValueConst *)
{
    NodeRef *r = listRefOf(t);
    return r ? JS_NewInt32(ctx, int(classTokens(r).size())) : JS_UNDEFINED;
}

// --- prototype installation ---------------------------------------------------------------------------------

struct MethodDef {
    const char *name;
    JSCFunction *fn;
    int length;
};

struct GetterDef {
    const char *name;
    JSCFunction *fn;
};

void defineMethods(JSContext *ctx, JSValueConst proto, const MethodDef *defs, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        JS_SetPropertyStr(ctx, proto, defs[i].name, JS_NewCFunction(ctx, defs[i].fn, defs[i].name, defs[i].length));
}

void defineGetters(JSContext *ctx, JSValueConst proto, const GetterDef *defs, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        const JSAtom atom = JS_NewAtom(ctx, defs[i].name);
        JSValue getter = JS_NewCFunction(ctx, defs[i].fn, defs[i].name, 0);
        // The getter reference is consumed by the call; there is no setter (assignments are ignored).
        JS_DefinePropertyGetSet(ctx, proto, atom, getter, JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
        JS_FreeAtom(ctx, atom);
    }
}

const MethodDef kCommonMethods[] = {
    {"getAttribute", mGetAttribute, 1},
    {"setAttribute", mSetAttribute, 2},
    {"hasAttribute", mHasAttribute, 1},
    {"removeAttribute", mRemoveAttribute, 1},
    {"getAttributeNames", mGetAttributeNames, 0},
    {"remove", mRemove, 0},
    {"querySelector", mQuerySelector, 1},
    {"querySelectorAll", mQuerySelectorAll, 1},
    {"closest", mClosest, 1},
    {"matches", mMatches, 1},
    {"contains", mContains, 1},
    {"getElementsByTagName", mGetElementsByTagName, 1},
    {"getElementsByClassName", mGetElementsByClassName, 1},
    {"getElementById", mGetElementById, 1},
};

const MethodDef kDocumentMethods[] = {
    {"createElement", mCreateElement, 1},
    {"createTextNode", mCreateTextNode, 1},
};

const GetterDef kCommonGetters[] = {
    {"nodeType", gNodeType},
    {"nodeName", gNodeName},
    {"childNodes", gChildNodes},
    {"children", gChildren},
    {"firstElementChild", gFirstElementChild},
    {"lastElementChild", gLastElementChild},
    {"firstChild", gFirstChild},
    {"lastChild", gLastChild},
};

const GetterDef kNodeGetters[] = {
    {"tagName", gTagName},
    {"nodeValue", gNodeValue},
    {"data", gNodeValue},
    {"textContent", gTextContent},
    {"innerHTML", gInnerHtml},
    {"outerHTML", gOuterHtml},
    {"id", gId},
    {"className", gClassName},
    {"href", gHref},
    {"src", gSrc},
    {"attributes", gAttributes},
    {"parentElement", gParentElement},
    {"parentNode", gParentNode},
    {"nextElementSibling", gNextElementSibling},
    {"previousElementSibling", gPrevElementSibling},
    {"nextSibling", gNextSibling},
    {"previousSibling", gPrevSibling},
    {"classList", gClassList},
};

const GetterDef kDocumentGetters[] = {
    {"body", gBody},
    {"head", gHead},
    {"documentElement", gDocumentElement},
    {"title", gTitle},
};

const MethodDef kListMethods[] = {
    {"contains", lContains, 1},
    {"add", lAdd, 1},
    {"remove", lRemove, 1},
    {"toggle", lToggle, 2},
    {"item", lItem, 1},
    {"toString", lToString, 0},
};

const GetterDef kListGetters[] = {
    {"length", lLength},
    {"value", lToString},
};

template <size_t N>
void defineMethods(JSContext *ctx, JSValueConst proto, const MethodDef (&defs)[N])
{
    defineMethods(ctx, proto, defs, N);
}

template <size_t N>
void defineGetters(JSContext *ctx, JSValueConst proto, const GetterDef (&defs)[N])
{
    defineGetters(ctx, proto, defs, N);
}

// Class prototypes live in the context, so this runs once per context.
void installDomClasses(JSContext *ctx)
{
    JSValue nodeProto = JS_NewObject(ctx);
    defineGetters(ctx, nodeProto, kCommonGetters);
    defineGetters(ctx, nodeProto, kNodeGetters);
    defineMethods(ctx, nodeProto, kCommonMethods);
    JS_SetClassProto(ctx, gNodeClass, nodeProto);  // takes ownership

    JSValue docProto = JS_NewObject(ctx);
    defineGetters(ctx, docProto, kCommonGetters);
    defineGetters(ctx, docProto, kDocumentGetters);
    defineMethods(ctx, docProto, kCommonMethods);
    defineMethods(ctx, docProto, kDocumentMethods);
    JS_SetClassProto(ctx, gDocClass, docProto);

    JSValue listProto = JS_NewObject(ctx);
    defineGetters(ctx, listProto, kListGetters);
    defineMethods(ctx, listProto, kListMethods);
    JS_SetClassProto(ctx, gListClass, listProto);
}

JSValue jsDomParserParse(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 1)
        return JS_ThrowTypeError(ctx, "parseFromString requires a string");
    std::shared_ptr<dom::Document> doc = dom::parseHtml(jsValueToString(ctx, argv[0]));
    return wrapDocument(ctx, doc);
}

JSValue jsDomParserNew(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "parseFromString", JS_NewCFunction(ctx, jsDomParserParse, "parseFromString", 2));
    return obj;
}

// ===============================================================================================================
// Other globals
// ===============================================================================================================

JSValue jsConsoleLog(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int tag)
{
    QStringList parts;
    for (int k = 0; k < argc; ++k)
        parts.append(jsValueToString(ctx, argv[k]));
    const QString msg = parts.join(QLatin1Char(' '));
    if (tag == 0)
        qDebug().noquote() << "[JS]" << msg;
    else if (tag == 1)
        qWarning().noquote() << "[JS]" << msg;
    else
        qCritical().noquote() << "[JS]" << msg;
    return JS_UNDEFINED;
}

JSValue jsBtoa(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 1)
        return JS_ThrowTypeError(ctx, "btoa requires an argument");
    const QByteArray b64 = jsValueToString(ctx, argv[0]).toUtf8().toBase64();
    return JS_NewStringLen(ctx, b64.constData(), size_t(b64.size()));
}

JSValue jsAtob(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 1)
        return JS_ThrowTypeError(ctx, "atob requires an argument");
    const QByteArray decoded = QByteArray::fromBase64(jsValueToString(ctx, argv[0]).toLatin1());
    return jsString(ctx, QString::fromUtf8(decoded));
}

JSValue jsUrlNew(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    if (argc < 1)
        return JS_ThrowTypeError(ctx, "URL requires an href argument");
    const QString href = jsValueToString(ctx, argv[0]);
    QUrl url;
    if (argc >= 2 && !JS_IsUndefined(argv[1]))
        url = QUrl(jsValueToString(ctx, argv[1])).resolved(QUrl(href));
    else
        url = QUrl(href);
    JSValue obj = JS_NewObject(ctx);
    auto set = [&](const char *key, const QString &value) {
        JS_SetPropertyStr(ctx, obj, key, jsString(ctx, value));
    };
    QString origin = url.scheme() + QLatin1String("://") + url.host();
    if (url.port() > 0)
        origin += QLatin1Char(':') + QString::number(url.port());
    set("href", url.toString());
    set("origin", origin);
    set("host", url.port() > 0 ? url.host() + QLatin1Char(':') + QString::number(url.port()) : url.host());
    set("hostname", url.host());
    set("pathname", url.path());
    set("search", url.hasQuery() ? QStringLiteral("?") + url.query() : QString());
    set("hash", url.hasFragment() ? QStringLiteral("#") + url.fragment() : QString());
    set("protocol", url.scheme() + QStringLiteral(":"));
    return obj;
}

// Page setup that is simplest to write in JavaScript.
const char *const kPrelude =
    "var Blob = function (parts, opts) { this._parts = parts; this._opts = opts; };\n"
    "var Image = function () { this.naturalWidth = 0; this.naturalHeight = 0; };\n"
    "var XMLHttpRequest = function () {\n"
    "  this.open = function () {}; this.send = function () {}; this.setRequestHeader = function () {};\n"
    "};\n"
    "globalThis.setTimeout = function (fn, ms) {\n"
    "  var args = Array.prototype.slice.call(arguments, 2);\n"
    "  Promise.resolve().then(function () { fn.apply(null, args); });\n"
    "  return 1;\n"
    "};\n"
    "globalThis.clearTimeout = function () {};\n";

} // namespace

// ===============================================================================================================
// JsWorker::Impl
// ===============================================================================================================

struct JsWorker::Impl
{
    struct ExtCtx {
        QString id;
        QByteArray hash;
        JSContext *ctx = nullptr;
        JSValue ext = JS_UNDEFINED;  // the script's `extension` object
    };

    struct PendingFetch {
        JSContext *ctx = nullptr;
        JSValue resolve = JS_UNDEFINED;
        JSValue reject = JS_UNDEFINED;
        QPointer<QNetworkReply> reply;
        QString requestUrl;
        QVariantMap opts;
        bool tooLarge = false;
    };

    struct PendingCall {
        QString extId;
        QTimer *timer = nullptr;
    };

    explicit Impl(JsWorker *owner) : q(owner) {}
    ~Impl() { shutdown(); }
    Impl(const Impl &) = delete;
    Impl &operator=(const Impl &) = delete;

    JsWorker *q;
    JSRuntime *rt = nullptr;
    JSContext *scratch = nullptr;
    std::unique_ptr<HttpService> http;
    QHash<QString, ExtCtx *> exts;
    QHash<quint64, PendingFetch> fetches;
    quint64 nextFetchId = 0;
    QHash<quint64, PendingCall> calls;
    qint64 sliceDeadline = 0;  // 0 = no script is running, so nothing to interrupt
    int sliceMs = JsWorker::kDefaultSliceMs;
    int callMs = JsWorker::kDefaultCallMs;
    JsWorker::FetchInterceptor interceptor;
    JsWorker::FetchTap tap;

    void init();
    void shutdown();
    JSContext *newContext();
    void installGlobals(JSContext *ctx);
    void armSlice() { sliceDeadline = nowMs() + sliceMs; }
    void disarmSlice() { sliceDeadline = 0; }
    void pump();

    ExtCtx *ensureExtension(const QString &id, const QString &code, QString *error);
    void dropExtension(const QString &id);
    void invoke(quint64 callId, const QString &extId, const QString &code, const QString &method,
                const QString &argsJson);
    void finishCall(quint64 callId, const JsResult &result);

    void startFetch(JSContext *ctx, const QString &url, const QVariantMap &opts, JSValue resolveFn, JSValue rejectFn);
    bool takeFetch(quint64 id, PendingFetch *out);
    void discardFetch(quint64 id);
    void finishFetch(quint64 id, QNetworkReply *reply);
    void deliverFake(quint64 id, const JsWorker::FakeResponse &fake);
    void resolveFetch(quint64 id, int status, const QString &text, const QString &finalUrl);
    void failFetch(quint64 id, const QString &message);
    void notifyTap(const PendingFetch &pending, int status, const QString &text, const QString &finalUrl,
                   const QString &transportError);
    JSValue buildFetchResult(JSContext *ctx, int status, const QString &text, const QString &finalUrl);

    JsResult evalSync(const QString &code);
};

namespace {

JsWorker::Impl *implOf(JSContext *ctx)
{
    return static_cast<JsWorker::Impl *>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
}

// Called by the engine every few thousand instructions; stops a script that is stuck in a loop.
int interruptHandler(JSRuntime *, void *opaque)
{
    auto *d = static_cast<JsWorker::Impl *>(opaque);
    return (d->sliceDeadline != 0 && nowMs() > d->sliceDeadline) ? 1 : 0;
}

// apiFetch(url[, opts]) -> Promise. Never blocks: the request runs on the worker's event loop.
JSValue jsApiFetch(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    JsWorker::Impl *d = implOf(ctx);
    if (!d)
        return JS_ThrowInternalError(ctx, "apiFetch: worker unavailable");
    if (argc < 1)
        return JS_ThrowTypeError(ctx, "apiFetch requires a url");

    const QString url = jsValueToString(ctx, argv[0]);
    QVariantMap opts;
    if (argc >= 2 && JS_IsObject(argv[1]))
        opts = jsObjectToMap(ctx, argv[1], 0);

    JSValue funcs[2];
    JSValue promise = JS_NewPromiseCapability(ctx, funcs);
    if (JS_IsException(promise))
        return promise;
    d->startFetch(ctx, url, opts, funcs[0], funcs[1]);  // takes ownership of resolve and reject
    return promise;
}

// Promise reaction bound to one call id: magic 0 = fulfilled, 1 = rejected.
JSValue jsCallSettled(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic, JSValueConst *data)
{
    JsWorker::Impl *d = implOf(ctx);
    if (!d)
        return JS_UNDEFINED;
    int64_t id = 0;
    JS_ToInt64(ctx, &id, data[0]);

    JsResult result;
    if (magic == 0) {
        result.ok = true;
        result.value = argc > 0 ? jsValueToVariant(ctx, argv[0]) : QVariant();
    } else {
        result.errorText = argc > 0 ? describeError(ctx, argv[0]) : QStringLiteral("rejected");
    }
    d->finishCall(quint64(id), result);
    return JS_UNDEFINED;
}

JSValue makeSettleFn(JSContext *ctx, quint64 callId, int magic)
{
    JSValue data[1] = {JS_NewInt64(ctx, qint64(callId))};
    JSValue fn = JS_NewCFunctionData(ctx, reinterpret_cast<JSCFunctionData *>(jsCallSettled), 1, magic, 1, data);
    JS_FreeValue(ctx, data[0]);
    return fn;
}

// Parses a JSON array into positional arguments. Elements are owned by `out`.
bool parseArgs(JSContext *ctx, const QString &argsJson, std::vector<JSValue> &out)
{
    if (argsJson.trimmed().isEmpty())
        return true;
    const QByteArray utf8 = argsJson.toUtf8();
    JSValue parsed = JS_ParseJSON(ctx, utf8.constData(), size_t(utf8.size()), "<args>");
    if (JS_IsException(parsed)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        return false;
    }
    if (!JS_IsArray(parsed)) {
        JS_FreeValue(ctx, parsed);
        return false;
    }
    JSValue lenV = JS_GetPropertyStr(ctx, parsed, "length");
    int32_t n = 0;
    JS_ToInt32(ctx, &n, lenV);
    JS_FreeValue(ctx, lenV);
    for (int32_t k = 0; k < n && k < 16; ++k)
        out.push_back(JS_GetPropertyUint32(ctx, parsed, uint32_t(k)));
    JS_FreeValue(ctx, parsed);
    return true;
}

// Promise.resolve(value): turns a plain return value and a Promise into the same thing. Consumes `value`.
JSValue promiseResolve(JSContext *ctx, JSValue value)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue ctor = JS_GetPropertyStr(ctx, global, "Promise");
    JSValue resolveFn = JS_GetPropertyStr(ctx, ctor, "resolve");
    JSValue promise = JS_Call(ctx, resolveFn, ctor, 1, &value);
    JS_FreeValue(ctx, resolveFn);
    JS_FreeValue(ctx, ctor);
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, value);
    return promise;
}

QString classifyException(JsWorker::Impl *d, const QString &text)
{
    if (text.contains(QLatin1String("interrupted")))
        return QStringLiteral("[timeout] script ran for more than %1 s without waiting").arg(d->sliceMs / 1000.0);
    return text;
}

} // namespace

void JsWorker::Impl::init()
{
    rt = JS_NewRuntime();
    JS_SetMemoryLimit(rt, size_t(JsWorker::kMemLimitMB) * 1024 * 1024);
    JS_SetMaxStackSize(rt, 1024 * 1024);
    JS_SetRuntimeOpaque(rt, this);
    JS_SetInterruptHandler(rt, interruptHandler, this);
    registerDomClasses(rt);

    http = std::make_unique<HttpService>();
    scratch = newContext();
    emit q->initDone();
}

void JsWorker::Impl::shutdown()
{
    for (auto it = calls.begin(); it != calls.end(); ++it) {
        if (it.value().timer) {
            it.value().timer->stop();
            it.value().timer->deleteLater();
        }
    }
    calls.clear();

    const QList<quint64> fetchIds = fetches.keys();
    for (quint64 id : fetchIds)
        discardFetch(id);

    const QStringList extIds = exts.keys();
    for (const QString &id : extIds)
        dropExtension(id);

    if (scratch) {
        JS_FreeContext(scratch);
        scratch = nullptr;
    }
    if (rt) {
        JS_FreeRuntime(rt);
        rt = nullptr;
    }
    http.reset();
}

JSContext *JsWorker::Impl::newContext()
{
    JSContext *ctx = JS_NewContext(rt);
    if (ctx)
        installGlobals(ctx);
    return ctx;
}

void JsWorker::Impl::installGlobals(JSContext *ctx)
{
    installDomClasses(ctx);
    JSValue global = JS_GetGlobalObject(ctx);

    JSValue console = JS_NewObject(ctx);
    const struct {
        const char *name;
        int tag;
    } consoleFns[] = {{"log", 0}, {"info", 0}, {"debug", 0}, {"warn", 1}, {"error", 2}};
    for (const auto &fn : consoleFns) {
        JS_SetPropertyStr(ctx, console, fn.name,
                          JS_NewCFunctionMagic(ctx, reinterpret_cast<JSCFunctionMagic *>(jsConsoleLog), fn.name, 1,
                                               JS_CFUNC_generic_magic, fn.tag));
    }
    JS_SetPropertyStr(ctx, global, "console", console);

    JS_SetPropertyStr(ctx, global, "btoa", JS_NewCFunction(ctx, jsBtoa, "btoa", 1));
    JS_SetPropertyStr(ctx, global, "atob", JS_NewCFunction(ctx, jsAtob, "atob", 1));
    JS_SetPropertyStr(ctx, global, "URL", JS_NewCFunction2(ctx, jsUrlNew, "URL", 1, JS_CFUNC_constructor, 0));
    JS_SetPropertyStr(ctx, global, "DOMParser",
                      JS_NewCFunction2(ctx, jsDomParserNew, "DOMParser", 0, JS_CFUNC_constructor, 0));
    JS_SetPropertyStr(ctx, global, "apiFetch", JS_NewCFunction(ctx, jsApiFetch, "apiFetch", 2));
    JS_FreeValue(ctx, global);

    JSValue r = JS_Eval(ctx, kPrelude, std::strlen(kPrelude), "<prelude>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r))
        qWarning().noquote() << "[JsWorker] prelude failed:" << takeExceptionText(ctx);
    JS_FreeValue(ctx, r);
}

// Runs every queued promise continuation, from any context.
void JsWorker::Impl::pump()
{
    for (;;) {
        armSlice();
        JSContext *jobCtx = nullptr;
        const int rc = JS_ExecutePendingJob(rt, &jobCtx);
        if (rc == 0)
            break;
        if (rc < 0 && jobCtx)
            qWarning().noquote() << "[JsWorker] a promise job threw:" << takeExceptionText(jobCtx);
    }
    disarmSlice();
}

JsWorker::Impl::ExtCtx *JsWorker::Impl::ensureExtension(const QString &id, const QString &code, QString *error)
{
    const QByteArray hash = QCryptographicHash::hash(code.toUtf8(), QCryptographicHash::Sha1);
    if (ExtCtx *existing = exts.value(id, nullptr)) {
        if (existing->hash == hash)
            return existing;
        dropExtension(id);  // the script changed on disk: start from a clean context
    }

    JSContext *ctx = newContext();
    if (!ctx) {
        *error = QStringLiteral("[load] could not create a JavaScript context for %1").arg(id);
        return nullptr;
    }

    const QByteArray source = code.toUtf8();
    const QByteArray fileName = ("<" + id + ".js>").toUtf8();
    armSlice();
    JSValue evaluated = JS_Eval(ctx, source.constData(), size_t(source.size()), fileName.constData(),
                                JS_EVAL_TYPE_GLOBAL);
    disarmSlice();
    if (JS_IsException(evaluated)) {
        *error = QStringLiteral("[load] ") + classifyException(this, takeExceptionText(ctx));
        JS_FreeContext(ctx);
        return nullptr;
    }
    JS_FreeValue(ctx, evaluated);
    pump();

    // `extension` may be a var or a const/let, so read it with an expression rather than as a global property.
    armSlice();
    JSValue ext = JS_Eval(ctx, "extension", 9, "<extension>", JS_EVAL_TYPE_GLOBAL);
    disarmSlice();
    if (JS_IsException(ext)) {
        *error = QStringLiteral("[load] the script does not define `extension`: ") + takeExceptionText(ctx);
        JS_FreeContext(ctx);
        return nullptr;
    }
    if (!JS_IsObject(ext)) {
        JS_FreeValue(ctx, ext);
        JS_FreeContext(ctx);
        *error = QStringLiteral("[load] `extension` is not an object");
        return nullptr;
    }

    auto *entry = new ExtCtx;
    entry->id = id;
    entry->hash = hash;
    entry->ctx = ctx;
    entry->ext = ext;
    exts.insert(id, entry);
    return entry;
}

void JsWorker::Impl::dropExtension(const QString &id)
{
    ExtCtx *entry = exts.take(id);
    if (!entry)
        return;

    const QList<quint64> callIds = calls.keys();
    for (quint64 callId : callIds) {
        if (calls.value(callId).extId == id) {
            JsResult r;
            r.errorText = QStringLiteral("[load] the extension script was replaced while the call was running");
            finishCall(callId, r);
        }
    }
    const QList<quint64> fetchIds = fetches.keys();
    for (quint64 fetchId : fetchIds) {
        if (fetches.value(fetchId).ctx == entry->ctx)
            discardFetch(fetchId);
    }
    JS_FreeValue(entry->ctx, entry->ext);
    JS_FreeContext(entry->ctx);
    delete entry;
}

void JsWorker::Impl::finishCall(quint64 callId, const JsResult &result)
{
    const auto it = calls.find(callId);
    if (it == calls.end())
        return;  // already finished (for example it timed out before the script settled)
    QTimer *timer = it.value().timer;
    calls.erase(it);
    if (timer) {
        timer->stop();
        timer->deleteLater();
    }
    emit q->scriptFinished(callId, result);
}

void JsWorker::Impl::invoke(quint64 callId, const QString &extId, const QString &code, const QString &method,
                            const QString &argsJson)
{
    JsResult res;
    if (!rt) {
        res.errorText = QStringLiteral("[load] the JavaScript worker is not initialised");
        emit q->scriptFinished(callId, res);
        return;
    }

    QString loadError;
    ExtCtx *ext = ensureExtension(extId, code, &loadError);
    if (!ext) {
        res.errorText = loadError;
        emit q->scriptFinished(callId, res);
        return;
    }
    JSContext *ctx = ext->ctx;

    std::vector<JSValue> args;
    if (!parseArgs(ctx, argsJson, args)) {
        for (JSValue &a : args)
            JS_FreeValue(ctx, a);
        res.errorText = QStringLiteral("[call] arguments must be a JSON array, got: %1").arg(argsJson.left(80));
        emit q->scriptFinished(callId, res);
        return;
    }

    JSValue fn = JS_GetPropertyStr(ctx, ext->ext, method.toUtf8().constData());
    if (!JS_IsFunction(ctx, fn)) {
        JS_FreeValue(ctx, fn);
        for (JSValue &a : args)
            JS_FreeValue(ctx, a);
        res.errorText = QStringLiteral("[call] extension '%1' has no method '%2'").arg(extId, method);
        emit q->scriptFinished(callId, res);
        return;
    }

    // Call it as a method of `extension`: scripts rely on `this` (asurascans calls this.search()).
    armSlice();
    JSValue ret = JS_Call(ctx, fn, ext->ext, int(args.size()), args.empty() ? nullptr : args.data());
    disarmSlice();
    JS_FreeValue(ctx, fn);
    for (JSValue &a : args)
        JS_FreeValue(ctx, a);

    if (JS_IsException(ret)) {
        res.errorText = classifyException(this, takeExceptionText(ctx));
        emit q->scriptFinished(callId, res);
        return;
    }

    // Whatever the method returned (value or Promise), settle through promise reactions bound to this call id.
    JSValue promise = promiseResolve(ctx, ret);
    if (JS_IsException(promise)) {
        res.errorText = takeExceptionText(ctx);
        emit q->scriptFinished(callId, res);
        return;
    }
    JSValue reactions[2] = {makeSettleFn(ctx, callId, 0), makeSettleFn(ctx, callId, 1)};
    JSValue thenFn = JS_GetPropertyStr(ctx, promise, "then");
    JS_FreeValue(ctx, JS_Call(ctx, thenFn, promise, 2, reactions));
    JS_FreeValue(ctx, thenFn);
    JS_FreeValue(ctx, reactions[0]);
    JS_FreeValue(ctx, reactions[1]);
    JS_FreeValue(ctx, promise);

    auto *timer = new QTimer(q);
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, q, [this, callId] {
        JsResult r;
        r.errorText = QStringLiteral("[timeout] the call did not finish within %1 s").arg(callMs / 1000.0);
        finishCall(callId, r);
    });
    timer->start(callMs);
    calls.insert(callId, PendingCall{extId, timer});

    pump();  // a script that never waits settles right here
}

// --- networking ---------------------------------------------------------------------------------------------

void JsWorker::Impl::startFetch(JSContext *ctx, const QString &url, const QVariantMap &opts, JSValue resolveFn,
                                JSValue rejectFn)
{
    const quint64 id = ++nextFetchId;
    PendingFetch pending;
    pending.ctx = ctx;
    pending.resolve = resolveFn;
    pending.reject = rejectFn;
    pending.requestUrl = url;
    pending.opts = opts;
    fetches.insert(id, pending);

    // Everything below settles the promise later, from the event loop: apiFetch itself must return first.
    if (interceptor) {
        JsWorker::FakeResponse fake;
        if (interceptor(url, opts, fake)) {
            QTimer::singleShot(fake.delayMs, q, [this, id, fake] { deliverFake(id, fake); });
            return;
        }
    }

    QString error;
    QNetworkReply *reply = http->send(url, opts, &error);
    if (!reply) {
        QTimer::singleShot(0, q, [this, id, error] { failFetch(id, QStringLiteral("[network] ") + error); });
        return;
    }
    fetches[id].reply = reply;
    QObject::connect(reply, &QNetworkReply::downloadProgress, q, [this, id, reply](qint64 received, qint64) {
        if (received > JsWorker::kMaxBodyBytes) {
            const auto it = fetches.find(id);
            if (it != fetches.end())
                it.value().tooLarge = true;
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, q, [this, id, reply] { finishFetch(id, reply); });
}

bool JsWorker::Impl::takeFetch(quint64 id, PendingFetch *out)
{
    const auto it = fetches.find(id);
    if (it == fetches.end())
        return false;
    *out = it.value();
    fetches.erase(it);
    return true;
}

// Drops a request that nobody will wait for (shutdown, or its extension was replaced).
void JsWorker::Impl::discardFetch(quint64 id)
{
    PendingFetch pending;
    if (!takeFetch(id, &pending))
        return;
    if (pending.reply) {
        QObject::disconnect(pending.reply, nullptr, q, nullptr);
        pending.reply->abort();
        pending.reply->deleteLater();
    }
    JS_FreeValue(pending.ctx, pending.resolve);
    JS_FreeValue(pending.ctx, pending.reject);
}

void JsWorker::Impl::finishFetch(quint64 id, QNetworkReply *reply)
{
    const auto it = fetches.find(id);
    if (it == fetches.end()) {
        reply->deleteLater();
        return;
    }
    const bool tooLarge = it.value().tooLarge;
    const HttpService::Response response = HttpService::toResponse(reply);
    reply->deleteLater();

    if (tooLarge) {
        failFetch(id, QStringLiteral("[network] the response is larger than %1 MB").arg(JsWorker::kMaxBodyBytes >> 20));
    } else if (response.transportError) {
        const QString tag = response.timedOut ? QStringLiteral("[timeout] ") : QStringLiteral("[network] ");
        failFetch(id, tag + response.errorText);
    } else {
        resolveFetch(id, response.status, response.text, response.url);
    }
}

void JsWorker::Impl::deliverFake(quint64 id, const JsWorker::FakeResponse &fake)
{
    if (!fake.transportError.isEmpty())
        failFetch(id, QStringLiteral("[network] ") + fake.transportError);
    else
        resolveFetch(id, fake.status, fake.text, fake.url);
}

// { status, text, html, url, json, ...top-level keys of json }. The reserved keys are never overwritten by
// the body: a JSON body that contains "status" must not replace the HTTP status.
JSValue JsWorker::Impl::buildFetchResult(JSContext *ctx, int status, const QString &text, const QString &finalUrl)
{
    JSValue json = JS_NULL;
    const QByteArray utf8 = text.toUtf8();
    int first = 0;
    while (first < utf8.size() && (utf8[first] == ' ' || utf8[first] == '\t' || utf8[first] == '\n' || utf8[first] == '\r'))
        ++first;
    if (first < utf8.size() && (utf8[first] == '{' || utf8[first] == '[')) {
        JSValue parsed = JS_ParseJSON(ctx, utf8.constData() + first, size_t(utf8.size() - first), "<response>");
        if (JS_IsException(parsed))
            JS_FreeValue(ctx, JS_GetException(ctx));  // not JSON after all: json stays null
        else
            json = parsed;
    }

    JSValue result;
    if (JS_IsArray(json)) {
        result = json;
    } else {
        result = JS_NewObject(ctx);
        if (JS_IsObject(json)) {
            JSPropertyEnum *props = nullptr;
            uint32_t count = 0;
            if (JS_GetOwnPropertyNames(ctx, &props, &count, json, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
                for (uint32_t k = 0; k < count; ++k) {
                    if (JS_HasProperty(ctx, result, props[k].atom) == 0)
                        JS_SetProperty(ctx, result, props[k].atom, JS_GetProperty(ctx, json, props[k].atom));
                    JS_FreeAtom(ctx, props[k].atom);
                }
                js_free(ctx, props);
            }
        }
    }

    JS_SetPropertyStr(ctx, result, "status", JS_NewInt32(ctx, status));
    JSValue textValue = jsString(ctx, text);
    JS_SetPropertyStr(ctx, result, "html", JS_DupValue(ctx, textValue));
    JS_SetPropertyStr(ctx, result, "text", textValue);
    JS_SetPropertyStr(ctx, result, "url", jsString(ctx, finalUrl));
    JS_SetPropertyStr(ctx, result, "json", JS_DupValue(ctx, json));
    if (!JS_IsArray(json))
        JS_FreeValue(ctx, json);
    return result;
}

void JsWorker::Impl::notifyTap(const PendingFetch &pending, int status, const QString &text, const QString &finalUrl,
                               const QString &transportError)
{
    if (!tap)
        return;
    JsWorker::FetchTapEvent ev;
    ev.requestUrl = pending.requestUrl;
    ev.opts = pending.opts;
    ev.status = status;
    ev.text = text;
    ev.finalUrl = finalUrl;
    ev.transportError = transportError;
    tap(ev);
}

void JsWorker::Impl::resolveFetch(quint64 id, int status, const QString &text, const QString &finalUrl)
{
    PendingFetch pending;
    if (!takeFetch(id, &pending))
        return;
    notifyTap(pending, status, text, finalUrl, {});
    JSContext *ctx = pending.ctx;
    JSValue result = buildFetchResult(ctx, status, text, finalUrl);
    armSlice();
    JS_FreeValue(ctx, JS_Call(ctx, pending.resolve, JS_UNDEFINED, 1, &result));
    disarmSlice();
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, pending.resolve);
    JS_FreeValue(ctx, pending.reject);
    pump();
}

void JsWorker::Impl::failFetch(quint64 id, const QString &message)
{
    PendingFetch pending;
    if (!takeFetch(id, &pending))
        return;
    notifyTap(pending, 0, {}, {}, message);
    JSContext *ctx = pending.ctx;
    JSValue error = JS_NewError(ctx);
    JS_SetPropertyStr(ctx, error, "message", jsString(ctx, message));
    armSlice();
    JS_FreeValue(ctx, JS_Call(ctx, pending.reject, JS_UNDEFINED, 1, &error));
    disarmSlice();
    JS_FreeValue(ctx, error);
    JS_FreeValue(ctx, pending.resolve);
    JS_FreeValue(ctx, pending.reject);
    pump();
}

JsResult JsWorker::Impl::evalSync(const QString &code)
{
    JsResult res;
    if (!scratch) {
        res.errorText = QStringLiteral("[load] the JavaScript worker is not initialised");
        return res;
    }
    const QByteArray utf8 = code.toUtf8();
    armSlice();
    JSValue val = JS_Eval(scratch, utf8.constData(), size_t(utf8.size()), "<eval>", JS_EVAL_TYPE_GLOBAL);
    disarmSlice();
    pump();
    if (JS_IsException(val)) {
        res.errorText = takeExceptionText(scratch);
        return res;
    }
    res.ok = true;
    res.value = jsValueToVariant(scratch, val);
    JS_FreeValue(scratch, val);
    return res;
}

// ===============================================================================================================
// JsWorker
// ===============================================================================================================

JsWorker::JsWorker(QObject *parent) : QObject(parent), d(std::make_unique<Impl>(this)) {}

JsWorker::~JsWorker() = default;

void JsWorker::init()
{
    d->init();
}

void JsWorker::invokeScript(quint64 callId, const QString &extensionId, const QString &code, const QString &method,
                            const QString &argsJson)
{
    d->invoke(callId, extensionId, code, method, argsJson);
}

void JsWorker::setLimits(int sliceMs, int callMs)
{
    d->sliceMs = qMax(50, sliceMs);
    d->callMs = qMax(50, callMs);
}

void JsWorker::setFetchInterceptor(FetchInterceptor interceptor)
{
    d->interceptor = std::move(interceptor);
}

void JsWorker::setFetchTap(FetchTap tap)
{
    d->tap = std::move(tap);
}

JsResult JsWorker::evalSync(const QString &code)
{
    return d->evalSync(code);
}

// ===============================================================================================================
// JsWorkerThread
// ===============================================================================================================

JsWorkerThread::JsWorkerThread(QObject *parent) : QObject(parent)
{
    m_thread = new QThread(this);
    m_thread->setObjectName(QStringLiteral("manga-dl js"));
    m_worker = new JsWorker;
    m_worker->moveToThread(m_thread);
    QObject::connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread->start();
    // Initialise before anyone can queue a call, so a call can never reach an uninitialised worker.
    QMetaObject::invokeMethod(m_worker, "init", Qt::BlockingQueuedConnection);
}

JsWorkerThread::~JsWorkerThread()
{
    m_thread->quit();
    if (!m_thread->wait(10000))
        qWarning("[JsWorkerThread] the JavaScript thread did not stop within 10 s");
}
