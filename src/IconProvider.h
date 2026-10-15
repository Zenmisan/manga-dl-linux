#pragma once

#include <QQuickImageProvider>

// Serves the app's line icons as image://icon/<Name>?color=<css color>.
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    static bool has(const QString &name);
};
