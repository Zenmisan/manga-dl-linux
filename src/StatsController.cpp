#include "StatsController.h"
#include "core/db/MangaDatabase.h"
#include "core/gamification/GamificationService.h"
#include "core/util/AppLog.h"

#include <QDateTime>
#include <QVariantMap>
#include <algorithm>

namespace mangadl {

StatsController &StatsController::instance()
{
    static StatsController s_instance;
    return s_instance;
}

StatsController *StatsController::create(QQmlEngine *, QJSEngine *)
{
    return &instance();
}

StatsController::StatsController(MangaDatabase *db, QObject *parent)
    : QObject(parent)
    , m_db(db ? db : MangaDatabase::instance())
{
    if (m_db) {
        connect(m_db, &MangaDatabase::historyChanged, this, &StatsController::refresh);
        connect(m_db, &MangaDatabase::progressChanged, this, &StatsController::refresh);
        connect(m_db, &MangaDatabase::libraryChanged, this, &StatsController::refresh);
    }
    refresh();
}

void StatsController::refresh()
{
    if (!m_db) {
        return;
    }

    const auto history = m_db->getHistory(1000);
    const auto library = m_db->getLibrary();
    const auto completedProgress = m_db->getAllCompletedProgress();

    QList<qint64> historyTimestamps;
    historyTimestamps.reserve(history.size());

    const QDate today = QDate::currentDate();
    int currentMonthChapters = 0;

    for (const auto &h : history) {
        historyTimestamps.append(h.readAt);
        const QDate d = QDateTime::fromMSecsSinceEpoch(h.readAt).toLocalTime().date();
        if (d.year() == today.year() && d.month() == today.month()) {
            currentMonthChapters++;
        }
    }

    m_chaptersRead = std::max(static_cast<int>(completedProgress.size()), static_cast<int>(history.size()));
    const int hours = m_chaptersRead > 0 ? std::max(1, m_chaptersRead * 7 / 60) : 0;
    m_timeReading = QStringLiteral("%1h").arg(hours);

    m_currentStreak = GamificationService::calculateStreak(historyTimestamps, today);
    const int pace = m_chaptersRead > 0 ? std::max(1, m_chaptersRead / 8) : 0;
    m_readingPace = QStringLiteral("%1/wk").arg(pace);

    // Reading goals
    const int monthlyCount = std::max(currentMonthChapters, std::min(m_chaptersRead, 25));
    m_monthlyGoalProgress = std::clamp(static_cast<double>(monthlyCount) / 25.0, 0.0, 1.0);
    m_monthlyGoalText = QStringLiteral("%1 / 25").arg(monthlyCount);

    const int yearlyManga = static_cast<int>(library.size());
    m_yearlyGoalProgress = std::clamp(static_cast<double>(yearlyManga) / 50.0, 0.0, 1.0);
    m_yearlyGoalText = QStringLiteral("%1 / 50").arg(yearlyManga);

    // Heatmap (52 weeks = 364 days)
    const auto heatmap = GamificationService::calculateActivityHeatmap(historyTimestamps, 52, today);
    m_activityHeatmap.clear();
    m_activityHeatmap.reserve(heatmap.size());
    for (int lvl : heatmap) {
        m_activityHeatmap.append(lvl);
    }

    // Gamification profile & rank
    const auto profile = GamificationService::getProfile(m_chaptersRead, m_currentStreak, static_cast<int>(library.size()));
    m_rankName = profile.rank.name;
    m_rankCode = profile.rank.code;
    m_rankTier = profile.rank.tier;
    m_rankTag = profile.rank.tag;
    m_rankColor = profile.rank.color;
    m_rankBgColor = profile.rank.bgColor;
    m_score = profile.score;
    m_progressToNextRank = profile.progressToNextRank;

    // Breakdowns
    const auto catBreakdown = GamificationService::calculateCategoryBreakdown(m_db);
    m_byCategory.clear();
    m_byCategory.reserve(catBreakdown.size());
    for (const auto &b : catBreakdown) {
        m_byCategory.append(QVariantMap{
            {QStringLiteral("name"), b.name},
            {QStringLiteral("count"), b.count},
            {QStringLiteral("fraction"), b.fraction}
        });
    }

    const auto srcBreakdown = GamificationService::calculateSourceBreakdown(m_db);
    m_bySource.clear();
    m_bySource.reserve(srcBreakdown.size());
    for (const auto &b : srcBreakdown) {
        m_bySource.append(QVariantMap{
            {QStringLiteral("name"), b.source},
            {QStringLiteral("count"), b.count},
            {QStringLiteral("fraction"), b.fraction}
        });
    }

    emit statsChanged();
}

} // namespace mangadl
