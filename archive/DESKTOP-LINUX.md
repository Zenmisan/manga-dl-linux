# manga-dl Linux — Implementation Guide

> Platform: Linux (GNOME / any desktop)  
> UI: GTK4 + libadwaita  
> Language: Vala (with C bridge for QuickJS)  
> JS Engine: QuickJS (vendored C source — `quickjs.c` + `quickjs.h`)  
> Database: SQLite3 (Vala built-in bindings)  
> HTTP: libsoup 3.0 (GObject async HTTP)  
> Build: Meson + Ninja  
> Package: Flatpak (distribution) / native pkg (dev)  

See `PROJECT.md` (in `manga-dl-windows/`) for the full architecture, shared SQLite schema, and API endpoint reference.

---

## Project Setup

### Dependencies

```bash
# Arch / Manjaro
sudo pacman -S gtk4 libadwaita libsoup vala meson ninja sqlite glib2 gobject-introspection

# Ubuntu / Debian
sudo apt install libgtk-4-dev libadwaita-1-dev libsoup-3.0-dev valac meson ninja-build libsqlite3-dev libglib2.0-dev

# Fedora
sudo dnf install gtk4-devel libadwaita-devel libsoup3-devel vala meson ninja-build sqlite-devel
```

### Vendor QuickJS

```bash
mkdir -p vendor/quickjs
# Download quickjs.c, quickjs.h, quickjs-libc.c, cutils.c, cutils.h, libregexp.c, libregexp.h, libunicode.c, libunicode.h
# from https://bellard.org/quickjs/ or https://github.com/bellard/quickjs
```

### meson.build

```meson
project('manga-dl', ['vala', 'c'],
  version: '1.0.0',
  default_options: ['warning_level=2', 'c_std=c11'])

gnome = import('gnome')
vala = meson.get_compiler('vala')

deps = [
  dependency('gtk4'),
  dependency('libadwaita-1'),
  dependency('libsoup-3.0'),
  dependency('sqlite3'),
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  vala.find_library('posix'),
]

# Vendor QuickJS (two source files, zero external deps)
quickjs_src = [
  'vendor/quickjs/quickjs.c',
  'vendor/quickjs/libregexp.c',
  'vendor/quickjs/libunicode.c',
  'vendor/quickjs/cutils.c',
  'vendor/quickjs/quickjs-libc.c',
]

executable('manga-dl',
  sources: [
    'src/main.vala',
    'src/app.vala',
    'src/window.vala',
    'src/extension_bridge.c',  # C bridge for QuickJS
    'src/views/library_view.vala',
    'src/views/search_view.vala',
    'src/views/browse_view.vala',
    'src/views/detail_view.vala',
    'src/views/reader_view.vala',
    'src/views/novel_reader_view.vala',
    'src/views/updates_view.vala',
    'src/views/history_view.vala',
    'src/views/downloads_view.vala',
    'src/views/sources_view.vala',
    'src/views/settings_view.vala',
    'src/db/database.vala',
    'src/db/models.vala',
    'src/services/extension_manager.vala',
    'src/services/http_service.vala',
    'src/services/download_service.vala',
    'src/services/supabase_sync.vala',
  ] + quickjs_src,
  vala_args: [
    '--pkg', 'libsoup-3.0',
    '--pkg', 'sqlite3',
  ],
  c_args: ['-I' + meson.source_root() / 'vendor/quickjs'],
  dependencies: deps,
  install: true,
)

# Install extension .js files
install_subdir('assets/extensions', install_dir: get_option('datadir') / 'manga-dl')
```

### Copy extensions at build time

```bash
# In project root, after cloning:
mkdir -p assets/extensions
cp ../manga-dl/backend/app/services/extensions/*.js assets/extensions/
```

Or add a Meson custom target:

```meson
copy_extensions = custom_target('copy-extensions',
  input: [],
  output: 'extensions-copied',
  command: [
    'sh', '-c',
    'cp @0@/../manga-dl/backend/app/services/extensions/*.js @1@/assets/extensions/ && touch @OUTPUT@'.format(
      meson.source_root(), meson.source_root())
  ],
  build_by_default: true)
```

---

## Extension Bridge (C + QuickJS)

QuickJS is pure C. The bridge is written in C and exposed to Vala via a header.

### `src/extension_bridge.h`

```c
#ifndef EXTENSION_BRIDGE_H
#define EXTENSION_BRIDGE_H

#include <glib.h>

typedef struct ExtensionBridge ExtensionBridge;

ExtensionBridge* extension_bridge_new     (void);
void             extension_bridge_free    (ExtensionBridge *bridge);
gboolean         extension_bridge_load    (ExtensionBridge *bridge, const char *js_source, GError **error);
char*            extension_bridge_search  (ExtensionBridge *bridge, const char *query, int page, GError **error);
char*            extension_bridge_detail  (ExtensionBridge *bridge, const char *manga_id, GError **error);
char*            extension_bridge_pages   (ExtensionBridge *bridge, const char *chapter_id, GError **error);

#endif
```

### `src/extension_bridge.c`

```c
#include "extension_bridge.h"
#include "quickjs/quickjs.h"
#include "quickjs/quickjs-libc.h"
#include <libsoup/soup.h>
#include <string.h>
#include <stdlib.h>

#define UA "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 " \
           "(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"

struct ExtensionBridge {
    JSRuntime *rt;
    JSContext *ctx;
};

static JSValue js_api_fetch(JSContext *ctx, JSValueConst this_val,
                            int argc, JSValueConst *argv)
{
    const char *url = JS_ToCString(ctx, argv[0]);
    if (!url) return JS_EXCEPTION;

    SoupSession *session = soup_session_new();
    g_object_set(session, "user-agent", UA, NULL);

    SoupMessage *msg = soup_message_new("GET", url);
    JS_FreeCString(ctx, url);

    GError *err = NULL;
    GBytes *bytes = soup_session_send_and_read(session, msg, NULL, &err);

    if (err || !bytes) {
        g_clear_error(&err);
        g_object_unref(msg);
        g_object_unref(session);
        return JS_NewString(ctx, "{\"status\":0,\"text\":\"\"}");
    }

    gsize size;
    const char *body = g_bytes_get_data(bytes, &size);
    int status = soup_message_get_status(msg);

    /* Build { status, text } JSON */
    char *escaped = g_strescape(body, NULL);
    char *result_json = g_strdup_printf("{\"status\":%d,\"text\":\"%s\"}", status, escaped);
    JSValue result = JS_NewString(ctx, result_json);

    g_free(escaped);
    g_free(result_json);
    g_bytes_unref(bytes);
    g_object_unref(msg);
    g_object_unref(session);
    return result;
}

ExtensionBridge* extension_bridge_new(void)
{
    ExtensionBridge *b = g_new0(ExtensionBridge, 1);
    b->rt  = JS_NewRuntime();
    b->ctx = JS_NewContext(b->rt);
    js_std_init_handlers(b->rt);

    /* Register apiFetch */
    JSValue global = JS_GetGlobalObject(b->ctx);
    JS_SetPropertyStr(b->ctx, global, "apiFetch",
        JS_NewCFunction(b->ctx, js_api_fetch, "apiFetch", 1));
    JS_FreeValue(b->ctx, global);
    return b;
}

void extension_bridge_free(ExtensionBridge *b)
{
    if (!b) return;
    JS_FreeContext(b->ctx);
    JS_FreeRuntime(b->rt);
    g_free(b);
}

gboolean extension_bridge_load(ExtensionBridge *b, const char *js_source, GError **error)
{
    JSValue val = JS_Eval(b->ctx, js_source, strlen(js_source),
                          "<extension>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val)) {
        JSValue exc = JS_GetException(b->ctx);
        const char *msg = JS_ToCString(b->ctx, exc);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s", msg);
        JS_FreeCString(b->ctx, msg);
        JS_FreeValue(b->ctx, exc);
        return FALSE;
    }
    JS_FreeValue(b->ctx, val);
    return TRUE;
}

static char* call_extension_fn(ExtensionBridge *b, const char *fn_name,
                                int argc, JSValue *args, GError **error)
{
    JSValue global = JS_GetGlobalObject(b->ctx);
    JSValue ext    = JS_GetPropertyStr(b->ctx, global, "extension");
    JSValue fn     = JS_GetPropertyStr(b->ctx, ext, fn_name);

    JSValue result = JS_Call(b->ctx, fn, ext, argc, args);
    JS_FreeValue(b->ctx, fn);
    JS_FreeValue(b->ctx, ext);
    JS_FreeValue(b->ctx, global);

    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(b->ctx);
        const char *msg = JS_ToCString(b->ctx, exc);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s", msg);
        JS_FreeCString(b->ctx, msg);
        JS_FreeValue(b->ctx, exc);
        return NULL;
    }

    /* Stringify result (may be array or object) */
    JSValue json_str = JS_JSONStringify(b->ctx, result, JS_UNDEFINED, JS_UNDEFINED);
    const char *str = JS_ToCString(b->ctx, json_str);
    char *out = g_strdup(str);
    JS_FreeCString(b->ctx, str);
    JS_FreeValue(b->ctx, json_str);
    JS_FreeValue(b->ctx, result);
    return out;
}

char* extension_bridge_search(ExtensionBridge *b, const char *query, int page, GError **error)
{
    JSValue args[2] = { JS_NewString(b->ctx, query), JS_NewInt32(b->ctx, page) };
    return call_extension_fn(b, "search", 2, args, error);
}

char* extension_bridge_detail(ExtensionBridge *b, const char *manga_id, GError **error)
{
    JSValue args[1] = { JS_NewString(b->ctx, manga_id) };
    return call_extension_fn(b, "getMangaDetail", 1, args, error);
}

char* extension_bridge_pages(ExtensionBridge *b, const char *chapter_id, GError **error)
{
    JSValue args[1] = { JS_NewString(b->ctx, chapter_id) };
    return call_extension_fn(b, "getPages", 1, args, error);
}
```

### Vala wrapper

```vala
// src/services/extension_manager.vala
[CCode (cheader_filename = "extension_bridge.h")]
extern class ExtensionBridge {
    public ExtensionBridge ();
    public void free ();
    public bool load (string js_source) throws GLib.Error;
    public string search (string query, int page) throws GLib.Error;
    public string detail (string manga_id) throws GLib.Error;
    public string pages (string chapter_id) throws GLib.Error;
}

public class ExtensionManager {
    private HashTable<string, ExtensionBridge> _loaded;

    public ExtensionManager () {
        _loaded = new HashTable<string, ExtensionBridge> (str_hash, str_equal);
    }

    public async ExtensionBridge get_bridge (string source_id) throws GLib.Error {
        if (_loaded.contains (source_id))
            return _loaded[source_id];

        var js_path = Path.build_filename (
            Environment.get_user_data_dir (), "manga-dl", "extensions", source_id + ".js");
        string js_source;
        FileUtils.get_contents (js_path, out js_source);

        var bridge = new ExtensionBridge ();
        bridge.load (js_source);
        _loaded[source_id] = bridge;
        return bridge;
    }
}
```

---

## Database (SQLite3 via Vala)

```vala
// src/db/database.vala
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
```

---

## HTTP Client

```vala
// src/services/http_service.vala
public class HttpService {
    private const string UA = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 " +
        "(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36";

    public static async Bytes fetch_bytes_async (string url) throws GLib.Error {
        var session = new Soup.Session ();
        session.user_agent = UA;
        var msg = new Soup.Message ("GET", url);
        var stream = yield session.send_async (msg, GLib.Priority.DEFAULT, null);
        return yield stream.read_bytes_async (int.MAX, GLib.Priority.DEFAULT, null);
    }
}
```

---

## UI Structure (GTK4 + libadwaita)

### Layout

```vala
// src/window.vala
public class MangaDlWindow : Adw.ApplicationWindow {
    private Adw.NavigationSplitView split_view;
    private Gtk.StackSidebar sidebar;
    private Gtk.Stack content_stack;

    construct {
        title = "manga-dl";
        default_width = 1200;
        default_height = 800;

        // Dark theme
        Adw.StyleManager.get_default ().color_scheme = Adw.ColorScheme.FORCE_DARK;

        var nav = new Adw.NavigationView ();
        // Add pages: library, search, updates, history, downloads, sources, settings
        content = nav;
    }
}
```

Use `Adw.NavigationView` for page stack, `Adw.PreferencesPage` for settings, `Gtk.GridView` for manga grid, `Gtk.ScrolledWindow` + `Gtk.Box` for the webtoon reader.

### Background `#050505`

```css
/* src/style.css */
window {
    background-color: #050505;
}
.manga-grid {
    background-color: #050505;
}
```

Load via `Gtk.CssProvider` in `app.vala`.

---

## CBZ Download

```vala
// src/services/download_service.vala
public class DownloadService {
    public async void download_chapter (string source_id, string manga_title,
                                         string chapter_id, string chapter_title) throws GLib.Error {
        var bridge = yield extension_manager.get_bridge (source_id);
        var pages_json = bridge.pages (chapter_id);
        var parser = new Json.Parser ();
        parser.load_from_data (pages_json);
        var urls = parser.get_root ().get_array ();

        var cbz_dir = Path.build_filename (
            Environment.get_home_dir (), "Documents", "manga-dl", sanitize (manga_title));
        DirUtils.create_with_parents (cbz_dir, 0755);
        var cbz_path = Path.build_filename (cbz_dir, sanitize (chapter_title) + ".cbz");

        // Write CBZ (ZIP with stored pages)
        // Use libarchive or write ZIP manually with GLib.MemoryOutputStream
        // ComicInfo.xml included for Kavita/Komga compatibility
        // ...
    }

    private string sanitize (string name) {
        return name.replace ("/", "-").replace ("\\", "-").replace (":", "-");
    }
}
```

For ZIP writing, link against `libarchive` or use a pure-Vala ZIP writer. Add `dependency('libarchive')` to `meson.build`.

---

## Flatpak (Distribution)

`org.mangadl.App.yml`:

```yaml
app-id: org.mangadl.App
runtime: org.gnome.Platform
runtime-version: '46'
sdk: org.gnome.Sdk
command: manga-dl
finish-args:
  - --share=network
  - --socket=wayland
  - --socket=fallback-x11
  - --filesystem=home
  - --device=dri

modules:
  - name: manga-dl
    buildsystem: meson
    sources:
      - type: dir
        path: .
```

```bash
flatpak-builder --force-clean build-dir org.mangadl.App.yml
flatpak-builder --run build-dir org.mangadl.App.yml manga-dl
```

---

## Build & Run

```bash
meson setup build
cd build && ninja
./manga-dl

# Install to system
sudo ninja install
```

---

## Platform Checklist

- [ ] Extension JS evaluates (search returns results)
- [ ] Chapter pages load directly (no proxy)
- [ ] Paginated manga reader (LTR + RTL)
- [ ] Webtoon scroll reader
- [ ] Library persists across restarts
- [ ] Reading progress saves and restores
- [ ] Chapter download to CBZ
- [ ] Downloaded CBZ opens in third-party reader (Kavita, Foliate)
- [ ] Dark mode (force dark, #050505 background)
- [ ] Offline: saved library loads without network
- [ ] Flatpak builds cleanly
