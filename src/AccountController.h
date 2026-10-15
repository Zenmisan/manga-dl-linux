#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QString>
#include <QVariantMap>
#include <memory>
#include "src/core/sync/SupabaseAuthService.h"
#include "src/core/sync/SupabaseSyncService.h"
#include "src/core/tracking/OAuthLoopbackListener.h"

namespace mangadl {

class AccountController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool signedIn READ signedIn NOTIFY signedInChanged)
    Q_PROPERTY(QString userId READ userId NOTIFY profileChanged)
    Q_PROPERTY(QString email READ email NOTIFY profileChanged)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY profileChanged)
    Q_PROPERTY(QString bio READ bio WRITE setBio NOTIFY profileChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY profileChanged)
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY syncingChanged)
    Q_PROPERTY(QString lastSyncedText READ lastSyncedText NOTIFY lastSyncedTextChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)

public:
    explicit AccountController(QObject *parent = nullptr);
    ~AccountController() override = default;

    bool signedIn() const;
    QString userId() const;
    QString email() const;
    QString displayName() const;
    void setDisplayName(const QString &val);
    QString bio() const;
    void setBio(const QString &val);
    QString avatarUrl() const;
    bool isSyncing() const { return m_isSyncing; }
    QString lastSyncedText() const;
    QString errorMessage() const { return m_errorMessage; }
    bool isBusy() const { return m_isBusy; }

    Q_INVOKABLE void signIn(const QString &email, const QString &password);
    Q_INVOKABLE void signUp(const QString &username, const QString &email, const QString &password);
    Q_INVOKABLE void requestPasswordReset(const QString &email);
    Q_INVOKABLE void signOut();
    Q_INVOKABLE void signInWithGoogle();
    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void saveProfile(const QString &displayName, const QString &bio);
    Q_INVOKABLE void testConnection(const QString &url, const QString &key);

signals:
    void signedInChanged();
    void profileChanged();
    void syncingChanged();
    void lastSyncedTextChanged();
    void errorMessageChanged();
    void busyChanged();
    void authSuccess(const QString &action);
    void connectionTestResult(bool success, int latencyMs, const QString &message);

private slots:
    void onGoogleAuthCodeReceived(const QString &code, const QString &path);

private:
    void setBusy(bool busy);
    void setErrorMessage(const QString &msg);
    void handleAuthSuccess(const AuthSession &session);

    std::unique_ptr<SupabaseAuthService> m_authService;
    std::unique_ptr<SupabaseSyncService> m_syncService;
    std::unique_ptr<OAuthLoopbackListener> m_loopback;

    QString m_googleCodeVerifier;
    bool m_isSyncing = false;
    bool m_isBusy = false;
    QString m_errorMessage;
};

} // namespace mangadl
