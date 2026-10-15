#pragma once

#include "MangaDatabase.h"

#include <QAbstractListModel>
#include <QStringList>

namespace mangadl {

class CategoryModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QStringList names READ names NOTIFY categoriesChanged)
    Q_PROPERTY(QStringList categoryChips READ categoryChips NOTIFY categoriesChanged)

public:
    enum Roles {
        ModelDataRole = Qt::UserRole + 1,
        IdRole,
        NameRole,
        SortOrderRole,
        MangaCountRole
    };
    Q_ENUM(Roles)

    explicit CategoryModel(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~CategoryModel() override = default;

    static CategoryModel &instance();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_items.size()); }
    QStringList names() const;
    QStringList categoryChips() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void addCategory(const QString &name);
    Q_INVOKABLE void renameCategory(const QString &id, const QString &newName);
    Q_INVOKABLE void deleteCategory(const QString &id);
    Q_INVOKABLE void reorder(const QStringList &orderedIds);
    Q_INVOKABLE QStringList getCategoriesForManga(const QString &provider, const QString &mangaId) const;
    Q_INVOKABLE void setCategoriesForManga(const QString &provider, const QString &mangaId, const QStringList &categoryIds);

signals:
    void countChanged();
    void categoriesChanged();

private:
    struct Item {
        CategoryEntity entity;
        int mangaCount = 0;
    };

    void reload();

    MangaDatabase *m_db = nullptr;
    std::vector<Item> m_items;
};

} // namespace mangadl
