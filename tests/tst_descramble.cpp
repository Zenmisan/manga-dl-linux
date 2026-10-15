// tst_descramble.cpp - native descrambling: URL parsing and pixel-level round trips
#include <QBuffer>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QtTest>

#include "extensions/Descramble.h"

namespace {

QByteArray pngBytes(const QImage &image)
{
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "PNG");
    return bytes;
}

// A grid whose tiles each have their own colour, so any permutation is visible.
QImage coloredGrid(int cols, int rows, int tileW, int tileH)
{
    QImage img(cols * tileW, rows * tileH, QImage::Format_ARGB32);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const int i = r * cols + c;
            const QRgb color = qRgb((i * 37) % 256, (i * 91 + 20) % 256, (i * 53 + 90) % 256);
            for (int y = 0; y < tileH; ++y) {
                for (int x = 0; x < tileW; ++x)
                    img.setPixel(c * tileW + x, r * tileH + y, color);
            }
        }
    }
    return img;
}

QRgb tileColor(const QImage &img, int col, int row, int tileW, int tileH)
{
    return img.pixel(col * tileW + tileW / 2, row * tileH + tileH / 2);
}

// The encryption streams from comixto.js, written out independently of Descramble.cpp.
QByteArray xorWithLcg(QByteArray bytes, qint32 seed, int length)
{
    qint32 state = seed;
    for (int i = 0; i < length && i < bytes.size(); ++i) {
        state = qint32(qint64(state) * 1000005 + 1234567891);
        bytes[i] = char(uchar(bytes[i]) ^ uchar(quint32(state) >> 24));
    }
    return bytes;
}

QByteArray xorWithXorshiftLow(QByteArray bytes, quint32 state, int length)
{
    for (int i = 0; i < length && i < bytes.size(); ++i) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        bytes[i] = char(uchar(bytes[i]) ^ uchar(state & 0xFF));
    }
    return bytes;
}

} // namespace

class TstDescramble : public QObject
{
    Q_OBJECT

private slots:

    // ── parseComixtoUrl ───────────────────────────────────────────────────────
    void parseComixtoUrl_valid()
    {
        const QString url = "comixto://img?url=https%3A%2F%2Fcomix.to%2Fimg.jpg"
                            "&seed=12345&algo=3&encSeed=67890&encLen=512&encAlgo=2";
        const auto p = Descramble::parseComixtoUrl(url);
        QVERIFY(p.valid);
        QCOMPARE(p.imageUrl, QStringLiteral("https://comix.to/img.jpg"));
        QCOMPARE(p.seed, 12345);
        QCOMPARE(p.algo, QStringLiteral("3"));
        QCOMPARE(p.encSeed, 67890);
        QCOMPARE(p.encLen, 512);
        QCOMPARE(p.encAlgo, QStringLiteral("2"));
    }

    void parseComixtoUrl_notComixto()
    {
        QVERIFY(!Descramble::parseComixtoUrl(QStringLiteral("https://example.com/img.jpg")).valid);
    }

    void parseComixtoUrl_seedsWrapLikeInt32()
    {
        // comixto.js applies int32() to its seeds, so values above 2^31 and negative values are legal.
        const auto big = Descramble::parseComixtoUrl(
            QStringLiteral("comixto://img?url=https%3A%2F%2Fx.test%2Fa.jpg&seed=3000000000&encSeed=4294967295"));
        QVERIFY(big.valid);
        QCOMPARE(big.seed, qint32(-1294967296));  // 3000000000 - 2^32
        QCOMPARE(big.encSeed, qint32(-1));

        const auto negative = Descramble::parseComixtoUrl(
            QStringLiteral("comixto://img?url=https%3A%2F%2Fx.test%2Fa.jpg&seed=-5"));
        QCOMPARE(negative.seed, qint32(-5));
    }

    void parseComixtoUrl_decodesOnce()
    {
        // A literal "%20" and a "&" inside the image address must survive a single round of decoding.
        const QString original = QStringLiteral("https://cdn.test/a%20b.jpg?x=1&y=2");
        const QString url = QStringLiteral("comixto://img?url=")
            + QString::fromLatin1(QUrl::toPercentEncoding(original)) + QStringLiteral("&seed=1");
        QCOMPARE(Descramble::parseComixtoUrl(url).imageUrl, original);
    }

    // ── comixto pixels ────────────────────────────────────────────────────────
    void comixtoXorLcgRoundTrip()
    {
        const QImage original = coloredGrid(4, 4, 8, 8);
        const QByteArray encrypted = xorWithLcg(pngBytes(original), 12345, 64);
        QVERIFY(QImage::fromData(encrypted).isNull());  // really scrambled

        Descramble::ComixtoParams p;
        p.valid = true;
        p.imageUrl = QStringLiteral("x");
        p.encSeed = 12345;
        p.encLen = 64;
        const QImage out = Descramble::descrambleComixto(encrypted, p);
        QVERIFY(!out.isNull());
        QCOMPARE(out.convertToFormat(QImage::Format_ARGB32), original);
    }

    void comixtoXorshiftRoundTrip()
    {
        const QImage original = coloredGrid(4, 4, 8, 8);
        // encAlgo "2" tries xorshift with (seed | 1) first; 777 is already odd.
        const QByteArray encrypted = xorWithXorshiftLow(pngBytes(original), 777, 64);
        QVERIFY(QImage::fromData(encrypted).isNull());

        Descramble::ComixtoParams p;
        p.valid = true;
        p.imageUrl = QStringLiteral("x");
        p.encSeed = 777;
        p.encLen = 64;
        p.encAlgo = QStringLiteral("2");
        const QImage out = Descramble::descrambleComixto(encrypted, p);
        QVERIFY(!out.isNull());
        QCOMPARE(out.convertToFormat(QImage::Format_ARGB32), original);
    }

    void comixtoGridIsAPermutationOfTheTiles()
    {
        constexpr int T = 10;
        const QImage original = coloredGrid(5, 5, T, T);
        QSet<QRgb> originalColors;
        for (int i = 0; i < 25; ++i)
            originalColors.insert(tileColor(original, i % 5, i / 5, T, T));
        QCOMPARE(originalColors.size(), 25);  // the test image really has 25 distinct tiles

        for (const QString &algo : {QString(), QStringLiteral("3")}) {
            Descramble::ComixtoParams p;
            p.valid = true;
            p.imageUrl = QStringLiteral("x");
            p.seed = 12345;
            p.algo = algo;
            const QImage out = Descramble::descrambleComixto(pngBytes(original), p);
            QVERIFY(!out.isNull());
            QCOMPARE(out.size(), original.size());

            QSet<QRgb> outColors;
            int moved = 0;
            for (int i = 0; i < 25; ++i) {
                const QRgb c = tileColor(out, i % 5, i / 5, T, T);
                outColors.insert(c);
                if (c != tileColor(original, i % 5, i / 5, T, T))
                    ++moved;
            }
            QCOMPARE(outColors, originalColors);  // every tile exactly once
            QVERIFY2(moved > 0, "a non-zero seed must rearrange something");

            // Deterministic: the same parameters give the same image.
            QCOMPARE(Descramble::descrambleComixto(pngBytes(original), p), out);
        }
    }

    // ── parseAsuraUrl ─────────────────────────────────────────────────────────
    void parseAsuraUrl_valid()
    {
        const QString url = QStringLiteral("/manga/asura-descramble?url=https%3A%2F%2Fasura.gg%2Fp.jpg"
                                           "&tiles=%5B1%2C0%5D&tileCols=2&tileRows=1");
        const auto p = Descramble::parseAsuraUrl(url);
        QVERIFY(p.valid);
        QCOMPARE(p.imageUrl, QStringLiteral("https://asura.gg/p.jpg"));
        QCOMPARE(p.tileCols, 2);
        QCOMPARE(p.tileRows, 1);
        QCOMPARE(p.tiles.size(), 2);
    }

    void parseAsuraUrl_notAsura()
    {
        QVERIFY(!Descramble::parseAsuraUrl(QStringLiteral("https://example.com/p.jpg")).valid);
    }

    void parseAsuraUrl_rejectsAnInconsistentGrid()
    {
        const QString base = QStringLiteral("/manga/asura-descramble?url=https%3A%2F%2Fa.test%2Fp.jpg&tiles=%5B1%2C0%5D");
        QVERIFY(!Descramble::parseAsuraUrl(base + QStringLiteral("&tileCols=0&tileRows=0")).valid);  // no grid
        QVERIFY(!Descramble::parseAsuraUrl(base).valid);                                             // grid missing
        QVERIFY(!Descramble::parseAsuraUrl(base + QStringLiteral("&tileCols=3&tileRows=1")).valid);  // 2 tiles, 3 cells
    }

    // ── reassembleAsura ───────────────────────────────────────────────────────
    void reassembleAsura_swapsTwoTiles()
    {
        QImage src(200, 100, QImage::Format_ARGB32);
        for (int y = 0; y < 100; ++y) {
            for (int x = 0; x < 200; ++x)
                src.setPixel(x, y, x < 100 ? qRgb(255, 0, 0) : qRgb(0, 0, 255));
        }
        Descramble::AsuraParams p;
        p.valid = true;
        p.imageUrl = QStringLiteral("x");
        p.tileCols = 2;
        p.tileRows = 1;
        p.tiles = QJsonArray{1, 0};  // tile 0 goes to slot 1 and tile 1 to slot 0

        const QImage out = Descramble::reassembleAsura(pngBytes(src), p);
        QVERIFY(!out.isNull());
        QCOMPARE(out.pixel(10, 10), qRgba(0, 0, 255, 255));
        QCOMPARE(out.pixel(110, 10), qRgba(255, 0, 0, 255));
    }

    void reassembleAsura_movesEveryTileToItsSlot()
    {
        constexpr int COLS = 4, ROWS = 5, T = 6, N = COLS * ROWS;
        const QImage src = coloredGrid(COLS, ROWS, T, T);

        // Reverse order: tile i belongs at slot N-1-i.
        QJsonArray tiles;
        for (int i = 0; i < N; ++i)
            tiles.append(N - 1 - i);
        Descramble::AsuraParams p;
        p.valid = true;
        p.imageUrl = QStringLiteral("x");
        p.tileCols = COLS;
        p.tileRows = ROWS;
        p.tiles = tiles;

        const QImage out = Descramble::reassembleAsura(pngBytes(src), p);
        QVERIFY(!out.isNull());
        for (int i = 0; i < N; ++i) {
            const int slot = N - 1 - i;
            QCOMPARE(tileColor(out, slot % COLS, slot / COLS, T, T), tileColor(src, i % COLS, i / COLS, T, T));
        }
    }

    void reassembleAsura_refusesFormatsItDoesNotKnow()
    {
        const QByteArray png = pngBytes(coloredGrid(2, 1, 10, 10));
        Descramble::AsuraParams p;
        p.valid = true;
        p.imageUrl = QStringLiteral("x");
        p.tileCols = 2;
        p.tileRows = 1;

        // Objects instead of integers: no guessing, the caller falls back to the original image.
        p.tiles = QJsonArray{QJsonObject{{"x", 0}}, QJsonObject{{"x", 10}}};
        QVERIFY(Descramble::reassembleAsura(png, p).isNull());

        // A slot outside the grid.
        p.tiles = QJsonArray{5, 0};
        QVERIFY(Descramble::reassembleAsura(png, p).isNull());
    }
};

QTEST_MAIN(TstDescramble)
#include "tst_descramble.moc"
