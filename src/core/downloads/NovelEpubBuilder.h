#pragma once

#include <QString>

namespace mangadl {

class NovelEpubBuilder {
public:
    /// Builds a valid EPUB 3.0 archive at targetFilePath with the specified chapter content.
    /// The 'mimetype' entry is stored first and uncompressed (ZIP_CM_STORE) per EPUB standard.
    /// Atomically replaces targetFilePath upon completion.
    static bool buildEpub(
        const QString &targetFilePath,
        const QString &mangaTitle,
        const QString &chapterTitle,
        const QString &content,
        const QString &format = QStringLiteral("html"),
        QString *errorMessage = nullptr);

    /// Normalizes HTML content into clean paragraph strings.
    static QString htmlToPlainParagraphs(const QString &html);
};

} // namespace mangadl
