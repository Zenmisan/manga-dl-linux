#include "StatsController.h"
#include "core/db/MangaDatabase.h"
#include "core/gamification/GamificationService.h"
#include "core/util/AppLog.h"

#include <QDateTime>
#include <QVariantMap>
#include <algorithm>

namespace mangadl {

static StatsController *s_statsInstance = nullptr;

StatsController &StatsController::instance()
{
    if (s_statsInstance) {
        return *s_statsInstance;
    }
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
    s_statsInstance = this;
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
    int last7DaysCount = 0;
    const qint64 sevenDaysAgo = QDateTime::currentDateTime().addDays(-7).toMSecsSinceEpoch();

    for (const auto &h : history) {
        historyTimestamps.append(h.readAt);
        if (h.readAt >= sevenDaysAgo) {
            last7DaysCount++;
        }
        const QDate d = QDateTime::fromMSecsSinceEpoch(h.readAt).toLocalTime().date();
        if (d.year() == today.year() && d.month() == today.month()) {
            currentMonthChapters++;
        }
    }

    m_chaptersRead = std::max(static_cast<int>(completedProgress.size()), static_cast<int>(history.size()));
    const int totalMinutes = m_chaptersRead * 7;
    if (totalMinutes < 60) {
        m_timeReading = QStringLiteral("%1m").arg(totalMinutes);
    } else {
        m_timeReading = QStringLiteral("%1h").arg(totalMinutes / 60);
    }

    m_currentStreak = GamificationService::calculateStreak(historyTimestamps, today);
    m_readingPace = QStringLiteral("%1/wk").arg(last7DaysCount);

    // Reading goals
    m_monthlyGoalProgress = std::clamp(static_cast<double>(currentMonthChapters) / 25.0, 0.0, 1.0);
    m_monthlyGoalText = QStringLiteral("%1 / 25").arg(currentMonthChapters);

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
