// tst_r3.cpp - replay recorded fixtures against every real extension (search → detail → chapter).
#include <QtTest>

#include "R3Support.h"

#include <QDir>

class TstR3 : public QObject
{
    Q_OBJECT

    ExtensionManager *m_mgr = nullptr;
    QString m_root;

    void installReplay(const R3Pack &pack)
    {
        JsWorker *w = m_mgr->worker();
        auto interceptor = r3ReplayInterceptor(pack);
        QMetaObject::invokeMethod(w, [w, interceptor] { w->setFetchInterceptor(interceptor); },
                                  Qt::BlockingQueuedConnection);
    }

private slots:
    void initTestCase()
    {
        m_root = QStringLiteral(MANGADL_FIXTURES_DIR);
        m_mgr = new ExtensionManager(this);
        QVERIFY2(m_mgr->addScriptsFrom(QStringLiteral(MANGADL_ASSETS_DIR "/extensions"), true) > 0,
                 "run tools/sync-extensions.sh first");
        // Nothing recorded yet is not a failure of the code: skip. Once the directory exists, a source without a
        // pack (or a pack that no longer replays) fails below.
        if (!QDir(m_root).exists())
            QSKIP("no fixtures recorded yet: run r3-capture (needs network) to create tests/fixtures/r3");
    }

    void cleanup()
    {
        JsWorker *w = m_mgr->worker();
        QMetaObject::invokeMethod(w, [w] { w->setFetchInterceptor({}); }, Qt::BlockingQueuedConnection);
    }

    void everyBuiltinHasAFixturePack_data()
    {
        QTest::addColumn<QString>("id");
        for (const ExtensionInfo &info : m_mgr->extensions())
            QTest::newRow(qPrintable(info.id)) << info.id;
    }

    void everyBuiltinHasAFixturePack()
    {
        QFETCH(QString, id);
        const QString dir = QDir(m_root).filePath(id);
        QVERIFY2(QFile::exists(QDir(dir).filePath(QStringLiteral("manifest.json"))),
                 qPrintable(QStringLiteral("missing fixture: run r3-capture (looked in %1)").arg(dir)));
    }

    void replaySearchDetailChapter_data()
    {
        QTest::addColumn<QString>("id");
        for (const ExtensionInfo &info : m_mgr->extensions())
            QTest::newRow(qPrintable(info.id)) << info.id;
    }

    void replaySearchDetailChapter()
    {
        QFETCH(QString, id);
        const QString dir = QDir(m_root).filePath(id);
        R3Pack recorded;
        QString err;
        QVERIFY2(r3LoadPack(dir, &recorded, &err), qPrintable(err));
        if (!recorded.search.ok || !recorded.detail.ok || !recorded.chapter.ok) {
            QSKIP(qPrintable(QStringLiteral("%1 was recorded as a failure: search=%2 detail=%3 chapter=%4")
                                 .arg(id, recorded.search.error, recorded.detail.error, recorded.chapter.error)));
        }

        installReplay(recorded);
        R3Pack replay;
        const ExtensionInfo *info = m_mgr->findById(id);
        QVERIFY(info);
        r3RunPipeline(m_mgr, id, info->type == ExtensionType::Novel, recorded.query, &replay);

        QVERIFY2(replay.search.ok, qPrintable(replay.search.error));
        QCOMPARE(replay.search.count, recorded.search.count);
        QCOMPARE(replay.search.id, recorded.search.id);
        QCOMPARE(replay.search.title, recorded.search.title);

        QVERIFY2(replay.detail.ok, qPrintable(replay.detail.error));
        QCOMPARE(replay.detail.count, recorded.detail.count);
        QCOMPARE(replay.detail.title, recorded.detail.title);

        QVERIFY2(replay.chapter.ok, qPrintable(replay.chapter.error));
        QCOMPARE(replay.chapter.count, recorded.chapter.count);
    }
};

QTEST_MAIN(TstR3)
#include "tst_r3.moc"
