#include "ProfileController.h"
#include "core/db/MangaDatabase.h"
#include "core/gamification/GamificationService.h"
#include "core/settings/AppSettings.h"
#include "core/util/AppLog.h"

#include <QDateTime>
#include <QVariantMap>

namespace mangadl {

static ProfileController *s_profileInstance = nullptr;

ProfileController &ProfileController::instance()
{
    if (s_profileInstance) {
        return *s_profileInstance;
    }
    static ProfileController s_instance;
    return s_instance;
}

ProfileController *ProfileController::create(QQmlEngine *, QJSEngine *)
{
    return &instance();
}

ProfileController::ProfileController(MangaDatabase *db, QObject *parent)
    : QObject(parent)
    , m_db(db ? db : MangaDatabase::instance())
{
    s_profileInstance = this;
    if (m_db) {
        connect(m_db, &MangaDatabase::historyChanged, this, &ProfileController::refresh);
        connect(m_db, &MangaDatabase::progressChanged, this, &ProfileController::refresh);
        connect(m_db, &MangaDatabase::libraryChanged, this, &ProfileController::refresh);
    }
    auto *settings = AppSettings::instance();
    if (settings) {
        connect(settings, &AppSettings::displayNameChanged, this, &ProfileController::refresh);
        connect(settings, &AppSettings::bioChanged, this, &ProfileController::refresh);
        connect(settings, &AppSettings::userEmailChanged, this, &ProfileController::refresh);
    }
    refresh();
}

void ProfileController::refresh()
{
    auto *settings = AppSettings::instance();
    const QString email = settings ? settings->userEmail() : QString();
    const QString name = settings ? settings->displayName() : QString();
    const QString bioText = settings ? settings->bio() : QString();

    if (!name.trimmed().isEmpty()) {
        m_displayName = name.trimmed();
        m_username = QStringLiteral("@%1 · Active Hunter").arg(m_displayName.toLower().replace(QLatin1Char(' '), QLatin1Char('_')));
    } else if (!email.trimmed().isEmpty()) {
        const QString part = email.split(QLatin1Char('@')).first();
        m_displayName = part.isEmpty() ? QStringLiteral("Reader") : part.left(1).toUpper() + part.mid(1);
        m_username = QStringLiteral("@%1 · Active Hunter").arg(part);
    } else {
        m_displayName = QStringLiteral("Reader");
        m_username = QStringLiteral("@reader · Solo Hunter");
    }

    m_bio = bioText.trimmed().isEmpty() ? QStringLiteral("Passionate manga and web novel reader.") : bioText.trimmed();

    if (!m_db) {
        emit profileChanged();
        return;
    }

    const auto lib = m_db->getLibrary();
    const auto hist = m_db->getHistory(100);
    const auto comp = m_db->getAllCompletedProgress();

    m_libraryCount = static_cast<int>(lib.size());
    m_chaptersRead = std::max(static_cast<int>(comp.size()), static_cast<int>(hist.size()));

    QList<qint64> historyTimestamps;
    historyTimestamps.reserve(hist.size());
    qint64 earliestTime = 0;

    for (const auto &h : hist) {
        historyTimestamps.append(h.readAt);
        if (earliestTime == 0 || (h.readAt > 0 && h.readAt < earliestTime)) {
            earliestTime = h.readAt;
        }
    }

    if (earliestTime > 0) {
        m_readingSince = QDateTime::fromMSecsSinceEpoch(earliestTime).toLocalTime().toString(QStringLiteral("MMMM yyyy"));
    } else {
        m_readingSince = QStringLiteral("October 2026");
    }

    m_currentStreak = GamificationService::calculateStreak(historyTimestamps);

    const auto profile = GamificationService::getProfile(m_chaptersRead, m_currentStreak, m_libraryCount);
    m_rankName = profile.rank.name;
    m_rankTag = profile.rank.tag;
    m_rankTier = profile.rank.tier;
    m_rankCode = profile.rank.code;
    m_rankColor = profile.rank.color;
    m_rankBgColor = profile.rank.bgColor;
    m_score = profile.score;
    m_progressToNextRank = profile.progressToNextRank;

    if (profile.hasNextRank) {
        m_nextRankInfo = QStringLiteral("Next: %1 at %2 EXP").arg(profile.nextRank.name).arg(profile.nextRank.minScore);
    } else {
        m_nextRankInfo = QStringLiteral("Maximum rank reached · Shadow Monarch");
    }

    m_unlockedBadges.clear();
    m_lockedBadges.clear();
    m_unlockedMilestonesCount = 0;
    m_totalMilestonesCount = profile.milestones.size();

    for (const auto &m : profile.milestones) {
        QVariantMap badgeMap{
            {QStringLiteral("id"), m.badge.id},
            {QStringLiteral("title"), m.badge.title},
            {QStringLiteral("category"), m.badge.category},
            {QStringLiteral("threshold"), m.badge.threshold},
            {QStringLiteral("description"), m.badge.description},
            {QStringLiteral("tier"), m.badge.tier},
            {QStringLiteral("color"), m.badge.color},
            {QStringLiteral("currentValue"), m.currentValue},
            {QStringLiteral("progressPercentage"), m.progressPercentage},
            {QStringLiteral("unlocked"), m.unlocked}
        };

        if (m.unlocked) {
            m_unlockedMilestonesCount++;
            m_unlockedBadges.append(badgeMap);
        } else {
            m_lockedBadges.append(badgeMap);
        }
    }

    // Recent activity (up to 5 items)
    m_recentActivity.clear();
    for (int i = 0; i < std::min(5, static_cast<int>(hist.size())); ++i) {
        const auto &h = hist[i];
        const QString chTitle = h.chapterTitle.trimmed().isEmpty()
            ? QStringLiteral("Ch. %1").arg(h.chapterId)
            : h.chapterTitle.trimmed();
        m_recentActivity.append(QStringLiteral("Read %1 · %2").arg(h.mangaTitle, chTitle));
    }

    if (m_recentActivity.isEmpty()) {
        m_recentActivity.append(QStringLiteral("No reading history yet."));
    }

    // Pinned covers (up to 4 from library)
    m_pinnedCovers.clear();
    for (int i = 0; i < std::min(4, static_cast<int>(lib.size())); ++i) {
        m_pinnedCovers.append(lib[i].coverUrl.isEmpty() ? QStringLiteral("#1A2433") : lib[i].coverUrl);
    }
    // Pad with fallback colors if fewer than 4
    static const QStringList fallbackColors = {
        QStringLiteral("#3A1518"), QStringLiteral("#1A2433"),
        QStringLiteral("#2E2412"), QStringLiteral("#13282A")
    };
    while (m_pinnedCovers.size() < 4) {
        m_pinnedCovers.append(fallbackColors[m_pinnedCovers.size() % fallbackColors.size()]);
    }

    emit profileChanged();
}

} // namespace mangadl
