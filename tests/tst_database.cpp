#include <QtTest>
#include "db/MangaDatabase.h"
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QThread>
#include <thread>
#include <vector>

using namespace mangadl;

class tst_database : public QObject {
    Q_OBJECT

private slots:
    void initAndMigrations();
    void libraryCrud();
    void readingProgressAndUnread();
    void historyUpsert();
    void downloadsCrud();
    void categoriesAndAssignment();
    void trackerBinds();
    void newChapters();
    void persistenceAcrossRestart();
    void concurrency();
};

void tst_database::initAndMigrations()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.path() + QStringLiteral("/manga.db");

    MangaDatabase db(dbPath);
    QVERIFY(db.initialize());
    QVERIFY(db.isInitialized());

    // Second initialize is idempotent
    QVERIFY(db.initialize());

    // Default categories were created
    auto categories = db.getCategories();
    QCOMPARE(categories.size(), 4);
    QCOMPARE(categories[0].id, QStringLiteral("reading"));
    QCOMPARE(categories[0].name, QStringLiteral("Reading"));
    QCOMPARE(categories[1].id, QStringLiteral("plan_to_read"));
    QCOMPARE(categories[2].id, QStringLiteral("completed"));
    QCOMPARE(categories[3].id, QStringLiteral("local_files"));
}

void tst_database::libraryCrud()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    QVERIFY(!db.isInLibrary(QStringLiteral("mangadex"), QStringLiteral("one-piece")));

    LibraryEntity item;
    item.provider = QStringLiteral("mangadex");
    item.mangaId = QStringLiteral("one-piece");
    item.title = QStringLiteral("One Piece");
    item.coverUrl = QStringLiteral("https://example.com/op.jpg");
    item.totalChapters = 1100;
    QVERIFY(db.addToLibrary(item));

    QVERIFY(db.isInLibrary(QStringLiteral("mangadex"), QStringLiteral("one-piece")));

    auto fetched = db.getLibraryItem(QStringLiteral("mangadex"), QStringLiteral("one-piece"));
    QVERIFY(fetched.has_value());
    QCOMPARE(fetched->title, QStringLiteral("One Piece"));
    QCOMPARE(fetched->totalChapters, 1100);
    QCOMPARE(fetched->id, QStringLiteral("mangadex/one-piece"));

    // Update total chapters
    QVERIFY(db.updateTotalChapters(QStringLiteral("mangadex"), QStringLiteral("one-piece"), 1105));
    fetched = db.getLibraryItem(QStringLiteral("mangadex"), QStringLiteral("one-piece"));
    QCOMPARE(fetched->totalChapters, 1105);

    // List library
    auto list = db.getLibrary();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list[0].mangaId, QStringLiteral("one-piece"));

    // Remove
    QVERIFY(db.removeFromLibrary(QStringLiteral("mangadex"), QStringLiteral("one-piece")));
    QVERIFY(!db.isInLibrary(QStringLiteral("mangadex"), QStringLiteral("one-piece")));
    QCOMPARE(db.getLibrary().size(), 0);
}

void tst_database::readingProgressAndUnread()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    LibraryEntity item;
    item.provider = QStringLiteral("mangadex");
    item.mangaId = QStringLiteral("sololeveling");
    item.title = QStringLiteral("Solo Leveling");
    item.totalChapters = 10;
    QVERIFY(db.addToLibrary(item));

    QCOMPARE(db.getUnreadCount(QStringLiteral("mangadex"), QStringLiteral("sololeveling")), 10);

    // Read chapter 1 (page 15, not completed)
    QVERIFY(db.saveProgress(QStringLiteral("mangadex"), QStringLiteral("sololeveling"), QStringLiteral("ch-1"), 1.0, 15, false));
    auto p1 = db.getProgress(QStringLiteral("mangadex"), QStringLiteral("sololeveling"), QStringLiteral("ch-1"));
    QVERIFY(p1.has_value());
    QCOMPARE(p1->page, 15);
    QCOMPARE(p1->completed, 0);
    QCOMPARE(db.getUnreadCount(QStringLiteral("mangadex"), QStringLiteral("sololeveling")), 10);

    // Complete chapter 1
    QVERIFY(db.saveProgress(QStringLiteral("mangadex"), QStringLiteral("sololeveling"), QStringLiteral("ch-1"), 1.0, 20, true));
    p1 = db.getProgress(QStringLiteral("mangadex"), QStringLiteral("sololeveling"), QStringLiteral("ch-1"));
    QCOMPARE(p1->completed, 1);
    QCOMPARE(db.getUnreadCount(QStringLiteral("mangadex"), QStringLiteral("sololeveling")), 9);

    // Complete chapter 2
    QVERIFY(db.saveProgress(QStringLiteral("mangadex"), QStringLiteral("sololeveling"), QStringLiteral("ch-2"), 2.0, 18, true));
    QCOMPARE(db.getUnreadCount(QStringLiteral("mangadex"), QStringLiteral("sololeveling")), 8);

    auto allProg = db.getMangaProgress(QStringLiteral("mangadex"), QStringLiteral("sololeveling"));
    QCOMPARE(allProg.size(), 2);
    QCOMPARE(db.getAllCompletedProgress().size(), 2);
}

void tst_database::historyUpsert()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    QVERIFY(db.recordHistory(QStringLiteral("mangadex"), QStringLiteral("m1"), QStringLiteral("Title 1"),
                             QStringLiteral("ch1"), QStringLiteral("Chapter 1"), QStringLiteral("cover1.jpg")));
    QThread::msleep(5);
    QVERIFY(db.recordHistory(QStringLiteral("mangadex"), QStringLiteral("m2"), QStringLiteral("Title 2"),
                             QStringLiteral("ch1"), QStringLiteral("Chapter 1"), QStringLiteral("cover2.jpg")));

    auto history = db.getHistory(10);
    QCOMPARE(history.size(), 2);
    // Most recent is m2
    QCOMPARE(history[0].mangaId, QStringLiteral("m2"));
    QCOMPARE(history[1].mangaId, QStringLiteral("m1"));

    // Re-recording m1 updates its timestamp, making it most recent
    QThread::msleep(10);
    QVERIFY(db.recordHistory(QStringLiteral("mangadex"), QStringLiteral("m1"), QStringLiteral("Title 1"),
                             QStringLiteral("ch1"), QStringLiteral("Chapter 1 Updated"), QStringLiteral("cover1.jpg")));
    history = db.getHistory(10);
    QCOMPARE(history.size(), 2); // Upserted, not duplicated
    QCOMPARE(history[0].mangaId, QStringLiteral("m1"));
    QCOMPARE(history[0].chapterTitle, QStringLiteral("Chapter 1 Updated"));

    // Remove single item
    QVERIFY(db.removeFromHistory(QStringLiteral("mangadex"), QStringLiteral("m2"), QStringLiteral("ch1")));
    QCOMPARE(db.getHistory(10).size(), 1);

    // Clear history
    QVERIFY(db.clearHistory());
    QCOMPARE(db.getHistory(10).size(), 0);
}

void tst_database::downloadsCrud()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    DownloadEntity dl;
    dl.provider = QStringLiteral("asura");
    dl.mangaId = QStringLiteral("returner");
    dl.mangaTitle = QStringLiteral("The Returner");
    dl.chapterId = QStringLiteral("101");
    dl.chapterTitle = QStringLiteral("Episode 101");
    dl.chapterNumber = 101.0;
    dl.status = QStringLiteral("queued");
    QVERIFY(db.enqueueDownload(dl));

    const QString dlId = QStringLiteral("asura/returner/101");
    auto fetched = db.getDownload(QStringLiteral("asura"), QStringLiteral("returner"), QStringLiteral("101"));
    QVERIFY(fetched.has_value());
    QCOMPARE(fetched->status, QStringLiteral("queued"));

    // Update progress
    QVERIFY(db.updateDownloadProgress(dlId, 15, 30, QStringLiteral("downloading")));
    fetched = db.getDownloadById(dlId);
    QCOMPARE(fetched->progress, 15);
    QCOMPARE(fetched->totalPages, 30);
    QCOMPARE(fetched->status, QStringLiteral("downloading"));
    QCOMPARE(fetched->completedAt, 0);

    // Complete download
    QVERIFY(db.updateDownloadProgress(dlId, 30, 30, QStringLiteral("completed")));
    QVERIFY(db.updateDownloadCbzPath(dlId, QStringLiteral("/home/user/Downloads/ch101.cbz")));
    fetched = db.getDownloadById(dlId);
    QCOMPARE(fetched->status, QStringLiteral("completed"));
    QVERIFY(fetched->completedAt > 0);
    QCOMPARE(fetched->cbzPath, QStringLiteral("/home/user/Downloads/ch101.cbz"));

    // Clear completed
    QVERIFY(db.clearCompletedDownloads());
    QCOMPARE(db.getDownloads().size(), 0);
}

void tst_database::categoriesAndAssignment()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    // Add custom category
    QVERIFY(db.addCategory(QStringLiteral("favorites"), QStringLiteral("Favorites"), 10));
    auto cat = db.getCategory(QStringLiteral("favorites"));
    QVERIFY(cat.has_value());
    QCOMPARE(cat->name, QStringLiteral("Favorites"));

    // Assign categories to a manga
    QVERIFY(db.setMangaCategories(QStringLiteral("mangadex"), QStringLiteral("bleach"),
                                  {QStringLiteral("reading"), QStringLiteral("favorites")}));

    auto assigned = db.getMangaCategories(QStringLiteral("mangadex"), QStringLiteral("bleach"));
    QCOMPARE(assigned.size(), 2);
    QVERIFY(assigned.contains(QStringLiteral("reading")));
    QVERIFY(assigned.contains(QStringLiteral("favorites")));

    QCOMPARE(db.getMangaCountInCategory(QStringLiteral("favorites")), 1);

    // Deleting category cascades and removes mappings
    QVERIFY(db.deleteCategory(QStringLiteral("favorites")));
    assigned = db.getMangaCategories(QStringLiteral("mangadex"), QStringLiteral("bleach"));
    QCOMPARE(assigned.size(), 1);
    QCOMPARE(assigned[0], QStringLiteral("reading"));
}

void tst_database::trackerBinds()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    TrackerBindEntity bind;
    bind.provider = QStringLiteral("mangadex");
    bind.mangaId = QStringLiteral("naruto");
    bind.tracker = QStringLiteral("AniList");
    bind.remoteId = 12345;
    bind.remoteTitle = QStringLiteral("Naruto (Remote)");
    bind.lastChapterRead = 700.0;
    QVERIFY(db.saveTrackerBind(bind));

    auto fetched = db.getTrackerBind(QStringLiteral("mangadex"), QStringLiteral("naruto"), QStringLiteral("AniList"));
    QVERIFY(fetched.has_value());
    QCOMPARE(fetched->remoteId, 12345);
    QCOMPARE(fetched->lastChapterRead, 700.0);

    QCOMPARE(db.getTrackerBinds(QStringLiteral("mangadex"), QStringLiteral("naruto")).size(), 1);

    QVERIFY(db.deleteTrackerBind(QStringLiteral("mangadex"), QStringLiteral("naruto"), QStringLiteral("AniList")));
    QCOMPARE(db.getTrackerBinds(QStringLiteral("mangadex"), QStringLiteral("naruto")).size(), 0);
}

void tst_database::newChapters()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    NewChapterEntity c1;
    c1.mangaId = QStringLiteral("m1");
    c1.mangaTitle = QStringLiteral("Title 1");
    c1.provider = QStringLiteral("mangadex");
    c1.chapterId = QStringLiteral("c1");
    c1.chapterTitle = QStringLiteral("Chapter 1");
    c1.chapterNumber = 1.0;

    NewChapterEntity c2;
    c2.mangaId = QStringLiteral("m2");
    c2.mangaTitle = QStringLiteral("Title 2");
    c2.provider = QStringLiteral("mangadex");
    c2.chapterId = QStringLiteral("c2");
    c2.chapterTitle = QStringLiteral("Chapter 2");
    c2.chapterNumber = 2.0;

    QVERIFY(db.saveNewChapters({c1, c2}));
    auto chapters = db.getNewChapters();
    QCOMPARE(chapters.size(), 2);

    QVERIFY(db.removeNewChapter(QStringLiteral("mangadex/m1/c1")));
    QCOMPARE(db.getNewChapters().size(), 1);

    QVERIFY(db.clearNewChapters());
    QCOMPARE(db.getNewChapters().size(), 0);
}

void tst_database::persistenceAcrossRestart()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.path() + QStringLiteral("/persistent.db");

    // Phase 1: Open, write data, close
    {
        MangaDatabase db(dbPath);
        QVERIFY(db.initialize());

        LibraryEntity item;
        item.provider = QStringLiteral("mangadex");
        item.mangaId = QStringLiteral("persisted-manga");
        item.title = QStringLiteral("Persisted Title");
        item.totalChapters = 50;
        QVERIFY(db.addToLibrary(item));

        QVERIFY(db.saveProgress(QStringLiteral("mangadex"), QStringLiteral("persisted-manga"),
                                QStringLiteral("ch-1"), 1.0, 10, true));

        QVERIFY(db.recordHistory(QStringLiteral("mangadex"), QStringLiteral("persisted-manga"),
                                 QStringLiteral("Persisted Title"), QStringLiteral("ch-1"),
                                 QStringLiteral("Chapter 1"), QStringLiteral("cover.jpg")));
        db.close();
    }

    // Phase 2: Reopen from same file, verify everything persisted
    {
        MangaDatabase db(dbPath);
        QVERIFY(db.initialize());

        QVERIFY(db.isInLibrary(QStringLiteral("mangadex"), QStringLiteral("persisted-manga")));
        auto item = db.getLibraryItem(QStringLiteral("mangadex"), QStringLiteral("persisted-manga"));
        QVERIFY(item.has_value());
        QCOMPARE(item->title, QStringLiteral("Persisted Title"));
        QCOMPARE(item->totalChapters, 50);

        auto prog = db.getProgress(QStringLiteral("mangadex"), QStringLiteral("persisted-manga"), QStringLiteral("ch-1"));
        QVERIFY(prog.has_value());
        QCOMPARE(prog->completed, 1);
        QCOMPARE(prog->page, 10);

        auto history = db.getHistory(10);
        QCOMPARE(history.size(), 1);
        QCOMPARE(history[0].mangaId, QStringLiteral("persisted-manga"));
    }
}

void tst_database::concurrency()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/concurrent.db"));
    QVERIFY(db.initialize());

    const int numThreads = 6;
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&db, t]() {
            for (int i = 0; i < 15; ++i) {
                LibraryEntity e;
                e.provider = QStringLiteral("source_%1").arg(t);
                e.mangaId = QStringLiteral("manga_%1").arg(i);
                e.title = QStringLiteral("Title %1_%2").arg(t).arg(i);
                e.totalChapters = 10;
                db.addToLibrary(e);

                db.saveProgress(e.provider, e.mangaId, QStringLiteral("ch1"), 1.0, 5, i % 2 == 0);
                auto fetched = db.getLibraryItem(e.provider, e.mangaId);
                Q_UNUSED(fetched);
            }
        });
    }

    for (auto &th : threads) {
        th.join();
    }

    auto lib = db.getLibrary();
    QCOMPARE(lib.size(), numThreads * 15);
}

QTEST_MAIN(tst_database)
#include "tst_database.moc"
