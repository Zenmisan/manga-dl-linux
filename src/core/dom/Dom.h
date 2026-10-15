#pragma once

#include <QString>
#include <QVector>

namespace dom {

/// Index of a node inside its Document. Stable for the lifetime of the document, even after the node is removed.
using NodeId = int;
constexpr NodeId kNone = -1;

enum class NodeType : unsigned char { Document, Element, Text, Comment };

struct Attribute {
    QString name;   ///< lower-case
    QString value;
};

struct Node {
    NodeType type = NodeType::Element;
    QString name;              ///< Element: lower-case tag name. Other types: empty.
    QString text;              ///< Text / Comment: the (already entity-decoded) content.
    QVector<Attribute> attrs;  ///< Element only.
    NodeId parent = kNone;
    NodeId firstChild = kNone;
    NodeId lastChild = kNone;
    NodeId prev = kNone;
    NodeId next = kNone;
};

/// True for elements that never have content or an end tag (br, img, meta, ...).
bool isVoidElement(const QString &lowerName);

/// A parsed document: an arena of nodes linked into a tree. Node 0 is the Document node.
///
/// Removing a node only unlinks it. The node and its subtree stay readable (scripts commonly keep a reference to an
/// element after calling remove()), they just no longer belong to the document tree.
class Document
{
public:
    Document();

    NodeId root() const { return 0; }
    int size() const { return int(m_nodes.size()); }
    bool valid(NodeId id) const { return id >= 0 && id < m_nodes.size(); }
    const Node &node(NodeId id) const { return m_nodes[id]; }

    // --- creation ---------------------------------------------------------------------------------------------
    NodeId createElement(const QString &lowerName);
    NodeId createText(const QString &text);
    NodeId createComment(const QString &text);

    // --- tree edits -------------------------------------------------------------------------------------------
    /// Moves `child` to the end of `parent`'s children. Ignored if it would create a cycle.
    void appendChild(NodeId parent, NodeId child);
    /// Unlinks the node from its parent (DOM `remove()`). Safe on a node that has no parent.
    void detach(NodeId id);
    /// Appends text to an existing Text node.
    void appendToText(NodeId textNode, const QString &more);

    // --- attributes (names are matched exactly, so pass them lower-case) --------------------------------------
    bool hasAttr(NodeId id, const QString &name) const { return attrPtr(id, name) != nullptr; }
    const QString *attrPtr(NodeId id, const QString &name) const;
    QString attr(NodeId id, const QString &name) const;  ///< empty when absent
    void setAttr(NodeId id, const QString &name, const QString &value);
    void removeAttr(NodeId id, const QString &name);

    // --- navigation -------------------------------------------------------------------------------------------
    NodeId parentElement(NodeId id) const;  ///< parent, but only if it is an Element
    QVector<NodeId> children(NodeId id) const;
    QVector<NodeId> elementChildren(NodeId id) const;
    NodeId firstElementChild(NodeId id) const;
    NodeId lastElementChild(NodeId id) const;
    NodeId nextElementSibling(NodeId id) const;
    NodeId prevElementSibling(NodeId id) const;
    /// Next node in document order inside the subtree of `subtreeRoot`, or kNone. Start with cur = subtreeRoot.
    NodeId nextPreorder(NodeId subtreeRoot, NodeId cur) const;

    // --- well-known elements ----------------------------------------------------------------------------------
    NodeId documentElement() const;  ///< <html>
    NodeId head() const;
    NodeId body() const;
    NodeId findById(const QString &id) const;
    /// All Element descendants of `scope` (document order) whose tag name equals `lowerName` ("*" = any).
    QVector<NodeId> elementsByTagName(NodeId scope, const QString &lowerName) const;

    // --- content ----------------------------------------------------------------------------------------------
    QString textContent(NodeId id) const;
    QString innerHtml(NodeId id) const;
    QString outerHtml(NodeId id) const;

private:
    void writeNode(NodeId id, QString &out, int depth) const;
    NodeId childNamed(NodeId parent, const QString &lowerName) const;

    QVector<Node> m_nodes;
};

} // namespace dom
