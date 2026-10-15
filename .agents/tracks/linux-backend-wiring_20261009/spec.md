# Spec: Linux backend wiring (Qt6/QML UI → real C++20 backend)

Created: 2026-10-09 · Project root: `/home/zenmi/Projects/manga-dl-linux` · App source: `manga-dl-linux/`

## 1. Problem

`manga-dl-linux/` contains a finished, compiled UI (28 QML screens, verified: `cmake --build` succeeds,
Qt 6.11.2 on this machine, project declares Qt ≥ 6.4). It is **screens only**:

- Every list comes from the `Sample` singleton (`src/SampleData.cpp`).
- 24 `TODO(backend)` stubs (found with `grep -rn "TODO(backend)" manga-dl-linux`), many more fake
  actions that only call `Nav.toast(...)`.
- Template/placeholder text such as `[Display name]`, `[title]`, `[n]`, `placeholder-key`,
  `https://[your-server]:8000`, hardcoded `"crown"` search text, hardcoded `Ch. 48`, `96 chapters`.
- Covers and reader pages are colour rectangles (`CoverArt.qml`, `PagePlaceholder.qml`).

The old `src/*.vala` + root `meson.build` + `build/` + `DESKTOP-LINUX.md` are an abandoned GTK4/Vala
attempt (empty stubs). They are **not** the product and are not touched until the Phase 9 cleanup decision.

Goal: production-grade, fully wired app with **no dead buttons, no placeholder values, no mock data**,
at feature parity with `manga-dl-windows` (the reference implementation, 72/72 tests passing).

## 2. Reference sources (read, do not modify)

| What | Where |
|---|---|
| Windows core (C#) — port target | `../manga-dl-windows/src/MangaDl.Core/**` |
| Windows UI wiring (behaviour reference) | `../manga-dl-windows/src/MangaDl.App/Pages/*.xaml.cs` |
| Windows feature history | `../manga-dl-windows/.agents/tracks.md` + `tracks/*/plan.md` |
| Extension scripts (40 files, shared unchanged) | `../manga-dl/backend/app/services/extensions/*.js` |
| Web (Supabase table shapes) | `../manga-dl/frontend/src/lib/supabaseSubscriptions.ts` |
| Architecture rules | `PROJECT.md`, `DESKTOP-LINUX.md` (note: its Vala/GTK stack is superseded by this Qt stack) |

## 3. Verified technical findings (measured on this machine, 2026-10-09)

1. **Extensions need only `apiFetch` + `DOMParser`.** 40 files, all `async/await`. 36 use `DOMParser`.
   DOM surface used: `querySelector` (321), `getAttribute` (304), `textContent` (230),
   `querySelectorAll` (159), `remove` (26), `innerHTML` (24), plus rare `setAttribute`, `closest`,
   `parentElement`, `outerHTML`, `nextElementSibling`, `classList`, `childNodes`.
2. Other globals: `console`, `URL`, `btoa/atob`, `encodeURIComponent`, `RegExp`. `comixto.js` uses
   `document.createElement('canvas')` to descramble images → needs a native (QImage) implementation.
   `asurascans.js` also needs tile reassembly (web has `/manga/asura-descramble`).
3. **Qt `QJSEngine` is rejected**: tested — `async function` ⇒ `SyntaxError` even on Qt 6.11.2.
4. **quickjs-ng v0.17.0 is accepted**: MIT licence, vendored C sources compile with GCC 16 on this box,
   tested async/await + Promise jobs + `JS_ExecutePendingJob` ⇒ `{"v":42,"done":true}`.
   Needed files: `quickjs.c dtoa.c libregexp.c libunicode.c` (+ headers). **No `cutils.c`** in v0.17.
   Build define: `-DQJS_BUILD_LIBC`.
5. Installed libs: libxml2 2.15.4, libzip 1.11.4, minizip 1.3.2, zlib 1.3.2, libarchive 3.8.9,
   libsoup-3.0, sqlite3, Qt6 (Core Gui Qml Quick QuickControls2 Svg Network Sql Concurrent
   QuickDialogs2 NetworkAuth Multimedia). **No lexbor / gumbo.**
6. Network access to github works (needed to vendor sources).
7. Sandbox note: some paths (`/home/zenmi/Projects` listing, direct `view_file` on sibling projects) are
   permission-gated for file tools; use shell (`cat/head/grep`) to read sibling projects.

## 4. Architecture decisions

**Stack:** C++20 + Qt 6 (Qt Quick/QML UI already built). One QObject "controller/model" per feature,
registered with `QML_ELEMENT`/`QML_SINGLETON`; QML only sees `Q_PROPERTY`/`Q_INVOKABLE`/signals/models.

| Concern | Decision | Why |
|---|---|---|
| JS engine | **quickjs-ng v0.17.0**, vendored in `manga-dl-linux/vendor/quickjs/` | only option verified for async/await; matches spec ("QuickJS C") |
| Threading | One dedicated **JS worker thread** owning the JSRuntime. All calls queued; `apiFetch` returns a real JS Promise resolved from the worker when the network reply arrives (no blocking, no deadlock) | JS is single-threaded; keeps UI responsive |
| HTTP | `QNetworkAccessManager` on the worker side, default browser UA (`Mozilla/5.0 (X11; Linux x86_64) … Chrome/124`), cookie jar, redirects, timeouts, same `/manga/proxy/*?url=` URL resolution as Windows `HttpService.ResolveUrl` | parity with Windows; no CORS on native |
| DOM for `DOMParser` | **To be decided by Phase 1 prototype** (see §6 Risk R1): preferred = vendor **lexbor** (HTML5 parser + CSS selector engine, MIT/Apache); fallback = libxml2 HTML parser + own CSS→XPath translator | no CSS selector engine installed |
| Database | `QtSql`/`QSQLITE` (or raw sqlite3) at `$XDG_DATA_HOME/manga-dl/manga.db`, WAL, versioned migrations. Same table/column names as Windows (`library`, `reading_progress`, `downloads`, `categories`, `library_categories`, `history`, `tracker_binds`, `new_chapters`) | Supabase sync and backups interoperate across platforms |
| ZIP/CBZ/EPUB | **libzip** (read in-memory pages from CBZ, write CBZ with STORE + `ComicInfo.xml`, EPUB with uncompressed first `mimetype` entry) | already installed; correct EPUB requirement |
| Images | `image://` providers: remote covers/pages with UA + Referer, on-disk cache, CBZ-entry provider, descramble hooks (comixto/asura) via `QImage` | QML `Image` needs headers/referer; no proxy |
| Settings | `QSettings` wrapped in an `AppSettings` QObject, every QML control bound two-way | persistence of all toggles |
| Secrets | Tokens in **libsecret** (Secret Service); fall back to a 0600 file with a visible warning if no keyring | spec: never plaintext by default |
| Auth/Sync | Supabase REST (`/auth/v1`, `/rest/v1`), tables `subscriptions`, `read_tracking`, `reading_progress`, `user_categories`, `profiles`; last-write-wins on `updated_at`; local-first, background, never on the critical path | matches web + Windows exactly |
| Trackers | AniList (GraphQL) + MyAnimeList (REST v2) OAuth PKCE via loopback `http://localhost:5678/...` `QTcpServer`; Kitsu/MangaUpdates/Shikimori/Bangumi shown by the UI **must be either implemented or removed** — decision in Phase 6 | no dead "Connect" buttons |
| Notifications | `org.freedesktop.Notifications` via D-Bus (QtDBus) + tray via `QSystemTrayIcon` if available | "Desktop notifications", "Keep running in tray" |
| Packaging | CMake install + `.desktop`; Flatpak manifest as a later deliverable | |

**Hard rules**
- Never use the mock `Sample` singleton in shipped code; delete `SampleData.*` at the end (Phase 8).
- No `[bracket]` text, no fake toasts, no hardcoded lists in QML. Empty/loading/error states instead.
- Every `catch`/error path is logged and user-visible where the user acted; no silent failures.
- No secrets committed. Supabase anon key may be read from env/config like Windows; never hardcode OAuth client secrets.
- Do not modify `../manga-dl*` projects. No git commits/pushes unless the user asks.
- Keep Qt API usage compatible with Qt 6.4 where practical (the project advertises 6.4+); anything newer must be guarded.

## 5. Screen → backend map (what "wired" means)

| QML page | Real data / actions required |
|---|---|
| Onboarding / Login / Register / ForgotPassword | Supabase auth, backend URL ping, skip-offline, stay-signed-in, reset email |
| Library | library rows, covers, unread badges, categories (chips + sheet/context menu), continue-reading, sort, search, Update Library |
| MangaDetail | `getMangaDetail`, chapter list + read/downloaded state, Resume, Add/Remove library, Track, Download, Migrate, categories |
| Reader / NovelReader | `getPages` / `getChapterText`, single/spread/webtoon/vertical, RTL/LTR, progress save, prev/next chapter, bookmarks, filters, EPUB/html local chapters |
| Search / BrowseSource / Extensions(Sources, Extensions, Migrate) | parallel multi-source search, popular/latest/genres paging, extension list/install/update/uninstall, migration flow, pin source |
| Updates / History / Notifications | `new_chapters`, history rows, clear/remove, mark read, notification inbox + desktop notifications |
| Downloads | queue (pause/resume/cancel/retry/clear), progress, save-to folder picker, storage stats |
| Import | file/folder dialogs + drag & drop, CBZ/ZIP/EPUB/image folders, add-to-existing-series, ComicInfo.xml |
| Stats / Profile | Hunter rank, EXP formula, 21 badges, 52-week heatmap, streak, goals, by-category/by-source |
| Settings (Account, General, Reader, Library, Trackers, System) | every control persisted + effective; backup/restore (`.mangadl`, `.tachibk`), cache clear, folder picker, sync now, category CRUD, tracker OAuth |
| Help | `mailto:` to the same support address as Windows/web (no ticket API exists) |
| UpdateDialog / AllPages(debug) | real version check or removal of the fake "update ready" copy |

Not in scope (as agreed in the UI README): DownloadHub, Landing, Terms.

## 6. Risks & open decisions (resolve inside the phase named)

- **R1 (Phase 1) DOM engine.** Need CSS selectors for 36 extensions. Prototype lexbor vs libxml2+translator
  against *every* selector string in `extensions/*.js`; pick the one that passes all. Gate: no extension may regress.
- **R2 (Phase 1) Sync `apiFetch` vs Promise.** Must be a true Promise resolved asynchronously on the JS worker
  (the Windows version blocks; do not copy that). Gate: 6 concurrent searches don't deadlock or freeze UI.
- **R3 (Phase 1) Extension contract drift.** Some extensions may rely on web-proxy-only endpoints
  (`/manga/image-proxy`, `/manga/descramble-proxy`, `/manga/asura-descramble`, `/sources/comixto/*`).
  Audit each of the 40 and record pass/fail in plan.md; Windows already audited them (reference: "40 extensions audited and passing contracts").
- **R4 (Phase 6) Trackers beyond AniList/MAL.** UI lists 6. Implement only what Windows implements, **remove the others from the UI** unless the user wants more.
- **R5 (Phase 7) Update dialog.** No update channel exists for Linux. Replace with a real GitHub-releases check (needs repo URL from user) or remove.
- **R6 Live-network tests are flaky** (sources change). Verification uses recorded HTML/JSON fixtures for CI-style tests plus a manual live smoke checklist.
- **R7 Windows reference may itself have gaps** (e.g. sync `apiFetch`, tracker secrets). Where Windows is wrong, do it correctly here and note it.

## 7. Definition of done

1. `grep -rn "TODO(backend)\|Sample\.\|\[[A-Za-z ]*\]" manga-dl-linux/qml manga-dl-linux/src` → no placeholder/mock hits (reviewed manually for legit brackets).
2. Every button/switch/field on all 28 pages is traced in `plan.md` Appendix A to a real handler or an intentional removal.
3. Automated tests pass (DB, JS bridge, DOM, CBZ/EPUB, gamification, sync serialisation, backup round-trip, update diff, settings).
4. Live smoke checklist (Appendix B) passes on this machine: search → detail → read → progress restore → download CBZ → open CBZ in a third-party reader → offline library.
5. Clean Release build, no new compiler warnings in our code, no leaked secrets, app starts from a clean profile and from an existing one.
