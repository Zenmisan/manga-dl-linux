#include "SupabaseAuthService.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>

namespace mangadl {

static QString base64UrlEncode(const QByteArray &data)
{
    QString encoded = QString::fromLatin1(data.toBase64());
    encoded.replace(QLatin1Char('+'), QLatin1Char('-'));
    encoded.replace(QLatin1Char('/'), QLatin1Char('_'));
    while (encoded.endsWith(QLatin1Char('='))) {
        encoded.chop(1);
    }
    return encoded;
}

SupabaseAuthService::SupabaseAuthService(const QString &supabaseUrl, const QString &anonKey, QObject *parent)
    : QObject(parent)
    , m_url(supabaseUrl.trimmed())
    , m_anonKey(anonKey.trimmed())
{
    while (m_url.endsWith(QLatin1Char('/'))) {
        m_url.chop(1);
    }
}

void SupabaseAuthService::setCredentials(const QString &url, const QString &anonKey)
{
    m_url = url.trimmed();
    while (m_url.endsWith(QLatin1Char('/'))) {
        m_url.chop(1);
    }
    m_anonKey = anonKey.trimmed();
}

QPair<QString, QString> SupabaseAuthService::generatePkceS256()
{
    QByteArray bytes(64, Qt::Uninitialized);
    QRandomGenerator::global()->fillRange(reinterpret_cast<quint32 *>(bytes.data()), bytes.size() / sizeof(quint32));

    const QString verifier = base64UrlEncode(bytes);
    const QByteArray hash = QCryptographicHash::hash(verifier.toLatin1(), QCryptographicHash::Sha256);
    const QString challenge = base64UrlEncode(hash);

    return qMakePair(verifier, challenge);
}

QString SupabaseAuthService::getOAuthAuthorizeUrl(const QString &provider, const QString &redirectUri, const QString &codeChallenge) const
{
    return QStringLiteral("%1/auth/v1/authorize?provider=%2&redirect_to=%3&code_challenge=%4&code_challenge_method=S256")
        .arg(m_url,
             QString::fromLatin1(QUrl::toPercentEncoding(provider)),
             QString::fromLatin1(QUrl::toPercentEncoding(redirectUri)),
             QString::fromLatin1(QUrl::toPercentEncoding(codeChallenge)));
}

QNetworkRequest SupabaseAuthService::createRequest(const QString &path, const QString &accessToken) const
{
    const QUrl url(m_url + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("apikey", m_anonKey.toUtf8());

    const QString token = accessToken.isEmpty() ? m_anonKey : accessToken;
    req.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    return req;
}

void SupabaseAuthService::signInWithPassword(const QString &email, const QString &password)
{
    QJsonObject json;
    json[QStringLiteral("email")] = email;
    json[QStringLiteral("password")] = password;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/token?grant_type=password"));
    QNetworkReply *reply = m_net.post(req, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            auto session = parseSession(responseData);
            if (session.has_value()) {
                emit signInFinished(true, *session, QString());
                return;
            }
        }
        const QString errMsg = friendlyError(extractErrorMessage(responseData));
        emit signInFinished(false, AuthSession{}, errMsg);
    });
}

void SupabaseAuthService::signInWithIdToken(const QString &idToken, const QString &provider)
{
    QJsonObject json;
    json[QStringLiteral("provider")] = provider;
    json[QStringLiteral("id_token")] = idToken;
    json[QStringLiteral("client_id")] = QStringLiteral("912012648755-2lhitqdiq1uqo5i8h20k2b3r63vaf8l0.apps.googleusercontent.com");
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/token?grant_type=id_token"));
    QNetworkReply *reply = m_net.post(req, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            auto session = parseSession(responseData);
            if (session.has_value()) {
                emit oauthFinished(true, *session, QString());
                return;
            }
        }
        const QString errMsg = friendlyError(extractErrorMessage(responseData));
        emit oauthFinished(false, AuthSession{}, errMsg);
    });
}

std::optional<AuthSession> SupabaseAuthService::signInWithIdTokenSync(const QString &idToken, const QString &provider, QString *errorOut, int timeoutMs)
{
    QJsonObject json;
    json[QStringLiteral("provider")] = provider;
    json[QStringLiteral("id_token")] = idToken;
    json[QStringLiteral("client_id")] = QStringLiteral("912012648755-2lhitqdiq1uqo5i8h20k2b3r63vaf8l0.apps.googleusercontent.com");
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/token?grant_type=id_token"));
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
        if (errorOut) *errorOut = QStringLiteral("Connection timed out.");
        return std::nullopt;
    }

    const QByteArray data = reply->readAll();
    const bool ok = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();

    if (ok) {
        return parseSession(data);
    }

    if (errorOut) *errorOut = friendlyError(extractErrorMessage(data));
    return std::nullopt;
}

void SupabaseAuthService::signUp(const QString &username, const QString &email, const QString &password)
{
    QJsonObject dataObj;
    dataObj[QStringLiteral("username")] = username;

    QJsonObject json;
    json[QStringLiteral("email")] = email;
    json[QStringLiteral("password")] = password;
    json[QStringLiteral("data")] = dataObj;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/signup"));
    QNetworkReply *reply = m_net.post(req, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            auto session = parseSession(responseData);
            if (session.has_value()) {
                emit signUpFinished(true, QString(), QString());
            } else {
                emit signUpFinished(true, QStringLiteral("Check your email to confirm your account, then sign in."), QString());
            }
            return;
        }
        const QString errMsg = friendlyError(extractErrorMessage(responseData));
        emit signUpFinished(false, QString(), errMsg);
    });
}

void SupabaseAuthService::requestPasswordReset(const QString &email)
{
    QJsonObject json;
    json[QStringLiteral("email")] = email;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/recover"));
    QNetworkReply *reply = m_net.post(req, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            emit passwordResetFinished(true, QString());
        } else {
            const QString errMsg = friendlyError(extractErrorMessage(responseData));
            emit passwordResetFinished(false, errMsg);
        }
    });
}

void SupabaseAuthService::signOut(const QString &accessToken)
{
    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/logout"), accessToken);
    QNetworkReply *reply = m_net.post(req, QByteArray("{}"));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        emit signOutFinished(true);
    });
}

void SupabaseAuthService::exchangeOAuthCode(const QString &code, const QString &codeVerifier)
{
    QJsonObject json;
    json[QStringLiteral("auth_code")] = code;
    json[QStringLiteral("code_verifier")] = codeVerifier;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/token?grant_type=pkce"));
    QNetworkReply *reply = m_net.post(req, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            auto session = parseSession(responseData);
            if (session.has_value()) {
                emit oauthFinished(true, *session, QString());
                return;
            }
        }
        const QString errMsg = friendlyError(extractErrorMessage(responseData));
        emit oauthFinished(false, AuthSession{}, errMsg);
    });
}

void SupabaseAuthService::refreshToken(const QString &refreshToken)
{
    QJsonObject json;
    json[QStringLiteral("refresh_token")] = refreshToken;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/token?grant_type=refresh_token"));
    QNetworkReply *reply = m_net.post(req, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            auto session = parseSession(responseData);
            if (session.has_value()) {
                emit signInFinished(true, *session, QString());
                return;
            }
        }
        const QString errMsg = friendlyError(extractErrorMessage(responseData));
        emit signInFinished(false, AuthSession{}, errMsg);
    });
}

std::optional<AuthSession> SupabaseAuthService::signInWithPasswordSync(const QString &email, const QString &password, QString *errorOut, int timeoutMs)
{
    QJsonObject json;
    json[QStringLiteral("email")] = email;
    json[QStringLiteral("password")] = password;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/token?grant_type=password"));
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
        if (errorOut) *errorOut = QStringLiteral("Connection timed out.");
        return std::nullopt;
    }

    const QByteArray data = reply->readAll();
    const bool ok = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();

    if (ok) {
        auto session = parseSession(data);
        if (session.has_value()) return session;
    }

    if (errorOut) *errorOut = friendlyError(extractErrorMessage(data));
    return std::nullopt;
}

AuthSignUpResult SupabaseAuthService::signUpSync(const QString &username, const QString &email, const QString &password, QString *errorOut, int timeoutMs)
{
    QJsonObject dataObj;
    dataObj[QStringLiteral("username")] = username;

    QJsonObject json;
    json[QStringLiteral("email")] = email;
    json[QStringLiteral("password")] = password;
    json[QStringLiteral("data")] = dataObj;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/signup"));
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
        if (errorOut) *errorOut = QStringLiteral("Connection timed out.");
        return AuthSignUpResult{};
    }

    const QByteArray data = reply->readAll();
    const bool ok = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();

    if (ok) {
        auto session = parseSession(data);
        if (session.has_value()) {
            return AuthSignUpResult{session, QString()};
        }
        return AuthSignUpResult{std::nullopt, QStringLiteral("Check your email to confirm your account, then sign in.")};
    }

    if (errorOut) *errorOut = friendlyError(extractErrorMessage(data));
    return AuthSignUpResult{};
}

bool SupabaseAuthService::requestPasswordResetSync(const QString &email, QString *errorOut, int timeoutMs)
{
    QJsonObject json;
    json[QStringLiteral("email")] = email;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/recover"));
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
        if (errorOut) *errorOut = QStringLiteral("Connection timed out.");
        return false;
    }

    const QByteArray data = reply->readAll();
    const bool ok = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();

    if (!ok && errorOut) {
        *errorOut = friendlyError(extractErrorMessage(data));
    }
    return ok;
}

std::optional<AuthSession> SupabaseAuthService::exchangeOAuthCodeSync(const QString &code, const QString &codeVerifier, QString *errorOut, int timeoutMs)
{
    QJsonObject json;
    json[QStringLiteral("auth_code")] = code;
    json[QStringLiteral("code_verifier")] = codeVerifier;
    const QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QNetworkRequest req = createRequest(QStringLiteral("/auth/v1/token?grant_type=pkce"));
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
        if (errorOut) *errorOut = QStringLiteral("Connection timed out.");
        return std::nullopt;
    }

    const QByteArray data = reply->readAll();
    const bool ok = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();

    if (ok) {
        auto session = parseSession(data);
        if (session.has_value()) return session;
    }

    if (errorOut) *errorOut = friendlyError(extractErrorMessage(data));
    return std::nullopt;
}

std::optional<AuthSession> SupabaseAuthService::parseSession(const QByteArray &jsonBytes)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }

    const QJsonObject root = doc.object();
    const QString accessToken = root.value(QStringLiteral("access_token")).toString();
    if (accessToken.isEmpty()) {
        return std::nullopt;
    }

    AuthSession session;
    session.accessToken = accessToken;
    session.refreshToken = root.value(QStringLiteral("refresh_token")).toString();

    const QJsonObject userObj = root.value(QStringLiteral("user")).toObject();
    session.userId = userObj.value(QStringLiteral("id")).toString();
    session.email = userObj.value(QStringLiteral("email")).toString();

    const QJsonObject meta = userObj.value(QStringLiteral("user_metadata")).toObject();
    for (const QString &key : { QStringLiteral("display_name"), QStringLiteral("full_name"), QStringLiteral("name"), QStringLiteral("username") }) {
        if (meta.contains(key) && !meta.value(key).toString().trimmed().isEmpty()) {
            session.displayName = meta.value(key).toString().trimmed();
            break;
        }
    }
    if (session.displayName.isEmpty() && !session.email.isEmpty()) {
        const QString prefix = session.email.split(QLatin1Char('@')).first();
        if (!prefix.isEmpty()) {
            session.displayName = prefix.left(1).toUpper() + prefix.mid(1);
        }
    }

    return session;
}

QString SupabaseAuthService::extractErrorMessage(const QByteArray &body)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject root = doc.object();
        for (const QString &key : { QStringLiteral("error_description"),
                                   QStringLiteral("msg"),
                                   QStringLiteral("error"),
                                   QStringLiteral("message") }) {
            if (root.contains(key) && root.value(key).isString()) {
                return root.value(key).toString();
            }
        }
    }
    return QString::fromUtf8(body);
}

QString SupabaseAuthService::friendlyError(const QString &raw)
{
    if (raw.isEmpty()) return QStringLiteral("Something went wrong. Try again.");
    if (raw.contains(QStringLiteral("Invalid login credentials"), Qt::CaseInsensitive))
        return QStringLiteral("Incorrect email or password.");
    if (raw.contains(QStringLiteral("Email not confirmed"), Qt::CaseInsensitive))
        return QStringLiteral("Check your email and confirm your account first.");
    if (raw.contains(QStringLiteral("User already registered"), Qt::CaseInsensitive))
        return QStringLiteral("An account with this email already exists.");
    if (raw.contains(QStringLiteral("Password should be at least"), Qt::CaseInsensitive))
        return QStringLiteral("Password must be at least 6 characters.");
    if (raw.contains(QStringLiteral("Unable to validate"), Qt::CaseInsensitive))
        return QStringLiteral("Session expired. Please sign in again.");
    if (raw.contains(QStringLiteral("network"), Qt::CaseInsensitive))
        return QStringLiteral("No internet connection.");
    return raw.left(120);
}

} // namespace mangadl
