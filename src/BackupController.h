#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QString>
#include <QVariantMap>
#include <memory>
#include "src/core/backup/BackupService.h"

namespace mangadl {

class BackupController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString lastBackupDate READ lastBackupDate NOTIFY backupDateChanged)
    Q_PROPERTY(QString autoBackupSchedule READ autoBackupSchedule WRITE setAutoBackupSchedule NOTIFY autoBackupScheduleChanged)
    Q_PROPERTY(QVariantMap lastSummary READ lastSummary NOTIFY lastSummaryChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit BackupController(QObject *parent = nullptr);
    ~BackupController() override = default;

    bool isBusy() const { return m_isBusy; }
    QString lastBackupDate() const;
    QString autoBackupSchedule() const;
    void setAutoBackupSchedule(const QString &schedule);
    QVariantMap lastSummary() const { return m_lastSummary; }
    QString errorMessage() const { return m_errorMessage; }

    Q_INVOKABLE void createBackup(const QString &targetPath = QString());
    Q_INVOKABLE void restoreBackup(const QString &filePath);

signals:
    void busyChanged();
    void backupDateChanged();
    void autoBackupScheduleChanged();
    void lastSummaryChanged();
    void errorMessageChanged();
    void backupCreated(const QString &filePath);
    void backupRestored(int mangaCount, int progressCount, int categoryCount, int trackerCount);

private:
    void setBusy(bool busy);
    void setErrorMessage(const QString &msg);

    std::unique_ptr<BackupService> m_backupService;
    bool m_isBusy = false;
    QVariantMap m_lastSummary;
    QString m_errorMessage;
};

} // namespace mangadl
