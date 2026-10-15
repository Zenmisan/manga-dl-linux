#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <atomic>
#include <memory>

class SearchController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool searching READ isSearching NOTIFY searchingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(int activeTab READ activeTab WRITE setActiveTab NOTIFY activeTabChanged)
    Q_PROPERTY(QString selectedStatus READ selectedStatus WRITE setSelectedStatus NOTIFY selectedStatusChanged)
    Q_PROPERTY(QString sortBy READ sortBy WRITE setSortBy NOTIFY sortByChanged)
    Q_PROPERTY(QVariantList groups READ groups NOTIFY groupsChanged)
    Q_PROPERTY(QVariantList availableSources READ availableSources NOTIFY availableSourcesChanged)
    Q_PROPERTY(QStringList selectedSources READ selectedSources WRITE setSelectedSources NOTIFY selectedSourcesChanged)

public:
    explicit SearchController(QObject *parent = nullptr);
    ~SearchController() override;

    QString query() const { return m_query; }
    void setQuery(const QString &q);

    bool isSearching() const { return m_searching; }
    QString statusText() const { return m_statusText; }

    int activeTab() const { return m_activeTab; }
    void setActiveTab(int tab);

    QString selectedStatus() const { return m_selectedStatus; }
    void setSelectedStatus(const QString &status);

    QString sortBy() const { return m_sortBy; }
    void setSortBy(const QString &sort);

    QVariantList groups() const { return m_groups; }
    QVariantList availableSources() const { return m_availableSources; }

    QStringList selectedSources() const { return m_selectedSources; }
    void setSelectedSources(const QStringList &sources);

    Q_INVOKABLE void search(const QString &queryText);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void resetFilters();
    Q_INVOKABLE void toggleSource(const QString &sourceId);
    Q_INVOKABLE void setSourceSelected(const QString &sourceId, bool selected);

signals:
    void queryChanged();
    void searchingChanged();
    void statusTextChanged();
    void activeTabChanged();
    void selectedStatusChanged();
    void sortByChanged();
    void groupsChanged();
    void availableSourcesChanged();
    void selectedSourcesChanged();

private:
    void populateAvailableSources();
    void applySortAndFilterToGroups();

    QString m_query;
    bool m_searching = false;
    QString m_statusText;
    int m_activeTab = 0; // 0: Manga, 1: Web Novels, 2: Readers
    QString m_selectedStatus = QStringLiteral("Any");
    QString m_sortBy = QStringLiteral("Relevance");
    QVariantList m_groups;
    QVariantList m_availableSources;
    QStringList m_selectedSources;

    uint64_t m_currentSearchId = 0;
    int m_totalSourcesCount = 0;
    int m_completedSourcesCount = 0;
    std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);
};
