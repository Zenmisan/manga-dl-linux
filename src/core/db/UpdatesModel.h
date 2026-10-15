#pragma once

#include "MangaDatabase.h"

#include <QAbstractListModel>
#include <QVariantMap>
#include <QVariantList>

namespace mangadl {

class UpdatesModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int totalNewChapters READ totalNewChapters NOTIFY totalNewChaptersChanged)
    Q_PROPERTY(QString lastCheckedText READ lastCheckedText NOTIFY lastCheckedTextChanged)

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        ItemsRole,
        ModelDataRole
    };
    Q_ENUM(Roles)

    explicit UpdatesModel(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~UpdatesModel() override = default;

    static UpdatesModel &instance();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_groups.size()); }
    int totalNewChapters() const { return m_totalCount; }
    QString lastCheckedText() const { return m_lastCheckedText; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void removeChapter(const QString &id);
    Q_INVOKABLE void addUpdate(const QString &mangaId, const QString &mangaTitle, const QString &coverUrl,
                               const QString &provider, const QString &chapterId, const QString &chapterTitle,
                               double chapterNumber);

signals:
    void countChanged();
    void totalNewChaptersChanged();
    void lastCheckedTextChanged();

private:
    struct Group {
        QString title;
        QVariantList items;
    };

    void reload();

    MangaDatabase *m_db = nullptr;
    std::vector<Group> m_groups;
    int m_totalCount = 0;
    QString m_lastCheckedText = QStringLiteral("Just now");
};

} // namespace mangadl
