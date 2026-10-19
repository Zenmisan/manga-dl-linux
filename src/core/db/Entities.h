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
    int pinned = 0; // 0 or 1
    int muted = 0;  // 0 or 1

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
               updatedAt == other.updatedAt &&
               pinned == other.pinned &&
               muted == other.muted;
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
    QString tracker; // "AniList" | "MyAnimeList" | "Kitsu" | "MangaUpdates" | "Shikimori" | "Bangumi"
    int remoteId = 0;
    QString remoteTitle;
    double lastChapterRead = 0.0;
    QString status = QStringLiteral("CURRENT"); // CURRENT|COMPLETED|PAUSED|DROPPED|PLANNING
    double score = 0.0;
    QString startDate;
    QString endDate;
    qint64 updatedAt = 0;

    bool operator==(const TrackerBindEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               tracker == other.tracker &&
               remoteId == other.remoteId &&
               remoteTitle == other.remoteTitle &&
               lastChapterRead == other.lastChapterRead &&
               status == other.status &&
               score == other.score &&
               startDate == other.startDate &&
               endDate == other.endDate &&
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

struct MangaNoteEntity {
    QString id; // "{provider}/{manga_id}"
    QString provider;
    QString mangaId;
    QString note;
    int rating = 0; // 0 to 5
    qint64 updatedAt = 0;

    bool operator==(const MangaNoteEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               note == other.note &&
               rating == other.rating &&
               updatedAt == other.updatedAt;
    }
};

struct MangaOverrideEntity {
    QString id; // "{provider}/{manga_id}"
    QString provider;
    QString mangaId;
    QString customTitle;
    QString customCoverUrl;
    QString customDescription;
    qint64 updatedAt = 0;

    bool operator==(const MangaOverrideEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               customTitle == other.customTitle &&
               customCoverUrl == other.customCoverUrl &&
               customDescription == other.customDescription &&
               updatedAt == other.updatedAt;
    }
};

struct ChapterBookmarkEntity {
    QString id; // "{provider}/{manga_id}/{chapter_id}"
    QString provider;
    QString mangaId;
    QString chapterId;
    qint64 createdAt = 0;

    bool operator==(const ChapterBookmarkEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               chapterId == other.chapterId &&
               createdAt == other.createdAt;
    }
};

struct ReadingSessionEntity {
    QString id; // "{provider}/{manga_id}/{chapter_id}/{started_at}"
    QString provider;
    QString mangaId;
    QString chapterId;
    QString mangaTitle;
    qint64 startedAt = 0;
    qint64 endedAt = 0;
    int durationSecs = 0;
    int pagesRead = 0;

    bool operator==(const ReadingSessionEntity &other) const {
        return id == other.id &&
               provider == other.provider &&
               mangaId == other.mangaId &&
               chapterId == other.chapterId &&
               mangaTitle == other.mangaTitle &&
               startedAt == other.startedAt &&
               endedAt == other.endedAt &&
               durationSecs == other.durationSecs &&
               pagesRead == other.pagesRead;
    }
};

struct CommentEntity {
    QString id;
    QString provider;
    QString mangaId;
    QString chapterId; // empty for manga-level comment
    QString mangaTitle;
    QString userId;
    QString username;
    QString avatarUrl;
    QString parentId; // empty for top-level comment
    QString text;
    int likesCount = 0;
    bool likedByMe = false;
    qint64 createdAt = 0;
};

} // namespace mangadl
