#include "UpdatesModel.h"

#include <QDateTime>
#include <map>

namespace mangadl {

UpdatesModel::UpdatesModel(MangaDatabase *db, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(db ? db : MangaDatabase::instance())
{
    if (m_db) {
        connect(m_db, &MangaDatabase::newChaptersChanged, this, &UpdatesModel::refresh);
    }
    reload();
}

UpdatesModel &UpdatesModel::instance()
{
    static UpdatesModel s_instance;
    return s_instance;
}

int UpdatesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_groups.size());
}

QVariant UpdatesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_groups.size())) {
        return QVariant();
    }

    const auto &group = m_groups[static_cast<size_t>(index.row())];

    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return group.title;
    case ItemsRole:
        return group.items;
    case ModelDataRole:
        return QVariantMap{
            {QStringLiteral("title"), group.title},
            {QStringLiteral("items"), group.items}
        };
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> UpdatesModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {ItemsRole, "items"},
        {ModelDataRole, "modelData"}
    };
}

void UpdatesModel::refresh()
{
    reload();
}

void UpdatesModel::clearAll()
{
    if (!m_db) return;
    m_db->clearNewChapters();
}

void UpdatesModel::removeChapter(const QString &id)
{
    if (!m_db) return;
    m_db->removeNewChapter(id);
}

void UpdatesModel::addUpdate(const QString &mangaId, const QString &mangaTitle, const QString &coverUrl,
                            const QString &provider, const QString &chapterId, const QString &chapterTitle,
                            double chapterNumber)
{
    if (!m_db) return;
    NewChapterEntity e;
    e.mangaId = mangaId;
    e.mangaTitle = mangaTitle;
    e.coverUrl = coverUrl;
    e.provider = provider;
    e.chapterId = chapterId;
    e.chapterTitle = chapterTitle;
    e.chapterNumber = chapterNumber;
    e.detectedAt = QDateTime::currentMSecsSinceEpoch();
    m_db->saveNewChapters({e});
}

void UpdatesModel::reload()
{
    beginResetModel();
    m_groups.clear();
    m_totalCount = 0;

    if (!m_db) {
        endResetModel();
        emit countChanged();
        emit totalNewChaptersChanged();
        return;
    }

    const auto chapters = m_db->getNewChapters();
    m_totalCount = static_cast<int>(chapters.size());

    const QDate today = QDate::currentDate();
    const QDate yesterday = today.addDays(-1);

    // Keep insertion order of groups
    std::vector<QString> groupTitles;
    std::map<QString, QVariantList> groupedMap;

    for (const auto &c : chapters) {
        const QDate d = QDateTime::fromMSecsSinceEpoch(c.detectedAt).date();
        QString groupTitle;
        if (d == today) {
            groupTitle = QStringLiteral("Today");
        } else if (d == yesterday) {
            groupTitle = QStringLiteral("Yesterday");
        } else if (d.year() == today.year()) {
            groupTitle = d.toString(QStringLiteral("MMMM d"));
        } else {
            groupTitle = d.toString(QStringLiteral("MMMM d, yyyy"));
        }

        if (groupedMap.find(groupTitle) == groupedMap.end()) {
            groupTitles.push_back(groupTitle);
        }

        QVariantMap mangaMap{
            {QStringLiteral("id"), c.mangaId},
            {QStringLiteral("title"), c.mangaTitle},
            {QStringLiteral("cover"), c.coverUrl.isEmpty() ? QStringLiteral("#1A2433") : c.coverUrl},
            {QStringLiteral("source"), c.provider},
            {QStringLiteral("provider"), c.provider},
            {QStringLiteral("inLibrary"), true}
        };

        const QString chapterLabel = c.chapterTitle.isEmpty()
            ? QStringLiteral("Ch. %1").arg(c.chapterNumber > 0 ? QString::number(c.chapterNumber) : c.chapterId)
            : QStringLiteral("Ch. %1 · %2").arg(c.chapterNumber > 0 ? QString::number(c.chapterNumber) : c.chapterId, c.chapterTitle);

        QVariantMap itemMap{
            {QStringLiteral("manga"), mangaMap},
            {QStringLiteral("chapter"), chapterLabel},
            {QStringLiteral("source"), c.provider},
            {QStringLiteral("provider"), c.provider},
            {QStringLiteral("id"), c.id},
            {QStringLiteral("chapterId"), c.chapterId},
            {QStringLiteral("chapterNumber"), c.chapterNumber},
            {QStringLiteral("detectedAt"), c.detectedAt}
        };

        groupedMap[groupTitle].append(itemMap);
    }

    m_groups.reserve(groupTitles.size());
    for (const auto &title : groupTitles) {
        m_groups.push_back(Group{title, groupedMap[title]});
    }

    endResetModel();
    emit countChanged();
    emit totalNewChaptersChanged();
}

} // namespace mangadl
