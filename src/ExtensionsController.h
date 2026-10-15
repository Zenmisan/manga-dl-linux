#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <atomic>
#include <memory>

class ExtensionsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(QVariantMap lastUsedSource READ lastUsedSource NOTIFY lastUsedSourceChanged)
    Q_PROPERTY(QVariantList extensions READ extensions NOTIFY extensionsChanged)
    Q_PROPERTY(QVariantList libraryItems READ libraryItems NOTIFY libraryItemsChanged)
    Q_PROPERTY(QString languageFilter READ languageFilter WRITE setLanguageFilter NOTIFY languageFilterChanged)
    Q_PROPERTY(bool show18Plus READ show18Plus WRITE setShow18Plus NOTIFY show18PlusChanged)
    Q_PROPERTY(int installedCount READ installedCount NOTIFY countsChanged)
    Q_PROPERTY(int updateCount READ updateCount NOTIFY countsChanged)

    Q_PROPERTY(QVariantList migrationCandidates READ migrationCandidates NOTIFY migrationCandidatesChanged)
    Q_PROPERTY(bool searchingMigration READ isSearchingMigration NOTIFY searchingMigrationChanged)

public:
    explicit ExtensionsController(QObject *parent = nullptr);
    ~ExtensionsController() override;

    QVariantList sources() const { return m_sources; }
    QVariantMap lastUsedSource() const { return m_lastUsedSource; }
    QVariantList extensions() const { return m_extensions; }
    QVariantList libraryItems() const { return m_libraryItems; }

    QString languageFilter() const { return m_languageFilter; }
    void setLanguageFilter(const QString &lang);

    bool show18Plus() const { return m_show18Plus; }
    void setShow18Plus(bool show);

    int installedCount() const { return m_installedCount; }
    int updateCount() const { return m_updateCount; }

    QVariantList migrationCandidates() const { return m_migrationCandidates; }
    bool isSearchingMigration() const { return m_searchingMigration; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void togglePinSource(const QString &sourceId);
    Q_INVOKABLE bool isPinned(const QString &sourceId) const;

    Q_INVOKABLE void installExtension(const QString &extId);
    Q_INVOKABLE void uninstallExtension(const QString &extId);
    Q_INVOKABLE void updateExtension(const QString &extId);
    Q_INVOKABLE void updateAll();

    Q_INVOKABLE void searchMigrationCandidates(const QString &targetProvider, const QString &title);
    Q_INVOKABLE bool migrateManga(const QString &oldProvider, const QString &oldMangaId,
                                  const QString &newProvider, const QString &newMangaId,
                                  const QString &newTitle = QString(), const QString &newCoverUrl = QString());

signals:
    void sourcesChanged();
    void lastUsedSourceChanged();
    void extensionsChanged();
    void libraryItemsChanged();
    void languageFilterChanged();
    void show18PlusChanged();
    void countsChanged();
    void migrationCandidatesChanged();
    void searchingMigrationChanged();
    void migrationFinished(bool success, const QString &message);

private:
    void loadPinnedSources();
    void savePinnedSources();
    void rebuildSources();
    void rebuildExtensions();
    void rebuildLibraryItems();

    static QString colorForId(const QString &id);

    QVariantList m_sources;
    QVariantMap m_lastUsedSource;
    QVariantList m_extensions;
    QVariantList m_libraryItems;

    QString m_languageFilter = QStringLiteral("English");
    bool m_show18Plus = false;
    int m_installedCount = 0;
    int m_updateCount = 0;

    QStringList m_pinnedSources;

    QVariantList m_migrationCandidates;
    bool m_searchingMigration = false;
    uint64_t m_migrationSearchId = 0;
    std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);
};
