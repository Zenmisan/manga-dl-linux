#include "MangaDetailController.h"

#include <QRegularExpression>
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

    m_pinned = MangaDatabase::instance()->isMangaPinned(m_provider, m_mangaId);
    emit pinnedChanged();

    m_muted = MangaDatabase::instance()->isMangaMuted(m_provider, m_mangaId);
    emit mutedChanged();

    auto noteEntity = MangaDatabase::instance()->getMangaNote(m_provider, m_mangaId);
    m_personalRating = noteEntity ? noteEntity->rating : 0;
    m_personalNote = noteEntity ? noteEntity->note : QString();
    emit personalNoteChanged();

    auto ovr = MangaDatabase::instance()->getMangaOverride(m_provider, m_mangaId);
    m_hasOverride = ovr.has_value();
    emit overrideChanged();

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
            const auto *ext = ExtensionManager::instance().findById(m_provider);
            const bool isNovelSource = (ext && ext->type == ExtensionType::Novel);
            detailMap[QStringLiteral("type")] = isNovelSource ? QStringLiteral("novel") : QStringLiteral("manga");
            detailMap[QStringLiteral("totalChapters")] = static_cast<int>(detail.chapters.size());

            auto ovrCheck = MangaDatabase::instance()->getMangaOverride(m_provider, m_mangaId);
            if (ovrCheck) {
                if (!ovrCheck->customTitle.isEmpty()) detailMap[QStringLiteral("title")] = ovrCheck->customTitle;
                if (!ovrCheck->customCoverUrl.isEmpty()) detailMap[QStringLiteral("coverUrl")] = ovrCheck->customCoverUrl;
                if (!ovrCheck->customDescription.isEmpty()) detailMap[QStringLiteral("description")] = ovrCheck->customDescription;
            }

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
                chMap[QStringLiteral("bookmarked")] = MangaDatabase::instance()->isChapterBookmarked(m_provider, m_mangaId, ch.id);

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
    QVariantMap lowestUnreadChapter;
    double lowestUnreadNum = 1e9;

    QVariantMap highestReadChapter;
    double highestReadNum = -1.0;

    QList<QVariantMap> unreadList;

    static const QRegularExpression numRegex(QStringLiteral(R"([\d.]+)"));

    for (int i = 0; i < m_chapters.size(); ++i) {
        const QVariantMap ch = m_chapters[i].toMap();
        const bool read = ch.value(QStringLiteral("read")).toBool();
        double num = ch.value(QStringLiteral("rawNumber")).toDouble();
        if (num <= 0.0) {
            const QString numStr = ch.value(QStringLiteral("number")).toString();
            const auto match = numRegex.match(numStr);
            if (match.hasMatch()) {
                num = match.captured(0).toDouble();
            }
        }

        if (read) {
            if (num > highestReadNum) {
                highestReadNum = num;
                highestReadChapter = ch;
            }
        } else {
            m_unreadCount++;
            unreadList.append(ch);
            if (num > 0.0 && num < lowestUnreadNum) {
                lowestUnreadNum = num;
                lowestUnreadChapter = ch;
            }
        }
    }

    QVariantMap resumeCandidate;
    if (highestReadNum >= 0) {
        // Find the next unread chapter directly after the highest read one
        double nextNum = 1e9;
        for (const auto &ch : unreadList) {
            double num = ch.value(QStringLiteral("rawNumber")).toDouble();
            if (num <= 0.0) {
                const auto match = numRegex.match(ch.value(QStringLiteral("number")).toString());
                if (match.hasMatch()) num = match.captured(0).toDouble();
            }
            if (num > highestReadNum && num < nextNum) {
                nextNum = num;
                resumeCandidate = ch;
            }
        }
    }

    if (resumeCandidate.isEmpty() && !lowestUnreadChapter.isEmpty()) {
        resumeCandidate = lowestUnreadChapter;
    } else if (resumeCandidate.isEmpty() && !unreadList.isEmpty()) {
        // If chapter numbers are not distinct, pick the last item (earliest in descending list)
        resumeCandidate = unreadList.last();
    } else if (resumeCandidate.isEmpty() && !m_chapters.isEmpty()) {
        resumeCandidate = m_chapters.last().toMap();
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

void MangaDetailController::markPreviousChaptersRead(const QString &chapterId)
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty() || chapterId.isEmpty())
        return;

    int targetIdx = -1;
    for (int i = 0; i < m_chapters.size(); ++i) {
        if (m_chapters[i].toMap().value(QStringLiteral("id")).toString() == chapterId) {
            targetIdx = i;
            break;
        }
    }

    if (targetIdx < 0)
        return;

    // Mark all chapters after target in the list (or previous chapters) as read
    for (int i = targetIdx; i < m_chapters.size(); ++i) {
        QVariantMap ch = m_chapters[i].toMap();
        const QString chId = ch.value(QStringLiteral("id")).toString();
        const double chNum = ch.value(QStringLiteral("rawNumber")).toDouble();
        ch[QStringLiteral("read")] = true;
        ch[QStringLiteral("status")] = QStringLiteral("Read");
        m_chapters[i] = ch;
        MangaDatabase::instance()->saveProgress(m_provider, m_mangaId, chId, chNum, 1, true);
    }

    emit chaptersChanged();
    updateResumeChapterAndUnread();
}

void MangaDetailController::savePersonalNote(int rating, const QString &note)
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;

    m_personalRating = rating;
    m_personalNote = note;
    MangaDatabase::instance()->saveMangaNote(m_provider, m_mangaId, note, rating);
    emit personalNoteChanged();
}

void MangaDetailController::saveOverride(const QString &title, const QString &coverUrl, const QString &synopsis)
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;

    MangaOverrideEntity ovr;
    ovr.provider = m_provider;
    ovr.mangaId = m_mangaId;
    ovr.customTitle = title;
    ovr.customCoverUrl = coverUrl;
    ovr.customDescription = synopsis;
    MangaDatabase::instance()->saveMangaOverride(ovr);

    m_hasOverride = true;
    emit overrideChanged();

    if (!title.isEmpty()) m_detail[QStringLiteral("title")] = title;
    if (!coverUrl.isEmpty()) m_detail[QStringLiteral("coverUrl")] = coverUrl;
    if (!synopsis.isEmpty()) m_detail[QStringLiteral("description")] = synopsis;
    emit detailChanged();
}

void MangaDetailController::resetOverride()
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;

    MangaDatabase::instance()->deleteMangaOverride(m_provider, m_mangaId);
    m_hasOverride = false;
    emit overrideChanged();
    reload();
}

void MangaDetailController::togglePinned()
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;

    m_pinned = !m_pinned;
    MangaDatabase::instance()->setMangaPinned(m_provider, m_mangaId, m_pinned);
    emit pinnedChanged();
}

void MangaDetailController::toggleMuted()
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return;

    m_muted = !m_muted;
    MangaDatabase::instance()->setMangaMuted(m_provider, m_mangaId, m_muted);
    emit mutedChanged();
}

bool MangaDetailController::isChapterBookmarked(const QString &chapterId) const
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty() || chapterId.isEmpty())
        return false;
    return MangaDatabase::instance()->isChapterBookmarked(m_provider, m_mangaId, chapterId);
}

void MangaDetailController::toggleChapterBookmark(const QString &chapterId)
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty() || chapterId.isEmpty())
        return;

    const bool current = isChapterBookmarked(chapterId);
    MangaDatabase::instance()->setChapterBookmarked(m_provider, m_mangaId, chapterId, !current);

    for (int i = 0; i < m_chapters.size(); ++i) {
        QVariantMap ch = m_chapters[i].toMap();
        if (ch.value(QStringLiteral("id")).toString() == chapterId) {
            ch[QStringLiteral("bookmarked")] = !current;
            m_chapters[i] = ch;
            break;
        }
    }
    emit chaptersChanged();
}

QVariantList MangaDetailController::getComments(const QString &chapterId) const
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty())
        return {};

    const auto list = MangaDatabase::instance()->getComments(m_provider, m_mangaId, chapterId);
    QVariantList out;
    out.reserve(list.size());
    for (const auto &c : list) {
        QVariantMap m;
        m[QStringLiteral("id")] = c.id;
        m[QStringLiteral("provider")] = c.provider;
        m[QStringLiteral("mangaId")] = c.mangaId;
        m[QStringLiteral("chapterId")] = c.chapterId;
        m[QStringLiteral("mangaTitle")] = c.mangaTitle;
        m[QStringLiteral("userId")] = c.userId;
        m[QStringLiteral("username")] = c.username;
        m[QStringLiteral("avatarUrl")] = c.avatarUrl;
        m[QStringLiteral("parentId")] = c.parentId;
        m[QStringLiteral("text")] = c.text;
        m[QStringLiteral("likesCount")] = c.likesCount;
        m[QStringLiteral("likedByMe")] = c.likedByMe;
        m[QStringLiteral("createdAt")] = c.createdAt;
        out.append(m);
    }
    return out;
}

bool MangaDetailController::addComment(const QString &text, const QString &chapterId, const QString &parentId)
{
    if (m_provider.isEmpty() || m_mangaId.isEmpty() || text.trimmed().isEmpty())
        return false;

    CommentEntity c;
    c.id = QStringLiteral("c_%1").arg(QDateTime::currentMSecsSinceEpoch());
    c.provider = m_provider;
    c.mangaId = m_mangaId;
    c.chapterId = chapterId;
    c.mangaTitle = m_detail.value(QStringLiteral("title")).toString();
    c.userId = QStringLiteral("local_user");
    c.username = QStringLiteral("You");
    c.parentId = parentId;
    c.text = text.trimmed();
    c.likesCount = 0;
    c.likedByMe = false;
    c.createdAt = QDateTime::currentMSecsSinceEpoch();

    const bool ok = MangaDatabase::instance()->saveComment(c);
    if (ok) {
        emit commentsChanged();
    }
    return ok;
}

bool MangaDetailController::toggleCommentLike(const QString &commentId)
{
    const bool ok = MangaDatabase::instance()->toggleCommentLike(commentId);
    if (ok) {
        emit commentsChanged();
    }
    return ok;
}

bool MangaDetailController::deleteComment(const QString &commentId)
{
    const bool ok = MangaDatabase::instance()->deleteComment(commentId);
    if (ok) {
        emit commentsChanged();
    }
    return ok;
}

bool MangaDetailController::isNovel() const
{
    const auto *ext = ExtensionManager::instance().findById(m_provider);
    return (ext && ext->type == ExtensionType::Novel) ||
           m_detail.value(QStringLiteral("type")).toString() == QLatin1String("novel");
}
