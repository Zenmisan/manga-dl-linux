#include "TachibkParser.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <cstring>
#include <zlib.h>

namespace mangadl {

struct ProtoField {
    enum Type { Varint, Fixed64, LengthDelimited, Fixed32 };
    Type type;
    quint64 uintVal = 0;
    QByteArray bytesVal;
};

static bool readVarint(const QByteArray &data, int &pos, quint64 &out)
{
    out = 0;
    int shift = 0;
    while (pos < data.size() && shift < 64) {
        const quint8 b = static_cast<quint8>(data.at(pos++));
        out |= (static_cast<quint64>(b & 0x7F) << shift);
        if ((b & 0x80) == 0) return true;
        shift += 7;
    }
    return false;
}

static QMultiMap<int, ProtoField> decodeMessage(const QByteArray &data)
{
    QMultiMap<int, ProtoField> fields;
    int pos = 0;
    while (pos < data.size()) {
        quint64 tag = 0;
        if (!readVarint(data, pos, tag)) break;
        const int fieldNum = static_cast<int>(tag >> 3);
        const int wireType = static_cast<int>(tag & 0x07);

        ProtoField f;
        if (wireType == 0) { // varint
            f.type = ProtoField::Varint;
            if (!readVarint(data, pos, f.uintVal)) break;
            fields.insert(fieldNum, f);
        } else if (wireType == 1) { // 64-bit
            if (pos + 8 > data.size()) break;
            f.type = ProtoField::Fixed64;
            memcpy(&f.uintVal, data.data() + pos, 8);
            pos += 8;
            fields.insert(fieldNum, f);
        } else if (wireType == 2) { // length-delimited
            quint64 len = 0;
            if (!readVarint(data, pos, len)) break;
            if (pos + static_cast<int>(len) > data.size()) break;
            f.type = ProtoField::LengthDelimited;
            f.bytesVal = data.mid(pos, static_cast<int>(len));
            pos += static_cast<int>(len);
            fields.insert(fieldNum, f);
        } else if (wireType == 5) { // 32-bit
            if (pos + 4 > data.size()) break;
            f.type = ProtoField::Fixed32;
            quint32 u32 = 0;
            memcpy(&u32, data.data() + pos, 4);
            f.uintVal = u32;
            pos += 4;
            fields.insert(fieldNum, f);
        } else {
            break;
        }
    }
    return fields;
}

static QString getStr(const QMultiMap<int, ProtoField> &fields, int num, const QString &def = QString())
{
    auto it = fields.find(num);
    if (it != fields.end() && it.value().type == ProtoField::LengthDelimited) {
        return QString::fromUtf8(it.value().bytesVal);
    }
    return def;
}

static qint64 getInt(const QMultiMap<int, ProtoField> &fields, int num, qint64 def = 0)
{
    auto it = fields.find(num);
    if (it != fields.end() && (it.value().type == ProtoField::Varint || it.value().type == ProtoField::Fixed64 || it.value().type == ProtoField::Fixed32)) {
        return static_cast<qint64>(it.value().uintVal);
    }
    return def;
}

static bool getBool(const QMultiMap<int, ProtoField> &fields, int num, bool def = false)
{
    auto it = fields.find(num);
    if (it != fields.end()) {
        return it.value().uintVal != 0;
    }
    return def;
}

static double getFloat(const QMultiMap<int, ProtoField> &fields, int num, double def = 0.0)
{
    auto it = fields.find(num);
    if (it != fields.end()) {
        const quint32 bits = static_cast<quint32>(it.value().uintVal);
        float f = 0.0f;
        memcpy(&f, &bits, 4);
        return f;
    }
    return def;
}

static QList<QByteArray> getMsgs(const QMultiMap<int, ProtoField> &fields, int num)
{
    QList<QByteArray> msgs;
    auto range = fields.equal_range(num);
    for (auto it = range.first; it != range.second; ++it) {
        if (it.value().type == ProtoField::LengthDelimited) {
            msgs.append(it.value().bytesVal);
        }
    }
    return msgs;
}

static QList<int> getRepeatedInts(const QMultiMap<int, ProtoField> &fields, int num)
{
    QList<int> ints;
    auto range = fields.equal_range(num);
    for (auto it = range.first; it != range.second; ++it) {
        if (it.value().type == ProtoField::Varint) {
            ints.append(static_cast<int>(it.value().uintVal));
        } else if (it.value().type == ProtoField::LengthDelimited) {
            // Packed repeated varints
            int p = 0;
            const QByteArray &b = it.value().bytesVal;
            while (p < b.size()) {
                quint64 v = 0;
                if (!readVarint(b, p, v)) break;
                ints.append(static_cast<int>(v));
            }
        }
    }
    return ints;
}

static BackupChapter parseChapter(const QByteArray &raw)
{
    const auto f = decodeMessage(raw);
    BackupChapter ch;
    ch.url = getStr(f, 1);
    ch.name = getStr(f, 2);
    ch.scanlator = getStr(f, 3);
    ch.read = getBool(f, 4);
    ch.bookmark = getBool(f, 5);
    ch.lastPageRead = static_cast<int>(getInt(f, 6));
    ch.chapterNumber = getFloat(f, 9);
    ch.sourceOrder = static_cast<int>(getInt(f, 10));
    return ch;
}

static BackupHistoryEntry parseHistory(const QByteArray &raw)
{
    const auto f = decodeMessage(raw);
    BackupHistoryEntry h;
    h.url = getStr(f, 1);
    h.lastRead = getInt(f, 2);
    return h;
}

static BackupTracking parseTracking(const QByteArray &raw)
{
    const auto f = decodeMessage(raw);
    BackupTracking t;
    t.syncId = static_cast<int>(getInt(f, 1));
    qint64 mediaId = getInt(f, 100);
    if (mediaId == 0) mediaId = getInt(f, 3);
    t.mediaId = mediaId;
    t.trackingUrl = getStr(f, 4);
    t.title = getStr(f, 5);
    t.lastChapterRead = getFloat(f, 6);
    t.score = getFloat(f, 8);
    t.status = static_cast<int>(getInt(f, 9));
    return t;
}

static BackupCategoryEntry parseCategory(const QByteArray &raw)
{
    const auto f = decodeMessage(raw);
    BackupCategoryEntry c;
    c.name = getStr(f, 1);
    c.order = static_cast<int>(getInt(f, 2));
    return c;
}

static BackupSourceEntry parseSource(const QByteArray &raw)
{
    const auto f = decodeMessage(raw);
    BackupSourceEntry s;
    s.name = getStr(f, 1);
    s.sourceId = getInt(f, 2);
    return s;
}

static QPair<BackupManga, QList<int>> parseMangaRaw(const QByteArray &raw)
{
    const auto f = decodeMessage(raw);
    BackupManga m;
    m.source = getInt(f, 1);
    m.url = getStr(f, 2);
    m.title = getStr(f, 3);
    m.artist = getStr(f, 4);
    m.author = getStr(f, 5);
    m.description = getStr(f, 6);
    m.thumbnailUrl = getStr(f, 9);
    m.favorite = getBool(f, 100, true);

    for (const auto &rawCh : getMsgs(f, 16)) {
        m.chapters.append(parseChapter(rawCh));
    }
    for (const auto &rawTr : getMsgs(f, 18)) {
        m.tracking.append(parseTracking(rawTr));
    }
    for (const auto &rawHi : getMsgs(f, 104)) {
        m.history.append(parseHistory(rawHi));
    }

    const QList<int> catIds = getRepeatedInts(f, 17);
    return qMakePair(m, catIds);
}

// =========================================================================
// TachibkParser Implementation
// =========================================================================

bool TachibkParser::isGzip(const QByteArray &data)
{
    return data.size() >= 2 && static_cast<quint8>(data[0]) == 0x1f && static_cast<quint8>(data[1]) == 0x8b;
}

bool TachibkParser::looksLikeJson(const QByteArray &data)
{
    for (char c : data) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        return c == '{';
    }
    return false;
}

QByteArray TachibkParser::gunzip(const QByteArray &data)
{
    if (data.isEmpty()) return QByteArray();

    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    strm.avail_in = static_cast<uInt>(data.size());

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
        return QByteArray();
    }

    QByteArray out;
    char buffer[16384];
    int ret = Z_OK;
    while (ret == Z_OK) {
        strm.next_out = reinterpret_cast<Bytef *>(buffer);
        strm.avail_out = sizeof(buffer);
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&strm);
            return QByteArray();
        }
        out.append(buffer, sizeof(buffer) - strm.avail_out);
    }

    inflateEnd(&strm);
    return out;
}

TachiyomiBackupResult TachibkParser::decodeTachibk(const QByteArray &data)
{
    QByteArray payload = data;
    if (isGzip(payload)) {
        payload = gunzip(payload);
    }

    const auto root = decodeMessage(payload);
    TachiyomiBackupResult result;

    for (const auto &rawCat : getMsgs(root, 2)) {
        result.categories.append(parseCategory(rawCat));
    }
    for (const auto &rawSrc : getMsgs(root, 101)) {
        result.sources.append(parseSource(rawSrc));
    }

    QMap<int, QString> catMap;
    for (int i = 0; i < result.categories.size(); ++i) {
        catMap[i + 1] = result.categories[i].name;
    }

    QMap<qint64, QString> sourceMap;
    for (const auto &s : result.sources) {
        if (s.sourceId != 0) sourceMap[s.sourceId] = s.name;
    }

    for (const auto &rawManga : getMsgs(root, 1)) {
        auto pair = parseMangaRaw(rawManga);
        BackupManga manga = pair.first;
        for (int cid : pair.second) {
            if (catMap.contains(cid)) manga.categoryNames.append(catMap.value(cid));
        }
        if (sourceMap.contains(manga.source)) {
            manga.sourceName = sourceMap.value(manga.source);
        }
        result.manga.append(manga);
    }

    return result;
}

TachiyomiBackupResult TachibkParser::parseJsonBackup(const QByteArray &data)
{
    TachiyomiBackupResult result;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return result;

    const QJsonObject root = doc.object();

    // Categories
    QJsonArray catsArray;
    if (root.contains(QStringLiteral("backupCategories"))) catsArray = root.value(QStringLiteral("backupCategories")).toArray();
    else if (root.contains(QStringLiteral("categories"))) catsArray = root.value(QStringLiteral("categories")).toArray();

    int catIdx = 0;
    QMap<int, QString> catMap;
    for (const auto &cVal : catsArray) {
        BackupCategoryEntry entry;
        if (cVal.isObject()) {
            const QJsonObject o = cVal.toObject();
            entry.name = o.value(QStringLiteral("name")).toString();
            entry.order = o.value(QStringLiteral("order")).toInt(catIdx);
        } else {
            entry.name = cVal.toString();
            entry.order = catIdx;
        }
        result.categories.append(entry);
        catMap[catIdx + 1] = entry.name;
        catIdx++;
    }

    // Sources
    QJsonArray srcArray;
    if (root.contains(QStringLiteral("backupSources"))) srcArray = root.value(QStringLiteral("backupSources")).toArray();
    else if (root.contains(QStringLiteral("sources"))) srcArray = root.value(QStringLiteral("sources")).toArray();

    QMap<qint64, QString> sourceMap;
    for (const auto &sVal : srcArray) {
        BackupSourceEntry entry;
        if (sVal.isObject()) {
            const QJsonObject o = sVal.toObject();
            entry.name = o.value(QStringLiteral("name")).toString();
            qint64 sid = o.value(QStringLiteral("sourceId")).toInteger();
            if (sid == 0) sid = o.value(QStringLiteral("source_id")).toInteger();
            entry.sourceId = sid;
        } else {
            entry.name = sVal.toString();
        }
        result.sources.append(entry);
        if (entry.sourceId != 0) sourceMap[entry.sourceId] = entry.name;
    }

    // Manga
    QJsonArray mangaArray;
    if (root.contains(QStringLiteral("backupManga"))) mangaArray = root.value(QStringLiteral("backupManga")).toArray();
    else if (root.contains(QStringLiteral("manga"))) mangaArray = root.value(QStringLiteral("manga")).toArray();
    else if (root.contains(QStringLiteral("library"))) mangaArray = root.value(QStringLiteral("library")).toArray();

    for (const auto &mVal : mangaArray) {
        if (!mVal.isObject()) continue;
        const QJsonObject m = mVal.toObject();
        BackupManga bm;
        bm.title = m.value(QStringLiteral("title")).toString();
        bm.url = m.value(QStringLiteral("url")).toString();
        bm.artist = m.value(QStringLiteral("artist")).toString();
        bm.author = m.value(QStringLiteral("author")).toString();
        bm.description = m.value(QStringLiteral("description")).toString();
        bm.thumbnailUrl = m.value(QStringLiteral("thumbnailUrl")).toString(m.value(QStringLiteral("thumbnail_url")).toString());
        bm.favorite = m.value(QStringLiteral("favorite")).toBool(true);
        bm.source = m.value(QStringLiteral("source")).toInteger();
        bm.sourceName = sourceMap.value(bm.source);

        // Categories
        QJsonArray catIds;
        if (m.contains(QStringLiteral("categories"))) catIds = m.value(QStringLiteral("categories")).toArray();
        else if (m.contains(QStringLiteral("category_ids"))) catIds = m.value(QStringLiteral("category_ids")).toArray();

        for (const auto &cid : catIds) {
            if (cid.isString()) bm.categoryNames.append(cid.toString());
            else if (cid.isDouble() && catMap.contains(cid.toInt())) bm.categoryNames.append(catMap.value(cid.toInt()));
        }

        // Chapters
        const QJsonArray chArray = m.value(QStringLiteral("chapters")).toArray();
        for (const auto &chVal : chArray) {
            if (!chVal.isObject()) continue;
            const QJsonObject ch = chVal.toObject();
            BackupChapter bc;
            bc.url = ch.value(QStringLiteral("url")).toString();
            bc.name = ch.value(QStringLiteral("name")).toString();
            bc.scanlator = ch.value(QStringLiteral("scanlator")).toString();
            bc.read = ch.value(QStringLiteral("read")).toBool();
            bc.bookmark = ch.value(QStringLiteral("bookmark")).toBool();
            bc.lastPageRead = ch.value(QStringLiteral("lastPageRead")).toInt(ch.value(QStringLiteral("last_page_read")).toInt(0));
            bc.chapterNumber = ch.value(QStringLiteral("chapterNumber")).toDouble(ch.value(QStringLiteral("chapter_number")).toDouble(0.0));
            bc.sourceOrder = ch.value(QStringLiteral("sourceOrder")).toInt(ch.value(QStringLiteral("source_order")).toInt(0));
            bm.chapters.append(bc);
        }

        // Tracking
        const QJsonArray trArray = m.value(QStringLiteral("tracking")).toArray();
        for (const auto &trVal : trArray) {
            if (!trVal.isObject()) continue;
            const QJsonObject tr = trVal.toObject();
            BackupTracking bt;
            bt.syncId = tr.value(QStringLiteral("syncId")).toInt(tr.value(QStringLiteral("sync_id")).toInt(0));
            bt.mediaId = tr.value(QStringLiteral("mediaId")).toInteger(tr.value(QStringLiteral("media_id")).toInteger(0));
            bt.title = tr.value(QStringLiteral("title")).toString();
            bt.lastChapterRead = tr.value(QStringLiteral("lastChapterRead")).toDouble(tr.value(QStringLiteral("last_chapter_read")).toDouble(0.0));
            bt.score = tr.value(QStringLiteral("score")).toDouble(0.0);
            bt.status = tr.value(QStringLiteral("status")).toInt(0);
            bt.trackingUrl = tr.value(QStringLiteral("trackingUrl")).toString(tr.value(QStringLiteral("tracking_url")).toString());
            bm.tracking.append(bt);
        }

        result.manga.append(bm);
    }

    return result;
}

TachiyomiBackupResult TachibkParser::parseTachiyomiBackup(const QByteArray &data, const QString &filename)
{
    const bool isJson = filename.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive) ||
                        (!isGzip(data) && looksLikeJson(data));

    if (isJson) {
        try {
            return parseJsonBackup(data);
        } catch (const std::exception &ex) {
            qDebug("[TachibkParser] JSON parse fallback to protobuf: %s", ex.what());
        } catch (...) {
            qDebug("[TachibkParser] JSON parse fallback to protobuf");
        }
    }

    return decodeTachibk(data);
}

// ── provider / id resolution ─────────────────────────────────────────────

QString TachibkParser::resolveProvider(const QString &sourceName, const QString &mangaUrl)
{
    const QString s = sourceName.trimmed().toLower();
    const QString u = mangaUrl.trimmed().toLower();
    const QString target = s + QLatin1Char(' ') + u;

    static const QList<QPair<QString, QString>> keywords = {
        { QStringLiteral("mangadex"), QStringLiteral("mangadex") },
        { QStringLiteral("asura"), QStringLiteral("asurascans") },
        { QStringLiteral("omega"), QStringLiteral("omegascans") },
        { QStringLiteral("flame"), QStringLiteral("flamescans") },
        { QStringLiteral("katana"), QStringLiteral("mangakatana") },
        { QStringLiteral("kakalot"), QStringLiteral("mangakakalot") },
        { QStringLiteral("manganato"), QStringLiteral("manganato") },
        { QStringLiteral("nato"), QStringLiteral("manganato") },
        { QStringLiteral("batoto"), QStringLiteral("bato") },
        { QStringLiteral("bato"), QStringLiteral("bato") },
        { QStringLiteral("pill"), QStringLiteral("mangapill") },
        { QStringLiteral("tcb"), QStringLiteral("tcbscans") },
        { QStringLiteral("mangahere"), QStringLiteral("mangahere") },
        { QStringLiteral("webtoon"), QStringLiteral("webtoons") },
        { QStringLiteral("mangaplus"), QStringLiteral("mangaplus") },
        { QStringLiteral("komga"), QStringLiteral("komga") },
        { QStringLiteral("suwayomi"), QStringLiteral("suwayomi") },
        { QStringLiteral("royalroad"), QStringLiteral("royalroad") },
        { QStringLiteral("scribblehub"), QStringLiteral("scribblehub") },
        { QStringLiteral("novelbin"), QStringLiteral("novelbin") },
        { QStringLiteral("novelfull"), QStringLiteral("novelfull") },
        { QStringLiteral("wuxiaworld"), QStringLiteral("wuxiaworld") },
        { QStringLiteral("lightnovelworld"), QStringLiteral("lightnovelworld") }
    };

    for (const auto &item : keywords) {
        if (target.contains(item.first)) {
            return item.second;
        }
    }

    static const QRegularExpression slugRegex(QStringLiteral("[^a-z0-9]"));
    QString slug = s;
    slug.remove(slugRegex);
    return slug.isEmpty() ? QStringLiteral("tachiyomi") : slug;
}

QString TachibkParser::cleanMangaId(const QString &url, const QString &provider)
{
    if (url.isEmpty()) return QStringLiteral("unknown");
    QString u = url.trimmed();

    const int schemeIdx = u.indexOf(QStringLiteral("://"));
    if (schemeIdx >= 0) {
        u = u.mid(schemeIdx + 3);
        const int slashIdx = u.indexOf(QLatin1Char('/'));
        if (slashIdx >= 0) {
            u = u.mid(slashIdx);
        }
    }

    u = u.section(QLatin1Char('?'), 0, 0).section(QLatin1Char('#'), 0, 0);
    while (u.startsWith(QLatin1Char('/'))) u.remove(0, 1);
    while (u.endsWith(QLatin1Char('/'))) u.chop(1);

    if (provider == QStringLiteral("mangadex")) {
        static const QRegularExpression uuidRegex(QStringLiteral(R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})"), QRegularExpression::CaseInsensitiveOption);
        const auto match = uuidRegex.match(u);
        if (match.hasMatch()) {
            return match.captured(0).toLower();
        }
    }

    for (const QString &prefix : { QStringLiteral("manga/"), QStringLiteral("series/"), QStringLiteral("title/"), QStringLiteral("comic/") }) {
        if (u.startsWith(prefix)) {
            u.remove(0, prefix.length());
            break;
        }
    }

    while (u.startsWith(QLatin1Char('/'))) u.remove(0, 1);
    while (u.endsWith(QLatin1Char('/'))) u.chop(1);
    return u.isEmpty() ? QStringLiteral("unknown") : u;
}

QString TachibkParser::cleanChapterId(const QString &url, double chapterNumber)
{
    if (url.isEmpty()) {
        return chapterNumber != 0.0 ? QString::number(chapterNumber) : QStringLiteral("ch-1");
    }

    QString u = url.trimmed().section(QLatin1Char('?'), 0, 0).section(QLatin1Char('#'), 0, 0);
    while (u.endsWith(QLatin1Char('/'))) u.chop(1);

    const int lastSlash = u.lastIndexOf(QLatin1Char('/'));
    if (lastSlash >= 0) {
        const QString last = u.mid(lastSlash + 1);
        if (!last.isEmpty()) return last;
    }

    return !u.isEmpty() ? u : (chapterNumber != 0.0 ? QString::number(chapterNumber) : QStringLiteral("ch-1"));
}

} // namespace mangadl
