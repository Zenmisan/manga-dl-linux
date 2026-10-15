#include "UpdateCheckerService.h"
#include "NotificationService.h"
#include "core/db/MangaDatabase.h"
#include "extensions/ExtensionManager.h"
#include "core/downloads/DownloadManager.h"
#include "core/settings/AppSettings.h"
#include "core/util/AppLog.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace mangadl {

UpdateCheckerService &UpdateCheckerService::instance()
{
    static UpdateCheckerService s_instance;
    return s_instance;
}

UpdateCheckerService::UpdateCheckerService(
    MangaDatabase *db,
    ExtensionManager *extensions,
    DownloadManager *downloads,
    QObject *parent)
    : QObject(parent)
    , m_db(db ? db : MangaDatabase::instance())
    , m_extensions(extensions ? extensions : &ExtensionManager::instance())
    , m_downloads(downloads ? downloads : &DownloadManager::instance())
{
    connect(&m_autoTimer, &QTimer::timeout, this, &UpdateCheckerService::onAutoTimerTriggered);
}

UpdateCheckerService::~UpdateCheckerService()
{
    stopAutoTimer();
}

double UpdateCheckerService::parseChapterNumber(const QString &numberStr)
{
    const QString trimmed = numberStr.trimmed();
    if (trimmed.isEmpty()) return 0.0;

    bool ok = false;
    double val = trimmed.toDouble(&ok);
    if (ok) return val;

    static const QRegularExpression regex(QStringLiteral(R"(\d+(\.\d+)?)"));
    auto match = regex.match(trimmed);
    if (match.hasMatch()) {
        val = match.captured(0).toDouble(&ok);
        if (ok) return val;
    }

    return 0.0;
}

void UpdateCheckerService::startAutoTimer(int intervalMinutes)
{
    m_autoTimer.stop();
    if (intervalMinutes > 0) {
        m_autoTimer.setInterval(intervalMinutes * 60 * 1000);
        m_autoTimer.start();
    }
}

void UpdateCheckerService::stopAutoTimer()
{
    m_autoTimer.stop();
}

void UpdateCheckerService::onAutoTimerTriggered()
{
    auto *settings = AppSettings::instance();
    if (settings && !settings->autoCheckUpdates()) {
        return;
    }
    checkUpdates(settings ? settings->autoDownloadNew() : false);
}

void UpdateCheckerService::checkUpdates(bool autoDownload)
{
    if (m_isChecking || !m_db || !m_extensions) {
        return;
    }

    m_isChecking = true;
    emit isCheckingChanged();
    emit checkStarted();

    auto *settings = AppSettings::instance();
    m_autoDownload = autoDownload || (settings && settings->autoDownloadNew());

    const auto library = m_db->getLibrary();
    m_pendingQueue.clear();

    const bool skipCompleted = settings && settings->skipCompletedSeries();

    for (const auto &item : library) {
        if (item.provider.compare(QStringLiteral("local"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        if (skipCompleted && item.type.compare(QStringLiteral("completed"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        m_pendingQueue.append(item);
    }

    m_totalToProcess = m_pendingQueue.size();
    m_processedCount = 0;
    m_foundNewChapters = 0;
    m_detectedChapters.clear();
    m_errors.clear();

    if (m_totalToProcess == 0) {
        m_isChecking = false;
        m_lastCheckTime = QDateTime::currentMSecsSinceEpoch();
        m_lastNewChaptersCount = 0;
        emit isCheckingChanged();
        emit lastCheckTimeChanged();
        emit lastNewChaptersCountChanged();
        emit checkFinished(0, 0, QStringList());
        return;
    }

    processNextManga();
}

void UpdateCheckerService::processNextManga()
{
    if (m_pendingQueue.isEmpty()) {
        // Complete!
        m_isChecking = false;
        m_lastCheckTime = QDateTime::currentMSecsSinceEpoch();
        m_lastNewChaptersCount = m_foundNewChapters;

        auto *settings = AppSettings::instance();
        if (settings) {
            settings->setLastUpdateCheck(m_lastCheckTime);
        }

        if (!m_detectedChapters.isEmpty()) {
            emit newChaptersDetected(m_detectedChapters);

            QSet<QString> seriesTitles;
            for (const auto &c : m_detectedChapters) {
                seriesTitles.insert(c.mangaTitle);
            }

            const QString notifBody = m_detectedChapters.size() == 1
                ? QStringLiteral("%1 · %2").arg(m_detectedChapters.first().mangaTitle, m_detectedChapters.first().chapterTitle)
                : QStringLiteral("%1 new chapters available across %2 series.").arg(m_detectedChapters.size()).arg(seriesTitles.size());

            NotificationService::instance().notify(
                QStringLiteral("New Chapters Released"),
                notifBody,
                QStringLiteral("manga-dl"));
        }

        emit isCheckingChanged();
        emit lastCheckTimeChanged();
        emit lastNewChaptersCountChanged();
        emit checkFinished(m_processedCount, m_foundNewChapters, m_errors);
        return;
    }

    const auto manga = m_pendingQueue.takeFirst();
    m_processedCount++;
    emit checkProgress(m_processedCount, m_totalToProcess, manga.title);

    m_extensions->getMangaDetail(
        manga.provider, manga.mangaId,
        [this, manga](MangaDetail detail, SourceError err) {
            if (!err.message.isEmpty() && detail.chapters.isEmpty()) {
                m_errors.append(QStringLiteral("%1: %2").arg(manga.title, err.message));
            } else if (!detail.chapters.isEmpty()) {
                const int latestCount = detail.chapters.size();

                if (manga.totalChapters == 0) {
                    // Initial baseline
                    m_db->updateTotalChapters(manga.provider, manga.mangaId, latestCount);
                } else if (latestCount > manga.totalChapters) {
                    const int newCount = latestCount - manga.totalChapters;

                    // Sort chapters descending by chapter number
                    auto sortedChapters = detail.chapters;
                    std::sort(sortedChapters.begin(), sortedChapters.end(), [](const ChapterInfo &a, const ChapterInfo &b) {
                        return a.number > b.number;
                    });

                    const qint64 now = QDateTime::currentMSecsSinceEpoch();
                    QList<NewChapterEntity> detected;

                    for (int i = 0; i < std::min(newCount, static_cast<int>(sortedChapters.size())); ++i) {
                        const auto &ch = sortedChapters[i];
                        NewChapterEntity entity;
                        entity.id = QStringLiteral("%1/%2/%3").arg(manga.provider, manga.mangaId, ch.id);
                        entity.mangaId = manga.mangaId;
                        entity.mangaTitle = manga.title;
                        entity.coverUrl = manga.coverUrl.isEmpty() ? detail.coverUrl : manga.coverUrl;
                        entity.provider = manga.provider;
                        entity.chapterId = ch.id;
                        entity.chapterTitle = ch.title.trimmed().isEmpty() ? QStringLiteral("Chapter %1").arg(ch.number) : ch.title;
                        entity.chapterNumber = ch.number;
                        entity.detectedAt = now;
                        detected.append(entity);
                    }

                    if (!detected.isEmpty()) {
                        m_db->saveNewChapters(detected);
                        m_detectedChapters.append(detected);
                        m_foundNewChapters += detected.size();

                        if (m_autoDownload && m_downloads) {
                            for (const auto &ch : detected) {
                                m_downloads->enqueue(
                                    ch.provider,
                                    ch.mangaId,
                                    ch.mangaTitle,
                                    ch.chapterId,
                                    ch.chapterTitle,
                                    ch.chapterNumber);
                            }
                        }
                    }

                    m_db->updateTotalChapters(manga.provider, manga.mangaId, latestCount);
                }
            }

            // Throttle slightly between requests (50ms) to prevent overwhelming scrapers
            QTimer::singleShot(50, this, [this]() {
                processNextManga();
            });
        });
}

} // namespace mangadl
