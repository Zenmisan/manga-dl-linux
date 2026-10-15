// Descramble.cpp - native QImage descrambling for comix.to and Asura Scans pages.
//
// Comixto (ported from comixto.js, which does this with browser canvas APIs):
//   1. Optional XOR decryption of the first `encLen` bytes:
//      - encAlgo "2": xorshift32. Four variants are tried in order (seed|1 low byte, seed low byte, seed|1 high
//        byte, LCG) and the first whose result starts with an image signature wins.
//      - anything else: LCG stream (multiplier 1000005, increment 1234567891), key = state >>> 24.
//   2. Optional 5x5 tile unscramble: a Fisher-Yates permutation driven by an LCG (multiplier 1664525,
//      increment 1013904223) or, for algo "3", by xorshift32. All arithmetic is 32-bit signed like the JS.
//
// Asura Scans: the page gives `tiles`, a flat list of integers with one entry per tile, row by row. The tile at
// position i of the scrambled image belongs at position tiles[i] of the result (same convention as the Android
// app's TileDescramblerInterceptor).

#include "extensions/Descramble.h"

#include <QImage>
#include <QJsonDocument>
#include <QPainter>
#include <QUrl>
#include <QUrlQuery>
#include <QVector>

namespace Descramble {

// ─── integer helpers (32-bit signed, matching the int32() wrapping in the JS) ─────────────────────────────

static inline qint32 i32(qint64 n) { return qint32(quint32(quint64(n) & 0xFFFFFFFFull)); }

static qint32 lcgNext(qint32 state, qint32 mult, qint32 inc)
{
    return i32(qint64(state) * mult + inc);
}

static qint32 xorshift32(qint32 s)
{
    s = i32(s ^ i32(qint32(quint32(s) << 13)));
    s = i32(s ^ (qint32(quint32(s) >> 17)));
    s = i32(s ^ i32(qint32(quint32(s) << 5)));
    return s;
}

static constexpr qint32 kEncMult = 1000005;
static constexpr qint32 kEncInc = 1234567891;
static constexpr qint32 kLcgMult = 1664525;
static constexpr qint32 kLcgInc = 1013904223;

// Seeds arrive as decimal text and may be negative or above 2^31; the JS wraps them with int32().
static qint32 seedFromText(const QString &text)
{
    bool ok = false;
    const qint64 n = text.toLongLong(&ok);
    return ok ? i32(n) : 0;
}

// ─── tile permutation ─────────────────────────────────────────────────────────────────────────────────────

// Fisher-Yates shuffle using the LCG, then inverted.
static QVector<int> buildOrderLcg(qint32 seed, int n)
{
    QVector<int> arr(n);
    for (int k = 0; k < n; ++k)
        arr[k] = k;
    qint32 state = seed;
    for (int k = n - 1; k >= 1; --k) {
        state = lcgNext(state, kLcgMult, kLcgInc);
        const int j = int(quint32(state) % uint(k + 1));
        std::swap(arr[k], arr[j]);
    }
    QVector<int> inv(n);
    for (int k = 0; k < n; ++k)
        inv[arr[k]] = k;
    return inv;
}

// Fisher-Yates shuffle using xorshift32, then inverted.
static QVector<int> buildOrderXorshift(qint32 seed, int n)
{
    QVector<int> arr(n);
    for (int k = 0; k < n; ++k)
        arr[k] = k;
    qint32 state = i32(seed | 1);  // xorshift must not start at 0
    for (int k = n - 1; k >= 1; --k) {
        state = xorshift32(state);
        const int j = int(quint32(state) % uint(k + 1));
        std::swap(arr[k], arr[j]);
    }
    QVector<int> inv(n);
    for (int k = 0; k < n; ++k)
        inv[arr[k]] = k;
    return inv;
}

// ─── XOR decode ───────────────────────────────────────────────────────────────────────────────────────────

static QByteArray xorLcg(QByteArray bytes, qint32 seed, int length)
{
    const int limit = qMin(int(bytes.size()), length);
    qint32 state = seed;
    for (int i = 0; i < limit; ++i) {
        state = lcgNext(state, kEncMult, kEncInc);
        bytes[i] = char(uchar(bytes[i]) ^ uchar(quint32(state) >> 24));
    }
    return bytes;
}

static QByteArray xorXorshift(QByteArray bytes, qint32 seed, int length, bool highByte)
{
    const int limit = qMin(int(bytes.size()), length);
    qint32 state = seed;
    for (int i = 0; i < limit; ++i) {
        state = xorshift32(state);
        const uchar key = highByte ? uchar(quint32(state) >> 24) : uchar(quint32(state) & 0xFF);
        bytes[i] = char(uchar(bytes[i]) ^ key);
    }
    return bytes;
}

static bool hasImageSignature(const QByteArray &b)
{
    if (b.size() < 12)
        return false;
    const uchar *d = reinterpret_cast<const uchar *>(b.constData());
    if (d[0] == 0xFF && d[1] == 0xD8)
        return true;  // JPEG
    if (d[0] == 0x89 && d[1] == 0x50 && d[2] == 0x4E && d[3] == 0x47)
        return true;  // PNG
    if (d[0] == 0x52 && d[1] == 0x49 && d[2] == 0x46 && d[3] == 0x46 && d[8] == 0x57 && d[9] == 0x45 && d[10] == 0x42
        && d[11] == 0x50)
        return true;  // WEBP
    return false;
}

// ─── Comixto ──────────────────────────────────────────────────────────────────────────────────────────────

ComixtoParams parseComixtoUrl(const QString &url)
{
    ComixtoParams p;
    if (!url.startsWith(QLatin1String("comixto://img?")))
        return p;
    const QUrlQuery q(url.mid(int(qstrlen("comixto://img?"))));
    // FullyDecoded exactly once: a literal "%20" inside the image address must stay "%20".
    auto value = [&q](const char *key) {
        return q.queryItemValue(QString::fromLatin1(key), QUrl::FullyDecoded);
    };
    p.imageUrl = value("url");
    p.seed = seedFromText(value("seed"));
    p.algo = value("algo");
    p.encSeed = seedFromText(value("encSeed"));
    p.encLen = value("encLen").toInt();
    p.encAlgo = value("encAlgo");
    p.valid = !p.imageUrl.isEmpty();
    return p;
}

QImage descrambleComixto(const QByteArray &imageBytes, const ComixtoParams &p)
{
    if (!p.valid || imageBytes.isEmpty())
        return {};

    QByteArray bytes = imageBytes;

    // Step 1: XOR decryption of the header.
    if (p.encSeed != 0 && p.encLen > 0) {
        if (p.encAlgo == QLatin1String("2")) {
            const qint32 s32 = p.encSeed;
            const QByteArray candidates[4] = {
                xorXorshift(bytes, i32(s32 | 1), p.encLen, false),
                xorXorshift(bytes, s32, p.encLen, false),
                xorXorshift(bytes, i32(s32 | 1), p.encLen, true),
                xorLcg(bytes, s32, p.encLen),
            };
            bool found = false;
            for (const QByteArray &c : candidates) {
                if (hasImageSignature(c)) {
                    bytes = c;
                    found = true;
                    break;
                }
            }
            if (!found)
                bytes = candidates[0];
        } else {
            bytes = xorLcg(bytes, p.encSeed, p.encLen);
        }
    }

    QImage src;
    if (!src.loadFromData(bytes))
        return {};  // not a decodable image (WebP needs Qt's imageformats plugin)

    // Step 2: unscramble the 5x5 grid.
    if (p.seed == 0)
        return src;

    constexpr int COLS = 5, ROWS = 5, N = COLS * ROWS;
    const int w = src.width(), h = src.height();
    const int tileW = w / COLS, tileH = h / ROWS;
    if (tileW <= 0 || tileH <= 0)
        return src;

    const QVector<int> order = (p.algo == QLatin1String("3")) ? buildOrderXorshift(p.seed, N)
                                                              : buildOrderLcg(p.seed, N);
    QImage out(w, h, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    for (int dst = 0; dst < N; ++dst) {
        const int srcIdx = order[dst];
        painter.drawImage(QPoint((dst % COLS) * tileW, (dst / COLS) * tileH), src,
                          QRect((srcIdx % COLS) * tileW, (srcIdx / COLS) * tileH, tileW, tileH));
    }
    painter.end();
    return out;
}

// ─── Asura tile reassembly ────────────────────────────────────────────────────────────────────────────────

AsuraParams parseAsuraUrl(const QString &url)
{
    AsuraParams p;
    if (!url.startsWith(QLatin1String("/manga/asura-descramble?")))
        return p;
    const QUrlQuery q(url.mid(int(qstrlen("/manga/asura-descramble?"))));
    auto value = [&q](const char *key) {
        return q.queryItemValue(QString::fromLatin1(key), QUrl::FullyDecoded);
    };
    p.imageUrl = value("url");
    p.tileCols = value("tileCols").toInt();
    p.tileRows = value("tileRows").toInt();
    const QJsonDocument doc = QJsonDocument::fromJson(value("tiles").toUtf8());
    if (doc.isArray())
        p.tiles = doc.array();
    // The grid must be known and the list must have exactly one entry per tile.
    p.valid = !p.imageUrl.isEmpty() && p.tileCols > 0 && p.tileRows > 0 && p.tileCols <= 64 && p.tileRows <= 64
        && p.tiles.size() == p.tileCols * p.tileRows;
    return p;
}

QImage reassembleAsura(const QByteArray &imageBytes, const AsuraParams &p)
{
    if (!p.valid || imageBytes.isEmpty())
        return {};

    const int count = p.tileCols * p.tileRows;
    QVector<int> destination(count);
    for (int i = 0; i < count; ++i) {
        const QJsonValue v = p.tiles.at(i);
        const int d = v.isDouble() ? v.toInt(-1) : -1;
        if (d < 0 || d >= count)
            return {};  // not the format we know: let the caller fall back to the original image
        destination[i] = d;
    }

    QImage src;
    if (!src.loadFromData(imageBytes))
        return {};

    const int w = src.width(), h = src.height();
    const int tileW = w / p.tileCols, tileH = h / p.tileRows;
    if (tileW <= 0 || tileH <= 0)
        return {};

    QImage out(w, h, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    for (int srcIdx = 0; srcIdx < count; ++srcIdx) {
        const int dstIdx = destination[srcIdx];
        painter.drawImage(QPoint((dstIdx % p.tileCols) * tileW, (dstIdx / p.tileCols) * tileH), src,
                          QRect((srcIdx % p.tileCols) * tileW, (srcIdx / p.tileCols) * tileH, tileW, tileH));
    }
    painter.end();
    return out;
}

} // namespace Descramble
