#include <QtTest>
#include <QGuiApplication>
#include <QImage>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QPainter>

#include "ImageProvider.h"
#include "util/AppPaths.h"

class tst_imageprovider : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void localImageLoadingTest();
    void imageCachingTest();
    void invalidUrlTest();

private:
    QTemporaryDir m_tempDir;
};

void tst_imageprovider::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    AppPaths::setRootOverride(m_tempDir.path());
    QVERIFY(AppPaths::ensureDirs());
}

void tst_imageprovider::cleanupTestCase()
{
    AppPaths::setRootOverride(QString());
}

void tst_imageprovider::localImageLoadingTest()
{
    // Create a local sample image
    const QString imgPath = m_tempDir.path() + QStringLiteral("/sample.png");
    QImage testImg(64, 96, QImage::Format_RGBA8888);
    testImg.fill(QColor(100, 150, 200));
    QVERIFY(testImg.save(imgPath, "PNG"));

    ImageProvider provider;
    QQuickImageResponse *resp = provider.requestImageResponse(imgPath, QSize());
    QVERIFY(resp != nullptr);

    QSignalSpy spy(resp, &QQuickImageResponse::finished);
    if (spy.isEmpty()) {
        QVERIFY(spy.wait(2000));
    }

    QVERIFY(resp->errorString().isEmpty());
    QQuickTextureFactory *factory = resp->textureFactory();
    QVERIFY(factory != nullptr);
    QCOMPARE(factory->textureSize().width(), 64);
    QCOMPARE(factory->textureSize().height(), 96);
    delete factory;
    delete resp;
}

void tst_imageprovider::imageCachingTest()
{
    // Write an image directly to the cache
    const QString cacheDir = AppPaths::imageCacheDir();
    QDir().mkpath(cacheDir);

    const QString testUrl = QStringLiteral("https://example.com/cached_cover.jpg");
    const QString cacheKey = QString::fromUtf8(
        QCryptographicHash::hash(testUrl.toUtf8(), QCryptographicHash::Sha256).toHex());
    const QString cachePath = cacheDir + QLatin1Char('/') + cacheKey + QLatin1String(".img");

    QImage cachedImg(80, 120, QImage::Format_RGBA8888);
    cachedImg.fill(QColor(220, 80, 50));
    QVERIFY(cachedImg.save(cachePath, "PNG"));

    ImageProvider provider;
    QQuickImageResponse *resp = provider.requestImageResponse(testUrl, QSize());
    QVERIFY(resp != nullptr);

    QSignalSpy spy(resp, &QQuickImageResponse::finished);
    if (spy.isEmpty()) {
        QVERIFY(spy.wait(2000));
    }

    QVERIFY(resp->errorString().isEmpty());
    QQuickTextureFactory *factory = resp->textureFactory();
    QVERIFY(factory != nullptr);
    QCOMPARE(factory->textureSize().width(), 80);
    QCOMPARE(factory->textureSize().height(), 120);
    delete factory;
    delete resp;
}

void tst_imageprovider::invalidUrlTest()
{
    ImageProvider provider;
    QQuickImageResponse *resp = provider.requestImageResponse(QStringLiteral("invalid://not_found.xyz"), QSize());
    QVERIFY(resp != nullptr);

    QSignalSpy spy(resp, &QQuickImageResponse::finished);
    if (spy.isEmpty()) {
        QVERIFY(spy.wait(2000));
    }

    // Should finish with an error
    QVERIFY(!resp->errorString().isEmpty());
    QVERIFY(resp->textureFactory() == nullptr);
    delete resp;
}

QTEST_MAIN(tst_imageprovider)
#include "tst_imageprovider.moc"
