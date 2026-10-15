#pragma once

#include <QString>

namespace mangadl {

class FileNaming {
public:
    /// Replaces invalid filesystem characters (/ \ : * ? " < > | etc.) with '_', trims, falls back to "untitled".
    static QString sanitizeFileName(const QString &name);

    /// Lowercase, alnum-and-dash slug for use as a manga_id or chapter_id.
    static QString slugify(const QString &name);
};

} // namespace mangadl
