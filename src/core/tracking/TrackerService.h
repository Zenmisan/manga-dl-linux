#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <optional>
#include "TrackerModels.h"
#include "db/MangaDatabase.h"

namespace mangadl {

class TrackerService : public QObject {
    Q_OBJECT

public:
    explicit TrackerService(QObject *parent = nullptr);
    ~TrackerService() override = default;

    static const QString AnilistGraphqlUrl;
    static const QString AnilistTokenUrl;
    static const QString MalApiUrl;
    static const QString MalTokenUrl;

    // AniList
    static QString getAnilistAuthUrl(const QString &clientId, const QString &redirectUri);
    std::optional<TrackerTokenResult> exchangeAnilistToken(const QString &code,
                                                          const QString &clientId,
                                                          const QString &clientSecret,
                                                          const QString &redirectUri);
    QString getAnilistViewer(const QString &token);
    QList<TrackerSearchResult> searchAnilist(const QString &query, const QString &token = QString());
    bool updateAnilistProgress(const QString &token, int mediaId, int progress, bool isCompleted);

    // MyAnimeList (MAL)
    static QPair<QString, QString> generateMalPkce();
    static QString getMalAuthUrl(const QString &clientId, const QString &codeVerifier, const QString &redirectUri);
    std::optional<TrackerTokenResult> exchangeMalToken(const QString &code,
                                                      const QString &codeVerifier,
                                                      const QString &clientId,
                                                      const QString &redirectUri);
    QString getMalUser(const QString &token);
    QList<TrackerSearchResult> searchMal(const QString &query, const QString &token);
    bool updateMalProgress(const QString &token, int malId, int progress, bool isCompleted);

    // High-Level Synchronization
    void syncChapterProgress(MangaDatabase *db,
                             const QString &provider,
                             const QString &mangaId,
                             const QString &mangaTitle,
                             double chapterNumber,
                             bool isCompleted,
                             const QString &anilistToken,
                             const QString &malToken,
                             bool autoSyncEnabled = true);

signals:
    void trackerProgressUpdated(const QString &tracker, const QString &mangaTitle, double chapterNumber);

private:
    QByteArray executePost(const QString &url, const QByteArray &body, const QMap<QString, QString> &headers, int timeoutMs = 15000);
    QByteArray executeGet(const QString &url, const QMap<QString, QString> &headers, int timeoutMs = 15000);
    QByteArray executePut(const QString &url, const QByteArray &body, const QMap<QString, QString> &headers, int timeoutMs = 15000);

    QNetworkAccessManager m_net;
};

} // namespace mangadl
