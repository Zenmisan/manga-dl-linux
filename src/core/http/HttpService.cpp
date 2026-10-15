#include "http/HttpService.h"

#include <QByteArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QUrlQuery>

namespace {

const char *const kUserAgent =
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/124.0.0.0 Safari/537.36";

bool isHttp(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    return url.isValid() && !url.host().isEmpty()
        && (scheme == QLatin1String("http") || scheme == QLatin1String("https"));
}

// "https://host[:port]/" - what a browser sends as Referer for a same-site request.
QByteArray originOf(const QUrl &url)
{
    QString origin = url.scheme().toLower() + QLatin1String("://") + url.host();
    if (url.port() > 0)
        origin += QLatin1Char(':') + QString::number(url.port());
    origin += QLatin1Char('/');
    return origin.toUtf8();
}

void applyCommon(QNetworkRequest &req)
{
    req.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(HttpService::kDefaultTimeoutMs);
}

} // namespace

HttpService::HttpService(QObject *parent) : QObject(parent) {}

QString HttpService::userAgent()
{
    return QString::fromLatin1(kUserAgent);
}

// static
QUrl HttpService::resolveUrl(const QString &url)
{
    // Extensions were written for the web backend, which exposes a fetch proxy:
    //   /manga/proxy/html?url=<encoded>      /manga/proxy/json?url=<encoded>
    // A native app talks to the site directly, so unwrap the real target.
    if (url.startsWith(QLatin1String("/manga/proxy/")) || url.startsWith(QLatin1String("/manga/image-proxy"))) {
        const QUrl dummy(QLatin1String("http://proxy.invalid") + url);  // only used to parse the query string
        const QString target = QUrlQuery(dummy).queryItemValue(QStringLiteral("url"), QUrl::FullyDecoded);
        if (!target.isEmpty())
            return QUrl(target);
    }
    return QUrl(url);
}

// static
QString HttpService::decodeBody(const QByteArray &body, const QString &contentType)
{
    QString charset;
    const int at = int(contentType.indexOf(QLatin1String("charset="), 0, Qt::CaseInsensitive));
    if (at >= 0) {
        charset = contentType.mid(at + 8).section(QLatin1Char(';'), 0, 0).trimmed();
        charset.remove(QLatin1Char('"'));
        charset.remove(QLatin1Char('\''));
    }
    if (charset.isEmpty()) {
        static const QRegularExpression metaCharset(
            QStringLiteral("<meta[^>]+charset\\s*=\\s*[\"']?\\s*([A-Za-z0-9_\\-]+)"),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch m = metaCharset.match(QString::fromLatin1(body.left(2048)));
        if (m.hasMatch())
            charset = m.captured(1);
    }

    QString text;
    bool decoded = false;
    const bool isUtf8 = charset.isEmpty() || charset.compare(QLatin1String("utf-8"), Qt::CaseInsensitive) == 0
        || charset.compare(QLatin1String("utf8"), Qt::CaseInsensitive) == 0;
    if (!isUtf8) {
        QStringDecoder decoder(charset.toLatin1().constData());
        if (decoder.isValid()) {
            text = decoder(body);
            decoded = !decoder.hasError();
        }
    }
    if (!decoded)
        text = QString::fromUtf8(body);
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);
    return text;
}

// static
HttpService::Response HttpService::toResponse(QNetworkReply *reply)
{
    Response r;
    r.url = reply->url().toString();
    r.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    r.text = decodeBody(reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toString());

    const QNetworkReply::NetworkError err = reply->error();
    // Codes 1-199 are connection-level and proxy errors (refused, host not found, timeout, aborted, TLS, reset).
    // Codes from 200 up describe HTTP/content problems for which a response (and status) exists.
    const int code = int(err);
    if (err != QNetworkReply::NoError && ((code > 0 && code < 200) || r.status == 0)) {
        r.transportError = true;
        r.timedOut = (err == QNetworkReply::TimeoutError);
        r.errorText = reply->errorString();
    }
    return r;
}

QNetworkReply *HttpService::send(const QString &url, const QVariantMap &opts, QString *error)
{
    const QUrl target = resolveUrl(url);
    if (!isHttp(target)) {
        if (error)
            *error = QStringLiteral("Unsupported or invalid URL: %1").arg(url);
        return nullptr;
    }

    QString method = opts.value(QStringLiteral("method"), QStringLiteral("GET")).toString().toUpper();
    if (method.isEmpty())
        method = QStringLiteral("GET");
    const QVariantMap headers = opts.value(QStringLiteral("headers")).toMap();
    const QVariant bodyVar = opts.value(QStringLiteral("body"));

    QNetworkRequest req(target);
    applyCommon(req);

    bool hasReferer = false;
    bool hasLanguage = false;
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        const QString lower = it.key().toLower();
        if (lower == QLatin1String("referer"))
            hasReferer = true;
        else if (lower == QLatin1String("accept-language"))
            hasLanguage = true;
        req.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    }
    if (!hasReferer)
        req.setRawHeader("Referer", originOf(target));
    if (!hasLanguage)
        req.setRawHeader("Accept-Language", "en-US,en;q=0.9");

    QByteArray body;
    if (bodyVar.metaType().id() == QMetaType::QByteArray)
        body = bodyVar.toByteArray();
    else if (!bodyVar.isNull())
        body = bodyVar.toString().toUtf8();

    QNetworkReply *reply = nullptr;
    if (method == QLatin1String("GET"))
        reply = m_nam.get(req);
    else if (method == QLatin1String("POST"))
        reply = m_nam.post(req, body);
    else
        reply = m_nam.sendCustomRequest(req, method.toLatin1(), body);

    if (!reply && error)
        *error = QStringLiteral("Could not start the request for %1").arg(url);
    return reply;
}

QNetworkReply *HttpService::get(const QUrl &url, const QString &referer)
{
    if (!isHttp(url))
        return nullptr;
    QNetworkRequest req(url);
    applyCommon(req);
    if (!referer.isEmpty())
        req.setRawHeader("Referer", referer.toUtf8());
    return m_nam.get(req);
}
