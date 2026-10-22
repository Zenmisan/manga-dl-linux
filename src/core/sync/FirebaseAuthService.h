#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include "AuthModels.h"

namespace mangadl {

struct FirebaseAuthResult {
    bool success = false;
    QString idToken;
    QString refreshToken;
    QString localId;
    QString email;
    QString displayName;
    QString errorMessage;
};

class FirebaseAuthService : public QObject {
    Q_OBJECT

public:
    explicit FirebaseAuthService(const QString &apiKey = QString(), QObject *parent = nullptr);
    ~FirebaseAuthService() override = default;

    QString apiKey() const { return m_apiKey; }
    void setApiKey(const QString &key) { m_apiKey = key.trimmed(); }

    void signInWithPassword(const QString &email, const QString &password);
    void signUp(const QString &email, const QString &password);
    void signUp(const QString &username, const QString &email, const QString &password);
    void updateProfile(const QString &idToken, const QString &displayName);
    void sendPasswordReset(const QString &email);

    static QString extractErrorMessage(const QByteArray &body);
    static QString friendlyError(const QString &raw);

signals:
    void signInFinished(const mangadl::FirebaseAuthResult &result);
    void signUpFinished(const mangadl::FirebaseAuthResult &result);
    void passwordResetFinished(bool success, const QString &errorMessage);

private:
    QString m_apiKey;
    QNetworkAccessManager m_net;
};

} // namespace mangadl
