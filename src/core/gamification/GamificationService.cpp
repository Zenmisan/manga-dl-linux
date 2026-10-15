#include "GamificationService.h"
#include "core/db/MangaDatabase.h"

#include <QDateTime>
#include <QSet>
#include <QMap>
#include <algorithm>
#include <limits>
#include <cmath>

namespace mangadl {

const QList<HunterRank> &GamificationService::ranks()
{
    static const QList<HunterRank> s_ranks = {
        {QStringLiteral("E"), QStringLiteral("E-Rank Novice"), QStringLiteral("Awakened Novice"), QStringLiteral("Bronze"), QStringLiteral("#71717A"), QStringLiteral("#27272A"), 0, 500},
        {QStringLiteral("D"), QStringLiteral("D-Rank Hunter"), QStringLiteral("Dungeon Scavenger"), QStringLiteral("Bronze"), QStringLiteral("#CD7F32"), QStringLiteral("#3A2016"), 500, 1500},
        {QStringLiteral("C"), QStringLiteral("C-Rank Hunter"), QStringLiteral("Raid Ready"), QStringLiteral("Silver"), QStringLiteral("#94A3B8"), QStringLiteral("#1E293B"), 1500, 4000},
        {QStringLiteral("B"), QStringLiteral("B-Rank Hunter"), QStringLiteral("Veteran"), QStringLiteral("Gold"), QStringLiteral("#F59E0B"), QStringLiteral("#3B2A10"), 4000, 8000},
        {QStringLiteral("A"), QStringLiteral("A-Rank Hunter"), QStringLiteral("High Guild"), QStringLiteral("Platinum"), QStringLiteral("#38BDF8"), QStringLiteral("#0C2A4A"), 8000, 15000},
        {QStringLiteral("S"), QStringLiteral("S-Rank Hunter"), QStringLiteral("Apex Elite"), QStringLiteral("Diamond"), QStringLiteral("#A855F7"), QStringLiteral("#2E1065"), 15000, 25000},
        {QStringLiteral("MONARCH"), QStringLiteral("Shadow Monarch"), QStringLiteral("#1 Sovereign"), QStringLiteral("Mythic"), QStringLiteral("#C084FC"), QStringLiteral("#3B0764"), 25000, std::numeric_limits<int>::max()}
    };
    return s_ranks;
}

const QList<MilestoneBadge> &GamificationService::milestones()
{
    static const QList<MilestoneBadge> s_milestones = {
        // Chapters Read
        {QStringLiteral("first_page"), QStringLiteral("First Page"), QStringLiteral("chapters"), 1, QStringLiteral("Took the very first step into the panel multiverse."), QStringLiteral("bronze"), QStringLiteral("#CD7F32")},
        {QStringLiteral("page_turner"), QStringLiteral("Page Turner"), QStringLiteral("chapters"), 10, QStringLiteral("Getting hooked on the panel flow and cliffhangers."), QStringLiteral("bronze"), QStringLiteral("#CD7F32")},
        {QStringLiteral("casual_reader"), QStringLiteral("Casual Reader"), QStringLiteral("chapters"), 25, QStringLiteral("Finding curiosity in every story arc."), QStringLiteral("bronze"), QStringLiteral("#CD7F32")},
        {QStringLiteral("manga_enthusiast"), QStringLiteral("Manga Enthusiast"), QStringLiteral("chapters"), 50, QStringLiteral("A dependable appetite for weekly releases."), QStringLiteral("silver"), QStringLiteral("#94A3B8")},
        {QStringLiteral("volume_devourer"), QStringLiteral("Volume Devourer"), QStringLiteral("chapters"), 100, QStringLiteral("Consumed a physical shelf worth of tankōbon."), QStringLiteral("silver"), QStringLiteral("#94A3B8")},
        {QStringLiteral("arc_conqueror"), QStringLiteral("Arc Conqueror"), QStringLiteral("chapters"), 250, QStringLiteral("Cruised through epic sagas without blinking."), QStringLiteral("silver"), QStringLiteral("#94A3B8")},
        {QStringLiteral("binge_specialist"), QStringLiteral("Binge Specialist"), QStringLiteral("chapters"), 500, QStringLiteral("Lost entire weekends to non-stop chapter binges."), QStringLiteral("gold"), QStringLiteral("#F59E0B")},
        {QStringLiteral("panel_virtuoso"), QStringLiteral("Panel Virtuoso"), QStringLiteral("chapters"), 750, QStringLiteral("Appreciating every masterstroke and speed line."), QStringLiteral("gold"), QStringLiteral("#F59E0B")},
        {QStringLiteral("grandmaster"), QStringLiteral("Grandmaster Reader"), QStringLiteral("chapters"), 1000, QStringLiteral("A thousand chapters traversed across countless realms."), QStringLiteral("platinum"), QStringLiteral("#38BDF8")},
        {QStringLiteral("mythic_sage"), QStringLiteral("Mythic Sage"), QStringLiteral("chapters"), 2500, QStringLiteral("Living library of manga and web novels."), QStringLiteral("mythic"), QStringLiteral("#A855F7")},

        // Library Collections
        {QStringLiteral("starter_shelf"), QStringLiteral("Starter Shelf"), QStringLiteral("library"), 1, QStringLiteral("Added your very first series to the collection."), QStringLiteral("bronze"), QStringLiteral("#CD7F32")},
        {QStringLiteral("curator"), QStringLiteral("Curator"), QStringLiteral("library"), 10, QStringLiteral("Curating a respectable personal reading bookshelf."), QStringLiteral("bronze"), QStringLiteral("#CD7F32")},
        {QStringLiteral("collector"), QStringLiteral("Collector"), QStringLiteral("library"), 25, QStringLiteral("An expansive library spanning genres and styles."), QStringLiteral("silver"), QStringLiteral("#94A3B8")},
        {QStringLiteral("archivist"), QStringLiteral("Archivist"), QStringLiteral("library"), 50, QStringLiteral("Archiving complete sagas for future generations."), QStringLiteral("gold"), QStringLiteral("#F59E0B")},
        {QStringLiteral("grand_librarian"), QStringLiteral("Grand Librarian"), QStringLiteral("library"), 100, QStringLiteral("A towering library worthy of an Alexandria."), QStringLiteral("platinum"), QStringLiteral("#38BDF8")},

        // Reading Streaks
        {QStringLiteral("spark"), QStringLiteral("Daily Spark"), QStringLiteral("streak"), 1, QStringLiteral("Read at least one chapter today."), QStringLiteral("bronze"), QStringLiteral("#CD7F32")},
        {QStringLiteral("habituated"), QStringLiteral("Habituated"), QStringLiteral("streak"), 3, QStringLiteral("Formed a consistent 3-day reading habit."), QStringLiteral("bronze"), QStringLiteral("#CD7F32")},
        {QStringLiteral("consistent"), QStringLiteral("Consistent"), QStringLiteral("streak"), 7, QStringLiteral("A full week of dedicated daily reading."), QStringLiteral("silver"), QStringLiteral("#94A3B8")},
        {QStringLiteral("dedicated"), QStringLiteral("Dedicated"), QStringLiteral("streak"), 14, QStringLiteral("Two weeks without missing a single day."), QStringLiteral("gold"), QStringLiteral("#F59E0B")},
        {QStringLiteral("unshakeable"), QStringLiteral("Unshakeable"), QStringLiteral("streak"), 30, QStringLiteral("A full month streak. Nothing can distract you."), QStringLiteral("platinum"), QStringLiteral("#38BDF8")},
        {QStringLiteral("century_reader"), QStringLiteral("Century Reader"), QStringLiteral("streak"), 100, QStringLiteral("100 consecutive days of immersion in stories."), QStringLiteral("mythic"), QStringLiteral("#A855F7")}
    };
    return s_milestones;
}

int GamificationService::calculateScore(int chaptersRead, int streakDays, int libraryCount)
{
    return std::max(0, (chaptersRead * 10) + (streakDays * 50) + (libraryCount * 25));
}

HunterRank GamificationService::getHunterRank(int score)
{
    const auto &rankList = ranks();
    for (int i = rankList.size() - 1; i >= 0; --i) {
        if (score >= rankList[i].minScore) {
            return rankList[i];
        }
    }
    return rankList.first();
}

std::optional<HunterRank> GamificationService::getNextRank(const HunterRank &currentRank)
{
    const auto &rankList = ranks();
    for (int i = 0; i < rankList.size() - 1; ++i) {
        if (rankList[i].code == currentRank.code) {
            return rankList[i + 1];
        }
    }
    return std::nullopt;
}

double GamificationService::calculateProgressToNext(int score, const HunterRank &currentRank, const std::optional<HunterRank> &nextRank)
{
    if (!nextRank.has_value() || currentRank.nextScore == std::numeric_limits<int>::max()) {
        return 1.0;
    }
    const int range = nextRank->minScore - currentRank.minScore;
    if (range <= 0) return 1.0;
    const int progress = score - currentRank.minScore;
    return std::clamp(static_cast<double>(progress) / static_cast<double>(range), 0.0, 1.0);
}

QList<MilestoneStatus> GamificationService::evaluateMilestones(int chaptersRead, int streakDays, int libraryCount)
{
    QList<MilestoneStatus> result;
    const auto &list = milestones();
    result.reserve(list.size());

    for (const auto &badge : list) {
        int currentVal = 0;
        if (badge.category == QStringLiteral("chapters")) {
            currentVal = chaptersRead;
        } else if (badge.category == QStringLiteral("streak")) {
            currentVal = streakDays;
        } else if (badge.category == QStringLiteral("library")) {
            currentVal = libraryCount;
        }

        const bool unlocked = currentVal >= badge.threshold;
        const double pct = badge.threshold > 0
            ? std::clamp(static_cast<double>(currentVal) / static_cast<double>(badge.threshold), 0.0, 1.0)
            : 1.0;

        result.append(MilestoneStatus{badge, currentVal, unlocked, pct});
    }

    return result;
}

GamificationProfile GamificationService::getProfile(int chaptersRead, int streakDays, int libraryCount)
{
    const int score = calculateScore(chaptersRead, streakDays, libraryCount);
    const HunterRank rank = getHunterRank(score);
    const auto next = getNextRank(rank);
    const double progress = calculateProgressToNext(score, rank, next);
    const auto milestoneList = evaluateMilestones(chaptersRead, streakDays, libraryCount);

    GamificationProfile profile;
    profile.score = score;
    profile.rank = rank;
    if (next.has_value()) {
        profile.nextRank = *next;
        profile.hasNextRank = true;
    } else {
        profile.hasNextRank = false;
    }
    profile.progressToNextRank = progress;
    profile.chaptersRead = chaptersRead;
    profile.streakDays = streakDays;
    profile.libraryCount = libraryCount;
    profile.milestones = milestoneList;

    return profile;
}

int GamificationService::calculateStreak(const QList<qint64> &historyTimestamps, const QDate &today)
{
    if (historyTimestamps.isEmpty()) {
        return 0;
    }

    QSet<QDate> readDates;
    for (qint64 ts : historyTimestamps) {
        if (ts > 0) {
            readDates.insert(QDateTime::fromMSecsSinceEpoch(ts).toLocalTime().date());
        }
    }

    QDate testDate = today;
    if (!readDates.contains(testDate)) {
        testDate = today.addDays(-1);
        if (!readDates.contains(testDate)) {
            return 0;
        }
    }

    int streak = 0;
    while (readDates.contains(testDate)) {
        streak++;
        testDate = testDate.addDays(-1);
    }

    return streak;
}

QList<int> GamificationService::calculateActivityHeatmap(const QList<qint64> &historyTimestamps, int weeks, const QDate &today)
{
    const int totalCells = weeks * 7;
    QList<int> result(totalCells, 0);

    // Grid in QML is rows: 7, flow: Grid.TopToBottom.
    // Sunday = row 0, Monday = row 1, ..., Saturday = row 6.
    // In Qt, Monday is 1, ..., Sunday is 7.
    auto dayOfWeekIndex = [](const QDate &d) -> int {
        int dow = d.dayOfWeek(); // 1 (Mon) .. 7 (Sun)
        return dow == 7 ? 0 : dow;
    };

    // The grid ends on Saturday of the week containing 'today'.
    int todayRow = dayOfWeekIndex(today);
    QDate gridEndDate = today.addDays(6 - todayRow);
    QDate gridStartDate = gridEndDate.addDays(-totalCells + 1);

    // Count chapters read per date
    QMap<QDate, int> dateCounts;
    for (qint64 ts : historyTimestamps) {
        if (ts > 0) {
            QDate d = QDateTime::fromMSecsSinceEpoch(ts).toLocalTime().date();
            if (d >= gridStartDate && d <= gridEndDate) {
                dateCounts[d]++;
            }
        }
    }

    // Populate columns
    for (int col = 0; col < weeks; ++col) {
        for (int row = 0; row < 7; ++row) {
            int dayOffset = col * 7 + row;
            QDate cellDate = gridStartDate.addDays(dayOffset);
            int count = dateCounts.value(cellDate, 0);

            int level = 0;
            if (count >= 6) {
                level = 3;
            } else if (count >= 3) {
                level = 2;
            } else if (count >= 1) {
                level = 1;
            }

            int index = col * 7 + row;
            if (index < totalCells) {
                result[index] = level;
            }
        }
    }

    return result;
}

QList<CategoryBreakdown> GamificationService::calculateCategoryBreakdown(MangaDatabase *db)
{
    QList<CategoryBreakdown> result;
    if (!db) return result;

    const auto library = db->getLibrary();
    if (library.isEmpty()) return result;

    const auto categories = db->getCategories();
    QMap<QString, QString> catNameMap;
    for (const auto &cat : categories) {
        catNameMap[cat.id] = cat.name;
    }

    QMap<QString, int> categoryCounts;
    int totalCategorized = 0;

    for (const auto &item : library) {
        const auto catIds = db->getMangaCategories(item.provider, item.mangaId);
        if (catIds.isEmpty()) {
            categoryCounts[QStringLiteral("Default")]++;
            totalCategorized++;
        } else {
            for (const auto &cid : catIds) {
                const QString cName = catNameMap.value(cid, cid);
                categoryCounts[cName]++;
                totalCategorized++;
            }
        }
    }

    if (totalCategorized == 0) totalCategorized = 1;

    for (auto it = categoryCounts.constBegin(); it != categoryCounts.constEnd(); ++it) {
        const double frac = static_cast<double>(it.value()) / static_cast<double>(totalCategorized);
        result.append(CategoryBreakdown{it.key(), it.value(), frac});
    }

    std::sort(result.begin(), result.end(), [](const CategoryBreakdown &a, const CategoryBreakdown &b) {
        return a.count > b.count;
    });

    return result;
}

QList<SourceBreakdown> GamificationService::calculateSourceBreakdown(MangaDatabase *db)
{
    QList<SourceBreakdown> result;
    if (!db) return result;

    const auto library = db->getLibrary();
    if (library.isEmpty()) return result;

    QMap<QString, int> sourceCounts;
    for (const auto &item : library) {
        sourceCounts[item.provider]++;
    }

    const int total = library.size();
    for (auto it = sourceCounts.constBegin(); it != sourceCounts.constEnd(); ++it) {
        const double frac = static_cast<double>(it.value()) / static_cast<double>(total);
        result.append(SourceBreakdown{it.key(), it.value(), frac});
    }

    std::sort(result.begin(), result.end(), [](const SourceBreakdown &a, const SourceBreakdown &b) {
        return a.count > b.count;
    });

    return result;
}

} // namespace mangadl
