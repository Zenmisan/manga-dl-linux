#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <cstdint>

namespace mangadl {

struct BackupChapter {
    QString url;
    QString name;
    QString scanlator;
    bool read = false;
    bool bookmark = false;
    int lastPageRead = 0;
    double chapterNumber = 0.0;
    int sourceOrder = 0;
};

struct BackupTracking {
    int syncId = 0;
    qint64 mediaId = 0;
    QString title;
    double lastChapterRead = 0.0;
    double score = 0.0;
    int status = 0;
    QString trackingUrl;
};

struct BackupHistoryEntry {
    QString url;
    qint64 lastRead = 0;
};

struct BackupManga {
    qint64 source = 0;
    QString url;
    QString title;
    QString artist;
    QString author;
    QString description;
    QString thumbnailUrl;
    bool favorite = true;
    QList<BackupChapter> chapters;
    QList<BackupTracking> tracking;
    QList<BackupHistoryEntry> history;
    QStringList categoryNames;
    QString sourceName;
};

struct BackupCategoryEntry {
    QString name;
    int order = 0;
};

struct BackupSourceEntry {
    QString name;
    qint64 sourceId = 0;
};

struct TachiyomiBackupResult {
    QList<BackupManga> manga;
    QList<BackupCategoryEntry> categories;
    QList<BackupSourceEntry> sources;
};

struct TachibkImportSummary {
    int mangaImported = 0;
    int categoriesImported = 0;
    int chaptersMarkedRead = 0;
    int trackerLinksImported = 0;
};

} // namespace mangadl
