#include <QtTest/QtTest>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <zlib.h>

#include "src/core/db/MangaDatabase.h"
#include "src/core/db/Entities.h"
#include "src/core/sync/AuthModels.h"
#include "src/core/sync/SupabaseAuthService.h"
#include "src/core/sync/SupabaseSyncModels.h"
#include "src/core/sync/SupabaseSyncService.h"
#include "src/core/tracking/OAuthLoopbackListener.h"
#include "src/core/tracking/TrackerModels.h"
#include "src/core/backup/MangadlBackupModels.h"
#include "src/core/backup/TachibkModels.h"
#include "src/core/backup/TachibkParser.h"
#include "src/core/backup/BackupService.h"

using namespace mangadl;

class tst_sync : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 1. Supabase models & serialization
    void testSupabaseSyncModelsSerialization();

    // 2. Database operations used in sync
    void testSupabaseSyncDatabaseOperations();

    // 3. Backup & restore roundtrip
    void testMangadlBackupRoundtrip();

    // 4. Tachiyomi Protobuf decoder & gzip
    void testTachibkParserProtobufDecoder();

    // 5. Tachiyomi JSON decoder & URL cleaners
    void testTachibkJsonBackupAndResolvers();

    // 6. Supabase auth helpers (PKCE, URL, error normalization)
    void testSupabaseAuthHelpers();

    // 7. OAuth loopback listener
    void testOAuthLoopbackListener();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_dbPath;
    std::unique_ptr<MangaDatabase> m_db;

    // Helper to encode protobuf varint
    static void appendVarint(QByteArray &buf, quint64 val) {
        while (val > 0x7F) {
            buf.append(static_cast<char>((val & 0x7F) | 0x80));
            val >>= 7;
        }
        buf.append(static_cast<char>(val & 0x7F));
    }

    // Helper to encode protobuf length-delimited field
    static void appendLengthDelimited(QByteArray &buf, int fieldNum, const QByteArray &data) {
        appendVarint(buf, (static_cast<quint64>(fieldNum) << 3) | 2);
        appendVarint(buf, data.size());
        buf.append(data);
    }

    // Helper to encode protobuf string field
    static void appendString(QByteArray &buf, int fieldNum, const QString &str) {
        appendLengthDelimited(buf, fieldNum, str.toUtf8());
    }

    // Helper to encode protobuf int64 varint field
    static void appendInt64(QByteArray &buf, int fieldNum, quint64 val) {
        appendVarint(buf, (static_cast<quint64>(fieldNum) << 3) | 0);
        appendVarint(buf, val);
    }

    // Helper to encode protobuf 32-bit fixed float field
    static void appendFloat(QByteArray &buf, int fieldNum, float val) {
        appendVarint(buf, (static_cast<quint64>(fieldNum) << 3) | 5);
        quint32 bits = 0;
        memcpy(&bits, &val, 4);
        buf.append(reinterpret_cast<const char *>(&bits), 4);
    }

    // Helper to gzip compress
    static QByteArray gzipCompress(const QByteArray &data) {
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);

        strm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
        strm.avail_in = data.size();

        QByteArray out;
        out.resize(data.size() + 1024);
        strm.next_out = reinterpret_cast<Bytef *>(out.data());
        strm.avail_out = out.size();

        deflate(&strm, Z_FINISH);
        out.resize(strm.total_out);
        deflateEnd(&strm);
        return out;
    }
};

void tst_sync::initTestCase()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    m_dbPath = m_tempDir->filePath("test_sync.db");
    m_db = std::make_unique<MangaDatabase>(m_dbPath);
    QVERIFY(m_db->initialize());
}

void tst_sync::cleanupTestCase()
{
    if (m_db) {
        m_db->close();
        m_db.reset();
    }
    m_tempDir.reset();
}

void tst_sync::testSupabaseSyncModelsSerialization()
{
    // Test SupabaseSubscriptionRecord
    SupabaseSubscriptionRecord sub;
    sub.id = "mangadex/one-piece";
    sub.userId = "test-user-123";
    sub.mangaId = "one-piece";
    sub.provider = "mangadex";
    sub.title = "One Piece";
    sub.coverUrl = "https://example.com/op.jpg";
    sub.type = "manga";
    sub.addedAt = "2026-10-15T12:00:00Z";

    const QJsonObject subJson = sub.toJson();
    QCOMPARE(subJson["id"].toString(), QString("mangadex/one-piece"));
    QCOMPARE(subJson["user_id"].toString(), QString("test-user-123"));
    QCOMPARE(subJson["manga_id"].toString(), QString("one-piece"));
    QCOMPARE(subJson["provider"].toString(), QString("mangadex"));
    QCOMPARE(subJson["title"].toString(), QString("One Piece"));
    QCOMPARE(subJson["cover_url"].toString(), QString("https://example.com/op.jpg"));
    QCOMPARE(subJson["type"].toString(), QString("manga"));

    const auto subParsed = SupabaseSubscriptionRecord::fromJson(subJson);
    QCOMPARE(subParsed.id, sub.id);
    QCOMPARE(subParsed.userId, sub.userId);
    QCOMPARE(subParsed.mangaId, sub.mangaId);
    QCOMPARE(subParsed.provider, sub.provider);
    QCOMPARE(subParsed.title, sub.title);
    QCOMPARE(subParsed.coverUrl, sub.coverUrl);
    QCOMPARE(subParsed.type, sub.type);

    // Test SupabaseReadingProgressRecord
    SupabaseReadingProgressRecord prog;
    prog.userId = "test-user-123";
    prog.mangaId = "frieren";
    prog.provider = "mangadex";
    prog.chapterId = "ch-10";
    prog.lastPage = 15;
    prog.mangaTitle = "Frieren";
    prog.chapterTitle = "Chapter 10";
    prog.updatedAt = "2026-10-15T12:00:00Z";

    const QJsonObject progJson = prog.toJson();
    QCOMPARE(progJson["user_id"].toString(), QString("test-user-123"));
    QCOMPARE(progJson["manga_id"].toString(), QString("frieren"));
    QCOMPARE(progJson["provider"].toString(), QString("mangadex"));
    QCOMPARE(progJson["chapter_id"].toString(), QString("ch-10"));
    QCOMPARE(progJson["last_page"].toInt(), 15);
    QCOMPARE(progJson["manga_title"].toString(), QString("Frieren"));
    QCOMPARE(progJson["chapter_title"].toString(), QString("Chapter 10"));

    const auto progParsed = SupabaseReadingProgressRecord::fromJson(progJson);
    QCOMPARE(progParsed.userId, prog.userId);
    QCOMPARE(progParsed.mangaId, prog.mangaId);
    QCOMPARE(progParsed.provider, prog.provider);
    QCOMPARE(progParsed.chapterId, prog.chapterId);
    QCOMPARE(progParsed.lastPage, prog.lastPage);
    QCOMPARE(progParsed.mangaTitle, prog.mangaTitle);
    QCOMPARE(progParsed.chapterTitle, prog.chapterTitle);
}

void tst_sync::testSupabaseSyncDatabaseOperations()
{
    auto *db = m_db.get();

    // 1. Insert local manga entry
    LibraryEntity localManga;
    localManga.id = "mangadex/sync-test-1";
    localManga.provider = "mangadex";
    localManga.mangaId = "sync-test-1";
    localManga.title = "Original Title";
    localManga.coverUrl = "https://example.com/cover1.jpg";
    localManga.totalChapters = 10;
    localManga.updatedAt = 1000;
    QVERIFY(db->addToLibrary(localManga));

    auto retrieved = db->getLibraryItem("mangadex", "sync-test-1");
    QVERIFY(retrieved.has_value());
    QCOMPARE(retrieved->title, QString("Original Title"));
    QCOMPARE(retrieved->totalChapters, 10);

    // 2. Reading progress
    QVERIFY(db->saveProgress("mangadex", "sync-test-1", "ch-1", 1.0, 18, true));
    auto progRetrieved = db->getProgress("mangadex", "sync-test-1", "ch-1");
    QVERIFY(progRetrieved.has_value());
    QCOMPARE(progRetrieved->page, 18);
    QCOMPARE(progRetrieved->completed, 1);
}

void tst_sync::testMangadlBackupRoundtrip()
{
    auto *db = m_db.get();

    // 1. Insert custom category
    QVERIFY(db->addCategory("favorites", "Favorites", 10));

    // 2. Insert library entry
    LibraryEntity manga;
    manga.id = "mangadex/backup-manga-1";
    manga.provider = "mangadex";
    manga.mangaId = "backup-manga-1";
    manga.title = "Backup Manga";
    manga.coverUrl = "https://example.com/cover.jpg";
    manga.totalChapters = 50;
    manga.updatedAt = 1600000;
    QVERIFY(db->addToLibrary(manga));
    QVERIFY(db->setMangaCategories("mangadex", "backup-manga-1", {"favorites"}));

    // 3. Insert progress
    QVERIFY(db->saveProgress("mangadex", "backup-manga-1", "ch-5", 5.0, 12, false));

    // 4. Insert tracker bind
    TrackerBindEntity bind;
    bind.id = "mangadex/backup-manga-1/AniList";
    bind.provider = "mangadex";
    bind.mangaId = "backup-manga-1";
    bind.tracker = "AniList";
    bind.remoteId = 109283;
    bind.remoteTitle = "AniList Title";
    bind.lastChapterRead = 5.0;
    bind.updatedAt = 1600100;
    QVERIFY(db->saveTrackerBind(bind));

    // 5. Export backup
    const QString backupPath = m_tempDir->filePath("test_backup.mangadl");
    BackupService backupService(db, nullptr);
    QVERIFY(backupService.exportBackupFile(backupPath));

    QFile f(backupPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray content = f.readAll();
    f.close();
    QVERIFY(!content.isEmpty());

    QJsonDocument doc = QJsonDocument::fromJson(content);
    QVERIFY(doc.isObject());
    const QJsonObject root = doc.object();
    QCOMPARE(root["version"].toString(), QString("1.0"));
    QCOMPARE(root["app"].toString(), QString("manga-dl"));
    QVERIFY(root["library"].isArray());
    QVERIFY(root["categories"].isArray());
    QVERIFY(root["progress"].isArray());
    QVERIFY(root["trackerBinds"].isArray());

    // 6. Delete manga and category
    QVERIFY(db->removeFromLibrary("mangadex", "backup-manga-1"));
    QVERIFY(db->deleteCategory("favorites"));
    QVERIFY(!db->getLibraryItem("mangadex", "backup-manga-1").has_value());

    // 7. Restore backup
    const auto summary = backupService.restoreBackupFile(backupPath);
    QVERIFY(summary.mangaRestored >= 1);
    QVERIFY(summary.categoriesRestored >= 1);
    QVERIFY(summary.progressRestored >= 1);
    QVERIFY(summary.trackerBindsRestored >= 1);

    // 8. Verify restored state
    auto restoredManga = db->getLibraryItem("mangadex", "backup-manga-1");
    QVERIFY(restoredManga.has_value());
    QCOMPARE(restoredManga->title, QString("Backup Manga"));

    const auto restoredCats = db->getMangaCategories("mangadex", "backup-manga-1");
    QVERIFY(restoredCats.contains("favorites"));

    auto restoredProg = db->getProgress("mangadex", "backup-manga-1", "ch-5");
    QVERIFY(restoredProg.has_value());
    QCOMPARE(restoredProg->page, 12);

    auto restoredBind = db->getTrackerBind("mangadex", "backup-manga-1", "AniList");
    QVERIFY(restoredBind.has_value());
    QCOMPARE(restoredBind->remoteId, 109283);
}

void tst_sync::testTachibkParserProtobufDecoder()
{
    // Build binary protobuf buffer according to Tachiyomi / Mihon schema
    QByteArray proto;

    // 1. BackupCategory (Field 2 of root)
    QByteArray catMsg;
    appendString(catMsg, 1, "Favorites"); // name
    appendInt64(catMsg, 2, 1);            // order
    appendLengthDelimited(proto, 2, catMsg);

    // 2. BackupSource (Field 101 of root)
    QByteArray srcMsg;
    appendString(srcMsg, 1, "MangaDex");                 // name
    appendInt64(srcMsg, 2, 2499280145261642279ULL);     // sourceId
    appendLengthDelimited(proto, 101, srcMsg);

    // 3. BackupManga (Field 1 of root)
    QByteArray mangaMsg;
    appendInt64(mangaMsg, 1, 2499280145261642279ULL);   // source
    appendString(mangaMsg, 2, "/title/f98660a0-81e9-4eec-b85b-424e17ff15eb"); // url
    appendString(mangaMsg, 3, "Frieren: Beyond Journey's End");              // title
    appendString(mangaMsg, 4, "Abe Tsukasa");                                 // artist
    appendString(mangaMsg, 5, "Yamada Kanehito");                             // author
    appendString(mangaMsg, 9, "https://example.com/frieren.jpg");             // thumbnailUrl

    // Chapter inside mangaMsg (Field 16)
    QByteArray chMsg;
    appendString(chMsg, 1, "/chapter/98765432-1111-2222-3333-444455556666"); // url
    appendString(chMsg, 2, "Chapter 10");                                      // name
    appendInt64(chMsg, 4, 1);                                                  // read (bool varint)
    appendInt64(chMsg, 6, 15);                                                 // lastPageRead
    appendFloat(chMsg, 9, 10.0f);                                              // chapterNumber (float 32-bit fixed)
    appendLengthDelimited(mangaMsg, 16, chMsg);

    // Attach manga to root
    appendLengthDelimited(proto, 1, mangaMsg);

    // Compress with gzip
    const QByteArray gzipped = gzipCompress(proto);
    QVERIFY(TachibkParser::isGzip(gzipped));

    // Parse backup
    const auto result = TachibkParser::parseTachiyomiBackup(gzipped, "test.tachibk");
    QCOMPARE(result.categories.size(), 1);
    QCOMPARE(result.categories[0].name, QString("Favorites"));

    QCOMPARE(result.manga.size(), 1);
    const auto &parsedManga = result.manga[0];
    QCOMPARE(parsedManga.title, QString("Frieren: Beyond Journey's End"));
    QCOMPARE(parsedManga.author, QString("Yamada Kanehito"));
    QCOMPARE(parsedManga.thumbnailUrl, QString("https://example.com/frieren.jpg"));

    // Verify parsed chapters
    QCOMPARE(parsedManga.chapters.size(), 1);
    const auto &parsedCh = parsedManga.chapters[0];
    QCOMPARE(parsedCh.name, QString("Chapter 10"));
    QCOMPARE(parsedCh.read, true);
    QCOMPARE(parsedCh.lastPageRead, 15);
    QCOMPARE(parsedCh.chapterNumber, 10.0);
}

void tst_sync::testTachibkJsonBackupAndResolvers()
{
    // Test legacy JSON backup
    const QString jsonContent = R"({
        "version": 2,
        "categories": [
            { "name": "Reading", "order": 0 }
        ],
        "manga": [
            {
                "url": "/manga/chainsaw-man",
                "title": "Chainsaw Man",
                "source": 123456789,
                "author": "Fujimoto Tatsuki",
                "thumbnailUrl": "https://example.com/csm.jpg",
                "chapters": [
                    {
                        "url": "/chapter/csm-1",
                        "name": "Chapter 1",
                        "chapterNumber": 1.0
                    }
                ]
            }
        ]
    })";

    const auto result = TachibkParser::parseJsonBackup(jsonContent.toUtf8());
    QCOMPARE(result.categories.size(), 1);
    QCOMPARE(result.categories[0].name, QString("Reading"));
    QCOMPARE(result.manga.size(), 1);
    QCOMPARE(result.manga[0].title, QString("Chainsaw Man"));

    // Test resolvers
    QCOMPARE(TachibkParser::resolveProvider("MangaDex"), QString("mangadex"));
    QCOMPARE(TachibkParser::resolveProvider("Asura Scans"), QString("asurascans"));
    QCOMPARE(TachibkParser::resolveProvider("Comick"), QString("comick"));
    QCOMPARE(TachibkParser::cleanMangaId("/manga/12345/my-title/"), QString("12345/my-title"));
    QCOMPARE(TachibkParser::cleanChapterId("/chapter/ch-99/"), QString("ch-99"));
}

void tst_sync::testSupabaseAuthHelpers()
{
    // PKCE code verifier and challenge
    const auto pkce = SupabaseAuthService::generatePkceS256();
    const QString verifier = pkce.first;
    const QString challenge = pkce.second;

    QVERIFY(verifier.length() >= 43 && verifier.length() <= 128);
    for (const QChar c : verifier) {
        QVERIFY(c.isLetterOrNumber() || c == '-' || c == '.' || c == '_' || c == '~');
    }
    QVERIFY(!challenge.isEmpty());
    QVERIFY(!challenge.contains('=')); // Base64URL without padding

    // Build OAuth Authorize URL
    SupabaseAuthService auth("https://gyivwfweldwvzccbpgoz.supabase.co", "anon-key");
    const QString oauthUrl = auth.getOAuthAuthorizeUrl("google", "http://127.0.0.1:5678/google-callback", challenge);
    QVERIFY(oauthUrl.contains("gyivwfweldwvzccbpgoz.supabase.co"));
    QVERIFY(oauthUrl.contains("/auth/v1/authorize"));
    QVERIFY(oauthUrl.contains("provider=google"));
    QVERIFY(oauthUrl.contains("code_challenge=" + challenge));
    QVERIFY(oauthUrl.contains("code_challenge_method=S256"));

    // Error normalization
    QCOMPARE(SupabaseAuthService::friendlyError("Invalid login credentials"),
             QString("Incorrect email or password."));
    QCOMPARE(SupabaseAuthService::friendlyError("User already registered"),
             QString("An account with this email already exists."));
    QCOMPARE(SupabaseAuthService::friendlyError("Email not confirmed"),
             QString("Check your email and confirm your account first."));
}

void tst_sync::testOAuthLoopbackListener()
{
    OAuthLoopbackListener listener;
    QVERIFY(listener.start(5678));

    QString receivedCode;
    QString receivedPath;

    connect(&listener, &OAuthLoopbackListener::authCodeReceived,
            [&](const QString &code, const QString &path) {
                receivedCode = code;
                receivedPath = path;
            });

    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", 5678);
    QTRY_COMPARE_WITH_TIMEOUT(socket.state(), QAbstractSocket::ConnectedState, 3000);

    // Spin event loop to allow QTcpServer to dispatch newConnection
    QCoreApplication::processEvents();

    const QByteArray request = "GET /mal-callback?code=mock_code_xyz&state=mock_state HTTP/1.1\r\n"
                               "Host: 127.0.0.1:5678\r\n"
                               "Connection: close\r\n\r\n";
    socket.write(request);
    socket.flush();

    QTRY_COMPARE_WITH_TIMEOUT(receivedCode, QString("mock_code_xyz"), 3000);
    QCOMPARE(receivedPath, QString("/mal-callback"));

    QTRY_VERIFY_WITH_TIMEOUT(socket.bytesAvailable() > 0 || socket.state() == QAbstractSocket::UnconnectedState, 3000);
    const QByteArray response = socket.readAll();
    QVERIFY(response.contains("200 OK") || response.contains("Authentication Successful"));

    listener.stop();
}

QTEST_MAIN(tst_sync)
#include "tst_sync.moc"
