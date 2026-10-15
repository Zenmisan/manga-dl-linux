#include "LocalImportService.h"

#include "ArchiveInspector.h"
#include "EpubParser.h"
#include "../downloads/CbzBuilder.h"
#include "../db/MangaDatabase.h"
#include "../settings/AppSettings.h"
#include "../util/AppPaths.h"
#include "../util/FileNaming.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <algorithm>
#include <zip.h>

namespace mangadl {

const QString LocalImportService::Provider = QStringLiteral("local");

LocalImportService::LocalImportService(MangaDatabase *db, const QString &libraryRoot, QObject *parent)
    : QObject(parent)
    , m_db(db ? db : MangaDatabase::instance())
    , m_libraryRoot(libraryRoot)
{
    if (m_libraryRoot.isEmpty()) {
        const QString settingPath = AppSettings::instance()->downloadPath();
        m_libraryRoot = settingPath.isEmpty() ? AppPaths::defaultDownloadDir() : settingPath;
    }
}

LocalImportService &LocalImportService::instance()
{
    static LocalImportService s_instance;
    return s_instance;
}

QString LocalImportService::libraryRoot() const
{
    return m_libraryRoot;
}

void LocalImportService::setLibraryRoot(const QString &root)
{
    m_libraryRoot = root;
}

static QString cleanPath(const QString &raw)
{
    if (raw.startsWith(QStringLiteral("file://"))) {
        return QUrl(raw).toLocalFile();
    }
    return raw;
}

QVariantList LocalImportService::importPaths(const QStringList &paths, const QString &targetMangaId, bool readComicInfo)
{
    QVariantList results;
    for (const auto &rawPath : paths) {
        const QString localPath = cleanPath(rawPath);
        if (localPath.isEmpty())
            continue;

        const QFileInfo fi(localPath);
        if (!fi.exists()) {
            LocalImportResult res{fi.fileName(), fi.suffix().toUpper(), LocalImportStatus::Error, QStringLiteral("File not found")};
            results.append(res.toMap());
            continue;
        }

        if (fi.isDir()) {
            const auto dirRes = importFolder(localPath, targetMangaId, readComicInfo);
            for (const auto &r : dirRes) {
                results.append(r.toMap());
            }
        } else {
            const auto fileRes = importStandaloneFile(localPath, targetMangaId, readComicInfo);
            for (const auto &r : fileRes) {
                results.append(r.toMap());
            }
        }
    }
    return results;
}

QList<LocalImportResult> LocalImportService::importStandaloneFile(const QString &sourcePath,
                                                               const QString &targetMangaId,
                                                               bool readComicInfo)
{
    const QFileInfo fi(sourcePath);
    const QString ext = fi.suffix().toLower();
    const QString name = fi.fileName();

    if (ext == QStringLiteral("epub")) {
        return {importEpub(sourcePath, targetMangaId)};
    }

    if (ext != QStringLiteral("cbz") && ext != QStringLiteral("zip")) {
        return {LocalImportResult{name, ext.toUpper(), LocalImportStatus::Unsupported, QStringLiteral("Unsupported format")}};
    }

    return importArchiveFile(sourcePath, targetMangaId, readComicInfo);
}

QList<LocalImportResult> LocalImportService::importFolder(const QString &folderPath,
                                                         const QString &targetMangaId,
                                                         bool readComicInfo)
{
    QList<LocalImportResult> results;
    const QDir dir(folderPath);
    const QString folderName = dir.dirName();

    QString seriesTitleOverride;
    if (targetMangaId.isEmpty()) {
        seriesTitleOverride = folderName;
    }

    // 1. Check for child CBZ/ZIP archives
    QStringList archiveFiles = dir.entryList({QStringLiteral("*.cbz"), QStringLiteral("*.zip")}, QDir::Files);
    std::sort(archiveFiles.begin(), archiveFiles.end(), ArchiveInspector::naturalCompare);

    if (!archiveFiles.isEmpty()) {
        for (const auto &af : archiveFiles) {
            const QString fullPath = dir.filePath(af);
            results.append(importArchiveFile(fullPath, targetMangaId, readComicInfo, seriesTitleOverride));
        }
        return results;
    }

    // 2. Check for child EPUB files
    QStringList epubFiles = dir.entryList({QStringLiteral("*.epub")}, QDir::Files);
    std::sort(epubFiles.begin(), epubFiles.end(), ArchiveInspector::naturalCompare);
    if (!epubFiles.isEmpty()) {
        for (const auto &ef : epubFiles) {
            results.append(importEpub(dir.filePath(ef), targetMangaId));
        }
        return results;
    }

    // 3. Check for loose images in this folder
    QStringList imageFiles = dir.entryList(
        {QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
         QStringLiteral("*.webp"), QStringLiteral("*.gif"), QStringLiteral("*.bmp"),
         QStringLiteral("*.avif")}, QDir::Files);
    std::sort(imageFiles.begin(), imageFiles.end(), ArchiveInspector::naturalCompare);

    if (!imageFiles.isEmpty()) {
        QStringList fullImagePaths;
        fullImagePaths.reserve(imageFiles.size());
        for (const auto &img : imageFiles) {
            fullImagePaths.append(dir.filePath(img));
        }

        const double chNum = ArchiveInspector::extractChapterNumber(folderName, 1.0);
        const QString seriesTitle = targetMangaId.isEmpty() ? folderName : targetMangaId;
        results.append(importImagesAsChapter(fullImagePaths, seriesTitle, folderName, chNum, folderName + QStringLiteral("/"), targetMangaId));
        return results;
    }

    results.append(LocalImportResult{folderName + QStringLiteral("/"), QStringLiteral("DIR"), LocalImportStatus::Unsupported, QStringLiteral("No CBZ, ZIP, EPUB or images found")});
    return results;
}

QList<LocalImportResult> LocalImportService::importArchiveFile(const QString &sourcePath,
                                                             const QString &targetMangaId,
                                                             bool readComicInfo,
                                                             const QString &seriesTitleOverride)
{
    const QFileInfo fi(sourcePath);
    const QString displayName = fi.fileName();

    int err = 0;
    zip_t *za = zip_open(sourcePath.toUtf8().constData(), ZIP_RDONLY, &err);
    if (!za) {
        return {LocalImportResult{displayName, QStringLiteral("CBZ"), LocalImportStatus::Error, QStringLiteral("Not a valid ZIP/CBZ archive")}};
    }

    // Check ComicInfo.xml if requested
    ComicInfoMetadata comicInfo;
    bool hasComicInfo = false;
    if (readComicInfo) {
        hasComicInfo = CbzBuilder::readComicInfo(sourcePath, comicInfo);
    }

    // Group images by folder
    struct ChapterGroup {
        double number = 1.0;
        QString title;
        QStringList entryPaths;
    };
    QMap<QString, ChapterGroup> groups; // dirPrefix -> group

    const zip_int64_t numEntries = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < numEntries; ++i) {
        const char *name = zip_get_name(za, i, 0);
        if (!name) continue;
        const QString qName = QString::fromUtf8(name);
        if (qName.contains(QLatin1String("..")) || qName.startsWith(QLatin1Char('/')))
            continue;
        const QFileInfo entryFi(qName);
        if (!ArchiveInspector::isImageExtension(entryFi.suffix()))
            continue;

        QString dirPrefix;
        const int slashIdx = qName.lastIndexOf(QLatin1Char('/'));
        if (slashIdx >= 0) {
            dirPrefix = qName.left(slashIdx);
        }

        groups[dirPrefix].entryPaths.append(qName);
    }

    if (groups.isEmpty()) {
        zip_close(za);
        return {LocalImportResult{displayName, QStringLiteral("CBZ"), LocalImportStatus::Error, QStringLiteral("No image files found inside archive")}};
    }

    // Determine series title and chapters
    const auto meta = ArchiveInspector::parseArchiveFilename(displayName);
    QString seriesTitle = seriesTitleOverride;
    if (seriesTitle.isEmpty()) {
        if (hasComicInfo && !comicInfo.series.isEmpty()) {
            seriesTitle = comicInfo.series;
        } else {
            seriesTitle = meta.seriesTitle;
        }
    }

    // Determine target manga id
    QString effectiveMangaId = targetMangaId;
    QString effectiveProvider = Provider;

    if (effectiveMangaId.isEmpty()) {
        effectiveMangaId = FileNaming::slugify(seriesTitle);
        // Add series to library if not exists
        if (m_db) {
            LibraryEntity lib;
            lib.provider = effectiveProvider;
            lib.mangaId = effectiveMangaId;
            lib.title = seriesTitle;
            lib.type = QStringLiteral("manga");
            lib.totalChapters = 0;
            m_db->addToLibrary(lib);
        }
    } else if (m_db) {
        auto existing = m_db->getLibraryItem(effectiveProvider, effectiveMangaId);
        if (!existing) {
            existing = m_db->getLibraryItem(QString(), effectiveMangaId);
        }
        if (existing) {
            effectiveProvider = existing->provider;
            seriesTitle = existing->title;
        }
    }

    const QString destDir = QDir(m_libraryRoot).filePath(FileNaming::sanitizeFileName(seriesTitle));
    QDir().mkpath(destDir);

    QList<LocalImportResult> results;
    int groupIdx = 0;

    for (auto it = groups.begin(); it != groups.end(); ++it, ++groupIdx) {
        auto &group = it.value();
        std::sort(group.entryPaths.begin(), group.entryPaths.end(), ArchiveInspector::naturalCompare);

        double chNum = meta.rangeStart;
        if (groups.size() > 1) {
            chNum = ArchiveInspector::extractChapterNumber(it.key(), groupIdx + 1);
            group.title = ArchiveInspector::cleanDirTitle(it.key(), chNum);
        } else {
            if (hasComicInfo && !comicInfo.title.isEmpty()) {
                group.title = comicInfo.title;
                if (!comicInfo.number.isEmpty()) {
                    chNum = comicInfo.number.toDouble();
                }
            } else if (meta.isRange) {
                group.title = QStringLiteral("%1 (%2-%3)")
                                  .arg(seriesTitle,
                                       ArchiveInspector::formatChapterNumber(meta.rangeStart),
                                       ArchiveInspector::formatChapterNumber(meta.rangeEnd));
            } else if (meta.hasChapter) {
                group.title = QStringLiteral("Chapter %1").arg(ArchiveInspector::formatChapterNumber(chNum));
            } else {
                group.title = QStringLiteral("Chapter 1");
                chNum = 1.0;
            }
        }
        group.number = chNum;

        // Extract image bytes in memory
        QList<QByteArray> pageImages;
        QStringList extensions;
        pageImages.reserve(group.entryPaths.size());
        extensions.reserve(group.entryPaths.size());

        for (const auto &entryPath : group.entryPaths) {
            zip_file_t *zf = zip_fopen(za, entryPath.toUtf8().constData(), 0);
            if (!zf) continue;

            QByteArray buf;
            char chunk[8192];
            zip_int64_t n = 0;
            while ((n = zip_fread(zf, chunk, sizeof(chunk))) > 0) {
                buf.append(chunk, static_cast<int>(n));
            }
            zip_fclose(zf);

            if (!buf.isEmpty()) {
                pageImages.append(buf);
                const QFileInfo eFi(entryPath);
                extensions.append(eFi.suffix());
            }
        }

        const QString destFileName = FileNaming::sanitizeFileName(group.title) + QStringLiteral(".cbz");
        const QString destPath = QDir(destDir).filePath(destFileName);

        QString errMsg;
        const bool built = CbzBuilder::buildCbz(
            destPath,
            seriesTitle,
            group.title,
            ArchiveInspector::formatChapterNumber(group.number),
            pageImages,
            extensions,
            &errMsg);

        if (!built) {
            results.append(LocalImportResult{displayName, QStringLiteral("CBZ"), LocalImportStatus::Error, errMsg});
            continue;
        }

        // Add download entity
        if (m_db) {
            DownloadEntity d;
            d.provider = effectiveProvider;
            d.mangaId = effectiveMangaId;
            d.mangaTitle = seriesTitle;
            d.chapterId = FileNaming::slugify(group.title);
            d.chapterTitle = group.title;
            d.chapterNumber = group.number;
            d.cbzPath = destPath;
            d.status = QStringLiteral("completed");
            d.progress = 100;
            d.totalPages = static_cast<int>(pageImages.size());
            d.completedAt = QDateTime::currentMSecsSinceEpoch();
            d.id = QStringLiteral("%1/%2/%3").arg(d.provider, d.mangaId, d.chapterId);
            m_db->enqueueDownload(d);
            m_db->updateDownloadProgress(d.id, 100, d.totalPages, QStringLiteral("completed"));
        }

        const QString resultLabel = groups.size() > 1
                                        ? QStringLiteral("%1 — %2").arg(displayName, group.title)
                                        : displayName;
        results.append(LocalImportResult{resultLabel, QStringLiteral("CBZ"), LocalImportStatus::Done, QStringLiteral("Imported")});
    }

    zip_close(za);
    return results;
}

LocalImportResult LocalImportService::importImagesAsChapter(const QStringList &imageFiles,
                                                           const QString &seriesTitle,
                                                           const QString &chapterLabel,
                                                           double chapterNumber,
                                                           const QString &displayName,
                                                           const QString &targetMangaId)
{
    const QString destDir = QDir(m_libraryRoot).filePath(FileNaming::sanitizeFileName(seriesTitle));
    QDir().mkpath(destDir);

    const QString destFileName = FileNaming::sanitizeFileName(chapterLabel) + QStringLiteral(".cbz");
    const QString destPath = QDir(destDir).filePath(destFileName);

    QList<QByteArray> pageImages;
    QStringList extensions;
    pageImages.reserve(imageFiles.size());
    extensions.reserve(imageFiles.size());

    for (const auto &imgPath : imageFiles) {
        QFile file(imgPath);
        if (file.open(QIODevice::ReadOnly)) {
            pageImages.append(file.readAll());
            extensions.append(QFileInfo(imgPath).suffix());
        }
    }

    QString errMsg;
    const bool built = CbzBuilder::buildCbz(
        destPath,
        seriesTitle,
        chapterLabel,
        ArchiveInspector::formatChapterNumber(chapterNumber),
        pageImages,
        extensions,
        &errMsg);

    if (!built) {
        return LocalImportResult{displayName, QStringLiteral("DIR"), LocalImportStatus::Error, errMsg};
    }

    QString effectiveSeriesTitle = seriesTitle;
    QString effectiveMangaId = targetMangaId;
    QString effectiveProvider = Provider;

    if (effectiveMangaId.isEmpty()) {
        effectiveMangaId = FileNaming::slugify(effectiveSeriesTitle);
        if (m_db) {
            LibraryEntity lib;
            lib.provider = effectiveProvider;
            lib.mangaId = effectiveMangaId;
            lib.title = effectiveSeriesTitle;
            lib.type = QStringLiteral("manga");
            lib.totalChapters = 0;
            m_db->addToLibrary(lib);
        }
    } else if (m_db) {
        auto existing = m_db->getLibraryItem(effectiveProvider, effectiveMangaId);
        if (existing) {
            effectiveProvider = existing->provider;
            effectiveSeriesTitle = existing->title;
        }
    }

    if (m_db) {
        DownloadEntity d;
        d.provider = effectiveProvider;
        d.mangaId = effectiveMangaId;
        d.mangaTitle = effectiveSeriesTitle;
        d.chapterId = FileNaming::slugify(chapterLabel);
        d.chapterTitle = chapterLabel;
        d.chapterNumber = chapterNumber;
        d.cbzPath = destPath;
        d.status = QStringLiteral("completed");
        d.progress = 100;
        d.totalPages = static_cast<int>(pageImages.size());
        d.completedAt = QDateTime::currentMSecsSinceEpoch();
        d.id = QStringLiteral("%1/%2/%3").arg(d.provider, d.mangaId, d.chapterId);
        m_db->enqueueDownload(d);
        m_db->updateDownloadProgress(d.id, 100, d.totalPages, QStringLiteral("completed"));
    }

    return LocalImportResult{displayName, QStringLiteral("DIR"), LocalImportStatus::Done, QStringLiteral("Imported")};
}

LocalImportResult LocalImportService::importEpub(const QString &sourcePath, const QString &targetMangaId)
{
    const QFileInfo fi(sourcePath);
    const QString displayName = fi.fileName();

    const EpubBook book = EpubParser::parse(sourcePath);
    if (book.chapters.isEmpty()) {
        return LocalImportResult{displayName, QStringLiteral("EPUB"), LocalImportStatus::Error, QStringLiteral("No chapters found inside EPUB")};
    }

    QString seriesTitle = book.title;
    QString effectiveMangaId = targetMangaId;
    QString effectiveProvider = Provider;

    if (effectiveMangaId.isEmpty()) {
        effectiveMangaId = FileNaming::slugify(seriesTitle);
        if (m_db) {
            LibraryEntity lib;
            lib.provider = effectiveProvider;
            lib.mangaId = effectiveMangaId;
            lib.title = seriesTitle;
            lib.type = QStringLiteral("novel");
            lib.totalChapters = 0;
            m_db->addToLibrary(lib);
        }
    } else if (m_db) {
        auto existing = m_db->getLibraryItem(effectiveProvider, effectiveMangaId);
        if (existing) {
            effectiveProvider = existing->provider;
            seriesTitle = existing->title;
        }
    }

    const QString destDir = QDir(m_libraryRoot).filePath(FileNaming::sanitizeFileName(seriesTitle));
    const QString chaptersDir = QDir(destDir).filePath(QStringLiteral("chapters"));
    QDir().mkpath(chaptersDir);

    for (int i = 0; i < book.chapters.size(); ++i) {
        const auto &chapter = book.chapters[i];
        const QString chapterId = QStringLiteral("%1").arg(i + 1, 4, 10, QLatin1Char('0'));
        const QString chapterPath = QDir(chaptersDir).filePath(QStringLiteral("%1.html").arg(chapterId));

        QFile outFile(chapterPath);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            outFile.write(chapter.html.toUtf8());
            outFile.close();
        }

        if (m_db) {
            DownloadEntity d;
            d.provider = effectiveProvider;
            d.mangaId = effectiveMangaId;
            d.mangaTitle = seriesTitle;
            d.chapterId = chapterId;
            d.chapterTitle = chapter.title;
            d.chapterNumber = i + 1;
            d.cbzPath = chapterPath;
            d.status = QStringLiteral("completed");
            d.progress = 100;
            d.totalPages = 1;
            d.completedAt = QDateTime::currentMSecsSinceEpoch();
            d.id = QStringLiteral("%1/%2/%3").arg(d.provider, d.mangaId, d.chapterId);
            m_db->enqueueDownload(d);
            m_db->updateDownloadProgress(d.id, 100, 1, QStringLiteral("completed"));
        }
    }

    return LocalImportResult{displayName, QStringLiteral("EPUB"), LocalImportStatus::Done, QStringLiteral("Imported %1 chapters").arg(book.chapters.size())};
}

} // namespace mangadl
