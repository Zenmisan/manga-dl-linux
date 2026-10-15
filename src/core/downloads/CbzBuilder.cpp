#include "CbzBuilder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QRegularExpression>
#include <algorithm>
#include <zip.h>

namespace mangadl {

static QString escapeXml(const QString &str)
{
    QString escaped;
    escaped.reserve(str.size());
    for (const QChar c : str) {
        if (c == QLatin1Char('&')) escaped += QStringLiteral("&amp;");
        else if (c == QLatin1Char('<')) escaped += QStringLiteral("&lt;");
        else if (c == QLatin1Char('>')) escaped += QStringLiteral("&gt;");
        else if (c == QLatin1Char('"')) escaped += QStringLiteral("&quot;");
        else if (c == QLatin1Char('\'')) escaped += QStringLiteral("&apos;");
        else escaped += c;
    }
    return escaped;
}

static bool isImageFilename(const QString &name)
{
    static const QStringList validExts = {
        QStringLiteral(".jpg"), QStringLiteral(".jpeg"),
        QStringLiteral(".png"), QStringLiteral(".webp"),
        QStringLiteral(".gif"), QStringLiteral(".bmp"),
        QStringLiteral(".avif")
    };
    for (const auto &ext : validExts) {
        if (name.endsWith(ext, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

// Natural sort comparer for archive entry names (e.g. "0002.jpg" or "page10" after "page2")
static bool naturalSortLessThan(const QString &a, const QString &b)
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

            // Strip leading zeroes for comparison
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

QString CbzBuilder::generateComicInfoXml(
    const QString &series,
    const QString &title,
    const QString &number,
    int pageCount)
{
    QString xml;
    xml += QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    xml += QStringLiteral("<ComicInfo xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n");
    xml += QStringLiteral("  <Series>") + escapeXml(series) + QStringLiteral("</Series>\n");
    xml += QStringLiteral("  <Title>") + escapeXml(title) + QStringLiteral("</Title>\n");
    if (!number.isEmpty()) {
        xml += QStringLiteral("  <Number>") + escapeXml(number) + QStringLiteral("</Number>\n");
    }
    xml += QStringLiteral("  <PageCount>%1</PageCount>\n").arg(pageCount);
    xml += QStringLiteral("</ComicInfo>\n");
    return xml;
}

bool CbzBuilder::buildCbz(
    const QString &targetFilePath,
    const QString &mangaTitle,
    const QString &chapterTitle,
    const QString &chapterNumber,
    const QList<QByteArray> &pageImages,
    const QStringList &fileExtensions,
    QString *errorMessage)
{
    const QFileInfo targetInfo(targetFilePath);
    QDir().mkpath(targetInfo.dir().absolutePath());

    const QString tempFilePath = targetFilePath + QStringLiteral(".tmp");
    QFile::remove(tempFilePath);

    int err = 0;
    zip_t *za = zip_open(tempFilePath.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!za) {
        zip_error_t zerr;
        zip_error_init_with_code(&zerr, err);
        if (errorMessage)
            *errorMessage = QString::fromUtf8(zip_error_strerror(&zerr));
        zip_error_fini(&zerr);
        return false;
    }

    const int count = pageImages.size();
    for (int i = 0; i < count; ++i) {
        QString ext = (i < fileExtensions.size() && !fileExtensions[i].isEmpty())
                          ? fileExtensions[i]
                          : QStringLiteral(".jpg");
        if (!ext.startsWith(QLatin1Char('.')))
            ext.prepend(QLatin1Char('.'));

        const QString entryName = QStringLiteral("%1%2")
                                      .arg(i + 1, 4, 10, QLatin1Char('0'))
                                      .arg(ext);

        const QByteArray &data = pageImages[i];
        zip_source_t *s = zip_source_buffer(za, data.constData(), data.size(), 0);
        if (!s) {
            if (errorMessage)
                *errorMessage = QString::fromUtf8(zip_strerror(za));
            zip_discard(za);
            QFile::remove(tempFilePath);
            return false;
        }

        zip_int64_t idx = zip_file_add(za, entryName.toUtf8().constData(), s, ZIP_FL_OVERWRITE);
        if (idx < 0) {
            zip_source_free(s);
            if (errorMessage)
                *errorMessage = QString::fromUtf8(zip_strerror(za));
            zip_discard(za);
            QFile::remove(tempFilePath);
            return false;
        }

        // STORE mode (no compression) for fastest decompression in readers
        zip_set_file_compression(za, idx, ZIP_CM_STORE, 0);
    }

    // ComicInfo.xml metadata
    const QString comicInfo = generateComicInfoXml(mangaTitle, chapterTitle, chapterNumber, count);
    const QByteArray comicInfoBytes = comicInfo.toUtf8();
    zip_source_t *ciSource = zip_source_buffer(za, comicInfoBytes.constData(), comicInfoBytes.size(), 0);
    if (ciSource) {
        zip_int64_t ciIdx = zip_file_add(za, "ComicInfo.xml", ciSource, ZIP_FL_OVERWRITE);
        if (ciIdx >= 0) {
            zip_set_file_compression(za, ciIdx, ZIP_CM_DEFAULT, 0);
        } else {
            zip_source_free(ciSource);
        }
    }

    if (zip_close(za) < 0) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to finalize CBZ archive");
        QFile::remove(tempFilePath);
        return false;
    }

    // Atomic move to target
    QFile::remove(targetFilePath);
    if (!QFile::rename(tempFilePath, targetFilePath)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to move temporary CBZ to destination");
        QFile::remove(tempFilePath);
        return false;
    }

    return true;
}

bool CbzBuilder::readComicInfo(const QString &cbzPath, ComicInfoMetadata &outMeta)
{
    int err = 0;
    zip_t *za = zip_open(cbzPath.toUtf8().constData(), ZIP_RDONLY, &err);
    if (!za)
        return false;

    zip_file_t *zf = zip_fopen(za, "ComicInfo.xml", ZIP_FL_NODIR);
    if (!zf) {
        // Try case-insensitive lookup
        const zip_int64_t idx = zip_name_locate(za, "ComicInfo.xml", ZIP_FL_NOCASE | ZIP_FL_NODIR);
        if (idx >= 0)
            zf = zip_fopen_index(za, idx, 0);
    }

    if (!zf) {
        zip_close(za);
        return false;
    }

    QByteArray xmlContent;
    char buf[4096];
    zip_int64_t bytesRead = 0;
    while ((bytesRead = zip_fread(zf, buf, sizeof(buf))) > 0) {
        xmlContent.append(buf, static_cast<int>(bytesRead));
    }
    zip_fclose(zf);
    zip_close(za);

    QXmlStreamReader reader(xmlContent);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            const QString tag = reader.name().toString();
            if (tag.compare(QStringLiteral("Series"), Qt::CaseInsensitive) == 0) {
                outMeta.series = reader.readElementText();
            } else if (tag.compare(QStringLiteral("Title"), Qt::CaseInsensitive) == 0) {
                outMeta.title = reader.readElementText();
            } else if (tag.compare(QStringLiteral("Number"), Qt::CaseInsensitive) == 0) {
                outMeta.number = reader.readElementText();
            } else if (tag.compare(QStringLiteral("PageCount"), Qt::CaseInsensitive) == 0) {
                outMeta.pageCount = reader.readElementText().toInt();
            }
        }
    }

    return !reader.hasError();
}

QStringList CbzBuilder::getImageEntries(const QString &cbzPath)
{
    QStringList result;
    int err = 0;
    zip_t *za = zip_open(cbzPath.toUtf8().constData(), ZIP_RDONLY, &err);
    if (!za)
        return result;

    const zip_int64_t numEntries = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < numEntries; ++i) {
        const char *name = zip_get_name(za, i, 0);
        if (!name) continue;
        const QString qName = QString::fromUtf8(name);
        if (isImageFilename(qName)) {
            result.append(qName);
        }
    }
    zip_close(za);

    std::sort(result.begin(), result.end(), naturalSortLessThan);
    return result;
}

QByteArray CbzBuilder::extractEntry(const QString &cbzPath, const QString &entryName)
{
    if (entryName.contains(QLatin1String("..")) || entryName.startsWith(QLatin1Char('/')))
        return {};

    int err = 0;
    zip_t *za = zip_open(cbzPath.toUtf8().constData(), ZIP_RDONLY, &err);
    if (!za)
        return {};

    zip_file_t *zf = zip_fopen(za, entryName.toUtf8().constData(), 0);
    if (!zf) {
        // Try case-insensitive or without directory
        const zip_int64_t idx = zip_name_locate(za, entryName.toUtf8().constData(), ZIP_FL_NOCASE);
        if (idx >= 0)
            zf = zip_fopen_index(za, idx, 0);
    }

    if (!zf) {
        zip_close(za);
        return {};
    }

    QByteArray data;
    char buf[8192];
    zip_int64_t bytesRead = 0;
    while ((bytesRead = zip_fread(zf, buf, sizeof(buf))) > 0) {
        data.append(buf, static_cast<int>(bytesRead));
    }

    zip_fclose(zf);
    zip_close(za);
    return data;
}

QImage CbzBuilder::extractImage(const QString &cbzPath, const QString &entryName)
{
    const QByteArray bytes = extractEntry(cbzPath, entryName);
    if (bytes.isEmpty())
        return {};

    QImage img;
    img.loadFromData(bytes);
    return img;
}

} // namespace mangadl
