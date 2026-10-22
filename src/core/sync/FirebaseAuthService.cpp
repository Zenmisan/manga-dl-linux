#include "FirebaseAuthService.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace mangadl {

static const char *kDefaultFirebaseApiKey = "AIzaSyDzw5OFZJ_r4uO_wtGK30c8i92qV_aaXbw";

FirebaseAuthService::FirebaseAuthService(const QString &apiKey, QObject *parent)
    : QObject(parent)
    , m_apiKey(apiKey.isEmpty() ? QString::fromLatin1(kDefaultFirebaseApiKey) : apiKey.trimmed())
{
}

QString FirebaseAuthService::extractErrorMessage(const QByteArray &body)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject root = doc.object();
        if (root.contains(QStringLiteral("error"))) {
            const QJsonObject errObj = root.value(QStringLiteral("error")).toObject();
            const QString msg = errObj.value(QStringLiteral("message")).toString();
            if (!msg.isEmpty()) return msg;
        }
    }
    return QString::fromUtf8(body);
}

QString FirebaseAuthService::friendlyError(const QString &raw)
{
    if (raw.isEmpty()) return QStringLiteral("Something went wrong. Try again.");
    if (raw.contains(QStringLiteral("INVALID_LOGIN_CREDENTIALS")) ||
        raw.contains(QStringLiteral("EMAIL_NOT_FOUND")) ||
        raw.contains(QStringLiteral("INVALID_PASSWORD"))) {
        return QStringLiteral("Incorrect email or password.");
    }
    if (raw.contains(QStringLiteral("EMAIL_EXISTS"))) {
        return QStringLiteral("An account with this email already exists.");
    }
    if (raw.contains(QStringLiteral("TOO_MANY_ATTEMPTS_TRY_LATER"))) {
        return QStringLiteral("Too many attempts. Please try again later.");
    }
    if (raw.contains(QStringLiteral("USER_DISABLED"))) {
        return QStringLiteral("This user account has been disabled.");
    }
    if (raw.contains(QStringLiteral("WEAK_PASSWORD"))) {
        return QStringLiteral("Password must be at least 6 characters.");
    }
    if (raw.contains(QStringLiteral("INVALID_EMAIL"))) {
        return QStringLiteral("Please enter a valid email address.");
    }
    return raw.length() > 120 ? raw.left(120) : raw;
}

void FirebaseAuthService::signInWithPassword(const QString &email, const QString &password)
{
    const QString urlStr = QStringLiteral("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%1")
                               .arg(m_apiKey);
    QNetworkRequest req{QUrl(urlStr)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject json;
    json[QStringLiteral("email")] = email;
    json[QStringLiteral("password")] = password;
    json[QStringLiteral("returnSecureToken")] = true;

    QNetworkReply *reply = m_net.post(req, QJsonDocument(json).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        FirebaseAuthResult res;

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                res.success = true;
                res.idToken = obj.value(QStringLiteral("idToken")).toString();
                res.refreshToken = obj.value(QStringLiteral("refreshToken")).toString();
                res.localId = obj.value(QStringLiteral("localId")).toString();
                res.email = obj.value(QStringLiteral("email")).toString();
                res.displayName = obj.value(QStringLiteral("displayName")).toString();
                emit signInFinished(res);
                return;
            }
        }

        res.success = false;
        res.errorMessage = friendlyError(extractErrorMessage(responseData));
        emit signInFinished(res);
    });
}

void FirebaseAuthService::signUp(const QString &email, const QString &password)
{
    signUp(QString(), email, password);
}

void FirebaseAuthService::signUp(const QString &username, const QString &email, const QString &password)
{
    const QString urlStr = QStringLiteral("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=%1")
                               .arg(m_apiKey);
    QNetworkRequest req{QUrl(urlStr)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject json;
    json[QStringLiteral("email")] = email;
    json[QStringLiteral("password")] = password;
    json[QStringLiteral("returnSecureToken")] = true;

    QNetworkReply *reply = m_net.post(req, QJsonDocument(json).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, username]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        FirebaseAuthResult res;

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                res.success = true;
                res.idToken = obj.value(QStringLiteral("idToken")).toString();
                res.refreshToken = obj.value(QStringLiteral("refreshToken")).toString();
                res.localId = obj.value(QStringLiteral("localId")).toString();
                res.email = obj.value(QStringLiteral("email")).toString();
                res.displayName = obj.value(QStringLiteral("displayName")).toString();

                if (!username.trimmed().isEmpty() && !res.idToken.isEmpty()) {
                    res.displayName = username.trimmed();
                    updateProfile(res.idToken, username.trimmed());
                }

                emit signUpFinished(res);
                return;
            }
        }

        res.success = false;
        res.errorMessage = friendlyError(extractErrorMessage(responseData));
        emit signUpFinished(res);
    });
}

void FirebaseAuthService::updateProfile(const QString &idToken, const QString &displayName)
{
    if (idToken.isEmpty() || displayName.isEmpty()) return;

    const QString urlStr = QStringLiteral("https://identitytoolkit.googleapis.com/v1/accounts:update?key=%1")
                               .arg(m_apiKey);
    QNetworkRequest req{QUrl(urlStr)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject json;
    json[QStringLiteral("idToken")] = idToken;
    json[QStringLiteral("displayName")] = displayName;
    json[QStringLiteral("returnSecureToken")] = true;

    QNetworkReply *reply = m_net.post(req, QJsonDocument(json).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void FirebaseAuthService::sendPasswordReset(const QString &email)
{
    const QString urlStr = QStringLiteral("https://identitytoolkit.googleapis.com/v1/accounts:sendOobCode?key=%1")
                               .arg(m_apiKey);
    QNetworkRequest req{QUrl(urlStr)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject json;
    json[QStringLiteral("requestType")] = QStringLiteral("PASSWORD_RESET");
    json[QStringLiteral("email")] = email;

    QNetworkReply *reply = m_net.post(req, QJsonDocument(json).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            emit passwordResetFinished(true, QString());
        } else {
            const QString err = friendlyError(extractErrorMessage(responseData));
            emit passwordResetFinished(false, err);
        }
    });
}

} // namespace mangadl
