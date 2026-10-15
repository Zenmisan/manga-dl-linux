#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "settings/AppSettings.h"

using namespace mangadl;

class tst_settings : public QObject {
    Q_OBJECT

private slots:
    void defaultValues();
    void gettersSettersAndSignals();
    void hasSupabase();
    void persistenceAcrossInstances();
    void resetToDefaults();
};

void tst_settings::defaultValues()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString iniPath = tempDir.path() + QStringLiteral("/settings.ini");

    AppSettings settings(iniPath);

    QCOMPARE(settings.theme(), QStringLiteral("Dark"));
    QCOMPARE(settings.accentColor(), QStringLiteral("Red"));
    QCOMPARE(settings.ambilight(), true);

    QCOMPARE(settings.backendUrl(), QStringLiteral("https://manga-dl.onrender.com"));
    QCOMPARE(settings.supabaseUrl(), QStringLiteral("https://gyivwfweldwvzccbpgoz.supabase.co"));
    QVERIFY(!settings.supabaseAnonKey().isEmpty());
    QVERIFY(settings.hasSupabase());

    QCOMPARE(settings.readerMode(), QStringLiteral("Right to left"));
    QCOMPARE(settings.readingDirection(), QStringLiteral("RTL"));
    QCOMPARE(settings.twoPageSpread(), true);
    QCOMPARE(settings.webtoonSidePadding(), 15);
    QCOMPARE(settings.clickZones(), true);
    QCOMPARE(settings.imageBrightness(), 0);
    QCOMPARE(settings.sharpenImages(), true);

    QCOMPARE(settings.gridColumns(), QStringLiteral("Auto"));
    QCOMPARE(settings.unreadBadges(), true);
    QCOMPARE(settings.downloadedBadges(), true);
    QCOMPARE(settings.continueReadingRow(), true);
    QCOMPARE(settings.checkNewChapters(), QStringLiteral("Every 12 hours"));

    QCOMPARE(settings.parallelDownloads(), 3);
    QCOMPARE(settings.downloadFormat(), QStringLiteral("cbz"));

    QCOMPARE(settings.anilistClientId(), QStringLiteral("53109"));
    QCOMPARE(settings.malClientId(), QStringLiteral("4ee644500f513dd7887120b026b65f39"));
    QCOMPARE(settings.autoSyncTrackers(), true);
}

void tst_settings::gettersSettersAndSignals()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString iniPath = tempDir.path() + QStringLiteral("/settings.ini");

    AppSettings settings(iniPath);

    QSignalSpy themeSpy(&settings, &AppSettings::themeChanged);
    QSignalSpy generalSpy(&settings, &AppSettings::settingsChanged);

    settings.setTheme(QStringLiteral("AMOLED"));
    QCOMPARE(settings.theme(), QStringLiteral("AMOLED"));
    QCOMPARE(themeSpy.count(), 1);
    QCOMPARE(generalSpy.count(), 1);

    // Setting the same value doesn't re-emit
    settings.setTheme(QStringLiteral("AMOLED"));
    QCOMPARE(themeSpy.count(), 1);

    QSignalSpy paddingSpy(&settings, &AppSettings::webtoonSidePaddingChanged);
    settings.setWebtoonSidePadding(25);
    QCOMPARE(settings.webtoonSidePadding(), 25);
    QCOMPARE(paddingSpy.count(), 1);

    settings.setIncognito(true);
    QVERIFY(settings.incognito());

    settings.setUserEmail(QStringLiteral("user@example.com"));
    QCOMPARE(settings.userEmail(), QStringLiteral("user@example.com"));

    settings.setDisplayName(QStringLiteral("Alice"));
    QCOMPARE(settings.displayName(), QStringLiteral("Alice"));
}

void tst_settings::hasSupabase()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString iniPath = tempDir.path() + QStringLiteral("/settings.ini");

    AppSettings settings(iniPath);
    QVERIFY(settings.hasSupabase());

    settings.setSupabaseUrl(QStringLiteral(""));
    QVERIFY(!settings.hasSupabase());

    settings.setSupabaseUrl(QStringLiteral("https://custom.supabase.co"));
    QVERIFY(settings.hasSupabase());

    settings.setSupabaseAnonKey(QStringLiteral(""));
    QVERIFY(!settings.hasSupabase());
}

void tst_settings::persistenceAcrossInstances()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString iniPath = tempDir.path() + QStringLiteral("/settings.ini");

    {
        AppSettings settings(iniPath);
        settings.setAccentColor(QStringLiteral("Indigo"));
        settings.setWebtoonSidePadding(30);
        settings.setReaderMode(QStringLiteral("Webtoon"));
        settings.setUserId(QStringLiteral("usr_12345"));
        settings.sync();
    }

    {
        AppSettings loaded(iniPath);
        QCOMPARE(loaded.accentColor(), QStringLiteral("Indigo"));
        QCOMPARE(loaded.webtoonSidePadding(), 30);
        QCOMPARE(loaded.readerMode(), QStringLiteral("Webtoon"));
        QCOMPARE(loaded.userId(), QStringLiteral("usr_12345"));
    }
}

void tst_settings::resetToDefaults()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString iniPath = tempDir.path() + QStringLiteral("/settings.ini");

    AppSettings settings(iniPath);
    settings.setTheme(QStringLiteral("AMOLED"));
    settings.setAccentColor(QStringLiteral("Emerald"));
    settings.setWebtoonSidePadding(35);
    settings.setReaderMode(QStringLiteral("Vertical"));

    settings.resetToDefaults();

    QCOMPARE(settings.theme(), QStringLiteral("Dark"));
    QCOMPARE(settings.accentColor(), QStringLiteral("Red"));
    QCOMPARE(settings.webtoonSidePadding(), 15);
    QCOMPARE(settings.readerMode(), QStringLiteral("Right to left"));
}

QTEST_MAIN(tst_settings)
#include "tst_settings.moc"
