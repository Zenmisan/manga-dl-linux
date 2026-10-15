#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include <QJSEngine>

namespace mangadl {

class MangaDatabase;

class NotificationsController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList notices READ notices NOTIFY noticesChanged)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(QString selectedTab READ selectedTab WRITE setSelectedTab NOTIFY selectedTabChanged)
    Q_PROPERTY(QStringList tabs READ tabs NOTIFY tabsChanged)

public:
    explicit NotificationsController(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~NotificationsController() override = default;

    static NotificationsController &instance();
    static NotificationsController *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    QVariantList notices() const;
    int unreadCount() const;
    QString selectedTab() const { return m_selectedTab; }
    void setSelectedTab(const QString &tab);
    QStringList tabs() const;

    Q_INVOKABLE void markAllRead();
    Q_INVOKABLE void handleAction(const QString &action, const QVariantMap &data);
    Q_INVOKABLE void refresh();

signals:
    void noticesChanged();
    void unreadCountChanged();
    void selectedTabChanged();
    void tabsChanged();

private:
    struct NoticeItem {
        QString id;
        QString title;
        QString body;
        QString kind; // "info", "success", "error"
        QString icon;
        QString when;
        QString action;
        QString category; // "Chapters", "Downloads", "App"
        bool unread = true;
        qint64 timestamp = 0;
    };

    static QString formatRelativeTime(qint64 epochMs);

    MangaDatabase *m_db = nullptr;
    QString m_selectedTab = QStringLiteral("All");
    QList<NoticeItem> m_allNotices;
    QSet<QString> m_readNoticeIds;
};

} // namespace mangadl
