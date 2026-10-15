#include "ArchiveInspector.h"

#include <cmath>
#include <QRegularExpression>

namespace mangadl {

bool ArchiveInspector::isImageExtension(const QString &ext)
{
    QString clean = ext.toLower();
    if (!clean.startsWith(QLatin1Char('.')))
        clean.prepend(QLatin1Char('.'));

    return (clean == QStringLiteral(".jpg") || clean == QStringLiteral(".jpeg") ||
            clean == QStringLiteral(".png") || clean == QStringLiteral(".webp") ||
            clean == QStringLiteral(".gif") || clean == QStringLiteral(".bmp") ||
            clean == QStringLiteral(".avif"));
}

bool ArchiveInspector::isArchiveExtension(const QString &ext)
{
    QString clean = ext.toLower();
    if (!clean.startsWith(QLatin1Char('.')))
        clean.prepend(QLatin1Char('.'));

    return (clean == QStringLiteral(".cbz") || clean == QStringLiteral(".zip"));
}

bool ArchiveInspector::naturalCompare(const QString &a, const QString &b)
{
    int ai = 0, bi = 0;
    const int aLen = a.length(), bLen = b.length();
    while (ai < aLen && bi < bLen) {
        if (a[ai].isDigit() && b[bi].isDigit()) {
            int aStart = ai;
            while (ai < aLen && a[ai].isDigit()) ai++;
            int bStart = bi;
            while (bi < bLen && b[bi].isDigit()) bi++;

            QStringView aNum = QStringView(a).mid(aStart, ai - aStart);
            QStringView bNum = QStringView(b).mid(bStart, bi - bStart);

            int aNonZero = 0;
            while (aNonZero < aNum.length() && aNum[aNonZero] == QLatin1Char('0')) aNonZero++;
            int bNonZero = 0;
            while (bNonZero < bNum.length() && bNum[bNonZero] == QLatin1Char('0')) bNonZero++;

            QStringView aClean = aNum.mid(aNonZero);
            QStringView bClean = bNum.mid(bNonZero);

            if (aClean.length() != bClean.length())
                return aClean.length() < bClean.length();
            const int cmp = aClean.compare(bClean);
            if (cmp != 0)
                return cmp < 0;
        } else {
            const QChar ca = a[ai].toLower();
            const QChar cb = b[bi].toLower();
            if (ca != cb)
                return ca < cb;
            ai++;
            bi++;
        }
    }
    return (aLen - ai) < (bLen - bi);
}

ArchiveFilenameMeta ArchiveInspector::parseArchiveFilename(const QString &filename)
{
    ArchiveFilenameMeta meta;
    QString clean = filename;
    static const QRegularExpression extRegex(QStringLiteral(R"(\.(cbz|zip|epub|cbr)$)"), QRegularExpression::CaseInsensitiveOption);
    clean.remove(extRegex);

    // Strip [Group] and (v01) tags first so they don't confuse range/number detection
    static const QRegularExpression tagRegex(QStringLiteral(R"([\[\(][^\]\)]*[\]\)])"));
    QString withoutTags = clean;
    withoutTags.remove(tagRegex);
    if (withoutTags.trimmed().isEmpty()) {
        withoutTags = clean; // Avoid stripping if name was entirely in brackets
    }

    static const QRegularExpression rangeRegex(
        QStringLiteral(R"((?:ch(?:apter)?\.?[\s_-]*|c[\s_-]*)?(\d+)[\s_]*[-–to]+[\s_]*(?:ch(?:apter)?\.?[\s_-]*|c[\s_-]*)?(\d+))"),
        QRegularExpression::CaseInsensitiveOption);

    const auto rangeMatch = rangeRegex.match(withoutTags);
    if (rangeMatch.hasMatch()) {
        meta.isRange = true;
        meta.hasChapter = true;
        meta.rangeStart = rangeMatch.captured(1).toDouble();
        meta.rangeEnd = rangeMatch.captured(2).toDouble();
        if (meta.rangeStart > meta.rangeEnd)
            std::swap(meta.rangeStart, meta.rangeEnd);
    }

    static const QRegularExpression singleRegex(
        QStringLiteral(R"((?:ch(?:apter)?\.?[\s_-]*|c[\s_-]*|v(?:ol(?:ume)?)?\.?[\s_-]*)(\d+(?:\.\d+)?))"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression fallbackNumRegex(
        QStringLiteral(R"((?:^|[^\d])(\d+(?:\.\d+)?)(?:$|[^\d]))"),
        QRegularExpression::CaseInsensitiveOption);

    double singleCh = 0.0;
    bool hasSingle = false;
    auto singleMatch = singleRegex.match(withoutTags);
    auto fbMatch = fallbackNumRegex.match(withoutTags);

    if (singleMatch.hasMatch()) {
        singleCh = singleMatch.captured(1).toDouble();
        hasSingle = true;
    } else if (fbMatch.hasMatch()) {
        singleCh = fbMatch.captured(1).toDouble();
        hasSingle = true;
    }

    if (!meta.isRange && hasSingle) {
        meta.rangeStart = singleCh;
        meta.rangeEnd = singleCh;
        meta.hasChapter = true;
    }

    QString seriesTitle = withoutTags;
    if (rangeMatch.hasMatch() && rangeMatch.capturedStart() > 0) {
        seriesTitle = withoutTags.left(rangeMatch.capturedStart());
    } else if (singleMatch.hasMatch() && singleMatch.capturedStart() > 0) {
        seriesTitle = withoutTags.left(singleMatch.capturedStart());
    } else if (fbMatch.hasMatch() && fbMatch.capturedStart(1) > 0) {
        seriesTitle = withoutTags.left(fbMatch.capturedStart(1));
    }

    // Replace dashes, underscores, dots with spaces
    static const QRegularExpression separatorRegex(QStringLiteral(R"([-_.]+)"));
    seriesTitle.replace(separatorRegex, QStringLiteral(" "));
    seriesTitle = seriesTitle.trimmed();

    meta.seriesTitle = seriesTitle.isEmpty() ? clean : seriesTitle;
    return meta;
}

double ArchiveInspector::extractChapterNumber(const QString &name, double fallback)
{
    static const QRegularExpression numRegex(
        QStringLiteral(R"((?:ch(?:apter)?\.?[\s_-]*|c[\s_-]*)?(\d+(?:\.\d+)?))"),
        QRegularExpression::CaseInsensitiveOption);

    const auto match = numRegex.match(name);
    if (match.hasMatch()) {
        bool ok = false;
        const double val = match.captured(1).toDouble(&ok);
        if (ok)
            return val;
    }
    return fallback;
}

QString ArchiveInspector::cleanDirTitle(const QString &dirName, double chapterNumber)
{
    QString cleaned = dirName;
    static const QRegularExpression sep(QStringLiteral(R"([-_]+)"));
    cleaned = cleaned.replace(sep, QStringLiteral(" ")).trimmed();
    return cleaned.isEmpty() ? QStringLiteral("Chapter %1").arg(formatChapterNumber(chapterNumber)) : cleaned;
}

QString ArchiveInspector::formatChapterNumber(double n)
{
    if (std::floor(n) == n) {
        return QString::number(static_cast<qint64>(n));
    }
    return QString::number(n, 'f', 1);
}

} // namespace mangadl
