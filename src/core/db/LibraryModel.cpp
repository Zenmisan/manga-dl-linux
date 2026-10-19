#include "LibraryModel.h"

#include <algorithm>

namespace mangadl {

QVariantMap LibraryModel::Item::toMap() const
{
    return QVariantMap{
        {QStringLiteral("id"), entity.mangaId},
        {QStringLiteral("mangaId"), entity.mangaId},
        {QStringLiteral("provider"), entity.provider},
        {QStringLiteral("source"), entity.provider},
        {QStringLiteral("title"), entity.title},
        {QStringLiteral("cover"), entity.coverUrl.isEmpty() ? QStringLiteral("#1A2433") : entity.coverUrl},
        {QStringLiteral("url"), entity.url},
        {QStringLiteral("type"), entity.type},
        {QStringLiteral("totalChapters"), entity.totalChapters},
        {QStringLiteral("unread"), unread},
        {QStringLiteral("downloaded"), downloaded},
        {QStringLiteral("inLibrary"), true},
        {QStringLiteral("addedAt"), entity.addedAt},
        {QStringLiteral("updatedAt"), entity.updatedAt}
    };
}

LibraryModel::LibraryModel(MangaDatabase *db, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(db ? db : MangaDatabase::instance())
{
    if (m_db) {
        connect(m_db, &MangaDatabase::libraryChanged, this, &LibraryModel::refresh);
        connect(m_db, &MangaDatabase::historyChanged, this, &LibraryModel::continueReadingChanged);
        connect(m_db, &MangaDatabase::progressChanged, this, &LibraryModel::refresh);
        connect(m_db, &MangaDatabase::categoriesChanged, this, &LibraryModel::refresh);
    }
    reload();
}

LibraryModel &LibraryModel::instance()
{
    static LibraryModel s_instance;
    return s_instance;
}

int LibraryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant LibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return QVariant();
    }

    const auto &item = m_items[static_cast<size_t>(index.row())];

    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return item.entity.title;
    case ModelDataRole:
    case MangaRole:
        return item.toMap();
    case IdRole:
        return item.entity.mangaId;
    case ProviderRole:
    case SourceRole:
        return item.entity.provider;
    case CoverRole:
        return item.entity.coverUrl.isEmpty() ? QStringLiteral("#1A2433") : item.entity.coverUrl;
    case UrlRole:
        return item.entity.url;
    case TypeRole:
        return item.entity.type;
    case TotalChaptersRole:
        return item.entity.totalChapters;
    case UnreadRole:
        return item.unread;
    case DownloadedRole:
        return item.downloaded;
    case InLibraryRole:
        return true;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LibraryModel::roleNames() const
{
    return {
        {ModelDataRole, "modelData"},
        {MangaRole, "manga"},
        {IdRole, "id"},
        {ProviderRole, "provider"},
        {TitleRole, "title"},
        {CoverRole, "cover"},
        {UrlRole, "url"},
        {TypeRole, "type"},
        {TotalChaptersRole, "totalChapters"},
        {UnreadRole, "unread"},
        {DownloadedRole, "downloaded"},
        {InLibraryRole, "inLibrary"},
        {SourceRole, "source"}
    };
}

void LibraryModel::setCategoryFilter(const QString &filter)
{
    if (m_categoryFilter != filter) {
        m_categoryFilter = filter;
        reload();
        emit filterChanged();
    }
}

void LibraryModel::setSearchFilter(const QString &filter)
{
    if (m_searchFilter != filter) {
        m_searchFilter = filter;
        reload();
        emit filterChanged();
    }
}

void LibraryModel::setContentType(const QString &type)
{
    if (m_contentType != type) {
        m_contentType = type;
        reload();
        emit filterChanged();
    }
}

void LibraryModel::setFilterMode(const QString &mode)
{
    if (m_filterMode != mode) {
        m_filterMode = mode;
        reload();
        emit filterChanged();
    }
}

void LibraryModel::setSortMode(int mode)
{
    if (m_sortMode != mode) {
        m_sortMode = mode;
        reload();
        emit sortModeChanged();
    }
}

int LibraryModel::totalUnread() const
{
    int total = 0;
    for (const auto &item : m_items) {
        total += item.unread;
    }
    return total;
}

QVariantList LibraryModel::continueReading() const
{
    if (!m_db) return {};

    const auto history = m_db->getHistory(6);
    QVariantList list;

    for (const auto &h : history) {
        QVariantMap mangaMap{
            {QStringLiteral("id"), h.mangaId},
            {QStringLiteral("title"), h.mangaTitle},
            {QStringLiteral("cover"), h.coverUrl.isEmpty() ? QStringLiteral("#1A2433") : h.coverUrl},
            {QStringLiteral("source"), h.provider},
            {QStringLiteral("provider"), h.provider},
            {QStringLiteral("inLibrary"), true}
        };

        QString chapterLabel = h.chapterTitle.trimmed();
        if (chapterLabel.isEmpty()) {
            QString raw = h.chapterId;
            const int slashIdx = raw.lastIndexOf(QLatin1Char('/'));
            if (slashIdx >= 0) raw = raw.mid(slashIdx + 1);
            chapterLabel = raw.startsWith(QLatin1String("c"), Qt::CaseInsensitive)
                ? (QStringLiteral("Ch. ") + raw.mid(1))
                : (QStringLiteral("Ch. ") + raw);
        } else if (!chapterLabel.startsWith(QLatin1String("Ch"), Qt::CaseInsensitive) &&
                   !chapterLabel.startsWith(QLatin1String("Ep"), Qt::CaseInsensitive)) {
            chapterLabel = QStringLiteral("Ch. ") + chapterLabel;
        }

        QString where = chapterLabel;
        double progressFraction = 0.5;
        auto p = m_db->getProgress(h.provider, h.mangaId, h.chapterId);
        if (p && p->completed == 1) {
            progressFraction = 1.0;
            where = chapterLabel + QStringLiteral(" · finished");
        } else if (p && p->page > 0) {
            where = chapterLabel + QStringLiteral(" · page ") + QString::number(p->page);
        }

        list.append(QVariantMap{
            {QStringLiteral("manga"), mangaMap},
            {QStringLiteral("where"), where},
            {QStringLiteral("progress"), progressFraction}
        });
    }
    return list;
}

void LibraryModel::refresh()
{
    reload();
}

QVariantMap LibraryModel::get(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        return m_items[static_cast<size_t>(index)].toMap();
    }
    return {};
}

void LibraryModel::add(const QString &provider, const QString &mangaId, const QString &title,
                       const QString &coverUrl, const QString &type)
{
    if (!m_db) return;
    LibraryEntity e;
    e.provider = provider;
    e.mangaId = mangaId;
    e.title = title;
    e.coverUrl = coverUrl;
    e.type = type;
    m_db->addToLibrary(e);
}

void LibraryModel::remove(const QString &provider, const QString &mangaId)
{
    if (!m_db) return;
    m_db->removeFromLibrary(provider, mangaId);
}

bool LibraryModel::contains(const QString &provider, const QString &mangaId) const
{
    if (!m_db) return false;
    return m_db->isInLibrary(provider, mangaId);
}

void LibraryModel::reload()
{
    beginResetModel();
    m_items.clear();

    if (!m_db) {
        endResetModel();
        emit countChanged();
        emit totalUnreadChanged();
        emit continueReadingChanged();
        return;
    }

    const auto all = m_db->getLibrary();
    const bool isAllCategory = m_categoryFilter.isEmpty() ||
                               m_categoryFilter.startsWith(QStringLiteral("All"), Qt::CaseInsensitive);

    // Map of category id / name for matching
    const auto categories = m_db->getCategories();
    QString targetCategoryId;
    if (!isAllCategory) {
        for (const auto &cat : categories) {
            if (cat.id.compare(m_categoryFilter, Qt::CaseInsensitive) == 0 ||
                cat.name.compare(m_categoryFilter, Qt::CaseInsensitive) == 0) {
                targetCategoryId = cat.id;
                break;
            }
        }
        if (targetCategoryId.isEmpty()) {
            targetCategoryId = m_categoryFilter;
        }
    }

    for (const auto &item : all) {
        // Content type filter: "all", "manga", "novel"
        if (!m_contentType.isEmpty() && m_contentType != QLatin1String("all")) {
            if (m_contentType == QLatin1String("novel") && item.type != QLatin1String("novel")) {
                continue;
            } else if (m_contentType == QLatin1String("manga") && item.type == QLatin1String("novel")) {
                continue;
            }
        }

        // Search filter
        if (!m_searchFilter.isEmpty() &&
            !item.title.contains(m_searchFilter, Qt::CaseInsensitive)) {
            continue;
        }

        // Category filter
        if (!isAllCategory) {
            const auto itemCategories = m_db->getMangaCategories(item.provider, item.mangaId);
            bool match = false;
            for (const auto &catId : itemCategories) {
                if (catId.compare(targetCategoryId, Qt::CaseInsensitive) == 0) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                continue;
            }
        }

        Item row;
        row.entity = item;
        row.unread = m_db->getUnreadCount(item.provider, item.mangaId);

        // Downloaded check: any completed download
        auto dl = m_db->getDownloads();
        for (const auto &d : dl) {
            if (d.provider == item.provider && d.mangaId == item.mangaId && d.status == QStringLiteral("completed")) {
                row.downloaded = true;
                break;
            }
        }

        // Filter Mode: "all", "unread", "downloaded", "started", "completed"
        if (m_filterMode == QLatin1String("unread") && row.unread <= 0) {
            continue;
        }
        if (m_filterMode == QLatin1String("downloaded") && !row.downloaded) {
            continue;
        }
        if (m_filterMode == QLatin1String("started")) {
            auto prog = m_db->getMangaProgress(item.provider, item.mangaId);
            if (prog.isEmpty()) {
                continue;
            }
        }
        if (m_filterMode == QLatin1String("completed")) {
            if (row.unread > 0 || item.totalChapters <= 0) {
                continue;
            }
        }

        m_items.push_back(std::move(row));
    }

    // Sort
    if (m_sortMode == 1) { // Title A-Z
        std::sort(m_items.begin(), m_items.end(), [](const Item &a, const Item &b) {
            return a.entity.title.localeAwareCompare(b.entity.title) < 0;
        });
    } else if (m_sortMode == 2) { // Unread
        std::sort(m_items.begin(), m_items.end(), [](const Item &a, const Item &b) {
            if (a.unread != b.unread) return a.unread > b.unread;
            return a.entity.updatedAt > b.entity.updatedAt;
        });
    } else { // 0: Last read / updated
        std::sort(m_items.begin(), m_items.end(), [](const Item &a, const Item &b) {
            return a.entity.updatedAt > b.entity.updatedAt;
        });
    }

    endResetModel();
    emit countChanged();
    emit totalUnreadChanged();
    emit continueReadingChanged();
}

} // namespace mangadl
