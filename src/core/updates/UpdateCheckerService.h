#pragma once

#include <QObject>
#include <QTimer>
#include <QList>
#include <QStringList>
#include "core/db/Entities.h"
#include "extensions/ExtensionManager.h"

namespace mangadl {

class MangaDatabase;
class DownloadManager;
using ::ExtensionManager;

class UpdateCheckerService : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isChecking READ isChecking NOTIFY isCheckingChanged)
    Q_PROPERTY(qint64 lastCheckTime READ lastCheckTime NOTIFY lastCheckTimeChanged)
    Q_PROPERTY(int lastNewChaptersCount READ lastNewChaptersCount NOTIFY lastNewChaptersCountChanged)

public:
    explicit UpdateCheckerService(
        MangaDatabase *db = nullptr,
        ExtensionManager *extensions = nullptr,
        DownloadManager *downloads = nullptr,
        QObject *parent = nullptr);
    ~UpdateCheckerService() override;

    static UpdateCheckerService &instance();

    bool isChecking() const { return m_isChecking; }
    qint64 lastCheckTime() const { return m_lastCheckTime; }
    int lastNewChaptersCount() const { return m_lastNewChaptersCount; }

    Q_INVOKABLE void checkUpdates(bool autoDownload = false);
    Q_INVOKABLE void startAutoTimer(int intervalMinutes = 30);
    Q_INVOKABLE void stopAutoTimer();

    static double parseChapterNumber(const QString &numberStr);

signals:
    void checkStarted();
    void checkProgress(int current, int total, const QString &title);
    void checkFinished(int totalChecked, int newChaptersFound, const QStringList &errors);
    void newChaptersDetected(const QList<NewChapterEntity> &newChapters);
    void isCheckingChanged();
    void lastCheckTimeChanged();
    void lastNewChaptersCountChanged();

private slots:
    void onAutoTimerTriggered();

private:
    void processNextManga();

    MangaDatabase *m_db = nullptr;
    ExtensionManager *m_extensions = nullptr;
    DownloadManager *m_downloads = nullptr;

    QTimer m_autoTimer;
    bool m_isChecking = false;
    bool m_autoDownload = false;
    qint64 m_lastCheckTime = 0;
    int m_lastNewChaptersCount = 0;

    QList<LibraryEntity> m_pendingQueue;
    int m_totalToProcess = 0;
    int m_processedCount = 0;
    int m_foundNewChapters = 0;
    QList<NewChapterEntity> m_detectedChapters;
    QStringList m_errors;
};

} // namespace mangadl
