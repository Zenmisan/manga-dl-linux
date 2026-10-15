#include <QtTest>
#include <QTemporaryDir>

#include "SearchController.h"
#include "BrowseSourceController.h"
#include "ExtensionsController.h"
#include "db/MangaDatabase.h"
#include "extensions/ExtensionManager.h"
#include "util/AppPaths.h"

using namespace mangadl;

class tst_discovery : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void databaseMigrationTest();
    void extensionsControllerTest();
    void searchControllerTest();
    void browseSourceControllerTest();

private:
    std::unique_ptr<QTemporaryDir> m_tmpDir;
};

void tst_discovery::initTestCase()
{
    m_tmpDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tmpDir->isValid());
    AppPaths::setRootOverride(m_tmpDir->path());
    QVERIFY(AppPaths::ensureDirs());
    QVERIFY(MangaDatabase::instance()->initialize());
    ExtensionManager::instance().init();
#ifdef MANGADL_ASSETS_DIR
    ExtensionManager::instance().addScriptsFrom(QStringLiteral(MANGADL_ASSETS_DIR "/extensions"), true);
#endif
}

void tst_discovery::cleanupTestCase()
{
    QTest::qWait(300);
    AppPaths::setRootOverride(QString());
}

void tst_discovery::databaseMigrationTest()
{
    const QString oldProv = QStringLiteral("mangadex");
    const QString oldId = QStringLiteral("one-piece-old");
    const QString newProv = QStringLiteral("mangakatana");
    const QString newId = QStringLiteral("one-piece-new");
    const QString newTitle = QStringLiteral("One Piece (Katana)");
    const QString newCover = QStringLiteral("https://example.com/op-katana.jpg");

    // 1. Setup old manga in library
    LibraryEntity oldItem;
    oldItem.provider = oldProv;
    oldItem.mangaId = oldId;
    oldItem.title = QStringLiteral("One Piece (Dex)");
    oldItem.coverUrl = QStringLiteral("https://example.com/op-dex.jpg");
    oldItem.type = QStringLiteral("manga");
    oldItem.totalChapters = 1100;
    QVERIFY(MangaDatabase::instance()->addToLibrary(oldItem));
    QVERIFY(MangaDatabase::instance()->isInLibrary(oldProv, oldId));

    // 2. Attach category
    QVERIFY(MangaDatabase::instance()->setMangaCategories(oldProv, oldId, {QStringLiteral("reading"), QStringLiteral("completed")}));

    // 3. Attach progress
    QVERIFY(MangaDatabase::instance()->saveProgress(oldProv, oldId, QStringLiteral("ch-1"), 1.0, 15, true));
    QVERIFY(MangaDatabase::instance()->saveProgress(oldProv, oldId, QStringLiteral("ch-2"), 2.0, 5, false));

    // 4. Attach tracker bind
    TrackerBindEntity bind;
    bind.provider = oldProv;
    bind.mangaId = oldId;
    bind.tracker = QStringLiteral("anilist");
    bind.remoteId = 12345;
    QVERIFY(MangaDatabase::instance()->saveTrackerBind(bind));

    // 5. Attach history
    QVERIFY(MangaDatabase::instance()->recordHistory(oldProv, oldId, oldItem.title, QStringLiteral("ch-2"), QStringLiteral("Chapter 2"), oldItem.coverUrl));

    // 6. Execute migration
    const bool migrated = MangaDatabase::instance()->migrateManga(oldProv, oldId, newProv, newId, newTitle, newCover);
    QVERIFY(migrated);

    // 7. Verify old manga removed from library
    QVERIFY(!MangaDatabase::instance()->isInLibrary(oldProv, oldId));

    // 8. Verify new manga in library with updated title and cover
    QVERIFY(MangaDatabase::instance()->isInLibrary(newProv, newId));
    auto newItem = MangaDatabase::instance()->getLibraryItem(newProv, newId);
    QVERIFY(newItem.has_value());
    QCOMPARE(newItem->title, newTitle);
    QCOMPARE(newItem->coverUrl, newCover);
    QCOMPARE(newItem->totalChapters, 1100);

    // 9. Verify categories preserved
    const QStringList newCats = MangaDatabase::instance()->getMangaCategories(newProv, newId);
    QCOMPARE(newCats.size(), 2);
    QVERIFY(newCats.contains(QStringLiteral("reading")));
    QVERIFY(newCats.contains(QStringLiteral("completed")));

    // 10. Verify reading progress preserved
    const auto p1 = MangaDatabase::instance()->getProgress(newProv, newId, QStringLiteral("ch-1"));
    QVERIFY(p1.has_value());
    QCOMPARE(p1->page, 15);
    QCOMPARE(p1->completed, 1);

    const auto p2 = MangaDatabase::instance()->getProgress(newProv, newId, QStringLiteral("ch-2"));
    QVERIFY(p2.has_value());
    QCOMPARE(p2->page, 5);
    QCOMPARE(p2->completed, 0);

    // 11. Verify tracker bind preserved
    const auto newBind = MangaDatabase::instance()->getTrackerBind(newProv, newId, QStringLiteral("anilist"));
    QVERIFY(newBind.has_value());
    QCOMPARE(newBind->remoteId, 12345);

    // 12. Verify history updated
    const auto hist = MangaDatabase::instance()->getHistory(10);
    bool foundInHistory = false;
    for (const auto &h : hist) {
        if (h.provider == newProv && h.mangaId == newId) {
            foundInHistory = true;
            QCOMPARE(h.mangaTitle, newTitle);
            break;
        }
    }
    QVERIFY(foundInHistory);
}

void tst_discovery::extensionsControllerTest()
{
    ExtensionsController ctrl;
    ctrl.refresh();

    // Check sources
    const QVariantList sources = ctrl.sources();
    QVERIFY(!sources.isEmpty());

    // Pinned test
    const QString srcId = sources.first().toMap().value(QStringLiteral("id")).toString();
    QVERIFY(!ctrl.isPinned(srcId));
    ctrl.togglePinSource(srcId);
    QVERIFY(ctrl.isPinned(srcId));
    ctrl.togglePinSource(srcId);
    QVERIFY(!ctrl.isPinned(srcId));

    // Extensions list & language filtering
    QVERIFY(ctrl.installedCount() > 0);
    ctrl.setLanguageFilter(QStringLiteral("English"));
    QCOMPARE(ctrl.languageFilter(), QStringLiteral("English"));
    QVERIFY(ctrl.extensions().size() > 0);

    ctrl.setLanguageFilter(QStringLiteral("All languages"));
    QCOMPARE(ctrl.languageFilter(), QStringLiteral("All languages"));
    QVERIFY(ctrl.extensions().size() >= ctrl.installedCount());

    // Migration helper
    ctrl.refresh();
    const QVariantList libItems = ctrl.libraryItems();
    QVERIFY(!libItems.isEmpty()); // from previous migration test
}

void tst_discovery::searchControllerTest()
{
    SearchController ctrl;
    QVERIFY(!ctrl.availableSources().isEmpty());

    // Tabs
    ctrl.setActiveTab(0); // Manga
    QCOMPARE(ctrl.activeTab(), 0);
    ctrl.setActiveTab(1); // Novels
    QCOMPARE(ctrl.activeTab(), 1);

    // Sources filter
    ctrl.toggleSource(QStringLiteral("mangadex"));
    QVERIFY(ctrl.selectedSources().contains(QStringLiteral("mangadex")));
    ctrl.toggleSource(QStringLiteral("mangadex"));
    QVERIFY(!ctrl.selectedSources().contains(QStringLiteral("mangadex")));

    // Status and Sort
    ctrl.setSelectedStatus(QStringLiteral("Completed"));
    QCOMPARE(ctrl.selectedStatus(), QStringLiteral("Completed"));
    ctrl.setSortBy(QStringLiteral("Title A–Z"));
    QCOMPARE(ctrl.sortBy(), QStringLiteral("Title A–Z"));

    ctrl.resetFilters();
    QCOMPARE(ctrl.selectedStatus(), QStringLiteral("Any"));
    QCOMPARE(ctrl.sortBy(), QStringLiteral("Relevance"));
    QVERIFY(ctrl.selectedSources().isEmpty());

    // Cancellation
    ctrl.search(QStringLiteral("naruto"));
    QVERIFY(ctrl.isSearching());
    ctrl.cancel();
    QVERIFY(!ctrl.isSearching());
}

void tst_discovery::browseSourceControllerTest()
{
    BrowseSourceController ctrl;
    ctrl.load(QStringLiteral("MangaDex"));

    QCOMPARE(ctrl.sourceId(), QStringLiteral("mangadex"));
    QCOMPARE(ctrl.sourceName().toLower(), QStringLiteral("mangadex"));
    QCOMPARE(ctrl.sourceUrl(), QStringLiteral("https://mangadex.org"));
    QCOMPARE(ctrl.mode(), QStringLiteral("popular"));

    ctrl.setBrowseMode(QStringLiteral("latest"));
    QCOMPARE(ctrl.mode(), QStringLiteral("latest"));
}

QTEST_MAIN(tst_discovery)
#include "tst_discovery.moc"
