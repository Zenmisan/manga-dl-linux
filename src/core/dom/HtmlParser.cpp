#include "dom/HtmlParser.h"

#include <QHash>
#include <QSet>
#include <QStringView>

#include <cstring>

namespace dom {

namespace {

// ---------------------------------------------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------------------------------------------

constexpr int kMaxDepth = 400;  // deeper start tags become siblings instead of children (bounds recursion elsewhere)

bool isWs(QChar c)
{
    const ushort u = c.unicode();
    return u == ' ' || u == '\t' || u == '\n' || u == '\r' || u == '\f';
}

bool isAsciiLetter(QChar c)
{
    const ushort u = c.unicode();
    return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z');
}

bool isAsciiAlnum(QChar c)
{
    const ushort u = c.unicode();
    return isAsciiLetter(c) || (u >= '0' && u <= '9');
}

bool isAllWs(const QString &s)
{
    for (const QChar c : s) {
        if (!isWs(c))
            return false;
    }
    return true;
}

QSet<QString> makeSet(std::initializer_list<const char *> names)
{
    QSet<QString> set;
    for (const char *n : names)
        set.insert(QString::fromLatin1(n));
    return set;
}

bool isHeading(const QString &name)
{
    return name.size() == 2 && name[0] == 'h' && name[1] >= '1' && name[1] <= '6';
}

// ---------------------------------------------------------------------------------------------------------------
// Character references
// ---------------------------------------------------------------------------------------------------------------

QString fromCodePoint(uint cp)
{
    QString s;
    if (cp >= 0x10000) {
        s += QChar(QChar::highSurrogate(cp));
        s += QChar(QChar::lowSurrogate(cp));
    } else {
        s += QChar(ushort(cp));
    }
    return s;
}

const char *const kLatin1Names[96] = {
    "nbsp",  "iexcl", "cent",  "pound", "curren", "yen",   "brvbar", "sect",  "uml",   "copy",  "ordf",  "laquo",
    "not",   "shy",   "reg",   "macr",  "deg",    "plusmn", "sup2",  "sup3",  "acute", "micro", "para",  "middot",
    "cedil", "sup1",  "ordm",  "raquo", "frac14", "frac12", "frac34", "iquest", "Agrave", "Aacute", "Acirc", "Atilde",
    "Auml",  "Aring", "AElig", "Ccedil", "Egrave", "Eacute", "Ecirc", "Euml",  "Igrave", "Iacute", "Icirc", "Iuml",
    "ETH",   "Ntilde", "Ograve", "Oacute", "Ocirc", "Otilde", "Ouml", "times", "Oslash", "Ugrave", "Uacute", "Ucirc",
    "Uuml",  "Yacute", "THORN", "szlig", "agrave", "aacute", "acirc", "atilde", "auml",  "aring", "aelig", "ccedil",
    "egrave", "eacute", "ecirc", "euml", "igrave", "iacute", "icirc", "iuml",  "eth",   "ntilde", "ograve", "oacute",
    "ocirc", "otilde", "ouml", "divide", "oslash", "ugrave", "uacute", "ucirc", "uuml",  "yacute", "thorn", "yuml"};

struct NamedEntity {
    const char *name;
    uint cp;
};

const NamedEntity kOtherEntities[] = {
    {"amp", 38},        {"AMP", 38},        {"lt", 60},         {"LT", 60},         {"gt", 62},
    {"GT", 62},         {"quot", 34},       {"QUOT", 34},       {"apos", 39},       {"OElig", 338},
    {"oelig", 339},     {"Scaron", 352},    {"scaron", 353},    {"Yuml", 376},      {"fnof", 402},
    {"circ", 710},      {"tilde", 732},     {"ensp", 8194},     {"emsp", 8195},     {"thinsp", 8201},
    {"zwnj", 8204},     {"zwj", 8205},      {"lrm", 8206},      {"rlm", 8207},      {"hyphen", 8208},
    {"dash", 8208},     {"ndash", 8211},    {"mdash", 8212},    {"horbar", 8213},   {"lsquo", 8216},
    {"rsquo", 8217},    {"sbquo", 8218},    {"ldquo", 8220},    {"rdquo", 8221},    {"bdquo", 8222},
    {"dagger", 8224},   {"Dagger", 8225},   {"bull", 8226},     {"hellip", 8230},   {"permil", 8240},
    {"prime", 8242},    {"Prime", 8243},    {"lsaquo", 8249},   {"rsaquo", 8250},   {"oline", 8254},
    {"frasl", 8260},    {"euro", 8364},     {"trade", 8482},    {"TRADE", 8482},    {"larr", 8592},
    {"uarr", 8593},     {"rarr", 8594},     {"darr", 8595},     {"harr", 8596},     {"lArr", 8656},
    {"rArr", 8658},     {"hArr", 8660},     {"forall", 8704},   {"part", 8706},     {"exist", 8707},
    {"empty", 8709},    {"nabla", 8711},    {"isin", 8712},     {"notin", 8713},    {"ni", 8715},
    {"prod", 8719},     {"sum", 8721},      {"minus", 8722},    {"lowast", 8727},   {"radic", 8730},
    {"prop", 8733},     {"infin", 8734},    {"ang", 8736},      {"and", 8743},      {"or", 8744},
    {"cap", 8745},      {"cup", 8746},      {"int", 8747},      {"there4", 8756},   {"sim", 8764},
    {"cong", 8773},     {"asymp", 8776},    {"ne", 8800},       {"equiv", 8801},    {"le", 8804},
    {"ge", 8805},       {"sub", 8834},      {"sup", 8835},      {"nsub", 8836},     {"sube", 8838},
    {"supe", 8839},     {"oplus", 8853},    {"otimes", 8855},   {"perp", 8869},     {"sdot", 8901},
    {"loz", 9674},      {"spades", 9824},   {"clubs", 9827},    {"hearts", 9829},   {"diams", 9830},
    {"star", 9734},     {"starf", 9733},    {"check", 10003},   {"cross", 10007},   {"half", 189},
    {"Tab", 9},         {"NewLine", 10},    {"excl", 33},       {"num", 35},        {"dollar", 36},
    {"percnt", 37},     {"lpar", 40},       {"rpar", 41},       {"ast", 42},        {"plus", 43},
    {"comma", 44},      {"period", 46},     {"sol", 47},        {"colon", 58},      {"semi", 59},
    {"equals", 61},     {"quest", 63},      {"commat", 64},     {"lsqb", 91},       {"lbrack", 91},
    {"bsol", 92},       {"rsqb", 93},       {"rbrack", 93},     {"lowbar", 95},     {"grave", 96},
    {"lcub", 123},      {"lbrace", 123},    {"verbar", 124},    {"vert", 124},      {"rcub", 125},
    {"rbrace", 125},    {"Alpha", 913},     {"Beta", 914},      {"Gamma", 915},     {"Delta", 916},
    {"Omega", 937},     {"alpha", 945},     {"beta", 946},      {"gamma", 947},     {"delta", 948},
    {"epsilon", 949},   {"theta", 952},     {"lambda", 955},    {"mu", 956},        {"pi", 960},
    {"sigma", 963},     {"tau", 964},       {"phi", 966},       {"omega", 969},
};

const QHash<QString, QString> &entityTable()
{
    static const QHash<QString, QString> table = [] {
        QHash<QString, QString> t;
        for (int k = 0; k < 96; ++k)
            t.insert(QString::fromLatin1(kLatin1Names[k]), fromCodePoint(uint(160 + k)));
        for (const NamedEntity &e : kOtherEntities)
            t.insert(QString::fromLatin1(e.name), fromCodePoint(e.cp));
        return t;
    }();
    return table;
}

// Names that browsers also accept without the trailing semicolon.
const QSet<QString> &legacyEntityNames()
{
    static const QSet<QString> legacy = [] {
        QSet<QString> s;
        for (int k = 0; k < 96; ++k)
            s.insert(QString::fromLatin1(kLatin1Names[k]));
        for (const char *n : {"amp", "AMP", "lt", "LT", "gt", "GT", "quot", "QUOT"})
            s.insert(QString::fromLatin1(n));
        return s;
    }();
    return legacy;
}

uint fixNumericReference(uint cp)
{
    static const ushort cp1252[32] = {0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
                                      0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
                                      0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
                                      0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178};
    if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        return 0xFFFD;
    if (cp >= 0x80 && cp <= 0x9F)
        return cp1252[cp - 0x80];
    return cp;
}

int digitValue(QChar c, bool hex)
{
    const ushort u = c.unicode();
    if (u >= '0' && u <= '9')
        return int(u - '0');
    if (hex) {
        if (u >= 'a' && u <= 'f')
            return int(u - 'a') + 10;
        if (u >= 'A' && u <= 'F')
            return int(u - 'A') + 10;
    }
    return -1;
}

QString decodeEntitiesImpl(const QString &in, bool inAttribute)
{
    if (!in.contains(QLatin1Char('&')))
        return in;

    QString out;
    out.reserve(in.size());
    const int n = int(in.size());
    int i = 0;
    while (i < n) {
        const QChar c = in[i];
        if (c != '&') {
            out += c;
            ++i;
            continue;
        }

        int j = i + 1;
        if (j < n && in[j] == '#') {
            ++j;
            bool hex = false;
            if (j < n && (in[j] == 'x' || in[j] == 'X')) {
                hex = true;
                ++j;
            }
            const int digitsStart = j;
            uint cp = 0;
            while (j < n && digitValue(in[j], hex) >= 0) {
                cp = cp * (hex ? 16u : 10u) + uint(digitValue(in[j], hex));
                if (cp > 0x10FFFF)
                    cp = 0x110000;  // clamp: stays "too big" without overflowing
                ++j;
            }
            if (j == digitsStart) {  // "&#" or "&#x" with no digits: literal text
                out += c;
                ++i;
                continue;
            }
            if (j < n && in[j] == ';')
                ++j;
            out += fromCodePoint(fixNumericReference(cp));
            i = j;
            continue;
        }

        const int nameStart = j;
        while (j < n && isAsciiAlnum(in[j]))
            ++j;
        const QString name = in.mid(nameStart, j - nameStart);
        const bool semicolon = j < n && in[j] == ';';
        if (!name.isEmpty()) {
            const auto it = entityTable().constFind(name);
            if (it != entityTable().constEnd()) {
                if (semicolon) {
                    out += it.value();
                    i = j + 1;
                    continue;
                }
                // No semicolon: only the legacy names, and in attribute values not before '=' (so that
                // "?a=1&copy=2" stays a query string).
                const bool nextIsEquals = j < n && in[j] == '=';
                if (legacyEntityNames().contains(name) && !(inAttribute && nextIsEquals)) {
                    out += it.value();
                    i = j;
                    continue;
                }
            }
        }
        out += c;  // not a reference: keep the ampersand as text
        ++i;
    }
    return out;
}

// ---------------------------------------------------------------------------------------------------------------
// Element classification
// ---------------------------------------------------------------------------------------------------------------

const QSet<QString> &headTags()
{
    static const QSet<QString> s = makeSet({"title", "meta", "link", "base", "script", "style", "template", "basefont",
                                            "bgsound", "command"});
    return s;
}

const QSet<QString> &blockStartTags()
{
    static const QSet<QString> s = makeSet({"address", "article", "aside", "blockquote", "center", "details", "dialog",
                                            "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "form",
                                            "h1", "h2", "h3", "h4", "h5", "h6", "header", "hgroup", "hr", "main",
                                            "menu", "nav", "ol", "p", "pre", "section", "summary", "table", "ul",
                                            "listing", "xmp"});
    return s;
}

// The HTML "special" category: used to decide how far an implied </li> or </dd> may reach.
const QSet<QString> &specialTags()
{
    static const QSet<QString> s = makeSet(
        {"address", "applet", "area", "article", "aside", "base", "basefont", "bgsound", "blockquote", "body", "br",
         "button", "caption", "center", "col", "colgroup", "dd", "details", "dir", "div", "dl", "dt", "embed",
         "fieldset", "figcaption", "figure", "footer", "form", "frame", "frameset", "h1", "h2", "h3", "h4", "h5",
         "h6", "head", "header", "hgroup", "hr", "html", "iframe", "img", "input", "keygen", "li", "link", "listing",
         "main", "marquee", "menu", "meta", "nav", "noembed", "noframes", "noscript", "object", "ol", "p", "param",
         "plaintext", "pre", "script", "section", "select", "source", "style", "summary", "table", "tbody", "td",
         "template", "textarea", "tfoot", "th", "thead", "title", "tr", "track", "ul", "wbr", "xmp"});
    return s;
}

const QSet<QString> &scopeBoundaryTags()
{
    static const QSet<QString> s = makeSet({"html", "table", "td", "th", "caption", "marquee", "object", "applet",
                                            "template"});
    return s;
}

const QSet<QString> &rawTextTags()
{
    static const QSet<QString> s = makeSet({"script", "style", "textarea", "title", "iframe", "xmp", "noembed",
                                            "noframes"});
    return s;
}

// ---------------------------------------------------------------------------------------------------------------
// Tree builder
// ---------------------------------------------------------------------------------------------------------------

class Builder
{
public:
    explicit Builder(const QString &src) : s(src), n(int(src.size()))
    {
        htmlEl = doc->createElement(QStringLiteral("html"));
        doc->appendChild(doc->root(), htmlEl);
        headEl = doc->createElement(QStringLiteral("head"));
        doc->appendChild(htmlEl, headEl);
        stack.append(htmlEl);
        stack.append(headEl);
    }

    std::unique_ptr<Document> run()
    {
        while (i < n) {
            if (s[i] != '<') {
                int j = int(s.indexOf(QLatin1Char('<'), i));
                if (j < 0)
                    j = n;
                addText(decodeEntitiesImpl(s.mid(i, j - i), false));
                i = j;
            } else {
                parseMarkup();
            }
        }
        switchToBody();  // every document has a body, even if the input only had <head> content
        return std::move(doc);
    }

private:
    enum class Mode { InHead, InBody };

    const QString &s;
    int n;
    int i = 0;
    std::unique_ptr<Document> doc = std::make_unique<Document>();
    QVector<NodeId> stack;  // open elements; [0] is <html>, [1] is <head> or <body>
    NodeId htmlEl = kNone;
    NodeId headEl = kNone;
    NodeId bodyEl = kNone;
    Mode mode = Mode::InHead;

    // --- input helpers -----------------------------------------------------------------------------------------
    bool lookingAt(const char *literal, bool caseInsensitive = false) const
    {
        const int len = int(std::strlen(literal));
        if (i + len > n)
            return false;
        for (int k = 0; k < len; ++k) {
            QChar a = s[i + k];
            QChar b = QLatin1Char(literal[k]);
            if (caseInsensitive) {
                a = a.toLower();
                b = b.toLower();
            }
            if (a != b)
                return false;
        }
        return true;
    }

    void skipWs()
    {
        while (i < n && isWs(s[i]))
            ++i;
    }

    // --- markup ------------------------------------------------------------------------------------------------
    void parseMarkup()
    {
        const QChar c1 = (i + 1 < n) ? s[i + 1] : QChar();
        if (lookingAt("<!--"))
            parseComment();
        else if (c1 == '!')
            parseDeclaration();
        else if (c1 == '?')
            parseBogusComment(i + 1);
        else if (c1 == '/')
            parseEndTag();
        else if (isAsciiLetter(c1))
            parseStartTag();
        else {  // a lone '<' is just text
            addText(QStringLiteral("<"));
            ++i;
        }
    }

    void parseComment()
    {
        const int start = i + 4;
        QString data;
        if (start < n && s[start] == '>') {  // "<!-->"
            i = start + 1;
        } else if (start + 1 < n && s[start] == '-' && s[start + 1] == '>') {  // "<!--->"
            i = start + 2;
        } else {
            const int end = int(s.indexOf(QLatin1String("-->"), start));
            if (end < 0) {
                data = s.mid(start);
                i = n;
            } else {
                data = s.mid(start, end - start);
                i = end + 3;
            }
        }
        addComment(data);
    }

    void parseDeclaration()
    {
        if (lookingAt("<!doctype", true)) {
            const int gt = int(s.indexOf(QLatin1Char('>'), i));
            i = gt < 0 ? n : gt + 1;
        } else {
            parseBogusComment(i + 2);  // <![CDATA[ ... ]]> and friends
        }
    }

    void parseBogusComment(int dataStart)
    {
        const int gt = int(s.indexOf(QLatin1Char('>'), i));
        const int end = gt < 0 ? n : gt;
        const QString data = dataStart < end ? s.mid(dataStart, end - dataStart) : QString();
        i = gt < 0 ? n : gt + 1;
        addComment(data);
    }

    void parseEndTag()
    {
        const int nameStart = i + 2;
        if (nameStart < n && isAsciiLetter(s[nameStart])) {
            int k = nameStart;
            while (k < n && !isWs(s[k]) && s[k] != '/' && s[k] != '>')
                ++k;
            const QString name = s.mid(nameStart, k - nameStart).toLower();
            const int gt = int(s.indexOf(QLatin1Char('>'), k));
            i = gt < 0 ? n : gt + 1;
            endTag(name);
        } else if (nameStart < n && s[nameStart] == '>') {  // "</>" is ignored
            i = nameStart + 1;
        } else {
            parseBogusComment(i + 2);
        }
    }

    void parseStartTag()
    {
        const int nameStart = i + 1;
        int k = nameStart;
        while (k < n && !isWs(s[k]) && s[k] != '/' && s[k] != '>')
            ++k;
        const QString name = s.mid(nameStart, k - nameStart).toLower();
        i = k;

        QVector<Attribute> attrs;
        bool selfClosing = false;
        bool terminated = false;
        while (i < n) {
            skipWs();
            if (i >= n)
                break;
            if (s[i] == '>') {
                ++i;
                terminated = true;
                break;
            }
            if (s[i] == '/') {
                if (i + 1 < n && s[i + 1] == '>') {
                    selfClosing = true;
                    i += 2;
                    terminated = true;
                    break;
                }
                ++i;
                continue;
            }

            const int attrStart = i;
            if (s[i] == '=')
                ++i;  // a leading '=' belongs to the name, as in the HTML spec
            while (i < n && !isWs(s[i]) && s[i] != '/' && s[i] != '>' && s[i] != '=')
                ++i;
            const QString attrName = s.mid(attrStart, i - attrStart).toLower();
            skipWs();

            QString value;
            if (i < n && s[i] == '=') {
                ++i;
                skipWs();
                if (i < n && (s[i] == '"' || s[i] == '\'')) {
                    const QChar quote = s[i];
                    const int valueStart = i + 1;
                    const int valueEnd = int(s.indexOf(quote, valueStart));
                    if (valueEnd < 0) {  // end of input inside a quoted value: the tag is dropped
                        i = n;
                        break;
                    }
                    value = decodeEntitiesImpl(s.mid(valueStart, valueEnd - valueStart), true);
                    i = valueEnd + 1;
                } else {
                    const int valueStart = i;
                    while (i < n && !isWs(s[i]) && s[i] != '>')
                        ++i;
                    value = decodeEntitiesImpl(s.mid(valueStart, i - valueStart), true);
                }
            }

            if (attrName.isEmpty())
                continue;
            bool duplicate = false;
            for (const Attribute &a : attrs) {
                if (a.name == attrName) {
                    duplicate = true;  // the first occurrence wins
                    break;
                }
            }
            if (!duplicate)
                attrs.append(Attribute{attrName, value});
        }
        if (!terminated)
            return;  // end of input inside a tag: dropped, like browsers do

        startTag(name, attrs, selfClosing);
        if (rawTextTags().contains(name))
            readRawText(name);
    }

    // Reads everything up to the matching end tag as one text node (script, style, textarea, title, ...).
    void readRawText(const QString &name)
    {
        const int start = i;
        const int end = findRawEnd(name, start);
        QString data = s.mid(start, (end < 0 ? n : end) - start);
        if (name == QLatin1String("textarea") || name == QLatin1String("title"))
            data = decodeEntitiesImpl(data, false);
        if (name == QLatin1String("textarea")) {  // a newline right after <textarea> is not content
            if (data.startsWith(QLatin1String("\r\n")))
                data.remove(0, 2);
            else if (data.startsWith(QLatin1Char('\n')))
                data.remove(0, 1);
        }
        if (!data.isEmpty())
            appendTextTo(stack.last(), data);

        if (end < 0) {
            i = n;
        } else {
            const int gt = int(s.indexOf(QLatin1Char('>'), end));
            i = gt < 0 ? n : gt + 1;
        }
        endTag(name);
    }

    int findRawEnd(const QString &name, int from) const
    {
        const int len = int(name.size());
        int p = from;
        for (;;) {
            const int lt = int(s.indexOf(QLatin1Char('<'), p));
            if (lt < 0)
                return -1;
            if (lt + 1 < n && s[lt + 1] == '/' && lt + 2 + len <= n
                && QStringView(s).mid(lt + 2, len).compare(name, Qt::CaseInsensitive) == 0) {
                const int after = lt + 2 + len;
                if (after >= n || isWs(s[after]) || s[after] == '/' || s[after] == '>')
                    return lt;
            }
            p = lt + 1;
        }
    }

    // --- tree building -----------------------------------------------------------------------------------------
    const QString &nameOf(NodeId id) const { return doc->node(id).name; }

    void addComment(const QString &data) { doc->appendChild(stack.last(), doc->createComment(data)); }

    void addText(const QString &text)
    {
        if (text.isEmpty())
            return;
        if (mode == Mode::InHead && stack.last() == headEl) {
            if (isAllWs(text))
                return;  // layout whitespace between head tags
            switchToBody();
        }
        appendTextTo(stack.last(), text);
    }

    void appendTextTo(NodeId parent, const QString &text)
    {
        const NodeId last = doc->node(parent).lastChild;
        if (last != kNone && doc->node(last).type == NodeType::Text)
            doc->appendToText(last, text);
        else
            doc->appendChild(parent, doc->createText(text));
    }

    void popTo(int index)
    {
        if (index < 1)
            index = 1;
        if (index < stack.size())
            stack.resize(index);
    }

    void switchToBody()
    {
        if (mode == Mode::InBody)
            return;
        mode = Mode::InBody;
        stack.resize(1);  // keep <html>
        if (bodyEl == kNone) {
            bodyEl = doc->createElement(QStringLiteral("body"));
            doc->appendChild(htmlEl, bodyEl);
        }
        stack.append(bodyEl);
    }

    void mergeAttrs(NodeId el, const QVector<Attribute> &attrs)
    {
        for (const Attribute &a : attrs) {
            if (!doc->hasAttr(el, a.name))
                doc->setAttr(el, a.name, a.value);
        }
    }

    bool inForeignContent() const
    {
        for (const NodeId id : stack) {
            const QString &nm = nameOf(id);
            if (nm == QLatin1String("svg") || nm == QLatin1String("math"))
                return true;
        }
        return false;
    }

    NodeId insertElement(const QString &name, const QVector<Attribute> &attrs, bool selfClosing)
    {
        const NodeId e = doc->createElement(name);
        for (const Attribute &a : attrs)
            doc->setAttr(e, a.name, a.value);
        doc->appendChild(stack.last(), e);
        // "/>" only closes void and foreign (svg/math) elements; on a <div/> HTML ignores the slash.
        const bool closesItself = isVoidElement(name) || (selfClosing && inForeignContent());
        if (!closesItself && stack.size() < kMaxDepth)
            stack.append(e);
        return e;
    }

    void insertImplied(const QString &name)
    {
        const NodeId e = doc->createElement(name);
        doc->appendChild(stack.last(), e);
        if (stack.size() < kMaxDepth)
            stack.append(e);
    }

    // --- scope searches ----------------------------------------------------------------------------------------
    // Index in `stack` of the nearest open element called `name`, not looking past a scope boundary. -1 if none.
    int findInScope(const QString &name, const QSet<QString> *extraBoundaries = nullptr) const
    {
        for (int k = int(stack.size()) - 1; k >= 1; --k) {
            const QString &nm = nameOf(stack[k]);
            if (nm == name)
                return k;
            if (scopeBoundaryTags().contains(nm) || (extraBoundaries && extraBoundaries->contains(nm)))
                return -1;
        }
        return -1;
    }

    int findHeadingInScope() const
    {
        for (int k = int(stack.size()) - 1; k >= 1; --k) {
            const QString &nm = nameOf(stack[k]);
            if (isHeading(nm))
                return k;
            if (scopeBoundaryTags().contains(nm))
                return -1;
        }
        return -1;
    }

    // Pops everything above and including the nearest element whose name is in `targets`, giving up at a boundary.
    void popToNearest(const QSet<QString> &targets, const QSet<QString> &boundaries)
    {
        for (int k = int(stack.size()) - 1; k >= 1; --k) {
            const QString &nm = nameOf(stack[k]);
            if (targets.contains(nm)) {
                popTo(k);
                return;
            }
            if (boundaries.contains(nm))
                return;
        }
    }

    void closeParagraph()
    {
        static const QSet<QString> buttonScope = makeSet({"button"});
        const int k = findInScope(QStringLiteral("p"), &buttonScope);
        if (k >= 1)
            popTo(k);
    }

    // <li>, <dd>, <dt> close the previous item of the same kind, unless a "special" element is in between.
    void closeItem(const QString &a, const QString &b)
    {
        for (int k = int(stack.size()) - 1; k >= 1; --k) {
            const QString &nm = nameOf(stack[k]);
            if (nm == a || nm == b) {
                popTo(k);
                return;
            }
            if (specialTags().contains(nm) && nm != QLatin1String("address") && nm != QLatin1String("div")
                && nm != QLatin1String("p"))
                return;
        }
    }

    // Implied end tags and implied parents that apply before inserting <name> in the body.
    void applyImpliedEnds(const QString &name)
    {
        static const QSet<QString> tableBoundaries = makeSet({"table", "html", "body", "template"});
        static const QSet<QString> cellBoundaries = makeSet({"tr", "table", "html", "body", "template"});

        if (blockStartTags().contains(name))
            closeParagraph();
        if (isHeading(name) && isHeading(nameOf(stack.last())))
            popTo(int(stack.size()) - 1);

        if (name == QLatin1String("li")) {
            closeItem(QStringLiteral("li"), QStringLiteral("li"));
        } else if (name == QLatin1String("dd") || name == QLatin1String("dt")) {
            closeItem(QStringLiteral("dd"), QStringLiteral("dt"));
        } else if (name == QLatin1String("option")) {
            if (nameOf(stack.last()) == QLatin1String("option"))
                popTo(int(stack.size()) - 1);
        } else if (name == QLatin1String("optgroup")) {
            if (nameOf(stack.last()) == QLatin1String("option"))
                popTo(int(stack.size()) - 1);
            if (nameOf(stack.last()) == QLatin1String("optgroup"))
                popTo(int(stack.size()) - 1);
        } else if (name == QLatin1String("a")) {
            if (nameOf(stack.last()) == QLatin1String("a"))
                popTo(int(stack.size()) - 1);
        } else if (name == QLatin1String("tr")) {
            popToNearest(makeSet({"tr"}), tableBoundaries);
            if (nameOf(stack.last()) == QLatin1String("table"))
                insertImplied(QStringLiteral("tbody"));
        } else if (name == QLatin1String("td") || name == QLatin1String("th")) {
            popToNearest(makeSet({"td", "th"}), cellBoundaries);
            const QString top = nameOf(stack.last());  // a copy: insertImplied() grows the node array
            if (top == QLatin1String("table")) {
                insertImplied(QStringLiteral("tbody"));
                insertImplied(QStringLiteral("tr"));
            } else if (top == QLatin1String("tbody") || top == QLatin1String("thead") || top == QLatin1String("tfoot")) {
                insertImplied(QStringLiteral("tr"));
            }
        } else if (name == QLatin1String("tbody") || name == QLatin1String("thead") || name == QLatin1String("tfoot")) {
            popToNearest(makeSet({"tbody", "thead", "tfoot"}), tableBoundaries);
        }
    }

    void skipLeadingNewline()
    {
        if (i < n && s[i] == '\n')
            ++i;
        else if (i + 1 < n && s[i] == '\r' && s[i + 1] == '\n')
            i += 2;
    }

    void startTag(const QString &name, const QVector<Attribute> &attrs, bool selfClosing)
    {
        if (name == QLatin1String("html")) {
            mergeAttrs(htmlEl, attrs);
            return;
        }
        if (name == QLatin1String("head"))
            return;
        if (name == QLatin1String("body")) {
            switchToBody();
            mergeAttrs(bodyEl, attrs);
            return;
        }
        if (mode == Mode::InHead) {
            if (headTags().contains(name)) {
                insertElement(name, attrs, selfClosing);
                return;
            }
            switchToBody();
        }
        applyImpliedEnds(name);
        insertElement(name, attrs, selfClosing);
        if (name == QLatin1String("pre") || name == QLatin1String("listing"))
            skipLeadingNewline();
    }

    void endTag(const QString &name)
    {
        if (name == QLatin1String("head")) {
            if (mode == Mode::InHead)
                switchToBody();
            return;
        }
        if (name == QLatin1String("body") || name == QLatin1String("html"))
            return;  // content after </body> still belongs to the body
        if (name == QLatin1String("br")) {  // "</br>" means "<br>"
            startTag(QStringLiteral("br"), {}, false);
            return;
        }
        if (name == QLatin1String("p")) {
            static const QSet<QString> buttonScope = makeSet({"button"});
            const int k = findInScope(name, &buttonScope);
            if (k >= 1) {
                popTo(k);
            } else if (mode == Mode::InBody) {  // a stray "</p>" produces an empty paragraph
                doc->appendChild(stack.last(), doc->createElement(name));
            }
            return;
        }
        if (isHeading(name)) {
            const int k = findHeadingInScope();
            if (k >= 1)
                popTo(k);
            return;
        }
        static const QSet<QString> listBoundaries = makeSet({"ol", "ul"});
        const int k = findInScope(name, name == QLatin1String("li") ? &listBoundaries : nullptr);
        if (k >= 1)
            popTo(k);
    }
};

} // namespace

QString decodeHtmlEntities(const QString &text)
{
    return decodeEntitiesImpl(text, false);
}

std::unique_ptr<Document> parseHtml(const QString &html)
{
    Builder builder(html);
    return builder.run();
}

} // namespace dom
