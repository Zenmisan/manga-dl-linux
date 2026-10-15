#include "FileNaming.h"

#include <QRegularExpression>

namespace mangadl {

QString FileNaming::sanitizeFileName(const QString &name)
{
    // Invalid chars on Linux and cross-platform: / \ : * ? " < > | NUL
    static const QRegularExpression invalidChars(QStringLiteral(R"([/\\:*?"<>|\x00-\x1f])"));
    QString clean = name;
    clean = clean.replace(invalidChars, QStringLiteral("_")).trimmed();

    // Remove leading/trailing dots or spaces which cause issues
    while (clean.startsWith(QLatin1Char('.')))
        clean.remove(0, 1);
    while (clean.endsWith(QLatin1Char('.')))
        clean.chop(1);

    clean = clean.trimmed();
    return clean.isEmpty() ? QStringLiteral("untitled") : clean;
}

QString FileNaming::slugify(const QString &name)
{
    QString lowered = name.toLower().trimmed();
    static const QRegularExpression nonAlnum(QStringLiteral("[^a-z0-9]+"));
    QString slug = lowered.replace(nonAlnum, QStringLiteral("-"));

    while (slug.startsWith(QLatin1Char('-')))
        slug.remove(0, 1);
    while (slug.endsWith(QLatin1Char('-')))
        slug.chop(1);

    return slug.isEmpty() ? QStringLiteral("untitled") : slug;
}

} // namespace mangadl
