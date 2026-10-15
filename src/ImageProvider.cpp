#include "ImageProvider.h"

#include "extensions/Descramble.h"
#include "http/HttpService.h"
#include "util/AppPaths.h"
#include "util/AppLog.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <functional>

namespace {

class ImageDownloaderWorker : public QObject
{
public:
    explicit ImageDownloaderWorker(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_nam = new QNetworkAccessManager(this);
    }

    void download(const QUrl &url, const QString &referer, std::function<void(QByteArray, QString)> callback)
    {
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, HttpService::userAgent());
        req.setRawHeader("Accept", "image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8");
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setTransferTimeout(30000);

        if (!referer.isEmpty()) {
            req.setRawHeader("Referer", referer.toUtf8());
        }

        QNetworkReply *reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                callback(QByteArray(), reply->errorString());
            } else {
                callback(reply->readAll(), QString());
            }
        });
    }

private:
    QNetworkAccessManager *m_nam = nullptr;
};

class ImageDownloader : public QObject
{
public:
    static ImageDownloader &instance()
    {
        static ImageDownloader s_instance;
        return s_instance;
    }

    void fetch(const QUrl &url, const QString &referer, std::function<void(QByteArray, QString)> callback)
    {
        QMetaObject::invokeMethod(m_worker, [worker = m_worker, url, referer, callback]() {
            worker->download(url, referer, callback);
        });
    }

private:
    ImageDownloader()
    {
        m_worker = new ImageDownloaderWorker();
        m_worker->moveToThread(&m_thread);
        m_thread.start();
    }

    ~ImageDownloader() override
    {
        m_thread.quit();
        m_thread.wait();
        delete m_worker;
    }

    QThread m_thread;
    ImageDownloaderWorker *m_worker = nullptr;
};

} // namespace

// ─── AsyncImageResponse ───────────────────────────────────────────────────────

AsyncImageResponse::AsyncImageResponse(const QString &id, const QSize &requestedSize)
    : m_requestedSize(requestedSize)
{
    QString raw = id;
    QString referer;

    // Extract referer query parameter if present
    const int refIdx = raw.indexOf(QLatin1String("?referer="));
    if (refIdx >= 0) {
        referer = QUrl::fromPercentEncoding(raw.mid(refIdx + 9).toUtf8());
        raw = raw.left(refIdx);
    }

    // Decode percent-encoding if the protocol or path was encoded
    if (raw.startsWith(QLatin1String("http%3A"), Qt::CaseInsensitive) ||
        raw.startsWith(QLatin1String("https%3A"), Qt::CaseInsensitive) ||
        raw.startsWith(QLatin1String("comixto%3A"), Qt::CaseInsensitive) ||
        raw.startsWith(QLatin1String("%2F"), Qt::CaseInsensitive)) {
        raw = QUrl::fromPercentEncoding(raw.toUtf8());
    }

    // Check local file
    if (raw.startsWith(QLatin1String("file://"))) {
        const QString localPath = QUrl(raw).toLocalFile();
        QImage img;
        if (img.load(localPath)) {
            finishWithImage(img);
            return;
        }
    } else if (raw.startsWith(QLatin1Char('/')) && !raw.startsWith(QLatin1String("/manga/"))) {
        QImage img;
        if (img.load(raw)) {
            finishWithImage(img);
            return;
        }
    }

    // Parse descramble parameters
    const auto comixto = Descramble::parseComixtoUrl(raw);
    const auto asura = Descramble::parseAsuraUrl(raw);

    // Compute disk cache key from full input specification
    const QString cacheKey = QString::fromUtf8(
        QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Sha256).toHex());
    const QString cacheDir = AppPaths::imageCacheDir();
    QDir().mkpath(cacheDir);
    const QString cacheFile = cacheDir + QLatin1Char('/') + cacheKey + QLatin1String(".img");

    // Check disk cache
    if (QFile::exists(cacheFile) && QFileInfo(cacheFile).size() > 0) {
        QImage cached;
        if (cached.load(cacheFile)) {
            finishWithImage(cached);
            return;
        }
    }

    // Determine target URL for network fetch
    QUrl fetchUrl;
    if (comixto.valid) {
        fetchUrl = QUrl(comixto.imageUrl);
    } else if (asura.valid) {
        fetchUrl = QUrl(asura.imageUrl);
    } else {
        fetchUrl = HttpService::resolveUrl(raw);
    }

    if (!fetchUrl.isValid() || fetchUrl.scheme().isEmpty()) {
        finishWithError(QStringLiteral("Invalid image URL: ") + raw);
        return;
    }

    if (referer.isEmpty()) {
        referer = fetchUrl.scheme() + QLatin1String("://") + fetchUrl.host() + QLatin1Char('/');
    }

    // Download asynchronously on background thread
    ImageDownloader::instance().fetch(fetchUrl, referer, [this, cacheFile, comixto, asura](QByteArray data, QString err) {
        if (m_cancelled.load()) {
            return;
        }
        if (!err.isEmpty() || data.isEmpty()) {
            finishWithError(err.isEmpty() ? QStringLiteral("Empty image data") : err);
            return;
        }

        if (comixto.valid) {
            QImage descrambled = Descramble::descrambleComixto(data, comixto);
            if (descrambled.isNull()) {
                finishWithError(QStringLiteral("Comixto descramble failed"));
                return;
            }
            descrambled.save(cacheFile, "PNG");
            finishWithImage(descrambled);
        } else if (asura.valid) {
            QImage descrambled = Descramble::reassembleAsura(data, asura);
            if (descrambled.isNull()) {
                finishWithError(QStringLiteral("Asura reassembly failed"));
                return;
            }
            descrambled.save(cacheFile, "PNG");
            finishWithImage(descrambled);
        } else {
            QImage img;
            if (!img.loadFromData(data)) {
                finishWithError(QStringLiteral("Failed to decode image data"));
                return;
            }
            QFile f(cacheFile);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(data);
                f.close();
            }
            finishWithImage(img);
        }
    });
}

void AsyncImageResponse::cancel()
{
    m_cancelled.store(true);
}

void AsyncImageResponse::finishWithImage(const QImage &image)
{
    if (m_requestedSize.isValid() && m_requestedSize.width() > 0 && m_requestedSize.height() > 0) {
        m_image = image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        m_image = image;
    }
    QTimer::singleShot(0, this, [this] { emit finished(); });
}

void AsyncImageResponse::finishWithError(const QString &error)
{
    m_error = error;
    QTimer::singleShot(0, this, [this] { emit finished(); });
}

QQuickTextureFactory *AsyncImageResponse::textureFactory() const
{
    if (m_image.isNull()) {
        return nullptr;
    }
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

// ─── ImageProvider ──────────────────────────────────────────────────────────

QQuickImageResponse *ImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    return new AsyncImageResponse(id, requestedSize);
}

// ─── CbzImageProvider ───────────────────────────────────────────────────────

#include "downloads/CbzBuilder.h"

class CbzImageResponse : public QQuickImageResponse
{
public:
    CbzImageResponse(const QString &id, const QSize &requestedSize)
        : m_requestedSize(requestedSize)
    {
        QString raw = id;
        QString entryName;
        const int entryIdx = raw.indexOf(QLatin1String("?entry="));
        if (entryIdx >= 0) {
            entryName = QUrl::fromPercentEncoding(raw.mid(entryIdx + 7).toUtf8());
            raw = raw.left(entryIdx);
        }
        QString cbzPath = QUrl::fromPercentEncoding(raw.toUtf8());
        if (cbzPath.startsWith(QLatin1String("file://"))) {
            cbzPath = QUrl(cbzPath).toLocalFile();
        }

        const QImage img = mangadl::CbzBuilder::extractImage(cbzPath, entryName);
        if (!img.isNull()) {
            if (m_requestedSize.isValid() && m_requestedSize.width() > 0 && m_requestedSize.height() > 0) {
                m_image = img.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            } else {
                m_image = img;
            }
        } else {
            m_error = QStringLiteral("Failed to read image from CBZ: %1 (entry %2)").arg(cbzPath, entryName);
        }
        QTimer::singleShot(0, this, [this] { emit finished(); });
    }

    QQuickTextureFactory *textureFactory() const override
    {
        if (m_image.isNull()) return nullptr;
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    QString errorString() const override { return m_error; }

private:
    QSize m_requestedSize;
    QImage m_image;
    QString m_error;
};

QQuickImageResponse *CbzImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    return new CbzImageResponse(id, requestedSize);
}
