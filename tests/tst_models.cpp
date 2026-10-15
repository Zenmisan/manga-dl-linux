#include <QtTest>
#include <QTemporaryDir>
#include "db/MangaDatabase.h"
#include "db/LibraryModel.h"
#include "db/CategoryModel.h"
#include "db/HistoryModel.h"
#include "db/UpdatesModel.h"
#include "db/DownloadsModel.h"

using namespace mangadl;

class tst_models : public QObject {
    Q_OBJECT

private slots:
    void libraryModelTest();
    void categoryModelTest();
    void historyModelTest();
    void updatesModelTest();
    void downloadsModelTest();
};

void tst_models::libraryModelTest()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    LibraryModel model(&db);
    QCOMPARE(model.rowCount(), 0);

    // Add 3 manga
    model.add(QStringLiteral("mangadex"), QStringLiteral("op"), QStringLiteral("One Piece"), QStringLiteral("op.jpg"));
    model.add(QStringLiteral("mangadex"), QStringLiteral("bleach"), QStringLiteral("Bleach"), QStringLiteral("bleach.jpg"));
    model.add(QStringLiteral("mangadex"), QStringLiteral("naruto"), QStringLiteral("Naruto"), QStringLiteral("naruto.jpg"));

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.count(), 3);
    QVERIFY(model.contains(QStringLiteral("mangadex"), QStringLiteral("op")));

    // Role access
    QModelIndex idx0 = model.index(0);
    QVariant titleVal = model.data(idx0, LibraryModel::TitleRole);
    QVERIFY(!titleVal.toString().isEmpty());

    QVariant modelDataVal = model.data(idx0, LibraryModel::ModelDataRole);
    QVERIFY(modelDataVal.canConvert<QVariantMap>());
    QVariantMap map = modelDataVal.toMap();
    QVERIFY(map.contains(QStringLiteral("mangaId")));
    QVERIFY(map.contains(QStringLiteral("title")));
    QVERIFY(map.contains(QStringLiteral("inLibrary")));

    // Search filter
    model.setSearchFilter(QStringLiteral("Piece"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), LibraryModel::TitleRole).toString(), QStringLiteral("One Piece"));
    model.setSearchFilter(QString());
    QCOMPARE(model.rowCount(), 3);

    // Category filter
    db.setMangaCategories(QStringLiteral("mangadex"), QStringLiteral("op"), {QStringLiteral("reading")});
    model.setCategoryFilter(QStringLiteral("reading"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), LibraryModel::TitleRole).toString(), QStringLiteral("One Piece"));

    model.setCategoryFilter(QStringLiteral("All"));
    QCOMPARE(model.rowCount(), 3);

    // Sort by Title A-Z (Bleach, Naruto, One Piece)
    model.setSortMode(1);
    QCOMPARE(model.data(model.index(0), LibraryModel::TitleRole).toString(), QStringLiteral("Bleach"));
    QCOMPARE(model.data(model.index(1), LibraryModel::TitleRole).toString(), QStringLiteral("Naruto"));
    QCOMPARE(model.data(model.index(2), LibraryModel::TitleRole).toString(), QStringLiteral("One Piece"));

    // Continue reading
    db.recordHistory(QStringLiteral("mangadex"), QStringLiteral("op"), QStringLiteral("One Piece"),
                     QStringLiteral("ch-1"), QStringLiteral("Romance Dawn"), QStringLiteral("op.jpg"));
    auto continueList = model.continueReading();
    QCOMPARE(continueList.size(), 1);
    QVariantMap contItem = continueList[0].toMap();
    QVERIFY(contItem.contains(QStringLiteral("manga")));
    QVERIFY(contItem.contains(QStringLiteral("where")));
    QVERIFY(contItem.contains(QStringLiteral("progress")));

    // Remove
    model.remove(QStringLiteral("mangadex"), QStringLiteral("bleach"));
    QCOMPARE(model.rowCount(), 2);
}

void tst_models::categoryModelTest()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    CategoryModel model(&db);
    // 4 default categories
    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.count(), 4);

    QStringList chips = model.categoryChips();
    QCOMPARE(chips.size(), 5); // "All · 0", "Reading", "Plan to read", "Completed", "Local files"
    QVERIFY(chips[0].startsWith(QStringLiteral("All ·")));

    // Add category
    model.addCategory(QStringLiteral("Favorites"));
    QCOMPARE(model.rowCount(), 5);
    QVERIFY(model.names().contains(QStringLiteral("Favorites")));

    // Rename category
    model.renameCategory(QStringLiteral("favorites"), QStringLiteral("Super Favorites"));
    QVERIFY(model.names().contains(QStringLiteral("Super Favorites")));

    // Manga category assignment
    model.setCategoriesForManga(QStringLiteral("mangadex"), QStringLiteral("op"), {QStringLiteral("favorites")});
    auto assigned = model.getCategoriesForManga(QStringLiteral("mangadex"), QStringLiteral("op"));
    QCOMPARE(assigned.size(), 1);
    QCOMPARE(assigned[0], QStringLiteral("favorites"));

    // Delete category
    model.deleteCategory(QStringLiteral("favorites"));
    QCOMPARE(model.rowCount(), 4);
}

void tst_models::historyModelTest()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    HistoryModel model(&db);
    QCOMPARE(model.rowCount(), 0);

    model.record(QStringLiteral("mangadex"), QStringLiteral("m1"), QStringLiteral("Title 1"),
                 QStringLiteral("ch-1"), QStringLiteral("Pilot"), QStringLiteral("cover.jpg"));
    QThread::msleep(5);
    model.record(QStringLiteral("mangadex"), QStringLiteral("m2"), QStringLiteral("Title 2"),
                 QStringLiteral("ch-2"), QStringLiteral("Episode 2"), QStringLiteral("cover.jpg"));

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.count(), 2);

    QModelIndex idx0 = model.index(0);
    QVariant mapVal = model.data(idx0, HistoryModel::ModelDataRole);
    QVERIFY(mapVal.canConvert<QVariantMap>());
    QVariantMap map = mapVal.toMap();
    QCOMPARE(map[QStringLiteral("mangaId")].toString(), QStringLiteral("m2"));
    QVERIFY(map.contains(QStringLiteral("where")));
    QVERIFY(map.contains(QStringLiteral("when")));

    // Search
    model.setSearchFilter(QStringLiteral("Title 1"));
    QCOMPARE(model.rowCount(), 1);
    model.setSearchFilter(QString());
    QCOMPARE(model.rowCount(), 2);

    // Remove single
    model.removeItem(0);
    QCOMPARE(model.rowCount(), 1);

    // Clear
    model.clearHistory();
    QCOMPARE(model.rowCount(), 0);
}

void tst_models::updatesModelTest()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    UpdatesModel model(&db);
    QCOMPARE(model.rowCount(), 0);

    model.addUpdate(QStringLiteral("m1"), QStringLiteral("Solo Leveling"), QStringLiteral("sl.jpg"),
                    QStringLiteral("asura"), QStringLiteral("ch-100"), QStringLiteral("Chapter 100"), 100.0);
    model.addUpdate(QStringLiteral("m2"), QStringLiteral("Tower of God"), QStringLiteral("tog.jpg"),
                    QStringLiteral("webtoons"), QStringLiteral("ch-500"), QStringLiteral("Chapter 500"), 500.0);

    QCOMPARE(model.rowCount(), 1); // Grouped into "Today"
    QCOMPARE(model.totalNewChapters(), 2);

    QModelIndex groupIdx = model.index(0);
    QCOMPARE(model.data(groupIdx, UpdatesModel::TitleRole).toString(), QStringLiteral("Today"));

    QVariant itemsVal = model.data(groupIdx, UpdatesModel::ItemsRole);
    QVERIFY(itemsVal.canConvert<QVariantList>());
    QVariantList items = itemsVal.toList();
    QCOMPARE(items.size(), 2);

    QVariantMap item0 = items[0].toMap();
    QVERIFY(item0.contains(QStringLiteral("manga")));
    QVERIFY(item0.contains(QStringLiteral("chapter")));
    QVERIFY(item0.contains(QStringLiteral("source")));

    // Clear
    model.clearAll();
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.totalNewChapters(), 0);
}

void tst_models::downloadsModelTest()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    MangaDatabase db(tempDir.path() + QStringLiteral("/manga.db"));
    QVERIFY(db.initialize());

    DownloadsModel model(&db);
    QCOMPARE(model.rowCount(), 0);

    model.enqueue(QStringLiteral("mangadex"), QStringLiteral("op"), QStringLiteral("One Piece"),
                  QStringLiteral("ch-1"), QStringLiteral("Romance Dawn"), 1.0);
    model.enqueue(QStringLiteral("mangadex"), QStringLiteral("op"), QStringLiteral("One Piece"),
                  QStringLiteral("ch-2"), QStringLiteral("Luffy"), 2.0);

    QCOMPARE(model.queueCount(), 2);
    QCOMPARE(model.finishedCount(), 0);
    QCOMPARE(model.rowCount(), 2); // Default tab is queue

    // Update one to completed
    db.updateDownloadProgress(QStringLiteral("mangadex/op/ch-1"), 20, 20, QStringLiteral("completed"));

    QCOMPARE(model.queueCount(), 1);
    QCOMPARE(model.finishedCount(), 1);
    QCOMPARE(model.rowCount(), 1); // Only queued in queue tab

    // Switch to finished tab
    model.setFilterTab(QStringLiteral("finished"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), DownloadsModel::StateRole).toString(), QStringLiteral("done"));

    // Switch to all tab
    model.setFilterTab(QStringLiteral("all"));
    QCOMPARE(model.rowCount(), 2);

    // Pause all toggle
    model.pauseAll();
    QVERIFY(model.isPaused());

    model.resumeAll();
    QVERIFY(!model.isPaused());

    // Clear finished
    model.clearFinished();
    QCOMPARE(model.finishedCount(), 0);
}

QTEST_MAIN(tst_models)
#include "tst_models.moc"
