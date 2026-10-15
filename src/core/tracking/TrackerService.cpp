#include "TrackerService.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QRandomGenerator>
#include <cmath>
#include "core/settings/AppSettings.h"

namespace mangadl {

const QString TrackerService::AnilistGraphqlUrl = QStringLiteral("https://graphql.anilist.co");
const QString TrackerService::AnilistTokenUrl = QStringLiteral("https://anilist.co/api/v2/oauth/token");
const QString TrackerService::MalApiUrl = QStringLiteral("https://api.myanimelist.net/v2");
const QString TrackerService::MalTokenUrl = QStringLiteral("https://myanimelist.net/v1/oauth2/token");

TrackerService::TrackerService(QObject *parent)
    : QObject(parent)
{
}

QByteArray TrackerService::executePost(const QString &url, const QByteArray &body, const QMap<QString, QString> &headers, int timeoutMs)
{
    QNetworkRequest req{QUrl(url)};
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        req.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

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
        return QByteArray();
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

QByteArray TrackerService::executeGet(const QString &url, const QMap<QString, QString> &headers, int timeoutMs)
{
    QNetworkRequest req{QUrl(url)};
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        req.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

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
        return QByteArray();
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

QByteArray TrackerService::executePut(const QString &url, const QByteArray &body, const QMap<QString, QString> &headers, int timeoutMs)
{
    QNetworkRequest req{QUrl(url)};
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        req.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply *reply = m_net.put(req, body);

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
        return QByteArray();
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

// =========================================================================
// AniList
// =========================================================================

QString TrackerService::getAnilistAuthUrl(const QString &clientId, const QString &redirectUri)
{
    return QStringLiteral("https://anilist.co/api/v2/oauth/authorize?client_id=%1&response_type=code&redirect_uri=%2")
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(clientId)),
             QString::fromLatin1(QUrl::toPercentEncoding(redirectUri)));
}

std::optional<TrackerTokenResult> TrackerService::exchangeAnilistToken(const QString &code,
                                                                      const QString &clientId,
                                                                      const QString &clientSecret,
                                                                      const QString &redirectUri)
{
    QJsonObject json;
    json[QStringLiteral("grant_type")] = QStringLiteral("authorization_code");
    bool okInt = false;
    int intId = clientId.toInt(&okInt);
    if (okInt) {
        json[QStringLiteral("client_id")] = intId;
    } else {
        json[QStringLiteral("client_id")] = clientId;
    }
    json[QStringLiteral("client_secret")] = clientSecret;
    json[QStringLiteral("redirect_uri")] = redirectUri;
    json[QStringLiteral("code")] = code;

    QMap<QString, QString> headers;
    headers[QStringLiteral("Content-Type")] = QStringLiteral("application/json");
    headers[QStringLiteral("Accept")] = QStringLiteral("application/json");

    const QByteArray res = executePost(AnilistTokenUrl, QJsonDocument(json).toJson(QJsonDocument::Compact), headers);
    if (res.isEmpty()) return std::nullopt;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(res, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return std::nullopt;

    const QJsonObject root = doc.object();
    const QString token = root.value(QStringLiteral("access_token")).toString();
    if (token.isEmpty()) return std::nullopt;

    TrackerTokenResult result;
    result.accessToken = token;
    result.refreshToken = root.value(QStringLiteral("refresh_token")).toString();
    result.expiresIn = root.value(QStringLiteral("expires_in")).toInt(0);
    result.username = getAnilistViewer(token);

    return result;
}

QString TrackerService::getAnilistViewer(const QString &token)
{
    QJsonObject json;
    json[QStringLiteral("query")] = QStringLiteral("query { Viewer { id name } }");

    QMap<QString, QString> headers;
    headers[QStringLiteral("Authorization")] = QStringLiteral("Bearer ") + token;
    headers[QStringLiteral("Content-Type")] = QStringLiteral("application/json");
    headers[QStringLiteral("Accept")] = QStringLiteral("application/json");

    const QByteArray res = executePost(AnilistGraphqlUrl, QJsonDocument(json).toJson(QJsonDocument::Compact), headers);
    if (res.isEmpty()) return QString();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(res, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return QString();

    const QJsonObject data = doc.object().value(QStringLiteral("data")).toObject();
    const QJsonObject viewer = data.value(QStringLiteral("Viewer")).toObject();
    return viewer.value(QStringLiteral("name")).toString();
}

QList<TrackerSearchResult> TrackerService::searchAnilist(const QString &query, const QString &token)
{
    const QString gql = QStringLiteral(R"(
        query Search($search: String) {
          Page(perPage: 10) {
            media(search: $search, type: MANGA) {
              id
              title {
                romaji
                english
                userPreferred
              }
              chapters
              coverImage {
                medium
                large
              }
            }
          }
        }
    )");

    QJsonObject variables;
    variables[QStringLiteral("search")] = query;

    QJsonObject json;
    json[QStringLiteral("query")] = gql;
    json[QStringLiteral("variables")] = variables;

    QMap<QString, QString> headers;
    headers[QStringLiteral("Content-Type")] = QStringLiteral("application/json");
    headers[QStringLiteral("Accept")] = QStringLiteral("application/json");
    if (!token.isEmpty()) {
        headers[QStringLiteral("Authorization")] = QStringLiteral("Bearer ") + token;
    }

    const QByteArray res = executePost(AnilistGraphqlUrl, QJsonDocument(json).toJson(QJsonDocument::Compact), headers);
    QList<TrackerSearchResult> results;
    if (res.isEmpty()) return results;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(res, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return results;

    const QJsonObject data = doc.object().value(QStringLiteral("data")).toObject();
    const QJsonObject page = data.value(QStringLiteral("Page")).toObject();
    const QJsonArray media = page.value(QStringLiteral("media")).toArray();

    for (const auto &itemVal : media) {
        if (!itemVal.isObject()) continue;
        const QJsonObject item = itemVal.toObject();
        TrackerSearchResult r;
        r.id = item.value(QStringLiteral("id")).toInt();
        r.tracker = QStringLiteral("AniList");
        r.totalChapters = item.value(QStringLiteral("chapters")).toInt(0);

        const QJsonObject titleObj = item.value(QStringLiteral("title")).toObject();
        if (titleObj.contains(QStringLiteral("english")) && !titleObj.value(QStringLiteral("english")).toString().isEmpty()) {
            r.title = titleObj.value(QStringLiteral("english")).toString();
        } else if (titleObj.contains(QStringLiteral("userPreferred")) && !titleObj.value(QStringLiteral("userPreferred")).toString().isEmpty()) {
            r.title = titleObj.value(QStringLiteral("userPreferred")).toString();
        } else {
            r.title = titleObj.value(QStringLiteral("romaji")).toString(QStringLiteral("Unknown"));
        }

        const QJsonObject coverObj = item.value(QStringLiteral("coverImage")).toObject();
        r.coverUrl = coverObj.value(QStringLiteral("medium")).toString();

        results.append(r);
    }

    return results;
}

bool TrackerService::updateAnilistProgress(const QString &token, int mediaId, int progress, bool isCompleted)
{
    const QString gql = QStringLiteral(R"(
        mutation SaveMediaList($mediaId: Int, $progress: Int, $status: MediaListStatus) {
          SaveMediaListEntry(mediaId: $mediaId, progress: $progress, status: $status) {
            id
            progress
            status
          }
        }
    )");

    QJsonObject variables;
    variables[QStringLiteral("mediaId")] = mediaId;
    variables[QStringLiteral("progress")] = progress;
    variables[QStringLiteral("status")] = isCompleted ? QStringLiteral("COMPLETED") : QStringLiteral("CURRENT");

    QJsonObject json;
    json[QStringLiteral("query")] = gql;
    json[QStringLiteral("variables")] = variables;

    QMap<QString, QString> headers;
    headers[QStringLiteral("Authorization")] = QStringLiteral("Bearer ") + token;
    headers[QStringLiteral("Content-Type")] = QStringLiteral("application/json");
    headers[QStringLiteral("Accept")] = QStringLiteral("application/json");

    const QByteArray res = executePost(AnilistGraphqlUrl, QJsonDocument(json).toJson(QJsonDocument::Compact), headers);
    return !res.isEmpty();
}

// =========================================================================
// MyAnimeList (MAL)
// =========================================================================

QPair<QString, QString> TrackerService::generateMalPkce()
{
    const QString chars = QStringLiteral("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~");
    QString verifier;
    verifier.reserve(128);
    for (int i = 0; i < 128; ++i) {
        const int idx = QRandomGenerator::global()->bounded(chars.length());
        verifier.append(chars.at(idx));
    }
    return qMakePair(verifier, verifier);
}

QString TrackerService::getMalAuthUrl(const QString &clientId, const QString &codeVerifier, const QString &redirectUri)
{
    return QStringLiteral("https://myanimelist.net/v1/oauth2/authorize?response_type=code&client_id=%1&code_challenge=%2&code_challenge_method=plain&redirect_uri=%3")
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(clientId)),
             QString::fromLatin1(QUrl::toPercentEncoding(codeVerifier)),
             QString::fromLatin1(QUrl::toPercentEncoding(redirectUri)));
}

std::optional<TrackerTokenResult> TrackerService::exchangeMalToken(const QString &code,
                                                                  const QString &codeVerifier,
                                                                  const QString &clientId,
                                                                  const QString &redirectUri)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), clientId);
    query.addQueryItem(QStringLiteral("code"), code);
    query.addQueryItem(QStringLiteral("code_verifier"), codeVerifier);
    query.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    query.addQueryItem(QStringLiteral("redirect_uri"), redirectUri);

    const QByteArray formData = query.toString(QUrl::FullyEncoded).toUtf8();

    QMap<QString, QString> headers;
    headers[QStringLiteral("Content-Type")] = QStringLiteral("application/x-www-form-urlencoded");

    const QByteArray res = executePost(MalTokenUrl, formData, headers);
    if (res.isEmpty()) return std::nullopt;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(res, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return std::nullopt;

    const QJsonObject root = doc.object();
    const QString token = root.value(QStringLiteral("access_token")).toString();
    if (token.isEmpty()) return std::nullopt;

    TrackerTokenResult result;
    result.accessToken = token;
    result.refreshToken = root.value(QStringLiteral("refresh_token")).toString();
    result.expiresIn = root.value(QStringLiteral("expires_in")).toInt(0);
    result.username = getMalUser(token);

    return result;
}

QString TrackerService::getMalUser(const QString &token)
{
    QMap<QString, QString> headers;
    headers[QStringLiteral("Authorization")] = QStringLiteral("Bearer ") + token;

    const QByteArray res = executeGet(MalApiUrl + QStringLiteral("/users/@me"), headers);
    if (res.isEmpty()) return QString();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(res, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return QString();

    return doc.object().value(QStringLiteral("name")).toString();
}

QList<TrackerSearchResult> TrackerService::searchMal(const QString &query, const QString &token)
{
    const QString escaped = QString::fromLatin1(QUrl::toPercentEncoding(query.left(64)));
    const QString url = QStringLiteral("%1/manga?q=%2&nsfw=true&fields=id,title,num_chapters,main_picture")
        .arg(MalApiUrl, escaped);

    QMap<QString, QString> headers;
    headers[QStringLiteral("Authorization")] = QStringLiteral("Bearer ") + token;

    const QByteArray res = executeGet(url, headers);
    QList<TrackerSearchResult> results;
    if (res.isEmpty()) return results;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(res, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return results;

    const QJsonArray data = doc.object().value(QStringLiteral("data")).toArray();
    for (const auto &itemVal : data) {
        if (!itemVal.isObject()) continue;
        const QJsonObject node = itemVal.toObject().value(QStringLiteral("node")).toObject();

        TrackerSearchResult r;
        r.id = node.value(QStringLiteral("id")).toInt();
        r.title = node.value(QStringLiteral("title")).toString(QStringLiteral("Unknown"));
        r.totalChapters = node.value(QStringLiteral("num_chapters")).toInt(0);
        r.tracker = QStringLiteral("MyAnimeList");

        const QJsonObject pic = node.value(QStringLiteral("main_picture")).toObject();
        r.coverUrl = pic.value(QStringLiteral("medium")).toString();

        results.append(r);
    }

    return results;
}

bool TrackerService::updateMalProgress(const QString &token, int malId, int progress, bool isCompleted)
{
    const QString status = isCompleted ? QStringLiteral("completed") : QStringLiteral("reading");
    const QString url = QStringLiteral("%1/manga/%2/my_list_status").arg(MalApiUrl).arg(malId);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("status"), status);
    query.addQueryItem(QStringLiteral("num_chapters_read"), QString::number(progress));
    const QByteArray formData = query.toString(QUrl::FullyEncoded).toUtf8();

    QMap<QString, QString> headers;
    headers[QStringLiteral("Authorization")] = QStringLiteral("Bearer ") + token;
    headers[QStringLiteral("Content-Type")] = QStringLiteral("application/x-www-form-urlencoded");

    const QByteArray res = executePut(url, formData, headers);
    return !res.isEmpty();
}

// =========================================================================
// High-Level Synchronization
// =========================================================================

void TrackerService::syncChapterProgress(MangaDatabase *db,
                                        const QString &provider,
                                        const QString &mangaId,
                                        const QString &mangaTitle,
                                        double chapterNumber,
                                        bool isCompleted,
                                        const QString &anilistToken,
                                        const QString &malToken,
                                        bool autoSyncEnabled)
{
    if (!autoSyncEnabled || !db) return;
    auto *settings = AppSettings::instance();
    if (settings && settings->incognito()) return;

    const auto binds = db->getTrackerBinds(provider, mangaId);

    // 1. AniList
    if (!anilistToken.isEmpty()) {
        std::optional<TrackerBindEntity> bind;
        for (const auto &b : binds) {
            if (b.tracker.compare(QStringLiteral("AniList"), Qt::CaseInsensitive) == 0) {
                bind = b;
                break;
            }
        }

        if (!bind.has_value() && !mangaTitle.isEmpty()) {
            const auto results = searchAnilist(mangaTitle, anilistToken);
            if (!results.isEmpty()) {
                TrackerBindEntity newBind;
                newBind.provider = provider;
                newBind.mangaId = mangaId;
                newBind.tracker = QStringLiteral("AniList");
                newBind.remoteId = results[0].id;
                newBind.remoteTitle = results[0].title;
                newBind.lastChapterRead = 0;
                db->saveTrackerBind(newBind);
                bind = newBind;
            }
        }

        if (bind.has_value() && (chapterNumber > bind->lastChapterRead || isCompleted)) {
            const int progress = static_cast<int>(std::floor(chapterNumber));
            if (updateAnilistProgress(anilistToken, bind->remoteId, progress, isCompleted)) {
                bind->lastChapterRead = std::max(bind->lastChapterRead, chapterNumber);
                db->saveTrackerBind(*bind);
                emit trackerProgressUpdated(QStringLiteral("AniList"), mangaTitle, chapterNumber);
            }
        }
    }

    // 2. MyAnimeList
    if (!malToken.isEmpty()) {
        std::optional<TrackerBindEntity> bind;
        for (const auto &b : binds) {
            if (b.tracker.compare(QStringLiteral("MyAnimeList"), Qt::CaseInsensitive) == 0) {
                bind = b;
                break;
            }
        }

        if (!bind.has_value() && !mangaTitle.isEmpty()) {
            const auto results = searchMal(mangaTitle, malToken);
            if (!results.isEmpty()) {
                TrackerBindEntity newBind;
                newBind.provider = provider;
                newBind.mangaId = mangaId;
                newBind.tracker = QStringLiteral("MyAnimeList");
                newBind.remoteId = results[0].id;
                newBind.remoteTitle = results[0].title;
                newBind.lastChapterRead = 0;
                db->saveTrackerBind(newBind);
                bind = newBind;
            }
        }

        if (bind.has_value() && (chapterNumber > bind->lastChapterRead || isCompleted)) {
            const int progress = static_cast<int>(std::floor(chapterNumber));
            if (updateMalProgress(malToken, bind->remoteId, progress, isCompleted)) {
                bind->lastChapterRead = std::max(bind->lastChapterRead, chapterNumber);
                db->saveTrackerBind(*bind);
                emit trackerProgressUpdated(QStringLiteral("MyAnimeList"), mangaTitle, chapterNumber);
            }
        }
    }
}

} // namespace mangadl
