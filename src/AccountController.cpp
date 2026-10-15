#include "AccountController.h"
#include "src/core/settings/AppSettings.h"
#include "src/core/sync/SecretStorage.h"
#include <QDesktopServices>
#include <QUrl>
#include <QElapsedTimer>
#include <QNetworkRequest>
#include <QNetworkReply>

namespace mangadl {

AccountController::AccountController(QObject *parent)
    : QObject(parent)
{
    auto *settings = AppSettings::instance();
    const QString url = settings->supabaseUrl();
    const QString anonKey = settings->supabaseAnonKey();

    m_authService = std::make_unique<SupabaseAuthService>(url, anonKey, this);
    m_syncService = std::make_unique<SupabaseSyncService>(MangaDatabase::instance(), url, anonKey, this);
    m_loopback = std::make_unique<OAuthLoopbackListener>(this);

    connect(m_loopback.get(), &OAuthLoopbackListener::authCodeReceived,
            this, &AccountController::onGoogleAuthCodeReceived);
    connect(m_loopback.get(), &OAuthLoopbackListener::authErrorReceived,
            this, [this](const QString &err, const QString &) {
                setBusy(false);
                setErrorMessage(err);
            });
    connect(m_loopback.get(), &OAuthLoopbackListener::timedOut,
            this, [this]() {
                setBusy(false);
                setErrorMessage(QStringLiteral("Google sign-in timed out."));
            });

    connect(m_authService.get(), &SupabaseAuthService::signInFinished,
            this, [this](bool success, const AuthSession &session, const QString &err) {
                setBusy(false);
                if (success) {
                    handleAuthSuccess(session);
                    emit authSuccess(QStringLiteral("signIn"));
                } else {
                    setErrorMessage(err);
                }
            });

    connect(m_authService.get(), &SupabaseAuthService::signUpFinished,
            this, [this](bool success, const QString &msg, const QString &err) {
                setBusy(false);
                if (success) {
                    setErrorMessage(QString());
                    emit authSuccess(msg.isEmpty() ? QStringLiteral("signUp") : msg);
                } else {
                    setErrorMessage(err);
                }
            });

    connect(m_authService.get(), &SupabaseAuthService::passwordResetFinished,
            this, [this](bool success, const QString &err) {
                setBusy(false);
                if (success) {
                    setErrorMessage(QString());
                    emit authSuccess(QStringLiteral("passwordReset"));
                } else {
                    setErrorMessage(err);
                }
            });

    connect(m_authService.get(), &SupabaseAuthService::oauthFinished,
            this, [this](bool success, const AuthSession &session, const QString &err) {
                setBusy(false);
                if (success) {
                    handleAuthSuccess(session);
                    emit authSuccess(QStringLiteral("google"));
                } else {
                    setErrorMessage(err);
                }
            });

    connect(m_syncService.get(), &SupabaseSyncService::syncStarted, this, [this]() {
        m_isSyncing = true;
        emit syncingChanged();
    });

    connect(m_syncService.get(), &SupabaseSyncService::syncFinished, this, [this](const SyncResult &result) {
        m_isSyncing = false;
        emit syncingChanged();
        if (result.success) {
            AppSettings::instance()->setLastSyncTime(QDateTime::currentSecsSinceEpoch());
            emit lastSyncedTextChanged();
        }
    });
}

bool AccountController::signedIn() const
{
    return !AppSettings::instance()->userId().isEmpty();
}

QString AccountController::userId() const
{
    return AppSettings::instance()->userId();
}

QString AccountController::email() const
{
    return AppSettings::instance()->userEmail();
}

QString AccountController::displayName() const
{
    QString name = AppSettings::instance()->displayName();
    if (name.isEmpty()) {
        const QString mail = email();
        if (!mail.isEmpty()) {
            name = mail.section(QLatin1Char('@'), 0, 0);
        }
    }
    return name.isEmpty() ? QStringLiteral("Reader") : name;
}

void AccountController::setDisplayName(const QString &val)
{
    AppSettings::instance()->setDisplayName(val);
    emit profileChanged();
}

QString AccountController::bio() const
{
    return AppSettings::instance()->bio();
}

void AccountController::setBio(const QString &val)
{
    AppSettings::instance()->setBio(val);
    emit profileChanged();
}

QString AccountController::avatarUrl() const
{
    return AppSettings::instance()->avatarUrl();
}

QString AccountController::lastSyncedText() const
{
    const qint64 t = AppSettings::instance()->lastSyncTime();
    if (t == 0) return QStringLiteral("Never");
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(t);
    const qint64 diffSecs = dt.secsTo(QDateTime::currentDateTime());
    if (diffSecs < 60) return QStringLiteral("Just now");
    if (diffSecs < 3600) return QStringLiteral("%1m ago").arg(diffSecs / 60);
    if (diffSecs < 86400) return QStringLiteral("%1h ago").arg(diffSecs / 3600);
    return dt.toString(QStringLiteral("MMM d, yyyy"));
}

void AccountController::setBusy(bool busy)
{
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit busyChanged();
    }
}

void AccountController::setErrorMessage(const QString &msg)
{
    if (m_errorMessage != msg) {
        m_errorMessage = msg;
        emit errorMessageChanged();
    }
}

void AccountController::handleAuthSuccess(const AuthSession &session)
{
    auto *settings = AppSettings::instance();
    settings->setUserId(session.userId);
    settings->setUserEmail(session.email);

    SecretStorage::storeSecret(QStringLiteral("supabase_access_token"), session.accessToken);
    SecretStorage::storeSecret(QStringLiteral("supabase_refresh_token"), session.refreshToken);

    setErrorMessage(QString());
    emit signedInChanged();
    emit profileChanged();

    // Trigger sync
    syncNow();
}

void AccountController::signIn(const QString &email, const QString &password)
{
    setBusy(true);
    setErrorMessage(QString());
    m_authService->signInWithPassword(email.trimmed(), password);
}

void AccountController::signUp(const QString &username, const QString &email, const QString &password)
{
    setBusy(true);
    setErrorMessage(QString());
    m_authService->signUp(username.trimmed(), email.trimmed(), password);
}

void AccountController::requestPasswordReset(const QString &email)
{
    setBusy(true);
    setErrorMessage(QString());
    m_authService->requestPasswordReset(email.trimmed());
}

void AccountController::signOut()
{
    const QString token = SecretStorage::getSecret(QStringLiteral("supabase_access_token"));
    if (!token.isEmpty()) {
        m_authService->signOut(token);
    }

    auto *settings = AppSettings::instance();
    settings->setUserId(QString());
    settings->setUserEmail(QString());
    settings->setDisplayName(QString());
    settings->setBio(QString());

    SecretStorage::clearSecret(QStringLiteral("supabase_access_token"));
    SecretStorage::clearSecret(QStringLiteral("supabase_refresh_token"));

    emit signedInChanged();
    emit profileChanged();
}

void AccountController::signInWithGoogle()
{
    setBusy(true);
    setErrorMessage(QString());

    const auto pkce = SupabaseAuthService::generatePkceS256();
    m_googleCodeVerifier = pkce.first;

    const QString redirectUri = AppSettings::instance()->googleRedirectUri();
    const QString authUrl = m_authService->getOAuthAuthorizeUrl(QStringLiteral("google"), redirectUri, pkce.second);

    if (!m_loopback->start(5678, 120000)) {
        setBusy(false);
        setErrorMessage(QStringLiteral("Could not start local OAuth listener on port 5678."));
        return;
    }

    QDesktopServices::openUrl(QUrl(authUrl));
}

void AccountController::onGoogleAuthCodeReceived(const QString &code, const QString &path)
{
    Q_UNUSED(path);
    m_authService->exchangeOAuthCode(code, m_googleCodeVerifier);
}

void AccountController::syncNow()
{
    if (!signedIn()) return;
    const QString uid = userId();
    const QString token = SecretStorage::getSecret(QStringLiteral("supabase_access_token"));
    if (uid.isEmpty() || token.isEmpty()) return;

    m_syncService->syncAllAsync(uid, token);
}

void AccountController::saveProfile(const QString &displayName, const QString &bio)
{
    setDisplayName(displayName);
    setBio(bio);
}

void AccountController::testConnection(const QString &url, const QString &key)
{
    auto *timer = new QElapsedTimer();
    timer->start();

    auto *net = new QNetworkAccessManager(this);
    QString testUrl = url.trimmed();
    while (testUrl.endsWith(QLatin1Char('/'))) {
        testUrl.chop(1);
    }

    QNetworkRequest req{QUrl(testUrl)};
    if (!key.isEmpty()) {
        req.setRawHeader("apikey", key.toUtf8());
        req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    }

    QNetworkReply *reply = net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, net, timer]() {
        const qint64 latency = timer->elapsed();
        delete timer;

        const bool ok = (reply->error() == QNetworkReply::NoError || reply->error() == QNetworkReply::ContentAccessDenied);
        const QString err = reply->errorString();
        reply->deleteLater();
        net->deleteLater();

        emit connectionTestResult(ok, static_cast<int>(latency), ok ? QString() : err);
    });
}

} // namespace mangadl
