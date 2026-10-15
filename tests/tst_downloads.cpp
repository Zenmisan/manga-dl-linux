#include <QtTest>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QProcess>
#include <QTemporaryDir>
#include <zip.h>

#include "db/MangaDatabase.h"
#include "db/DownloadsModel.h"
#include "downloads/CbzBuilder.h"
#include "downloads/NovelEpubBuilder.h"
#include "downloads/DownloadManager.h"
#include "local/ArchiveInspector.h"
#include "local/EpubParser.h"
#include "local/LocalImportService.h"
#include "util/AppPaths.h"
#include "util/FileNaming.h"

using namespace mangadl;

class tst_downloads : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 1. CbzBuilder tests
    void testCbzBuilderStoreAndComicInfo();
    void testCbzBuilderInMemoryExtraction();

    // 2. NovelEpubBuilder tests
    void testNovelEpubBuilderLayoutAndMimetype();
    void testNovelHtmlToPlainParagraphs();

    // 3. ArchiveInspector tests
    void testArchiveInspectorNaturalSort();
    void testArchiveInspectorFilenameParsing();

    // 4. EpubParser tests
    void testEpubParserRoundtrip();

    // 5. LocalImportService tests
    void testLocalImportServiceCbzAndFolder();
    void testLocalImportServiceEpub();
    void testLocalImportUnsupported();

    // 6. DownloadManager & DownloadsModel tests
    void testDownloadManagerQueueAndPause();
    void testDownloadsModelStorageUsed();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QByteArray createTestImageBytes(const QString &label, int width = 100, int height = 150);
};

QByteArray tst_downloads::createTestImageBytes(const QString &label, int width, int height)
{
    QImage img(width, height, QImage::Format_RGB32);
    img.fill(QColor(QStringLiteral("#2E384D")));
    QPainter p(&img);
    p.setPen(Qt::white);
    p.drawText(img.rect(), Qt::AlignCenter, label);
    p.end();

    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "JPG", 85);
    return bytes;
}

void tst_downloads::initTestCase()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    AppPaths::setRootOverride(m_tempDir->path());
    MangaDatabase::instance()->initialize();
}

void tst_downloads::cleanupTestCase()
{
    AppPaths::setRootOverride(QString());
}

void tst_downloads::testCbzBuilderStoreAndComicInfo()
{
    const QString targetCbz = QDir(m_tempDir->path()).filePath(QStringLiteral("test_manga/Ch_01.cbz"));

    QList<QByteArray> pages = {
        createTestImageBytes(QStringLiteral("Page 1")),
        createTestImageBytes(QStringLiteral("Page 2")),
        createTestImageBytes(QStringLiteral("Page 3"))
    };
    QStringList exts = {QStringLiteral(".jpg"), QStringLiteral(".jpg"), QStringLiteral(".jpg")};

    QString err;
    const bool ok = CbzBuilder::buildCbz(
        targetCbz,
        QStringLiteral("Test Manga"),
        QStringLiteral("Chapter 1: The Beginning"),
        QStringLiteral("1"),
        pages,
        exts,
        &err);

    QVERIFY2(ok, qPrintable(err));
    QVERIFY(QFile::exists(targetCbz));

    // Verify with libzip directly
    int zerr = 0;
    zip_t *za = zip_open(targetCbz.toUtf8().constData(), ZIP_RDONLY, &zerr);
    QVERIFY(za != nullptr);

    const zip_int64_t numEntries = zip_get_num_entries(za, 0);
    QCOMPARE(numEntries, 4); // 3 pages + ComicInfo.xml

    // Verify page entries are STORE (no compression)
    for (zip_int64_t i = 0; i < 3; ++i) {
        zip_stat_t st;
        zip_stat_index(za, i, 0, &st);
        QCOMPARE(st.comp_method, static_cast<zip_uint16_t>(ZIP_CM_STORE));
    }

    zip_close(za);

    // Verify ComicInfo.xml metadata reading
    ComicInfoMetadata meta;
    const bool readOk = CbzBuilder::readComicInfo(targetCbz, meta);
    QVERIFY(readOk);
    QCOMPARE(meta.series, QStringLiteral("Test Manga"));
    QCOMPARE(meta.title, QStringLiteral("Chapter 1: The Beginning"));
    QCOMPARE(meta.number, QStringLiteral("1"));
    QCOMPARE(meta.pageCount, 3);

    // Verify image entries list in natural order
    const QStringList imgEntries = CbzBuilder::getImageEntries(targetCbz);
    QCOMPARE(imgEntries.size(), 3);
    QCOMPARE(imgEntries[0], QStringLiteral("0001.jpg"));
    QCOMPARE(imgEntries[1], QStringLiteral("0002.jpg"));
    QCOMPARE(imgEntries[2], QStringLiteral("0003.jpg"));

    // Verify with external unzip -t tool if available on the system
    QProcess unzip;
    unzip.start(QStringLiteral("unzip"), {QStringLiteral("-t"), targetCbz});
    if (unzip.waitForFinished(3000)) {
        QCOMPARE(unzip.exitCode(), 0);
    }
}

void tst_downloads::testCbzBuilderInMemoryExtraction()
{
    const QString targetCbz = QDir(m_tempDir->path()).filePath(QStringLiteral("test_manga/Ch_01.cbz"));
    QVERIFY(QFile::exists(targetCbz));

    // Extract entry bytes directly in memory
    const QByteArray entryBytes = CbzBuilder::extractEntry(targetCbz, QStringLiteral("0001.jpg"));
    QVERIFY(!entryBytes.isEmpty());

    // Extract QImage directly in memory
    const QImage img = CbzBuilder::extractImage(targetCbz, QStringLiteral("0001.jpg"));
    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), 100);
    QCOMPARE(img.height(), 150);
}

void tst_downloads::testNovelEpubBuilderLayoutAndMimetype()
{
    const QString targetEpub = QDir(m_tempDir->path()).filePath(QStringLiteral("test_novel/Ch_01.epub"));

    const QString htmlContent = QStringLiteral(
        "<h1>Prologue</h1><p>It was a dark and stormy night.</p><p>A hero appeared &amp; saved the realm.</p>");

    QString err;
    const bool ok = NovelEpubBuilder::buildEpub(
        targetEpub,
        QStringLiteral("Test Novel Series"),
        QStringLiteral("Chapter 1: Prologue"),
        htmlContent,
        QStringLiteral("html"),
        &err);

    QVERIFY2(ok, qPrintable(err));
    QVERIFY(QFile::exists(targetEpub));

    // Verify EPUB specification requirements with libzip:
    // 1. 'mimetype' must be the first entry (index 0).
    // 2. 'mimetype' must be uncompressed (ZIP_CM_STORE).
    // 3. 'mimetype' content must be "application/epub+zip".
    int zerr = 0;
    zip_t *za = zip_open(targetEpub.toUtf8().constData(), ZIP_RDONLY, &zerr);
    QVERIFY(za != nullptr);

    const char *firstEntryName = zip_get_name(za, 0, 0);
    QCOMPARE(QString::fromUtf8(firstEntryName), QStringLiteral("mimetype"));

    zip_stat_t st;
    zip_stat_index(za, 0, 0, &st);
    QCOMPARE(st.comp_method, static_cast<zip_uint16_t>(ZIP_CM_STORE));

    zip_file_t *zf = zip_fopen_index(za, 0, 0);
    QVERIFY(zf != nullptr);
    char mimeBuf[64] = {0};
    const zip_int64_t n = zip_fread(zf, mimeBuf, sizeof(mimeBuf) - 1);
    zip_fclose(zf);
    QCOMPARE(QByteArray(mimeBuf, static_cast<int>(n)), QByteArray("application/epub+zip"));

    // Check required EPUB structure files
    QVERIFY(zip_name_locate(za, "META-INF/container.xml", 0) >= 0);
    QVERIFY(zip_name_locate(za, "OEBPS/content.opf", 0) >= 0);
    QVERIFY(zip_name_locate(za, "OEBPS/nav.xhtml", 0) >= 0);
    QVERIFY(zip_name_locate(za, "OEBPS/chapter.xhtml", 0) >= 0);
    QVERIFY(zip_name_locate(za, "OEBPS/style.css", 0) >= 0);

    zip_close(za);

    // Verify with external unzip -t if present
    QProcess unzip;
    unzip.start(QStringLiteral("unzip"), {QStringLiteral("-t"), targetEpub});
    if (unzip.waitForFinished(3000)) {
        QCOMPARE(unzip.exitCode(), 0);
    }
}

void tst_downloads::testNovelHtmlToPlainParagraphs()
{
    const QString input = QStringLiteral("<p>First line<br/>Second line</p><p>Third line &amp; &quot;quoted&quot;</p>");
    const QString plain = NovelEpubBuilder::htmlToPlainParagraphs(input);

    QVERIFY(plain.contains(QStringLiteral("First line")));
    QVERIFY(plain.contains(QStringLiteral("Second line")));
    QVERIFY(plain.contains(QStringLiteral("& \"quoted\"")));
    QVERIFY(!plain.contains(QStringLiteral("<p>")));
    QVERIFY(!plain.contains(QStringLiteral("<br/>")));
}

void tst_downloads::testArchiveInspectorNaturalSort()
{
    // Natural compare asserts "Chapter 2" < "Chapter 10"
    QVERIFY(ArchiveInspector::naturalCompare(QStringLiteral("Chapter 2"), QStringLiteral("Chapter 10")));
    QVERIFY(ArchiveInspector::naturalCompare(QStringLiteral("01.jpg"), QStringLiteral("02.jpg")));
    QVERIFY(ArchiveInspector::naturalCompare(QStringLiteral("page2.png"), QStringLiteral("page10.png")));
    QVERIFY(!ArchiveInspector::naturalCompare(QStringLiteral("Chapter 10"), QStringLiteral("Chapter 2")));
}

void tst_downloads::testArchiveInspectorFilenameParsing()
{
    // Range parsing
    const auto rangeMeta = ArchiveInspector::parseArchiveFilename(QStringLiteral("Solo_Leveling_01-10.zip"));
    QCOMPARE(rangeMeta.seriesTitle, QStringLiteral("Solo Leveling"));
    QVERIFY(rangeMeta.isRange);
    QCOMPARE(rangeMeta.rangeStart, 1.0);
    QCOMPARE(rangeMeta.rangeEnd, 10.0);

    // Single chapter parsing
    const auto singleMeta = ArchiveInspector::parseArchiveFilename(QStringLiteral("One_Piece_Chapter_1080.cbz"));
    QCOMPARE(singleMeta.seriesTitle, QStringLiteral("One Piece"));
    QVERIFY(!singleMeta.isRange);
    QCOMPARE(singleMeta.rangeStart, 1080.0);

    // Stripping release group brackets
    const auto groupMeta = ArchiveInspector::parseArchiveFilename(QStringLiteral("[ScanGroup] Bleach - c005 (v01) [Digital].cbz"));
    QCOMPARE(groupMeta.seriesTitle, QStringLiteral("Bleach"));
    QCOMPARE(groupMeta.rangeStart, 5.0);

    // Chapter number extraction
    QCOMPARE(ArchiveInspector::extractChapterNumber(QStringLiteral("Chapter 42.5")), 42.5);
    QCOMPARE(ArchiveInspector::extractChapterNumber(QStringLiteral("c10")), 10.0);
}

void tst_downloads::testEpubParserRoundtrip()
{
    const QString targetEpub = QDir(m_tempDir->path()).filePath(QStringLiteral("test_novel/Ch_01.epub"));
    QVERIFY(QFile::exists(targetEpub));

    const EpubBook book = EpubParser::parse(targetEpub);
    QCOMPARE(book.title, QStringLiteral("Chapter 1: Prologue"));
    QCOMPARE(book.chapters.size(), 1);
    QVERIFY(book.chapters[0].html.contains(QStringLiteral("Prologue")));
    QVERIFY(book.chapters[0].html.contains(QStringLiteral("It was a dark and stormy night.")));
}

void tst_downloads::testLocalImportServiceCbzAndFolder()
{
    LocalImportService importService(MangaDatabase::instance(), m_tempDir->path());

    // 1. Create a dummy CBZ to import
    const QString sampleCbz = QDir(m_tempDir->path()).filePath(QStringLiteral("source_import/Berserk_Ch_01.cbz"));
    QList<QByteArray> pages = { createTestImageBytes(QStringLiteral("B1")), createTestImageBytes(QStringLiteral("B2")) };
    CbzBuilder::buildCbz(sampleCbz, QStringLiteral("Berserk"), QStringLiteral("Chapter 1"), QStringLiteral("1"), pages);

    // Import standalone file
    const auto fileResults = importService.importStandaloneFile(sampleCbz);
    QCOMPARE(fileResults.size(), 1);
    QCOMPARE(fileResults[0].status, LocalImportStatus::Done);

    // Verify series was added to Library
    auto libEntry = MangaDatabase::instance()->getLibraryItem(LocalImportService::Provider, QStringLiteral("berserk"));
    QVERIFY(libEntry.has_value());
    QCOMPARE(libEntry->title, QStringLiteral("Berserk"));

    // Verify download entity was created
    auto dl = MangaDatabase::instance()->getDownload(LocalImportService::Provider, QStringLiteral("berserk"), QStringLiteral("chapter-1"));
    QVERIFY(dl.has_value());
    QCOMPARE(dl->status, QStringLiteral("completed"));
    QVERIFY(!dl->cbzPath.isEmpty());
    QVERIFY(QFile::exists(dl->cbzPath));

    // 2. Create a loose image folder to import as a chapter
    const QString imgFolder = QDir(m_tempDir->path()).filePath(QStringLiteral("source_folder/Kingdom_Ch_02"));
    QDir().mkpath(imgFolder);
    QFile f1(QDir(imgFolder).filePath(QStringLiteral("01.jpg")));
    QVERIFY(f1.open(QIODevice::WriteOnly));
    f1.write(createTestImageBytes(QStringLiteral("K1")));
    f1.close();

    QFile f2(QDir(imgFolder).filePath(QStringLiteral("02.jpg")));
    QVERIFY(f2.open(QIODevice::WriteOnly));
    f2.write(createTestImageBytes(QStringLiteral("K2")));
    f2.close();

    const auto folderResults = importService.importFolder(imgFolder);
    QCOMPARE(folderResults.size(), 1);
    QCOMPARE(folderResults[0].status, LocalImportStatus::Done);

    auto kingdomEntry = MangaDatabase::instance()->getLibraryItem(LocalImportService::Provider, QStringLiteral("kingdom-ch-02"));
    QVERIFY(kingdomEntry.has_value());
}

void tst_downloads::testLocalImportServiceEpub()
{
    LocalImportService importService(MangaDatabase::instance(), m_tempDir->path());

    const QString sampleEpub = QDir(m_tempDir->path()).filePath(QStringLiteral("source_import/Overlord_Vol_01.epub"));
    NovelEpubBuilder::buildEpub(
        sampleEpub,
        QStringLiteral("Overlord"),
        QStringLiteral("Volume 1: The Undead King"),
        QStringLiteral("<p>Ainz Ooal Gown awoke in the Great Tomb of Nazarick.</p>"));

    const auto epubResults = importService.importStandaloneFile(sampleEpub);
    QCOMPARE(epubResults.size(), 1);
    QCOMPARE(epubResults[0].status, LocalImportStatus::Done);

    auto overlordLib = MangaDatabase::instance()->getLibraryItem(LocalImportService::Provider, QStringLiteral("volume-1-the-undead-king"));
    QVERIFY(overlordLib.has_value());
    QCOMPARE(overlordLib->type, QStringLiteral("novel"));
}

void tst_downloads::testLocalImportUnsupported()
{
    LocalImportService importService(MangaDatabase::instance(), m_tempDir->path());

    const QString txtFile = QDir(m_tempDir->path()).filePath(QStringLiteral("dummy.txt"));
    QFile f(txtFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("Just some text");
    f.close();

    const auto res = importService.importStandaloneFile(txtFile);
    QCOMPARE(res.size(), 1);
    QCOMPARE(res[0].status, LocalImportStatus::Unsupported);
}

void tst_downloads::testDownloadManagerQueueAndPause()
{
    DownloadManager &dm = DownloadManager::instance();
    DownloadsModel &model = DownloadsModel::instance();

    dm.pause();
    QVERIFY(dm.isPaused());

    // Enqueue an item
    dm.enqueue(
        QStringLiteral("mangadex"),
        QStringLiteral("naruto"),
        QStringLiteral("Naruto"),
        QStringLiteral("ch-01"),
        QStringLiteral("Chapter 1"),
        1.0);

    auto item = MangaDatabase::instance()->getDownload(QStringLiteral("mangadex"), QStringLiteral("naruto"), QStringLiteral("ch-01"));
    QVERIFY(item.has_value());
    QCOMPARE(item->status, QStringLiteral("queued"));

    model.refresh();
    QVERIFY(model.queueCount() >= 1);

    // Clean up
    dm.cancelDownload(item->id);
    auto afterCancel = MangaDatabase::instance()->getDownload(QStringLiteral("mangadex"), QStringLiteral("naruto"), QStringLiteral("ch-01"));
    QVERIFY(!afterCancel.has_value());

    dm.resume();
    QVERIFY(!dm.isPaused());
}

void tst_downloads::testDownloadsModelStorageUsed()
{
    DownloadsModel &model = DownloadsModel::instance();
    const QString storage = model.storageUsed();
    QVERIFY(!storage.isEmpty());
    // Should contain a valid unit: B, KB, MB, or GB
    QVERIFY(storage.endsWith(QStringLiteral("B")) || storage.endsWith(QStringLiteral("KB")) ||
            storage.endsWith(QStringLiteral("MB")) || storage.endsWith(QStringLiteral("GB")));
}

QTEST_MAIN(tst_downloads)
#include "tst_downloads.moc"
