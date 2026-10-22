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
    const QString path = url.path();

    if (path == QStringLiteral("/google-login")) {
        const QString html = generateFirebaseLoginHtml(serverPort());
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
        return;
    }

    const QUrlQuery query(url);
    const QString idToken = query.queryItemValue(QStringLiteral("id_token"));
    const QString userId = query.queryItemValue(QStringLiteral("user_id"));
    const QString email = query.queryItemValue(QStringLiteral("email"));
    const QString displayName = query.queryItemValue(QStringLiteral("display_name"));
    const QString code = query.queryItemValue(QStringLiteral("code"));
    const QString error = query.queryItemValue(QStringLiteral("error"));

    if (idToken.isEmpty() && code.isEmpty() && error.isEmpty()) {
        // Favicon or unrelated ping
        const QByteArray response = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();
        return;
    }

    const bool success = error.isEmpty() && (!idToken.isEmpty() || !code.isEmpty());
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

    if (success) {
        if (!idToken.isEmpty()) {
            emit idTokenReceived(idToken, userId, email, displayName);
        } else {
            emit authCodeReceived(code, path);
        }
    } else {
        emit authErrorReceived(error.isEmpty() ? QStringLiteral("Unknown error") : error, path);
    }

    stop();
}

QString OAuthLoopbackListener::generateFirebaseLoginHtml(quint16 port)
{
    return QStringLiteral(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <title>manga-dl - Google Sign In</title>\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <script src=\"https://www.gstatic.com/firebasejs/10.12.0/firebase-app-compat.js\"></script>\n"
        "  <script src=\"https://www.gstatic.com/firebasejs/10.12.0/firebase-auth-compat.js\"></script>\n"
        "  <style>\n"
        "    body { background: #09090b; color: #f4f4f5; font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, sans-serif; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }\n"
        "    .card { background: #18181b; border: 1px solid #27272a; border-radius: 16px; padding: 36px 32px; max-width: 380px; width: 90%; text-align: center; box-shadow: 0 10px 40px rgba(0,0,0,0.6); }\n"
        "    h1 { font-size: 22px; font-weight: 800; margin: 0 0 8px; text-transform: uppercase; letter-spacing: 0.05em; color: #ef4444; font-family: monospace; }\n"
        "    p { font-size: 13px; color: #a1a1aa; margin: 0 0 24px; line-height: 1.5; }\n"
        "    .btn { display: inline-flex; align-items: center; justify-content: center; gap: 12px; width: 100%; padding: 13px 20px; background: #ffffff; color: #18181b; font-weight: 700; font-size: 14px; border: none; border-radius: 12px; cursor: pointer; transition: all 0.2s; box-sizing: border-box; }\n"
        "    .btn:hover { background: #e4e4e7; transform: translateY(-1px); }\n"
        "    .status { font-size: 13px; color: #71717a; margin-top: 16px; min-height: 20px; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <div class=\"card\">\n"
        "    <h1>MANGA-DL</h1>\n"
        "    <p>Sign in with your Google account to sync your library, reading progress, and backups.</p>\n"
        "    <button class=\"btn\" id=\"loginBtn\" onclick=\"doGoogleSignIn()\">\n"
        "      <svg width=\"18\" height=\"18\" viewBox=\"0 0 24 24\"><path fill=\"#EA4335\" d=\"M12 5c1.6 0 3 .6 4.1 1.6l3.1-3.1C17.3 1.7 14.8 1 12 1 7.5 1 3.7 3.6 1.9 7.3l3.7 2.9C6.5 7.3 9 5 12 5z\"/><path fill=\"#4285F4\" d=\"M23.5 12.3c0-.8-.1-1.6-.2-2.3H12v4.5h6.5c-.3 1.5-1.1 2.8-2.4 3.7l3.7 2.9c2.2-2 3.7-5 3.7-8.8z\"/><path fill=\"#FBBC05\" d=\"M5.6 14.8c-.3-.8-.4-1.8-.4-2.8s.1-2 .4-2.8L1.9 6.3C.7 8.7 0 10.3 0 12s.7 3.3 1.9 5.7l3.7-2.9z\"/><path fill=\"#34A853\" d=\"M12 23c3.2 0 6-1.1 8-3l-3.7-2.9c-1.1.7-2.5 1.2-4.3 1.2-3 0-5.5-2.3-6.4-5.2L1.9 16c1.8 3.7 5.6 7 10.1 7z\"/></svg>\n"
        "      Continue with Google\n"
        "    </button>\n"
        "    <div class=\"status\" id=\"status\">Connecting to Firebase Auth...</div>\n"
        "  </div>\n"
        "  <script>\n"
        "    const firebaseConfig = {\n"
        "      apiKey: \"AIzaSyDzw5OFZJ_r4uO_wtGK30c8i92qV_aaXbw\",\n"
        "      authDomain: \"z-manga-dl.firebaseapp.com\",\n"
        "      projectId: \"z-manga-dl\",\n"
        "      storageBucket: \"z-manga-dl.firebasestorage.app\",\n"
        "      messagingSenderId: \"912012648755\",\n"
        "      appId: \"1:912012648755:web:1c2f4197c23e9dd6409e46\",\n"
        "      measurementId: \"G-M1HFP6V2DS\"\n"
        "    };\n"
        "    firebase.initializeApp(firebaseConfig);\n"
        "    let started = false;\n"
        "    async function doGoogleSignIn() {\n"
        "      const status = document.getElementById('status');\n"
        "      status.innerText = 'Opening Google sign-in window...';\n"
        "      const provider = new firebase.auth.GoogleAuthProvider();\n"
        "      try {\n"
        "        const result = await firebase.auth().signInWithPopup(provider);\n"
        "        status.innerText = 'Authenticated! Syncing with manga-dl...';\n"
        "        const credential = firebase.auth.GoogleAuthProvider.credentialFromResult(result);\n"
        "        const idToken = credential?.idToken || (await result.user.getIdToken());\n"
        "        const userId = encodeURIComponent(result.user.uid || '');\n"
        "        const email = encodeURIComponent(result.user.email || '');\n"
        "        const displayName = encodeURIComponent(result.user.displayName || '');\n"
        "        window.location.href = 'http://localhost:%1/auth/callback?id_token=' + encodeURIComponent(idToken) + '&user_id=' + userId + '&email=' + email + '&display_name=' + displayName;\n"
        "      } catch (err) {\n"
        "        status.innerText = err.message || 'Google sign-in was cancelled or failed.';\n"
        "      }\n"
        "    }\n"
        "    window.addEventListener('load', () => {\n"
        "      setTimeout(() => {\n"
        "        if (!started) { started = true; doGoogleSignIn(); }\n"
        "      }, 300);\n"
        "    });\n"
        "  </script>\n"
        "</body>\n"
        "</html>\n"
    ).arg(port);
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
