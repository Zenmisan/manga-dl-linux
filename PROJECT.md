# manga-dl — Full Project Reference

> Web source: `/home/zenmi/Projects/manga-dl`  
> Stack: React 19 + Vite + TypeScript · FastAPI + Python · SQLite · Supabase · Firebase  
> Platforms: Web (Firebase) · Android (`manga-dl-android`, native Kotlin/Jetpack Compose) · Windows (`manga-dl-windows`, native WinUI 3/C#) · Linux (`manga-dl-linux`, native GTK4/Vala)

---

## Architecture

### Web app (requires backend)

```
Browser
  │
  ├── Extension Worker (Web Worker)
  │       └── runs source .js files, calls /manga/proxy/* for CORS bypass
  ├── Supabase client (auth, reading progress, profiles)
  └── Backend API (FastAPI on Render.com)
          ├── /api/manga/*     proxy + subscriptions
          ├── /api/sources/*   extension registry
          ├── /api/library/*   CBZ management
          ├── /api/downloads/* download queue
          ├── /api/users/*     reading progress, devices
          ├── /api/auth/*      MAL/AniList OAuth relay
          └── /api/backup/*    Tachiyomi import, export
```

### Native apps (self-contained, no backend)

```
Native UI (Jetpack Compose / WinUI 3 / GTK4+libadwaita)
  │
  ├── Embedded JS Engine (QuickJS-Android / ChakraCore / QuickJS C)
  │       └── evaluates same .js extension files from backend/app/services/extensions/
  │           apiFetch() → native HTTP client (no CORS, no proxy needed)
  ├── Local SQLite (Room / sqlite-net-pcl / SQLite3)
  └── Optional Supabase sync (background only, never on critical path)
```

Native apps reuse the extension `.js` files unchanged. The only adaptation is the JS engine host and the `apiFetch` bridge it exposes. Everything else — UI, database, HTTP — is platform-native.

---

## Extension System

### How it works

The same `.js` files power all platforms. Each extension implements:

```javascript
var extension = {
  async search(query, page) { /* returns [{id, title, cover_url, provider, url}] */ },
  async getMangaDetail(mangaId) { /* returns {id, title, description, chapters, ...} */ },
  async getPages(chapterId) { /* returns [imageUrl, ...] */ },
  // novels only:
  async getChapterText(chapterId) { /* returns {content, format: 'html'|'plain'} */ },
}
```

Extensions may only use: `apiFetch`, `JSON`, `btoa`/`atob`, `encodeURIComponent`, `RegExp`. No DOM, no Node.js globals — these already work on all four JS engines.

### `apiFetch` bridge contract

```javascript
// Must be exposed by every platform host:
apiFetch(url, options?) → Promise<{ status, text, json }>
// options: { method?, headers?, body? }
```

Web uses `/manga/proxy/html?url=` to relay through the backend. Native uses OkHttp / URLSession / HttpClient / libsoup directly — no proxy needed.

### Built-in sources (50 total)

**Manga:**
MangaDex, Asura Scans, MangaKatana, Omega Scans, Bato.to, MangaKakalot, MangaPill, TCB Scans, MangaHere, Webtoons, Azuki, Comix.to, FlameComics, MangaPlus  
+ ~16 Madara/WordPress template sites (ManhuaHot, ManhwaTop, LHTranslation, ZinManga, etc.)

**Web novels:**
Royal Road, Scribble Hub, LightNovelWorld, WuxiaWorld, NovelBin, NovelFire, NovelFull, FreeWebNovel, AllNovel, NovelPhoenix, ReadNovelFull, Libread, BrightNovel, ChrysanthemumGarden, ComradeMao, and more

**Community:** Keiyoushi index (400+ sources) — web only via `/api/sources/market`

---

## Backend

### Entry point
`backend/app/main.py` — FastAPI. Lifespan: init SQLite, start sync task, pre-warm Comixto token, validate providers.

### Auth
- **API key** (`X-API-Key` or `?api_key=`) — protects most endpoints
- **Supabase JWT** — protects user-scoped endpoints

### Database
SQLite via SQLAlchemy async. Key models: `MangaRecord`, `DownloadRecord`, `ReadingProgress`, `UserDevice`, `UserProfile`, `Comment`.

---

## API Endpoints

### `/api/manga/*`

| Method | Path | Description |
|---|---|---|
| GET/POST | `/manga/proxy/html?url=` | CORS proxy — raw HTML |
| GET | `/manga/proxy/json?url=` | CORS proxy — JSON |
| GET | `/manga/image-proxy?url=` | Image proxy |
| GET | `/manga/descramble-proxy` | Comix.to descramble |
| GET | `/manga/asura-descramble` | Asura tile reassembly |
| GET | `/manga/updates` | New chapters from subscriptions |
| GET | `/manga/subscriptions` | List user subscriptions |
| POST | `/manga/subscriptions` | Subscribe to manga |
| DELETE | `/manga/subscriptions/{provider}/{manga_id}` | Unsubscribe |
| POST | `/manga/subscribe/{provider}/{manga_id}` | Toggle subscription |
| POST | `/manga/migrate` | Migrate manga between sources |

### `/api/sources/*`

| Method | Path | Description |
|---|---|---|
| GET | `/sources/builtins` | All built-in extension metadata |
| GET | `/sources/market` | Built-ins + Keiyoushi index (24h cache) |
| GET | `/sources/code/{pkg_id}` | JS source code for an extension |
| POST | `/sources/configure/komga` | Set Komga server URL/credentials |
| POST | `/sources/configure/suwayomi` | Set Suwayomi server URL |
| POST | `/sources/comixto/token` | Relay Comixto `_=` token |
| POST | `/sources/comixto/cache` | Relay Comixto cached response |

### `/api/downloads/*`

| Method | Path | Description |
|---|---|---|
| POST | `/downloads/queue` | Queue download (disabled on Render) |
| POST | `/downloads/pause` / `/resume` | Pause/resume queue |
| GET | `/downloads/active` | Active downloads |
| GET | `/downloads/history` | Download history |
| WS | `/downloads/ws` | Real-time progress WebSocket |

### `/api/library/*`

| Method | Path | Description |
|---|---|---|
| GET | `/library` | List user library |
| GET | `/library/read/{title}/{filename}` | CBZ manifest |
| GET | `/library/read/{title}/{filename}/page/{n}` | Serve one page |
| POST | `/library/upload` | Upload CBZ |
| GET | `/library/read/{title}/{filename}/export/pdf` | Stream as PDF |
| GET | `/library/read/{title}/{filename}/export/epub` | Stream as EPUB |

### `/api/users/*`

| Method | Path | Description |
|---|---|---|
| POST | `/users/progress` | Upsert reading progress |
| GET | `/users/progress/{provider}/{manga_id}` | Get progress |
| GET | `/users/history` | Reading history |
| GET | `/users/stats` | Reading stats |
| PUT | `/users/profile` | Update profile (bio, avatar, username) |
| POST | `/users/device/register` | Register device session |
| GET | `/users/leaderboard` | Top readers |

### `/api/auth/*`

| Method | Path | Description |
|---|---|---|
| POST | `/auth/mal/token` | MAL OAuth code exchange |
| POST | `/auth/mal/track` | Update MAL tracking |
| POST | `/auth/anilist/token` | AniList OAuth code exchange |
| POST | `/auth/anilist/track` | Update AniList tracking |

### `/api/backup/*`

| Method | Path | Description |
|---|---|---|
| POST | `/backup/import/tachibk` | Import Tachiyomi `.tachibk` |
| GET | `/backup/export/manual` | Export JSON backup |
| POST | `/backup/import/manual` | Import JSON backup |

---

## Local SQLite Schema

Same table names on all platforms (enables Supabase sync without mapping):

```sql
-- Library
CREATE TABLE library (
  id TEXT PRIMARY KEY,           -- "{provider}/{manga_id}"
  provider TEXT NOT NULL,
  manga_id TEXT NOT NULL,
  title TEXT NOT NULL,
  cover_url TEXT,
  url TEXT,
  type TEXT DEFAULT 'manga',     -- 'manga' | 'novel'
  added_at INTEGER NOT NULL,     -- Unix ms
  updated_at INTEGER NOT NULL
);

-- Reading progress
CREATE TABLE reading_progress (
  id TEXT PRIMARY KEY,           -- "{provider}/{manga_id}/{chapter_id}"
  provider TEXT NOT NULL,
  manga_id TEXT NOT NULL,
  chapter_id TEXT NOT NULL,
  chapter_number REAL,
  page INTEGER DEFAULT 0,
  completed INTEGER DEFAULT 0,   -- 0 | 1
  read_at INTEGER NOT NULL       -- Unix ms
);

-- Downloads
CREATE TABLE downloads (
  id TEXT PRIMARY KEY,
  provider TEXT NOT NULL,
  manga_id TEXT NOT NULL,
  manga_title TEXT NOT NULL,
  chapter_id TEXT NOT NULL,
  chapter_title TEXT,
  chapter_number REAL,
  cbz_path TEXT,
  status TEXT NOT NULL,          -- 'queued' | 'downloading' | 'completed' | 'failed'
  progress INTEGER DEFAULT 0,
  total_pages INTEGER DEFAULT 0,
  queued_at INTEGER NOT NULL,
  completed_at INTEGER
);

-- Categories
CREATE TABLE categories (
  id TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  sort_order INTEGER DEFAULT 0
);

CREATE TABLE library_categories (
  library_id TEXT REFERENCES library(id),
  category_id TEXT REFERENCES categories(id),
  PRIMARY KEY (library_id, category_id)
);
```

---

## Supabase Sync (Optional)

- Write to local SQLite first, then sync to Supabase in background
- On launch: pull diff from Supabase, merge with local (last-write-wins on `updated_at`)
- Downloads never sync — CBZ files stay local
- Never require internet for core features; queue sync when offline

---

## Frontend (Web)

### Pages

| Route | Description |
|---|---|
| `/` | Landing |
| `/dashboard` | Library grid |
| `/search` | Multi-source search |
| `/browse` | Browse one source |
| `/manga/:provider/:id` | Manga detail |
| `/read/:provider/:mangaId/:chapterId` | Manga reader |
| `/read/novel/:provider/:novelId/:chapterId` | Novel reader |
| `/updates` | New chapters |
| `/history` | Reading history |
| `/downloads` | Download manager |
| `/sources` | Source manager |
| `/settings/*` | Settings hub |
| `/stats` | Reading statistics |

### Reader modes
Webtoon (vertical scroll) · LTR pager · RTL pager · Vertical pager

### Trackers
AniList (client 51213 local / 50135 prod) · MyAnimeList — OAuth PKCE, relay via `/api/auth/*`

### Config (`.env`)
```
VITE_API_BASE_URL
VITE_API_KEY
VITE_SUPABASE_URL
VITE_SUPABASE_ANON_KEY
VITE_ANILIST_CLIENT_ID
VITE_MAL_CLIENT_ID
```

### Dev commands
```bash
cd frontend && bun run dev      # http://localhost:5173
cd frontend && bun run build    # → dist/
```

---

## Android App (`manga-dl-android`)

**Stack:** Kotlin · Jetpack Compose · Material3 · QuickJS-Android · Room · OkHttp · Coil · Supabase-kt · Firebase Google Sign-In

Extension bridge: `QuickJS.createRuntime().createContext()`, `_nativeFetch` registered as `JSCallFunction`, calls `OkHttpClient` with spoofed User-Agent.

See the Android project at `/home/zenmi/Projects/manga-dl-android` for full source.

---

## Windows App (`manga-dl-windows`)

See `DESKTOP-WINDOWS.md` in this directory.

## Linux App (`manga-dl-linux`)

See `/home/zenmi/Projects/manga-dl-linux/DESKTOP-LINUX.md`.
