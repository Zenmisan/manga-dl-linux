#include "SearchController.h"
#include "extensions/ExtensionManager.h"
#include "db/MangaDatabase.h"

#include <algorithm>

using namespace mangadl;

SearchController::SearchController(QObject *parent)
    : QObject(parent)
{
    populateAvailableSources();
}

SearchController::~SearchController()
{
    *m_alive = false;
    cancel();
}

void SearchController::populateAvailableSources()
{
    m_availableSources.clear();
    const auto list = ExtensionManager::instance().extensions();
    for (const auto &info : list) {
        QVariantMap map;
        map[QStringLiteral("id")] = info.id;
        map[QStringLiteral("name")] = info.name;
        map[QStringLiteral("language")] = info.language;
        map[QStringLiteral("type")] = (info.type == ExtensionType::Novel) ? QStringLiteral("novel") : QStringLiteral("manga");
        m_availableSources.append(map);
    }
    emit availableSourcesChanged();
}

void SearchController::setQuery(const QString &q)
{
    if (m_query != q) {
        m_query = q;
        emit queryChanged();
    }
}

void SearchController::setActiveTab(int tab)
{
    if (m_activeTab != tab) {
        m_activeTab = tab;
        emit activeTabChanged();
        if (!m_query.isEmpty()) {
            search(m_query);
        }
    }
}

void SearchController::setSelectedStatus(const QString &status)
{
    if (m_selectedStatus != status) {
        m_selectedStatus = status;
        emit selectedStatusChanged();
    }
}

void SearchController::setSortBy(const QString &sort)
{
    if (m_sortBy != sort) {
        m_sortBy = sort;
        emit sortByChanged();
        applySortAndFilterToGroups();
    }
}

void SearchController::setSelectedSources(const QStringList &sources)
{
    if (m_selectedSources != sources) {
        m_selectedSources = sources;
        emit selectedSourcesChanged();
    }
}

void SearchController::toggleSource(const QString &sourceId)
{
    if (m_selectedSources.contains(sourceId)) {
        m_selectedSources.removeAll(sourceId);
    } else {
        m_selectedSources.append(sourceId);
    }
    emit selectedSourcesChanged();
}

void SearchController::setSourceSelected(const QString &sourceId, bool selected)
{
    if (selected && !m_selectedSources.contains(sourceId)) {
        m_selectedSources.append(sourceId);
        emit selectedSourcesChanged();
    } else if (!selected && m_selectedSources.contains(sourceId)) {
        m_selectedSources.removeAll(sourceId);
        emit selectedSourcesChanged();
    }
}

void SearchController::resetFilters()
{
    m_selectedSources.clear();
    m_selectedStatus = QStringLiteral("Any");
    m_sortBy = QStringLiteral("Relevance");
    emit selectedSourcesChanged();
    emit selectedStatusChanged();
    emit sortByChanged();

    if (!m_query.isEmpty()) {
        search(m_query);
    }
}

void SearchController::cancel()
{
    m_currentSearchId++;
    if (m_searching) {
        m_searching = false;
        m_statusText = QStringLiteral("Search cancelled");
        emit searchingChanged();
        emit statusTextChanged();
    }
}

void SearchController::search(const QString &queryText)
{
    const QString q = queryText.trimmed();
    if (q.isEmpty()) {
        cancel();
        m_groups.clear();
        m_statusText.clear();
        emit groupsChanged();
        emit statusTextChanged();
        return;
    }

    m_query = q;
    emit queryChanged();

    m_currentSearchId++;
    const uint64_t searchId = m_currentSearchId;

    m_groups.clear();
    emit groupsChanged();

    // Determine candidate extensions
    auto allExts = ExtensionManager::instance().extensions();
    static const QStringList popularOrder = {
        QStringLiteral("mangadex"),
        QStringLiteral("asurascans"),
        QStringLiteral("flamecomics"),
        QStringLiteral("mangakakalot"),
        QStringLiteral("manganato"),
        QStringLiteral("mangakatana"),
        QStringLiteral("novelbin"),
        QStringLiteral("freewebnovel"),
        QStringLiteral("novelfull"),
        QStringLiteral("lightnovelworld"),
        QStringLiteral("bato"),
        QStringLiteral("webtoons"),
        QStringLiteral("royalroad"),
        QStringLiteral("comixto"),
        QStringLiteral("mangapill"),
        QStringLiteral("wuxiaworld")
    };

    std::stable_sort(allExts.begin(), allExts.end(), [](const ExtensionInfo &a, const ExtensionInfo &b) {
        int idxA = popularOrder.indexOf(a.id.toLower());
        int idxB = popularOrder.indexOf(b.id.toLower());
        if (idxA < 0) idxA = 999;
        if (idxB < 0) idxB = 999;
        return idxA < idxB;
    });

    QList<ExtensionInfo> candidates;

    for (const auto &ext : allExts) {
        // Tab filtering: 0: Manga, 1: Web Novels, 2: Readers (All)
        if (m_activeTab == 0 && ext.type != ExtensionType::Manga)
            continue;
        if (m_activeTab == 1 && ext.type != ExtensionType::Novel)
            continue;

        // If specific sources are selected, must match
        if (!m_selectedSources.isEmpty()) {
            if (m_selectedSources.contains(ext.id, Qt::CaseInsensitive)) {
                candidates.append(ext);
            }
        } else {
            // Default: pick top sources (up to 12)
            candidates.append(ext);
            if (candidates.size() >= 12)
                break;
        }
    }

    if (candidates.isEmpty()) {
        m_searching = false;
        m_statusText = QStringLiteral("No sources selected");
        emit searchingChanged();
        emit statusTextChanged();
        return;
    }

    m_searching = true;
    m_totalSourcesCount = candidates.size();
    m_completedSourcesCount = 0;
    m_statusText = QStringLiteral("Searching %1 sources...").arg(m_totalSourcesCount);
    emit searchingChanged();
    emit statusTextChanged();

    auto alive = m_alive;
    for (const auto &ext : candidates) {
        const QString extId = ext.id;
        const QString extName = ext.name;
        const QString extLang = ext.language;

        ExtensionManager::instance().search(extId, q, 1, [this, alive, searchId, extId, extName, extLang](QList<MangaPreview> previews, SourceError err) {
            if (!*alive || searchId != m_currentSearchId) {
                return; // Stale search or destroyed controller
            }

            m_completedSourcesCount++;

            if (!err.message.isEmpty()) {
                qDebug() << "SearchController:" << extName << "search error:" << err.toString();
            }

            if (!previews.isEmpty()) {
                QVariantList mappedResults;
                mappedResults.reserve(previews.size());

                for (const auto &p : previews) {
                    QVariantMap item;
                    item[QStringLiteral("id")] = p.id;
                    item[QStringLiteral("mangaId")] = p.id;
                    item[QStringLiteral("title")] = p.title;
                    item[QStringLiteral("cover")] = p.coverUrl;
                    item[QStringLiteral("coverUrl")] = p.coverUrl;
                    item[QStringLiteral("provider")] = extId;
                    item[QStringLiteral("source")] = extName;
                    item[QStringLiteral("inLibrary")] = MangaDatabase::instance()->isInLibrary(extId, p.id);
                    mappedResults.append(item);
                }

                // Sort results according to m_sortBy
                if (m_sortBy == QStringLiteral("Title A–Z")) {
                    std::sort(mappedResults.begin(), mappedResults.end(), [](const QVariant &a, const QVariant &b) {
                        return a.toMap().value(QStringLiteral("title")).toString().localeAwareCompare(
                                   b.toMap().value(QStringLiteral("title")).toString()) < 0;
                    });
                }

                QVariantMap group;
                group[QStringLiteral("source")] = extName;
                group[QStringLiteral("sourceId")] = extId;
                group[QStringLiteral("meta")] = QStringLiteral("%1 · %2 results").arg(extLang.toUpper()).arg(previews.size());
                group[QStringLiteral("results")] = mappedResults;

                m_groups.append(group);
                emit groupsChanged();
            }

            if (m_completedSourcesCount >= m_totalSourcesCount) {
                m_searching = false;
                emit searchingChanged();

                int totalResults = 0;
                for (const auto &g : m_groups) {
                    totalResults += g.toMap().value(QStringLiteral("results")).toList().size();
                }

                if (totalResults > 0) {
                    m_statusText = QStringLiteral("%1 results across %2 sources").arg(totalResults).arg(m_groups.size());
                } else {
                    m_statusText = QStringLiteral("No results found");
                }
                emit statusTextChanged();
            }
        });
    }
}

void SearchController::applySortAndFilterToGroups()
{
    if (m_groups.isEmpty())
        return;

    for (int i = 0; i < m_groups.size(); ++i) {
        QVariantMap group = m_groups[i].toMap();
        QVariantList results = group.value(QStringLiteral("results")).toList();

        if (m_sortBy == QStringLiteral("Title A–Z")) {
            std::sort(results.begin(), results.end(), [](const QVariant &a, const QVariant &b) {
                return a.toMap().value(QStringLiteral("title")).toString().localeAwareCompare(
                           b.toMap().value(QStringLiteral("title")).toString()) < 0;
            });
        }
        group[QStringLiteral("results")] = results;
        m_groups[i] = group;
    }
    emit groupsChanged();
}
