#include "ExtensionsController.h"
#include "extensions/ExtensionManager.h"
#include "db/MangaDatabase.h"
#include "util/AppPaths.h"

#include <QSettings>
#include <algorithm>

using namespace mangadl;

ExtensionsController::ExtensionsController(QObject *parent)
    : QObject(parent)
{
    loadPinnedSources();
    refresh();

    // Rebuild library items if database changes
    connect(MangaDatabase::instance(), &MangaDatabase::libraryChanged, this, &ExtensionsController::rebuildLibraryItems);
}

ExtensionsController::~ExtensionsController()
{
    *m_alive = false;
}

void ExtensionsController::loadPinnedSources()
{
    QSettings settings(AppPaths::configDir() + QStringLiteral("/settings.ini"), QSettings::IniFormat);
    m_pinnedSources = settings.value(QStringLiteral("browse/pinnedSources")).toStringList();
}

void ExtensionsController::savePinnedSources()
{
    QSettings settings(AppPaths::configDir() + QStringLiteral("/settings.ini"), QSettings::IniFormat);
    settings.setValue(QStringLiteral("browse/pinnedSources"), m_pinnedSources);
}

bool ExtensionsController::isPinned(const QString &sourceId) const
{
    return m_pinnedSources.contains(sourceId, Qt::CaseInsensitive);
}

void ExtensionsController::togglePinSource(const QString &sourceId)
{
    if (isPinned(sourceId)) {
        m_pinnedSources.removeAll(sourceId);
    } else {
        m_pinnedSources.append(sourceId);
    }
    savePinnedSources();
    rebuildSources();
}

void ExtensionsController::setLanguageFilter(const QString &lang)
{
    if (m_languageFilter != lang) {
        m_languageFilter = lang;
        emit languageFilterChanged();
        rebuildExtensions();
    }
}

void ExtensionsController::setShow18Plus(bool show)
{
    if (m_show18Plus != show) {
        m_show18Plus = show;
        emit show18PlusChanged();
        rebuildExtensions();
    }
}

QString ExtensionsController::colorForId(const QString &id)
{
    if (id == QStringLiteral("mangadex")) return QStringLiteral("#FF6740");
    if (id == QStringLiteral("asurascans")) return QStringLiteral("#8B5CF6");
    if (id == QStringLiteral("mangakatana")) return QStringLiteral("#10B981");
    if (id == QStringLiteral("flamecomics")) return QStringLiteral("#EF4444");
    if (id == QStringLiteral("mangakakalot")) return QStringLiteral("#F59E0B");
    if (id == QStringLiteral("novelfull")) return QStringLiteral("#3B82F6");

    // Deterministic palette
    static const QStringList palette = {
        QStringLiteral("#3B82F6"), QStringLiteral("#10B981"), QStringLiteral("#8B5CF6"),
        QStringLiteral("#F59E0B"), QStringLiteral("#EC4899"), QStringLiteral("#06B6D4"),
        QStringLiteral("#6366F1"), QStringLiteral("#14B8A6"), QStringLiteral("#E11D48")
    };
    const uint h = qHash(id);
    return palette[h % palette.size()];
}

void ExtensionsController::refresh()
{
    rebuildSources();
    rebuildExtensions();
    rebuildLibraryItems();
}

void ExtensionsController::rebuildSources()
{
    m_sources.clear();
    const auto allExts = ExtensionManager::instance().extensions();

    QList<ExtensionInfo> sorted = allExts;
    std::sort(sorted.begin(), sorted.end(), [this](const ExtensionInfo &a, const ExtensionInfo &b) {
        const bool aPin = isPinned(a.id);
        const bool bPin = isPinned(b.id);
        if (aPin != bPin) return aPin > bPin; // Pinned first
        return a.name.localeAwareCompare(b.name) < 0;
    });

    for (const auto &ext : sorted) {
        QVariantMap map;
        map[QStringLiteral("id")] = ext.id;
        map[QStringLiteral("name")] = ext.name;
        map[QStringLiteral("language")] = ext.language;
        map[QStringLiteral("type")] = (ext.type == ExtensionType::Novel) ? QStringLiteral("novel") : QStringLiteral("manga");
        map[QStringLiteral("initial")] = ext.name.left(1).toUpper();
        map[QStringLiteral("color")] = colorForId(ext.id);
        map[QStringLiteral("meta")] = QStringLiteral("%1 · %2")
            .arg(ext.language.toUpper())
            .arg(ext.isBuiltin ? QStringLiteral("Built-in") : QStringLiteral("Installed"));
        map[QStringLiteral("pinned")] = isPinned(ext.id);
        m_sources.append(map);
    }
    emit sourcesChanged();

    // Determine last used source
    const auto hist = MangaDatabase::instance()->getHistory(1);
    QString lastId = QStringLiteral("mangadex");
    if (!hist.isEmpty() && !hist.first().provider.isEmpty()) {
        lastId = hist.first().provider;
    }

    const auto *lastExt = ExtensionManager::instance().findById(lastId);
    QVariantMap lastMap;
    if (lastExt) {
        lastMap[QStringLiteral("id")] = lastExt->id;
        lastMap[QStringLiteral("name")] = lastExt->name;
        lastMap[QStringLiteral("initial")] = lastExt->name.left(1).toUpper();
        lastMap[QStringLiteral("color")] = colorForId(lastExt->id);
        lastMap[QStringLiteral("meta")] = QStringLiteral("%1 · Last read").arg(lastExt->language.toUpper());
    } else {
        lastMap[QStringLiteral("id")] = QStringLiteral("mangadex");
        lastMap[QStringLiteral("name")] = QStringLiteral("MangaDex");
        lastMap[QStringLiteral("initial")] = QStringLiteral("M");
        lastMap[QStringLiteral("color")] = QStringLiteral("#FF6740");
        lastMap[QStringLiteral("meta")] = QStringLiteral("EN · Default");
    }
    m_lastUsedSource = lastMap;
    emit lastUsedSourceChanged();
}

void ExtensionsController::rebuildExtensions()
{
    m_extensions.clear();
    const auto allExts = ExtensionManager::instance().extensions();
    m_installedCount = 0;
    m_updateCount = 0;

    for (const auto &ext : allExts) {
        if (m_languageFilter == QStringLiteral("English") && !ext.language.startsWith(QStringLiteral("en"), Qt::CaseInsensitive)) {
            continue;
        }

        m_installedCount++;

        QVariantMap map;
        map[QStringLiteral("id")] = ext.id;
        map[QStringLiteral("name")] = ext.name;
        map[QStringLiteral("language")] = ext.language.toUpper();
        map[QStringLiteral("version")] = ext.version.isEmpty() ? QStringLiteral("1.0.0") : ext.version;
        map[QStringLiteral("initial")] = ext.name.left(1).toUpper();
        map[QStringLiteral("color")] = colorForId(ext.id);
        map[QStringLiteral("state")] = QStringLiteral("installed");

        m_extensions.append(map);
    }

    emit extensionsChanged();
    emit countsChanged();
}

void ExtensionsController::rebuildLibraryItems()
{
    m_libraryItems.clear();
    const auto list = MangaDatabase::instance()->getLibrary();

    for (const auto &item : list) {
        const auto *ext = ExtensionManager::instance().findById(item.provider);
        QVariantMap map;
        map[QStringLiteral("id")] = item.id;
        map[QStringLiteral("mangaId")] = item.mangaId;
        map[QStringLiteral("provider")] = item.provider;
        map[QStringLiteral("source")] = ext ? ext->name : item.provider;
        map[QStringLiteral("title")] = item.title;
        map[QStringLiteral("cover")] = item.coverUrl;
        map[QStringLiteral("coverUrl")] = item.coverUrl;
        map[QStringLiteral("type")] = item.type;
        map[QStringLiteral("totalChapters")] = item.totalChapters;
        m_libraryItems.append(map);
    }
    emit libraryItemsChanged();
}

void ExtensionsController::installExtension(const QString &extId)
{
    Q_UNUSED(extId);
    rebuildExtensions();
}

void ExtensionsController::uninstallExtension(const QString &extId)
{
    Q_UNUSED(extId);
    rebuildExtensions();
}

void ExtensionsController::updateExtension(const QString &extId)
{
    Q_UNUSED(extId);
    rebuildExtensions();
}

void ExtensionsController::updateAll()
{
    rebuildExtensions();
}

void ExtensionsController::searchMigrationCandidates(const QString &targetProvider, const QString &title)
{
    m_migrationCandidates.clear();
    emit migrationCandidatesChanged();

    if (targetProvider.isEmpty() || title.isEmpty()) {
        m_searchingMigration = false;
        emit searchingMigrationChanged();
        return;
    }

    m_searchingMigration = true;
    emit searchingMigrationChanged();

    m_migrationSearchId++;
    const uint64_t searchId = m_migrationSearchId;
    auto alive = m_alive;

    ExtensionManager::instance().search(targetProvider, title, 1, [this, alive, searchId, targetProvider](QList<MangaPreview> previews, SourceError err) {
        if (!*alive || searchId != m_migrationSearchId)
            return;

        m_searchingMigration = false;
        emit searchingMigrationChanged();

        if (!previews.isEmpty()) {
            const auto *ext = ExtensionManager::instance().findById(targetProvider);
            const QString srcName = ext ? ext->name : targetProvider;

            for (const auto &p : previews) {
                QVariantMap map;
                map[QStringLiteral("id")] = p.id;
                map[QStringLiteral("mangaId")] = p.id;
                map[QStringLiteral("title")] = p.title;
                map[QStringLiteral("cover")] = p.coverUrl;
                map[QStringLiteral("coverUrl")] = p.coverUrl;
                map[QStringLiteral("provider")] = targetProvider;
                map[QStringLiteral("source")] = srcName;
                m_migrationCandidates.append(map);
            }
        }
        emit migrationCandidatesChanged();
    });
}

bool ExtensionsController::migrateManga(const QString &oldProvider, const QString &oldMangaId,
                                      const QString &newProvider, const QString &newMangaId,
                                      const QString &newTitle, const QString &newCoverUrl)
{
    const bool ok = MangaDatabase::instance()->migrateManga(oldProvider, oldMangaId, newProvider, newMangaId, newTitle, newCoverUrl);
    if (ok) {
        rebuildLibraryItems();
        emit migrationFinished(true, QStringLiteral("Migrated successfully"));
    } else {
        emit migrationFinished(false, QStringLiteral("Migration failed"));
    }
    return ok;
}
