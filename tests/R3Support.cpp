#include "R3Support.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDeadlineTimer>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QSaveFile>
#include <QTimer>
#include <algorithm>

QString r3CanonUrl(const QString &url)
{
    const QUrl resolved = HttpService::resolveUrl(url);
    if (!resolved.isValid() || resolved.host().isEmpty())
        return url.trimmed();
    QList<QPair<QString, QString>> items = QUrlQuery(resolved).queryItems(QUrl::FullyDecoded);
    std::sort(items.begin(), items.end());
    QUrlQuery sorted;
    for (const auto &item : items)
        sorted.addQueryItem(item.first, item.second);
    QUrl out = resolved;
    out.setQuery(sorted);
    return out.toString(QUrl::FullyEncoded);
}

QString r3FetchKey(const QString &request, const QString &method, const QString &body)
{
    return method.toUpper() + QLatin1Char('\n') + r3CanonUrl(request) + QLatin1Char('\n') + body;
}

bool r3Wait(const std::function<bool()> &cond, int msec)
{
    const QDeadlineTimer deadline(msec);
    while (!deadline.hasExpired()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (cond())
            return true;
    }
    return cond();
}

static QJsonObject stepToJson(const R3Step &s)
{
    QJsonObject o;
    o.insert(QStringLiteral("ok"), s.ok);
    o.insert(QStringLiteral("error"), s.error);
    o.insert(QStringLiteral("count"), s.count);
    o.insert(QStringLiteral("id"), s.id);
    o.insert(QStringLiteral("title"), s.title);
    return o;
}

static R3Step stepFromJson(const QJsonObject &o)
{
    R3Step s;
    s.ok = o.value(QStringLiteral("ok")).toBool();
    s.error = o.value(QStringLiteral("error")).toString();
    s.count = o.value(QStringLiteral("count")).toInt();
    s.id = o.value(QStringLiteral("id")).toString();
    s.title = o.value(QStringLiteral("title")).toString();
    return s;
}

bool r3SavePack(const R3Pack &pack, const QString &dir, QString *error)
{
    QDir d;
    if (!d.mkpath(dir)) {
        if (error)
            *error = QStringLiteral("cannot create %1").arg(dir);
        return false;
    }
    QDir out(dir);
    const QFileInfoList stale = out.entryInfoList(QDir::Files);
    for (const QFileInfo &f : stale)
        QFile::remove(f.absoluteFilePath());

    QJsonArray fetches;
    for (int i = 0; i < pack.fetches.size(); ++i) {
        const R3Fetch &f = pack.fetches[i];
        const QString file = QStringLiteral("%1.dat").arg(i + 1, 4, 10, QLatin1Char('0'));
        QFile body(out.filePath(file));
        if (!body.open(QIODevice::WriteOnly)) {
            if (error)
                *error = QStringLiteral("cannot write %1").arg(body.fileName());
            return false;
        }
        body.write(f.text.toUtf8());

        QJsonObject o;
        o.insert(QStringLiteral("file"), file);
        o.insert(QStringLiteral("request"), f.request);
        o.insert(QStringLiteral("method"), f.method);
        o.insert(QStringLiteral("body"), f.body);
        o.insert(QStringLiteral("status"), f.status);
        o.insert(QStringLiteral("finalUrl"), f.finalUrl);
        o.insert(QStringLiteral("transportError"), f.transportError);
        fetches.append(o);
    }

    QJsonObject root;
    root.insert(QStringLiteral("id"), pack.id);
    root.insert(QStringLiteral("query"), pack.query);
    root.insert(QStringLiteral("recordedAt"), pack.recordedAt);
    root.insert(QStringLiteral("novel"), pack.novel);
    root.insert(QStringLiteral("search"), stepToJson(pack.search));
    root.insert(QStringLiteral("detail"), stepToJson(pack.detail));
    root.insert(QStringLiteral("chapter"), stepToJson(pack.chapter));
    root.insert(QStringLiteral("fetches"), fetches);

    QSaveFile mf(out.filePath(QStringLiteral("manifest.json")));
    if (!mf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("cannot write manifest");
        return false;
    }
    mf.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return mf.commit();
}

bool r3LoadPack(const QString &dir, R3Pack *pack, QString *error)
{
    QFile mf(QDir(dir).filePath(QStringLiteral("manifest.json")));
    if (!mf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("no manifest in %1").arg(dir);
        return false;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(mf.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("bad manifest: %1").arg(pe.errorString());
        return false;
    }
    const QJsonObject root = doc.object();
    pack->id = root.value(QStringLiteral("id")).toString();
    pack->query = root.value(QStringLiteral("query")).toString();
    pack->recordedAt = root.value(QStringLiteral("recordedAt")).toString();
    pack->novel = root.value(QStringLiteral("novel")).toBool();
    pack->search = stepFromJson(root.value(QStringLiteral("search")).toObject());
    pack->detail = stepFromJson(root.value(QStringLiteral("detail")).toObject());
    pack->chapter = stepFromJson(root.value(QStringLiteral("chapter")).toObject());
    pack->fetches.clear();
    const QJsonArray fetches = root.value(QStringLiteral("fetches")).toArray();
    for (const QJsonValue &v : fetches) {
        const QJsonObject o = v.toObject();
        R3Fetch f;
        f.request = o.value(QStringLiteral("request")).toString();
        f.method = o.value(QStringLiteral("method")).toString(QStringLiteral("GET"));
        f.body = o.value(QStringLiteral("body")).toString();
        f.status = o.value(QStringLiteral("status")).toInt();
        f.finalUrl = o.value(QStringLiteral("finalUrl")).toString();
        f.transportError = o.value(QStringLiteral("transportError")).toString();
        QFile body(QDir(dir).filePath(o.value(QStringLiteral("file")).toString()));
        if (body.open(QIODevice::ReadOnly))
            f.text = QString::fromUtf8(body.readAll());
        pack->fetches.append(f);
    }
    return true;
}

JsWorker::FetchInterceptor r3ReplayInterceptor(const R3Pack &pack)
{
    QHash<QString, int> index;
    for (int i = 0; i < pack.fetches.size(); ++i) {
        const R3Fetch &f = pack.fetches[i];
        index.insert(r3FetchKey(f.request, f.method, f.body), i);
        index.insert(r3CanonUrl(f.request), i);
        const QString resolved = HttpService::resolveUrl(f.request).toString();
        if (!resolved.isEmpty())
            index.insert(resolved, i);
    }
    return [pack, index](const QString &url, const QVariantMap &opts, JsWorker::FakeResponse &out) {
        const QString method = opts.value(QStringLiteral("method"), QStringLiteral("GET")).toString();
        const QString body = opts.value(QStringLiteral("body")).toString();
        int i = index.value(r3FetchKey(url, method, body), -1);
        if (i < 0)
            i = index.value(r3CanonUrl(url), -1);
        if (i < 0)
            i = index.value(HttpService::resolveUrl(url).toString(), -1);
        if (i < 0) {
            out.transportError = QStringLiteral("no fixture for ") + url.left(180);
            return true;
        }
        const R3Fetch &f = pack.fetches[i];
        if (!f.transportError.isEmpty()) {
            out.transportError = f.transportError;
            return true;
        }
        out.status = f.status == 0 ? 200 : f.status;
        out.text = f.text;
        out.url = f.finalUrl.isEmpty() ? HttpService::resolveUrl(url).toString() : f.finalUrl;
        return true;
    };
}

void r3RunPipeline(ExtensionManager *mgr, const QString &extId, bool novel, const QString &query, R3Pack *pack)
{
    pack->id = extId;
    pack->query = query;
    pack->novel = novel;
    pack->recordedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    auto failStep = [](R3Step *s, const SourceError &e) {
        s->ok = false;
        s->error = e.message.isEmpty() ? QStringLiteral("empty error") : e.message;
    };

    {
        bool done = false;
        QList<MangaPreview> rows;
        SourceError err;
        mgr->search(extId, query, 1, [&](QList<MangaPreview> r, SourceError e) {
            rows = r;
            err = e;
            done = true;
        });
        if (!r3Wait([&] { return done; }, 90000)) {
            pack->search.error = QStringLiteral("search timed out");
            return;
        }
        pack->search.count = rows.size();
        if (!err.message.isEmpty()) {
            failStep(&pack->search, err);
            return;
        }
        if (rows.isEmpty()) {
            pack->search.error = QStringLiteral("no search results");
            return;
        }
        pack->search.ok = true;
        pack->search.id = rows.first().id;
        pack->search.title = rows.first().title;
    }

    {
        bool done = false;
        MangaDetail detail;
        SourceError err;
        mgr->getMangaDetail(extId, pack->search.id, [&](MangaDetail d, SourceError e) {
            detail = d;
            err = e;
            done = true;
        });
        if (!r3Wait([&] { return done; }, 90000)) {
            pack->detail.error = QStringLiteral("detail timed out");
            return;
        }
        pack->detail.count = detail.chapters.size();
        pack->detail.title = detail.title;
        pack->detail.id = detail.id.isEmpty() ? pack->search.id : detail.id;
        if (!err.message.isEmpty()) {
            failStep(&pack->detail, err);
            return;
        }
        if (detail.title.isEmpty()) {
            pack->detail.error = QStringLiteral("detail has no title");
            return;
        }
        if (detail.chapters.isEmpty()) {
            pack->detail.error = QStringLiteral("detail has no chapters");
            return;
        }
        pack->detail.ok = true;
        QString chapterId;
        for (const ChapterInfo &c : detail.chapters) {
            if (!c.id.isEmpty()) {
                chapterId = c.id;
                break;
            }
        }
        if (chapterId.isEmpty()) {
            pack->chapter.error = QStringLiteral("first chapter has no id");
            return;
        }
        pack->chapter.id = chapterId;
    }

    if (novel) {
        bool done = false;
        QString text;
        SourceError err;
        mgr->getChapterText(extId, pack->chapter.id, [&](QString t, SourceError e) {
            text = t;
            err = e;
            done = true;
        });
        if (!r3Wait([&] { return done; }, 90000)) {
            pack->chapter.error = QStringLiteral("getChapterText timed out");
            return;
        }
        pack->chapter.count = text.size();
        if (!err.message.isEmpty()) {
            failStep(&pack->chapter, err);
            return;
        }
        if (text.trimmed().size() < 20) {
            pack->chapter.error = QStringLiteral("chapter text too short (%1 chars)").arg(text.size());
            return;
        }
        pack->chapter.ok = true;
        pack->chapter.title = text.left(80).simplified();
    } else {
        bool done = false;
        QList<PageInfo> pages;
        SourceError err;
        mgr->getPages(extId, pack->chapter.id, [&](QList<PageInfo> p, SourceError e) {
            pages = p;
            err = e;
            done = true;
        });
        if (!r3Wait([&] { return done; }, 90000)) {
            pack->chapter.error = QStringLiteral("getPages timed out");
            return;
        }
        pack->chapter.count = pages.size();
        if (!err.message.isEmpty()) {
            failStep(&pack->chapter, err);
            return;
        }
        if (pages.isEmpty()) {
            pack->chapter.error = QStringLiteral("no pages");
            return;
        }
        pack->chapter.ok = true;
        pack->chapter.title = pages.first().url.left(120);
    }
}
