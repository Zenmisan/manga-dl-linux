#include "extensions/ExtensionManager.h"

#include "util/AppPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaObject>
#include <QRegularExpression>

// ─── helpers ─────────────────────────────────────────────────────────────────

namespace {

// "mangadex.js" -> "mangadex"
QString extensionIdFromFilename(const QString &filename)
{
    return QFileInfo(filename).baseName().toLower();
}

QString readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("[ExtensionManager] cannot open %s", qPrintable(path));
        return {};
    }
    return QString::fromUtf8(f.readAll());
}

// A source is a novel source when its script can return chapter text. Reading it from the script (instead of a
// hard-coded id list) means user-installed extensions are classified correctly too. Both the method shorthand
// `async getChapterText(id) {` and the property form `getChapterText: async function (id) {` count.
ExtensionType typeFromSource(const QString &source)
{
    static const QRegularExpression definesChapterText(
        QStringLiteral("^\\s*(?:async\\s+)?getChapterText\\s*(?:\\(|:)"), QRegularExpression::MultilineOption);
    return definesChapterText.match(source).hasMatch() ? ExtensionType::Novel : ExtensionType::Manga;
}

QString displayNameFromId(const QString &id)
{
    if (id.isEmpty())
        return id;
    QString s = id;
    s[0] = s[0].toUpper();
    return s;  // best effort until sources carry a display name of their own
}

QString toJsonArray(const QJsonArray &args)
{
    return QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
}

SourceError errorFrom(const JsResult &r)
{
    return SourceError::fromText(r.errorText, SourceErrorKind::ParseError);
}

QString firstString(const QVariantMap &m, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QString v = m.value(QString::fromLatin1(key)).toString();
        if (!v.isEmpty())
            return v;
    }
    return {};
}

QStringList stringList(const QVariantMap &m, const char *listKey, const char *singleKey)
{
    QStringList out;
    const QVariant list = m.value(QString::fromLatin1(listKey));
    for (const QVariant &v : list.toList()) {
        const QString s = v.toString();
        if (!s.isEmpty())
            out.append(s);
    }
    if (out.isEmpty()) {
        const QString single = m.value(QString::fromLatin1(singleKey)).toString();
        if (!single.isEmpty())
            out.append(single);
    }
    return out;
}

} // namespace

// ─── ExtensionManager ────────────────────────────────────────────────────────

ExtensionManager::ExtensionManager(QObject *parent) : QObject(parent)
{
    m_workerThread = new JsWorkerThread(this);
    // One connection for every call: look the finished call up by id.
    connect(m_workerThread->worker(), &JsWorker::scriptFinished, this,
            [this](quint64 callId, JsResult result) {
                const auto it = m_pending.find(callId);
                if (it == m_pending.end())
                    return;
                const Decoder decoder = it.value();
                m_pending.erase(it);
                decoder(result);
            },
            Qt::QueuedConnection);
}

ExtensionManager::~ExtensionManager() = default;

ExtensionManager &ExtensionManager::instance()
{
    static ExtensionManager s_instance;
    return s_instance;
}

void ExtensionManager::init()
{
    m_extensions.clear();
    m_idIndex.clear();

    addScriptsFrom(QStringLiteral(":/qt/qml/MangaDl/assets/extensions"), true);
    addScriptsFrom(AppPaths::extensionsDir(), false);

    int novels = 0;
    for (const ExtensionInfo &info : std::as_const(m_extensions)) {
        if (info.type == ExtensionType::Novel)
            ++novels;
    }
    qInfo("[ExtensionManager] %lld extensions (%d manga, %d novel)", static_cast<long long>(m_extensions.size()),
          int(m_extensions.size()) - novels, novels);
}

int ExtensionManager::addScriptsFrom(const QString &directory, bool builtin)
{
    const QDir dir(directory);
    if (!dir.exists())
        return 0;

    int added = 0;
    const QStringList files = dir.entryList({QStringLiteral("*.js")}, QDir::Files, QDir::Name);
    for (const QString &fn : files) {
        if (fn.endsWith(QLatin1String(".template.js")))
            continue;  // generators for installable sources, not sources themselves
        const QString id = extensionIdFromFilename(fn);
        if (m_idIndex.contains(id))
            continue;

        ExtensionInfo info;
        info.id = id;
        info.name = displayNameFromId(id);
        info.filePath = dir.filePath(fn);
        info.type = typeFromSource(readFile(info.filePath));
        info.language = QStringLiteral("en");
        info.version = QStringLiteral("1.0");
        info.isBuiltin = builtin;
        info.isInstalled = true;
        m_idIndex.insert(info.id, int(m_extensions.size()));
        m_extensions.append(info);
        ++added;
    }
    return added;
}

QList<ExtensionInfo> ExtensionManager::extensions() const
{
    return m_extensions;
}

const ExtensionInfo *ExtensionManager::findById(const QString &id) const
{
    const auto it = m_idIndex.constFind(id);
    return it == m_idIndex.constEnd() ? nullptr : &m_extensions[it.value()];
}

QString ExtensionManager::scriptSource(const ExtensionInfo &info) const
{
    return readFile(info.filePath);
}

// ─── Parse helpers ────────────────────────────────────────────────────────────

QList<MangaPreview> ExtensionManager::parsePreviews(const QVariant &val, const QString &provider)
{
    QList<MangaPreview> out;
    for (const QVariant &item : val.toList()) {
        const QVariantMap m = item.toMap();
        MangaPreview p;
        p.title = m.value(QStringLiteral("title")).toString();
        p.coverUrl = firstString(m, {"cover_url", "cover"});
        p.url = m.value(QStringLiteral("url")).toString();
        p.id = m.value(QStringLiteral("id")).toString();
        if (p.id.isEmpty())
            p.id = p.url;
        p.provider = provider;
        if (!p.title.isEmpty())
            out.append(p);
    }
    return out;
}

MangaDetail ExtensionManager::parseDetail(const QVariant &val, const QString &provider)
{
    MangaDetail d;
    const QVariantMap m = val.toMap();
    d.id = m.value(QStringLiteral("id")).toString();
    d.title = m.value(QStringLiteral("title")).toString();
    d.coverUrl = firstString(m, {"cover_url", "cover"});
    d.url = m.value(QStringLiteral("url")).toString();
    d.description = m.value(QStringLiteral("description")).toString();
    d.status = m.value(QStringLiteral("status")).toString();
    d.provider = provider;
    d.raw = m;

    for (const QVariant &g : m.value(QStringLiteral("genres")).toList())
        d.genres.append(g.toString());
    // The scripts emit `authors: []`; the Windows app read `author`. Accept both spellings.
    d.authors = stringList(m, "authors", "author");
    d.artists = stringList(m, "artists", "artist");

    for (const QVariant &ch : m.value(QStringLiteral("chapters")).toList()) {
        const QVariantMap cm = ch.toMap();
        ChapterInfo c;
        c.id = cm.value(QStringLiteral("id")).toString();
        c.title = cm.value(QStringLiteral("title")).toString();
        c.number = cm.value(QStringLiteral("number")).toDouble();
        c.publishedAt = firstString(cm, {"published_at", "date"});
        c.scanlator = cm.value(QStringLiteral("scanlator")).toString();
        c.url = cm.value(QStringLiteral("url")).toString();
        d.chapters.append(c);
    }
    return d;
}

QList<PageInfo> ExtensionManager::parsePages(const QVariant &val)
{
    QList<PageInfo> out;
    for (const QVariant &item : val.toList()) {
        PageInfo p;
        if (item.metaType().id() == QMetaType::QString) {
            p.url = item.toString();
        } else {
            const QVariantMap m = item.toMap();
            p.url = m.value(QStringLiteral("url")).toString();
            p.width = m.value(QStringLiteral("width")).toInt();
            p.height = m.value(QStringLiteral("height")).toInt();
        }
        if (!p.url.isEmpty())
            out.append(p);
    }
    return out;
}

QString ExtensionManager::parseChapterText(const QVariant &val)
{
    if (val.metaType().id() == QMetaType::QString)
        return val.toString();
    const QVariantMap m = val.toMap();
    return m.value(QStringLiteral("content"), m.value(QStringLiteral("text"))).toString();
}

// ─── Dispatch ─────────────────────────────────────────────────────────────────

void ExtensionManager::dispatch(const QString &extId, const QString &method, const QString &argsJson,
                                const Decoder &decoder)
{
    // Failures that are known before any script runs are still reported asynchronously, so a caller never sees its
    // callback run before the call has returned.
    auto fail = [this, decoder](const QString &text) {
        JsResult r;
        r.errorText = text;
        QMetaObject::invokeMethod(this, [decoder, r] { decoder(r); }, Qt::QueuedConnection);
    };

    const ExtensionInfo *info = findById(extId);
    if (!info)
        return fail(QStringLiteral("[load] unknown extension: %1").arg(extId));
    const QString source = scriptSource(*info);
    if (source.isEmpty())
        return fail(QStringLiteral("[load] cannot read the script of %1").arg(extId));

    const quint64 callId = ++m_nextCallId;
    m_pending.insert(callId, decoder);
    QMetaObject::invokeMethod(worker(), "invokeScript", Qt::QueuedConnection, Q_ARG(quint64, callId),
                              Q_ARG(QString, extId), Q_ARG(QString, source), Q_ARG(QString, method),
                              Q_ARG(QString, argsJson));
}

// ─── Public call wrappers ─────────────────────────────────────────────────────

void ExtensionManager::search(const QString &extId, const QString &query, int page, const SearchCb &cb)
{
    dispatch(extId, QStringLiteral("search"), toJsonArray({query, qMax(1, page)}), [extId, cb](const JsResult &r) {
        if (r.ok)
            cb(parsePreviews(r.value, extId), {});
        else
            cb({}, errorFrom(r));
    });
}

void ExtensionManager::getMangaDetail(const QString &extId, const QString &mangaId, const DetailCb &cb)
{
    dispatch(extId, QStringLiteral("getMangaDetail"), toJsonArray({mangaId}), [extId, cb](const JsResult &r) {
        if (r.ok)
            cb(parseDetail(r.value, extId), {});
        else
            cb({}, errorFrom(r));
    });
}

void ExtensionManager::getPages(const QString &extId, const QString &chapterId, const PagesCb &cb)
{
    dispatch(extId, QStringLiteral("getPages"), toJsonArray({chapterId}), [cb](const JsResult &r) {
        if (r.ok)
            cb(parsePages(r.value), {});
        else
            cb({}, errorFrom(r));
    });
}

void ExtensionManager::getChapterText(const QString &extId, const QString &chapterId, const ChapterTextCb &cb)
{
    dispatch(extId, QStringLiteral("getChapterText"), toJsonArray({chapterId}), [cb](const JsResult &r) {
        if (r.ok)
            cb(parseChapterText(r.value), {});
        else
            cb({}, errorFrom(r));
    });
}

void ExtensionManager::getPopular(const QString &extId, int page, const SearchCb &cb)
{
    dispatch(extId, QStringLiteral("getPopular"), toJsonArray({qMax(1, page)}), [extId, cb](const JsResult &r) {
        if (r.ok)
            cb(parsePreviews(r.value, extId), {});
        else
            cb({}, errorFrom(r));
    });
}

void ExtensionManager::getLatest(const QString &extId, int page, const SearchCb &cb)
{
    dispatch(extId, QStringLiteral("getLatest"), toJsonArray({qMax(1, page)}), [extId, cb](const JsResult &r) {
        if (r.ok)
            cb(parsePreviews(r.value, extId), {});
        else
            cb({}, errorFrom(r));
    });
}
