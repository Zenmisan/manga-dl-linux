#pragma once

#include "GamificationModels.h"
#include <QDate>
#include <optional>

namespace mangadl {

class MangaDatabase;

class GamificationService {
public:
    static const QList<HunterRank> &ranks();
    static const QList<MilestoneBadge> &milestones();

    static int calculateScore(int chaptersRead, int streakDays, int libraryCount);
    static HunterRank getHunterRank(int score);
    static std::optional<HunterRank> getNextRank(const HunterRank &currentRank);
    static double calculateProgressToNext(int score, const HunterRank &currentRank, const std::optional<HunterRank> &nextRank);
    static QList<MilestoneStatus> evaluateMilestones(int chaptersRead, int streakDays, int libraryCount);
    static GamificationProfile getProfile(int chaptersRead, int streakDays, int libraryCount);

    static int calculateStreak(const QList<qint64> &historyTimestamps, const QDate &today = QDate::currentDate());
    static QList<int> calculateActivityHeatmap(const QList<qint64> &historyTimestamps, int weeks = 52, const QDate &today = QDate::currentDate());

    static QList<CategoryBreakdown> calculateCategoryBreakdown(MangaDatabase *db);
    static QList<SourceBreakdown> calculateSourceBreakdown(MangaDatabase *db);
};

} // namespace mangadl
