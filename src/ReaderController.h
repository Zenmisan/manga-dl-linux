#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class ReaderController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString provider READ provider NOTIFY chapterLoaded)
    Q_PROPERTY(QString mangaId READ mangaId NOTIFY chapterLoaded)
    Q_PROPERTY(QString mangaTitle READ mangaTitle NOTIFY chapterLoaded)
    Q_PROPERTY(QString coverUrl READ coverUrl NOTIFY chapterLoaded)
    Q_PROPERTY(QString chapterId READ chapterId NOTIFY chapterLoaded)
    Q_PROPERTY(QString chapterTitle READ chapterTitle NOTIFY chapterLoaded)
    Q_PROPERTY(double chapterNumber READ chapterNumber NOTIFY chapterLoaded)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QStringList pages READ pages NOTIFY pagesChanged)
    Q_PROPERTY(QString novelText READ novelText NOTIFY novelTextChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(int totalPages READ totalPages NOTIFY totalPagesChanged)
    Q_PROPERTY(bool isNovel READ isNovel NOTIFY isNovelChanged)
    Q_PROPERTY(QString prevChapterId READ prevChapterId NOTIFY navigationChanged)
    Q_PROPERTY(QString nextChapterId READ nextChapterId NOTIFY navigationChanged)

public:
    explicit ReaderController(QObject *parent = nullptr);
    ~ReaderController() override = default;

    QString provider() const { return m_provider; }
    QString mangaId() const { return m_mangaId; }
    QString mangaTitle() const { return m_mangaTitle; }
    QString coverUrl() const { return m_coverUrl; }
    QString chapterId() const { return m_chapterId; }
    QString chapterTitle() const { return m_chapterTitle; }
    double chapterNumber() const { return m_chapterNumber; }
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    QStringList pages() const { return m_pages; }
    QString novelText() const { return m_novelText; }
    int currentPage() const { return m_currentPage; }
    void setCurrentPage(int p);
    int totalPages() const { return m_totalPages; }
    bool isNovel() const { return m_isNovel; }
    QString prevChapterId() const { return m_prevChapterId; }
    QString nextChapterId() const { return m_nextChapterId; }

    Q_INVOKABLE void loadChapter(const QString &provider, const QString &mangaId, const QString &chapterId,
                                 const QString &mangaTitle = "", const QString &coverUrl = "",
                                 const QString &chapterTitle = "", double chapterNumber = 0.0);
    Q_INVOKABLE void saveProgress(int page, int totalPages, bool completed);
    Q_INVOKABLE void setNavigation(const QString &prevId, const QString &nextId);

signals:
    void chapterLoaded();
    void loadingChanged();
    void errorChanged();
    void pagesChanged();
    void novelTextChanged();
    void currentPageChanged();
    void totalPagesChanged();
    void isNovelChanged();
    void navigationChanged();

private:
    QString m_provider;
    QString m_mangaId;
    QString m_mangaTitle;
    QString m_coverUrl;
    QString m_chapterId;
    QString m_chapterTitle;
    double m_chapterNumber = 0.0;
    bool m_loading = false;
    QString m_error;
    QStringList m_pages;
    QString m_novelText;
    int m_currentPage = 1;
    int m_totalPages = 1;
    bool m_isNovel = false;
    QString m_prevChapterId;
    QString m_nextChapterId;
};
