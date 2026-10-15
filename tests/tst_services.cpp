#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDateTime>

#include "core/gamification/GamificationService.h"
#include "core/gamification/GamificationModels.h"
#include "core/updates/UpdateCheckerService.h"
#include "core/db/MangaDatabase.h"
#include "core/db/Entities.h"
#include "SystemController.h"

using namespace mangadl;

class tst_services : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Gamification tests
    void testCalculateScore();
    void testGetHunterRank();
    void testProgressToNextRank();
    void testEvaluateMilestones();
    void testGetProfile();
    void testStreakCalculation();
    void testHeatmapBucketing();

    // Update checker tests
    void testParseChapterNumber();
    void testNewChaptersDatabaseOperations();

    // System controller tests
    void testFormatBytes();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<MangaDatabase> m_db;
};

void tst_services::initTestCase()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString dbPath = m_tempDir->path() + QStringLiteral("/test_services.db");
    m_db = std::make_unique<MangaDatabase>(dbPath);
    QVERIFY(m_db->initialize());
    QVERIFY(m_db->isInitialized());
}

void tst_services::cleanupTestCase()
{
    if (m_db) {
        m_db->close();
        m_db.reset();
    }
    m_tempDir.reset();
}

void tst_services::testCalculateScore()
{
    // Exact Windows formula: (chapters * 10) + (streak * 50) + (library * 25)
    QCOMPARE(GamificationService::calculateScore(10, 2, 4), 300);
    QCOMPARE(GamificationService::calculateScore(0, 0, 0), 0);
    QCOMPARE(GamificationService::calculateScore(100, 10, 20), 1000 + 500 + 500); // 2000
}

void tst_services::testGetHunterRank()
{
    QCOMPARE(GamificationService::getHunterRank(0).code, QStringLiteral("E"));
    QCOMPARE(GamificationService::getHunterRank(0).name, QStringLiteral("E-Rank Novice"));

    QCOMPARE(GamificationService::getHunterRank(499).code, QStringLiteral("E"));
    QCOMPARE(GamificationService::getHunterRank(500).code, QStringLiteral("D"));
    QCOMPARE(GamificationService::getHunterRank(500).name, QStringLiteral("D-Rank Hunter"));

    QCOMPARE(GamificationService::getHunterRank(1499).code, QStringLiteral("D"));
    QCOMPARE(GamificationService::getHunterRank(1500).code, QStringLiteral("C"));
    QCOMPARE(GamificationService::getHunterRank(1500).name, QStringLiteral("C-Rank Hunter"));

    QCOMPARE(GamificationService::getHunterRank(3999).code, QStringLiteral("C"));
    QCOMPARE(GamificationService::getHunterRank(4000).code, QStringLiteral("B"));
    QCOMPARE(GamificationService::getHunterRank(4000).name, QStringLiteral("B-Rank Hunter"));

    QCOMPARE(GamificationService::getHunterRank(7999).code, QStringLiteral("B"));
    QCOMPARE(GamificationService::getHunterRank(8000).code, QStringLiteral("A"));
    QCOMPARE(GamificationService::getHunterRank(8000).name, QStringLiteral("A-Rank Hunter"));

    QCOMPARE(GamificationService::getHunterRank(14999).code, QStringLiteral("A"));
    QCOMPARE(GamificationService::getHunterRank(15000).code, QStringLiteral("S"));
    QCOMPARE(GamificationService::getHunterRank(15000).name, QStringLiteral("S-Rank Hunter"));
    QCOMPARE(GamificationService::getHunterRank(15000).tag, QStringLiteral("Apex Elite"));

    QCOMPARE(GamificationService::getHunterRank(25000).code, QStringLiteral("MONARCH"));
    QCOMPARE(GamificationService::getHunterRank(25000).name, QStringLiteral("Shadow Monarch"));
    QCOMPARE(GamificationService::getHunterRank(50000).code, QStringLiteral("MONARCH"));
}

void tst_services::testProgressToNextRank()
{
    // D-Rank: 500 to 1500. Score 1000 is (1000 - 500) / (1500 - 500) = 500 / 1000 = 0.50
    const auto rank = GamificationService::getHunterRank(1000);
    const auto next = GamificationService::getNextRank(rank);
    QVERIFY(next.has_value());
    QCOMPARE(next->code, QStringLiteral("C"));

    const double progress = GamificationService::calculateProgressToNext(1000, rank, next);
    QVERIFY(qAbs(progress - 0.50) < 0.001);

    // Monarch has no next rank
    const auto monarch = GamificationService::getHunterRank(30000);
    const auto monarchNext = GamificationService::getNextRank(monarch);
    QVERIFY(!monarchNext.has_value());
    const double monarchProgress = GamificationService::calculateProgressToNext(30000, monarch, monarchNext);
    QCOMPARE(monarchProgress, 1.0);
}

void tst_services::testEvaluateMilestones()
{
    // 50 chapters read, 7 streak, 10 library
    const auto milestones = GamificationService::evaluateMilestones(50, 7, 10);
    QCOMPARE(milestones.size(), 21);

    auto findMilestone = [&](const QString &id) -> std::optional<MilestoneStatus> {
        for (const auto &m : milestones) {
            if (m.badge.id == id) return m;
        }
        return std::nullopt;
    };

    auto firstPage = findMilestone(QStringLiteral("first_page"));
    QVERIFY(firstPage.has_value());
    QVERIFY(firstPage->unlocked);
    QCOMPARE(firstPage->progressPercentage, 1.0);

    auto enthusiast = findMilestone(QStringLiteral("manga_enthusiast"));
    QVERIFY(enthusiast.has_value());
    QVERIFY(enthusiast->unlocked);
    QCOMPARE(enthusiast->progressPercentage, 1.0);

    auto volumeDevourer = findMilestone(QStringLiteral("volume_devourer"));
    QVERIFY(volumeDevourer.has_value());
    QVERIFY(!volumeDevourer->unlocked);
    QVERIFY(qAbs(volumeDevourer->progressPercentage - 0.50) < 0.001);

    auto consistent = findMilestone(QStringLiteral("consistent"));
    QVERIFY(consistent.has_value());
    QVERIFY(consistent->unlocked);
    QCOMPARE(consistent->progressPercentage, 1.0);

    auto curator = findMilestone(QStringLiteral("curator"));
    QVERIFY(curator.has_value());
    QVERIFY(curator->unlocked);
    QCOMPARE(curator->progressPercentage, 1.0);
}

void tst_services::testGetProfile()
{
    const auto profile = GamificationService::getProfile(150, 5, 20);
    QVERIFY(profile.score > 0);
    QCOMPARE(profile.chaptersRead, 150);
    QCOMPARE(profile.streakDays, 5);
    QCOMPARE(profile.libraryCount, 20);
    QVERIFY(!profile.rank.code.isEmpty());
    QCOMPARE(profile.milestones.size(), 21);
}

void tst_services::testStreakCalculation()
{
    const QDate today = QDate(2026, 10, 15);

    // Case 1: Today, Yesterday, 2 days ago -> streak 3
    {
        QList<qint64> timestamps = {
            QDateTime(today, QTime(12, 0)).toMSecsSinceEpoch(),
            QDateTime(today.addDays(-1), QTime(14, 0)).toMSecsSinceEpoch(),
            QDateTime(today.addDays(-2), QTime(9, 30)).toMSecsSinceEpoch(),
        };
        QCOMPARE(GamificationService::calculateStreak(timestamps, today), 3);
    }

    // Case 2: Yesterday, 2 days ago (not today yet) -> streak 2
    {
        QList<qint64> timestamps = {
            QDateTime(today.addDays(-1), QTime(20, 0)).toMSecsSinceEpoch(),
            QDateTime(today.addDays(-2), QTime(21, 0)).toMSecsSinceEpoch(),
        };
        QCOMPARE(GamificationService::calculateStreak(timestamps, today), 2);
    }

    // Case 3: Gap! 2 days ago, 3 days ago (nothing today or yesterday) -> streak 0
    {
        QList<qint64> timestamps = {
            QDateTime(today.addDays(-2), QTime(12, 0)).toMSecsSinceEpoch(),
            QDateTime(today.addDays(-3), QTime(12, 0)).toMSecsSinceEpoch(),
        };
        QCOMPARE(GamificationService::calculateStreak(timestamps, today), 0);
    }

    // Case 4: Multiple reads on same day -> counts as 1 day
    {
        QList<qint64> timestamps = {
            QDateTime(today, QTime(8, 0)).toMSecsSinceEpoch(),
            QDateTime(today, QTime(12, 0)).toMSecsSinceEpoch(),
            QDateTime(today, QTime(18, 0)).toMSecsSinceEpoch(),
            QDateTime(today.addDays(-1), QTime(10, 0)).toMSecsSinceEpoch(),
        };
        QCOMPARE(GamificationService::calculateStreak(timestamps, today), 2);
    }

    // Case 5: Empty timestamps -> streak 0
    {
        QCOMPARE(GamificationService::calculateStreak({}, today), 0);
    }
}

void tst_services::testHeatmapBucketing()
{
    const QDate today = QDate(2026, 10, 15);
    QList<qint64> timestamps;

    // Day 1: 1 read -> level 1
    timestamps.append(QDateTime(today, QTime(10, 0)).toMSecsSinceEpoch());

    // Day 2 (yesterday): 4 reads -> level 2
    for (int i = 0; i < 4; ++i) {
        timestamps.append(QDateTime(today.addDays(-1), QTime(10 + i, 0)).toMSecsSinceEpoch());
    }

    // Day 3 (2 days ago): 8 reads -> level 3
    for (int i = 0; i < 8; ++i) {
        timestamps.append(QDateTime(today.addDays(-2), QTime(8 + i, 0)).toMSecsSinceEpoch());
    }

    const auto heatmap = GamificationService::calculateActivityHeatmap(timestamps, 52, today);
    QCOMPARE(heatmap.size(), 52 * 7);

    // Verify all values are within [0, 3]
    for (int lvl : heatmap) {
        QVERIFY(lvl >= 0 && lvl <= 3);
    }

    // Heatmap contains level 1, 2, and 3
    QVERIFY(heatmap.contains(1));
    QVERIFY(heatmap.contains(2));
    QVERIFY(heatmap.contains(3));
}

void tst_services::testParseChapterNumber()
{
    QCOMPARE(UpdateCheckerService::parseChapterNumber(QStringLiteral("1")), 1.0);
    QCOMPARE(UpdateCheckerService::parseChapterNumber(QStringLiteral("12.5")), 12.5);
    QCOMPARE(UpdateCheckerService::parseChapterNumber(QStringLiteral("Chapter 42")), 42.0);
    QCOMPARE(UpdateCheckerService::parseChapterNumber(QStringLiteral("Ch. 104.5 - Climax")), 104.5);
    QCOMPARE(UpdateCheckerService::parseChapterNumber(QStringLiteral("")), 0.0);
    QCOMPARE(UpdateCheckerService::parseChapterNumber(QStringLiteral("   ")), 0.0);
}

void tst_services::testNewChaptersDatabaseOperations()
{
    // Clean initial state
    m_db->clearNewChapters();
    QCOMPARE(m_db->getNewChapters().size(), 0);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    NewChapterEntity c1;
    c1.id = QStringLiteral("mangadex/m1/c10");
    c1.mangaId = QStringLiteral("m1");
    c1.mangaTitle = QStringLiteral("Solo Leveling");
    c1.coverUrl = QStringLiteral("https://example.com/cover.jpg");
    c1.provider = QStringLiteral("mangadex");
    c1.chapterId = QStringLiteral("c10");
    c1.chapterTitle = QStringLiteral("Chapter 10");
    c1.chapterNumber = 10.0;
    c1.detectedAt = now;

    NewChapterEntity c2;
    c2.id = QStringLiteral("mangadex/m1/c11");
    c2.mangaId = QStringLiteral("m1");
    c2.mangaTitle = QStringLiteral("Solo Leveling");
    c2.coverUrl = QStringLiteral("https://example.com/cover.jpg");
    c2.provider = QStringLiteral("mangadex");
    c2.chapterId = QStringLiteral("c11");
    c2.chapterTitle = QStringLiteral("Chapter 11");
    c2.chapterNumber = 11.0;
    c2.detectedAt = now + 1000;

    QVERIFY(m_db->saveNewChapters({c1, c2}));

    auto list = m_db->getNewChapters();
    QCOMPARE(list.size(), 2);
    // Ordered by detected_at DESC
    QCOMPARE(list[0].id, QStringLiteral("mangadex/m1/c11"));
    QCOMPARE(list[1].id, QStringLiteral("mangadex/m1/c10"));

    // Remove single chapter
    QVERIFY(m_db->removeNewChapter(QStringLiteral("mangadex/m1/c10")));
    list = m_db->getNewChapters();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list[0].id, QStringLiteral("mangadex/m1/c11"));

    // Clear all
    QVERIFY(m_db->clearNewChapters());
    QCOMPARE(m_db->getNewChapters().size(), 0);
}

void tst_services::testFormatBytes()
{
    QCOMPARE(SystemController::formatBytes(500), QStringLiteral("500 B"));
    QCOMPARE(SystemController::formatBytes(2048), QStringLiteral("2.0 KB"));
    QCOMPARE(SystemController::formatBytes(1048576 * 5), QStringLiteral("5.0 MB"));
    QCOMPARE(SystemController::formatBytes(1073741824LL * 2), QStringLiteral("2.0 GB"));
}

QTEST_MAIN(tst_services)
#include "tst_services.moc"
