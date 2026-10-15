#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantMap>

class QNetworkReply;

/// HTTP client used by the JS extension engine and (later) the image pipeline.
///
/// Everything is asynchronous: callers get a QNetworkReply back and react to its signals. There are deliberately no
/// helpers that spin a nested event loop; one of those inside the JS worker thread would let unrelated queued work
/// run in the middle of a script.
///
/// One HttpService lives on the JsWorker thread and owns the QNetworkAccessManager, so connection pooling,
/// HTTP/2 multiplexing and the in-memory cookie jar are shared by every extension.
class HttpService : public QObject
{
    Q_OBJECT
public:
    static constexpr int kDefaultTimeoutMs = 30000;

    /// What a finished request looked like, in the terms the JS bridge needs.
    ///   * An HTTP response of any status (404, 500, ...) is NOT an error: status and text are filled in and the
    ///     script decides what to do, exactly like fetch() in a browser.
    ///   * transportError is true only when no usable HTTP response exists (DNS failure, refused, TLS, timeout,
    ///     connection reset, aborted).
    struct Response {
        int status = 0;
        QString text;
        QString url;                  ///< final URL after redirects
        bool transportError = false;
        bool timedOut = false;
        QString errorText;
    };

    explicit HttpService(QObject *parent = nullptr);

    /// Resolves a backend-relative proxy URL such as `/manga/proxy/html?url=<encoded>` into the real target.
    /// Anything else is returned as a plain QUrl (callers must still check that it is http/https).
    static QUrl resolveUrl(const QString &url);

    /// Decodes a response body using, in order: the Content-Type charset, a <meta charset> in the first 2 KB,
    /// then UTF-8. A byte-order mark is dropped.
    static QString decodeBody(const QByteArray &body, const QString &contentType);

    /// Reads a FINISHED reply into a Response. Does not delete the reply.
    static Response toResponse(QNetworkReply *reply);

    /// Starts a request. `opts` keys: method (default GET), headers (map), body (string or bytes).
    /// Only http and https are accepted. Returns nullptr and sets *error when the request cannot be started.
    /// The caller owns the reply (call deleteLater() when done).
    QNetworkReply *send(const QString &url, const QVariantMap &opts, QString *error);

    /// Plain GET for binary content such as page images. `referer` is sent when not empty.
    QNetworkReply *get(const QUrl &url, const QString &referer = QString());

    static QString userAgent();

private:
    QNetworkAccessManager m_nam;
};
