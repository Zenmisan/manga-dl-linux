#include "OAuthLoopbackListener.h"
#include <QUrl>
#include <QUrlQuery>
#include <QHostAddress>

namespace mangadl {

OAuthLoopbackListener::OAuthLoopbackListener(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &OAuthLoopbackListener::handleNewConnection);
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this]() {
        stop();
        emit timedOut();
    });
}

OAuthLoopbackListener::~OAuthLoopbackListener()
{
    stop();
}

bool OAuthLoopbackListener::start(quint16 port, int timeoutMs)
{
    stop();
    if (!m_server.listen(QHostAddress::LocalHost, port)) {
        return false;
    }
    if (timeoutMs > 0) {
        m_timeoutTimer.start(timeoutMs);
    }
    return true;
}

void OAuthLoopbackListener::stop()
{
    m_timeoutTimer.stop();
    if (m_server.isListening()) {
        m_server.close();
    }
}

bool OAuthLoopbackListener::isListening() const
{
    return m_server.isListening();
}

quint16 OAuthLoopbackListener::serverPort() const
{
    return m_server.serverPort();
}

void OAuthLoopbackListener::handleNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        if (!socket) continue;
        connect(socket, &QTcpSocket::readyRead, this, &OAuthLoopbackListener::handleClientData);
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void OAuthLoopbackListener::handleClientData()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) return;

    const QByteArray data = socket->readAll();
    const QString requestStr = QString::fromUtf8(data);

    // Look for GET request line
    const QString firstLine = requestStr.section(QStringLiteral("\r\n"), 0, 0);
    const QStringList parts = firstLine.split(QLatin1Char(' '));
    if (parts.size() < 2 || parts[0] != QStringLiteral("GET")) {
        socket->disconnectFromHost();
        return;
    }

    const QString rawTarget = parts[1];
    const QUrl url(QStringLiteral("http://localhost") + rawTarget);
    const QUrlQuery query(url);

    const QString code = query.queryItemValue(QStringLiteral("code"));
    const QString error = query.queryItemValue(QStringLiteral("error"));

    if (code.isEmpty() && error.isEmpty()) {
        // Favicon or unrelated ping
        const QByteArray response = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();
        return;
    }

    const bool success = error.isEmpty() && !code.isEmpty();
    const QString html = generateResponseHtml(success);
    const QByteArray htmlBytes = html.toUtf8();

    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(htmlBytes.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += htmlBytes;

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();

    const QString path = url.path();
    if (success) {
        emit authCodeReceived(code, path);
    } else {
        emit authErrorReceived(error.isEmpty() ? QStringLiteral("Unknown error") : error, path);
    }

    stop();
}

QString OAuthLoopbackListener::generateResponseHtml(bool success)
{
    const QString title = success ? QStringLiteral("Connected to manga-dl") : QStringLiteral("Authorization Error");
    const QString heading = success ? QStringLiteral("Authorization Successful!") : QStringLiteral("Connection Failed");
    const QString message = success
        ? QStringLiteral("Your account was connected successfully. You can close this tab and return to the manga-dl app.")
        : QStringLiteral("An error occurred during authentication. Please return to the app and try again.");
    const QString accentColor = success ? QStringLiteral("#3fb950") : QStringLiteral("#f85149");
    const QString icon = success ? QStringLiteral("✓") : QStringLiteral("✕");

    return QStringLiteral(R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>%1</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background-color: #0d1117;
      color: #e6edf3;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      display: flex;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      padding: 24px;
    }
    .card {
      background: #161b22;
      border: 1px solid #30363d;
      border-radius: 16px;
      padding: 40px;
      max-width: 440px;
      width: 100%;
      text-align: center;
      box-shadow: 0 16px 32px rgba(0,0,0,0.4);
    }
    .icon {
      width: 48px;
      height: 48px;
      border-radius: 50%;
      background: %222;
      color: %2;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      font-size: 24px;
      font-weight: bold;
      margin-bottom: 20px;
    }
    h1 {
      font-size: 20px;
      font-weight: 800;
      margin-bottom: 10px;
      color: #ffffff;
    }
    p {
      font-size: 14px;
      line-height: 1.6;
      color: #8b949e;
    }
  </style>
</head>
<body>
  <div class="card">
    <div class="icon">%3</div>
    <h1>%4</h1>
    <p>%5</p>
  </div>
</body>
</html>)")
    .arg(title, accentColor, icon, heading, message);
}

} // namespace mangadl
