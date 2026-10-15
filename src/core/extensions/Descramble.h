#pragma once

#include <QByteArray>
#include <QImage>
#include <QJsonArray>
#include <QString>

/// Native image descrambling to replace browser Canvas-based descramble in comixto.js
/// and the tile-reassembly used by asurascans.js.
///
/// Page URLs from those extensions arrive as:
///   comixto://img?url=<enc>&seed=<s>&algo=<a>&encSeed=<es>&encLen=<el>&encAlgo=<ea>
///   /manga/asura-descramble?url=<enc>&tiles=<json>&tileCols=<n>&tileRows=<n>
///
/// Call parseComixtoUrl / parseAsuraUrl to check whether a URL is a descramble request,
/// then call the appropriate descramble function.

namespace Descramble {

// ─── Comixto ──────────────────────────────────────────────────────────────────

struct ComixtoParams {
    QString imageUrl;
    qint32  seed      = 0;
    QString algo;         // "", "3" = xorshift; otherwise LCG
    qint32  encSeed   = 0;
    int     encLen    = 0;
    QString encAlgo;      // "", "2" = xorshift; otherwise LCG
    bool    valid     = false;
};

/// Parse a comixto:// URL.  Returns {valid=false} if not a comixto URL.
ComixtoParams parseComixtoUrl(const QString &url);

/// Descramble raw image bytes.
/// @param imageBytes  Raw JPEG/PNG/WEBP bytes (already fetched from imageUrl)
/// @param p           Parsed parameters from parseComixtoUrl()
/// @return             Reassembled image, or a null QImage on failure.
QImage descrambleComixto(const QByteArray &imageBytes, const ComixtoParams &p);

// ─── Asura tile reassembly ────────────────────────────────────────────────────

struct AsuraParams {
    QString imageUrl;
    QJsonArray tiles;    // flat list of integers, one per tile, row by row: the tile at position i of the
                         // scrambled image belongs at position tiles[i] of the result
    int tileCols = 0;
    int tileRows = 0;
    bool valid = false;   // true only when the grid is known and tiles.size() == tileCols * tileRows
};

/// Parse a /manga/asura-descramble?... URL.
AsuraParams parseAsuraUrl(const QString &url);

/// Reassemble tiles.
/// @param imageBytes  Raw image bytes of the scrambled source image
/// @param p           Parsed parameters from parseAsuraUrl()
/// @return             Reassembled image, or null on failure.
QImage reassembleAsura(const QByteArray &imageBytes, const AsuraParams &p);

} // namespace Descramble
