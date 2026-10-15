#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <cstdint>

namespace mangadl {

struct BackupMangaEntry {
    QString id;
    QString title;
    QString coverUrl;
    QString provider;
    QString url;
    QString type = QStringLiteral("manga");
    qint64 addedAt = 0;
    QString lastReadChapterId;
    qint64 lastReadAt = 0;
    int totalChapters = 0;
    int readCount = 0;
    QStringList categories;
};

struct BackupProgressEntry {
    QString mangaId;
    QString chapterId;
    QString provider;
    int page = 1;
    int totalPages = 0;
    qint64 readAt = 0;
    bool completed = false;
    double chapterNumber = 0.0;
};

struct BackupTrackerBindEntry {
    QString provider;
    QString mangaId;
    QString tracker; // "AniList" or "MyAnimeList"
    int remoteId = 0;
    QString remoteTitle;
    double lastChapterRead = 0.0;
    double score = 0.0;
    QString status;
};

struct MangaDlBackup {
    QString version = QStringLiteral("1.0");
    QString app = QStringLiteral("manga-dl");
    qint64 exportedAt = 0;
    QList<BackupMangaEntry> library;
    QList<BackupProgressEntry> progress;
    QStringList categories;
    QMap<QString, QStringList> mangaCategories; // key: "provider/mangaId" -> list of category names
    QList<BackupTrackerBindEntry> trackerBinds;
};

struct MangadlRestoreSummary {
    int mangaRestored = 0;
    int progressRestored = 0;
    int categoriesRestored = 0;
    int trackerBindsRestored = 0;
};

} // namespace mangadl
