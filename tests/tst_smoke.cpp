#include <QtTest>

// Proves the test harness itself is wired: Qt Test links, ctest runs it, the version define reaches core.
class TstSmoke : public QObject
{
    Q_OBJECT
private slots:
    void harnessRuns() { QCOMPARE(1 + 1, 2); }

    void versionDefineIsSet()
    {
        QVERIFY(!QString::fromLatin1(MANGADL_VERSION).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstSmoke)
#include "tst_smoke.moc"
