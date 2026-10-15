#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QTimer>
#include "SupabaseSyncModels.h"
#include "db/MangaDatabase.h"

namespace mangadl {

class SupabaseSyncService : public QObject {
    Q_OBJECT

public:
    explicit SupabaseSyncService(MangaDatabase *db = nullptr,
                                 const QString &supabaseUrl = QString(),
                                 const QString &anonKey = QString(),
                                 QObject *parent = nullptr);
    ~SupabaseSyncService() override = default;

    bool isConfigured() const;
    void setCredentials(const QString &url, const QString &anonKey);
    void setDatabase(MangaDatabase *db) { m_db = db; }

    // Bidirectional sync
    SyncResult syncAll(const QString &userId, const QString &accessToken);
    void syncAllAsync(const QString &userId, const QString &accessToken);

    // Pull operations
    int pullLibrary(const QString &userId, const QString &accessToken);
    int pullReadTracking(const QString &userId, const QString &accessToken);
    int pullReadingProgressAndHistory(const QString &userId, const QString &accessToken);

    // Push operations
    bool pushLibraryItem(const QString &userId, const QString &accessToken, const LibraryEntity &item, bool subscribed);
    bool pushChapterProgress(const QString &userId, const QString &accessToken,
                            const QString &provider, const QString &mangaId,
                            const QString &chapterId, int page,
                            const QString &mangaTitle, const QString &chapterTitle,
                            bool isCompleted);
    int pushAllLibrary(const QString &userId, const QString &accessToken);
    int pushAllReadTracking(const QString &userId, const QString &accessToken);

    // Debounced chapter sync
    void queueChapterSync(const QString &userId, const QString &accessToken,
                          const QString &provider, const QString &mangaId,
                          const QString &chapterId, int page,
                          const QString &mangaTitle, const QString &chapterTitle,
                          bool isCompleted);

signals:
    void syncStarted();
    void syncProgress(const QString &message);
    void syncFinished(const mangadl::SyncResult &result);

private:
    QByteArray executeGet(const QString &path, const QString &accessToken, bool *ok = nullptr, int timeoutMs = 15000);
    QByteArray executePostUpsert(const QString &path, const QByteArray &body, const QString &accessToken, bool *ok = nullptr, int timeoutMs = 15000);
    bool executeDelete(const QString &path, const QString &accessToken, int timeoutMs = 15000);

    MangaDatabase *m_db = nullptr;
    QString m_url;
    QString m_anonKey;
    QNetworkAccessManager m_net;

    struct PendingProgress {
        QString userId;
        QString accessToken;
        QString provider;
        QString mangaId;
        QString chapterId;
        int page;
        QString mangaTitle;
        QString chapterTitle;
        bool isCompleted;
    };
    QList<PendingProgress> m_pendingProgressQueue;
    QTimer m_debounceTimer;
};

} // namespace mangadl
