#pragma once

#include "../db/MangaDatabase.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QString>
#include <atomic>
#include <memory>

#include "../extensions/ExtensionManager.h"

namespace mangadl {

class DownloadManager : public QObject {
    Q_OBJECT

public:
    explicit DownloadManager(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~DownloadManager() override;

    static DownloadManager &instance();

    bool isPaused() const { return m_paused; }
    int activeCount() const { return m_activeIds.size(); }

    Q_INVOKABLE void enqueue(const QString &provider,
                            const QString &mangaId,
                            const QString &mangaTitle,
                            const QString &chapterId,
                            const QString &chapterTitle,
                            double chapterNumber,
                            const QString &coverUrl = QString());

    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void pauseDownload(const QString &id);
    Q_INVOKABLE void resumeDownload(const QString &id);
    Q_INVOKABLE void retryDownload(const QString &id);
    Q_INVOKABLE void cancelDownload(const QString &id);
    Q_INVOKABLE void clearFinished();

    /// Process the queue, respecting maximum concurrent downloads setting.
    void processQueue();

signals:
    void downloadStarted(const QString &id);
    void downloadProgressChanged(const QString &id, int progress, int totalPages);
    void downloadCompleted(const QString &id, const QString &cbzPath);
    void downloadFailed(const QString &id, const QString &error);
    void pauseStateChanged(bool isPaused);

private:
    void executeDownload(const DownloadEntity &item);
    void executeNovelDownload(const DownloadEntity &item);
    void executeMangaDownload(const DownloadEntity &item);
    void downloadNextPage(const DownloadEntity &item,
                          const QStringList &urls,
                          int pageIndex,
                          QList<QByteArray> collectedPages,
                          QStringList collectedExts);

    MangaDatabase *m_db = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    bool m_paused = false;
    QSet<QString> m_activeIds;
    std::shared_ptr<std::atomic<bool>> m_alive;
};

} // namespace mangadl
