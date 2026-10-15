public class MangaDlDb {
    private Sqlite.Database db;

    public MangaDlDb () throws GLib.Error {
        var db_dir = Path.build_filename (
            Environment.get_user_data_dir (), "manga-dl");
        DirUtils.create_with_parents (db_dir, 0755);
        var db_path = Path.build_filename (db_dir, "manga.db");

        if (Sqlite.Database.open (db_path, out db) != Sqlite.OK)
            throw new GLib.IOError.FAILED (db.errmsg ());

        init_schema ();
    }

    private void init_schema () {
        db.exec ("""
            CREATE TABLE IF NOT EXISTS library (
                id TEXT PRIMARY KEY,
                provider TEXT NOT NULL,
                manga_id TEXT NOT NULL,
                title TEXT NOT NULL,
                cover_url TEXT,
                url TEXT,
                type TEXT DEFAULT 'manga',
                added_at INTEGER NOT NULL,
                updated_at INTEGER NOT NULL
            );
            CREATE TABLE IF NOT EXISTS reading_progress (
                id TEXT PRIMARY KEY,
                provider TEXT NOT NULL,
                manga_id TEXT NOT NULL,
                chapter_id TEXT NOT NULL,
                chapter_number REAL,
                page INTEGER DEFAULT 0,
                completed INTEGER DEFAULT 0,
                read_at INTEGER NOT NULL
            );
            CREATE TABLE IF NOT EXISTS downloads (
                id TEXT PRIMARY KEY,
                provider TEXT NOT NULL,
                manga_id TEXT NOT NULL,
                manga_title TEXT NOT NULL,
                chapter_id TEXT NOT NULL,
                chapter_title TEXT,
                chapter_number REAL,
                cbz_path TEXT,
                status TEXT NOT NULL DEFAULT 'queued',
                progress INTEGER DEFAULT 0,
                total_pages INTEGER DEFAULT 0,
                queued_at INTEGER NOT NULL,
                completed_at INTEGER
            );
        """, null, null);
    }

    public void add_to_library (string id, string provider, string manga_id,
                                 string title, string? cover_url, string type) {
        var stmt = prepare ("INSERT OR REPLACE INTO library VALUES (?,?,?,?,?,NULL,?,?,?)");
        stmt.bind_text (1, id);
        stmt.bind_text (2, provider);
        stmt.bind_text (3, manga_id);
        stmt.bind_text (4, title);
        stmt.bind_text (5, cover_url ?? "");
        stmt.bind_text (6, type);
        var now = (int64) (GLib.get_real_time () / 1000);
        stmt.bind_int64 (7, now);
        stmt.bind_int64 (8, now);
        stmt.step ();
    }

    public LibraryItem[] get_library () {
        var result = new GLib.Array<LibraryItem> ();
        var stmt = prepare ("SELECT id,provider,manga_id,title,cover_url,type,added_at FROM library ORDER BY updated_at DESC");
        while (stmt.step () == Sqlite.ROW) {
            var item = LibraryItem () {
                id         = stmt.column_text (0),
                provider   = stmt.column_text (1),
                manga_id   = stmt.column_text (2),
                title      = stmt.column_text (3),
                cover_url  = stmt.column_text (4),
                type       = stmt.column_text (5),
                added_at   = stmt.column_int64 (6),
            };
            result.append_val (item);
        }
        return result.data;
    }

    private Sqlite.Statement prepare (string sql) {
        Sqlite.Statement stmt;
        db.prepare_v2 (sql, -1, out stmt);
        return stmt;
    }
}

public struct LibraryItem {
    public string id;
    public string provider;
    public string manga_id;
    public string title;
    public string? cover_url;
    public string type;
    public int64 added_at;
}