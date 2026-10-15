#include "NotificationsController.h"
#include "core/db/MangaDatabase.h"
#include "core/downloads/DownloadManager.h"
#include "core/settings/AppSettings.h"
#include "core/util/AppLog.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <algorithm>

namespace mangadl {

NotificationsController &NotificationsController::instance()
{
    static NotificationsController s_instance;
    return s_instance;
}

NotificationsController *NotificationsController::create(QQmlEngine *, QJSEngine *)
{
    return &instance();
}

NotificationsController::NotificationsController(MangaDatabase *db, QObject *parent)
    : QObject(parent)
    , m_db(db ? db : MangaDatabase::instance())
{
    if (m_db) {
        connect(m_db, &MangaDatabase::newChaptersChanged, this, &NotificationsController::refresh);
        connect(m_db, &MangaDatabase::downloadsChanged, this, &NotificationsController::refresh);
    }
    refresh();
}

QString NotificationsController::formatRelativeTime(qint64 epochMs)
{
    if (epochMs <= 0) return QStringLiteral("Just now");

    const qint64 diffMs = QDateTime::currentMSecsSinceEpoch() - epochMs;
    if (diffMs < 60 * 1000) {
        return QStringLiteral("Just now");
    }
    if (diffMs < 60 * 60 * 1000) {
        const int mins = static_cast<int>(diffMs / (60 * 1000));
        return QStringLiteral("%1m ago").arg(mins);
    }
    if (diffMs < 24 * 60 * 60 * 1000) {
        const int hours = static_cast<int>(diffMs / (60 * 60 * 1000));
        return QStringLiteral("%1h ago").arg(hours);
    }
    if (diffMs < 48 * 60 * 60 * 1000) {
        return QStringLiteral("Yesterday");
    }

    return QDateTime::fromMSecsSinceEpoch(epochMs).toLocalTime().toString(QStringLiteral("MMM d"));
}

void NotificationsController::setSelectedTab(const QString &tab)
{
    if (m_selectedTab != tab) {
        m_selectedTab = tab;
        emit selectedTabChanged();
        emit noticesChanged();
    }
}

int NotificationsController::unreadCount() const
{
    int count = 0;
    for (const auto &item : m_allNotices) {
        if (item.unread) {
            count++;
        }
    }
    return count;
}

QStringList NotificationsController::tabs() const
{
    const int unread = unreadCount();
    return {
        QStringLiteral("All"),
        unread > 0 ? QStringLiteral("Unread · %1").arg(unread) : QStringLiteral("Unread"),
        QStringLiteral("Chapters"),
        QStringLiteral("Downloads"),
        QStringLiteral("App")
    };
}

void NotificationsController::markAllRead()
{
    for (const auto &item : m_allNotices) {
        m_readNoticeIds.insert(item.id);
    }
    for (auto &item : m_allNotices) {
        item.unread = false;
    }
    emit noticesChanged();
    emit unreadCountChanged();
    emit tabsChanged();
}

void NotificationsController::handleAction(const QString &action, const QVariantMap &data)
{
    if (action == QStringLiteral("Open Folder")) {
        auto *settings = AppSettings::instance();
        if (settings) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(settings->downloadPath()));
        }
    } else if (action == QStringLiteral("Retry")) {
        const QString dlId = data.value(QStringLiteral("id")).toString();
        DownloadManager::instance().retryDownload(dlId);
    }
}

void NotificationsController::refresh()
{
    m_allNotices.clear();
    if (!m_db) {
        emit noticesChanged();
        emit unreadCountChanged();
        emit tabsChanged();
        return;
    }

    // 1. New chapters notices
    const auto chapters = m_db->getNewChapters();
    for (const auto &c : chapters) {
        NoticeItem item;
        item.id = QStringLiteral("ch_%1").arg(c.id);
        item.title = QStringLiteral("New Chapter: %1").arg(c.mangaTitle);
        item.body = c.chapterTitle.isEmpty()
            ? QStringLiteral("Chapter %1 available on %2").arg(QString::number(c.chapterNumber), c.provider)
            : QStringLiteral("%1 · %2").arg(c.chapterTitle, c.provider);
        item.kind = QStringLiteral("info");
        item.icon = QStringLiteral("BookOpen");
        item.category = QStringLiteral("Chapters");
        item.action = QStringLiteral("View");
        item.timestamp = c.detectedAt;
        item.unread = !m_readNoticeIds.contains(item.id);
        item.when = formatRelativeTime(item.timestamp);
        m_allNotices.append(item);
    }

    // 2. Download notices
    const auto downloads = m_db->getDownloads();
    for (const auto &d : downloads) {
        if (d.status == QStringLiteral("completed")) {
            NoticeItem item;
            item.id = QStringLiteral("dl_comp_%1").arg(d.id);
            item.title = QStringLiteral("Download Complete");
            item.body = QStringLiteral("%1 · %2").arg(d.mangaTitle, d.chapterTitle);
            item.kind = QStringLiteral("success");
            item.icon = QStringLiteral("Download");
            item.category = QStringLiteral("Downloads");
            item.action = QStringLiteral("Open Folder");
            item.timestamp = d.completedAt > 0 ? d.completedAt : d.queuedAt;
            item.unread = !m_readNoticeIds.contains(item.id);
            item.when = formatRelativeTime(item.timestamp);
            m_allNotices.append(item);
        } else if (d.status == QStringLiteral("failed")) {
            NoticeItem item;
            item.id = QStringLiteral("dl_err_%1").arg(d.id);
            item.title = QStringLiteral("Download Failed");
            item.body = QStringLiteral("%1 · %2").arg(d.mangaTitle, d.chapterTitle);
            item.kind = QStringLiteral("error");
            item.icon = QStringLiteral("AlertCircle");
            item.category = QStringLiteral("Downloads");
            item.action = QStringLiteral("Retry");
            item.timestamp = d.queuedAt;
            item.unread = !m_readNoticeIds.contains(item.id);
            item.when = formatRelativeTime(item.timestamp);
            m_allNotices.append(item);
        }
    }

    // 3. App / System notice
    {
        NoticeItem item;
        item.id = QStringLiteral("app_welcome");
        item.title = QStringLiteral("manga-dl Linux is ready");
        item.body = QStringLiteral("Your library, downloads, and reading progress are stored locally and synced.");
        item.kind = QStringLiteral("info");
        item.icon = QStringLiteral("BookOpen");
        item.category = QStringLiteral("App");
        item.action = QString();
        item.timestamp = QDateTime::currentMSecsSinceEpoch() - 3600000;
        item.unread = !m_readNoticeIds.contains(item.id);
        item.when = formatRelativeTime(item.timestamp);
        m_allNotices.append(item);
    }

    // Sort descending by timestamp
    std::sort(m_allNotices.begin(), m_allNotices.end(), [](const NoticeItem &a, const NoticeItem &b) {
        return a.timestamp > b.timestamp;
    });

    emit noticesChanged();
    emit unreadCountChanged();
    emit tabsChanged();
}

QVariantList NotificationsController::notices() const
{
    QVariantList result;
    const bool isUnreadTab = m_selectedTab.startsWith(QStringLiteral("Unread"), Qt::CaseInsensitive);

    for (const auto &item : m_allNotices) {
        if (isUnreadTab) {
            if (!item.unread) continue;
        } else if (m_selectedTab == QStringLiteral("Chapters")) {
            if (item.category != QStringLiteral("Chapters")) continue;
        } else if (m_selectedTab == QStringLiteral("Downloads")) {
            if (item.category != QStringLiteral("Downloads")) continue;
        } else if (m_selectedTab == QStringLiteral("App")) {
            if (item.category != QStringLiteral("App")) continue;
        }

        result.append(QVariantMap{
            {QStringLiteral("id"), item.id},
            {QStringLiteral("title"), item.title},
            {QStringLiteral("body"), item.body},
            {QStringLiteral("kind"), item.kind},
            {QStringLiteral("icon"), item.icon},
            {QStringLiteral("when"), item.when},
            {QStringLiteral("action"), item.action},
            {QStringLiteral("category"), item.category},
            {QStringLiteral("unread"), item.unread}
        });
    }

    return result;
}

} // namespace mangadl
