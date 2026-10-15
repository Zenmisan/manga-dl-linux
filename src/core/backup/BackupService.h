#pragma once

#include <QObject>
#include <QString>
#include "MangadlBackupModels.h"
#include "TachibkModels.h"
#include "db/MangaDatabase.h"

namespace mangadl {

class BackupService : public QObject {
    Q_OBJECT

public:
    explicit BackupService(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~BackupService() override = default;

    void setDatabase(MangaDatabase *db) { m_db = db; }

    // Export operations
    MangaDlBackup createBackup() const;
    QByteArray exportBackupBytes() const;
    bool exportBackupFile(const QString &filePath) const;

    // Restore operations
    static bool isMangadlBackup(const QByteArray &data);
    MangadlRestoreSummary restoreBackup(const QByteArray &data);
    MangadlRestoreSummary restoreBackupFile(const QString &filePath);

    // Specific restore helpers
    MangadlRestoreSummary restoreMangadlBackup(const MangaDlBackup &backup);
    MangadlRestoreSummary restoreTachiyomiBackup(const TachiyomiBackupResult &backup);

signals:
    void backupExported(const QString &filePath);
    void backupRestored(const mangadl::MangadlRestoreSummary &summary);
    void backupFailed(const QString &errorMessage);

private:
    MangaDatabase *m_db = nullptr;
};

} // namespace mangadl
