#include "DownloadsModel.h"
#include "../downloads/DownloadManager.h"
#include "../settings/AppSettings.h"
#include "../util/AppPaths.h"
#include <QDir>
#include <QDirIterator>

namespace mangadl {

QVariantMap DownloadsModel::Item::toMap() const
{
    QVariantMap mangaMap{
        {QStringLiteral("id"), entity.mangaId},
        {QStringLiteral("title"), entity.mangaTitle},
        {QStringLiteral("cover"), coverUrl.isEmpty() ? QStringLiteral("#1A2433") : coverUrl},
        {QStringLiteral("source"), entity.provider},
        {QStringLiteral("provider"), entity.provider},
        {QStringLiteral("inLibrary"), true}
    };

    return QVariantMap{
        {QStringLiteral("manga"), mangaMap},
        {QStringLiteral("chapter"), chapter},
        {QStringLiteral("status"), statusText},
        {QStringLiteral("progress"), progressFraction},
        {QStringLiteral("state"), state},
        {QStringLiteral("id"), entity.id},
        {QStringLiteral("cbzPath"), entity.cbzPath}
    };
}

DownloadsModel::DownloadsModel(MangaDatabase *db, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(db ? db : MangaDatabase::instance())
{
    if (m_db) {
        connect(m_db, &MangaDatabase::downloadsChanged, this, &DownloadsModel::refresh);
    }
    connect(&DownloadManager::instance(), &DownloadManager::downloadProgressChanged, this, &DownloadsModel::refresh);
    connect(&DownloadManager::instance(), &DownloadManager::downloadCompleted, this, &DownloadsModel::refresh);
    connect(&DownloadManager::instance(), &DownloadManager::downloadFailed, this, &DownloadsModel::refresh);
    reload();
}

DownloadsModel &DownloadsModel::instance()
{
    static DownloadsModel s_instance;
    return s_instance;
}

int DownloadsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant DownloadsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return QVariant();
    }

    const auto &item = m_items[static_cast<size_t>(index.row())];

    switch (role) {
    case Qt::DisplayRole:
    case ModelDataRole:
        return item.toMap();
    case MangaRole: {
        return QVariantMap{
            {QStringLiteral("id"), item.entity.mangaId},
            {QStringLiteral("title"), item.entity.mangaTitle},
            {QStringLiteral("cover"), item.coverUrl.isEmpty() ? QStringLiteral("#1A2433") : item.coverUrl},
            {QStringLiteral("source"), item.entity.provider},
            {QStringLiteral("provider"), item.entity.provider},
            {QStringLiteral("inLibrary"), true}
        };
    }
    case ChapterRole:
        return item.chapter;
    case StatusRole:
        return item.statusText;
    case ProgressRole:
        return item.progressFraction;
    case StateRole:
        return item.state;
    case IdRole:
        return item.entity.id;
    case CbzPathRole:
        return item.entity.cbzPath;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DownloadsModel::roleNames() const
{
    return {
        {ModelDataRole, "modelData"},
        {MangaRole, "manga"},
        {ChapterRole, "chapter"},
        {StatusRole, "status"},
        {ProgressRole, "progress"},
        {StateRole, "state"},
        {IdRole, "id"},
        {CbzPathRole, "cbzPath"}
    };
}

void DownloadsModel::setFilterTab(const QString &tab)
{
    if (m_filterTab != tab) {
        m_filterTab = tab;
        reload();
        emit filterTabChanged();
    }
}

void DownloadsModel::refresh()
{
    reload();
}

void DownloadsModel::pauseAll()
{
    m_isPaused = !m_isPaused;
    if (m_isPaused) {
        DownloadManager::instance().pause();
    } else {
        DownloadManager::instance().resume();
    }
    if (m_db) {
        const auto downloads = m_db->getDownloads();
        for (const auto &d : downloads) {
            if (m_isPaused && (d.status == QStringLiteral("queued") || d.status == QStringLiteral("downloading"))) {
                m_db->updateDownloadProgress(d.id, d.progress, d.totalPages, QStringLiteral("paused"));
            } else if (!m_isPaused && d.status == QStringLiteral("paused")) {
                m_db->updateDownloadProgress(d.id, d.progress, d.totalPages, QStringLiteral("queued"));
            }
        }
    }
    emit pauseStateChanged();
}

void DownloadsModel::resumeAll()
{
    m_isPaused = false;
    DownloadManager::instance().resume();
    if (m_db) {
        const auto downloads = m_db->getDownloads();
        for (const auto &d : downloads) {
            if (d.status == QStringLiteral("paused")) {
                m_db->updateDownloadProgress(d.id, d.progress, d.totalPages, QStringLiteral("queued"));
            }
        }
    }
    emit pauseStateChanged();
}

void DownloadsModel::clearFinished()
{
    if (m_db && m_db != MangaDatabase::instance()) {
        const auto downloads = m_db->getDownloads();
        for (const auto &d : downloads) {
            if (d.status == QStringLiteral("completed") || d.status == QStringLiteral("failed")) {
                m_db->removeDownload(d.id);
            }
        }
    } else {
        DownloadManager::instance().clearFinished();
    }
    reload();
}

void DownloadsModel::pauseDownload(const QString &id)
{
    if (m_db && m_db != MangaDatabase::instance()) {
        auto item = m_db->getDownloadById(id);
        if (item) {
            m_db->updateDownloadProgress(id, item->progress, item->totalPages, QStringLiteral("paused"));
        }
    } else {
        DownloadManager::instance().pauseDownload(id);
    }
    reload();
}

void DownloadsModel::resumeDownload(const QString &id)
{
    if (m_db && m_db != MangaDatabase::instance()) {
        auto item = m_db->getDownloadById(id);
        if (item) {
            m_db->updateDownloadProgress(id, item->progress, item->totalPages, QStringLiteral("queued"));
        }
    } else {
        DownloadManager::instance().resumeDownload(id);
    }
    reload();
}

void DownloadsModel::retryDownload(const QString &id)
{
    if (m_db && m_db != MangaDatabase::instance()) {
        auto item = m_db->getDownloadById(id);
        if (item) {
            m_db->updateDownloadProgress(id, 0, item->totalPages, QStringLiteral("queued"));
        }
    } else {
        DownloadManager::instance().retryDownload(id);
    }
    reload();
}

void DownloadsModel::cancelDownload(const QString &id)
{
    if (m_db && m_db != MangaDatabase::instance()) {
        m_db->removeDownload(id);
    } else {
        DownloadManager::instance().cancelDownload(id);
    }
    reload();
}

void DownloadsModel::enqueue(const QString &provider, const QString &mangaId, const QString &mangaTitle,
                            const QString &chapterId, const QString &chapterTitle, double chapterNumber,
                            const QString &coverUrl)
{
    if (m_db && m_db != MangaDatabase::instance()) {
        DownloadEntity entity;
        entity.id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, chapterId);
        entity.provider = provider;
        entity.mangaId = mangaId;
        entity.chapterId = chapterId;
        entity.mangaTitle = mangaTitle;
        entity.chapterTitle = chapterTitle.isEmpty() ? QStringLiteral("Chapter %1").arg(chapterNumber) : chapterTitle;
        entity.chapterNumber = chapterNumber;
        entity.status = QStringLiteral("queued");
        entity.progress = 0;
        entity.totalPages = 0;
        m_db->enqueueDownload(entity);
    } else {
        DownloadManager::instance().enqueue(provider, mangaId, mangaTitle, chapterId, chapterTitle, chapterNumber, coverUrl);
    }
    reload();
}

void DownloadsModel::reload()
{
    beginResetModel();
    m_items.clear();
    m_queueCount = 0;
    m_finishedCount = 0;
    m_failedCount = 0;

    if (!m_db) {
        endResetModel();
        emit countChanged();
        emit countsChanged();
        return;
    }

    const auto downloads = m_db->getDownloads();

    for (const auto &d : downloads) {
        if (d.status == QStringLiteral("completed")) {
            m_finishedCount++;
        } else if (d.status == QStringLiteral("failed")) {
            m_failedCount++;
        } else {
            m_queueCount++;
        }
    }

    const QString tab = m_filterTab.toLower();

    for (const auto &d : downloads) {
        bool match = false;
        if (tab == QStringLiteral("finished")) {
            match = (d.status == QStringLiteral("completed"));
        } else if (tab == QStringLiteral("failed")) {
            match = (d.status == QStringLiteral("failed"));
        } else if (tab == QStringLiteral("all")) {
            match = true;
        } else { // default: queue
            match = (d.status == QStringLiteral("queued") ||
                     d.status == QStringLiteral("downloading") ||
                     d.status == QStringLiteral("paused"));
        }

        if (!match) continue;

        Item row;
        row.entity = d;

        // State mapping for QML
        if (d.status == QStringLiteral("completed")) {
            row.state = QStringLiteral("done");
            row.statusText = QStringLiteral("Done");
            row.progressFraction = 1.0;
        } else if (d.status == QStringLiteral("downloading")) {
            row.state = QStringLiteral("downloading");
            row.statusText = (d.totalPages > 0)
                ? QStringLiteral("Downloading · %1 / %2 pages").arg(d.progress).arg(d.totalPages)
                : QStringLiteral("Downloading…");
            row.progressFraction = (d.totalPages > 0) ? (static_cast<double>(d.progress) / d.totalPages) : 0.0;
        } else if (d.status == QStringLiteral("paused")) {
            row.state = QStringLiteral("paused");
            row.statusText = QStringLiteral("Paused");
            row.progressFraction = (d.totalPages > 0) ? (static_cast<double>(d.progress) / d.totalPages) : 0.0;
        } else if (d.status == QStringLiteral("failed")) {
            row.state = QStringLiteral("failed");
            row.statusText = QStringLiteral("Failed");
            row.progressFraction = (d.totalPages > 0) ? (static_cast<double>(d.progress) / d.totalPages) : 0.0;
        } else {
            row.state = QStringLiteral("queued");
            row.statusText = QStringLiteral("Queued");
            row.progressFraction = 0.0;
        }

        // Chapter label
        row.chapter = d.chapterTitle.isEmpty()
            ? (d.chapterNumber > 0 ? QStringLiteral("Ch. %1").arg(d.chapterNumber) : QStringLiteral("Ch. %1").arg(d.chapterId))
            : (d.chapterNumber > 0 ? QStringLiteral("Ch. %1 · %2").arg(QString::number(d.chapterNumber), d.chapterTitle) : d.chapterTitle);

        // Fetch cover from library if available
        auto libItem = m_db->getLibraryItem(d.provider, d.mangaId);
        if (libItem && !libItem->coverUrl.isEmpty()) {
            row.coverUrl = libItem->coverUrl;
        }

        m_items.push_back(std::move(row));
    }

    endResetModel();
    emit countChanged();
    emit countsChanged();
    emit storageUsedChanged();
}

QString DownloadsModel::storageUsed() const
{
    QString path = AppSettings::instance()->downloadPath();
    if (path.isEmpty()) path = AppPaths::defaultDownloadDir();
    QDir dir(path);
    if (!dir.exists()) return QStringLiteral("0 B");

    qint64 totalBytes = 0;
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        totalBytes += it.fileInfo().size();
    }

    if (totalBytes < 1024) return QStringLiteral("%1 B").arg(totalBytes);
    if (totalBytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(totalBytes / 1024);
    if (totalBytes < 1024 * 1024 * 1024) return QStringLiteral("%1 MB").arg(totalBytes / (1024 * 1024));
    return QStringLiteral("%1 GB").arg(QString::number(totalBytes / (1024.0 * 1024.0 * 1024.0), 'f', 1));
}

} // namespace mangadl
