#include <QtTest>

#include "util/AppPaths.h"

class TstAppPaths : public QObject
{
    Q_OBJECT

    static constexpr const char *kVars[4] = {"XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME", "XDG_STATE_HOME"};
    QByteArray m_saved[4];
    bool m_wasSet[4] = {false, false, false, false};

private slots:
    void init()
    {
        for (int i = 0; i < 4; ++i) {
            m_wasSet[i] = qEnvironmentVariableIsSet(kVars[i]);
            m_saved[i] = qgetenv(kVars[i]);
        }
        AppPaths::setRootOverride({});
    }

    void cleanup()
    {
        AppPaths::setRootOverride({});
        for (int i = 0; i < 4; ++i) {
            if (m_wasSet[i])
                qputenv(kVars[i], m_saved[i]);
            else
                qunsetenv(kVars[i]);
        }
    }

    void rootOverrideRedirectsEverything()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        AppPaths::setRootOverride(tmp.path());

        const QString root = QDir::cleanPath(tmp.path());
        QCOMPARE(AppPaths::dataDir(), root + "/data");
        QCOMPARE(AppPaths::configDir(), root + "/config");
        QCOMPARE(AppPaths::cacheDir(), root + "/cache");
        QCOMPARE(AppPaths::stateDir(), root + "/state");
        QCOMPARE(AppPaths::extensionsDir(), root + "/data/extensions");
        QCOMPARE(AppPaths::databasePath(), root + "/data/manga.db");
        QCOMPARE(AppPaths::imageCacheDir(), root + "/cache/images");
        QCOMPARE(AppPaths::defaultDownloadDir(), root + "/Downloads/manga-dl");
    }

    void xdgVariablesAreHonoured()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString base = QDir::cleanPath(tmp.path());
        qputenv("XDG_DATA_HOME", (base + "/d").toLocal8Bit());
        qputenv("XDG_CONFIG_HOME", (base + "/c").toLocal8Bit());
        qputenv("XDG_CACHE_HOME", (base + "/k").toLocal8Bit());
        qputenv("XDG_STATE_HOME", (base + "/s").toLocal8Bit());

        QCOMPARE(AppPaths::dataDir(), base + "/d/manga-dl");
        QCOMPARE(AppPaths::configDir(), base + "/c/manga-dl");
        QCOMPARE(AppPaths::cacheDir(), base + "/k/manga-dl");
        QCOMPARE(AppPaths::stateDir(), base + "/s/manga-dl");
    }

    void relativeXdgValuesAreIgnored()
    {
        // The XDG spec says a relative path in these variables is invalid and must be ignored.
        qputenv("XDG_DATA_HOME", "relative/path");
        qunsetenv("XDG_STATE_HOME");

        const QString home = QDir::cleanPath(QDir::homePath());
        QCOMPARE(AppPaths::dataDir(), home + "/.local/share/manga-dl");
        QCOMPARE(AppPaths::stateDir(), home + "/.local/state/manga-dl");
    }

    void ensureDirsCreatesTheTree()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        AppPaths::setRootOverride(tmp.path());

        QVERIFY(AppPaths::ensureDirs());
        QVERIFY(QFileInfo(AppPaths::dataDir()).isDir());
        QVERIFY(QFileInfo(AppPaths::configDir()).isDir());
        QVERIFY(QFileInfo(AppPaths::cacheDir()).isDir());
        QVERIFY(QFileInfo(AppPaths::stateDir()).isDir());
        QVERIFY(QFileInfo(AppPaths::extensionsDir()).isDir());
        QVERIFY(QFileInfo(AppPaths::imageCacheDir()).isDir());

        // Calling it again on an existing tree must still succeed.
        QVERIFY(AppPaths::ensureDirs());
    }
};

QTEST_GUILESS_MAIN(TstAppPaths)
#include "tst_apppaths.moc"
