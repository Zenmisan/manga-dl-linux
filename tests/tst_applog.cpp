#include <QtTest>

#include "util/AppLog.h"

class TstAppLog : public QObject
{
    Q_OBJECT

    static QString readAll(const QString &path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        return QString::fromUtf8(f.readAll());
    }

private slots:
    void cleanup() { AppLog::shutdown(); }

    void writesMessagesToTheLogFile()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(AppLog::init(tmp.path()));

        QVERIFY(AppLog::logFilePath().endsWith(QStringLiteral("manga-dl.log")));
        qInfo("marker-info-12345");
        qWarning("marker-warn-67890");
        AppLog::shutdown();

        const QString text = readAll(QDir(tmp.path()).filePath(QStringLiteral("manga-dl.log")));
        QVERIFY2(text.contains(QStringLiteral("[info]")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("marker-info-12345")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("[warn]")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("marker-warn-67890")), qPrintable(text));
    }

    void createsMissingDirectory()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString nested = QDir(tmp.path()).filePath(QStringLiteral("a/b/c"));
        QVERIFY(AppLog::init(nested));
        QVERIFY(QFileInfo(nested).isDir());
    }

    void rotatesAndKeepsAtMostTheConfiguredFiles()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        // Tiny limit so a few lines force several rotations; keep = 3 -> .log, .log.1, .log.2 only.
        QVERIFY(AppLog::init(tmp.path(), 400, 3));
        for (int i = 0; i < 60; ++i)
            qInfo("rotation line number %d padded to be reasonably long for the size limit", i);
        AppLog::shutdown();

        const QDir dir(tmp.path());
        QVERIFY(dir.exists(QStringLiteral("manga-dl.log")));
        QVERIFY(dir.exists(QStringLiteral("manga-dl.log.1")));
        QVERIFY(dir.exists(QStringLiteral("manga-dl.log.2")));
        QVERIFY2(!dir.exists(QStringLiteral("manga-dl.log.3")), "more rotated files than 'keep' allows");

        // The newest line must survive rotation. It sits in the live file, or in .1 if that very line
        // pushed the file over the limit and triggered the rotation.
        const QString recent = readAll(dir.filePath(QStringLiteral("manga-dl.log")))
            + readAll(dir.filePath(QStringLiteral("manga-dl.log.1")));
        QVERIFY(recent.contains(QStringLiteral("number 59")));
    }

    void appendsAcrossRestarts()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(AppLog::init(tmp.path()));
        qInfo("first-run-marker");
        AppLog::shutdown();

        QVERIFY(AppLog::init(tmp.path()));
        qInfo("second-run-marker");
        AppLog::shutdown();

        const QString text = readAll(QDir(tmp.path()).filePath(QStringLiteral("manga-dl.log")));
        QVERIFY(text.contains(QStringLiteral("first-run-marker")));
        QVERIFY(text.contains(QStringLiteral("second-run-marker")));
    }
};

QTEST_GUILESS_MAIN(TstAppLog)
#include "tst_applog.moc"
