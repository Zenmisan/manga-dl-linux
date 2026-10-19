#pragma once

#include "Entities.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMutex>
#include <QSet>
#include <QSqlDatabase>
#include <optional>

namespace mangadl {

class MangaDatabase : public QObject {
    Q_OBJECT

public:
    explicit MangaDatabase(const QString &dbPath = QString(), QObject *parent = nullptr);
    ~MangaDatabase() override;

    // Default instance management
    static MangaDatabase *instance();
    static void setInstance(MangaDatabase *customInstance);

    // Lifecycle
    bool initialize();
    bool isInitialized() const { return m_initialized; }
    void close();
    QString dbPath() const { return m_dbPath; }

    // --- Library -----------------------------------------------------------------
    QList<LibraryEntity> getLibrary() const;
    std::optional<LibraryEntity> getLibraryItem(const QString &provider, const QString &mangaId) const;
    bool isInLibrary(const QString &provider, const QString &mangaId) const;
    bool addToLibrary(const LibraryEntity &item);
    bool removeFromLibrary(const QString &provider, const QString &mangaId);
    bool updateTotalChapters(const QString &provider, const QString &mangaId, int totalChapters);
    bool setMangaPinned(const QString &provider, const QString &mangaId, bool pinned);
    bool isMangaPinned(const QString &provider, const QString &mangaId) const;
    bool setMangaMuted(const QString &provider, const QString &mangaId, bool muted);
    bool isMangaMuted(const QString &provider, const QString &mangaId) const;
    int getUnreadCount(const QString &provider, const QString &mangaId) const;
    int getCompletedChaptersCount(const QString &provider, const QString &mangaId) const;
    bool migrateManga(const QString &oldProvider, const QString &oldMangaId,
                      const QString &newProvider, const QString &newMangaId,
                      const QString &newTitle = QString(), const QString &newCoverUrl = QString());

    // --- Reading Progress --------------------------------------------------------
    std::optional<ProgressEntity> getProgress(const QString &provider, const QString &mangaId, const QString &chapterId) const;
    QList<ProgressEntity> getMangaProgress(const QString &provider, const QString &mangaId) const;
    bool saveProgress(const QString &provider, const QString &mangaId, const QString &chapterId,
                      double chapterNumber, int page, bool completed);
    QList<ProgressEntity> getAllProgress() const;
    QList<ProgressEntity> getAllCompletedProgress() const;

    // --- History -----------------------------------------------------------------
    QList<HistoryEntity> getHistory(int limit = 50) const;
    bool recordHistory(const QString &provider, const QString &mangaId, const QString &mangaTitle,
                       const QString &chapterId, const QString &chapterTitle, const QString &coverUrl);
    bool removeFromHistory(const QString &provider, const QString &mangaId, const QString &chapterId);
    bool removeFromHistoryById(const QString &id);
    bool clearHistory();

    // --- Downloads ---------------------------------------------------------------
    QList<DownloadEntity> getDownloads() const;
    std::optional<DownloadEntity> getDownload(const QString &provider, const QString &mangaId, const QString &chapterId) const;
    std::optional<DownloadEntity> getDownloadById(const QString &id) const;
    bool enqueueDownload(const DownloadEntity &item);
    bool updateDownloadProgress(const QString &id, int progress, int totalPages, const QString &status);
    bool updateDownloadCbzPath(const QString &id, const QString &cbzPath);
    bool removeDownload(const QString &id);
    bool clearCompletedDownloads();

    // --- Categories --------------------------------------------------------------
    QList<CategoryEntity> getCategories() const;
    std::optional<CategoryEntity> getCategory(const QString &id) const;
    bool addCategory(const QString &id, const QString &name, int sortOrder = 0);
    bool updateCategory(const QString &id, const QString &name, int sortOrder);
    bool deleteCategory(const QString &id);
    bool setMangaCategories(const QString &provider, const QString &mangaId, const QStringList &categoryIds);
    QStringList getMangaCategories(const QString &provider, const QString &mangaId) const;
    QList<LibraryCategoryEntity> getAllLibraryCategories() const;
    int getMangaCountInCategory(const QString &categoryId) const;

    // --- Tracker Binds -----------------------------------------------------------
    QList<TrackerBindEntity> getTrackerBinds(const QString &provider, const QString &mangaId) const;
    std::optional<TrackerBindEntity> getTrackerBind(const QString &provider, const QString &mangaId, const QString &tracker) const;
    bool saveTrackerBind(const TrackerBindEntity &bind);
    bool deleteTrackerBind(const QString &provider, const QString &mangaId, const QString &tracker);
    QList<TrackerBindEntity> getAllTrackerBinds() const;

    // --- New Chapters (Updates) --------------------------------------------------
    QList<NewChapterEntity> getNewChapters() const;
    bool saveNewChapters(const QList<NewChapterEntity> &entries);
    bool removeNewChapter(const QString &id);
    bool clearNewChapters();

    // --- Manga Notes & Ratings ---------------------------------------------------
    std::optional<MangaNoteEntity> getMangaNote(const QString &provider, const QString &mangaId) const;
    bool saveMangaNote(const QString &provider, const QString &mangaId, const QString &note, int rating, qint64 updatedAt = 0);
    QList<MangaNoteEntity> getAllMangaNotes() const;

    // --- Manga Metadata Overrides ------------------------------------------------
    std::optional<MangaOverrideEntity> getMangaOverride(const QString &provider, const QString &mangaId) const;
    bool saveMangaOverride(const MangaOverrideEntity &overrideEntity);
    bool deleteMangaOverride(const QString &provider, const QString &mangaId);

    // --- Chapter Bookmarks & Hidden Chapters -------------------------------------
    QStringList getBookmarkedChapters(const QString &provider, const QString &mangaId) const;
    bool isChapterBookmarked(const QString &provider, const QString &mangaId, const QString &chapterId) const;
    bool setChapterBookmarked(const QString &provider, const QString &mangaId, const QString &chapterId, bool bookmarked);

    QStringList getHiddenChapters(const QString &provider, const QString &mangaId) const;
    bool isChapterHidden(const QString &provider, const QString &mangaId, const QString &chapterId) const;
    bool setChapterHidden(const QString &provider, const QString &mangaId, const QString &chapterId, bool hidden);
    bool clearHiddenChapters(const QString &provider, const QString &mangaId);

    // --- Reading Sessions --------------------------------------------------------
    bool recordReadingSession(const ReadingSessionEntity &session);
    QList<ReadingSessionEntity> getReadingSessions(int limit = 500) const;

    // --- Comments (Local + Offline Cache) ----------------------------------------
    QList<CommentEntity> getComments(const QString &provider, const QString &mangaId, const QString &chapterId = QString()) const;
    bool saveComment(const CommentEntity &comment);
    bool toggleCommentLike(const QString &commentId);
    bool deleteComment(const QString &commentId);
    QList<CommentEntity> getUserComments(const QString &userId = QString()) const;

signals:
    void libraryChanged();
    void libraryItemChanged(const QString &provider, const QString &mangaId);
    void progressChanged(const QString &provider, const QString &mangaId, const QString &chapterId);
    void historyChanged();
    void categoriesChanged();
    void downloadsChanged();
    void downloadUpdated(const QString &id);
    void newChaptersChanged();
    void trackerBindsChanged();
    void mangaNotesChanged(const QString &provider, const QString &mangaId);
    void mangaOverridesChanged(const QString &provider, const QString &mangaId);
    void bookmarksChanged(const QString &provider, const QString &mangaId);
    void hiddenChaptersChanged(const QString &provider, const QString &mangaId);
    void readingSessionsChanged();
    void commentsChanged(const QString &provider, const QString &mangaId);

private:
    QSqlDatabase getThreadConnection() const;
    bool runMigrations(QSqlDatabase &db);
    bool applyMigration1(QSqlDatabase &db);
    bool applyMigration2(QSqlDatabase &db);

    QString m_dbPath;
    bool m_initialized = false;
    mutable QMutex m_writeLock;
    mutable QMutex m_connLock;
    mutable QSet<QString> m_activeConnections;
};

} // namespace mangadl
