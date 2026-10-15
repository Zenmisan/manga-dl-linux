#include "DownloadManager.h"

#include "CbzBuilder.h"
#include "NovelEpubBuilder.h"
#include "../extensions/ExtensionManager.h"
#include "../extensions/Descramble.h"
#include "../http/HttpService.h"
#include "../settings/AppSettings.h"
#include "../util/AppPaths.h"
#include "../util/FileNaming.h"

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace mangadl {

DownloadManager::DownloadManager(MangaDatabase *db, QObject *parent)
    : QObject(parent)
    , m_db(db ? db : MangaDatabase::instance())
    , m_nam(new QNetworkAccessManager(this))
    , m_alive(std::make_shared<std::atomic<bool>>(true))
{
    // Reset any interrupted downloads from a previous session back to queued
    if (m_db) {
        const auto all = m_db->getDownloads();
        for (const auto &d : all) {
            if (d.status == QStringLiteral("downloading")) {
                m_db->updateDownloadProgress(d.id, d.progress, d.totalPages, QStringLiteral("queued"));
            }
        }
    }

    QTimer::singleShot(500, this, [this]() {
        processQueue();
    });
}

DownloadManager::~DownloadManager()
{
    *m_alive = false;
}

DownloadManager &DownloadManager::instance()
{
    static DownloadManager s_instance;
    return s_instance;
}

void DownloadManager::enqueue(const QString &provider,
                             const QString &mangaId,
                             const QString &mangaTitle,
                             const QString &chapterId,
                             const QString &chapterTitle,
                             double chapterNumber,
                             const QString &coverUrl)
{
    if (!m_db) return;

    const QString id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, chapterId);
    auto existing = m_db->getDownloadById(id);
    if (existing && existing->status == QStringLiteral("completed") && !existing->cbzPath.isEmpty() && QFile::exists(existing->cbzPath)) {
        return;
    }

    DownloadEntity entity;
    entity.id = id;
    entity.provider = provider;
    entity.mangaId = mangaId;
    entity.mangaTitle = mangaTitle;
    entity.chapterId = chapterId;
    entity.chapterTitle = chapterTitle.isEmpty() ? QStringLiteral("Chapter %1").arg(chapterNumber) : chapterTitle;
    entity.chapterNumber = chapterNumber;
    entity.status = QStringLiteral("queued");
    entity.progress = 0;
    entity.totalPages = 0;
    Q_UNUSED(coverUrl);

    m_db->enqueueDownload(entity);
    processQueue();
}

void DownloadManager::pause()
{
    m_paused = true;
    emit pauseStateChanged(true);
}

void DownloadManager::resume()
{
    m_paused = false;
    emit pauseStateChanged(false);
    processQueue();
}

void DownloadManager::pauseDownload(const QString &id)
{
    if (!m_db) return;
    auto item = m_db->getDownloadById(id);
    if (item) {
        m_db->updateDownloadProgress(id, item->progress, item->totalPages, QStringLiteral("paused"));
    }
}

void DownloadManager::resumeDownload(const QString &id)
{
    if (!m_db) return;
    auto item = m_db->getDownloadById(id);
    if (item) {
        m_db->updateDownloadProgress(id, item->progress, item->totalPages, QStringLiteral("queued"));
        processQueue();
    }
}

void DownloadManager::retryDownload(const QString &id)
{
    if (!m_db) return;
    auto item = m_db->getDownloadById(id);
    if (item) {
        m_db->updateDownloadProgress(id, 0, item->totalPages, QStringLiteral("queued"));
        processQueue();
    }
}

void DownloadManager::cancelDownload(const QString &id)
{
    if (!m_db) return;
    m_activeIds.remove(id);
    m_db->removeDownload(id);
    processQueue();
}

void DownloadManager::clearFinished()
{
    if (!m_db) return;
    m_db->clearCompletedDownloads();
}

void DownloadManager::processQueue()
{
    if (m_paused || !m_db) return;

    int maxParallel = AppSettings::instance()->parallelDownloads();
    if (maxParallel <= 0) maxParallel = 3;
    maxParallel = qBound(1, maxParallel, 10);

    if (m_activeIds.size() >= maxParallel)
        return;

    const auto all = m_db->getDownloads();
    for (const auto &item : all) {
        if (m_activeIds.size() >= maxParallel)
            break;

        if (item.status == QStringLiteral("queued") && !m_activeIds.contains(item.id)) {
            m_activeIds.insert(item.id);
            executeDownload(item);
        }
    }
}

void DownloadManager::executeDownload(const DownloadEntity &item)
{
    if (!m_db) return;

    m_db->updateDownloadProgress(item.id, 0, 0, QStringLiteral("downloading"));
    emit downloadStarted(item.id);

    const auto *ext = ExtensionManager::instance().findById(item.provider);
    const bool isNovel = (ext && ext->type == ExtensionType::Novel);

    if (isNovel) {
        executeNovelDownload(item);
    } else {
        executeMangaDownload(item);
    }
}

void DownloadManager::executeNovelDownload(const DownloadEntity &item)
{
    auto alive = m_alive;
    ExtensionManager::instance().getChapterText(
        item.provider, item.chapterId,
        [this, alive, item](QString text, SourceError err) {
            if (!*alive) return;

            if (!err.message.isEmpty() && text.isEmpty()) {
                if (m_db)
                    m_db->updateDownloadProgress(item.id, 0, 1, QStringLiteral("failed"));
                m_activeIds.remove(item.id);
                emit downloadFailed(item.id, err.message);
                processQueue();
                return;
            }

            QString baseDir = AppSettings::instance()->downloadPath();
            if (baseDir.isEmpty())
                baseDir = AppPaths::defaultDownloadDir();

            const QString safeManga = FileNaming::sanitizeFileName(item.mangaTitle);
            const QString safeChapter = FileNaming::sanitizeFileName(item.chapterTitle);
            const QString targetFolder = QDir(baseDir).filePath(safeManga);
            const QString epubPath = QDir(targetFolder).filePath(safeChapter + QStringLiteral(".epub"));

            QString buildErr;
            const bool ok = NovelEpubBuilder::buildEpub(
                epubPath, item.mangaTitle, item.chapterTitle, text, QStringLiteral("html"), &buildErr);

            if (!ok) {
                if (m_db)
                    m_db->updateDownloadProgress(item.id, 0, 1, QStringLiteral("failed"));
                m_activeIds.remove(item.id);
                emit downloadFailed(item.id, buildErr);
                processQueue();
                return;
            }

            if (m_db) {
                auto updated = item;
                updated.cbzPath = epubPath;
                updated.status = QStringLiteral("completed");
                updated.progress = 100;
                updated.totalPages = 1;
                updated.completedAt = QDateTime::currentMSecsSinceEpoch();
                m_db->enqueueDownload(updated);
                m_db->updateDownloadProgress(item.id, 100, 1, QStringLiteral("completed"));
            }

            m_activeIds.remove(item.id);
            emit downloadProgressChanged(item.id, 100, 1);
            emit downloadCompleted(item.id, epubPath);
            processQueue();
        });
}

void DownloadManager::executeMangaDownload(const DownloadEntity &item)
{
    auto alive = m_alive;
    ExtensionManager::instance().getPages(
        item.provider, item.chapterId,
        [this, alive, item](QList<PageInfo> pages, SourceError err) {
            if (!*alive) return;

            if (!err.message.isEmpty() && pages.isEmpty()) {
                if (m_db)
                    m_db->updateDownloadProgress(item.id, 0, 0, QStringLiteral("failed"));
                m_activeIds.remove(item.id);
                emit downloadFailed(item.id, err.message);
                processQueue();
                return;
            }

            if (pages.isEmpty()) {
                if (m_db)
                    m_db->updateDownloadProgress(item.id, 0, 0, QStringLiteral("failed"));
                m_activeIds.remove(item.id);
                emit downloadFailed(item.id, QStringLiteral("No pages returned for chapter"));
                processQueue();
                return;
            }

            QStringList urls;
            urls.reserve(pages.size());
            for (const auto &p : pages) {
                urls.append(p.url);
            }

            if (m_db) {
                m_db->updateDownloadProgress(item.id, 0, static_cast<int>(urls.size()), QStringLiteral("downloading"));
            }

            downloadNextPage(item, urls, 0, {}, {});
        });
}

void DownloadManager::downloadNextPage(const DownloadEntity &item,
                                      const QStringList &urls,
                                      int pageIndex,
                                      QList<QByteArray> collectedPages,
                                      QStringList collectedExts)
{
    auto alive = m_alive;
    if (pageIndex >= urls.size()) {
        // All pages fetched, assemble CBZ
        QString baseDir = AppSettings::instance()->downloadPath();
        if (baseDir.isEmpty())
            baseDir = AppPaths::defaultDownloadDir();

        const QString safeManga = FileNaming::sanitizeFileName(item.mangaTitle);
        const QString safeChapter = FileNaming::sanitizeFileName(item.chapterTitle);
        const QString targetFolder = QDir(baseDir).filePath(safeManga);
        const QString cbzPath = QDir(targetFolder).filePath(safeChapter + QStringLiteral(".cbz"));

        QString buildErr;
        const bool ok = CbzBuilder::buildCbz(
            cbzPath,
            item.mangaTitle,
            item.chapterTitle,
            QString::number(item.chapterNumber),
            collectedPages,
            collectedExts,
            &buildErr);

        if (!ok) {
            if (m_db)
                m_db->updateDownloadProgress(item.id, 0, static_cast<int>(urls.size()), QStringLiteral("failed"));
            m_activeIds.remove(item.id);
            emit downloadFailed(item.id, buildErr);
            processQueue();
            return;
        }

        if (m_db) {
            auto updated = item;
            updated.cbzPath = cbzPath;
            updated.status = QStringLiteral("completed");
            updated.progress = 100;
            updated.totalPages = static_cast<int>(urls.size());
            updated.completedAt = QDateTime::currentMSecsSinceEpoch();
            m_db->enqueueDownload(updated);
            m_db->updateDownloadProgress(item.id, 100, updated.totalPages, QStringLiteral("completed"));
        }

        m_activeIds.remove(item.id);
        emit downloadProgressChanged(item.id, 100, static_cast<int>(urls.size()));
        emit downloadCompleted(item.id, cbzPath);
        processQueue();
        return;
    }

    const QString rawUrl = urls[pageIndex];
    QString realFetchUrl = rawUrl;
    QString referer;

    const auto comixto = Descramble::parseComixtoUrl(rawUrl);
    const auto asura = Descramble::parseAsuraUrl(rawUrl);

    if (comixto.valid) {
        realFetchUrl = comixto.imageUrl;
    } else if (asura.valid) {
        realFetchUrl = asura.imageUrl;
    }

    const QUrl target(realFetchUrl);
    if (target.isValid()) {
        referer = target.scheme() + QStringLiteral("://") + target.host() + QStringLiteral("/");
    }

    QNetworkRequest request(target);
    request.setHeader(QNetworkRequest::UserAgentHeader, HttpService::userAgent());
    if (!referer.isEmpty()) {
        request.setRawHeader("Referer", referer.toUtf8());
    }

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, alive, reply, item, urls, pageIndex, collectedPages, collectedExts, comixto, asura]() mutable {
        reply->deleteLater();
        if (!*alive) return;

        if (reply->error() != QNetworkReply::NoError) {
            if (m_db)
                m_db->updateDownloadProgress(item.id, 0, static_cast<int>(urls.size()), QStringLiteral("failed"));
            m_activeIds.remove(item.id);
            emit downloadFailed(item.id, reply->errorString());
            processQueue();
            return;
        }

        QByteArray data = reply->readAll();
        QString ext = QFileInfo(reply->url().path()).suffix();
        if (ext.isEmpty()) ext = QStringLiteral("jpg");

        // Descramble if needed
        if (comixto.valid && !data.isEmpty()) {
            QImage descrambled = Descramble::descrambleComixto(data, comixto);
            if (!descrambled.isNull()) {
                QByteArray out;
                QBuffer buf(&out);
                buf.open(QIODevice::WriteOnly);
                descrambled.save(&buf, "JPG", 90);
                data = out;
                ext = QStringLiteral("jpg");
            }
        } else if (asura.valid && !data.isEmpty()) {
            QImage descrambled = Descramble::reassembleAsura(data, asura);
            if (!descrambled.isNull()) {
                QByteArray out;
                QBuffer buf(&out);
                buf.open(QIODevice::WriteOnly);
                descrambled.save(&buf, "JPG", 90);
                data = out;
                ext = QStringLiteral("jpg");
            }
        }

        collectedPages.append(data);
        collectedExts.append(ext);

        const int progress = static_cast<int>(((pageIndex + 1) / static_cast<double>(urls.size())) * 100);
        if (m_db) {
            m_db->updateDownloadProgress(item.id, progress, static_cast<int>(urls.size()), QStringLiteral("downloading"));
        }
        emit downloadProgressChanged(item.id, progress, static_cast<int>(urls.size()));

        downloadNextPage(item, urls, pageIndex + 1, collectedPages, collectedExts);
    });
}

} // namespace mangadl
