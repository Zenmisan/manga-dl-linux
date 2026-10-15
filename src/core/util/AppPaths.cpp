#include "util/AppPaths.h"

#include <QByteArray>
#include <QDir>
#include <QStandardPaths>

namespace {

QString g_root;

// XDG spec: the variable is only honoured when it is an absolute path.
QString xdgBase(const char *envName, const char *homeRelativeFallback)
{
    const QString value = QString::fromLocal8Bit(qgetenv(envName));
    if (!value.isEmpty() && QDir::isAbsolutePath(value))
        return QDir::cleanPath(value);
    return QDir::cleanPath(QDir::homePath() + QLatin1Char('/') + QLatin1String(homeRelativeFallback));
}

QString resolve(const char *envName, const char *homeRelativeFallback, const char *overrideSubdir)
{
    if (!g_root.isEmpty())
        return QDir::cleanPath(g_root + QLatin1Char('/') + QLatin1String(overrideSubdir));
    return xdgBase(envName, homeRelativeFallback) + QStringLiteral("/manga-dl");
}

} // namespace

namespace AppPaths {

QString dataDir() { return resolve("XDG_DATA_HOME", ".local/share", "data"); }
QString configDir() { return resolve("XDG_CONFIG_HOME", ".config", "config"); }
QString cacheDir() { return resolve("XDG_CACHE_HOME", ".cache", "cache"); }
QString stateDir() { return resolve("XDG_STATE_HOME", ".local/state", "state"); }

QString extensionsDir() { return dataDir() + QStringLiteral("/extensions"); }
QString databasePath() { return dataDir() + QStringLiteral("/manga.db"); }
QString imageCacheDir() { return cacheDir() + QStringLiteral("/images"); }

QString defaultDownloadDir()
{
    if (!g_root.isEmpty())
        return QDir::cleanPath(g_root + QStringLiteral("/Downloads/manga-dl"));
    QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloads.isEmpty())
        downloads = QDir::homePath() + QStringLiteral("/Downloads");
    return QDir::cleanPath(downloads) + QStringLiteral("/manga-dl");
}

bool ensureDirs()
{
    bool ok = true;
    const QDir root;
    for (const QString &dir : {dataDir(), configDir(), cacheDir(), stateDir(), extensionsDir(), imageCacheDir()})
        ok = root.mkpath(dir) && ok;
    return ok;
}

void setRootOverride(const QString &root)
{
    g_root = root.isEmpty() ? QString() : QDir::cleanPath(root);
}

} // namespace AppPaths
