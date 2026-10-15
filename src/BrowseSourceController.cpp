#include "BrowseSourceController.h"
#include "extensions/ExtensionManager.h"
#include "db/MangaDatabase.h"

#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>

using namespace mangadl;

BrowseSourceController::BrowseSourceController(QObject *parent)
    : QObject(parent)
{
}

BrowseSourceController::~BrowseSourceController()
{
    *m_alive = false;
}

void BrowseSourceController::setSource(const QString &s)
{
    if (m_source != s) {
        m_source = s;
        emit sourceChanged();
        resolveSource(s);
        m_page = 1;
        fetchItems(false);
    }
}

void BrowseSourceController::setMode(const QString &m)
{
    if (m_mode != m) {
        m_mode = m;
        emit modeChanged();
        m_page = 1;
        fetchItems(false);
    }
}

void BrowseSourceController::setSearchQuery(const QString &q)
{
    if (m_searchQuery != q) {
        m_searchQuery = q;
        emit searchQueryChanged();
    }
}

void BrowseSourceController::load(const QString &sourceNameOrId)
{
    m_source = sourceNameOrId;
    emit sourceChanged();
    resolveSource(sourceNameOrId);
    m_page = 1;
    m_searchQuery.clear();
    emit searchQueryChanged();
    fetchItems(false);
}

void BrowseSourceController::setBrowseMode(const QString &newMode)
{
    setMode(newMode);
}

void BrowseSourceController::search(const QString &query)
{
    m_searchQuery = query.trimmed();
    emit searchQueryChanged();
    m_page = 1;
    fetchItems(false);
}

void BrowseSourceController::loadMore()
{
    if (!m_loading && m_hasMore) {
        m_page++;
        emit pageChanged();
        fetchItems(true);
    }
}

void BrowseSourceController::openInBrowser()
{
    if (!m_sourceUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_sourceUrl));
    }
}

void BrowseSourceController::resolveSource(const QString &nameOrId)
{
    const QString clean = nameOrId.trimmed();
    const auto *ext = ExtensionManager::instance().findById(clean.toLower());

    if (!ext) {
        // Search by name
        const auto allExts = ExtensionManager::instance().extensions();
        for (const auto &item : allExts) {
            if (item.name.compare(clean, Qt::CaseInsensitive) == 0 ||
                item.id.compare(clean, Qt::CaseInsensitive) == 0) {
                ext = &item;
                break;
            }
        }
    }

    if (ext) {
        m_sourceId = ext->id;
        m_sourceName = ext->name;
    } else {
        m_sourceId = clean.toLower().remove(QLatin1Char(' ')).remove(QLatin1Char('.'));
        m_sourceName = clean.isEmpty() ? QStringLiteral("MangaDex") : clean;
    }

    // Resolve website URL
    if (m_sourceId == QStringLiteral("mangadex")) {
        m_sourceUrl = QStringLiteral("https://mangadex.org");
    } else if (m_sourceId == QStringLiteral("mangakakalot")) {
        m_sourceUrl = QStringLiteral("https://mangakakalot.com");
    } else if (m_sourceId == QStringLiteral("mangakatana")) {
        m_sourceUrl = QStringLiteral("https://mangakatana.com");
    } else if (m_sourceId == QStringLiteral("asurascans")) {
        m_sourceUrl = QStringLiteral("https://asuracomic.net");
    } else if (m_sourceId == QStringLiteral("flamecomics")) {
        m_sourceUrl = QStringLiteral("https://flamecomics.xyz");
    } else {
        m_sourceUrl = QStringLiteral("https://www.google.com/search?q=") + QUrl::toPercentEncoding(m_sourceName);
    }

    emit sourceInfoChanged();
}

void BrowseSourceController::fetchItems(bool append)
{
    m_currentFetchId++;
    const uint64_t fetchId = m_currentFetchId;

    m_loading = true;
    emit loadingChanged();

    auto alive = m_alive;
    auto handleResult = [this, alive, fetchId, append](QList<MangaPreview> previews, SourceError err) {
        if (!*alive || fetchId != m_currentFetchId) {
            return;
        }

        m_loading = false;
        emit loadingChanged();

        if (previews.isEmpty()) {
            m_hasMore = false;
            emit hasMoreChanged();
            if (!append) {
                m_items.clear();
                emit itemsChanged();
            }
            return;
        }

        m_hasMore = previews.size() >= 10;
        emit hasMoreChanged();

        QVariantList mapped;
        mapped.reserve(previews.size());

        for (const auto &p : previews) {
            QVariantMap item;
            item[QStringLiteral("id")] = p.id;
            item[QStringLiteral("mangaId")] = p.id;
            item[QStringLiteral("title")] = p.title;
            item[QStringLiteral("cover")] = p.coverUrl;
            item[QStringLiteral("coverUrl")] = p.coverUrl;
            item[QStringLiteral("provider")] = m_sourceId;
            item[QStringLiteral("source")] = m_sourceName;
            item[QStringLiteral("inLibrary")] = MangaDatabase::instance()->isInLibrary(m_sourceId, p.id);
            mapped.append(item);
        }

        if (append) {
            m_items.append(mapped);
        } else {
            m_items = mapped;
        }
        emit itemsChanged();
    };

    if (!m_searchQuery.isEmpty()) {
        ExtensionManager::instance().search(m_sourceId, m_searchQuery, m_page, handleResult);
    } else if (m_mode == QStringLiteral("latest")) {
        ExtensionManager::instance().getLatest(m_sourceId, m_page, handleResult);
    } else {
        ExtensionManager::instance().getPopular(m_sourceId, m_page, handleResult);
    }
}
