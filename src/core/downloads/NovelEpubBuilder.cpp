#include "NovelEpubBuilder.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>
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

static const char s_containerXml[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n"
    "  <rootfiles>\n"
    "    <rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n"
    "  </rootfiles>\n"
    "</container>\n";

static const char s_styleCss[] =
    "body {\n"
    "  font-family: Georgia, serif;\n"
    "  line-height: 1.7;\n"
    "  margin: 5%;\n"
    "  color: #1a1a1a;\n"
    "  background-color: #fff;\n"
    "}\n"
    "h1 {\n"
    "  text-align: center;\n"
    "  font-size: 1.6em;\n"
    "  margin-top: 1em;\n"
    "  margin-bottom: 1.5em;\n"
    "  color: #111;\n"
    "}\n"
    "p {\n"
    "  margin-bottom: 1em;\n"
    "  text-indent: 1.5em;\n"
    "}\n";

QString NovelEpubBuilder::htmlToPlainParagraphs(const QString &html)
{
    QString text = html;
    static const QRegularExpression brRegex(QStringLiteral("<br\\s*/?>"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression pCloseRegex(QStringLiteral("</p>"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tagRegex(QStringLiteral("<[^>]+>"));
    static const QRegularExpression entityAmp(QStringLiteral("&amp;"));
    static const QRegularExpression entityLt(QStringLiteral("&lt;"));
    static const QRegularExpression entityGt(QStringLiteral("&gt;"));
    static const QRegularExpression entityQuot(QStringLiteral("&quot;"));
    static const QRegularExpression entityNbsp(QStringLiteral("&nbsp;"));

    text.replace(brRegex, QStringLiteral("\n"));
    text.replace(pCloseRegex, QStringLiteral("\n\n"));
    text.replace(tagRegex, QStringLiteral(" "));

    text.replace(entityNbsp, QStringLiteral(" "));
    text.replace(entityQuot, QStringLiteral("\""));
    text.replace(entityLt, QStringLiteral("<"));
    text.replace(entityGt, QStringLiteral(">"));
    text.replace(entityAmp, QStringLiteral("&"));

    return text.trimmed();
}

static QString buildNavXhtml(const QString &chapterTitle)
{
    const QString escapedTitle = escapeXml(chapterTitle);
    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE html>\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:epub=\"http://www.idpf.org/2007/ops\">\n"
        "  <head>\n"
        "    <title>%1</title>\n"
        "    <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\" />\n"
        "  </head>\n"
        "  <body>\n"
        "    <nav epub:type=\"toc\" id=\"toc\">\n"
        "      <h1>Table of Contents</h1>\n"
        "      <ol>\n"
        "        <li><a href=\"chapter.xhtml\">%1</a></li>\n"
        "      </ol>\n"
        "    </nav>\n"
        "  </body>\n"
        "</html>\n").arg(escapedTitle);
}

static QString buildChapterXhtml(const QString &chapterTitle, const QString &content, const QString &format)
{
    const QString escapedTitle = escapeXml(chapterTitle);
    const QString plain = (format.compare(QStringLiteral("html"), Qt::CaseInsensitive) == 0)
                              ? NovelEpubBuilder::htmlToPlainParagraphs(content)
                              : content;

    const QStringList lines = plain.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    QString body;
    for (const auto &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            body += QStringLiteral("    <p>") + escapeXml(trimmed) + QStringLiteral("</p>\n");
        }
    }
    if (body.isEmpty()) {
        body = QStringLiteral("    <p></p>\n");
    }

    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE html>\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
        "  <head>\n"
        "    <title>%1</title>\n"
        "    <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\" />\n"
        "  </head>\n"
        "  <body>\n"
        "    <h1>%1</h1>\n"
        "%2"
        "  </body>\n"
        "</html>\n").arg(escapedTitle, body);
}

static QString buildContentOpf(const QString &bookId, const QString &chapterTitle, const QString &mangaTitle, const QString &nowIso)
{
    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<package xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"BookId\" version=\"3.0\">\n"
        "  <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n"
        "    <dc:identifier id=\"BookId\">%1</dc:identifier>\n"
        "    <dc:title>%2</dc:title>\n"
        "    <dc:language>en</dc:language>\n"
        "    <dc:creator>%3</dc:creator>\n"
        "    <meta property=\"dcterms:modified\">%4</meta>\n"
        "  </metadata>\n"
        "  <manifest>\n"
        "    <item id=\"nav\" href=\"nav.xhtml\" media-type=\"application/xhtml+xml\" properties=\"nav\"/>\n"
        "    <item id=\"style\" href=\"style.css\" media-type=\"text/css\"/>\n"
        "    <item id=\"chapter\" href=\"chapter.xhtml\" media-type=\"application/xhtml+xml\"/>\n"
        "  </manifest>\n"
        "  <spine>\n"
        "    <itemref idref=\"chapter\"/>\n"
        "  </spine>\n"
        "</package>\n").arg(escapeXml(bookId), escapeXml(chapterTitle), escapeXml(mangaTitle), nowIso);
}

bool NovelEpubBuilder::buildEpub(
    const QString &targetFilePath,
    const QString &mangaTitle,
    const QString &chapterTitle,
    const QString &content,
    const QString &format,
    QString *errorMessage)
{
    const QFileInfo targetInfo(targetFilePath);
    QDir().mkpath(targetInfo.dir().absolutePath());

    const QString tempFilePath = targetFilePath + QStringLiteral(".tmp");
    QFile::remove(tempFilePath);

    int err = 0;
    zip_t *za = zip_open(tempFilePath.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!za) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to create temporary EPUB archive");
        return false;
    }

    // 1. EPUB standard requires 'mimetype' to be the first file and uncompressed (STORE)
    static const char s_mimetype[] = "application/epub+zip";
    zip_source_t *mimeSource = zip_source_buffer(za, s_mimetype, sizeof(s_mimetype) - 1, 0);
    if (!mimeSource) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8(zip_strerror(za));
        zip_discard(za);
        QFile::remove(tempFilePath);
        return false;
    }
    zip_int64_t mimeIdx = zip_file_add(za, "mimetype", mimeSource, ZIP_FL_OVERWRITE);
    if (mimeIdx < 0) {
        zip_source_free(mimeSource);
        zip_discard(za);
        QFile::remove(tempFilePath);
        return false;
    }
    zip_set_file_compression(za, mimeIdx, ZIP_CM_STORE, 0);

    // 2. META-INF/container.xml
    zip_source_t *containerSource = zip_source_buffer(za, s_containerXml, sizeof(s_containerXml) - 1, 0);
    if (containerSource) {
        zip_int64_t idx = zip_file_add(za, "META-INF/container.xml", containerSource, ZIP_FL_OVERWRITE);
        if (idx >= 0)
            zip_set_file_compression(za, idx, ZIP_CM_DEFAULT, 0);
        else
            zip_source_free(containerSource);
    }

    // 3. OEBPS/style.css
    zip_source_t *styleSource = zip_source_buffer(za, s_styleCss, sizeof(s_styleCss) - 1, 0);
    if (styleSource) {
        zip_int64_t idx = zip_file_add(za, "OEBPS/style.css", styleSource, ZIP_FL_OVERWRITE);
        if (idx >= 0)
            zip_set_file_compression(za, idx, ZIP_CM_DEFAULT, 0);
        else
            zip_source_free(styleSource);
    }

    // 4. OEBPS/nav.xhtml
    const QByteArray navXhtmlBytes = buildNavXhtml(chapterTitle).toUtf8();
    zip_source_t *navSource = zip_source_buffer(za, navXhtmlBytes.constData(), navXhtmlBytes.size(), 0);
    if (navSource) {
        zip_int64_t idx = zip_file_add(za, "OEBPS/nav.xhtml", navSource, ZIP_FL_OVERWRITE);
        if (idx >= 0)
            zip_set_file_compression(za, idx, ZIP_CM_DEFAULT, 0);
        else
            zip_source_free(navSource);
    }

    // 5. OEBPS/chapter.xhtml
    const QByteArray chapterXhtmlBytes = buildChapterXhtml(chapterTitle, content, format).toUtf8();
    zip_source_t *chapterSource = zip_source_buffer(za, chapterXhtmlBytes.constData(), chapterXhtmlBytes.size(), 0);
    if (chapterSource) {
        zip_int64_t idx = zip_file_add(za, "OEBPS/chapter.xhtml", chapterSource, ZIP_FL_OVERWRITE);
        if (idx >= 0)
            zip_set_file_compression(za, idx, ZIP_CM_DEFAULT, 0);
        else
            zip_source_free(chapterSource);
    }

    // 6. OEBPS/content.opf
    const QString bookId = QStringLiteral("urn:uuid:%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString nowIso = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ssZ"));
    const QByteArray opfBytes = buildContentOpf(bookId, chapterTitle, mangaTitle, nowIso).toUtf8();
    zip_source_t *opfSource = zip_source_buffer(za, opfBytes.constData(), opfBytes.size(), 0);
    if (opfSource) {
        zip_int64_t idx = zip_file_add(za, "OEBPS/content.opf", opfSource, ZIP_FL_OVERWRITE);
        if (idx >= 0)
            zip_set_file_compression(za, idx, ZIP_CM_DEFAULT, 0);
        else
            zip_source_free(opfSource);
    }

    if (zip_close(za) < 0) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to finalize EPUB archive");
        QFile::remove(tempFilePath);
        return false;
    }

    QFile::remove(targetFilePath);
    if (!QFile::rename(tempFilePath, targetFilePath)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to move temporary EPUB to destination");
        QFile::remove(tempFilePath);
        return false;
    }

    return true;
}

} // namespace mangadl
