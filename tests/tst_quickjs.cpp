#include <QtTest>

#include <cstring>

extern "C" {
#include "quickjs.h"
}

// Guards the one assumption the whole extension system rests on: the vendored quickjs-ng engine runs
// async/await and drains Promise jobs (all 40 extension scripts are async). Qt's own QJSEngine fails here.
class TstQuickJs : public QObject
{
    Q_OBJECT

    JSRuntime *m_rt = nullptr;
    JSContext *m_ctx = nullptr;

    // Evaluate `src`; returns the result converted to a string, or "EXC:<message>" if it threw.
    QString eval(const char *src)
    {
        JSValue v = JS_Eval(m_ctx, src, std::strlen(src), "<test>", JS_EVAL_TYPE_GLOBAL);
        QString out;
        if (JS_IsException(v)) {
            JSValue ex = JS_GetException(m_ctx);
            out = QStringLiteral("EXC:") + toQString(ex);
            JS_FreeValue(m_ctx, ex);
        } else {
            out = toQString(v);
        }
        JS_FreeValue(m_ctx, v);
        return out;
    }

    QString toQString(JSValueConst v)
    {
        const char *s = JS_ToCString(m_ctx, v);
        const QString out = s ? QString::fromUtf8(s) : QString();
        if (s)
            JS_FreeCString(m_ctx, s);
        return out;
    }

    // Run every queued microtask/job; returns how many ran, or -1 if one threw.
    int pumpJobs()
    {
        int ran = 0;
        JSContext *jobCtx = nullptr;
        for (;;) {
            const int rc = JS_ExecutePendingJob(m_rt, &jobCtx);
            if (rc < 0)
                return -1;
            if (rc == 0)
                return ran;
            ++ran;
        }
    }

private slots:
    void init()
    {
        m_rt = JS_NewRuntime();
        QVERIFY(m_rt);
        m_ctx = JS_NewContext(m_rt);
        QVERIFY(m_ctx);
    }

    void cleanup()
    {
        if (m_ctx)
            JS_FreeContext(m_ctx);
        if (m_rt)
            JS_FreeRuntime(m_rt);
        m_ctx = nullptr;
        m_rt = nullptr;
    }

    void asyncAwaitResolvesWhenJobsArePumped()
    {
        const QString r = eval("globalThis.out = '';"
                               "(async () => {"
                               "  const v = await Promise.resolve(42);"
                               "  globalThis.out = JSON.stringify({v: v, done: true});"
                               "})();"
                               "'started'");
        QCOMPARE(r, QStringLiteral("started"));
        // Nothing has resumed yet: the continuation is only a queued job.
        QCOMPARE(eval("globalThis.out"), QString());

        QVERIFY(pumpJobs() > 0);
        QCOMPARE(eval("globalThis.out"), QStringLiteral("{\"v\":42,\"done\":true}"));
    }

    void chainedAwaitsAndPromiseAllComplete()
    {
        eval("globalThis.out = '';"
             "async function step(n) { return await Promise.resolve(n * 2); }"
             "(async () => {"
             "  const all = await Promise.all([step(1), step(2), step(3)]);"
             "  const next = await step(all.reduce((a, b) => a + b, 0));"
             "  globalThis.out = String(next);"
             "})();");
        QVERIFY(pumpJobs() > 0);
        QCOMPARE(eval("globalThis.out"), QStringLiteral("24"));
    }

    void rejectedPromiseIsCatchableInsideTheScript()
    {
        eval("globalThis.out = '';"
             "(async () => {"
             "  try { await Promise.reject(new Error('boom')); }"
             "  catch (e) { globalThis.out = 'caught:' + e.message; }"
             "})();");
        QVERIFY(pumpJobs() >= 0);
        QCOMPARE(eval("globalThis.out"), QStringLiteral("caught:boom"));
    }

    void regexpAndJsonWork()
    {
        // Extensions lean heavily on RegExp and JSON.
        QCOMPARE(eval("'chapter-12.5'.match(/chapter-(\\d+(?:\\.\\d+)?)/)[1]"), QStringLiteral("12.5"));
        QCOMPARE(eval("JSON.stringify(JSON.parse('{\"a\":[1,2,{\"b\":null}]}'))"),
                 QStringLiteral("{\"a\":[1,2,{\"b\":null}]}"));
        QCOMPARE(eval("encodeURIComponent('a b&c')"), QStringLiteral("a%20b%26c"));
    }

    void syncExceptionIsReportedNotCrashed()
    {
        const QString r = eval("throw new Error('sync failure')");
        QVERIFY2(r.startsWith(QStringLiteral("EXC:")), qPrintable(r));
        QVERIFY2(r.contains(QStringLiteral("sync failure")), qPrintable(r));
    }
};

QTEST_GUILESS_MAIN(TstQuickJs)
#include "tst_quickjs.moc"
