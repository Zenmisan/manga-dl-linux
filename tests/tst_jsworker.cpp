// tst_jsworker.cpp - JS runtime: globals, live DOM, call contract, async networking, limits
#include <QElapsedTimer>
#include <QFile>
#include <QtTest>

#include "extensions/JsWorker.h"
#include "http/HttpService.h"

namespace {

bool waitFor(const std::function<bool()> &cond, int msec = 5000)
{
    const QDeadlineTimer deadline(msec);
    while (!deadline.hasExpired()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (cond())
            return true;
    }
    return cond();
}

QString readScript(const QString &relative)
{
    QFile f(QStringLiteral(MANGADL_ASSETS_DIR) + QLatin1Char('/') + relative);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

} // namespace

class TstJsWorker : public QObject
{
    Q_OBJECT

    JsWorkerThread *m_wt = nullptr;
    QHash<quint64, JsResult> m_results;
    quint64 m_nextId = 1000;

    JsWorker *w() const { return m_wt->worker(); }

    JsResult evalOnWorker(const QString &code)
    {
        JsResult r;
        QMetaObject::invokeMethod(w(), [this, &r, &code] { r = w()->evalSync(code); }, Qt::BlockingQueuedConnection);
        return r;
    }

    quint64 startCall(const QString &ext, const QString &code, const QString &method, const QString &args)
    {
        const quint64 id = ++m_nextId;
        QMetaObject::invokeMethod(w(), "invokeScript", Qt::QueuedConnection, Q_ARG(quint64, id), Q_ARG(QString, ext),
                                  Q_ARG(QString, code), Q_ARG(QString, method), Q_ARG(QString, args));
        return id;
    }

    JsResult finish(quint64 id, int timeoutMs = 8000)
    {
        if (!waitFor([&] { return m_results.contains(id); }, timeoutMs)) {
            JsResult r;
            r.errorText = QStringLiteral("TEST: the call never finished");
            return r;
        }
        return m_results.take(id);
    }

    JsResult call(const QString &ext, const QString &code, const QString &method = QStringLiteral("run"),
                  const QString &args = QStringLiteral("[]"), int timeoutMs = 8000)
    {
        return finish(startCall(ext, code, method, args), timeoutMs);
    }

    void setInterceptor(JsWorker::FetchInterceptor f)
    {
        QMetaObject::invokeMethod(w(), [this, f] { w()->setFetchInterceptor(f); }, Qt::BlockingQueuedConnection);
    }

    void setLimits(int sliceMs, int callMs)
    {
        QMetaObject::invokeMethod(w(), "setLimits", Qt::BlockingQueuedConnection, Q_ARG(int, sliceMs),
                                  Q_ARG(int, callMs));
    }

private slots:

    void initTestCase()
    {
        m_wt = new JsWorkerThread(this);  // ready when the constructor returns
        connect(w(), &JsWorker::scriptFinished, this,
                [this](quint64 id, JsResult r) { m_results.insert(id, r); }, Qt::QueuedConnection);
    }

    void cleanupTestCase()
    {
        delete m_wt;
        m_wt = nullptr;
    }

    void cleanup()
    {
        setInterceptor({});
        setLimits(JsWorker::kDefaultSliceMs, JsWorker::kDefaultCallMs);
    }

    // ── globals ───────────────────────────────────────────────────────────────
    void basicArithmetic()
    {
        const JsResult r = evalOnWorker(QStringLiteral("2 + 2"));
        QVERIFY(r.ok);
        QCOMPARE(r.value.toInt(), 4);
    }

    void consoleDoesNotCrash()
    {
        const JsResult r = evalOnWorker(QStringLiteral("console.log('hello from JS'); console.warn('w'); 'ok'"));
        QVERIFY(r.ok);
        QCOMPARE(r.value.toString(), QStringLiteral("ok"));
    }

    void btoaAtobKeepUnicode()
    {
        const JsResult r = evalOnWorker(QStringLiteral("atob(btoa('h\\u00e9llo \\u65e5\\u672c'))"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QString::fromUtf8("h\xc3\xa9llo \xe6\x97\xa5\xe6\x9c\xac"));
    }

    void uriFunctionsAreTheEnginesBuiltIns()
    {
        // The old override went through Latin-1 and turned every non-Latin-1 character into '?'.
        const JsResult enc = evalOnWorker(QStringLiteral("encodeURIComponent('\\u00e9 a')"));
        QVERIFY(enc.ok);
        QCOMPARE(enc.value.toString(), QStringLiteral("%C3%A9%20a"));

        const JsResult dec = evalOnWorker(QStringLiteral("decodeURIComponent('%E6%97%A5\\u672c')"));
        QVERIFY(dec.ok);
        QCOMPARE(dec.value.toString(), QString::fromUtf8("\xe6\x97\xa5\xe6\x9c\xac"));
    }

    void urlConstructor()
    {
        const JsResult r = evalOnWorker(
            QStringLiteral("var u = new URL('https://example.com:8080/path?q=1#h'); "
                           "[u.pathname, u.origin, u.search, u.hash, u.hostname].join('|')"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("/path|https://example.com:8080|?q=1|#h|example.com"));
    }

    void jsExceptionIsReported()
    {
        const JsResult r = evalOnWorker(QStringLiteral("throw new Error('intentional')"));
        QVERIFY(!r.ok);
        QVERIFY(r.errorText.contains(QStringLiteral("intentional")));
    }

    // ── DOM ───────────────────────────────────────────────────────────────────
    void domQuerySelectorAndAll()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var doc = new DOMParser().parseFromString("
            "'<div id=\"x\">Hello</div><ul><li>A</li><li>B</li><li>C</li></ul>', 'text/html');"
            " return doc.querySelector('#x').textContent + doc.querySelectorAll('li').length; })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("Hello3"));
    }

    void domGetAttribute()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var a = new DOMParser().parseFromString('<a href=\"/manga/test\" data-x=\"1\">l</a>',"
            " 'text/html').querySelector('a');"
            " return [a.getAttribute('href'), a.getAttribute('nope'), a.hasAttribute('data-x'), a.href].join('|'); })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("/manga/test||true|/manga/test"));
    }

    // The pattern used by 17 novel sources: strip ads/scripts, then read innerHTML.
    void removedNodesDisappearFromInnerHtml()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var doc = new DOMParser().parseFromString("
            "'<div id=\"c\"><p>keep</p><script>bad()</script><div class=\"ads\">ad</div></div>', 'text/html');"
            " var c = doc.querySelector('#c');"
            " c.querySelectorAll('script, style, .ads').forEach(function(e){ e.remove(); });"
            " return c.innerHTML + '|' + c.textContent; })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("<p>keep</p>|keep"));
    }

    void setAttributeShowsInOuterAndInnerHtml()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var doc = new DOMParser().parseFromString('<div id=\"c\"><img data-x=\"1\"></div>', 'text/html');"
            " var img = doc.querySelector('img'); img.setAttribute('src', 'https://x.test/y.png');"
            " return doc.querySelector('#c').innerHTML + '|' + img.outerHTML; })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        const QString html = r.value.toString();
        QVERIFY2(html.contains(QStringLiteral("src=\"https://x.test/y.png\"")), qPrintable(html));
        QVERIFY2(html.split(QLatin1Char('|')).last().contains(QStringLiteral("src=")), qPrintable(html));
    }

    void parentAndSiblingNavigation()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var doc = new DOMParser().parseFromString("
            "'<ul><li id=\"a\">A</li><li id=\"b\">B</li></ul>', 'text/html');"
            " var a = doc.getElementById('a'), b = doc.getElementById('b');"
            " return [a.parentElement.tagName, a.nextElementSibling.id, b.previousElementSibling.id,"
            " b.nextElementSibling === null, a.parentNode.tagName].join('|'); })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("UL|b|a|true|UL"));
    }

    // ranobes.js reads childNodes[0].textContent and expects the text node, not the first element.
    void childNodesIncludeTextNodes()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var h = new DOMParser().parseFromString('<h1>Title <small>x</small></h1>', 'text/html')"
            ".querySelector('h1');"
            " return [h.childNodes[0].textContent, h.childNodes[0].nodeType, h.childNodes.length, h.children.length]"
            ".join('|'); })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("Title |3|2|1"));
    }

    void classList()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var a = new DOMParser().parseFromString('<a class=\"group  big\">x</a>', 'text/html')"
            ".querySelector('a');"
            " var out = [a.classList.contains('group'), a.classList.contains('nope')];"
            " a.classList.add('x'); a.classList.remove('big');"
            " out.push(a.className); out.push(a.classList.length); return out.join('|'); })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("true|false|group x|2"));
    }

    // An element must keep its document alive. Before, `parse(html).querySelector(..)` left the element pointing
    // at freed memory as soon as the temporary document object was released.
    void elementOutlivesItsDocument()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ function parse(h){ return new DOMParser().parseFromString(h, 'text/html'); }"
            " var s = ''; for (var i = 0; i < 200; i++) { var e = parse('<p>' + i + '</p>').querySelector('p'); s += e.textContent; }"
            " return s.length; })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toInt(), 490);  // 10*1 + 90*2 + 100*3 digits
    }

    void closestAndMatches()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var s = new DOMParser().parseFromString("
            "'<div class=\"a\"><p class=\"b\"><span id=\"s\">x</span></p></div>', 'text/html').querySelector('#s');"
            " return [s.closest('.a').tagName, s.closest('p').className, s.matches('p span'), s.matches('.a'),"
            " s.closest('.zzz')].join('|'); })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("DIV|b|true|false|"));
    }

    void documentHelpers()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var doc = new DOMParser().parseFromString("
            "'<html><head><title> My   Title </title></head><body><div id=\"x\">1</div><div>2</div></body></html>',"
            " 'text/html');"
            " return [doc.title, doc.head.tagName, doc.body.tagName, doc.getElementById('x').textContent,"
            " doc.getElementsByTagName('div').length, doc.createElement('b').tagName,"
            " doc.documentElement.tagName].join('|'); })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("My Title|HEAD|BODY|1|2|B|HTML"));
    }

    void invalidSelectorGivesNoMatchesInsteadOfThrowing()
    {
        const JsResult r = evalOnWorker(QStringLiteral(
            "(function(){ var doc = new DOMParser().parseFromString('<p>x</p>', 'text/html');"
            " return [doc.querySelector('p:bogus(') === null, doc.querySelectorAll('p:bogus(').length].join('|'); })()"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("true|0"));
    }

    // ── call contract ─────────────────────────────────────────────────────────
    void argumentsArePositional()
    {
        const JsResult r = call(QStringLiteral("pos"),
                                QStringLiteral("var extension = { run: async function (q, p) {"
                                               " await null; return { q: q, p: p, n: arguments.length }; } };"),
                                QStringLiteral("run"), QStringLiteral("[\"one piece\", 3]"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        const QVariantMap m = r.value.toMap();
        QCOMPARE(m.value(QStringLiteral("q")).toString(), QStringLiteral("one piece"));
        QCOMPARE(m.value(QStringLiteral("p")).toInt(), 3);
        QCOMPARE(m.value(QStringLiteral("n")).toInt(), 2);
    }

    void methodsRunWithExtensionAsThis()
    {
        const JsResult r = call(QStringLiteral("self"),
                                QStringLiteral("const extension = { helper: function () { return 7; },"
                                               " run: function () { return this.helper(); } };"));
        QVERIFY2(r.ok, qPrintable(r.errorText));  // also proves `const extension` is found
        QCOMPARE(r.value.toInt(), 7);
    }

    void asyncRejectionAndThrownErrorsAreReported()
    {
        JsResult r = call(QStringLiteral("rej"),
                          QStringLiteral("var extension = { run: async function () { throw new Error('async boom'); } };"));
        QVERIFY(!r.ok);
        QCOMPARE(r.errorText, QStringLiteral("async boom"));

        r = call(QStringLiteral("thr"),
                 QStringLiteral("var extension = { run: function () { throw new TypeError('sync boom'); } };"));
        QVERIFY(!r.ok);
        QCOMPARE(r.errorText, QStringLiteral("TypeError: sync boom"));
    }

    void callErrorsAreTagged()
    {
        const QString ok = QStringLiteral("var extension = { run: function () { return 1; } };");

        JsResult r = call(QStringLiteral("tag1"), ok, QStringLiteral("missing"));
        QVERIFY(!r.ok);
        QVERIFY2(r.errorText.startsWith(QStringLiteral("[call]")), qPrintable(r.errorText));

        r = call(QStringLiteral("tag2"), ok, QStringLiteral("run"), QStringLiteral("{\"not\":\"an array\"}"));
        QVERIFY(!r.ok);
        QVERIFY2(r.errorText.startsWith(QStringLiteral("[call]")), qPrintable(r.errorText));

        r = call(QStringLiteral("tag3"), QStringLiteral("this is not valid JS @@@@"));
        QVERIFY(!r.ok);
        QVERIFY2(r.errorText.startsWith(QStringLiteral("[load]")), qPrintable(r.errorText));

        r = call(QStringLiteral("tag4"), QStringLiteral("var somethingElse = 1;"));
        QVERIFY(!r.ok);
        QVERIFY2(r.errorText.startsWith(QStringLiteral("[load]")), qPrintable(r.errorText));
    }

    // ── contexts ──────────────────────────────────────────────────────────────
    void scriptIsLoadedOnceAndReloadedWhenItChanges()
    {
        const QString v1 = QStringLiteral("var n = 0; var extension = { run: function () { n += 1; return n; } };");
        QCOMPARE(call(QStringLiteral("cache"), v1).value.toInt(), 1);
        QCOMPARE(call(QStringLiteral("cache"), v1).value.toInt(), 2);  // same context, state kept

        const QString v2 = QStringLiteral("var n = 100; var extension = { run: function () { n += 1; return n; } };");
        QCOMPARE(call(QStringLiteral("cache"), v2).value.toInt(), 101);  // new code: fresh context
    }

    void extensionsDoNotShareGlobals()
    {
        QVERIFY(call(QStringLiteral("iso-a"),
                     QStringLiteral("var extension = { run: function () { leaked = 'from A'; return 'set'; } };")).ok);
        const JsResult r = call(QStringLiteral("iso-b"),
                                QStringLiteral("var extension = { run: function () { return typeof leaked; } };"));
        QVERIFY(r.ok);
        QCOMPARE(r.value.toString(), QStringLiteral("undefined"));
    }

    // ── apiFetch (fake network) ───────────────────────────────────────────────
    void apiFetchResultShape()
    {
        setInterceptor([](const QString &, const QVariantMap &, JsWorker::FakeResponse &out) {
            out.status = 200;
            out.url = QStringLiteral("https://a.test/final");
            out.text = QStringLiteral("{\"status\":\"ok\",\"items\":[1,2]}");
            return true;
        });
        const JsResult r = call(QStringLiteral("shape"), QStringLiteral(
            "var extension = { run: async function () { var r = await apiFetch('https://a.test/x');"
            " return { s: r.status, inner: r.json.status, items: r.items, isArr: Array.isArray(r.json),"
            " same: r.html === r.text, url: r.url }; } };"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        const QVariantMap m = r.value.toMap();
        QCOMPARE(m.value(QStringLiteral("s")).toInt(), 200);  // the body's "status" must not replace the HTTP status
        QCOMPARE(m.value(QStringLiteral("inner")).toString(), QStringLiteral("ok"));
        QCOMPARE(m.value(QStringLiteral("items")).toList().size(), 2);
        QCOMPARE(m.value(QStringLiteral("isArr")).toBool(), false);
        QCOMPARE(m.value(QStringLiteral("same")).toBool(), true);
        QCOMPARE(m.value(QStringLiteral("url")).toString(), QStringLiteral("https://a.test/final"));
    }

    void apiFetchKeepsTopLevelArraysAsArrays()
    {
        setInterceptor([](const QString &, const QVariantMap &, JsWorker::FakeResponse &out) {
            out.text = QStringLiteral("  [1,2,3]");
            return true;
        });
        const JsResult r = call(QStringLiteral("arr"), QStringLiteral(
            "var extension = { run: async function () { var r = await apiFetch('https://a.test/x');"
            " return [Array.isArray(r.json), r.json.length].join('|'); } };"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("true|3"));
    }

    void httpErrorStatusesResolveAndNonJsonHasNullJson()
    {
        setInterceptor([](const QString &, const QVariantMap &, JsWorker::FakeResponse &out) {
            out.status = 404;
            out.text = QStringLiteral("<html>nope</html>");
            return true;
        });
        const JsResult r = call(QStringLiteral("e404"), QStringLiteral(
            "var extension = { run: async function () { var r = await apiFetch('https://a.test/x');"
            " return [r.status, r.json === null, r.text].join('|'); } };"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("404|true|<html>nope</html>"));
    }

    void transportFailuresRejectWithATag()
    {
        setInterceptor([](const QString &, const QVariantMap &, JsWorker::FakeResponse &out) {
            out.transportError = QStringLiteral("boom");
            return true;
        });
        JsResult r = call(QStringLiteral("net1"), QStringLiteral(
            "var extension = { run: async function () {"
            " try { await apiFetch('https://a.test/x'); return 'no error'; } catch (e) { return e.message; } } };"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("[network] boom"));

        r = call(QStringLiteral("net2"), QStringLiteral(
            "var extension = { run: async function () { return await apiFetch('https://a.test/x'); } };"));
        QVERIFY(!r.ok);
        QVERIFY2(r.errorText.startsWith(QStringLiteral("[network]")), qPrintable(r.errorText));
    }

    void apiFetchPassesTheRequestThrough()
    {
        QString seenUrl;
        QVariantMap seenOpts;
        setInterceptor([&](const QString &url, const QVariantMap &opts, JsWorker::FakeResponse &out) {
            seenUrl = url;
            seenOpts = opts;
            out.text = QStringLiteral("ok");
            return true;
        });
        const QString target = QStringLiteral("https://s.test/a?b=1&c=2");
        const JsResult r = call(QStringLiteral("opts"), QStringLiteral(
            "var extension = { run: async function () {"
            " await apiFetch('/manga/proxy/html?url=' + encodeURIComponent('https://s.test/a?b=1&c=2'),"
            " { method: 'POST', headers: { 'X-Test': '1' }, body: 'a=b' }); return 'done'; } };"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(HttpService::resolveUrl(seenUrl).toString(), target);  // the proxy wrapper is unwrapped
        QCOMPARE(seenOpts.value(QStringLiteral("method")).toString(), QStringLiteral("POST"));
        QCOMPARE(seenOpts.value(QStringLiteral("headers")).toMap().value(QStringLiteral("X-Test")).toString(),
                 QStringLiteral("1"));
        QCOMPARE(seenOpts.value(QStringLiteral("body")).toString(), QStringLiteral("a=b"));
    }

    // Five calls that each wait 300 ms for the "network" must overlap. Serial execution would need 1.5 s.
    void concurrentCallsOverlapAndKeepTheirOwnResults()
    {
        setInterceptor([](const QString &, const QVariantMap &, JsWorker::FakeResponse &out) {
            out.delayMs = 300;
            out.text = QStringLiteral("{}");
            return true;
        });
        const QString code = QStringLiteral(
            "var extension = { run: async function (n) { var r = await apiFetch('https://a.test/' + n);"
            " return { n: n, s: r.status }; } };");
        constexpr int N = 5;
        QElapsedTimer timer;
        timer.start();
        QVector<quint64> ids;
        for (int k = 0; k < N; ++k)
            ids.append(startCall(QStringLiteral("conc"), code, QStringLiteral("run"), QStringLiteral("[%1]").arg(k)));
        for (int k = 0; k < N; ++k) {
            const JsResult r = finish(ids[k]);
            QVERIFY2(r.ok, qPrintable(r.errorText));
            QCOMPARE(r.value.toMap().value(QStringLiteral("n")).toInt(), k);  // nobody got someone else's result
        }
        QVERIFY2(timer.elapsed() < 1100, qPrintable(QStringLiteral("took %1 ms").arg(timer.elapsed())));
    }

    // ── limits ────────────────────────────────────────────────────────────────
    void aPromiseThatNeverSettlesTimesOut()
    {
        setLimits(10000, 300);
        const JsResult r = call(QStringLiteral("hang"), QStringLiteral(
            "var extension = { run: function () { return new Promise(function () {}); } };"));
        QVERIFY(!r.ok);
        QVERIFY2(r.errorText.startsWith(QStringLiteral("[timeout]")), qPrintable(r.errorText));
    }

    void aRunawayLoopIsInterruptedAndTheWorkerSurvives()
    {
        setLimits(300, 60000);
        JsResult r = call(QStringLiteral("loop"), QStringLiteral(
            "var extension = { run: function () { while (true) {} } };"));
        QVERIFY(!r.ok);
        QVERIFY2(r.errorText.startsWith(QStringLiteral("[timeout]")), qPrintable(r.errorText));

        r = call(QStringLiteral("after"), QStringLiteral("var extension = { run: function () { return 'alive'; } };"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QCOMPARE(r.value.toString(), QStringLiteral("alive"));
    }

    // ── real scripts through the fake network ─────────────────────────────────
    void mangadexSearchGetsQueryAndPageAsPositionalArguments()
    {
        const QString script = readScript(QStringLiteral("extensions/mangadex.js"));
        QVERIFY2(!script.isEmpty(), "run tools/sync-extensions.sh first");

        QStringList urls;
        setInterceptor([&](const QString &url, const QVariantMap &, JsWorker::FakeResponse &out) {
            urls.append(HttpService::resolveUrl(url).toString());
            out.text = QStringLiteral("{\"data\":[]}");
            return true;
        });

        JsResult r = call(QStringLiteral("mangadex"), script, QStringLiteral("search"),
                          QStringLiteral("[\"one piece\", 2]"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QVERIFY(!urls.isEmpty());
        QVERIFY2(urls.last().contains(QStringLiteral("one%20piece")) || urls.last().contains(QStringLiteral("one piece"))
                     || urls.last().contains(QStringLiteral("one+piece")),
                 qPrintable(urls.last()));
        QVERIFY2(urls.last().contains(QStringLiteral("offset=20")), qPrintable(urls.last()));

        urls.clear();
        r = call(QStringLiteral("mangadex"), script, QStringLiteral("getPopular"), QStringLiteral("[1]"));
        QVERIFY2(r.ok, qPrintable(r.errorText));
        QVERIFY(!urls.isEmpty());
        QVERIFY2(urls.last().contains(QStringLiteral("offset=0")), qPrintable(urls.last()));
    }

    // ── shutdown with work in flight ──────────────────────────────────────────
    // A request that is still pending when the worker is destroyed must be released cleanly; the engine aborts
    // on exit if any value is still referenced.
    void destroyingTheWorkerWithAPendingRequestIsClean()
    {
        auto *wt = new JsWorkerThread;
        QMetaObject::invokeMethod(wt->worker(), [wt] {
            wt->worker()->setFetchInterceptor([](const QString &, const QVariantMap &, JsWorker::FakeResponse &out) {
                out.delayMs = 5000;
                out.text = QStringLiteral("{}");
                return true;
            });
        }, Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(wt->worker(), "invokeScript", Qt::QueuedConnection, Q_ARG(quint64, 1ULL),
                                  Q_ARG(QString, QStringLiteral("pending")),
                                  Q_ARG(QString, QStringLiteral("var extension = { run: async function () {"
                                                                " return await apiFetch('https://a.test/slow'); } };")),
                                  Q_ARG(QString, QStringLiteral("run")), Q_ARG(QString, QStringLiteral("[]")));
        QTest::qWait(150);  // let the call start and park on the request
        delete wt;
        QVERIFY(true);      // reaching this line without an abort is the assertion
    }
};

QTEST_MAIN(TstJsWorker)
#include "tst_jsworker.moc"
