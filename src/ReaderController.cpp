#include "ReaderController.h"

#include "db/MangaDatabase.h"
#include "downloads/CbzBuilder.h"
#include "local/EpubParser.h"
#include "extensions/ExtensionManager.h"
#include "settings/AppSettings.h"
#include "util/AppLog.h"
#include <QFile>
#include <QUrl>

using namespace mangadl;

ReaderController::ReaderController(QObject *parent)
    : QObject(parent)
{
}

void ReaderController::setCurrentPage(int p)
{
    const int clamped = qBound(1, p, qMax(1, m_totalPages));
    if (m_currentPage != clamped) {
        m_currentPage = clamped;
        emit currentPageChanged();
    }
}

void ReaderController::setNavigation(const QString &prevId, const QString &nextId)
{
    m_prevChapterId = prevId;
    m_nextChapterId = nextId;
    emit navigationChanged();
}

void ReaderController::loadChapter(const QString &provider, const QString &mangaId, const QString &chapterId,
                                   const QString &mangaTitle, const QString &coverUrl,
                                   const QString &chapterTitle, double chapterNumber)
{
    if (provider.isEmpty() || chapterId.isEmpty())
        return;

    m_provider = provider;
    m_mangaId = mangaId;
    m_chapterId = chapterId;
    m_mangaTitle = mangaTitle;
    m_coverUrl = coverUrl;
    m_chapterTitle = chapterTitle;
    m_chapterNumber = chapterNumber;
    emit chapterLoaded();

    const auto *ext = ExtensionManager::instance().findById(m_provider);
    m_isNovel = (ext && ext->type == ExtensionType::Novel);
    emit isNovelChanged();

    // Check last saved progress
    auto prog = MangaDatabase::instance()->getProgress(m_provider, m_mangaId, m_chapterId);
    m_currentPage = (prog && prog->page > 0) ? prog->page : 1;
    emit currentPageChanged();

    m_loading = true;
    m_error.clear();
    emit loadingChanged();
    emit errorChanged();

    m_pages.clear();
    emit pagesChanged();

    m_novelText.clear();
    emit novelTextChanged();

    // Record history
    MangaDatabase::instance()->recordHistory(
        m_provider, m_mangaId, m_mangaTitle, m_chapterId, m_chapterTitle, m_coverUrl);

    // Check if downloaded / local chapter is available on disk
    const QString downloadId = QStringLiteral("%1/%2/%3").arg(m_provider, m_mangaId, m_chapterId);
    auto dl = MangaDatabase::instance()->getDownloadById(downloadId);
    if (dl && dl->status == QStringLiteral("completed") && !dl->cbzPath.isEmpty() && QFile::exists(dl->cbzPath)) {
        const QString path = dl->cbzPath;
        if (path.endsWith(QStringLiteral(".cbz"), Qt::CaseInsensitive) || path.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
            const QStringList entries = CbzBuilder::getImageEntries(path);
            if (!entries.isEmpty()) {
                QStringList pages;
                pages.reserve(entries.size());
                for (const auto &entry : entries) {
                    pages.append(QStringLiteral("image://cbz/%1?entry=%2")
                                     .arg(QString::fromUtf8(QUrl::toPercentEncoding(path)),
                                          QString::fromUtf8(QUrl::toPercentEncoding(entry))));
                }
                m_loading = false;
                emit loadingChanged();
                m_pages = pages;
                m_totalPages = static_cast<int>(m_pages.size());
                emit pagesChanged();
                emit totalPagesChanged();
                if (m_currentPage > m_totalPages && m_totalPages > 0) {
                    m_currentPage = m_totalPages;
                    emit currentPageChanged();
                }
                return;
            }
        } else if (path.endsWith(QStringLiteral(".html"), Qt::CaseInsensitive)) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly)) {
                m_loading = false;
                emit loadingChanged();
                m_novelText = QString::fromUtf8(f.readAll());
                m_totalPages = 1;
                m_isNovel = true;
                emit isNovelChanged();
                emit novelTextChanged();
                emit totalPagesChanged();
                return;
            }
        } else if (path.endsWith(QStringLiteral(".epub"), Qt::CaseInsensitive)) {
            const auto book = EpubParser::parse(path);
            if (!book.chapters.isEmpty()) {
                m_loading = false;
                emit loadingChanged();
                m_novelText = book.chapters[0].html;
                m_totalPages = 1;
                m_isNovel = true;
                emit isNovelChanged();
                emit novelTextChanged();
                emit totalPagesChanged();
                return;
            }
        }
    }

    if (m_isNovel) {
        ExtensionManager::instance().getChapterText(
            m_provider, m_chapterId,
            [this](QString text, SourceError err) {
                m_loading = false;
                emit loadingChanged();

                if (!err.message.isEmpty() && text.isEmpty()) {
                    m_error = err.message;
                    emit errorChanged();
                    return;
                }

                m_novelText = text;
                m_totalPages = 1;
                emit novelTextChanged();
                emit totalPagesChanged();
            });
    } else {
        ExtensionManager::instance().getPages(
            m_provider, m_chapterId,
            [this](QList<PageInfo> pageList, SourceError err) {
                m_loading = false;
                emit loadingChanged();

                if (!err.message.isEmpty() && pageList.isEmpty()) {
                    m_error = err.message;
                    emit errorChanged();
                    return;
                }

                QStringList list;
                list.reserve(pageList.size());
                for (const auto &p : pageList) {
                    list.append(p.url);
                }

                m_pages = list;
                m_totalPages = static_cast<int>(m_pages.size());
                emit pagesChanged();
                emit totalPagesChanged();

                if (m_currentPage > m_totalPages && m_totalPages > 0) {
                    m_currentPage = m_totalPages;
                    emit currentPageChanged();
                }
            });
    }
}

void ReaderController::saveProgress(int page, int totalPages, bool completed)
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty() || m_chapterId.isEmpty())
        return;

    m_currentPage = page;
    m_totalPages = totalPages;

    MangaDatabase::instance()->saveProgress(
        m_provider, m_mangaId, m_chapterId,
        m_chapterNumber, page, completed);

    auto *settings = AppSettings::instance();
    if (settings && settings->incognito()) {
        // Incognito mode: do not write to reading history
        return;
    }

    MangaDatabase::instance()->recordHistory(
        m_provider, m_mangaId, m_mangaTitle, m_chapterId, m_chapterTitle, m_coverUrl);
}
