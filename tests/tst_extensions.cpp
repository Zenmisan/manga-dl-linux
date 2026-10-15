// tst_extensions.cpp - ExtensionManager against the REAL extension scripts (with a fake network)
#include <QtTest>

#include "extensions/ExtensionManager.h"
#include "http/HttpService.h"

namespace {

bool waitFor(const std::function<bool()> &cond, int msec = 15000)
{
    const QDeadlineTimer deadline(msec);
    while (!deadline.hasExpired()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (cond())
            return true;
    }
    return cond();
}

} // namespace

class TstExtensions : public QObject
{
    Q_OBJECT

    ExtensionManager *m_mgr = nullptr;

    void setInterceptor(JsWorker::FetchInterceptor f)
    {
        JsWorker *w = m_mgr->worker();
        QMetaObject::invokeMethod(w, [w, f] { w->setFetchInterceptor(f); }, Qt::BlockingQueuedConnection);
    }

private slots:

    void initTestCase()
    {
        m_mgr = new ExtensionManager(this);
        const int added = m_mgr->addScriptsFrom(QStringLiteral(MANGADL_ASSETS_DIR "/extensions"), true);
        QVERIFY2(added > 0, "no scripts found: run tools/sync-extensions.sh first");
    }

    void cleanup() { setInterceptor({}); }

    // ── registry ──────────────────────────────────────────────────────────────
    // The audit found 40 files = 38 sources + 2 templates, and 23 scripts that define getChapterText.
    void sourcesAreRegisteredAndClassifiedFromTheirScripts()
    {
        const QList<ExtensionInfo> all = m_mgr->extensions();
        QCOMPARE(int(all.size()), 38);

        int novels = 0;
        for (const ExtensionInfo &info : all)
            novels += info.type == ExtensionType::Novel ? 1 : 0;
        QCOMPARE(novels, 23);

        QVERIFY(m_mgr->findById(QStringLiteral("mangadex")));
        QCOMPARE(m_mgr->findById(QStringLiteral("mangadex"))->type, ExtensionType::Manga);
        QVERIFY(m_mgr->findById(QStringLiteral("royalroad")));
        QCOMPARE(m_mgr->findById(QStringLiteral("royalroad"))->type, ExtensionType::Novel);
        QVERIFY2(!m_mgr->findById(QStringLiteral("madara")), "templates are generators, not sources");
        QVERIFY(!m_mgr->findById(QStringLiteral("mangathemesia")));
    }

    void addingTheSameDirectoryTwiceAddsNothing()
    {
        QCOMPARE(m_mgr->addScriptsFrom(QStringLiteral(MANGADL_ASSETS_DIR "/extensions"), true), 0);
        QCOMPARE(int(m_mgr->extensions().size()), 38);
    }

    // ── every real script loads ───────────────────────────────────────────────
    // Each script is evaluated in its own context and asked for its popular list against an empty answer. A script
    // may legitimately fail to parse an empty page (reported below for information), but it must load, expose
    // `extension`, define getPopular, and finish.
    void everyScriptLoadsAndFinishes()
    {
        setInterceptor([](const QString &, const QVariantMap &, JsWorker::FakeResponse &out) {
            out.text = QStringLiteral("{}");
            return true;
        });

        QStringList bad;
        QStringList parseFailures;
        int clean = 0;
        for (const ExtensionInfo &info : m_mgr->extensions()) {
            bool done = false;
            SourceError error;
            bool hadError = false;
            m_mgr->getPopular(info.id, 1, [&](QList<MangaPreview>, SourceError e) {
                error = e;
                hadError = !e.message.isEmpty();
                done = true;
            });
            if (!waitFor([&] { return done; })) {
                bad.append(info.id + QStringLiteral(": no callback"));
                continue;
            }
            if (!hadError)
                ++clean;
            else if (error.kind == SourceErrorKind::ScriptLoadError || error.kind == SourceErrorKind::Timeout)
                bad.append(info.id + QStringLiteral(": ") + error.message);
            else
                parseFailures.append(info.id + QStringLiteral(": ") + error.message);
        }
        qInfo().noquote() << "scripts that handled an empty answer cleanly:" << clean
                          << "- threw on it:" << parseFailures.size();
        for (const QString &p : std::as_const(parseFailures))
            qInfo().noquote() << "  threw on an empty answer:" << p;
        QVERIFY2(bad.isEmpty(), qPrintable(bad.join(QLatin1Char('\n'))));
    }

    // ── call plumbing ─────────────────────────────────────────────────────────
    void searchPassesQueryAndPageToTheScript()
    {
        QString seen;
        setInterceptor([&](const QString &url, const QVariantMap &, JsWorker::FakeResponse &out) {
            seen = HttpService::resolveUrl(url).toString();
            out.text = QStringLiteral("{\"data\":[]}");
            return true;
        });
        bool done = false;
        SourceError error;
        m_mgr->search(QStringLiteral("mangadex"), QStringLiteral("one piece"), 2,
                      [&](QList<MangaPreview>, SourceError e) {
                          error = e;
                          done = true;
                      });
        QVERIFY(waitFor([&] { return done; }));
        QVERIFY2(error.message.isEmpty(), qPrintable(error.message));
        QVERIFY2(seen.contains(QStringLiteral("offset=20")), qPrintable(seen));
        QVERIFY2(seen.contains(QStringLiteral("piece")), qPrintable(seen));
    }

    void failuresBeforeAnyScriptRunsAreStillAsynchronousAndTagged()
    {
        bool returned = false;
        bool calledEarly = false;
        bool done = false;
        SourceError error;
        m_mgr->search(QStringLiteral("no-such-extension"), QStringLiteral("x"), 1,
                      [&](QList<MangaPreview>, SourceError e) {
                          calledEarly = !returned;
                          error = e;
                          done = true;
                      });
        returned = true;
        QVERIFY(waitFor([&] { return done; }));
        QVERIFY2(!calledEarly, "a callback must never run before the call has returned");
        QCOMPARE(error.kind, SourceErrorKind::ScriptLoadError);
        QVERIFY(error.message.contains(QStringLiteral("no-such-extension")));
    }

    // ── error classification ──────────────────────────────────────────────────
    void tagsMapToErrorKinds()
    {
        QCOMPARE(SourceError::fromText(QStringLiteral("[network] connection refused")).kind,
                 SourceErrorKind::NetworkError);
        QCOMPARE(SourceError::fromText(QStringLiteral("[network] connection refused")).message,
                 QStringLiteral("connection refused"));
        QCOMPARE(SourceError::fromText(QStringLiteral("[timeout] too slow")).kind, SourceErrorKind::Timeout);
        QCOMPARE(SourceError::fromText(QStringLiteral("[load] bad script")).kind, SourceErrorKind::ScriptLoadError);
        QCOMPARE(SourceError::fromText(QStringLiteral("[call] no such method")).kind, SourceErrorKind::ScriptLoadError);

        const SourceError thrown = SourceError::fromText(QStringLiteral("TypeError: x is undefined"),
                                                         SourceErrorKind::ParseError);
        QCOMPARE(thrown.kind, SourceErrorKind::ParseError);  // an exception thrown by the script
        QCOMPARE(thrown.message, QStringLiteral("TypeError: x is undefined"));
    }
};

QTEST_MAIN(TstExtensions)
#include "tst_extensions.moc"
