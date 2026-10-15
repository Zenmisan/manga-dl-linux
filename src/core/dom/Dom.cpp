#include "dom/Dom.h"

#include <QSet>

namespace dom {

namespace {

QSet<QString> makeSet(std::initializer_list<const char *> names)
{
    QSet<QString> set;
    for (const char *n : names)
        set.insert(QString::fromLatin1(n));
    return set;
}

// Parent elements whose text children are written without escaping.
bool isRawTextParent(const QString &name)
{
    static const QSet<QString> raw = makeSet({"style", "script", "xmp", "iframe", "noembed", "noframes", "plaintext"});
    return raw.contains(name);
}

constexpr int kMaxSerializeDepth = 2000;

void appendEscaped(const QString &s, QString &out, bool attribute)
{
    for (const QChar ch : s) {
        const ushort u = ch.unicode();
        if (u == '&')
            out += QLatin1String("&amp;");
        else if (u == 0xA0)
            out += QLatin1String("&nbsp;");
        else if (attribute && u == '"')
            out += QLatin1String("&quot;");
        else if (!attribute && u == '<')
            out += QLatin1String("&lt;");
        else if (!attribute && u == '>')
            out += QLatin1String("&gt;");
        else
            out += ch;
    }
}

} // namespace

bool isVoidElement(const QString &lowerName)
{
    static const QSet<QString> voids = makeSet({"area", "base", "br", "col", "embed", "hr", "img", "input", "link",
                                                "meta", "param", "source", "track", "wbr", "basefont", "bgsound",
                                                "frame", "keygen", "command"});
    return voids.contains(lowerName);
}

Document::Document()
{
    Node root;
    root.type = NodeType::Document;
    m_nodes.append(root);
}

NodeId Document::createElement(const QString &lowerName)
{
    Node n;
    n.type = NodeType::Element;
    n.name = lowerName;
    m_nodes.append(std::move(n));
    return NodeId(m_nodes.size() - 1);
}

NodeId Document::createText(const QString &text)
{
    Node n;
    n.type = NodeType::Text;
    n.text = text;
    m_nodes.append(std::move(n));
    return NodeId(m_nodes.size() - 1);
}

NodeId Document::createComment(const QString &text)
{
    Node n;
    n.type = NodeType::Comment;
    n.text = text;
    m_nodes.append(std::move(n));
    return NodeId(m_nodes.size() - 1);
}

void Document::appendChild(NodeId parent, NodeId child)
{
    if (!valid(parent) || !valid(child) || parent == child || child == root())
        return;
    // Refuse to make a node its own ancestor.
    for (NodeId a = parent; a != kNone; a = m_nodes[a].parent) {
        if (a == child)
            return;
    }
    detach(child);

    const NodeId last = m_nodes[parent].lastChild;
    m_nodes[child].parent = parent;
    m_nodes[child].prev = last;
    m_nodes[child].next = kNone;
    if (last != kNone)
        m_nodes[last].next = child;
    else
        m_nodes[parent].firstChild = child;
    m_nodes[parent].lastChild = child;
}

void Document::detach(NodeId id)
{
    if (!valid(id) || id == root())
        return;
    const NodeId parent = m_nodes[id].parent;
    if (parent == kNone)
        return;
    const NodeId prev = m_nodes[id].prev;
    const NodeId next = m_nodes[id].next;
    if (prev != kNone)
        m_nodes[prev].next = next;
    else
        m_nodes[parent].firstChild = next;
    if (next != kNone)
        m_nodes[next].prev = prev;
    else
        m_nodes[parent].lastChild = prev;
    m_nodes[id].parent = kNone;
    m_nodes[id].prev = kNone;
    m_nodes[id].next = kNone;
}

void Document::appendToText(NodeId textNode, const QString &more)
{
    if (valid(textNode) && m_nodes[textNode].type == NodeType::Text)
        m_nodes[textNode].text += more;
}

const QString *Document::attrPtr(NodeId id, const QString &name) const
{
    if (!valid(id))
        return nullptr;
    const Node &n = m_nodes[id];
    for (const Attribute &a : n.attrs) {
        if (a.name == name)
            return &a.value;
    }
    return nullptr;
}

QString Document::attr(NodeId id, const QString &name) const
{
    const QString *v = attrPtr(id, name);
    return v ? *v : QString();
}

void Document::setAttr(NodeId id, const QString &name, const QString &value)
{
    if (!valid(id) || m_nodes[id].type != NodeType::Element)
        return;
    for (Attribute &a : m_nodes[id].attrs) {
        if (a.name == name) {
            a.value = value;
            return;
        }
    }
    m_nodes[id].attrs.append(Attribute{name, value});
}

void Document::removeAttr(NodeId id, const QString &name)
{
    if (!valid(id))
        return;
    QVector<Attribute> &attrs = m_nodes[id].attrs;
    for (int i = 0; i < attrs.size(); ++i) {
        if (attrs[i].name == name) {
            attrs.removeAt(i);
            return;
        }
    }
}

NodeId Document::parentElement(NodeId id) const
{
    if (!valid(id))
        return kNone;
    const NodeId p = m_nodes[id].parent;
    return (p != kNone && m_nodes[p].type == NodeType::Element) ? p : kNone;
}

QVector<NodeId> Document::children(NodeId id) const
{
    QVector<NodeId> out;
    if (!valid(id))
        return out;
    for (NodeId c = m_nodes[id].firstChild; c != kNone; c = m_nodes[c].next)
        out.append(c);
    return out;
}

QVector<NodeId> Document::elementChildren(NodeId id) const
{
    QVector<NodeId> out;
    if (!valid(id))
        return out;
    for (NodeId c = m_nodes[id].firstChild; c != kNone; c = m_nodes[c].next) {
        if (m_nodes[c].type == NodeType::Element)
            out.append(c);
    }
    return out;
}

NodeId Document::firstElementChild(NodeId id) const
{
    if (!valid(id))
        return kNone;
    for (NodeId c = m_nodes[id].firstChild; c != kNone; c = m_nodes[c].next) {
        if (m_nodes[c].type == NodeType::Element)
            return c;
    }
    return kNone;
}

NodeId Document::lastElementChild(NodeId id) const
{
    if (!valid(id))
        return kNone;
    for (NodeId c = m_nodes[id].lastChild; c != kNone; c = m_nodes[c].prev) {
        if (m_nodes[c].type == NodeType::Element)
            return c;
    }
    return kNone;
}

NodeId Document::nextElementSibling(NodeId id) const
{
    if (!valid(id))
        return kNone;
    for (NodeId s = m_nodes[id].next; s != kNone; s = m_nodes[s].next) {
        if (m_nodes[s].type == NodeType::Element)
            return s;
    }
    return kNone;
}

NodeId Document::prevElementSibling(NodeId id) const
{
    if (!valid(id))
        return kNone;
    for (NodeId s = m_nodes[id].prev; s != kNone; s = m_nodes[s].prev) {
        if (m_nodes[s].type == NodeType::Element)
            return s;
    }
    return kNone;
}

NodeId Document::nextPreorder(NodeId subtreeRoot, NodeId cur) const
{
    if (!valid(cur))
        return kNone;
    if (m_nodes[cur].firstChild != kNone)
        return m_nodes[cur].firstChild;
    while (cur != subtreeRoot && cur != kNone) {
        if (m_nodes[cur].next != kNone)
            return m_nodes[cur].next;
        cur = m_nodes[cur].parent;
    }
    return kNone;
}

NodeId Document::childNamed(NodeId parent, const QString &lowerName) const
{
    if (!valid(parent))
        return kNone;
    for (NodeId c = m_nodes[parent].firstChild; c != kNone; c = m_nodes[c].next) {
        if (m_nodes[c].type == NodeType::Element && m_nodes[c].name == lowerName)
            return c;
    }
    return kNone;
}

NodeId Document::documentElement() const
{
    return childNamed(root(), QStringLiteral("html"));
}

NodeId Document::head() const
{
    return childNamed(documentElement(), QStringLiteral("head"));
}

NodeId Document::body() const
{
    return childNamed(documentElement(), QStringLiteral("body"));
}

NodeId Document::findById(const QString &id) const
{
    if (id.isEmpty())
        return kNone;
    const QString idAttr = QStringLiteral("id");
    for (NodeId c = nextPreorder(root(), root()); c != kNone; c = nextPreorder(root(), c)) {
        if (m_nodes[c].type != NodeType::Element)
            continue;
        const QString *v = attrPtr(c, idAttr);
        if (v && *v == id)
            return c;
    }
    return kNone;
}

QVector<NodeId> Document::elementsByTagName(NodeId scope, const QString &lowerName) const
{
    QVector<NodeId> out;
    if (!valid(scope))
        return out;
    const bool any = lowerName == QLatin1String("*");
    for (NodeId c = nextPreorder(scope, scope); c != kNone; c = nextPreorder(scope, c)) {
        if (m_nodes[c].type == NodeType::Element && (any || m_nodes[c].name == lowerName))
            out.append(c);
    }
    return out;
}

QString Document::textContent(NodeId id) const
{
    if (!valid(id))
        return QString();
    const Node &n = m_nodes[id];
    if (n.type == NodeType::Text || n.type == NodeType::Comment)
        return n.text;
    QString out;
    for (NodeId c = nextPreorder(id, id); c != kNone; c = nextPreorder(id, c)) {
        if (m_nodes[c].type == NodeType::Text)
            out += m_nodes[c].text;
    }
    return out;
}

void Document::writeNode(NodeId id, QString &out, int depth) const
{
    if (depth > kMaxSerializeDepth)
        return;
    const Node &n = m_nodes[id];
    switch (n.type) {
    case NodeType::Text: {
        const bool raw = n.parent != kNone && isRawTextParent(m_nodes[n.parent].name);
        if (raw)
            out += n.text;
        else
            appendEscaped(n.text, out, false);
        break;
    }
    case NodeType::Comment:
        out += QLatin1String("<!--");
        out += n.text;
        out += QLatin1String("-->");
        break;
    case NodeType::Element: {
        out += QLatin1Char('<');
        out += n.name;
        for (const Attribute &a : n.attrs) {
            out += QLatin1Char(' ');
            out += a.name;
            out += QLatin1String("=\"");
            appendEscaped(a.value, out, true);
            out += QLatin1Char('"');
        }
        out += QLatin1Char('>');
        if (isVoidElement(n.name))
            break;
        for (NodeId c = n.firstChild; c != kNone; c = m_nodes[c].next)
            writeNode(c, out, depth + 1);
        out += QLatin1String("</");
        out += n.name;
        out += QLatin1Char('>');
        break;
    }
    case NodeType::Document:
        for (NodeId c = n.firstChild; c != kNone; c = m_nodes[c].next)
            writeNode(c, out, depth + 1);
        break;
    }
}

QString Document::innerHtml(NodeId id) const
{
    QString out;
    if (!valid(id))
        return out;
    const Node &n = m_nodes[id];
    if (n.type != NodeType::Element && n.type != NodeType::Document)
        return out;
    for (NodeId c = n.firstChild; c != kNone; c = m_nodes[c].next)
        writeNode(c, out, 1);
    return out;
}

QString Document::outerHtml(NodeId id) const
{
    QString out;
    if (valid(id))
        writeNode(id, out, 0);
    return out;
}

} // namespace dom
