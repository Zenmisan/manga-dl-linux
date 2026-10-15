#include <QtTest>
#include <QTemporaryDir>

#include "MangaDetailController.h"
#include "ReaderController.h"
#include "db/MangaDatabase.h"
#include "util/AppPaths.h"

using namespace mangadl;

class tst_controllers : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void mangaDetailControllerTest();
    void readerControllerTest();

private:
    QTemporaryDir m_tempDir;
};

void tst_controllers::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    AppPaths::setRootOverride(m_tempDir.path());
    QVERIFY(AppPaths::ensureDirs());
    QVERIFY(MangaDatabase::instance()->initialize());
}

void tst_controllers::cleanupTestCase()
{
    AppPaths::setRootOverride(QString());
}

void tst_controllers::mangaDetailControllerTest()
{
    MangaDetailController c;
    QCOMPARE(c.inLibrary(), false);

    // Initial state
    const QString prov = QStringLiteral("mangadex");
    const QString mId = QStringLiteral("one-piece-ctrl-test");

    // Add to library via controller
    // First simulate detail loaded
    c.load(prov, mId);

    // Toggle library to add
    c.addToLibrary();
    QCOMPARE(c.inLibrary(), true);
    QVERIFY(MangaDatabase::instance()->isInLibrary(prov, mId));

    // Categories
    c.setCategories({QStringLiteral("reading"), QStringLiteral("completed")});
    const QStringList cats = c.getCategories();
    QCOMPARE(cats.size(), 2);
    QVERIFY(cats.contains(QStringLiteral("reading")));

    // Mark chapter read
    c.markChapterRead(QStringLiteral("ch-1"), true);
    auto prog = MangaDatabase::instance()->getProgress(prov, mId, QStringLiteral("ch-1"));
    QVERIFY(prog.has_value());
    QCOMPARE(prog->completed, 1);

    // Mark chapter unread
    c.markChapterRead(QStringLiteral("ch-1"), false);
    prog = MangaDatabase::instance()->getProgress(prov, mId, QStringLiteral("ch-1"));
    QVERIFY(prog.has_value());
    QCOMPARE(prog->completed, 0);

    // Remove from library
    c.removeFromLibrary();
    QCOMPARE(c.inLibrary(), false);
    QVERIFY(!MangaDatabase::instance()->isInLibrary(prov, mId));
}

void tst_controllers::readerControllerTest()
{
    ReaderController rc;
    const QString prov = QStringLiteral("mangadex");
    const QString mId = QStringLiteral("solo-leveling-ctrl");
    const QString chId = QStringLiteral("ch-48");

    // Set navigation
    rc.setNavigation(QStringLiteral("ch-47"), QStringLiteral("ch-49"));
    QCOMPARE(rc.prevChapterId(), QStringLiteral("ch-47"));
    QCOMPARE(rc.nextChapterId(), QStringLiteral("ch-49"));

    // Set page bounds clamping
    rc.setCurrentPage(5);
    QCOMPARE(rc.currentPage(), 1); // total is 1 initially, clamped

    // Save progress
    rc.loadChapter(prov, mId, chId, QStringLiteral("Solo Leveling"), QStringLiteral("cover.jpg"), QStringLiteral("Chapter 48"), 48.0);
    rc.saveProgress(15, 30, false);

    auto prog = MangaDatabase::instance()->getProgress(prov, mId, chId);
    QVERIFY(prog.has_value());
    QCOMPARE(prog->page, 15);
    QCOMPARE(prog->chapterNumber, 48.0);
    QCOMPARE(prog->completed, 0);

    // Check history was recorded
    auto hist = MangaDatabase::instance()->getHistory(5);
    QVERIFY(!hist.empty());
    bool foundInHistory = false;
    for (const auto &h : hist) {
        if (h.mangaId == mId && h.chapterId == chId) {
            foundInHistory = true;
            QCOMPARE(h.mangaTitle, QStringLiteral("Solo Leveling"));
            break;
        }
    }
    QVERIFY(foundInHistory);

    // Save progress completed
    rc.saveProgress(30, 30, true);
    prog = MangaDatabase::instance()->getProgress(prov, mId, chId);
    QVERIFY(prog.has_value());
    QCOMPARE(prog->completed, 1);
}

QTEST_MAIN(tst_controllers)
#include "tst_controllers.moc"
