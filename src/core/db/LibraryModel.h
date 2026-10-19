#pragma once

#include "MangaDatabase.h"

#include <QAbstractListModel>
#include <QVariantMap>

namespace mangadl {

class LibraryModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY filterChanged)
    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter NOTIFY filterChanged)
    Q_PROPERTY(QString contentType READ contentType WRITE setContentType NOTIFY filterChanged)
    Q_PROPERTY(QString filterMode READ filterMode WRITE setFilterMode NOTIFY filterChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int totalUnread READ totalUnread NOTIFY totalUnreadChanged)
    Q_PROPERTY(QVariantList continueReading READ continueReading NOTIFY continueReadingChanged)

public:
    enum Roles {
        ModelDataRole = Qt::UserRole + 1,
        MangaRole,
        IdRole,
        ProviderRole,
        TitleRole,
        CoverRole,
        UrlRole,
        TypeRole,
        TotalChaptersRole,
        UnreadRole,
        DownloadedRole,
        InLibraryRole,
        SourceRole
    };
    Q_ENUM(Roles)

    explicit LibraryModel(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~LibraryModel() override = default;

    static LibraryModel &instance();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString categoryFilter() const { return m_categoryFilter; }
    void setCategoryFilter(const QString &filter);

    QString searchFilter() const { return m_searchFilter; }
    void setSearchFilter(const QString &filter);

    QString contentType() const { return m_contentType; }
    void setContentType(const QString &type);

    QString filterMode() const { return m_filterMode; }
    void setFilterMode(const QString &mode);

    int sortMode() const { return m_sortMode; }
    void setSortMode(int mode);

    int count() const { return static_cast<int>(m_items.size()); }
    int totalUnread() const;
    QVariantList continueReading() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void add(const QString &provider, const QString &mangaId, const QString &title,
                         const QString &coverUrl, const QString &type = QStringLiteral("manga"));
    Q_INVOKABLE void remove(const QString &provider, const QString &mangaId);
    Q_INVOKABLE bool contains(const QString &provider, const QString &mangaId) const;

signals:
    void filterChanged();
    void sortModeChanged();
    void countChanged();
    void totalUnreadChanged();
    void continueReadingChanged();

private:
    struct Item {
        LibraryEntity entity;
        int unread = 0;
        bool downloaded = false;
        QVariantMap toMap() const;
    };

    void reload();

    MangaDatabase *m_db = nullptr;
    QString m_categoryFilter = QStringLiteral("All");
    QString m_searchFilter;
    QString m_contentType = QStringLiteral("all");
    QString m_filterMode = QStringLiteral("all");
    int m_sortMode = 0; // 0 = Last read/updated, 1 = Title A-Z, 2 = Unread
    std::vector<Item> m_items;
};

} // namespace mangadl
