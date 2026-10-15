#pragma once

#include <QString>
#include <QList>

namespace mangadl {

struct HunterRank {
    QString code;
    QString name;
    QString tag;
    QString tier;
    QString color;
    QString bgColor;
    int minScore = 0;
    int nextScore = 0;

    bool operator==(const HunterRank &other) const {
        return code == other.code;
    }
};

struct MilestoneBadge {
    QString id;
    QString title;
    QString category; // "chapters", "library", "streak"
    int threshold = 0;
    QString description;
    QString tier;
    QString color;

    bool operator==(const MilestoneBadge &other) const {
        return id == other.id;
    }
};

struct MilestoneStatus {
    MilestoneBadge badge;
    int currentValue = 0;
    bool unlocked = false;
    double progressPercentage = 0.0;
};

struct GamificationProfile {
    int score = 0;
    HunterRank rank;
    HunterRank nextRank;
    bool hasNextRank = false;
    double progressToNextRank = 0.0;
    int chaptersRead = 0;
    int streakDays = 0;
    int libraryCount = 0;
    QList<MilestoneStatus> milestones;
};

struct ActivityDay {
    qint64 date = 0; // epoch ms or day index
    int count = 0;
    int level = 0;   // 0..3
};

struct CategoryBreakdown {
    QString name;
    int count = 0;
    double fraction = 0.0;
};

struct SourceBreakdown {
    QString source;
    int count = 0;
    double fraction = 0.0;
};

} // namespace mangadl
