#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace mangadl {

class MangaDatabase;

enum class LocalImportStatus {
    Done,
    Error,
    Unsupported
};

struct LocalImportResult {
    QString name;
    QString ext;
    LocalImportStatus status = LocalImportStatus::Done;
    QString message;

    QVariantMap toMap() const {
        return {
            {QStringLiteral("name"), name},
            {QStringLiteral("ext"), ext},
            {QStringLiteral("status"), status == LocalImportStatus::Done ? QStringLiteral("done")
                                      : status == LocalImportStatus::Error ? QStringLiteral("error")
                                      : QStringLiteral("unsupported")},
            {QStringLiteral("kind"), status == LocalImportStatus::Done ? QStringLiteral("done")
                                    : status == LocalImportStatus::Error ? QStringLiteral("error")
                                    : QStringLiteral("unsupported")},
            {QStringLiteral("message"), message},
            {QStringLiteral("progress"), status == LocalImportStatus::Done ? 1.0 : 0.0}
        };
    }
};

class LocalImportService : public QObject {
    Q_OBJECT

public:
    static const QString Provider;

    explicit LocalImportService(MangaDatabase *db = nullptr,
                                const QString &libraryRoot = QString(),
                                QObject *parent = nullptr);
    ~LocalImportService() override = default;

    static LocalImportService &instance();

    QString libraryRoot() const;
    void setLibraryRoot(const QString &root);

    /// Imports a standalone archive (.cbz, .zip, .epub).
    QList<LocalImportResult> importStandaloneFile(const QString &sourcePath,
                                                 const QString &targetMangaId = QString(),
                                                 bool readComicInfo = true);

    /// Imports a directory containing archives or loose images.
    QList<LocalImportResult> importFolder(const QString &folderPath,
                                         const QString &targetMangaId = QString(),
                                         bool readComicInfo = true);

    /// High-level entry point for QML: imports a list of file or folder URLs/paths.
    /// 'mode' can be "new_series" or "existing_series" (with targetMangaId).
    Q_INVOKABLE QVariantList importPaths(const QStringList &paths,
                                        const QString &targetMangaId = QString(),
                                        bool readComicInfo = true);

signals:
    void importStarted(const QString &name);
    void importFinished(const QString &name, bool success, const QString &message);

private:
    QList<LocalImportResult> importArchiveFile(const QString &sourcePath,
                                              const QString &targetMangaId,
                                              bool readComicInfo,
                                              const QString &seriesTitleOverride = QString());
    LocalImportResult importEpub(const QString &sourcePath,
                                const QString &targetMangaId);
    LocalImportResult importImagesAsChapter(const QStringList &imageFiles,
                                           const QString &seriesTitle,
                                           const QString &chapterLabel,
                                           double chapterNumber,
                                           const QString &displayName,
                                           const QString &targetMangaId);

    MangaDatabase *m_db = nullptr;
    QString m_libraryRoot;
};

} // namespace mangadl
