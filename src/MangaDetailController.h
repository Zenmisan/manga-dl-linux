#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class MangaDetailController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString provider READ provider NOTIFY providerChanged)
    Q_PROPERTY(QString mangaId READ mangaId NOTIFY mangaIdChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QVariantMap detail READ detail NOTIFY detailChanged)
    Q_PROPERTY(QVariantList chapters READ chapters NOTIFY chaptersChanged)
    Q_PROPERTY(bool inLibrary READ inLibrary NOTIFY inLibraryChanged)
    Q_PROPERTY(QVariantMap resumeChapter READ resumeChapter NOTIFY resumeChapterChanged)
    Q_PROPERTY(int totalChapters READ totalChapters NOTIFY totalChaptersChanged)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(int personalRating READ personalRating NOTIFY personalNoteChanged)
    Q_PROPERTY(QString personalNote READ personalNote NOTIFY personalNoteChanged)
    Q_PROPERTY(bool pinned READ isPinned NOTIFY pinnedChanged)
    Q_PROPERTY(bool muted READ isMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool hasOverride READ hasOverride NOTIFY overrideChanged)
    Q_PROPERTY(bool isNovel READ isNovel NOTIFY detailChanged)

public:
    explicit MangaDetailController(QObject *parent = nullptr);
    ~MangaDetailController() override = default;

    bool isNovel() const;

    QString provider() const { return m_provider; }
    QString mangaId() const { return m_mangaId; }
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    QVariantMap detail() const { return m_detail; }
    QVariantList chapters() const { return m_chapters; }
    bool inLibrary() const { return m_inLibrary; }
    QVariantMap resumeChapter() const { return m_resumeChapter; }
    int totalChapters() const { return static_cast<int>(m_chapters.size()); }
    int unreadCount() const { return m_unreadCount; }
    int personalRating() const { return m_personalRating; }
    QString personalNote() const { return m_personalNote; }
    bool isPinned() const { return m_pinned; }
    bool isMuted() const { return m_muted; }
    bool hasOverride() const { return m_hasOverride; }

    Q_INVOKABLE void load(const QString &provider, const QString &mangaId);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void toggleLibrary();
    Q_INVOKABLE void addToLibrary();
    Q_INVOKABLE void removeFromLibrary();
    Q_INVOKABLE void markChapterRead(const QString &chapterId, bool read);
    Q_INVOKABLE void markPreviousChaptersRead(const QString &chapterId);
    Q_INVOKABLE QStringList getCategories() const;
    Q_INVOKABLE void setCategories(const QStringList &categoryIds);

    // Notes & Overrides
    Q_INVOKABLE void savePersonalNote(int rating, const QString &note);
    Q_INVOKABLE void saveOverride(const QString &title, const QString &coverUrl, const QString &synopsis);
    Q_INVOKABLE void resetOverride();
    Q_INVOKABLE void togglePinned();
    Q_INVOKABLE void toggleMuted();

    // Chapter Bookmarks
    Q_INVOKABLE bool isChapterBookmarked(const QString &chapterId) const;
    Q_INVOKABLE void toggleChapterBookmark(const QString &chapterId);

    // Comments
    Q_INVOKABLE QVariantList getComments(const QString &chapterId = QString()) const;
    Q_INVOKABLE bool addComment(const QString &text, const QString &chapterId = QString(), const QString &parentId = QString());
    Q_INVOKABLE bool toggleCommentLike(const QString &commentId);
    Q_INVOKABLE bool deleteComment(const QString &commentId);

signals:
    void providerChanged();
    void mangaIdChanged();
    void loadingChanged();
    void errorChanged();
    void detailChanged();
    void chaptersChanged();
    void inLibraryChanged();
    void resumeChapterChanged();
    void totalChaptersChanged();
    void unreadCountChanged();
    void personalNoteChanged();
    void pinnedChanged();
    void mutedChanged();
    void overrideChanged();
    void commentsChanged();

private:
    void updateResumeChapterAndUnread();

    QString m_provider;
    QString m_mangaId;
    bool m_loading = false;
    QString m_error;
    QVariantMap m_detail;
    QVariantList m_chapters;
    bool m_inLibrary = false;
    QVariantMap m_resumeChapter;
    int m_unreadCount = 0;
    int m_personalRating = 0;
    QString m_personalNote;
    bool m_pinned = false;
    bool m_muted = false;
    bool m_hasOverride = false;
};
