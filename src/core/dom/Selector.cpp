// Selector.cpp - CSS selector engine for dom::Document. See Selector.h for the supported grammar.
//
// Design:
//  * A strict recursive-descent parser turns the selector text into a small AST. Anything it does not understand is
//    an error (never silently dropped), so an unsupported selector matches nothing instead of the wrong elements.
//  * Matching is right-to-left and recursive, so a chain such as `div.x > p span` backtracks correctly when the
//    same tag is nested.
//  * Compiled selectors are cached per thread, so a script that calls querySelectorAll in a loop parses once and a
//    bad selector is reported once.

#include "dom/Selector.h"

#include <QDebug>
#include <QHash>
#include <QStringList>

#include <memory>

namespace dom {

namespace {

// ===============================================================================================================
// Character helpers
// ===============================================================================================================

bool isWs(QChar c)
{
    const ushort u = c.unicode();
    return u == ' ' || u == '\t' || u == '\n' || u == '\r' || u == '\f';
}

bool isIdentStart(QChar c)
{
    const ushort u = c.unicode();
    return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || u == '_' || u == '-' || u > 127;
}

bool isIdentChar(QChar c)
{
    const ushort u = c.unicode();
    return isIdentStart(c) || (u >= '0' && u <= '9');
}

bool isHexDigit(QChar c)
{
    const ushort u = c.unicode();
    return (u >= '0' && u <= '9') || (u >= 'a' && u <= 'f') || (u >= 'A' && u <= 'F');
}

int hexValue(QChar c)
{
    const ushort u = c.unicode();
    if (u >= '0' && u <= '9')
        return int(u - '0');
    if (u >= 'a' && u <= 'f')
        return int(u - 'a') + 10;
    return int(u - 'A') + 10;
}

// Decodes one CSS escape. s[i] is the backslash; on return i is just past the escape.
QString decodeEscape(const QString &s, int &i)
{
    ++i;  // the backslash
    if (i >= s.size())
        return QStringLiteral("\uFFFD");
    if (isHexDigit(s[i])) {
        uint cp = 0;
        int digits = 0;
        while (i < s.size() && digits < 6 && isHexDigit(s[i])) {
            cp = cp * 16u + uint(hexValue(s[i]));
            ++i;
            ++digits;
        }
        if (i < s.size() && isWs(s[i]))
            ++i;  // one whitespace character may terminate a hex escape
        if (cp == 0 || cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu))
            return QStringLiteral("\uFFFD");
        QString out;
        if (cp >= 0x10000u) {
            out += QChar(QChar::highSurrogate(cp));
            out += QChar(QChar::lowSurrogate(cp));
        } else {
            out += QChar(ushort(cp));
        }
        return out;
    }
    return QString(s[i++]);  // any other escaped character stands for itself
}

QString readIdent(const QString &s, int &i)
{
    QString out;
    while (i < s.size()) {
        if (s[i] == '\\')
            out += decodeEscape(s, i);
        else if (isIdentChar(s[i]))
            out += s[i++];
        else
            break;
    }
    return out;
}

// ===============================================================================================================
// AST
// ===============================================================================================================

enum class Combinator { Descendant, Child, Adjacent, Sibling };

struct AttrTest {
    enum class Op { Presence, Eq, Contains, StartsWith, EndsWith, Word, DashPrefix };
    QString name;
    Op op = Op::Presence;
    QString value;
    bool caseInsensitive = false;
};

enum class PseudoKind {
    FirstChild, LastChild, OnlyChild, NthChild, NthLastChild,
    FirstOfType, LastOfType, OnlyOfType, NthOfType, NthLastOfType,
    Empty, Root, Not, Is, Has, Contains
};

struct Group;

struct Pseudo {
    PseudoKind kind = PseudoKind::FirstChild;
    int a = 0;  // an+b
    int b = 1;
    QString text;                     // :contains
    std::shared_ptr<const Group> sub; // :not :is :has
};

struct Compound {
    QString tag;  // empty = any
    QString id;
    QVector<QString> classes;
    QVector<AttrTest> attrs;
    QVector<Pseudo> pseudos;
};

struct Step {
    Combinator comb = Combinator::Descendant;  // how this compound relates to the previous one (unused for step 0)
    Compound compound;
};

struct Selector {
    QVector<Step> steps;
    bool leadingChild = false;  // starts with '>': the first compound must be a direct child of the scope
    bool relative = false;      // anchored to the scope element (leading '>' and :has() arguments)
};

struct Group {
    QVector<Selector> selectors;
};

// ===============================================================================================================
// Parser
// ===============================================================================================================

bool parseNth(const QString &arg, int &a, int &b)
{
    QString t = arg.toLower();
    t.remove(QLatin1Char(' '));
    t.remove(QLatin1Char('\t'));
    t.remove(QLatin1Char('\n'));
    if (t == QLatin1String("odd")) {
        a = 2;
        b = 1;
        return true;
    }
    if (t == QLatin1String("even")) {
        a = 2;
        b = 0;
        return true;
    }
    bool ok = true;
    const int n = int(t.indexOf(QLatin1Char('n')));
    if (n < 0) {
        a = 0;
        b = t.toInt(&ok);
        return ok && !t.isEmpty();
    }
    const QString aText = t.left(n);
    if (aText.isEmpty() || aText == QLatin1String("+")) {
        a = 1;
    } else if (aText == QLatin1String("-")) {
        a = -1;
    } else {
        a = aText.toInt(&ok);
        if (!ok)
            return false;
    }
    const QString rest = t.mid(n + 1);
    if (rest.isEmpty()) {
        b = 0;
        return true;
    }
    if (rest[0] != '+' && rest[0] != '-')
        return false;
    b = rest.toInt(&ok);
    return ok;
}

class Parser
{
public:
    explicit Parser(const QString &text) : s(text) {}

    QString error;

    // nested: stop at the closing ')' of :not()/:is()/:has(). allowLeading: a leading '>' is allowed.
    // alwaysRelative: every selector is anchored to the scope (the argument of :has()).
    bool parseGroup(Group &out, bool nested, bool allowLeading, bool alwaysRelative)
    {
        if (++depth > kMaxDepth)
            return fail(QStringLiteral("selector is nested too deeply"));
        for (;;) {
            Selector sel;
            if (!parseSelector(sel, allowLeading))
                return false;
            if (alwaysRelative)
                sel.relative = true;
            out.selectors.append(std::move(sel));
            skipWs();
            if (!atEnd() && s[i] == ',') {
                ++i;
                continue;
            }
            --depth;
            if (nested) {
                if (!atEnd() && s[i] == ')')
                    return true;
                return fail(QStringLiteral("expected ')'"));
            }
            if (atEnd())
                return true;
            return fail(QStringLiteral("unexpected '%1'").arg(s[i]));
        }
    }

private:
    static constexpr int kMaxDepth = 16;

    const QString &s;
    int i = 0;
    int depth = 0;

    bool atEnd() const { return i >= s.size(); }

    void skipWs()
    {
        while (i < s.size() && isWs(s[i]))
            ++i;
    }

    bool fail(const QString &message)
    {
        if (error.isEmpty())
            error = message + QStringLiteral(" (at position %1)").arg(i);
        return false;
    }

    bool parseSelector(Selector &out, bool allowLeading)
    {
        skipWs();
        if (atEnd())
            return fail(QStringLiteral("empty selector"));
        if (s[i] == '>') {
            if (!allowLeading)
                return fail(QStringLiteral("unexpected '>'"));
            out.leadingChild = true;
            out.relative = true;
            ++i;
            skipWs();
        } else if (s[i] == '+' || s[i] == '~') {
            return fail(QStringLiteral("a selector cannot start with a sibling combinator"));
        }

        Compound first;
        if (!parseCompound(first))
            return false;
        out.steps.append(Step{Combinator::Descendant, std::move(first)});

        for (;;) {
            const int before = i;
            skipWs();
            const bool hadWhitespace = i > before;
            if (atEnd() || s[i] == ',' || s[i] == ')')
                return true;

            Combinator comb = Combinator::Descendant;
            if (s[i] == '>') {
                comb = Combinator::Child;
                ++i;
                skipWs();
            } else if (s[i] == '+') {
                comb = Combinator::Adjacent;
                ++i;
                skipWs();
            } else if (s[i] == '~') {
                comb = Combinator::Sibling;
                ++i;
                skipWs();
            } else if (!hadWhitespace) {
                return fail(QStringLiteral("unexpected '%1'").arg(s[i]));
            }

            Compound next;
            if (!parseCompound(next))
                return false;
            out.steps.append(Step{comb, std::move(next)});
        }
    }

    bool parseCompound(Compound &c)
    {
        bool any = false;
        if (!atEnd()) {
            if (s[i] == '*') {
                ++i;
                any = true;
            } else if (isIdentStart(s[i]) || s[i] == '\\') {
                c.tag = readIdent(s, i).toLower();
                any = true;
            }
        }
        while (!atEnd()) {
            const QChar ch = s[i];
            if (ch == '#') {
                ++i;
                const QString v = readIdent(s, i);
                if (v.isEmpty())
                    return fail(QStringLiteral("expected an id after '#'"));
                c.id = v;
                any = true;
            } else if (ch == '.') {
                ++i;
                const QString v = readIdent(s, i);
                if (v.isEmpty())
                    return fail(QStringLiteral("expected a class name after '.'"));
                c.classes.append(v);
                any = true;
            } else if (ch == '[') {
                if (!parseAttribute(c))
                    return false;
                any = true;
            } else if (ch == ':') {
                if (!parsePseudo(c))
                    return false;
                any = true;
            } else {
                break;
            }
        }
        if (!any)
            return fail(atEnd() ? QStringLiteral("expected a selector")
                                : QStringLiteral("unexpected '%1'").arg(s[i]));
        return true;
    }

    // Reads "..." or '...' with CSS escapes. s[i] is the opening quote.
    bool readQuoted(QString &out)
    {
        const QChar quote = s[i++];
        while (!atEnd() && s[i] != quote) {
            if (s[i] == '\\') {
                if (i + 1 < s.size() && s[i + 1] == '\n') {
                    i += 2;  // an escaped newline continues the string
                    continue;
                }
                out += decodeEscape(s, i);
            } else {
                out += s[i++];
            }
        }
        if (atEnd())
            return fail(QStringLiteral("unterminated string"));
        ++i;
        return true;
    }

    bool parseAttribute(Compound &c)
    {
        ++i;  // '['
        skipWs();
        AttrTest test;
        test.name = readIdent(s, i).toLower();
        if (test.name.isEmpty())
            return fail(QStringLiteral("expected an attribute name"));
        skipWs();
        if (atEnd())
            return fail(QStringLiteral("unterminated attribute selector"));
        if (s[i] == ']') {
            ++i;
            c.attrs.append(test);
            return true;
        }

        const QChar op1 = s[i];
        const QChar op2 = i + 1 < s.size() ? s[i + 1] : QChar();
        if (op1 == '=') {
            test.op = AttrTest::Op::Eq;
            ++i;
        } else if (op2 == '=' && (op1 == '*' || op1 == '^' || op1 == '$' || op1 == '~' || op1 == '|')) {
            if (op1 == '*')
                test.op = AttrTest::Op::Contains;
            else if (op1 == '^')
                test.op = AttrTest::Op::StartsWith;
            else if (op1 == '$')
                test.op = AttrTest::Op::EndsWith;
            else if (op1 == '~')
                test.op = AttrTest::Op::Word;
            else
                test.op = AttrTest::Op::DashPrefix;
            i += 2;
        } else {
            return fail(QStringLiteral("invalid attribute operator"));
        }

        skipWs();
        if (atEnd())
            return fail(QStringLiteral("unterminated attribute selector"));
        if (s[i] == '"' || s[i] == '\'') {
            if (!readQuoted(test.value))
                return false;
        } else {
            // Bare value: more lenient than CSS (which wants an identifier), so [href*=/chapter/] works.
            while (!atEnd() && !isWs(s[i]) && s[i] != ']') {
                if (s[i] == '\\')
                    test.value += decodeEscape(s, i);
                else
                    test.value += s[i++];
            }
            if (test.value.isEmpty())
                return fail(QStringLiteral("expected an attribute value"));
        }

        skipWs();
        if (!atEnd() && (s[i] == 'i' || s[i] == 'I')) {
            test.caseInsensitive = true;
            ++i;
            skipWs();
        } else if (!atEnd() && (s[i] == 's' || s[i] == 'S')) {
            ++i;
            skipWs();
        }
        if (atEnd() || s[i] != ']')
            return fail(QStringLiteral("expected ']'"));
        ++i;
        c.attrs.append(test);
        return true;
    }

    bool parsePseudo(Compound &c)
    {
        ++i;  // ':'
        if (!atEnd() && s[i] == ':')
            return fail(QStringLiteral("pseudo-elements are not supported"));
        const QString name = readIdent(s, i).toLower();
        if (name.isEmpty())
            return fail(QStringLiteral("expected a pseudo-class name"));

        Pseudo p;
        if (atEnd() || s[i] != '(') {
            if (name == QLatin1String("first-child"))
                p.kind = PseudoKind::FirstChild;
            else if (name == QLatin1String("last-child"))
                p.kind = PseudoKind::LastChild;
            else if (name == QLatin1String("only-child"))
                p.kind = PseudoKind::OnlyChild;
            else if (name == QLatin1String("first-of-type"))
                p.kind = PseudoKind::FirstOfType;
            else if (name == QLatin1String("last-of-type"))
                p.kind = PseudoKind::LastOfType;
            else if (name == QLatin1String("only-of-type"))
                p.kind = PseudoKind::OnlyOfType;
            else if (name == QLatin1String("empty"))
                p.kind = PseudoKind::Empty;
            else if (name == QLatin1String("root"))
                p.kind = PseudoKind::Root;
            else
                return fail(QStringLiteral("unsupported pseudo-class :%1").arg(name));
            c.pseudos.append(p);
            return true;
        }

        ++i;  // '('
        if (name == QLatin1String("nth-child") || name == QLatin1String("nth-last-child")
            || name == QLatin1String("nth-of-type") || name == QLatin1String("nth-last-of-type")) {
            int depthInside = 1;
            const int start = i;
            while (!atEnd() && depthInside > 0) {
                if (s[i] == '(')
                    ++depthInside;
                else if (s[i] == ')')
                    --depthInside;
                ++i;
            }
            if (depthInside != 0)
                return fail(QStringLiteral("expected ')'"));
            if (!parseNth(s.mid(start, i - start - 1), p.a, p.b))
                return fail(QStringLiteral("invalid argument for :%1").arg(name));
            if (name == QLatin1String("nth-child"))
                p.kind = PseudoKind::NthChild;
            else if (name == QLatin1String("nth-last-child"))
                p.kind = PseudoKind::NthLastChild;
            else if (name == QLatin1String("nth-of-type"))
                p.kind = PseudoKind::NthOfType;
            else
                p.kind = PseudoKind::NthLastOfType;
        } else if (name == QLatin1String("not") || name == QLatin1String("is") || name == QLatin1String("where")
                   || name == QLatin1String("has")) {
            const bool isHas = name == QLatin1String("has");
            auto group = std::make_shared<Group>();
            if (!parseGroup(*group, true, isHas, isHas))
                return false;
            ++i;  // the ')' parseGroup stopped at
            p.kind = isHas ? PseudoKind::Has : (name == QLatin1String("not") ? PseudoKind::Not : PseudoKind::Is);
            p.sub = group;
        } else if (name == QLatin1String("contains")) {
            skipWs();
            if (!atEnd() && (s[i] == '"' || s[i] == '\'')) {
                if (!readQuoted(p.text))
                    return false;
            } else {
                while (!atEnd() && s[i] != ')')
                    p.text += s[i++];
                p.text = p.text.trimmed();
            }
            skipWs();
            if (atEnd() || s[i] != ')')
                return fail(QStringLiteral("expected ')'"));
            ++i;
            p.kind = PseudoKind::Contains;
        } else {
            return fail(QStringLiteral("unsupported pseudo-class :%1()").arg(name));
        }
        c.pseudos.append(p);
        return true;
    }
};

// ===============================================================================================================
// Matching
// ===============================================================================================================

bool matchChain(const Document &doc, const Selector &sel, int si, NodeId el, NodeId scope);

bool matchGroup(const Document &doc, const Group &group, NodeId el, NodeId scope)
{
    for (const Selector &sel : group.selectors) {
        if (matchChain(doc, sel, int(sel.steps.size()) - 1, el, scope))
            return true;
    }
    return false;
}

bool matchAttribute(const QString &actual, const AttrTest &t)
{
    const Qt::CaseSensitivity cs = t.caseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive;
    switch (t.op) {
    case AttrTest::Op::Presence:
        return true;
    case AttrTest::Op::Eq:
        return actual.compare(t.value, cs) == 0;
    // The CSS spec says an empty value matches nothing for these three.
    case AttrTest::Op::Contains:
        return !t.value.isEmpty() && actual.contains(t.value, cs);
    case AttrTest::Op::StartsWith:
        return !t.value.isEmpty() && actual.startsWith(t.value, cs);
    case AttrTest::Op::EndsWith:
        return !t.value.isEmpty() && actual.endsWith(t.value, cs);
    case AttrTest::Op::Word: {
        if (t.value.isEmpty() || t.value.contains(QLatin1Char(' ')))
            return false;
        const QStringList words = actual.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString &w : words) {
            if (w.compare(t.value, cs) == 0)
                return true;
        }
        return false;
    }
    case AttrTest::Op::DashPrefix:
        return actual.compare(t.value, cs) == 0 || actual.startsWith(t.value + QLatin1Char('-'), cs);
    }
    return false;
}

bool hasClass(const QString &classAttr, const QString &wanted)
{
    const QStringList tokens = classAttr.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    return tokens.contains(wanted);
}

// 1-based position among sibling elements (optionally only those with the same tag), counted from either end.
int positionAmongSiblings(const Document &doc, NodeId el, bool fromEnd, bool sameTypeOnly)
{
    const QString &name = doc.node(el).name;
    int pos = 1;
    NodeId s = fromEnd ? doc.nextElementSibling(el) : doc.prevElementSibling(el);
    while (s != kNone) {
        if (!sameTypeOnly || doc.node(s).name == name)
            ++pos;
        s = fromEnd ? doc.nextElementSibling(s) : doc.prevElementSibling(s);
    }
    return pos;
}

bool nthMatches(int a, int b, int pos)
{
    if (a == 0)
        return pos == b;
    const int diff = pos - b;
    return diff % a == 0 && diff / a >= 0;
}

bool matchPseudo(const Document &doc, NodeId el, const Pseudo &p)
{
    switch (p.kind) {
    case PseudoKind::FirstChild:
        return positionAmongSiblings(doc, el, false, false) == 1;
    case PseudoKind::LastChild:
        return positionAmongSiblings(doc, el, true, false) == 1;
    case PseudoKind::OnlyChild:
        return positionAmongSiblings(doc, el, false, false) == 1 && positionAmongSiblings(doc, el, true, false) == 1;
    case PseudoKind::NthChild:
        return nthMatches(p.a, p.b, positionAmongSiblings(doc, el, false, false));
    case PseudoKind::NthLastChild:
        return nthMatches(p.a, p.b, positionAmongSiblings(doc, el, true, false));
    case PseudoKind::FirstOfType:
        return positionAmongSiblings(doc, el, false, true) == 1;
    case PseudoKind::LastOfType:
        return positionAmongSiblings(doc, el, true, true) == 1;
    case PseudoKind::OnlyOfType:
        return positionAmongSiblings(doc, el, false, true) == 1 && positionAmongSiblings(doc, el, true, true) == 1;
    case PseudoKind::NthOfType:
        return nthMatches(p.a, p.b, positionAmongSiblings(doc, el, false, true));
    case PseudoKind::NthLastOfType:
        return nthMatches(p.a, p.b, positionAmongSiblings(doc, el, true, true));
    case PseudoKind::Empty: {
        for (NodeId c = doc.node(el).firstChild; c != kNone; c = doc.node(c).next) {
            const Node &n = doc.node(c);
            if (n.type == NodeType::Element || (n.type == NodeType::Text && !n.text.isEmpty()))
                return false;
        }
        return true;
    }
    case PseudoKind::Root:
        return doc.node(el).parent == doc.root();
    case PseudoKind::Not:
        return !matchGroup(doc, *p.sub, el, el);
    case PseudoKind::Is:
        return matchGroup(doc, *p.sub, el, el);
    case PseudoKind::Has: {
        // The arguments are relative selectors: the subject `el` is their scope.
        for (NodeId c = doc.nextPreorder(el, el); c != kNone; c = doc.nextPreorder(el, c)) {
            if (doc.node(c).type == NodeType::Element && matchGroup(doc, *p.sub, c, el))
                return true;
        }
        return false;
    }
    case PseudoKind::Contains:
        return doc.textContent(el).contains(p.text);
    }
    return false;
}

bool matchCompound(const Document &doc, NodeId el, const Compound &c)
{
    const Node &n = doc.node(el);
    if (n.type != NodeType::Element)
        return false;
    if (!c.tag.isEmpty() && n.name != c.tag)
        return false;
    if (!c.id.isEmpty()) {
        const QString *v = doc.attrPtr(el, QStringLiteral("id"));
        if (!v || *v != c.id)
            return false;
    }
    if (!c.classes.isEmpty()) {
        const QString *classAttr = doc.attrPtr(el, QStringLiteral("class"));
        if (!classAttr)
            return false;
        for (const QString &cls : c.classes) {
            if (!hasClass(*classAttr, cls))
                return false;
        }
    }
    for (const AttrTest &t : c.attrs) {
        const QString *v = doc.attrPtr(el, t.name);
        if (!v || !matchAttribute(*v, t))
            return false;
    }
    for (const Pseudo &p : c.pseudos) {
        if (!matchPseudo(doc, el, p))
            return false;
    }
    return true;
}

bool isDescendantOf(const Document &doc, NodeId el, NodeId ancestor)
{
    for (NodeId a = doc.node(el).parent; a != kNone; a = doc.node(a).parent) {
        if (a == ancestor)
            return true;
    }
    return false;
}

// Does `el` match steps[si], with everything to its left satisfied too? Recursive, so every alternative for a
// descendant or sibling combinator is tried (a nearest-match-only walk would miss valid matches).
bool matchChain(const Document &doc, const Selector &sel, int si, NodeId el, NodeId scope)
{
    if (!matchCompound(doc, el, sel.steps[si].compound))
        return false;

    if (si == 0) {
        if (sel.leadingChild)
            return doc.node(el).parent == scope;
        if (sel.relative)
            return isDescendantOf(doc, el, scope);
        return true;
    }

    switch (sel.steps[si].comb) {
    case Combinator::Child: {
        const NodeId p = doc.parentElement(el);
        return p != kNone && matchChain(doc, sel, si - 1, p, scope);
    }
    case Combinator::Descendant:
        for (NodeId a = doc.parentElement(el); a != kNone; a = doc.parentElement(a)) {
            if (sel.relative && a == scope)
                break;  // a relative selector never looks above the scope element
            if (matchChain(doc, sel, si - 1, a, scope))
                return true;
        }
        return false;
    case Combinator::Adjacent: {
        const NodeId prev = doc.prevElementSibling(el);
        return prev != kNone && matchChain(doc, sel, si - 1, prev, scope);
    }
    case Combinator::Sibling:
        for (NodeId s = doc.prevElementSibling(el); s != kNone; s = doc.prevElementSibling(s)) {
            if (matchChain(doc, sel, si - 1, s, scope))
                return true;
        }
        return false;
    }
    return false;
}

// ===============================================================================================================
// Compilation and cache
// ===============================================================================================================

struct Compiled {
    Group group;
    bool ok = false;
    QString error;
};

std::shared_ptr<const Compiled> compile(const QString &selector)
{
    thread_local QHash<QString, std::shared_ptr<const Compiled>> cache;
    const auto it = cache.constFind(selector);
    if (it != cache.constEnd())
        return it.value();

    auto compiled = std::make_shared<Compiled>();
    Parser parser(selector);
    compiled->ok = parser.parseGroup(compiled->group, false, true, false);
    if (!compiled->ok) {
        compiled->error = parser.error;
        compiled->group = Group();
        // Once per distinct selector (the result is cached): a script that loops must not flood the log.
        qWarning().noquote() << "[dom] invalid selector" << selector << "-" << compiled->error;
    }
    if (cache.size() > 512)
        cache.clear();
    cache.insert(selector, compiled);
    return compiled;
}

} // namespace

// ===============================================================================================================
// Public API
// ===============================================================================================================

NodeId querySelector(const Document &doc, NodeId scope, const QString &selector)
{
    const auto compiled = compile(selector);
    if (!compiled->ok || !doc.valid(scope))
        return kNone;
    for (NodeId c = doc.nextPreorder(scope, scope); c != kNone; c = doc.nextPreorder(scope, c)) {
        if (doc.node(c).type == NodeType::Element && matchGroup(doc, compiled->group, c, scope))
            return c;
    }
    return kNone;
}

QVector<NodeId> querySelectorAll(const Document &doc, NodeId scope, const QString &selector)
{
    QVector<NodeId> result;
    const auto compiled = compile(selector);
    if (!compiled->ok || !doc.valid(scope))
        return result;
    for (NodeId c = doc.nextPreorder(scope, scope); c != kNone; c = doc.nextPreorder(scope, c)) {
        if (doc.node(c).type == NodeType::Element && matchGroup(doc, compiled->group, c, scope))
            result.append(c);
    }
    return result;
}

NodeId closest(const Document &doc, NodeId startNode, const QString &selector)
{
    const auto compiled = compile(selector);
    if (!compiled->ok || !doc.valid(startNode))
        return kNone;
    for (NodeId cur = startNode; cur != kNone; cur = doc.parentElement(cur)) {
        if (doc.node(cur).type == NodeType::Element && matchGroup(doc, compiled->group, cur, cur))
            return cur;
    }
    return kNone;
}

bool isValidSelector(const QString &selector, QString *error)
{
    const auto compiled = compile(selector);
    if (!compiled->ok && error)
        *error = compiled->error;
    return compiled->ok;
}

} // namespace dom
