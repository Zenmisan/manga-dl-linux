#include "SystemController.h"
#include "core/settings/AppSettings.h"
#include "core/util/AppPaths.h"
#include "core/util/AppLog.h"

#include <QStorageInfo>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QStandardPaths>

namespace mangadl {

SystemController &SystemController::instance()
{
    static SystemController s_instance;
    return s_instance;
}

SystemController::SystemController(QObject *parent)
    : QObject(parent)
{
    refreshStorage();
}

QString SystemController::downloadLocation() const
{
    auto *settings = AppSettings::instance();
    return settings ? settings->downloadPath() : AppPaths::defaultDownloadDir();
}

void SystemController::setDownloadLocation(const QString &path)
{
    auto *settings = AppSettings::instance();
    if (settings) {
        settings->setDownloadPath(path);
        emit downloadLocationChanged();
        refreshStorage();
    }
}

qint64 SystemController::calculateDirectorySize(const QString &path)
{
    qint64 totalSize = 0;
    QDir dir(path);
    if (!dir.exists()) {
        return 0;
    }

    QDirIterator it(path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        totalSize += it.fileInfo().size();
    }

    return totalSize;
}

QString SystemController::formatBytes(qint64 bytes)
{
    if (bytes >= 1024LL * 1024LL * 1024LL) {
        return QStringLiteral("%1 GB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
    if (bytes >= 1024LL * 1024LL) {
        return QStringLiteral("%1 MB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0), 0, 'f', 1);
    }
    if (bytes >= 1024LL) {
        return QStringLiteral("%1 KB").arg(static_cast<double>(bytes) / 1024.0, 0, 'f', 1);
    }
    return QStringLiteral("%1 B").arg(bytes);
}

void SystemController::refreshStorage()
{
    const QString dlPath = downloadLocation();
    QStorageInfo storage(dlPath);
    if (!storage.isValid() || !storage.isReady()) {
        storage = QStorageInfo::root();
    }

    m_totalDiskBytes = storage.bytesTotal();
    m_freeDiskBytes = storage.bytesAvailable();
    m_usedDiskBytes = m_totalDiskBytes > m_freeDiskBytes ? (m_totalDiskBytes - m_freeDiskBytes) : 0;
    m_usedDiskFraction = m_totalDiskBytes > 0 ? (static_cast<double>(m_usedDiskBytes) / static_cast<double>(m_totalDiskBytes)) : 0.0;

    m_totalDiskFormatted = formatBytes(m_totalDiskBytes);
    m_freeDiskFormatted = formatBytes(m_freeDiskBytes);
    m_usedDiskFormatted = formatBytes(m_usedDiskBytes);

    m_downloadDirBytes = calculateDirectorySize(dlPath);
    m_downloadDirFormatted = formatBytes(m_downloadDirBytes);

    m_cacheDirBytes = calculateDirectorySize(AppPaths::imageCacheDir());
    m_cacheDirFormatted = formatBytes(m_cacheDirBytes);

    emit storageChanged();
}

void SystemController::clearImageCache()
{
    const QString cacheDir = AppPaths::imageCacheDir();
    QDir dir(cacheDir);
    if (dir.exists()) {
        dir.removeRecursively();
        dir.mkpath(QStringLiteral("."));
    }

    refreshStorage();
    emit cacheCleared();
}

bool SystemController::setAutostart(bool enabled)
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString autostartDir = configDir + QStringLiteral("/autostart");
    const QString desktopFile = autostartDir + QStringLiteral("/manga-dl.desktop");

    if (!enabled) {
        if (QFile::exists(desktopFile)) {
            return QFile::remove(desktopFile);
        }
        return true;
    }

    QDir().mkpath(autostartDir);

    QFile file(desktopFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    const QString appPath = QCoreApplication::applicationFilePath();
    const QString content = QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=manga-dl\n"
        "Comment=Manga & Web Novel Reader\n"
        "Exec=%1\n"
        "Icon=manga-dl\n"
        "Terminal=false\n"
        "Categories=Utility;Viewer;\n"
    ).arg(appPath);

    file.write(content.toUtf8());
    file.close();
    return true;
}

bool SystemController::isAutostartEnabled()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString desktopFile = configDir + QStringLiteral("/autostart/manga-dl.desktop");
    return QFile::exists(desktopFile);
}

} // namespace mangadl
