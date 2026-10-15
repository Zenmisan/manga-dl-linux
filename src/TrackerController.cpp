#include "TrackerController.h"
#include "src/core/settings/AppSettings.h"
#include "src/core/sync/SecretStorage.h"
#include <QDesktopServices>
#include <QUrl>
#include <QThread>

namespace mangadl {

TrackerController::TrackerController(QObject *parent)
    : QObject(parent)
{
    m_trackerService = std::make_unique<TrackerService>(this);
    m_loopback = std::make_unique<OAuthLoopbackListener>(this);

    connect(m_loopback.get(), &OAuthLoopbackListener::authCodeReceived,
            this, &TrackerController::onAuthCodeReceived);
    connect(m_loopback.get(), &OAuthLoopbackListener::authErrorReceived,
            this, [this](const QString &err, const QString &) {
                setConnecting(false);
                setErrorMessage(err);
            });
    connect(m_loopback.get(), &OAuthLoopbackListener::timedOut,
            this, [this]() {
                setConnecting(false);
                setErrorMessage(QStringLiteral("OAuth authentication timed out."));
            });
}

QVariantList TrackerController::trackers() const
{
    auto *settings = AppSettings::instance();
    QVariantList list;

    // 1. AniList
    QVariantMap anilist;
    anilist[QStringLiteral("id")] = QStringLiteral("anilist");
    anilist[QStringLiteral("name")] = QStringLiteral("AniList");
    anilist[QStringLiteral("short")] = QStringLiteral("AL");
    anilist[QStringLiteral("color")] = QStringLiteral("#02A9FF");
    anilist[QStringLiteral("description")] = QStringLiteral("Track manga progress and sync scores with AniList.");
    anilist[QStringLiteral("connected")] = settings->anilistConnected();
    anilist[QStringLiteral("username")] = settings->anilistUsername();
    list.append(anilist);

    // 2. MyAnimeList
    QVariantMap mal;
    mal[QStringLiteral("id")] = QStringLiteral("mal");
    mal[QStringLiteral("name")] = QStringLiteral("MyAnimeList");
    mal[QStringLiteral("short")] = QStringLiteral("MAL");
    mal[QStringLiteral("color")] = QStringLiteral("#2E51A2");
    mal[QStringLiteral("description")] = QStringLiteral("Sync your reading list and chapters read with MyAnimeList.");
    mal[QStringLiteral("connected")] = settings->malConnected();
    mal[QStringLiteral("username")] = settings->malUsername();
    list.append(mal);

    return list;
}

void TrackerController::setConnecting(bool connecting)
{
    if (m_isConnecting != connecting) {
        m_isConnecting = connecting;
        emit connectingChanged();
    }
}

void TrackerController::setErrorMessage(const QString &msg)
{
    if (m_errorMessage != msg) {
        m_errorMessage = msg;
        emit errorMessageChanged();
    }
}

void TrackerController::connectTracker(const QString &trackerName)
{
    setConnecting(true);
    setErrorMessage(QString());
    m_activeConnectingTracker = trackerName;

    if (!m_loopback->start(5678, 120000)) {
        setConnecting(false);
        setErrorMessage(QStringLiteral("Could not start local OAuth listener on port 5678."));
        return;
    }

    auto *settings = AppSettings::instance();
    if (trackerName.compare(QStringLiteral("AniList"), Qt::CaseInsensitive) == 0 ||
        trackerName.compare(QStringLiteral("anilist"), Qt::CaseInsensitive) == 0) {
        const QString url = TrackerService::getAnilistAuthUrl(settings->anilistClientId(), settings->anilistRedirectUri());
        QDesktopServices::openUrl(QUrl(url));
    } else if (trackerName.compare(QStringLiteral("MyAnimeList"), Qt::CaseInsensitive) == 0 ||
               trackerName.compare(QStringLiteral("mal"), Qt::CaseInsensitive) == 0) {
        const auto pkce = TrackerService::generateMalPkce();
        m_malCodeVerifier = pkce.first;
        const QString url = TrackerService::getMalAuthUrl(settings->malClientId(), pkce.second, settings->malRedirectUri());
        QDesktopServices::openUrl(QUrl(url));
    } else {
        m_loopback->stop();
        setConnecting(false);
        setErrorMessage(QStringLiteral("Unsupported tracker: ") + trackerName);
    }
}

void TrackerController::disconnectTracker(const QString &trackerName)
{
    auto *settings = AppSettings::instance();
    if (trackerName.compare(QStringLiteral("AniList"), Qt::CaseInsensitive) == 0 ||
        trackerName.compare(QStringLiteral("anilist"), Qt::CaseInsensitive) == 0) {
        settings->setAnilistConnected(false);
        settings->setAnilistUsername(QString());
        SecretStorage::clearSecret(QStringLiteral("anilist_token"));
        SecretStorage::clearSecret(QStringLiteral("anilist_refresh_token"));
    } else if (trackerName.compare(QStringLiteral("MyAnimeList"), Qt::CaseInsensitive) == 0 ||
               trackerName.compare(QStringLiteral("mal"), Qt::CaseInsensitive) == 0) {
        settings->setMalConnected(false);
        settings->setMalUsername(QString());
        SecretStorage::clearSecret(QStringLiteral("mal_token"));
        SecretStorage::clearSecret(QStringLiteral("mal_refresh_token"));
    }

    emit trackersChanged();
}

void TrackerController::onAuthCodeReceived(const QString &code, const QString &path)
{
    Q_UNUSED(path);
    auto *settings = AppSettings::instance();
    const QString tracker = m_activeConnectingTracker;

    QThread::create([this, code, tracker, settings]() {
        if (tracker.compare(QStringLiteral("AniList"), Qt::CaseInsensitive) == 0 ||
            tracker.compare(QStringLiteral("anilist"), Qt::CaseInsensitive) == 0) {
            auto res = m_trackerService->exchangeAnilistToken(
                code, settings->anilistClientId(), settings->anilistClientSecret(), settings->anilistRedirectUri());
            if (res.has_value() && res->isValid()) {
                settings->setAnilistConnected(true);
                settings->setAnilistUsername(res->username);
                SecretStorage::storeSecret(QStringLiteral("anilist_token"), res->accessToken);
                if (!res->refreshToken.isEmpty()) {
                    SecretStorage::storeSecret(QStringLiteral("anilist_refresh_token"), res->refreshToken);
                }
                QMetaObject::invokeMethod(this, [this, res]() {
                    setConnecting(false);
                    emit trackersChanged();
                    emit trackerConnected(QStringLiteral("AniList"), res->username);
                });
                return;
            }
        } else if (tracker.compare(QStringLiteral("MyAnimeList"), Qt::CaseInsensitive) == 0 ||
                   tracker.compare(QStringLiteral("mal"), Qt::CaseInsensitive) == 0) {
            auto res = m_trackerService->exchangeMalToken(
                code, m_malCodeVerifier, settings->malClientId(), settings->malRedirectUri());
            if (res.has_value() && res->isValid()) {
                settings->setMalConnected(true);
                settings->setMalUsername(res->username);
                SecretStorage::storeSecret(QStringLiteral("mal_token"), res->accessToken);
                if (!res->refreshToken.isEmpty()) {
                    SecretStorage::storeSecret(QStringLiteral("mal_refresh_token"), res->refreshToken);
                }
                QMetaObject::invokeMethod(this, [this, res]() {
                    setConnecting(false);
                    emit trackersChanged();
                    emit trackerConnected(QStringLiteral("MyAnimeList"), res->username);
                });
                return;
            }
        }

        QMetaObject::invokeMethod(this, [this]() {
            setConnecting(false);
            setErrorMessage(QStringLiteral("Failed to exchange authentication code with tracker."));
        });
    })->start();
}

} // namespace mangadl
