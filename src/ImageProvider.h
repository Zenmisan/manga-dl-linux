#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QImage>
#include <QString>
#include <QSize>
#include <atomic>

/// Asynchronous image response that fetches, descrambles (Comixto / Asura),
/// caches on disk in AppPaths::imageCacheDir(), and returns a texture factory.
class AsyncImageResponse : public QQuickImageResponse
{
    Q_OBJECT
public:
    AsyncImageResponse(const QString &id, const QSize &requestedSize);
    ~AsyncImageResponse() override = default;

    QQuickTextureFactory *textureFactory() const override;
    QString errorString() const override { return m_error; }

    void cancel() override;

private:
    void finishWithImage(const QImage &image);
    void finishWithError(const QString &error);

    QSize m_requestedSize;
    QImage m_image;
    QString m_error;
    std::atomic<bool> m_cancelled{false};
};

/// Image provider for image://cover/ and image://page/.
class ImageProvider : public QQuickAsyncImageProvider
{
public:
    ImageProvider() = default;
    ~ImageProvider() override = default;

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};

/// Image provider for image://cbz/ that extracts page images in-memory from a CBZ/ZIP archive.
class CbzImageProvider : public QQuickAsyncImageProvider
{
public:
    CbzImageProvider() = default;
    ~CbzImageProvider() override = default;

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};
