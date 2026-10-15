#include "MangaDetailController.h"

#include "db/MangaDatabase.h"
#include "extensions/ExtensionManager.h"
#include "util/AppLog.h"

using namespace mangadl;

MangaDetailController::MangaDetailController(QObject *parent)
    : QObject(parent)
{
}

void MangaDetailController::load(const QString &provider, const QString &mangaId)
{
    if (provider.isEmpty() || mangaId.isEmpty())
        return;

    m_provider = provider;
    m_mangaId = mangaId;
    emit providerChanged();
    emit mangaIdChanged();

    reload();
}

void MangaDetailController::reload()
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;

    m_loading = true;
    m_error.clear();
    emit loadingChanged();
    emit errorChanged();

    m_inLibrary = MangaDatabase::instance()->isInLibrary(m_provider, m_mangaId);
    emit inLibraryChanged();

    ExtensionManager::instance().getMangaDetail(
        m_provider, m_mangaId,
        [this](MangaDetail detail, SourceError err) {
            m_loading = false;
            emit loadingChanged();

            if (!err.message.isEmpty() && detail.title.isEmpty() && detail.chapters.isEmpty()) {
                m_error = err.message;
                emit errorChanged();
                return;
            }

            QVariantMap detailMap;
            detailMap[QStringLiteral("id")] = detail.id.isEmpty() ? m_mangaId : detail.id;
            detailMap[QStringLiteral("title")] = detail.title;
            detailMap[QStringLiteral("coverUrl")] = detail.coverUrl;
            detailMap[QStringLiteral("url")] = detail.url;
            detailMap[QStringLiteral("description")] = detail.description;
            detailMap[QStringLiteral("status")] = detail.status.isEmpty() ? QStringLiteral("Ongoing") : detail.status;
            detailMap[QStringLiteral("genres")] = detail.genres;
            detailMap[QStringLiteral("authors")] = detail.authors;
            detailMap[QStringLiteral("artists")] = detail.artists;
            detailMap[QStringLiteral("provider")] = detail.provider.isEmpty() ? m_provider : detail.provider;
            detailMap[QStringLiteral("totalChapters")] = static_cast<int>(detail.chapters.size());

            m_detail = detailMap;
            emit detailChanged();

            // Chapters mapping
            QVariantList chList;
            chList.reserve(detail.chapters.size());

            for (const auto &ch : detail.chapters) {
                QVariantMap chMap;
                chMap[QStringLiteral("id")] = ch.id;
                chMap[QStringLiteral("title")] = ch.title;
                chMap[QStringLiteral("number")] = ch.number > 0.0 ? QString::number(ch.number) : ch.id;
                chMap[QStringLiteral("rawNumber")] = ch.number;
                chMap[QStringLiteral("released")] = ch.publishedAt;
                chMap[QStringLiteral("group")] = ch.scanlator;

                // Check DB progress
                auto prog = MangaDatabase::instance()->getProgress(m_provider, m_mangaId, ch.id);
                bool isRead = prog && prog->completed == 1;
                QString statusStr;
                if (isRead) {
                    statusStr = QStringLiteral("Read");
                } else if (prog && prog->page > 0) {
                    statusStr = QStringLiteral("Page %1").arg(prog->page);
                } else {
                    statusStr = QStringLiteral("Unread");
                }

                chMap[QStringLiteral("read")] = isRead;
                chMap[QStringLiteral("status")] = statusStr;
                chMap[QStringLiteral("downloaded")] = ch.downloaded;

                chList.append(chMap);
            }

            m_chapters = chList;
            emit chaptersChanged();
            emit totalChaptersChanged();

            updateResumeChapterAndUnread();

            // If already in library, update total chapters
            if (m_inLibrary) {
                LibraryEntity e;
                e.provider = m_provider;
                e.mangaId = m_mangaId;
                e.title = detail.title;
                e.coverUrl = detail.coverUrl;
                e.totalChapters = static_cast<int>(detail.chapters.size());
                MangaDatabase::instance()->addToLibrary(e);
            }
        });
}

void MangaDetailController::updateResumeChapterAndUnread()
{
    m_unreadCount = 0;
    QVariantMap resumeCandidate;

    for (int i = 0; i < m_chapters.size(); ++i) {
        const QVariantMap ch = m_chapters[i].toMap();
        const bool read = ch.value(QStringLiteral("read")).toBool();
        if (!read) {
            m_unreadCount++;
            if (resumeCandidate.isEmpty()) {
                resumeCandidate = ch;
            }
        }
    }

    if (resumeCandidate.isEmpty() && !m_chapters.isEmpty()) {
        resumeCandidate = m_chapters.first().toMap();
    }

    m_resumeChapter = resumeCandidate;
    emit resumeChapterChanged();
    emit unreadCountChanged();
}

void MangaDetailController::toggleLibrary()
{
    if (m_inLibrary) {
        removeFromLibrary();
    } else {
        addToLibrary();
    }
}

void MangaDetailController::addToLibrary()
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;

    LibraryEntity e;
    e.provider = m_provider;
    e.mangaId = m_mangaId;
    QString title = m_detail.value(QStringLiteral("title")).toString();
    if (title.isEmpty()) {
        title = m_mangaId;
    }
    e.title = title;
    e.coverUrl = m_detail.value(QStringLiteral("coverUrl")).toString();
    e.totalChapters = m_chapters.size();

    const auto *ext = ExtensionManager::instance().findById(m_provider);
    e.type = (ext && ext->type == ExtensionType::Novel)
        ? QStringLiteral("novel")
        : QStringLiteral("manga");

    MangaDatabase::instance()->addToLibrary(e);
    m_inLibrary = true;
    emit inLibraryChanged();
}

void MangaDetailController::removeFromLibrary()
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;

    MangaDatabase::instance()->removeFromLibrary(m_provider, m_mangaId);
    m_inLibrary = false;
    emit inLibraryChanged();
}

void MangaDetailController::markChapterRead(const QString &chapterId, bool read)
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty() || chapterId.isEmpty())
        return;

    double chNum = 0.0;
    for (int i = 0; i < m_chapters.size(); ++i) {
        QVariantMap ch = m_chapters[i].toMap();
        if (ch.value(QStringLiteral("id")).toString() == chapterId) {
            chNum = ch.value(QStringLiteral("rawNumber")).toDouble();
            ch[QStringLiteral("read")] = read;
            ch[QStringLiteral("status")] = read ? QStringLiteral("Read") : QStringLiteral("Unread");
            m_chapters[i] = ch;
            break;
        }
    }

    MangaDatabase::instance()->saveProgress(
        m_provider, m_mangaId, chapterId,
        chNum, 1, read);

    emit chaptersChanged();
    updateResumeChapterAndUnread();
}

QStringList MangaDetailController::getCategories() const
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return {};
    return MangaDatabase::instance()->getMangaCategories(m_provider, m_mangaId);
}

void MangaDetailController::setCategories(const QStringList &categoryIds)
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;
    MangaDatabase::instance()->setMangaCategories(m_provider, m_mangaId, categoryIds);
}
