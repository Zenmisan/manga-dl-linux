#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

/// Build facts for QML (`AppInfo.debugPages`, `AppInfo.version`).
class AppInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool debugPages READ debugPages CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    using QObject::QObject;

    /// True in Debug builds: shows the "All pages" index in the sidebar.
    bool debugPages() const
    {
#ifdef MANGADL_DEBUG_PAGES
        return qEnvironmentVariableIntValue("MANGADL_HIDE_DEBUG") == 0;
#else
        return qEnvironmentVariableIntValue("MANGADL_DEBUG_PAGES") != 0;
#endif
    }

    QString version() const { return QStringLiteral(MANGADL_VERSION); }
};
