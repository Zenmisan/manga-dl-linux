#pragma once

#include <QString>
#include <QStringList>

namespace mangadl {

struct ArchiveFilenameMeta {
    QString seriesTitle;
    double rangeStart = 0.0;
    double rangeEnd = 0.0;
    bool isRange = false;
    bool hasChapter = false;
};

class ArchiveInspector {
public:
    static bool isImageExtension(const QString &ext);
    static bool isArchiveExtension(const QString &ext);

    /// Natural sort comparison: "Chapter 2" < "Chapter 10".
    static bool naturalCompare(const QString &a, const QString &b);

    /// Parses filename into series title and chapter/range:
    /// "Solo_Leveling_01-10.zip" -> ("Solo Leveling", 1, 10, true)
    /// "One_Piece_Chapter_1080.cbz" -> ("One Piece", 1080, 1080, false)
    static ArchiveFilenameMeta parseArchiveFilename(const QString &filename);

    /// Extracts chapter number from a folder or file name: e.g. "Chapter 1" -> 1.0, "c05.5" -> 5.5.
    static double extractChapterNumber(const QString &name, double fallback = 1.0);

    /// Cleans a directory name into a chapter display title: e.g. "Chapter_05" -> "Chapter 05".
    static QString cleanDirTitle(const QString &dirName, double chapterNumber);

    /// Formats a chapter number into clean string (e.g. 1 -> "1", 1.5 -> "1.5").
    static QString formatChapterNumber(double n);
};

} // namespace mangadl
