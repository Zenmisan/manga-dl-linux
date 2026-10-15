#include "EpubParser.h"

#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QUrl>
#include <QXmlStreamReader>
#include <zip.h>

namespace mangadl {

static QByteArray readZipEntry(zip_t *za, const QString &entryPath)
{
    zip_file_t *zf = zip_fopen(za, entryPath.toUtf8().constData(), 0);
    if (!zf) {
        // Try case-insensitive locate
        const zip_int64_t idx = zip_name_locate(za, entryPath.toUtf8().constData(), ZIP_FL_NOCASE);
        if (idx >= 0)
            zf = zip_fopen_index(za, idx, 0);
    }

    if (!zf)
        return {};

    QByteArray data;
    char buf[8192];
    zip_int64_t bytesRead = 0;
    while ((bytesRead = zip_fread(zf, buf, sizeof(buf))) > 0) {
        data.append(buf, static_cast<int>(bytesRead));
    }
    zip_fclose(zf);
    return data;
}

QString EpubParser::normalizeZipPath(const QString &path)
{
    QStringList stack;
    const QStringList segments = path.split(QLatin1Char('/'));
    for (const auto &seg : segments) {
        if (seg.isEmpty() || seg == QStringLiteral("."))
            continue;
        if (seg == QStringLiteral("..")) {
            if (!stack.isEmpty())
                stack.removeLast();
            continue;
        }
        stack.append(seg);
    }
    return stack.join(QLatin1Char('/'));
}

QString EpubParser::extractChapterTitle(const QString &html)
{
    static const QRegularExpression titleRegex(
        QStringLiteral(R"(<title[^>]*>(.*?)</title>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const auto titleMatch = titleRegex.match(html);
    if (titleMatch.hasMatch()) {
        QString decoded = titleMatch.captured(1).trimmed();
        // unescape simple xml entities
        decoded.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        decoded.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
        decoded.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
        decoded.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
        if (!decoded.isEmpty())
            return decoded;
    }

    static const QRegularExpression hRegex(
        QStringLiteral(R"(<h[12][^>]*>(.*?)</h[12]>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const auto hMatch = hRegex.match(html);
    if (hMatch.hasMatch()) {
        QString text = hMatch.captured(1);
        text = text.remove(QRegularExpression(QStringLiteral("<[^>]+>"))).trimmed();
        text.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        text.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
        text.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
        text.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
        if (!text.isEmpty())
            return text;
    }

    return {};
}

static EpubBook parseZipArchive(zip_t *za)
{
    EpubBook book;
    if (!za)
        return book;

    // 1. Read META-INF/container.xml
    const QByteArray containerData = readZipEntry(za, QStringLiteral("META-INF/container.xml"));
    if (containerData.isEmpty())
        return book;

    QString rootfilePath;
    QXmlStreamReader containerReader(containerData);
    while (!containerReader.atEnd()) {
        containerReader.readNext();
        if (containerReader.isStartElement() &&
            containerReader.name().toString().compare(QStringLiteral("rootfile"), Qt::CaseInsensitive) == 0) {
            rootfilePath = containerReader.attributes().value(QStringLiteral("full-path")).toString();
            if (!rootfilePath.isEmpty())
                break;
        }
    }

    if (rootfilePath.isEmpty())
        return book;

    // 2. Read OPF file
    const QByteArray opfData = readZipEntry(za, rootfilePath);
    if (opfData.isEmpty())
        return book;

    QString opfDir;
    const int lastSlash = rootfilePath.lastIndexOf(QLatin1Char('/'));
    if (lastSlash >= 0) {
        opfDir = rootfilePath.left(lastSlash);
    }

    QString bookTitle;
    QMap<QString, QString> manifest; // id -> href
    QStringList spineIds;           // list of idrefs

    QXmlStreamReader opfReader(opfData);
    while (!opfReader.atEnd()) {
        opfReader.readNext();
        if (opfReader.isStartElement()) {
            const QString tag = opfReader.name().toString();
            if (tag.compare(QStringLiteral("title"), Qt::CaseInsensitive) == 0) {
                if (bookTitle.isEmpty())
                    bookTitle = opfReader.readElementText().trimmed();
            } else if (tag.compare(QStringLiteral("item"), Qt::CaseInsensitive) == 0) {
                const auto attrs = opfReader.attributes();
                const QString id = attrs.value(QStringLiteral("id")).toString();
                const QString href = attrs.value(QStringLiteral("href")).toString();
                if (!id.isEmpty() && !href.isEmpty()) {
                    manifest.insert(id, href);
                }
            } else if (tag.compare(QStringLiteral("itemref"), Qt::CaseInsensitive) == 0) {
                const QString idref = opfReader.attributes().value(QStringLiteral("idref")).toString();
                if (!idref.isEmpty()) {
                    spineIds.append(idref);
                }
            }
        }
    }

    book.title = bookTitle.isEmpty() ? QStringLiteral("Untitled") : bookTitle;

    // 3. Extract spine items in order
    for (int i = 0; i < spineIds.size(); ++i) {
        const QString &id = spineIds[i];
        if (!manifest.contains(id))
            continue;

        const QString href = QUrl::fromPercentEncoding(manifest.value(id).toUtf8());
        const QString combinedPath = opfDir.isEmpty() ? href : QStringLiteral("%1/%2").arg(opfDir, href);
        const QString normalizedPath = EpubParser::normalizeZipPath(combinedPath);

        QByteArray chapterBytes = readZipEntry(za, normalizedPath);
        if (chapterBytes.isEmpty()) {
            chapterBytes = readZipEntry(za, combinedPath);
        }

        if (chapterBytes.isEmpty())
            continue;

        const QString chapterHtml = QString::fromUtf8(chapterBytes);
        QString chTitle = EpubParser::extractChapterTitle(chapterHtml);
        if (chTitle.isEmpty()) {
            chTitle = QStringLiteral("Chapter %1").arg(book.chapters.size() + 1);
        }

        book.chapters.append(EpubChapter{chTitle, chapterHtml});
    }

    return book;
}

EpubBook EpubParser::parse(const QString &epubFilePath)
{
    int err = 0;
    zip_t *za = zip_open(epubFilePath.toUtf8().constData(), ZIP_RDONLY, &err);
    if (!za)
        return {};

    EpubBook book = parseZipArchive(za);
    zip_close(za);
    return book;
}

EpubBook EpubParser::parseBytes(const QByteArray &epubBytes)
{
    zip_error_t zerr;
    zip_error_init(&zerr);
    zip_source_t *src = zip_source_buffer_create(epubBytes.constData(), epubBytes.size(), 0, &zerr);
    if (!src) {
        zip_error_fini(&zerr);
        return {};
    }

    zip_t *za = zip_open_from_source(src, ZIP_RDONLY, &zerr);
    zip_error_fini(&zerr);
    if (!za) {
        zip_source_free(src);
        return {};
    }

    EpubBook book = parseZipArchive(za);
    zip_close(za);
    return book;
}

} // namespace mangadl
