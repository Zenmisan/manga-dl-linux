#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <atomic>
#include <memory>

class BrowseSourceController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY sourceInfoChanged)
    Q_PROPERTY(QString sourceId READ sourceId NOTIFY sourceInfoChanged)
    Q_PROPERTY(QString sourceUrl READ sourceUrl NOTIFY sourceInfoChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int page READ page NOTIFY pageChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)

public:
    explicit BrowseSourceController(QObject *parent = nullptr);
    ~BrowseSourceController() override;

    QString source() const { return m_source; }
    void setSource(const QString &s);

    QString sourceName() const { return m_sourceName; }
    QString sourceId() const { return m_sourceId; }
    QString sourceUrl() const { return m_sourceUrl; }

    QString mode() const { return m_mode; }
    void setMode(const QString &m);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &q);

    bool isLoading() const { return m_loading; }
    int page() const { return m_page; }
    bool hasMore() const { return m_hasMore; }
    QVariantList items() const { return m_items; }

    Q_INVOKABLE void load(const QString &sourceNameOrId);
    Q_INVOKABLE void setBrowseMode(const QString &newMode);
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void openInBrowser();

signals:
    void sourceChanged();
    void sourceInfoChanged();
    void modeChanged();
    void searchQueryChanged();
    void loadingChanged();
    void pageChanged();
    void hasMoreChanged();
    void itemsChanged();

private:
    void resolveSource(const QString &nameOrId);
    void fetchItems(bool append = false);

    QString m_source;
    QString m_sourceName = QStringLiteral("MangaDex");
    QString m_sourceId = QStringLiteral("mangadex");
    QString m_sourceUrl = QStringLiteral("https://mangadex.org");
    QString m_mode = QStringLiteral("popular"); // "popular" | "latest"
    QString m_searchQuery;
    bool m_loading = false;
    int m_page = 1;
    bool m_hasMore = true;
    QVariantList m_items;
    uint64_t m_currentFetchId = 0;
    std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);
};
