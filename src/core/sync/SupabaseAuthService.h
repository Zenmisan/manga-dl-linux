#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "AuthModels.h"

namespace mangadl {

class SupabaseAuthService : public QObject {
    Q_OBJECT

public:
    explicit SupabaseAuthService(const QString &supabaseUrl, const QString &anonKey, QObject *parent = nullptr);
    ~SupabaseAuthService() override = default;

    QString supabaseUrl() const { return m_url; }
    QString anonKey() const { return m_anonKey; }
    void setCredentials(const QString &url, const QString &anonKey);

    // PKCE generator for OAuth
    static QPair<QString, QString> generatePkceS256();

    // URL to open in browser for Google (or other provider) OAuth
    QString getOAuthAuthorizeUrl(const QString &provider, const QString &redirectUri, const QString &codeChallenge) const;

    // Async REST requests
    void signInWithPassword(const QString &email, const QString &password);
    void signUp(const QString &username, const QString &email, const QString &password);
    void requestPasswordReset(const QString &email);
    void signOut(const QString &accessToken);
    void exchangeOAuthCode(const QString &code, const QString &codeVerifier);
    void refreshToken(const QString &refreshToken);

    // Synchronous convenience helpers for testing / blocking worker contexts
    std::optional<AuthSession> signInWithPasswordSync(const QString &email, const QString &password, QString *errorOut = nullptr, int timeoutMs = 10000);
    AuthSignUpResult signUpSync(const QString &username, const QString &email, const QString &password, QString *errorOut = nullptr, int timeoutMs = 10000);
    bool requestPasswordResetSync(const QString &email, QString *errorOut = nullptr, int timeoutMs = 10000);
    std::optional<AuthSession> exchangeOAuthCodeSync(const QString &code, const QString &codeVerifier, QString *errorOut = nullptr, int timeoutMs = 10000);

    // Parse and error extraction helpers
    static std::optional<AuthSession> parseSession(const QByteArray &jsonBytes);
    static QString extractErrorMessage(const QByteArray &body);
    static QString friendlyError(const QString &raw);

signals:
    void signInFinished(bool success, const mangadl::AuthSession &session, const QString &errorMessage);
    void signUpFinished(bool success, const QString &message, const QString &errorMessage);
    void passwordResetFinished(bool success, const QString &errorMessage);
    void oauthFinished(bool success, const mangadl::AuthSession &session, const QString &errorMessage);
    void signOutFinished(bool success);

private:
    QNetworkRequest createRequest(const QString &path, const QString &accessToken = QString()) const;

    QString m_url;
    QString m_anonKey;
    QNetworkAccessManager m_net;
};

} // namespace mangadl
