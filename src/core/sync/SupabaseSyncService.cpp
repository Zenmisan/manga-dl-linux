#include "SupabaseSyncService.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QThread>

namespace mangadl {

SupabaseSyncService::SupabaseSyncService(MangaDatabase *db,
                                         const QString &supabaseUrl,
                                         const QString &anonKey,
                                         QObject *parent)
    : QObject(parent)
    , m_db(db)
    , m_url(supabaseUrl.trimmed())
    , m_anonKey(anonKey.trimmed())
{
    while (m_url.endsWith(QLatin1Char('/'))) {
        m_url.chop(1);
    }

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(500);
    connect(&m_debounceTimer, &QTimer::timeout, this, [this]() {
        if (m_pendingProgressQueue.isEmpty()) return;
        const auto items = m_pendingProgressQueue;
        m_pendingProgressQueue.clear();
        for (const auto &item : items) {
            pushChapterProgress(item.userId, item.accessToken,
                                item.provider, item.mangaId,
                                item.chapterId, item.page,
                                item.mangaTitle, item.chapterTitle,
                                item.isCompleted);
        }
    });
}

bool SupabaseSyncService::isConfigured() const
{
    return !m_url.isEmpty() && !m_anonKey.isEmpty();
}

void SupabaseSyncService::setCredentials(const QString &url, const QString &anonKey)
{
    m_url = url.trimmed();
    while (m_url.endsWith(QLatin1Char('/'))) {
        m_url.chop(1);
    }
    m_anonKey = anonKey.trimmed();
}

QByteArray SupabaseSyncService::executeGet(const QString &path, const QString &accessToken, bool *ok, int timeoutMs)
{
    const QUrl url(m_url + path);
    QNetworkRequest req(url);
    req.setRawHeader("apikey", m_anonKey.toUtf8());
    req.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());

    QNetworkReply *reply = m_net.get(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        if (ok) *ok = false;
        return QByteArray();
    }

    const QByteArray data = reply->readAll();
    const bool success = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    if (ok) *ok = success;
    return data;
}

QByteArray SupabaseSyncService::executePostUpsert(const QString &path, const QByteArray &body, const QString &accessToken, bool *ok, int timeoutMs)
{
    const QUrl url(m_url + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("apikey", m_anonKey.toUtf8());
    req.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    req.setRawHeader("Prefer", "resolution=merge-duplicates");

    QNetworkReply *reply = m_net.post(req, body);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        if (ok) *ok = false;
        return QByteArray();
    }

    const QByteArray data = reply->readAll();
    const bool success = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    if (ok) *ok = success;
    return data;
}

bool SupabaseSyncService::executeDelete(const QString &path, const QString &accessToken, int timeoutMs)
{
    const QUrl url(m_url + path);
    QNetworkRequest req(url);
    req.setRawHeader("apikey", m_anonKey.toUtf8());
    req.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());

    QNetworkReply *reply = m_net.deleteResource(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        return false;
    }

    const bool success = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    return success;
}

int SupabaseSyncService::pullLibrary(const QString &userId, const QString &accessToken)
{
    if (!m_db || !isConfigured()) return 0;

    const QString path = QStringLiteral("/rest/v1/subscriptions?user_id=eq.%1&select=*")
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(userId)));

    bool ok = false;
    const QByteArray data = executeGet(path, accessToken, &ok);
    if (!ok) return 0;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) return 0;

    const QJsonArray arr = doc.array();
    int count = 0;
    for (const auto &val : arr) {
        if (!val.isObject()) continue;
        const auto rec = SupabaseSubscriptionRecord::fromJson(val.toObject());
        if (rec.provider.isEmpty() || rec.mangaId.isEmpty()) continue;

        auto existing = m_db->getLibraryItem(rec.provider, rec.mangaId);
        LibraryEntity entity;
        entity.id = QStringLiteral("%1/%2").arg(rec.provider, rec.mangaId);
        entity.provider = rec.provider;
        entity.mangaId = rec.mangaId;
        entity.title = rec.title;
        entity.coverUrl = rec.coverUrl;
        entity.type = rec.type.isEmpty() ? QStringLiteral("manga") : rec.type;
        entity.addedAt = existing.has_value() ? existing->addedAt : QDateTime::currentSecsSinceEpoch();
        entity.updatedAt = QDateTime::currentSecsSinceEpoch();
        entity.totalChapters = existing.has_value() ? existing->totalChapters : 0;

        m_db->addToLibrary(entity);
        count++;
    }
    return count;
}

int SupabaseSyncService::pullReadTracking(const QString &userId, const QString &accessToken)
{
    if (!m_db || !isConfigured()) return 0;

    const QString path = QStringLiteral("/rest/v1/read_tracking?user_id=eq.%1&select=*")
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(userId)));

    bool ok = false;
    const QByteArray data = executeGet(path, accessToken, &ok);
    if (!ok) return 0;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) return 0;

    const QJsonArray arr = doc.array();
    int count = 0;
    for (const auto &val : arr) {
        if (!val.isObject()) continue;
        const auto rec = SupabaseReadTrackingRecord::fromJson(val.toObject());
        if (rec.provider.isEmpty() || rec.mangaId.isEmpty()) continue;

        for (const auto &chapterId : rec.chapterIds) {
            if (chapterId.isEmpty()) continue;
            auto existing = m_db->getProgress(rec.provider, rec.mangaId, chapterId);
            const int page = existing.has_value() ? existing->page : 1;
            m_db->saveProgress(rec.provider, rec.mangaId, chapterId, 0.0, page, true);
            count++;
        }
    }
    return count;
}

int SupabaseSyncService::pullReadingProgressAndHistory(const QString &userId, const QString &accessToken)
{
    if (!m_db || !isConfigured()) return 0;

    const QString path = QStringLiteral("/rest/v1/reading_progress?user_id=eq.%1&order=updated_at.desc&select=*")
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(userId)));

    bool ok = false;
    const QByteArray data = executeGet(path, accessToken, &ok);
    if (!ok) return 0;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) return 0;

    const QJsonArray arr = doc.array();
    int count = 0;
    for (const auto &val : arr) {
        if (!val.isObject()) continue;
        const auto rec = SupabaseReadingProgressRecord::fromJson(val.toObject());
        if (rec.provider.isEmpty() || rec.mangaId.isEmpty() || rec.chapterId.isEmpty()) continue;

        // 1. Reading progress
        auto existing = m_db->getProgress(rec.provider, rec.mangaId, rec.chapterId);
        const bool completed = existing.has_value() && existing->completed == 1;
        m_db->saveProgress(rec.provider, rec.mangaId, rec.chapterId, 0.0, rec.lastPage, completed);

        // 2. Reading history
        m_db->recordHistory(rec.provider, rec.mangaId,
                            rec.mangaTitle.isEmpty() ? rec.mangaId : rec.mangaTitle,
                            rec.chapterId,
                            rec.chapterTitle.isEmpty() ? rec.chapterId : rec.chapterTitle,
                            QString());
        count++;
    }
    return count;
}

bool SupabaseSyncService::pushLibraryItem(const QString &userId, const QString &accessToken, const LibraryEntity &item, bool subscribed)
{
    if (!isConfigured()) return false;
    const QString recordId = QStringLiteral("%1/%2").arg(item.provider, item.mangaId);

    if (!subscribed) {
        const QString path = QStringLiteral("/rest/v1/subscriptions?user_id=eq.%1&id=eq.%2")
            .arg(QString::fromLatin1(QUrl::toPercentEncoding(userId)),
                 QString::fromLatin1(QUrl::toPercentEncoding(recordId)));
        return executeDelete(path, accessToken);
    }

    SupabaseSubscriptionRecord rec;
    rec.id = recordId;
    rec.userId = userId;
    rec.provider = item.provider;
    rec.mangaId = item.mangaId;
    rec.title = item.title;
    rec.coverUrl = item.coverUrl;
    rec.type = item.type;
    rec.addedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    const QByteArray body = QJsonDocument(rec.toJson()).toJson(QJsonDocument::Compact);
    bool ok = false;
    executePostUpsert(QStringLiteral("/rest/v1/subscriptions?on_conflict=user_id,id"), body, accessToken, &ok);
    return ok;
}

bool SupabaseSyncService::pushChapterProgress(const QString &userId, const QString &accessToken,
                                             const QString &provider, const QString &mangaId,
                                             const QString &chapterId, int page,
                                             const QString &mangaTitle, const QString &chapterTitle,
                                             bool isCompleted)
{
    if (!isConfigured()) return false;
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // 1. Reading progress
    SupabaseReadingProgressRecord prog;
    prog.userId = userId;
    prog.provider = provider;
    prog.mangaId = mangaId;
    prog.chapterId = chapterId;
    prog.lastPage = page;
    prog.mangaTitle = mangaTitle;
    prog.chapterTitle = chapterTitle;
    prog.updatedAt = now;

    const QByteArray progBody = QJsonDocument(prog.toJson()).toJson(QJsonDocument::Compact);
    bool ok = false;
    executePostUpsert(QStringLiteral("/rest/v1/reading_progress"), progBody, accessToken, &ok);

    // 2. Read tracking if completed
    if (isCompleted) {
        const QString fetchPath = QStringLiteral("/rest/v1/read_tracking?user_id=eq.%1&provider=eq.%2&manga_id=eq.%3&select=*")
            .arg(QString::fromLatin1(QUrl::toPercentEncoding(userId)),
                 QString::fromLatin1(QUrl::toPercentEncoding(provider)),
                 QString::fromLatin1(QUrl::toPercentEncoding(mangaId)));

        bool getOk = false;
        const QByteArray trackData = executeGet(fetchPath, accessToken, &getOk);
        QSet<QString> chapterIds;
        chapterIds.insert(chapterId);

        if (getOk) {
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(trackData, &err);
            if (err.error == QJsonParseError::NoError && doc.isArray()) {
                const QJsonArray arr = doc.array();
                if (!arr.isEmpty() && arr[0].isObject()) {
                    auto existing = SupabaseReadTrackingRecord::fromJson(arr[0].toObject());
                    for (const auto &cid : existing.chapterIds) {
                        chapterIds.insert(cid);
                    }
                }
            }
        }

        SupabaseReadTrackingRecord track;
        track.userId = userId;
        track.provider = provider;
        track.mangaId = mangaId;
        track.chapterIds = chapterIds.values();
        track.updatedAt = now;

        const QByteArray trackBody = QJsonDocument(track.toJson()).toJson(QJsonDocument::Compact);
        executePostUpsert(QStringLiteral("/rest/v1/read_tracking"), trackBody, accessToken, &ok);
    }
    return ok;
}

int SupabaseSyncService::pushAllLibrary(const QString &userId, const QString &accessToken)
{
    if (!m_db || !isConfigured()) return 0;
    const auto library = m_db->getLibrary();
    int count = 0;
    for (const auto &item : library) {
        if (pushLibraryItem(userId, accessToken, item, true)) {
            count++;
        }
    }
    return count;
}

int SupabaseSyncService::pushAllReadTracking(const QString &userId, const QString &accessToken)
{
    if (!m_db || !isConfigured()) return 0;
    const auto completedList = m_db->getAllCompletedProgress();
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // Group by provider + mangaId
    QMap<QString, QStringList> grouped;
    for (const auto &item : completedList) {
        const QString key = QStringLiteral("%1:::%2").arg(item.provider, item.mangaId);
        grouped[key].append(item.chapterId);
    }

    int count = 0;
    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        const QStringList parts = it.key().split(QStringLiteral(":::"));
        if (parts.size() != 2) continue;

        SupabaseReadTrackingRecord track;
        track.userId = userId;
        track.provider = parts[0];
        track.mangaId = parts[1];
        track.chapterIds = it.value();
        track.updatedAt = now;

        const QByteArray body = QJsonDocument(track.toJson()).toJson(QJsonDocument::Compact);
        bool ok = false;
        executePostUpsert(QStringLiteral("/rest/v1/read_tracking"), body, accessToken, &ok);
        if (ok) count++;
    }
    return count;
}

void SupabaseSyncService::queueChapterSync(const QString &userId, const QString &accessToken,
                                           const QString &provider, const QString &mangaId,
                                           const QString &chapterId, int page,
                                           const QString &mangaTitle, const QString &chapterTitle,
                                           bool isCompleted)
{
    if (userId.isEmpty() || accessToken.isEmpty()) return;
    m_pendingProgressQueue.append(PendingProgress{
        userId, accessToken, provider, mangaId, chapterId, page, mangaTitle, chapterTitle, isCompleted
    });
    m_debounceTimer.start();
}

SyncResult SupabaseSyncService::syncAll(const QString &userId, const QString &accessToken)
{
    if (!isConfigured()) {
        return SyncResult{false, 0, 0, 0, 0, QStringLiteral("Supabase is not configured.")};
    }
    if (userId.isEmpty() || accessToken.isEmpty()) {
        return SyncResult{false, 0, 0, 0, 0, QStringLiteral("User is not authenticated.")};
    }

    emit syncStarted();
    emit syncProgress(QStringLiteral("Pulling library and tracking from cloud..."));

    const int pulledLib = pullLibrary(userId, accessToken);
    const int pulledTracking = pullReadTracking(userId, accessToken);
    const int pulledProgress = pullReadingProgressAndHistory(userId, accessToken);

    emit syncProgress(QStringLiteral("Pushing local library and progress to cloud..."));
    const int pushedLib = pushAllLibrary(userId, accessToken);
    const int pushedProgress = pushAllReadTracking(userId, accessToken);

    SyncResult result;
    result.success = true;
    result.pulledLibraryCount = pulledLib;
    result.pulledProgressCount = pulledTracking + pulledProgress;
    result.pushedLibraryCount = pushedLib;
    result.pushedProgressCount = pushedProgress;

    emit syncFinished(result);
    return result;
}

void SupabaseSyncService::syncAllAsync(const QString &userId, const QString &accessToken)
{
    QThread::create([this, userId, accessToken]() {
        syncAll(userId, accessToken);
    })->start();
}

} // namespace mangadl
