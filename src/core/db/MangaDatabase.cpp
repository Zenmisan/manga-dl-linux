#include "MangaDatabase.h"
#include "util/AppPaths.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QThread>

#include <atomic>
#include <map>

namespace mangadl {

static MangaDatabase *s_instance = nullptr;
static std::atomic<quint64> s_connCounter{1};
static thread_local std::map<const MangaDatabase*, QString> s_threadConnections;

MangaDatabase *MangaDatabase::instance()
{
    if (!s_instance) {
        s_instance = new MangaDatabase();
        s_instance->initialize();
    }
    return s_instance;
}

void MangaDatabase::setInstance(MangaDatabase *customInstance)
{
    s_instance = customInstance;
}

MangaDatabase::MangaDatabase(const QString &dbPath, QObject *parent)
    : QObject(parent)
    , m_dbPath(dbPath)
{
}

MangaDatabase::~MangaDatabase()
{
    close();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void MangaDatabase::close()
{
    QMutexLocker lock(&m_connLock);
    for (const QString &connName : m_activeConnections) {
        if (QSqlDatabase::contains(connName)) {
            {
                auto db = QSqlDatabase::database(connName, false);
                if (db.isOpen()) {
                    db.close();
                }
            }
            QSqlDatabase::removeDatabase(connName);
        }
    }
    m_activeConnections.clear();
    s_threadConnections.erase(this);
    m_initialized = false;
}

QSqlDatabase MangaDatabase::getThreadConnection() const
{
    auto it = s_threadConnections.find(this);
    if (it != s_threadConnections.end()) {
        const QString &connName = it->second;
        if (QSqlDatabase::contains(connName)) {
            auto db = QSqlDatabase::database(connName, false);
            if (db.isOpen()) {
                return db;
            }
        }
    }

    const QString connName = QStringLiteral("mangadl_db_%1_%2")
        .arg(reinterpret_cast<quintptr>(this))
        .arg(s_connCounter.fetch_add(1));

    s_threadConnections[this] = connName;

    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: Failed to open SQLite DB at %1: %2")
            .arg(m_dbPath, db.lastError().text());
        return db;
    }

    // Configure PRAGMAs
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
    q.exec(QStringLiteral("PRAGMA journal_mode = WAL;"));
    q.exec(QStringLiteral("PRAGMA synchronous = NORMAL;"));
    q.exec(QStringLiteral("PRAGMA busy_timeout = 5000;"));

    {
        QMutexLocker lock(&m_connLock);
        m_activeConnections.insert(connName);
    }
    return db;
}

bool MangaDatabase::initialize()
{
    if (m_initialized) {
        return true;
    }

    if (m_dbPath.isEmpty()) {
        m_dbPath = AppPaths::databasePath();
    }

    // In-memory sqlite paths (e.g. ":memory:") do not require a directory.
    if (!m_dbPath.startsWith(QStringLiteral(":memory:"))) {
        QFileInfo fi(m_dbPath);
        QDir().mkpath(fi.absolutePath());
    }

    auto db = getThreadConnection();
    if (!db.isOpen()) {
        return false;
    }

    if (!runMigrations(db)) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: Migrations failed for %1").arg(m_dbPath);
        return false;
    }

    m_initialized = true;
    return true;
}

bool MangaDatabase::runMigrations(QSqlDatabase &db)
{
    QMutexLocker locker(&m_writeLock);
    QSqlQuery q(db);

    if (!q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "  version INTEGER PRIMARY KEY,"
        "  applied_at INTEGER NOT NULL"
        ");"))) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: schema_migrations error: %1").arg(q.lastError().text());
        return false;
    }

    int currentVersion = 0;
    if (q.exec(QStringLiteral("SELECT COALESCE(MAX(version), 0) FROM schema_migrations;")) && q.next()) {
        currentVersion = q.value(0).toInt();
    }

    if (currentVersion < 1) {
        if (!applyMigration1(db)) {
            return false;
        }
    }

    return true;
}

bool MangaDatabase::applyMigration1(QSqlDatabase &db)
{
    db.transaction();
    QSqlQuery q(db);

    const char *ddlStatements[] = {
        // 1. library
        "CREATE TABLE IF NOT EXISTS library ("
        "  id TEXT PRIMARY KEY,"
        "  provider TEXT NOT NULL,"
        "  manga_id TEXT NOT NULL,"
        "  title TEXT NOT NULL,"
        "  cover_url TEXT,"
        "  url TEXT,"
        "  type TEXT NOT NULL DEFAULT 'manga',"
        "  total_chapters INTEGER NOT NULL DEFAULT 0,"
        "  added_at INTEGER NOT NULL DEFAULT 0,"
        "  updated_at INTEGER NOT NULL DEFAULT 0"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_library_updated ON library(updated_at DESC);",
        "CREATE INDEX IF NOT EXISTS idx_library_provider_manga ON library(provider, manga_id);",

        // 2. reading_progress
        "CREATE TABLE IF NOT EXISTS reading_progress ("
        "  id TEXT PRIMARY KEY,"
        "  provider TEXT NOT NULL,"
        "  manga_id TEXT NOT NULL,"
        "  chapter_id TEXT NOT NULL,"
        "  chapter_number REAL NOT NULL DEFAULT 0.0,"
        "  page INTEGER NOT NULL DEFAULT 0,"
        "  completed INTEGER NOT NULL DEFAULT 0,"
        "  read_at INTEGER NOT NULL DEFAULT 0"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_progress_manga ON reading_progress(provider, manga_id);",
        "CREATE INDEX IF NOT EXISTS idx_progress_read_at ON reading_progress(read_at DESC);",

        // 3. downloads
        "CREATE TABLE IF NOT EXISTS downloads ("
        "  id TEXT PRIMARY KEY,"
        "  provider TEXT NOT NULL,"
        "  manga_id TEXT NOT NULL,"
        "  manga_title TEXT NOT NULL,"
        "  chapter_id TEXT NOT NULL,"
        "  chapter_title TEXT,"
        "  chapter_number REAL NOT NULL DEFAULT 0.0,"
        "  cbz_path TEXT,"
        "  status TEXT NOT NULL DEFAULT 'queued',"
        "  progress INTEGER NOT NULL DEFAULT 0,"
        "  total_pages INTEGER NOT NULL DEFAULT 0,"
        "  queued_at INTEGER NOT NULL DEFAULT 0,"
        "  completed_at INTEGER"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_downloads_status ON downloads(status);",
        "CREATE INDEX IF NOT EXISTS idx_downloads_queued ON downloads(queued_at DESC);",

        // 4. categories
        "CREATE TABLE IF NOT EXISTS categories ("
        "  id TEXT PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  sort_order INTEGER NOT NULL DEFAULT 0"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_categories_order ON categories(sort_order ASC);",

        // 5. library_categories
        "CREATE TABLE IF NOT EXISTS library_categories ("
        "  library_id TEXT NOT NULL,"
        "  category_id TEXT NOT NULL,"
        "  PRIMARY KEY (library_id, category_id)"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_lib_cat ON library_categories(library_id, category_id);",
        "CREATE INDEX IF NOT EXISTS idx_cat_lib ON library_categories(category_id, library_id);",

        // 6. history
        "CREATE TABLE IF NOT EXISTS history ("
        "  id TEXT PRIMARY KEY,"
        "  provider TEXT NOT NULL,"
        "  manga_id TEXT NOT NULL,"
        "  manga_title TEXT NOT NULL,"
        "  chapter_id TEXT NOT NULL,"
        "  chapter_title TEXT,"
        "  cover_url TEXT,"
        "  read_at INTEGER NOT NULL DEFAULT 0"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_history_read_at ON history(read_at DESC);",

        // 7. tracker_binds
        "CREATE TABLE IF NOT EXISTS tracker_binds ("
        "  id TEXT PRIMARY KEY,"
        "  provider TEXT NOT NULL,"
        "  manga_id TEXT NOT NULL,"
        "  tracker TEXT NOT NULL,"
        "  remote_id INTEGER NOT NULL DEFAULT 0,"
        "  remote_title TEXT,"
        "  last_chapter_read REAL NOT NULL DEFAULT 0.0,"
        "  updated_at INTEGER NOT NULL DEFAULT 0"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_trackers_manga ON tracker_binds(provider, manga_id);",

        // 8. new_chapters
        "CREATE TABLE IF NOT EXISTS new_chapters ("
        "  id TEXT PRIMARY KEY,"
        "  manga_id TEXT NOT NULL,"
        "  manga_title TEXT NOT NULL,"
        "  cover_url TEXT,"
        "  provider TEXT NOT NULL,"
        "  chapter_id TEXT NOT NULL,"
        "  chapter_title TEXT NOT NULL DEFAULT '',"
        "  chapter_number REAL NOT NULL DEFAULT 0.0,"
        "  detected_at INTEGER NOT NULL DEFAULT 0"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_new_chapters_detected ON new_chapters(detected_at DESC);",
        nullptr
    };

    for (int i = 0; ddlStatements[i] != nullptr; ++i) {
        if (!q.exec(QString::fromUtf8(ddlStatements[i]))) {
            qCritical().noquote() << QStringLiteral("MangaDatabase: Migration 1 DDL failed: %1 -> %2")
                .arg(ddlStatements[i], q.lastError().text());
            db.rollback();
            return false;
        }
    }

    // Default categories if categories table is empty
    q.exec(QStringLiteral("SELECT COUNT(*) FROM categories;"));
    if (q.next() && q.value(0).toInt() == 0) {
        q.prepare(QStringLiteral("INSERT INTO categories (id, name, sort_order) VALUES (?, ?, ?);"));
        const struct { const char *id; const char *name; int order; } defaults[] = {
            {"reading", "Reading", 0},
            {"plan_to_read", "Plan to read", 1},
            {"completed", "Completed", 2},
            {"local_files", "Local files", 3}
        };
        for (const auto &d : defaults) {
            q.addBindValue(QString::fromUtf8(d.id));
            q.addBindValue(QString::fromUtf8(d.name));
            q.addBindValue(d.order);
            if (!q.exec()) {
                qCritical().noquote() << QStringLiteral("MangaDatabase: default category error: %1").arg(q.lastError().text());
                db.rollback();
                return false;
            }
        }
    }

    // Record migration version 1
    q.prepare(QStringLiteral("INSERT INTO schema_migrations (version, applied_at) VALUES (1, ?);"));
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!q.exec()) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: failed to record migration 1: %1").arg(q.lastError().text());
        db.rollback();
        return false;
    }

    db.commit();
    qInfo().noquote() << QStringLiteral("MangaDatabase: Migration 1 applied successfully.");
    return true;
}

// -----------------------------------------------------------------------------
// Library
// -----------------------------------------------------------------------------

QList<LibraryEntity> MangaDatabase::getLibrary() const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.exec(QStringLiteral(
        "SELECT id, provider, manga_id, title, cover_url, url, type, total_chapters, added_at, updated_at "
        "FROM library ORDER BY updated_at DESC;"));

    QList<LibraryEntity> list;
    while (q.next()) {
        LibraryEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.title = q.value(3).toString();
        e.coverUrl = q.value(4).toString();
        e.url = q.value(5).toString();
        e.type = q.value(6).toString();
        e.totalChapters = q.value(7).toInt();
        e.addedAt = q.value(8).toLongLong();
        e.updatedAt = q.value(9).toLongLong();
        list.append(e);
    }
    return list;
}

std::optional<LibraryEntity> MangaDatabase::getLibraryItem(const QString &provider, const QString &mangaId) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, provider, manga_id, title, cover_url, url, type, total_chapters, added_at, updated_at "
        "FROM library WHERE provider = ? AND manga_id = ?;"));
    q.addBindValue(provider);
    q.addBindValue(mangaId);

    if (q.exec() && q.next()) {
        LibraryEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.title = q.value(3).toString();
        e.coverUrl = q.value(4).toString();
        e.url = q.value(5).toString();
        e.type = q.value(6).toString();
        e.totalChapters = q.value(7).toInt();
        e.addedAt = q.value(8).toLongLong();
        e.updatedAt = q.value(9).toLongLong();
        return e;
    }
    return std::nullopt;
}

bool MangaDatabase::isInLibrary(const QString &provider, const QString &mangaId) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT 1 FROM library WHERE provider = ? AND manga_id = ? LIMIT 1;"));
    q.addBindValue(provider);
    q.addBindValue(mangaId);
    return q.exec() && q.next();
}

bool MangaDatabase::addToLibrary(const LibraryEntity &item)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const QString id = item.id.isEmpty() ? (item.provider + QStringLiteral("/") + item.mangaId) : item.id;
    const qint64 addedAt = item.addedAt == 0 ? now : item.addedAt;
    const qint64 updatedAt = item.updatedAt == 0 ? now : item.updatedAt;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO library (id, provider, manga_id, title, cover_url, url, type, total_chapters, added_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  provider = excluded.provider, "
        "  manga_id = excluded.manga_id, "
        "  title = excluded.title, "
        "  cover_url = excluded.cover_url, "
        "  url = excluded.url, "
        "  type = excluded.type, "
        "  total_chapters = excluded.total_chapters, "
        "  updated_at = excluded.updated_at;"));

    q.addBindValue(id);
    q.addBindValue(item.provider);
    q.addBindValue(item.mangaId);
    q.addBindValue(item.title);
    q.addBindValue(item.coverUrl.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : item.coverUrl);
    q.addBindValue(item.url.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : item.url);
    q.addBindValue(item.type.isEmpty() ? QStringLiteral("manga") : item.type);
    q.addBindValue(item.totalChapters);
    q.addBindValue(addedAt);
    q.addBindValue(updatedAt);

    if (!q.exec()) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: addToLibrary error: %1").arg(q.lastError().text());
        return false;
    }

    emit libraryChanged();
    emit libraryItemChanged(item.provider, item.mangaId);
    return true;
}

bool MangaDatabase::removeFromLibrary(const QString &provider, const QString &mangaId)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const QString libId = provider + QStringLiteral("/") + mangaId;

    db.transaction();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM library WHERE provider = ? AND manga_id = ?;"));
    q.addBindValue(provider);
    q.addBindValue(mangaId);
    if (!q.exec()) {
        db.rollback();
        return false;
    }

    q.prepare(QStringLiteral("DELETE FROM library_categories WHERE library_id = ?;"));
    q.addBindValue(libId);
    q.exec();

    db.commit();
    emit libraryChanged();
    emit libraryItemChanged(provider, mangaId);
    return true;
}

bool MangaDatabase::updateTotalChapters(const QString &provider, const QString &mangaId, int totalChapters)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE library SET total_chapters = ?, updated_at = ? WHERE provider = ? AND manga_id = ?;"));
    q.addBindValue(totalChapters);
    q.addBindValue(now);
    q.addBindValue(provider);
    q.addBindValue(mangaId);

    if (!q.exec()) {
        return false;
    }

    emit libraryChanged();
    emit libraryItemChanged(provider, mangaId);
    return true;
}

int MangaDatabase::getCompletedChaptersCount(const QString &provider, const QString &mangaId) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM reading_progress WHERE provider = ? AND manga_id = ? AND completed = 1;"));
    q.addBindValue(provider);
    q.addBindValue(mangaId);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

int MangaDatabase::getUnreadCount(const QString &provider, const QString &mangaId) const
{
    auto item = getLibraryItem(provider, mangaId);
    if (!item || item->totalChapters <= 0) {
        return 0;
    }
    const int completed = getCompletedChaptersCount(provider, mangaId);
    return std::max(0, item->totalChapters - completed);
}

bool MangaDatabase::migrateManga(const QString &oldProvider, const QString &oldMangaId,
                                const QString &newProvider, const QString &newMangaId,
                                const QString &newTitle, const QString &newCoverUrl)
{
    auto oldItem = getLibraryItem(oldProvider, oldMangaId);
    if (!oldItem) {
        return false;
    }

    const QStringList cats = getMangaCategories(oldProvider, oldMangaId);
    const QList<ProgressEntity> progress = getMangaProgress(oldProvider, oldMangaId);
    const QList<TrackerBindEntity> binds = getTrackerBinds(oldProvider, oldMangaId);

    LibraryEntity newEntry;
    newEntry.provider = newProvider;
    newEntry.mangaId = newMangaId;
    newEntry.title = newTitle.isEmpty() ? oldItem->title : newTitle;
    newEntry.coverUrl = newCoverUrl.isEmpty() ? oldItem->coverUrl : newCoverUrl;
    newEntry.url = oldItem->url;
    newEntry.type = oldItem->type;
    newEntry.totalChapters = oldItem->totalChapters;
    newEntry.addedAt = oldItem->addedAt;
    newEntry.updatedAt = QDateTime::currentMSecsSinceEpoch();

    if (!addToLibrary(newEntry)) {
        return false;
    }

    if (!cats.isEmpty()) {
        setMangaCategories(newProvider, newMangaId, cats);
    }

    for (const auto &p : progress) {
        saveProgress(newProvider, newMangaId, p.chapterId, p.chapterNumber, p.page, p.completed == 1);
    }

    {
        QMutexLocker locker(&m_writeLock);
        auto db = getThreadConnection();
        QSqlQuery q(db);
        q.prepare(QStringLiteral("DELETE FROM reading_progress WHERE provider = ? AND manga_id = ?;"));
        q.addBindValue(oldProvider);
        q.addBindValue(oldMangaId);
        q.exec();
    }

    for (const auto &b : binds) {
        TrackerBindEntity newBind = b;
        newBind.id.clear();
        newBind.provider = newProvider;
        newBind.mangaId = newMangaId;
        saveTrackerBind(newBind);
        deleteTrackerBind(oldProvider, oldMangaId, b.tracker);
    }

    // Update history rows referencing old provider/mangaId
    {
        QMutexLocker locker(&m_writeLock);
        auto db = getThreadConnection();
        QSqlQuery q(db);
        q.prepare(QStringLiteral("UPDATE history SET provider = ?, manga_id = ?, manga_title = ?, cover_url = ? WHERE provider = ? AND manga_id = ?;"));
        q.addBindValue(newProvider);
        q.addBindValue(newMangaId);
        q.addBindValue(newEntry.title);
        q.addBindValue(newEntry.coverUrl);
        q.addBindValue(oldProvider);
        q.addBindValue(oldMangaId);
        q.exec();
    }

    removeFromLibrary(oldProvider, oldMangaId);
    return true;
}

// -----------------------------------------------------------------------------
// Reading Progress
// -----------------------------------------------------------------------------

std::optional<ProgressEntity> MangaDatabase::getProgress(const QString &provider, const QString &mangaId, const QString &chapterId) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    const QString id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, chapterId);
    q.prepare(QStringLiteral(
        "SELECT id, provider, manga_id, chapter_id, chapter_number, page, completed, read_at "
        "FROM reading_progress WHERE id = ?;"));
    q.addBindValue(id);

    if (q.exec() && q.next()) {
        ProgressEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.chapterId = q.value(3).toString();
        e.chapterNumber = q.value(4).toDouble();
        e.page = q.value(5).toInt();
        e.completed = q.value(6).toInt();
        e.readAt = q.value(7).toLongLong();
        return e;
    }
    return std::nullopt;
}

QList<ProgressEntity> MangaDatabase::getMangaProgress(const QString &provider, const QString &mangaId) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, provider, manga_id, chapter_id, chapter_number, page, completed, read_at "
        "FROM reading_progress WHERE provider = ? AND manga_id = ? ORDER BY chapter_number ASC;"));
    q.addBindValue(provider);
    q.addBindValue(mangaId);

    QList<ProgressEntity> list;
    if (q.exec()) {
        while (q.next()) {
            ProgressEntity e;
            e.id = q.value(0).toString();
            e.provider = q.value(1).toString();
            e.mangaId = q.value(2).toString();
            e.chapterId = q.value(3).toString();
            e.chapterNumber = q.value(4).toDouble();
            e.page = q.value(5).toInt();
            e.completed = q.value(6).toInt();
            e.readAt = q.value(7).toLongLong();
            list.append(e);
        }
    }
    return list;
}

bool MangaDatabase::saveProgress(const QString &provider, const QString &mangaId, const QString &chapterId,
                                 double chapterNumber, int page, bool completed)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const QString id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, chapterId);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO reading_progress (id, provider, manga_id, chapter_id, chapter_number, page, completed, read_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  chapter_number = excluded.chapter_number, "
        "  page = excluded.page, "
        "  completed = excluded.completed, "
        "  read_at = excluded.read_at;"));

    q.addBindValue(id);
    q.addBindValue(provider);
    q.addBindValue(mangaId);
    q.addBindValue(chapterId);
    q.addBindValue(chapterNumber);
    q.addBindValue(page);
    q.addBindValue(completed ? 1 : 0);
    q.addBindValue(now);

    if (!q.exec()) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: saveProgress error: %1").arg(q.lastError().text());
        return false;
    }

    emit progressChanged(provider, mangaId, chapterId);
    return true;
}

QList<ProgressEntity> MangaDatabase::getAllProgress() const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.exec(QStringLiteral("SELECT id, provider, manga_id, chapter_id, chapter_number, page, completed, read_at FROM reading_progress;"));

    QList<ProgressEntity> list;
    while (q.next()) {
        ProgressEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.chapterId = q.value(3).toString();
        e.chapterNumber = q.value(4).toDouble();
        e.page = q.value(5).toInt();
        e.completed = q.value(6).toInt();
        e.readAt = q.value(7).toLongLong();
        list.append(e);
    }
    return list;
}

QList<ProgressEntity> MangaDatabase::getAllCompletedProgress() const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.exec(QStringLiteral("SELECT id, provider, manga_id, chapter_id, chapter_number, page, completed, read_at FROM reading_progress WHERE completed = 1;"));

    QList<ProgressEntity> list;
    while (q.next()) {
        ProgressEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.chapterId = q.value(3).toString();
        e.chapterNumber = q.value(4).toDouble();
        e.page = q.value(5).toInt();
        e.completed = q.value(6).toInt();
        e.readAt = q.value(7).toLongLong();
        list.append(e);
    }
    return list;
}

// -----------------------------------------------------------------------------
// History
// -----------------------------------------------------------------------------

QList<HistoryEntity> MangaDatabase::getHistory(int limit) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, provider, manga_id, manga_title, chapter_id, chapter_title, cover_url, read_at "
        "FROM history ORDER BY read_at DESC LIMIT ?;"));
    q.addBindValue(limit);

    QList<HistoryEntity> list;
    if (q.exec()) {
        while (q.next()) {
            HistoryEntity e;
            e.id = q.value(0).toString();
            e.provider = q.value(1).toString();
            e.mangaId = q.value(2).toString();
            e.mangaTitle = q.value(3).toString();
            e.chapterId = q.value(4).toString();
            e.chapterTitle = q.value(5).toString();
            e.coverUrl = q.value(6).toString();
            e.readAt = q.value(7).toLongLong();
            list.append(e);
        }
    }
    return list;
}

bool MangaDatabase::recordHistory(const QString &provider, const QString &mangaId, const QString &mangaTitle,
                                  const QString &chapterId, const QString &chapterTitle, const QString &coverUrl)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const QString id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, chapterId);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO history (id, provider, manga_id, manga_title, chapter_id, chapter_title, cover_url, read_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  manga_title = excluded.manga_title, "
        "  chapter_title = excluded.chapter_title, "
        "  cover_url = excluded.cover_url, "
        "  read_at = excluded.read_at;"));

    q.addBindValue(id);
    q.addBindValue(provider);
    q.addBindValue(mangaId);
    q.addBindValue(mangaTitle);
    q.addBindValue(chapterId);
    q.addBindValue(chapterTitle.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : chapterTitle);
    q.addBindValue(coverUrl.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : coverUrl);
    q.addBindValue(now);

    if (!q.exec()) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: recordHistory error: %1").arg(q.lastError().text());
        return false;
    }

    emit historyChanged();
    return true;
}

bool MangaDatabase::removeFromHistory(const QString &provider, const QString &mangaId, const QString &chapterId)
{
    const QString id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, chapterId);
    return removeFromHistoryById(id);
}

bool MangaDatabase::removeFromHistoryById(const QString &id)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM history WHERE id = ?;"));
    q.addBindValue(id);
    if (!q.exec()) {
        return false;
    }
    emit historyChanged();
    return true;
}

bool MangaDatabase::clearHistory()
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("DELETE FROM history;"))) {
        return false;
    }
    emit historyChanged();
    return true;
}

// -----------------------------------------------------------------------------
// Downloads
// -----------------------------------------------------------------------------

QList<DownloadEntity> MangaDatabase::getDownloads() const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.exec(QStringLiteral(
        "SELECT id, provider, manga_id, manga_title, chapter_id, chapter_title, chapter_number, "
        "cbz_path, status, progress, total_pages, queued_at, completed_at "
        "FROM downloads ORDER BY queued_at DESC;"));

    QList<DownloadEntity> list;
    while (q.next()) {
        DownloadEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.mangaTitle = q.value(3).toString();
        e.chapterId = q.value(4).toString();
        e.chapterTitle = q.value(5).toString();
        e.chapterNumber = q.value(6).toDouble();
        e.cbzPath = q.value(7).toString();
        e.status = q.value(8).toString();
        e.progress = q.value(9).toInt();
        e.totalPages = q.value(10).toInt();
        e.queuedAt = q.value(11).toLongLong();
        e.completedAt = q.value(12).toLongLong();
        list.append(e);
    }
    return list;
}

std::optional<DownloadEntity> MangaDatabase::getDownload(const QString &provider, const QString &mangaId, const QString &chapterId) const
{
    const QString id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, chapterId);
    return getDownloadById(id);
}

std::optional<DownloadEntity> MangaDatabase::getDownloadById(const QString &id) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, provider, manga_id, manga_title, chapter_id, chapter_title, chapter_number, "
        "cbz_path, status, progress, total_pages, queued_at, completed_at "
        "FROM downloads WHERE id = ?;"));
    q.addBindValue(id);

    if (q.exec() && q.next()) {
        DownloadEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.mangaTitle = q.value(3).toString();
        e.chapterId = q.value(4).toString();
        e.chapterTitle = q.value(5).toString();
        e.chapterNumber = q.value(6).toDouble();
        e.cbzPath = q.value(7).toString();
        e.status = q.value(8).toString();
        e.progress = q.value(9).toInt();
        e.totalPages = q.value(10).toInt();
        e.queuedAt = q.value(11).toLongLong();
        e.completedAt = q.value(12).toLongLong();
        return e;
    }
    return std::nullopt;
}

bool MangaDatabase::enqueueDownload(const DownloadEntity &item)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const QString id = item.id.isEmpty() ? QStringLiteral("%1/%2/%3").arg(item.provider, item.mangaId, item.chapterId) : item.id;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 queuedAt = item.queuedAt == 0 ? now : item.queuedAt;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO downloads (id, provider, manga_id, manga_title, chapter_id, chapter_title, "
        "chapter_number, cbz_path, status, progress, total_pages, queued_at, completed_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  manga_title = excluded.manga_title, "
        "  chapter_title = excluded.chapter_title, "
        "  chapter_number = excluded.chapter_number, "
        "  cbz_path = excluded.cbz_path, "
        "  status = excluded.status, "
        "  progress = excluded.progress, "
        "  total_pages = excluded.total_pages, "
        "  completed_at = excluded.completed_at;"));

    q.addBindValue(id);
    q.addBindValue(item.provider);
    q.addBindValue(item.mangaId);
    q.addBindValue(item.mangaTitle);
    q.addBindValue(item.chapterId);
    q.addBindValue(item.chapterTitle.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : item.chapterTitle);
    q.addBindValue(item.chapterNumber);
    q.addBindValue(item.cbzPath.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : item.cbzPath);
    q.addBindValue(item.status.isEmpty() ? QStringLiteral("queued") : item.status);
    q.addBindValue(item.progress);
    q.addBindValue(item.totalPages);
    q.addBindValue(queuedAt);
    q.addBindValue(item.completedAt == 0 ? QVariant(QMetaType(QMetaType::LongLong)) : item.completedAt);

    if (!q.exec()) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: enqueueDownload error: %1").arg(q.lastError().text());
        return false;
    }

    emit downloadsChanged();
    emit downloadUpdated(id);
    return true;
}

bool MangaDatabase::updateDownloadProgress(const QString &id, int progress, int totalPages, const QString &status)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery q(db);
    if (status == QStringLiteral("completed")) {
        q.prepare(QStringLiteral(
            "UPDATE downloads SET progress = ?, total_pages = ?, status = ?, completed_at = ? WHERE id = ?;"));
        q.addBindValue(progress);
        q.addBindValue(totalPages);
        q.addBindValue(status);
        q.addBindValue(now);
        q.addBindValue(id);
    } else {
        q.prepare(QStringLiteral(
            "UPDATE downloads SET progress = ?, total_pages = ?, status = ? WHERE id = ?;"));
        q.addBindValue(progress);
        q.addBindValue(totalPages);
        q.addBindValue(status);
        q.addBindValue(id);
    }

    if (!q.exec()) {
        return false;
    }

    emit downloadsChanged();
    emit downloadUpdated(id);
    return true;
}

bool MangaDatabase::updateDownloadCbzPath(const QString &id, const QString &cbzPath)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE downloads SET cbz_path = ? WHERE id = ?;"));
    q.addBindValue(cbzPath);
    q.addBindValue(id);
    if (!q.exec()) {
        return false;
    }
    emit downloadUpdated(id);
    return true;
}

bool MangaDatabase::removeDownload(const QString &id)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM downloads WHERE id = ?;"));
    q.addBindValue(id);
    if (!q.exec()) {
        return false;
    }
    emit downloadsChanged();
    return true;
}

bool MangaDatabase::clearCompletedDownloads()
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("DELETE FROM downloads WHERE status = 'completed';"))) {
        return false;
    }
    emit downloadsChanged();
    return true;
}

// -----------------------------------------------------------------------------
// Categories
// -----------------------------------------------------------------------------

QList<CategoryEntity> MangaDatabase::getCategories() const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.exec(QStringLiteral("SELECT id, name, sort_order FROM categories ORDER BY sort_order ASC;"));

    QList<CategoryEntity> list;
    while (q.next()) {
        CategoryEntity e;
        e.id = q.value(0).toString();
        e.name = q.value(1).toString();
        e.sortOrder = q.value(2).toInt();
        list.append(e);
    }
    return list;
}

std::optional<CategoryEntity> MangaDatabase::getCategory(const QString &id) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id, name, sort_order FROM categories WHERE id = ?;"));
    q.addBindValue(id);
    if (q.exec() && q.next()) {
        CategoryEntity e;
        e.id = q.value(0).toString();
        e.name = q.value(1).toString();
        e.sortOrder = q.value(2).toInt();
        return e;
    }
    return std::nullopt;
}

bool MangaDatabase::addCategory(const QString &id, const QString &name, int sortOrder)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO categories (id, name, sort_order) VALUES (?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET name = excluded.name, sort_order = excluded.sort_order;"));
    q.addBindValue(id);
    q.addBindValue(name);
    q.addBindValue(sortOrder);

    if (!q.exec()) {
        return false;
    }
    emit categoriesChanged();
    return true;
}

bool MangaDatabase::updateCategory(const QString &id, const QString &name, int sortOrder)
{
    return addCategory(id, name, sortOrder);
}

bool MangaDatabase::deleteCategory(const QString &id)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    db.transaction();

    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM categories WHERE id = ?;"));
    q.addBindValue(id);
    if (!q.exec()) {
        db.rollback();
        return false;
    }

    q.prepare(QStringLiteral("DELETE FROM library_categories WHERE category_id = ?;"));
    q.addBindValue(id);
    q.exec();

    db.commit();
    emit categoriesChanged();
    return true;
}

bool MangaDatabase::setMangaCategories(const QString &provider, const QString &mangaId, const QStringList &categoryIds)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const QString libId = provider + QStringLiteral("/") + mangaId;

    db.transaction();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM library_categories WHERE library_id = ?;"));
    q.addBindValue(libId);
    if (!q.exec()) {
        db.rollback();
        return false;
    }

    q.prepare(QStringLiteral("INSERT INTO library_categories (library_id, category_id) VALUES (?, ?);"));
    for (const QString &catId : categoryIds) {
        q.addBindValue(libId);
        q.addBindValue(catId);
        if (!q.exec()) {
            db.rollback();
            return false;
        }
    }

    db.commit();
    emit categoriesChanged();
    return true;
}

QStringList MangaDatabase::getMangaCategories(const QString &provider, const QString &mangaId) const
{
    auto db = getThreadConnection();
    const QString libId = provider + QStringLiteral("/") + mangaId;

    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT category_id FROM library_categories WHERE library_id = ?;"));
    q.addBindValue(libId);

    QStringList list;
    if (q.exec()) {
        while (q.next()) {
            list.append(q.value(0).toString());
        }
    }
    return list;
}

QList<LibraryCategoryEntity> MangaDatabase::getAllLibraryCategories() const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.exec(QStringLiteral("SELECT library_id, category_id FROM library_categories;"));

    QList<LibraryCategoryEntity> list;
    while (q.next()) {
        LibraryCategoryEntity e;
        e.libraryId = q.value(0).toString();
        e.categoryId = q.value(1).toString();
        list.append(e);
    }
    return list;
}

int MangaDatabase::getMangaCountInCategory(const QString &categoryId) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM library_categories WHERE category_id = ?;"));
    q.addBindValue(categoryId);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Tracker Binds
// -----------------------------------------------------------------------------

QList<TrackerBindEntity> MangaDatabase::getTrackerBinds(const QString &provider, const QString &mangaId) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, provider, manga_id, tracker, remote_id, remote_title, last_chapter_read, updated_at "
        "FROM tracker_binds WHERE provider = ? AND manga_id = ?;"));
    q.addBindValue(provider);
    q.addBindValue(mangaId);

    QList<TrackerBindEntity> list;
    if (q.exec()) {
        while (q.next()) {
            TrackerBindEntity e;
            e.id = q.value(0).toString();
            e.provider = q.value(1).toString();
            e.mangaId = q.value(2).toString();
            e.tracker = q.value(3).toString();
            e.remoteId = q.value(4).toInt();
            e.remoteTitle = q.value(5).toString();
            e.lastChapterRead = q.value(6).toDouble();
            e.updatedAt = q.value(7).toLongLong();
            list.append(e);
        }
    }
    return list;
}

std::optional<TrackerBindEntity> MangaDatabase::getTrackerBind(const QString &provider, const QString &mangaId, const QString &tracker) const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    const QString id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, tracker);
    q.prepare(QStringLiteral(
        "SELECT id, provider, manga_id, tracker, remote_id, remote_title, last_chapter_read, updated_at "
        "FROM tracker_binds WHERE id = ?;"));
    q.addBindValue(id);

    if (q.exec() && q.next()) {
        TrackerBindEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.tracker = q.value(3).toString();
        e.remoteId = q.value(4).toInt();
        e.remoteTitle = q.value(5).toString();
        e.lastChapterRead = q.value(6).toDouble();
        e.updatedAt = q.value(7).toLongLong();
        return e;
    }
    return std::nullopt;
}

bool MangaDatabase::saveTrackerBind(const TrackerBindEntity &bind)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const QString id = bind.id.isEmpty() ? QStringLiteral("%1/%2/%3").arg(bind.provider, bind.mangaId, bind.tracker) : bind.id;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO tracker_binds (id, provider, manga_id, tracker, remote_id, remote_title, last_chapter_read, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  remote_id = excluded.remote_id, "
        "  remote_title = excluded.remote_title, "
        "  last_chapter_read = excluded.last_chapter_read, "
        "  updated_at = excluded.updated_at;"));

    q.addBindValue(id);
    q.addBindValue(bind.provider);
    q.addBindValue(bind.mangaId);
    q.addBindValue(bind.tracker);
    q.addBindValue(bind.remoteId);
    q.addBindValue(bind.remoteTitle.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : bind.remoteTitle);
    q.addBindValue(bind.lastChapterRead);
    q.addBindValue(now);

    if (!q.exec()) {
        qCritical().noquote() << QStringLiteral("MangaDatabase: saveTrackerBind error: %1").arg(q.lastError().text());
        return false;
    }

    emit trackerBindsChanged();
    return true;
}

bool MangaDatabase::deleteTrackerBind(const QString &provider, const QString &mangaId, const QString &tracker)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const QString id = QStringLiteral("%1/%2/%3").arg(provider, mangaId, tracker);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM tracker_binds WHERE id = ?;"));
    q.addBindValue(id);
    if (!q.exec()) {
        return false;
    }
    emit trackerBindsChanged();
    return true;
}

QList<TrackerBindEntity> MangaDatabase::getAllTrackerBinds() const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.exec(QStringLiteral("SELECT id, provider, manga_id, tracker, remote_id, remote_title, last_chapter_read, updated_at FROM tracker_binds;"));

    QList<TrackerBindEntity> list;
    while (q.next()) {
        TrackerBindEntity e;
        e.id = q.value(0).toString();
        e.provider = q.value(1).toString();
        e.mangaId = q.value(2).toString();
        e.tracker = q.value(3).toString();
        e.remoteId = q.value(4).toInt();
        e.remoteTitle = q.value(5).toString();
        e.lastChapterRead = q.value(6).toDouble();
        e.updatedAt = q.value(7).toLongLong();
        list.append(e);
    }
    return list;
}

// -----------------------------------------------------------------------------
// New Chapters (Updates)
// -----------------------------------------------------------------------------

QList<NewChapterEntity> MangaDatabase::getNewChapters() const
{
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.exec(QStringLiteral(
        "SELECT id, manga_id, manga_title, cover_url, provider, chapter_id, chapter_title, chapter_number, detected_at "
        "FROM new_chapters ORDER BY detected_at DESC;"));

    QList<NewChapterEntity> list;
    while (q.next()) {
        NewChapterEntity e;
        e.id = q.value(0).toString();
        e.mangaId = q.value(1).toString();
        e.mangaTitle = q.value(2).toString();
        e.coverUrl = q.value(3).toString();
        e.provider = q.value(4).toString();
        e.chapterId = q.value(5).toString();
        e.chapterTitle = q.value(6).toString();
        e.chapterNumber = q.value(7).toDouble();
        e.detectedAt = q.value(8).toLongLong();
        list.append(e);
    }
    return list;
}

bool MangaDatabase::saveNewChapters(const QList<NewChapterEntity> &entries)
{
    if (entries.isEmpty()) {
        return true;
    }

    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    db.transaction();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO new_chapters (id, manga_id, manga_title, cover_url, provider, chapter_id, chapter_title, chapter_number, detected_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  manga_title = excluded.manga_title, "
        "  cover_url = excluded.cover_url, "
        "  chapter_title = excluded.chapter_title, "
        "  chapter_number = excluded.chapter_number, "
        "  detected_at = excluded.detected_at;"));

    for (const auto &entry : entries) {
        const QString id = entry.id.isEmpty()
            ? QStringLiteral("%1/%2/%3").arg(entry.provider, entry.mangaId, entry.chapterId)
            : entry.id;
        const qint64 detectedAt = entry.detectedAt == 0 ? now : entry.detectedAt;

        q.addBindValue(id);
        q.addBindValue(entry.mangaId);
        q.addBindValue(entry.mangaTitle);
        q.addBindValue(entry.coverUrl.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : entry.coverUrl);
        q.addBindValue(entry.provider);
        q.addBindValue(entry.chapterId);
        q.addBindValue(entry.chapterTitle);
        q.addBindValue(entry.chapterNumber);
        q.addBindValue(detectedAt);

        if (!q.exec()) {
            qCritical().noquote() << QStringLiteral("MangaDatabase: saveNewChapters error: %1").arg(q.lastError().text());
            db.rollback();
            return false;
        }
    }

    db.commit();
    emit newChaptersChanged();
    return true;
}

bool MangaDatabase::removeNewChapter(const QString &id)
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM new_chapters WHERE id = ?;"));
    q.addBindValue(id);
    if (!q.exec()) {
        return false;
    }
    emit newChaptersChanged();
    return true;
}

bool MangaDatabase::clearNewChapters()
{
    QMutexLocker locker(&m_writeLock);
    auto db = getThreadConnection();
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("DELETE FROM new_chapters;"))) {
        return false;
    }
    emit newChaptersChanged();
    return true;
}

} // namespace mangadl
