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

public:
    explicit MangaDetailController(QObject *parent = nullptr);
    ~MangaDetailController() override = default;

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

    Q_INVOKABLE void load(const QString &provider, const QString &mangaId);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void toggleLibrary();
    Q_INVOKABLE void addToLibrary();
    Q_INVOKABLE void removeFromLibrary();
    Q_INVOKABLE void markChapterRead(const QString &chapterId, bool read);
    Q_INVOKABLE QStringList getCategories() const;
    Q_INVOKABLE void setCategories(const QStringList &categoryIds);

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
};
