#include "BackupService.h"
#include "TachibkParser.h"
#include "src/core/util/FileNaming.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

namespace mangadl {

BackupService::BackupService(MangaDatabase *db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

MangaDlBackup BackupService::createBackup() const
{
    MangaDlBackup backup;
    if (!m_db) return backup;

    backup.version = QStringLiteral("1.0");
    backup.app = QStringLiteral("manga-dl");
    backup.exportedAt = QDateTime::currentMSecsSinceEpoch();

    const auto library = m_db->getLibrary();
    const auto categories = m_db->getCategories();
    const auto libraryCategories = m_db->getAllLibraryCategories();
    const auto progressList = m_db->getAllProgress();
    const auto trackerBinds = m_db->getAllTrackerBinds();

    QMap<QString, QString> catIdToName;
    for (const auto &c : categories) {
        catIdToName[c.id] = c.name;
        backup.categories.append(c.name);
    }

    for (const auto &mapping : libraryCategories) {
        const QString name = catIdToName.value(mapping.categoryId, mapping.categoryId);
        if (!backup.mangaCategories[mapping.libraryId].contains(name)) {
            backup.mangaCategories[mapping.libraryId].append(name);
        }
    }

    for (const auto &item : library) {
        const QString libKey = QStringLiteral("%1/%2").arg(item.provider, item.mangaId);
        BackupMangaEntry entry;
        entry.id = item.mangaId;
        entry.title = item.title;
        entry.coverUrl = item.coverUrl;
        entry.provider = item.provider;
        entry.url = item.url;
        entry.type = item.type.isEmpty() ? QStringLiteral("manga") : item.type;
        entry.addedAt = item.addedAt;
        entry.totalChapters = item.totalChapters;
        entry.categories = backup.mangaCategories.value(libKey);
        backup.library.append(entry);
    }

    for (const auto &p : progressList) {
        BackupProgressEntry entry;
        entry.mangaId = p.mangaId;
        entry.chapterId = p.chapterId;
        entry.provider = p.provider;
        entry.page = p.page;
        entry.completed = (p.completed == 1);
        entry.chapterNumber = p.chapterNumber;
        entry.readAt = p.readAt;
        backup.progress.append(entry);
    }

    for (const auto &t : trackerBinds) {
        BackupTrackerBindEntry entry;
        entry.provider = t.provider;
        entry.mangaId = t.mangaId;
        entry.tracker = t.tracker;
        entry.remoteId = t.remoteId;
        entry.remoteTitle = t.remoteTitle;
        entry.lastChapterRead = t.lastChapterRead;
        backup.trackerBinds.append(entry);
    }

    return backup;
}

QByteArray BackupService::exportBackupBytes() const
{
    const MangaDlBackup backup = createBackup();

    QJsonObject root;
    root[QStringLiteral("version")] = backup.version;
    root[QStringLiteral("app")] = backup.app;
    root[QStringLiteral("exportedAt")] = backup.exportedAt;

    QJsonArray catsArray;
    for (const auto &c : backup.categories) catsArray.append(c);
    root[QStringLiteral("categories")] = catsArray;

    QJsonObject mangaCatsObj;
    for (auto it = backup.mangaCategories.begin(); it != backup.mangaCategories.end(); ++it) {
        QJsonArray arr;
        for (const auto &c : it.value()) arr.append(c);
        mangaCatsObj[it.key()] = arr;
    }
    root[QStringLiteral("mangaCategories")] = mangaCatsObj;

    QJsonArray libArray;
    for (const auto &m : backup.library) {
        QJsonObject o;
        o[QStringLiteral("id")] = m.id;
        o[QStringLiteral("title")] = m.title;
        o[QStringLiteral("coverUrl")] = m.coverUrl;
        o[QStringLiteral("provider")] = m.provider;
        o[QStringLiteral("url")] = m.url;
        o[QStringLiteral("type")] = m.type;
        o[QStringLiteral("addedAt")] = m.addedAt;
        o[QStringLiteral("totalChapters")] = m.totalChapters;
        QJsonArray cArr;
        for (const auto &c : m.categories) cArr.append(c);
        o[QStringLiteral("categories")] = cArr;
        libArray.append(o);
    }
    root[QStringLiteral("library")] = libArray;

    QJsonArray progArray;
    for (const auto &p : backup.progress) {
        QJsonObject o;
        o[QStringLiteral("mangaId")] = p.mangaId;
        o[QStringLiteral("chapterId")] = p.chapterId;
        o[QStringLiteral("provider")] = p.provider;
        o[QStringLiteral("page")] = p.page;
        o[QStringLiteral("completed")] = p.completed;
        o[QStringLiteral("chapterNumber")] = p.chapterNumber;
        o[QStringLiteral("readAt")] = p.readAt;
        progArray.append(o);
    }
    root[QStringLiteral("progress")] = progArray;

    QJsonArray bindsArray;
    for (const auto &t : backup.trackerBinds) {
        QJsonObject o;
        o[QStringLiteral("provider")] = t.provider;
        o[QStringLiteral("mangaId")] = t.mangaId;
        o[QStringLiteral("tracker")] = t.tracker;
        o[QStringLiteral("remoteId")] = t.remoteId;
        o[QStringLiteral("remoteTitle")] = t.remoteTitle;
        o[QStringLiteral("lastChapterRead")] = t.lastChapterRead;
        bindsArray.append(o);
    }
    root[QStringLiteral("trackerBinds")] = bindsArray;

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool BackupService::exportBackupFile(const QString &filePath) const
{
    const QByteArray bytes = exportBackupBytes();
    if (bytes.isEmpty()) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const qint64 written = file.write(bytes);
    file.close();
    return written == bytes.size();
}

bool BackupService::isMangadlBackup(const QByteArray &data)
{
    if (data.size() < 2) return false;
    if (static_cast<quint8>(data[0]) == 0x1f && static_cast<quint8>(data[1]) == 0x8b) return false;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject root = doc.object();
        if (root.contains(QStringLiteral("app")) && root.value(QStringLiteral("app")).toString().compare(QStringLiteral("manga-dl"), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

MangadlRestoreSummary BackupService::restoreMangadlBackup(const MangaDlBackup &backup)
{
    MangadlRestoreSummary summary;
    if (!m_db) return summary;

    QMap<QString, QString> categoryNameToId;
    int sortOrder = 0;
    for (const auto &catName : backup.categories) {
        const QString trimmed = catName.trimmed();
        if (trimmed.isEmpty()) continue;
        const QString id = FileNaming::slugify(trimmed);
        m_db->addCategory(id, trimmed, sortOrder++);
        categoryNameToId[trimmed.toLower()] = id;
        summary.categoriesRestored++;
    }

    for (const auto &m : backup.library) {
        const QString title = m.title.trimmed();
        if (title.isEmpty()) continue;

        LibraryEntity e;
        e.id = QStringLiteral("%1/%2").arg(m.provider, m.id);
        e.provider = m.provider;
        e.mangaId = m.id;
        e.title = title;
        e.coverUrl = m.coverUrl;
        e.url = m.url;
        e.type = m.type.isEmpty() ? QStringLiteral("manga") : m.type;
        e.totalChapters = m.totalChapters;
        e.addedAt = m.addedAt > 0 ? m.addedAt : QDateTime::currentSecsSinceEpoch();
        e.updatedAt = QDateTime::currentSecsSinceEpoch();

        m_db->addToLibrary(e);
        summary.mangaRestored++;

        QStringList associatedCatIds;
        for (const auto &c : m.categories) {
            const QString lower = c.trimmed().toLower();
            if (categoryNameToId.contains(lower)) {
                associatedCatIds.append(categoryNameToId.value(lower));
            } else if (!lower.isEmpty()) {
                const QString newId = FileNaming::slugify(c.trimmed());
                categoryNameToId[lower] = newId;
                associatedCatIds.append(newId);
            }
        }
        if (!associatedCatIds.isEmpty()) {
            m_db->setMangaCategories(m.provider, m.id, associatedCatIds);
        }
    }

    for (const auto &p : backup.progress) {
        if (p.provider.isEmpty() || p.mangaId.isEmpty() || p.chapterId.isEmpty()) continue;
        const int page = p.page > 0 ? p.page : 1;
        m_db->saveProgress(p.provider, p.mangaId, p.chapterId, p.chapterNumber, page, p.completed);
        summary.progressRestored++;
    }

    for (const auto &t : backup.trackerBinds) {
        if (t.provider.isEmpty() || t.mangaId.isEmpty() || t.tracker.isEmpty()) continue;
        TrackerBindEntity bind;
        bind.provider = t.provider;
        bind.mangaId = t.mangaId;
        bind.tracker = t.tracker;
        bind.remoteId = t.remoteId;
        bind.remoteTitle = t.remoteTitle;
        bind.lastChapterRead = t.lastChapterRead;
        m_db->saveTrackerBind(bind);
        summary.trackerBindsRestored++;
    }

    return summary;
}

MangadlRestoreSummary BackupService::restoreTachiyomiBackup(const TachiyomiBackupResult &backup)
{
    MangadlRestoreSummary summary;
    if (!m_db) return summary;

    QMap<QString, QString> categoryNameToId;
    for (const auto &cat : backup.categories) {
        const QString trimmed = cat.name.trimmed();
        if (trimmed.isEmpty()) continue;
        const QString id = FileNaming::slugify(trimmed);
        m_db->addCategory(id, trimmed, cat.order);
        categoryNameToId[trimmed.toLower()] = id;
        summary.categoriesRestored++;
    }

    for (const auto &manga : backup.manga) {
        const QString title = manga.title.trimmed();
        if (title.isEmpty()) continue;

        const QString provider = TachibkParser::resolveProvider(manga.sourceName, manga.url);
        const QString mangaId = TachibkParser::cleanMangaId(manga.url, provider);

        LibraryEntity e;
        e.id = QStringLiteral("%1/%2").arg(provider, mangaId);
        e.provider = provider;
        e.mangaId = mangaId;
        e.title = title;
        e.coverUrl = manga.thumbnailUrl;
        e.url = manga.url;
        e.type = QStringLiteral("manga");
        e.totalChapters = manga.chapters.size();
        e.addedAt = QDateTime::currentSecsSinceEpoch();
        e.updatedAt = QDateTime::currentSecsSinceEpoch();

        m_db->addToLibrary(e);
        summary.mangaRestored++;

        QStringList mangaCatIds;
        for (const auto &cName : manga.categoryNames) {
            const QString lower = cName.trimmed().toLower();
            if (categoryNameToId.contains(lower)) {
                mangaCatIds.append(categoryNameToId.value(lower));
            }
        }
        if (!mangaCatIds.isEmpty()) {
            m_db->setMangaCategories(provider, mangaId, mangaCatIds);
        }

        for (const auto &ch : manga.chapters) {
            if (!ch.read) continue;
            const QString chId = TachibkParser::cleanChapterId(ch.url, ch.chapterNumber);
            const int page = ch.lastPageRead > 0 ? ch.lastPageRead : 1;
            m_db->saveProgress(provider, mangaId, chId, ch.chapterNumber, page, true);
            summary.progressRestored++;
        }

        for (const auto &tr : manga.tracking) {
            QString trackerName;
            if (tr.syncId == 1) trackerName = QStringLiteral("MyAnimeList");
            else if (tr.syncId == 2) trackerName = QStringLiteral("AniList");

            if (trackerName.isEmpty() || tr.mediaId == 0) continue;

            TrackerBindEntity bind;
            bind.provider = provider;
            bind.mangaId = mangaId;
            bind.tracker = trackerName;
            bind.remoteId = static_cast<int>(tr.mediaId);
            bind.remoteTitle = tr.title;
            bind.lastChapterRead = tr.lastChapterRead;
            m_db->saveTrackerBind(bind);
            summary.trackerBindsRestored++;
        }
    }

    return summary;
}

MangadlRestoreSummary BackupService::restoreBackup(const QByteArray &data)
{
    if (isMangadlBackup(data)) {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject root = doc.object();
            MangaDlBackup backup;
            backup.version = root.value(QStringLiteral("version")).toString();
            backup.app = root.value(QStringLiteral("app")).toString();
            backup.exportedAt = root.value(QStringLiteral("exportedAt")).toInteger();

            const QJsonArray cats = root.value(QStringLiteral("categories")).toArray();
            for (const auto &c : cats) backup.categories.append(c.toString());

            const QJsonObject mCats = root.value(QStringLiteral("mangaCategories")).toObject();
            for (auto it = mCats.begin(); it != mCats.end(); ++it) {
                QStringList list;
                for (const auto &v : it.value().toArray()) list.append(v.toString());
                backup.mangaCategories[it.key()] = list;
            }

            const QJsonArray lib = root.value(QStringLiteral("library")).toArray();
            for (const auto &val : lib) {
                const QJsonObject o = val.toObject();
                BackupMangaEntry e;
                e.id = o.value(QStringLiteral("id")).toString();
                e.title = o.value(QStringLiteral("title")).toString();
                e.coverUrl = o.value(QStringLiteral("coverUrl")).toString();
                e.provider = o.value(QStringLiteral("provider")).toString();
                e.url = o.value(QStringLiteral("url")).toString();
                e.type = o.value(QStringLiteral("type")).toString();
                e.addedAt = o.value(QStringLiteral("addedAt")).toInteger();
                e.totalChapters = o.value(QStringLiteral("totalChapters")).toInt();
                for (const auto &c : o.value(QStringLiteral("categories")).toArray()) {
                    e.categories.append(c.toString());
                }
                backup.library.append(e);
            }

            const QJsonArray prog = root.value(QStringLiteral("progress")).toArray();
            for (const auto &val : prog) {
                const QJsonObject o = val.toObject();
                BackupProgressEntry e;
                e.mangaId = o.value(QStringLiteral("mangaId")).toString();
                e.chapterId = o.value(QStringLiteral("chapterId")).toString();
                e.provider = o.value(QStringLiteral("provider")).toString();
                e.page = o.value(QStringLiteral("page")).toInt(1);
                e.completed = o.value(QStringLiteral("completed")).toBool();
                e.chapterNumber = o.value(QStringLiteral("chapterNumber")).toDouble();
                e.readAt = o.value(QStringLiteral("readAt")).toInteger();
                backup.progress.append(e);
            }

            const QJsonArray binds = root.value(QStringLiteral("trackerBinds")).toArray();
            for (const auto &val : binds) {
                const QJsonObject o = val.toObject();
                BackupTrackerBindEntry e;
                e.provider = o.value(QStringLiteral("provider")).toString();
                e.mangaId = o.value(QStringLiteral("mangaId")).toString();
                e.tracker = o.value(QStringLiteral("tracker")).toString();
                e.remoteId = o.value(QStringLiteral("remoteId")).toInt();
                e.remoteTitle = o.value(QStringLiteral("remoteTitle")).toString();
                e.lastChapterRead = o.value(QStringLiteral("lastChapterRead")).toDouble();
                backup.trackerBinds.append(e);
            }

            const auto summary = restoreMangadlBackup(backup);
            emit backupRestored(summary);
            return summary;
        }
    }

    // Otherwise, decode as Tachiyomi backup (either .tachibk or tachiyomi json)
    const TachiyomiBackupResult tachiResult = TachibkParser::parseTachiyomiBackup(data);
    const auto summary = restoreTachiyomiBackup(tachiResult);
    emit backupRestored(summary);
    return summary;
}

MangadlRestoreSummary BackupService::restoreBackupFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit backupFailed(QStringLiteral("Could not open file: ") + filePath);
        return MangadlRestoreSummary{};
    }
    const QByteArray data = file.readAll();
    file.close();

    return restoreBackup(data);
}

} // namespace mangadl
