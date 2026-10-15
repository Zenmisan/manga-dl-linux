#pragma once

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QString>
#include <QStringList>

namespace mangadl {

struct ComicInfoMetadata {
    QString series;
    QString title;
    QString number;
    int pageCount = 0;
};

class CbzBuilder {
public:
    /// Builds a CBZ archive at targetFilePath containing page images and ComicInfo.xml.
    /// Pages are stored uncompressed (ZIP_CM_STORE) for maximum reader performance.
    /// Writes to a temporary file first and renames atomically upon completion.
    static bool buildCbz(
        const QString &targetFilePath,
        const QString &mangaTitle,
        const QString &chapterTitle,
        const QString &chapterNumber,
        const QList<QByteArray> &pageImages,
        const QStringList &fileExtensions = {},
        QString *errorMessage = nullptr);

    /// Generates ComicInfo.xml string.
    static QString generateComicInfoXml(
        const QString &series,
        const QString &title,
        const QString &number,
        int pageCount);

    /// Reads ComicInfo.xml metadata from a CBZ archive.
    static bool readComicInfo(const QString &cbzPath, ComicInfoMetadata &outMeta);

    /// Returns all image entry filenames inside the CBZ in natural sort order.
    static QStringList getImageEntries(const QString &cbzPath);

    /// Extracts entry bytes directly in memory from a CBZ archive.
    static QByteArray extractEntry(const QString &cbzPath, const QString &entryName);

    /// Extracts entry as QImage directly in memory from a CBZ archive.
    static QImage extractImage(const QString &cbzPath, const QString &entryName);
};

} // namespace mangadl
