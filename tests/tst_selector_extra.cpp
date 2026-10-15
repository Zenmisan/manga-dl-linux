// tst_selector_extra.cpp - strictness, sibling combinators, backtracking and scope semantics of the selector engine
#include <QtTest>

#include "dom/HtmlParser.h"
#include "dom/Selector.h"

class TstSelectorExtra : public QObject
{
    Q_OBJECT

    static std::unique_ptr<dom::Document> parse(const char *html)
    {
        return dom::parseHtml(QString::fromUtf8(html));
    }

    static QStringList texts(const dom::Document &doc, const QVector<dom::NodeId> &ids)
    {
        QStringList out;
        for (dom::NodeId id : ids)
            out.append(doc.textContent(id).trimmed());
        return out;
    }

private slots:

    // ── strictness: unsupported input is an error, never a quietly simpler selector ────────────────────────
    void invalidSelectorsAreRejected_data()
    {
        QTest::addColumn<QString>("selector");
        const char *const invalid[] = {
            "p:bogus(",          // the case that used to degrade to plain `p`
            "p:bogus",           // unknown pseudo-class
            "a:hover",           // not meaningful for a static document, and not supported
            "li:nth-of-type()",  // empty argument
            "li:nth-child(x)",   // unparsable argument
            "a[",                // unterminated attribute selector
            "a[href",            //
            "a[href=]",          // missing value
            "a[href^]",          // operator without '='
            "a,,b",              // empty selector in a list
            "a,",                // trailing comma
            ",a",                // leading comma
            ">",                 // combinator with nothing after it
            "a >",               // dangling combinator
            "+ a",               // sibling combinator with nothing before it
            "p::before",         // pseudo-element
            "a)",                // stray parenthesis
            ":not()",            // empty argument
            "div:has(",          // unterminated argument
            ".",                 // class without a name
            "#",                 // id without a name
            "\"a\"",             // a string is not a selector
            "a $ b",             // stray character
            "",                  // empty
            "   ",               // blank
        };
        for (const char *s : invalid)
            QTest::newRow(s[0] ? s : "(empty)") << QString::fromUtf8(s);
    }

    void invalidSelectorsAreRejected()
    {
        QFETCH(QString, selector);
        QString error;
        QVERIFY2(!dom::isValidSelector(selector, &error), "this selector must be invalid");
        QVERIFY2(!error.isEmpty(), "an invalid selector must say why");

        // A parser that drops what it does not understand would match `p` or `a` here.
        auto doc = parse("<p>x</p><a href='y'>y</a>");
        QVERIFY(dom::querySelectorAll(*doc, doc->root(), selector).isEmpty());
        QCOMPARE(dom::querySelector(*doc, doc->root(), selector), dom::kNone);
    }

    void validSelectorsAreAccepted_data()
    {
        QTest::addColumn<QString>("selector");
        const char *const valid[] = {
            "a", "*", "div > p", "a + b", "a ~ b", "a, b", "a:not(.x)", "li:nth-child(2n+1)",
            "li:nth-child( -n + 3 )", "li:nth-child(odd)", "li:nth-last-child(2)", "a[href]", "a[href='x']",
            "a[href=x]", "a[href*=/x/ i]", ".a.b#c", "div:has(> a)", "div:has(span)", "p:contains(\"x\")",
            "p:contains(x)", ":root", "a:is(b, c)", "a:where(b)", "ul > li:first-child a", "> a", "> div > div",
            "[data-x]", "  a  ", "a\n>\nb", ".bg-\\[\\#1C1924\\]", "chapter-page img[data-src]",
            "span.leading-7, .author, span:contains(\"Author\")", "li:only-child", "p:empty",
            "span:first-of-type", "span:last-of-type", "span:nth-of-type(2)", "b:only-of-type",
        };
        for (const char *s : valid)
            QTest::newRow(s) << QString::fromUtf8(s);
    }

    void validSelectorsAreAccepted()
    {
        QFETCH(QString, selector);
        QString error;
        QVERIFY2(dom::isValidSelector(selector, &error), qPrintable(error));
    }

    // ── combinators ───────────────────────────────────────────────────────────
    void siblingCombinators()
    {
        auto doc = parse("<ul><li>1</li><li class='x'>2</li><li>3</li><li>4</li></ul>");
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, doc->root(), "li.x + li")), QStringList{"3"});
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, doc->root(), "li.x ~ li")), (QStringList{"3", "4"}));
        QCOMPARE(dom::querySelectorAll(*doc, doc->root(), "li + li").size(), 3);
        QVERIFY(dom::querySelectorAll(*doc, doc->root(), "li.x + li + li + li").isEmpty());
    }

    // `div.x > section span`: the nearest <section> ancestor of the span is not a child of div.x, but its
    // outer <section> is. A matcher that only tries the nearest ancestor misses this.
    void chainsBacktrack()
    {
        auto doc = parse("<div class='x'><section id='a'><div><section id='b'><span>deep</span></section></div></section></div>");
        const auto hits = dom::querySelectorAll(*doc, doc->root(), "div.x > section span");
        QCOMPARE(texts(*doc, hits), QStringList{"deep"});
    }

    // ── scope semantics ───────────────────────────────────────────────────────
    void plainSelectorsSeeAncestorsAboveTheScope()
    {
        auto doc = parse("<div class='a'><section id='s'><p>hit</p></section></div>");
        const dom::NodeId scope = doc->findById(QStringLiteral("s"));
        QVERIFY(scope != dom::kNone);
        // Same as element.querySelectorAll in a browser: `div.a` is above the scope but still takes part.
        QCOMPARE(dom::querySelectorAll(*doc, scope, "div.a p").size(), 1);
        QCOMPARE(dom::querySelectorAll(*doc, scope, ".a > section p").size(), 1);
    }

    void hasIsRelativeToItsSubject()
    {
        auto doc = parse("<div><section id='sec'><p>x</p></section></div>");
        QCOMPARE(dom::querySelectorAll(*doc, doc->root(), "section:has(p)").size(), 1);
        // `div` is ABOVE the section, so it cannot be part of what the section "has".
        QCOMPARE(dom::querySelectorAll(*doc, doc->root(), "section:has(div p)").size(), 0);
        QCOMPARE(dom::querySelectorAll(*doc, doc->root(), "div:has(section p)").size(), 1);
        QCOMPARE(dom::querySelectorAll(*doc, doc->root(), "div:has(> section)").size(), 1);
        QCOMPARE(dom::querySelectorAll(*doc, doc->root(), "div:has(> p)").size(), 0);  // p is a grandchild
    }

    void leadingChildMatchesOnlyDirectChildrenOfTheScope()
    {
        auto doc = parse("<div id='s'><div><div id='deep'>d</div></div><div id='near'>n</div></div>");
        const dom::NodeId scope = doc->findById(QStringLiteral("s"));
        QCOMPARE(dom::querySelectorAll(*doc, scope, "> div").size(), 2);
        QCOMPARE(dom::querySelectorAll(*doc, scope, "> div > div").size(), 1);  // the one nested in the first child
    }

    // ── pseudo-classes ────────────────────────────────────────────────────────
    void typeBasedPositions()
    {
        auto doc = parse("<div><span>a</span><b>b</b><span>c</span><span>d</span></div>");
        const dom::NodeId root = doc->root();
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "span:nth-of-type(2)")), QStringList{"c"});
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "span:first-of-type")), QStringList{"a"});
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "span:last-of-type")), QStringList{"d"});
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "span:nth-last-of-type(2)")), QStringList{"c"});
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "b:only-of-type")), QStringList{"b"});
        QCOMPARE(dom::querySelectorAll(*doc, root, "span:nth-child(2)").size(), 0);  // the 2nd child is the <b>
    }

    void nthExpressions()
    {
        auto doc = parse("<ul><li>1</li><li>2</li><li>3</li><li>4</li><li>5</li></ul>");
        const dom::NodeId root = doc->root();
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "li:nth-child(2n+1)")), (QStringList{"1", "3", "5"}));
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "li:nth-child(-n+2)")), (QStringList{"1", "2"}));
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "li:nth-child(even)")), (QStringList{"2", "4"}));
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "li:nth-last-child(1)")), QStringList{"5"});
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "li:nth-child(n+4)")), (QStringList{"4", "5"}));
    }

    void emptyOnlyChildAndRoot()
    {
        auto doc = parse("<div><p></p><p>x</p></div><section><i>only</i></section>");
        const dom::NodeId root = doc->root();
        QCOMPARE(dom::querySelectorAll(*doc, root, "p:empty").size(), 1);
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "i:only-child")), QStringList{"only"});
        const dom::NodeId html = dom::querySelector(*doc, root, ":root");
        QVERIFY(html != dom::kNone);
        QCOMPARE(doc->node(html).name, QStringLiteral("html"));
    }

    void isAndNotTakeSelectorLists()
    {
        auto doc = parse("<p class='a'>1</p><p class='b'>2</p><p class='c'>3</p>");
        const dom::NodeId root = doc->root();
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "p:is(.a, .c)")), (QStringList{"1", "3"}));
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "p:not(.a, .c)")), QStringList{"2"});
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, root, "p:not(.a):not(.b)")), QStringList{"3"});
    }

    // ── attributes ────────────────────────────────────────────────────────────
    void attributeOperators()
    {
        auto doc = parse("<a id='1' class='x b y' lang='en-US' href='http://a/p.JPG' data-k='Ab'>t</a>");
        const dom::NodeId root = doc->root();
        QCOMPARE(dom::querySelectorAll(*doc, root, "[class~=b]").size(), 1);
        QCOMPARE(dom::querySelectorAll(*doc, root, "[class~=bb]").size(), 0);
        QCOMPARE(dom::querySelectorAll(*doc, root, "[lang|=en]").size(), 1);
        QCOMPARE(dom::querySelectorAll(*doc, root, "[lang|=e]").size(), 0);
        QCOMPARE(dom::querySelectorAll(*doc, root, "[href^=http]").size(), 1);
        QCOMPARE(dom::querySelectorAll(*doc, root, "[href$=\".JPG\"]").size(), 1);
        QCOMPARE(dom::querySelectorAll(*doc, root, "[href$=\".jpg\"]").size(), 0);       // case-sensitive by default
        QCOMPARE(dom::querySelectorAll(*doc, root, "[href$=\".jpg\" i]").size(), 1);     // unless asked
        QCOMPARE(dom::querySelectorAll(*doc, root, "[data-k=Ab]").size(), 1);
        QCOMPARE(dom::querySelectorAll(*doc, root, "[href*=\"\"]").size(), 0);           // empty value matches nothing
        QCOMPARE(dom::querySelectorAll(*doc, root, "[href*=/p.]").size(), 1);            // bare value with slashes
    }

    void selectorListKeepsDocumentOrderWithoutDuplicates()
    {
        auto doc = parse("<p class='a b'>1</p><p class='a'>2</p><p class='b'>3</p>");
        QCOMPARE(texts(*doc, dom::querySelectorAll(*doc, doc->root(), ".b, .a")), (QStringList{"1", "2", "3"}));
    }

    void closestUsesTheSameEngine()
    {
        auto doc = parse("<div class='a'><p><span id='s'>x</span></p></div>");
        const dom::NodeId span = doc->findById(QStringLiteral("s"));
        QCOMPARE(doc->node(dom::closest(*doc, span, "div > p")).name, QStringLiteral("p"));
        QCOMPARE(dom::closest(*doc, span, "p:bogus("), dom::kNone);  // invalid: nothing, not "p"
    }
};

QTEST_MAIN(TstSelectorExtra)
#include "tst_selector_extra.moc"
