#include "HistoryModel.h"

#include <QDateTime>

namespace mangadl {

QVariantMap HistoryModel::Item::toMap() const
{
    QVariantMap mangaMap{
        {QStringLiteral("id"), entity.mangaId},
        {QStringLiteral("title"), entity.mangaTitle},
        {QStringLiteral("cover"), entity.coverUrl.isEmpty() ? QStringLiteral("#1A2433") : entity.coverUrl},
        {QStringLiteral("source"), entity.provider},
        {QStringLiteral("provider"), entity.provider},
        {QStringLiteral("inLibrary"), true}
    };

    return QVariantMap{
        {QStringLiteral("manga"), mangaMap},
        {QStringLiteral("where"), where},
        {QStringLiteral("when"), when},
        {QStringLiteral("provider"), entity.provider},
        {QStringLiteral("mangaId"), entity.mangaId},
        {QStringLiteral("chapterId"), entity.chapterId},
        {QStringLiteral("chapterTitle"), entity.chapterTitle},
        {QStringLiteral("readAt"), entity.readAt}
    };
}

HistoryModel::HistoryModel(MangaDatabase *db, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(db ? db : MangaDatabase::instance())
{
    if (m_db) {
        connect(m_db, &MangaDatabase::historyChanged, this, &HistoryModel::refresh);
        connect(m_db, &MangaDatabase::progressChanged, this, &HistoryModel::refresh);
    }
    reload();
}

HistoryModel &HistoryModel::instance()
{
    static HistoryModel s_instance;
    return s_instance;
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return QVariant();
    }

    const auto &item = m_items[static_cast<size_t>(index.row())];

    switch (role) {
    case Qt::DisplayRole:
    case ModelDataRole:
        return item.toMap();
    case MangaRole: {
        return QVariantMap{
            {QStringLiteral("id"), item.entity.mangaId},
            {QStringLiteral("title"), item.entity.mangaTitle},
            {QStringLiteral("cover"), item.entity.coverUrl.isEmpty() ? QStringLiteral("#1A2433") : item.entity.coverUrl},
            {QStringLiteral("source"), item.entity.provider},
            {QStringLiteral("provider"), item.entity.provider},
            {QStringLiteral("inLibrary"), true}
        };
    }
    case WhereRole:
        return item.where;
    case WhenRole:
        return item.when;
    case ProviderRole:
        return item.entity.provider;
    case MangaIdRole:
        return item.entity.mangaId;
    case ChapterIdRole:
        return item.entity.chapterId;
    case ChapterTitleRole:
        return item.entity.chapterTitle;
    case ReadAtRole:
        return item.entity.readAt;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {
        {ModelDataRole, "modelData"},
        {MangaRole, "manga"},
        {WhereRole, "where"},
        {WhenRole, "when"},
        {ProviderRole, "provider"},
        {MangaIdRole, "mangaId"},
        {ChapterIdRole, "chapterId"},
        {ChapterTitleRole, "chapterTitle"},
        {ReadAtRole, "readAt"}
    };
}

void HistoryModel::setSearchFilter(const QString &filter)
{
    if (m_searchFilter != filter) {
        m_searchFilter = filter;
        reload();
        emit filterChanged();
    }
}

void HistoryModel::setTimeRange(const QString &range)
{
    if (m_timeRange != range) {
        m_timeRange = range;
        reload();
        emit rangeChanged();
    }
}

void HistoryModel::refresh()
{
    reload();
}

void HistoryModel::clearHistory()
{
    if (!m_db) return;
    m_db->clearHistory();
}

void HistoryModel::removeItem(int index)
{
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        const auto &item = m_items[static_cast<size_t>(index)];
        remove(item.entity.provider, item.entity.mangaId, item.entity.chapterId);
    }
}

void HistoryModel::remove(const QString &provider, const QString &mangaId, const QString &chapterId)
{
    if (!m_db) return;
    m_db->removeFromHistory(provider, mangaId, chapterId);
}

void HistoryModel::record(const QString &provider, const QString &mangaId, const QString &mangaTitle,
                          const QString &chapterId, const QString &chapterTitle, const QString &coverUrl)
{
    if (!m_db) return;
    m_db->recordHistory(provider, mangaId, mangaTitle, chapterId, chapterTitle, coverUrl);
}

void HistoryModel::reload()
{
    beginResetModel();
    m_items.clear();

    if (!m_db) {
        endResetModel();
        emit countChanged();
        return;
    }

    const auto history = m_db->getHistory(100);
    const QDateTime now = QDateTime::currentDateTime();
    const QDate today = now.date();
    const QDate yesterday = today.addDays(-1);

    // Compute cutoff for timeRange
    qint64 cutoffMs = 0;
    if (m_timeRange == QStringLiteral("Today")) {
        cutoffMs = QDateTime(today, QTime(0, 0, 0)).toMSecsSinceEpoch();
    } else if (m_timeRange == QStringLiteral("This week")) {
        cutoffMs = QDateTime(today.addDays(-7), QTime(0, 0, 0)).toMSecsSinceEpoch();
    } else if (m_timeRange == QStringLiteral("This month")) {
        cutoffMs = QDateTime(today.addDays(-30), QTime(0, 0, 0)).toMSecsSinceEpoch();
    }

    for (const auto &h : history) {
        if (cutoffMs > 0 && h.readAt < cutoffMs) {
            continue;
        }

        if (!m_searchFilter.isEmpty()) {
            if (!h.mangaTitle.contains(m_searchFilter, Qt::CaseInsensitive) &&
                !h.chapterTitle.contains(m_searchFilter, Qt::CaseInsensitive) &&
                !h.chapterId.contains(m_searchFilter, Qt::CaseInsensitive)) {
                continue;
            }
        }

        Item row;
        row.entity = h;

        // Determine where
        auto p = m_db->getProgress(h.provider, h.mangaId, h.chapterId);
        if (p && p->completed == 1) {
            row.where = QStringLiteral("Ch. ") + h.chapterId + QStringLiteral(" · finished");
        } else if (p && p->page > 0) {
            row.where = QStringLiteral("Ch. ") + h.chapterId + QStringLiteral(" · page ") + QString::number(p->page);
        } else if (!h.chapterTitle.isEmpty()) {
            row.where = QStringLiteral("Ch. ") + h.chapterId + QStringLiteral(" · ") + h.chapterTitle;
        } else {
            row.where = QStringLiteral("Ch. ") + h.chapterId;
        }

        // Determine when
        const QDateTime readTime = QDateTime::fromMSecsSinceEpoch(h.readAt);
        const QDate readDate = readTime.date();
        if (readDate == today) {
            row.when = QStringLiteral("Today · ") + readTime.toString(QStringLiteral("hh:mm"));
        } else if (readDate == yesterday) {
            row.when = QStringLiteral("Yesterday");
        } else if (readDate.year() == today.year()) {
            row.when = readTime.toString(QStringLiteral("MMM d"));
        } else {
            row.when = readTime.toString(QStringLiteral("MMM d, yyyy"));
        }

        m_items.push_back(std::move(row));
    }

    endResetModel();
    emit countChanged();
}

} // namespace mangadl
