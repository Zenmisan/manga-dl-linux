#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QTimer>

namespace mangadl {

class OAuthLoopbackListener : public QObject {
    Q_OBJECT

public:
    explicit OAuthLoopbackListener(QObject *parent = nullptr);
    ~OAuthLoopbackListener() override;

    bool start(quint16 port = 5678, int timeoutMs = 120000);
    void stop();
    bool isListening() const;
    quint16 serverPort() const;

    static QString generateResponseHtml(bool success);

signals:
    void authCodeReceived(const QString &code, const QString &path);
    void authErrorReceived(const QString &error, const QString &path);
    void timedOut();

private slots:
    void handleNewConnection();
    void handleClientData();

private:
    QTcpServer m_server;
    QTimer m_timeoutTimer;
};

} // namespace mangadl
