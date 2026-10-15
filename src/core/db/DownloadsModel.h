#pragma once

#include "MangaDatabase.h"

#include <QAbstractListModel>
#include <QVariantMap>

namespace mangadl {

class DownloadsModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString filterTab READ filterTab WRITE setFilterTab NOTIFY filterTabChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int queueCount READ queueCount NOTIFY countsChanged)
    Q_PROPERTY(int finishedCount READ finishedCount NOTIFY countsChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY countsChanged)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY pauseStateChanged)
    Q_PROPERTY(QString storageUsed READ storageUsed NOTIFY storageUsedChanged)

public:
    enum Roles {
        ModelDataRole = Qt::UserRole + 1,
        MangaRole,
        ChapterRole,
        StatusRole,
        ProgressRole,
        StateRole,
        IdRole,
        CbzPathRole
    };
    Q_ENUM(Roles)

    explicit DownloadsModel(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~DownloadsModel() override = default;

    static DownloadsModel &instance();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filterTab() const { return m_filterTab; }
    void setFilterTab(const QString &tab);

    int count() const { return static_cast<int>(m_items.size()); }
    int queueCount() const { return m_queueCount; }
    int finishedCount() const { return m_finishedCount; }
    int failedCount() const { return m_failedCount; }
    bool isPaused() const { return m_isPaused; }
    QString storageUsed() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void clearFinished();
    Q_INVOKABLE void pauseDownload(const QString &id);
    Q_INVOKABLE void resumeDownload(const QString &id);
    Q_INVOKABLE void retryDownload(const QString &id);
    Q_INVOKABLE void cancelDownload(const QString &id);
    Q_INVOKABLE void enqueue(const QString &provider, const QString &mangaId, const QString &mangaTitle,
                             const QString &chapterId, const QString &chapterTitle, double chapterNumber,
                             const QString &coverUrl = QString());

signals:
    void filterTabChanged();
    void countChanged();
    void countsChanged();
    void pauseStateChanged();
    void storageUsedChanged();

private:
    struct Item {
        DownloadEntity entity;
        QString chapter;
        QString statusText;
        double progressFraction = 0.0;
        QString state; // "downloading" | "queued" | "paused" | "failed" | "done"
        QString coverUrl;
        QVariantMap toMap() const;
    };

    void reload();

    MangaDatabase *m_db = nullptr;
    QString m_filterTab = QStringLiteral("queue"); // "queue" | "finished" | "failed" | "all"
    std::vector<Item> m_items;
    int m_queueCount = 0;
    int m_finishedCount = 0;
    int m_failedCount = 0;
    bool m_isPaused = false;
};

} // namespace mangadl
