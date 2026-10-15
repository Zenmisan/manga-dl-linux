// tst_selector.cpp — CSS selector engine tests
#include <QtTest>
#include "dom/HtmlParser.h"
#include "dom/Selector.h"

class TstSelector : public QObject
{
    Q_OBJECT

    static std::unique_ptr<dom::Document> parse(const char *html)
    {
        return dom::parseHtml(QString::fromUtf8(html));
    }

    static QString text(const dom::Document &doc, dom::NodeId id)
    {
        return doc.textContent(id).trimmed();
    }

private slots:

    // ── Tag selector ──────────────────────────────────────────────────────────
    void tagSelector()
    {
        auto doc = parse("<div><span>Hello</span><p>World</p></div>");
        const dom::NodeId span = dom::querySelector(*doc, doc->root(), "span");
        QVERIFY(span != dom::kNone);
        QCOMPARE(text(*doc, span), QStringLiteral("Hello"));
    }

    // ── Id selector ──────────────────────────────────────────────────────────
    void idSelector()
    {
        auto doc = parse("<div id='rating'>42</div>");
        const dom::NodeId el = dom::querySelector(*doc, doc->root(), "#rating");
        QVERIFY(el != dom::kNone);
        QCOMPARE(text(*doc, el), QStringLiteral("42"));
    }

    // ── Class selector ───────────────────────────────────────────────────────
    void classSelector()
    {
        auto doc = parse("<p class='a b chapternum'>3</p><p class='other'>x</p>");
        const dom::NodeId el = dom::querySelector(*doc, doc->root(), ".chapternum");
        QVERIFY(el != dom::kNone);
        QCOMPARE(text(*doc, el), QStringLiteral("3"));
    }

    // ── CSS-escaped class name (.bg-\[\#1C1924\]) ─────────────────────────────
    void escapedClassSelector()
    {
        auto doc = parse("<div class='bg-[#1C1924]'>special</div>");
        const dom::NodeId el = dom::querySelector(*doc, doc->root(),
                                                  ".bg-\\[\\#1C1924\\]");
        QVERIFY(el != dom::kNone);
        QCOMPARE(text(*doc, el), QStringLiteral("special"));
    }

    // ── Attribute presence ───────────────────────────────────────────────────
    void attrPresence()
    {
        auto doc = parse("<img src='a.jpg'/><img/>");
        const auto nodes = dom::querySelectorAll(*doc, doc->root(), "img[src]");
        QCOMPARE(nodes.size(), 1);
    }

    // ── Attribute equals ─────────────────────────────────────────────────────
    void attrEquals()
    {
        auto doc = parse("<meta property='og:image' content='cover.jpg'/>");
        const dom::NodeId el = dom::querySelector(*doc, doc->root(),
                                                  "meta[property=\"og:image\"]");
        QVERIFY(el != dom::kNone);
        QCOMPARE(doc->attr(el, "content"), QStringLiteral("cover.jpg"));
    }

    // ── Attribute contains (*=) ───────────────────────────────────────────────
    void attrContains()
    {
        auto doc = parse("<img src='/asura-images/chapters/p1.jpg'/><img src='other.jpg'/>");
        const auto nodes = dom::querySelectorAll(*doc, doc->root(),
                                                 "img[src*='/asura-images/chapters/']");
        QCOMPARE(nodes.size(), 1);
    }

    // ── Child combinator (>) ──────────────────────────────────────────────────
    void childCombinator()
    {
        auto doc = parse("<div><h3><a href='x'>Title</a></h3></div>");
        // Direct child: div > h3 > a
        const dom::NodeId a = dom::querySelector(*doc, doc->root(), "div > h3 > a");
        QVERIFY(a != dom::kNone);
    }

    // ── Descendant combinator ─────────────────────────────────────────────────
    void descendantCombinator()
    {
        auto doc = parse("<div><p><span>deep</span></p></div>");
        const dom::NodeId sp = dom::querySelector(*doc, doc->root(), "div span");
        QVERIFY(sp != dom::kNone);
        QCOMPARE(text(*doc, sp), QStringLiteral("deep"));
    }

    // ── Comma list ────────────────────────────────────────────────────────────
    void commaList()
    {
        auto doc = parse("<h3 class='title'>A</h3><h1 class='title'>B</h1><p>C</p>");
        const auto nodes = dom::querySelectorAll(*doc, doc->root(), "h1, h3");
        QCOMPARE(nodes.size(), 2);
    }

    // ── :first-child ─────────────────────────────────────────────────────────
    void firstChild()
    {
        auto doc = parse("<ul><li>A</li><li>B</li><li>C</li></ul>");
        const dom::NodeId li = dom::querySelector(*doc, doc->root(), "li:first-child");
        QVERIFY(li != dom::kNone);
        QCOMPARE(text(*doc, li), QStringLiteral("A"));
    }

    // ── :last-child ──────────────────────────────────────────────────────────
    void lastChild()
    {
        auto doc = parse("<ul><li>A</li><li>B</li><li>C</li></ul>");
        const dom::NodeId li = dom::querySelector(*doc, doc->root(), "li:last-child");
        QVERIFY(li != dom::kNone);
        QCOMPARE(text(*doc, li), QStringLiteral("C"));
    }

    // ── :nth-child ───────────────────────────────────────────────────────────
    void nthChild()
    {
        auto doc = parse("<ul><li>1</li><li>2</li><li>3</li><li>4</li></ul>");
        // :nth-child(2) → second element
        dom::NodeId li = dom::querySelector(*doc, doc->root(), "li:nth-child(2)");
        QVERIFY(li != dom::kNone);
        QCOMPARE(text(*doc, li), QStringLiteral("2"));
        // :nth-child(odd) → 1st and 3rd
        const auto odd = dom::querySelectorAll(*doc, doc->root(), "li:nth-child(odd)");
        QCOMPARE(odd.size(), 2);
        QCOMPARE(text(*doc, odd[0]), QStringLiteral("1"));
        QCOMPARE(text(*doc, odd[1]), QStringLiteral("3"));
    }

    // ── :not() ───────────────────────────────────────────────────────────────
    void notPseudo()
    {
        auto doc = parse(
            "<div class='pages'><img class='notice-page' src='a'/>"
            "<img src='b'/><img src='c'/></div>");
        const auto nodes = dom::querySelectorAll(*doc, doc->root(),
                                                 "img:not(.notice-page)");
        QCOMPARE(nodes.size(), 2);
    }

    // ── :has() ───────────────────────────────────────────────────────────────
    void hasPseudo()
    {
        auto doc = parse("<div class='meta'><div><span>Author</span><a href='x'>Joe</a></div></div>");
        const dom::NodeId el = dom::querySelector(*doc, doc->root(), ".meta div:has(span) a");
        QVERIFY(el != dom::kNone);
        QCOMPARE(text(*doc, el), QStringLiteral("Joe"));
    }

    // ── :contains() ──────────────────────────────────────────────────────────
    void containsPseudo()
    {
        auto doc = parse("<span>Author: Joe</span><span>Genre: Fantasy</span>");
        const dom::NodeId el = dom::querySelector(*doc, doc->root(),
                                                  "span:contains(\"Author\")");
        QVERIFY(el != dom::kNone);
        QVERIFY(text(*doc, el).contains(QStringLiteral("Author")));
    }

    // ── Leading > (scoped child) ──────────────────────────────────────────────
    void leadingChildCombinator()
    {
        // "> a" should only match direct children of scope, not deeper
        auto doc = parse("<div id='scope'><a href='x'>Direct</a><p><a href='y'>Nested</a></p></div>");
        const dom::NodeId scope = doc->findById(QStringLiteral("scope"));
        QVERIFY(scope != dom::kNone);
        const auto all = dom::querySelectorAll(*doc, scope, "> a");
        QCOMPARE(all.size(), 1);
        QCOMPARE(text(*doc, all[0]), QStringLiteral("Direct"));
    }

    // ── closest() ────────────────────────────────────────────────────────────
    void closestTest()
    {
        auto doc = parse("<div class='outer'><div class='inner'><span id='s'>x</span></div></div>");
        const dom::NodeId span = doc->findById(QStringLiteral("s"));
        QVERIFY(span != dom::kNone);
        const dom::NodeId cl = dom::closest(*doc, span, ".outer");
        QVERIFY(cl != dom::kNone);
        QCOMPARE(doc->node(cl).name, QStringLiteral("div"));
        QCOMPARE(doc->attr(cl, "class"), QStringLiteral("outer"));
    }

    // ── querySelectorAll returns in document order ────────────────────────────
    void documentOrder()
    {
        auto doc = parse("<p>A</p><p>B</p><p>C</p>");
        const auto ps = dom::querySelectorAll(*doc, doc->root(), "p");
        QCOMPARE(ps.size(), 3);
        QCOMPARE(text(*doc, ps[0]), QStringLiteral("A"));
        QCOMPARE(text(*doc, ps[1]), QStringLiteral("B"));
        QCOMPARE(text(*doc, ps[2]), QStringLiteral("C"));
    }

    // ── Extension-style selectors from the audit ──────────────────────────────
    void realWorldSelectors()
    {
        // div.novel-info h1.novel-title
        auto doc = parse("<div class='novel-info'><h1 class='novel-title'>My Novel</h1></div>");
        dom::NodeId el = dom::querySelector(*doc, doc->root(), "div.novel-info h1.novel-title");
        QVERIFY(el != dom::kNone);
        QCOMPARE(text(*doc, el), QStringLiteral("My Novel"));

        // a[href*="/manga/"]
        auto doc2 = parse("<a href='/manga/some-title'>link</a><a href='/other'>x</a>");
        const auto links = dom::querySelectorAll(*doc2, doc2->root(), "a[href*=\"/manga/\"]");
        QCOMPARE(links.size(), 1);

        // script, style, .ads (comma, three parts)
        auto doc3 = parse("<script>js</script><style>css</style>"
                          "<div class='ads'>ad</div><p>content</p>");
        const auto noise = dom::querySelectorAll(*doc3, doc3->root(), "script, style, .ads");
        QCOMPARE(noise.size(), 3);
    }
};

QTEST_MAIN(TstSelector)
#include "tst_selector.moc"
