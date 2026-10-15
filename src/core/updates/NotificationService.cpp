#include "NotificationService.h"
#include "core/settings/AppSettings.h"
#include "core/util/AppLog.h"

#include <QDBusInterface>
#include <QDBusConnection>
#include <QProcess>

namespace mangadl {

NotificationService &NotificationService::instance()
{
    static NotificationService s_instance;
    return s_instance;
}

NotificationService::NotificationService(QObject *parent)
    : QObject(parent)
{
}

void NotificationService::notify(const QString &title, const QString &body, const QString &icon)
{
    auto *settings = AppSettings::instance();
    if (settings && !settings->desktopNotifications()) {
        return;
    }

    // Try org.freedesktop.Notifications via D-Bus
    if (QDBusConnection::sessionBus().isConnected()) {
        QDBusInterface notifyApp(
            QStringLiteral("org.freedesktop.Notifications"),
            QStringLiteral("/org/freedesktop/Notifications"),
            QStringLiteral("org.freedesktop.Notifications"),
            QDBusConnection::sessionBus());

        if (notifyApp.isValid()) {
            QVariantList args;
            args << QStringLiteral("manga-dl"); // app_name
            args << quint32(0);                 // replaces_id
            args << (icon.isEmpty() ? QStringLiteral("manga-dl") : icon); // app_icon
            args << title;                      // summary
            args << body;                       // body
            args << QStringList();              // actions
            args << QVariantMap();              // hints
            args << qint32(5000);               // expire_timeout (5s)

            notifyApp.callWithArgumentList(QDBus::NoBlock, QStringLiteral("Notify"), args);
            return;
        }
    }

    // Fallback: notify-send
    QStringList procArgs;
    procArgs << title << body;
    if (!icon.isEmpty()) {
        procArgs << QStringLiteral("-i") << icon;
    }
    QProcess::startDetached(QStringLiteral("notify-send"), procArgs);
}

} // namespace mangadl
