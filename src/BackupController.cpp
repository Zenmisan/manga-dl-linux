#include "BackupController.h"
#include "src/core/settings/AppSettings.h"
#include "src/core/util/AppPaths.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QThread>

namespace mangadl {

BackupController::BackupController(QObject *parent)
    : QObject(parent)
{
    m_backupService = std::make_unique<BackupService>(MangaDatabase::instance(), this);
}

QString BackupController::lastBackupDate() const
{
    return AppSettings::instance()->lastBackupDate();
}

QString BackupController::autoBackupSchedule() const
{
    return AppSettings::instance()->autoBackupSchedule();
}

void BackupController::setAutoBackupSchedule(const QString &schedule)
{
    AppSettings::instance()->setAutoBackupSchedule(schedule);
    emit autoBackupScheduleChanged();
}

void BackupController::setBusy(bool busy)
{
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit busyChanged();
    }
}

void BackupController::setErrorMessage(const QString &msg)
{
    if (m_errorMessage != msg) {
        m_errorMessage = msg;
        emit errorMessageChanged();
    }
}

void BackupController::createBackup(const QString &targetPath)
{
    setBusy(true);
    setErrorMessage(QString());

    QString path = targetPath;
    if (path.startsWith(QStringLiteral("file://"))) {
        path = QUrl(path).toLocalFile();
    }

    if (path.isEmpty() || QFileInfo(path).isDir()) {
        const QString dir = path.isEmpty() ? AppPaths::defaultDownloadDir() : path;
        QDir().mkpath(dir);
        const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"));
        path = QDir(dir).filePath(QStringLiteral("manga-dl_backup_%1.mangadl").arg(timestamp));
    }

    QThread::create([this, path]() {
        const bool ok = m_backupService->exportBackupFile(path);
        QMetaObject::invokeMethod(this, [this, ok, path]() {
            setBusy(false);
            if (ok) {
                const QString dateStr = QDateTime::currentDateTime().toString(QStringLiteral("MMM d, yyyy"));
                AppSettings::instance()->setLastBackupDate(dateStr);
                emit backupDateChanged();
                emit backupCreated(path);
            } else {
                setErrorMessage(QStringLiteral("Failed to write backup file: ") + path);
            }
        });
    })->start();
}

void BackupController::restoreBackup(const QString &filePath)
{
    setBusy(true);
    setErrorMessage(QString());

    QString path = filePath;
    if (path.startsWith(QStringLiteral("file://"))) {
        path = QUrl(path).toLocalFile();
    }

    QThread::create([this, path]() {
        const auto summary = m_backupService->restoreBackupFile(path);
        QMetaObject::invokeMethod(this, [this, summary]() {
            setBusy(false);
            QVariantMap map;
            map[QStringLiteral("mangaRestored")] = summary.mangaRestored;
            map[QStringLiteral("progressRestored")] = summary.progressRestored;
            map[QStringLiteral("categoriesRestored")] = summary.categoriesRestored;
            map[QStringLiteral("trackerBindsRestored")] = summary.trackerBindsRestored;
            m_lastSummary = map;
            emit lastSummaryChanged();

            emit backupRestored(summary.mangaRestored,
                                summary.progressRestored,
                                summary.categoriesRestored,
                                summary.trackerBindsRestored);
        });
    })->start();
}

} // namespace mangadl
