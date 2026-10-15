#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QString>
#include <QVariantList>
#include <memory>
#include "src/core/tracking/TrackerService.h"
#include "src/core/tracking/OAuthLoopbackListener.h"

namespace mangadl {

class TrackerController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList trackers READ trackers NOTIFY trackersChanged)
    Q_PROPERTY(bool isConnecting READ isConnecting NOTIFY connectingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit TrackerController(QObject *parent = nullptr);
    ~TrackerController() override = default;

    QVariantList trackers() const;
    bool isConnecting() const { return m_isConnecting; }
    QString errorMessage() const { return m_errorMessage; }

    Q_INVOKABLE void connectTracker(const QString &trackerName);
    Q_INVOKABLE void disconnectTracker(const QString &trackerName);

signals:
    void trackersChanged();
    void connectingChanged();
    void errorMessageChanged();
    void trackerConnected(const QString &trackerName, const QString &username);

private slots:
    void onAuthCodeReceived(const QString &code, const QString &path);

private:
    void setConnecting(bool connecting);
    void setErrorMessage(const QString &msg);

    std::unique_ptr<TrackerService> m_trackerService;
    std::unique_ptr<OAuthLoopbackListener> m_loopback;

    QString m_activeConnectingTracker;
    QString m_malCodeVerifier;
    bool m_isConnecting = false;
    QString m_errorMessage;
};

} // namespace mangadl
