#pragma once

#include "extensions/ExtensionModels.h"
#include "extensions/JsWorker.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

#include <functional>

/// Manages all manga/novel extension scripts.
///
/// init() discovers the built-in scripts (QRC) and user-installed ones (XDG data dir). Scripts are read when a call
/// is made and run on the shared JsWorkerThread, one JavaScript context per extension.
///
/// Every call method returns immediately. The callback runs later on the thread that owns this object (the thread
/// that created it, normally the GUI thread), exactly once, never from inside the call that scheduled it.
///
/// Arguments follow the extension contract, which is positional and uses ids, not URLs:
///   search(query, page)   getPopular(page)   getLatest(page)
///   getMangaDetail(mangaId)   getPages(chapterId)   getChapterText(chapterId)
class ExtensionManager : public QObject
{
    Q_OBJECT
public:
    explicit ExtensionManager(QObject *parent = nullptr);
    ~ExtensionManager() override;

    static ExtensionManager &instance();

    /// Loads the extension list. Call once before any extension call.
    void init();

    /// Registers every `*.js` file in `directory` (files named `*.template.js` are generators, not sources, and are
    /// skipped). An id that is already registered is left alone. Returns how many extensions were added.
    int addScriptsFrom(const QString &directory, bool builtin);

    /// All registered extensions.
    QList<ExtensionInfo> extensions() const;

    /// Find by id. Returns nullptr if not found.
    const ExtensionInfo *findById(const QString &id) const;

    // ── Async call wrappers ───────────────────────────────────────────────────
    using SearchCb = std::function<void(QList<MangaPreview>, SourceError)>;
    using DetailCb = std::function<void(MangaDetail, SourceError)>;
    using PagesCb = std::function<void(QList<PageInfo>, SourceError)>;
    using ChapterTextCb = std::function<void(QString, SourceError)>;

    void search(const QString &extensionId, const QString &query, int page, const SearchCb &cb);
    void getMangaDetail(const QString &extensionId, const QString &mangaId, const DetailCb &cb);
    void getPages(const QString &extensionId, const QString &chapterId, const PagesCb &cb);
    void getChapterText(const QString &extensionId, const QString &chapterId, const ChapterTextCb &cb);
    void getPopular(const QString &extensionId, int page, const SearchCb &cb);
    void getLatest(const QString &extensionId, int page, const SearchCb &cb);

    /// The worker that runs the scripts (tests use it to install a fake network).
    JsWorker *worker() const { return m_workerThread->worker(); }

private:
    using Decoder = std::function<void(const JsResult &)>;

    QString scriptSource(const ExtensionInfo &info) const;
    void dispatch(const QString &extId, const QString &method, const QString &argsJson, const Decoder &decoder);

    static QList<MangaPreview> parsePreviews(const QVariant &val, const QString &provider);
    static MangaDetail parseDetail(const QVariant &val, const QString &provider);
    static QList<PageInfo> parsePages(const QVariant &val);
    static QString parseChapterText(const QVariant &val);

    QList<ExtensionInfo> m_extensions;
    QHash<QString, int> m_idIndex;  // id -> index in m_extensions
    JsWorkerThread *m_workerThread = nullptr;
    QHash<quint64, Decoder> m_pending;  // call id -> what to do with the result
    quint64 m_nextCallId = 0;
};
