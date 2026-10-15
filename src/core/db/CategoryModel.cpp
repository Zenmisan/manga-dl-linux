#include "CategoryModel.h"

#include <QRegularExpression>
#include <QDateTime>

namespace mangadl {

CategoryModel::CategoryModel(MangaDatabase *db, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(db ? db : MangaDatabase::instance())
{
    if (m_db) {
        connect(m_db, &MangaDatabase::categoriesChanged, this, &CategoryModel::refresh);
        connect(m_db, &MangaDatabase::libraryChanged, this, &CategoryModel::refresh);
    }
    reload();
}

CategoryModel &CategoryModel::instance()
{
    static CategoryModel s_instance;
    return s_instance;
}

int CategoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant CategoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return QVariant();
    }

    const auto &item = m_items[static_cast<size_t>(index.row())];

    switch (role) {
    case Qt::DisplayRole:
    case ModelDataRole:
    case NameRole:
        return item.entity.name;
    case IdRole:
        return item.entity.id;
    case SortOrderRole:
        return item.entity.sortOrder;
    case MangaCountRole:
        return item.mangaCount;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> CategoryModel::roleNames() const
{
    return {
        {ModelDataRole, "modelData"},
        {IdRole, "id"},
        {NameRole, "name"},
        {SortOrderRole, "sortOrder"},
        {MangaCountRole, "mangaCount"}
    };
}

QStringList CategoryModel::names() const
{
    QStringList result;
    result.reserve(static_cast<qsizetype>(m_items.size()));
    for (const auto &item : m_items) {
        result.append(item.entity.name);
    }
    return result;
}

QStringList CategoryModel::categoryChips() const
{
    QStringList result;
    int totalManga = 0;
    if (m_db) {
        totalManga = static_cast<int>(m_db->getLibrary().size());
    }
    result.append(QStringLiteral("All · %1").arg(totalManga));
    for (const auto &item : m_items) {
        result.append(item.entity.name);
    }
    return result;
}

void CategoryModel::refresh()
{
    reload();
}

void CategoryModel::addCategory(const QString &name)
{
    if (!m_db || name.trimmed().isEmpty()) return;

    QString id = name.trimmed().toLower().replace(QRegularExpression(QStringLiteral("[^a-z0-9_]+")), QStringLiteral("_"));
    if (id.isEmpty()) {
        id = QStringLiteral("cat_%1").arg(QDateTime::currentMSecsSinceEpoch());
    }
    const int order = static_cast<int>(m_items.size());
    m_db->addCategory(id, name.trimmed(), order);
}

void CategoryModel::renameCategory(const QString &id, const QString &newName)
{
    if (!m_db || newName.trimmed().isEmpty()) return;

    auto cat = m_db->getCategory(id);
    const int order = cat ? cat->sortOrder : 0;
    m_db->addCategory(id, newName.trimmed(), order);
}

void CategoryModel::deleteCategory(const QString &id)
{
    if (!m_db) return;
    m_db->deleteCategory(id);
}

void CategoryModel::reorder(const QStringList &orderedIds)
{
    if (!m_db) return;
    for (int i = 0; i < orderedIds.size(); ++i) {
        const QString &id = orderedIds[i];
        auto cat = m_db->getCategory(id);
        if (cat) {
            m_db->addCategory(id, cat->name, i);
        }
    }
}

QStringList CategoryModel::getCategoriesForManga(const QString &provider, const QString &mangaId) const
{
    if (!m_db) return {};
    return m_db->getMangaCategories(provider, mangaId);
}

void CategoryModel::setCategoriesForManga(const QString &provider, const QString &mangaId, const QStringList &categoryIds)
{
    if (!m_db) return;
    m_db->setMangaCategories(provider, mangaId, categoryIds);
}

void CategoryModel::reload()
{
    beginResetModel();
    m_items.clear();

    if (m_db) {
        const auto cats = m_db->getCategories();
        m_items.reserve(cats.size());
        for (const auto &c : cats) {
            Item row;
            row.entity = c;
            row.mangaCount = m_db->getMangaCountInCategory(c.id);
            m_items.push_back(std::move(row));
        }
    }

    endResetModel();
    emit countChanged();
    emit categoriesChanged();
}

} // namespace mangadl
