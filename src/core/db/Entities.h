#pragma once

#include <QString>
#include <QDateTime>

namespace mangadl {

struct LibraryEntity {
    QString id; // "{provider}/{manga_id}"
    QString provider;
    QString mangaId;
    QString title;
    QString coverUrl;
    QString url;
    QString type = QStringLiteral("manga"); // "manga" | "novel"
    int totalChapters = 0;
    qint64 addedAt = 0;
    qint64 updatedAt = 0;

    bool operator==(const LibraryEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               title == other.title &&
               coverUrl == other.coverUrl &&
               url == other.url &&
               type == other.type &&
               totalChapters == other.totalChapters &&
               addedAt == other.addedAt &&
               updatedAt == other.updatedAt;
    }
};

struct ProgressEntity {
    QString id; // "{provider}/{manga_id}/{chapter_id}"
    QString provider;
    QString mangaId;
    QString chapterId;
    double chapterNumber = 0.0;
    int page = 0;
    int completed = 0; // 0 or 1
    qint64 readAt = 0;

    bool operator==(const ProgressEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               chapterId == other.chapterId &&
               chapterNumber == other.chapterNumber &&
               page == other.page &&
               completed == other.completed &&
               readAt == other.readAt;
    }
};

struct DownloadEntity {
    QString id; // "{provider}/{manga_id}/{chapter_id}"
    QString provider;
    QString mangaId;
    QString mangaTitle;
    QString chapterId;
    QString chapterTitle;
    double chapterNumber = 0.0;
    QString cbzPath;
    QString status = QStringLiteral("queued"); // queued|downloading|completed|failed|paused
    int progress = 0;
    int totalPages = 0;
    qint64 queuedAt = 0;
    qint64 completedAt = 0; // 0 if not completed

    bool operator==(const DownloadEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               mangaTitle == other.mangaTitle &&
               chapterId == other.chapterId &&
               chapterTitle == other.chapterTitle &&
               chapterNumber == other.chapterNumber &&
               cbzPath == other.cbzPath &&
               status == other.status &&
               progress == other.progress &&
               totalPages == other.totalPages &&
               queuedAt == other.queuedAt &&
               completedAt == other.completedAt;
    }
};

struct CategoryEntity {
    QString id;
    QString name;
    int sortOrder = 0;

    bool operator==(const CategoryEntity &other) const {
        return id == other.id &&
               name == other.name &&
               sortOrder == other.sortOrder;
    }
};

struct LibraryCategoryEntity {
    QString libraryId;
    QString categoryId;

    bool operator==(const LibraryCategoryEntity &other) const {
        return libraryId == other.libraryId &&
               categoryId == other.categoryId;
    }
};

struct HistoryEntity {
    QString id; // "{provider}/{manga_id}/{chapter_id}"
    QString provider;
    QString mangaId;
    QString mangaTitle;
    QString chapterId;
    QString chapterTitle;
    QString coverUrl;
    qint64 readAt = 0;

    bool operator==(const HistoryEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               mangaTitle == other.mangaTitle &&
               chapterId == other.chapterId &&
               chapterTitle == other.chapterTitle &&
               coverUrl == other.coverUrl &&
               readAt == other.readAt;
    }
};

struct TrackerBindEntity {
    QString id; // "{provider}/{manga_id}/{tracker}"
    QString provider;
    QString mangaId;
    QString tracker; // "AniList" | "MyAnimeList"
    int remoteId = 0;
    QString remoteTitle;
    double lastChapterRead = 0.0;
    qint64 updatedAt = 0;

    bool operator==(const TrackerBindEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               tracker == other.tracker &&
               remoteId == other.remoteId &&
               remoteTitle == other.remoteTitle &&
               lastChapterRead == other.lastChapterRead &&
               updatedAt == other.updatedAt;
    }
};

struct NewChapterEntity {
    QString id; // "{provider}/{manga_id}/{chapter_id}"
    QString mangaId;
    QString mangaTitle;
    QString coverUrl;
    QString provider;
    QString chapterId;
    QString chapterTitle;
    double chapterNumber = 0.0;
    qint64 detectedAt = 0;

    bool operator==(const NewChapterEntity &other) const {
        return id == other.id &&
               mangaId == other.mangaId &&
               mangaTitle == other.mangaTitle &&
               coverUrl == other.coverUrl &&
               provider == other.provider &&
               chapterId == other.chapterId &&
               chapterTitle == other.chapterTitle &&
               chapterNumber == other.chapterNumber &&
               detectedAt == other.detectedAt;
    }
};

} // namespace mangadl
