#pragma once

#include "MangaDatabase.h"

#include <QAbstractListModel>
#include <QVariantMap>

namespace mangadl {

class HistoryModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter NOTIFY filterChanged)
    Q_PROPERTY(QString timeRange READ timeRange WRITE setTimeRange NOTIFY rangeChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        ModelDataRole = Qt::UserRole + 1,
        MangaRole,
        WhereRole,
        WhenRole,
        ProviderRole,
        MangaIdRole,
        ChapterIdRole,
        ChapterTitleRole,
        ReadAtRole
    };
    Q_ENUM(Roles)

    explicit HistoryModel(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~HistoryModel() override = default;

    static HistoryModel &instance();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString searchFilter() const { return m_searchFilter; }
    void setSearchFilter(const QString &filter);

    QString timeRange() const { return m_timeRange; }
    void setTimeRange(const QString &range);

    int count() const { return static_cast<int>(m_items.size()); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void removeItem(int index);
    Q_INVOKABLE void remove(const QString &provider, const QString &mangaId, const QString &chapterId);
    Q_INVOKABLE void record(const QString &provider, const QString &mangaId, const QString &mangaTitle,
                            const QString &chapterId, const QString &chapterTitle, const QString &coverUrl);

signals:
    void filterChanged();
    void rangeChanged();
    void countChanged();

private:
    struct Item {
        HistoryEntity entity;
        QString where;
        QString when;
        QVariantMap toMap() const;
    };

    void reload();

    MangaDatabase *m_db = nullptr;
    QString m_searchFilter;
    QString m_timeRange = QStringLiteral("All time");
    std::vector<Item> m_items;
};

} // namespace mangadl
