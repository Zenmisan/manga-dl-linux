# Cross-Platform Feature, Screen & Bug Audit (`manga-dl` Ecosystem)

> **Scope**: Full audit comparing **Web Frontend & Backend** (`manga-dl`), **Linux** (`manga-dl-linux`), **Windows** (`manga-dl-windows`), and **Android** (`manga-dl-android`) so that **zero screens, modals, or nook-and-cranny features are missed** before finalizing UI designs in Stitch (`manga-dl UI Design System`, project `14212888750001196315`).

---

## Part 1: Missing Screens & Modals (What Stitch Needs Before Final Design)

The Web Frontend (`manga-dl/frontend/src`) and Android (`manga-dl-android`) have several full screens, sub-modes, and modals/drawers that **do not exist yet as dedicated screens or modals** in the Linux/Windows desktop UI (or are missing on one of the platforms). We must include these in our Stitch screen inventory so we never have to backtrack:

| # | Screen / Modal / View | In Web? | In Android? | In Linux? | In Windows? | What Needs to Be Designed in Stitch & Wired |
|---|-----------------------|:-------:|:-----------:|:---------:|:-----------:|---------------------------------------------|
| **1** | **Reader Hall of Fame / Hunter Leaderboard** (`Search.tsx` "Readers" tab on Web; `LeaderboardScreen.kt` on Android) | ✅ | ✅ | ❌ | ❌ | Top-3 Podium showcase (1st/2nd/3rd elevated cards) + Ranked list (1–50), Period filter pills (`All Time`, `This Month`, `This Week`), Hunter Rank badge, EXP/stats, and "Search Readers" input (`GET /api/users/leaderboard`, `/api/users/search`). |
| **2** | **Manga & Chapter Comments System** (`CommentSection.tsx`, `CommentSheet.tsx`, `Settings/Profile.tsx` "Your Recent Comments") | ✅ | ✅ | ❌ | ❌ | 1) Sidebar/bottom preview on `MangaDetailPage` (`GET/POST /api/comments`).<br>2) Slide-over / drawer `CommentSheet` inside both `ReaderPage` and `NovelReaderPage` (with nested replies, likes ❤️, delete, `@username`, time-ago).<br>3) "Your Recent Comments" history list in Account/Profile settings (`GET /api/comments/mine`). |
| **3** | **Per-Manga Tracker Search & Sync Modals** (`MangaTrackerLinks.tsx` + `MangaModals.tsx` on Web; `TrackingSheet.kt` on Android) | ✅ | ✅ | ⚠️ Basic | ⚠️ Basic | Two distinct modals on `MangaDetailPage`:<br>1) **Tracker Search Modal**: Search AniList/MAL/Kitsu/MangaUpdates/Shikimori/Bangumi by title and bind remote entry.<br>2) **Tracker Sync Modal**: Set Status (`CURRENT`, `COMPLETED`, `PAUSED`, `DROPPED`, `PLANNING`), Score (`0–10`), Chapters Read stepper, Start Date, and Finish Date. |
| **4** | **Manual Series Metadata Edit Modal** (`MangaModals.tsx` on Web) | ✅ | ❌ | ❌ | ❌ | Modal triggered from edit icon next to Manga Title on `MangaDetailPage`: custom Title, Cover Image URL, and Synopsis override (`manga-dl-meta-overrides`). |
| **5** | **Personal Rating (5-Star) & Notes Sheet/Card** (`MangaRatingNotes.tsx` on Web) | ✅ | ❌ | ❌ | ❌ | Interactive 1–5 star personal rating + personal text note editor on `MangaDetailPage`, synced to Supabase `manga_notes` table (`user_id, provider, manga_id, note, rating`). |
| **6** | **Local Archive Detail Screen** (`LocalMangaDetail.tsx` on Web; `LocalFileDetailScreen.kt` on Android) | ✅ | ✅ | ⚠️ Merged | ⚠️ Merged | Dedicated view (or adaptive mode of `MangaDetailPage`) for multi-chapter local CBZ/ZIP/EPUB archives showing sibling batch detection (`Ch. 1–10`, `Ch. 11–20`), archive filename badge, and local read state. |
| **7** | **Offline Archive Import Guide Screen** (`ImportGuide.tsx` on Web) | ✅ | ❌ | ⚠️ External URL | ⚠️ External URL | In-app 8-step visual guide ("How to Import & Read Offline Manga" for MangaKatana batches of 10, OmegaScans, CBZ/ZIP/EPUB format cards). Currently `ImportPage` "Read the import guide" button on Linux/Windows just opens a GitHub URL or toast instead of an in-app screen. |
| **8** | **Interactive App Tour / Spotlight Onboarding** (`AppTour.tsx` on Web) | ✅ | ❌ | ❌ | ❌ | Scrim with pulsing spotlight cutout + step card explaining Sidebar, Search/Discovery, Library Categories, and Reader Tap Zones. |
| **9** | **Reader Keyboard Shortcuts Overlay (`?` key)** (`ShortcutOverlay.tsx` & `NovelReader.tsx` on Web) | ✅ | N/A | ❌ | ⚠️ Toast only | Modal overlay triggered by pressing `?` or clicking the keyboard icon in `ReaderPage` and `NovelReaderPage`, listing all categorized keyboard shortcuts. |
| **10** | **All 31 Milestones Gallery Modal & Public Profile Search Modal** (`Profile.tsx` on Web) | ✅ | ⚠️ Partial | ⚠️ Partial | ⚠️ Partial | 1) Full modal grid of all unlocked/locked milestones filterable by Category (`All`, `Chapters`, `Library`, `Streak`) with progress bars.<br>2) "Search Readers" modal on `ProfilePage` to view any other user's public profile (`/api/users/profile/{id}`), plus "This is how other readers see you" preview banner & "Pin up to 4 Badges" selector in Edit Profile modal. |
| **11** | **App Lock / PIN / Biometric Gate** (`useAppLock` on Web; `BiometricLockGate.kt` on Android) | ✅ | ✅ | ❌ | ❌ | Lock screen overlay requiring PIN / OS Polkit / Keychain unlock when `appLockEnabled` is active. |
| **12** | **Terms of Service Modal / Page** (`Terms.tsx` on Web) | ✅ | ❌ | ⚠️ External link | ⚠️ Toast only | In-app Terms & Privacy view when clicking "Terms" on `RegisterPage` (Windows currently just shows a 1-line toast). |

---

## Part 2: Page-by-Page "Nook & Cranny" Features from the Web Frontend

Below is every subtle interaction, control, and data feature present in `/home/zenmi/Projects/manga-dl/frontend/src` that Linux, Windows, or Android are missing or only partially implement:

### 1. Library / Dashboard (`Dashboard.tsx`, `DashboardHeader.tsx`, `DashboardMangaCard.tsx`, `DashboardSortFilterPanel.tsx`, `DashboardBulkActionBar.tsx`)
- **View Mode & Density Switcher**:
  - Web has **Grid vs. List view** toggle AND **Large vs. Compact density** toggle (`140px` vs `100px` min column width), plus configurable Grid Columns (`2–6`).
  - Linux & Windows currently only have a single static grid layout on `LibraryPage`.
- **Content Type Filter Pills**:
  - Web has `All` | `Manga` | `Novels` filter pills right inside the Library header bar.
- **8 Filter Modes & 6 Sort Modes**:
  - Web Filters: `All`, `Subscribed`, `Downloading`, `Failed`, `Unread`, `Started`, `Completed`, `Downloaded-Only`.
  - Web Sorts: `Default`, `Title (A-Z)`, `Title (Z-A)`, `Downloaded Count`, `Last Read`, `Unread Count`.
  - Linux/Windows only sort by `Last read`, `Title`, `Date added`, `Unread` and lack the 8 filter pills.
- **Multi-Select Batch Mode & Floating Bulk Action Bar**:
  - Web has a `Select` mode toggle in the library header → adds checkboxes to every card → shows floating `DashboardBulkActionBar` (`X Selected`, `Select All / Deselect All`, `Move to Category`, `Download All`, `Delete`).
  - Linux & Windows only have single-card right-click context menus; they lack multi-select batch operations.
- **Card Badges & Indicators**:
  - Web cards show: **Pin badge** (prevents storage eviction), **Unread count pill** (top-left), **Downloaded count badge** (bottom-left), **Local hard-drive badge**, **Sub / Novel / Cloud tags**, and a quick **Continue Ch. X** hover button.

### 2. Search & Discovery (`Search.tsx`, `DiscoverySwimlane.tsx`, `SourceSwimlane.tsx`, `SourceToggleModal.tsx`, `Browse.tsx`)
- **3-Way Top Media Switcher**:
  - Web `Search.tsx` switches between: **Manga** | **Web Novels** | **Readers (Hall of Fame Leaderboard)**.
  - Linux & Windows `SearchPage` does not have the `Manga / Web Novels / Readers` mode switcher.
- **Idle-State Discovery Feed**:
  - When no search query is entered, Web `Search.tsx` displays horizontal **Discovery Swimlanes** (Popular & Latest per enabled source from `/api/sources/discovery` or local extensions) with left/right scroll buttons and "Browse →" links.
  - Linux `SearchPage` shows an empty prompt ("Type a title above and press Enter") when idle unless a query is typed.
- **Interactive Source Toggle Modal (`SourceToggleModal.tsx`)**:
  - Lets users toggle which specific sources are included in multi-source search (`Enable All`, individual switches, persisted in `sourceManager.ts`) without permanently uninstalling extensions.
- **Search Result View Switcher & Relevance Scorer**:
  - Web allows switching search results between **Per-Source Horizontal Swimlanes** vs **Unified Relevance-Sorted Grid ("Top Matches")** using `relevanceScorer.ts`.
- **Quick "Add to Library / In Library" Button on Search Cards**:
  - Web search cards let you subscribe/add to library directly from the search grid with one click.

### 3. Manga Detail Page (`MangaDetail.tsx`, `MangaChaptersSection.tsx`, `MangaHeroHeader.tsx`, `MangaModals.tsx`)
- **Dynamic Theme Color Extraction & Ambient Hero Backdrop**:
  - Extracts dominant cover color to tint the hero glow and cover drop-shadow (`--theme-color`), plus blurred cover backdrop banner.
- **Contextual Smart Resume Button**:
  - Dynamically changes label between `"Start Reading"` (0 read), `"Continue Ch. X (p. Y)"` (in-progress), and `"Re-read Ch. X"` (all read).
- **Per-Manga Notification Mute Toggle**:
  - Bell / BellOff button next to Subscribe on `MangaDetailPage` to mute update notifications for a specific series while keeping it in Library.
- **Rich Chapter Filtering & Sorting**:
  - **Search chapters** input (`Search chapters...`).
  - **5 Chapter Sort modes**: `Default Sort`, `Newest First`, `Oldest First`, `Highest Number`, `Lowest Number`.
  - **Read Filter Pills**: `All` | `Unread` | `Read`.
  - **Scanlator Group Filter Dropdown**: Dynamically populated list of scanlation groups (`All Scanlators` + each group).
  - **Mark All Read** one-click pill button.
- **In-Chapter Read Percentage Progress Bar**:
  - A subtle `2px` bottom progress line on each chapter row showing `0–100%` page completion for partially read chapters.
- **Chapter Row Context Menu (6 Actions) + Bookmark Icon**:
  - Direct **Bookmark star/ribbon toggle** on each chapter row.
  - Right-click (Desktop) / Long-press (Mobile) context menu with:
    1. `Mark as Read` / `Mark as Unread`
    2. `Mark Previous Read` (marks all chapters below/prior to this chapter as read in one click!)
    3. `Download Chapter`
    4. `Bookmark Chapter` / `Remove Bookmark`
    5. `Copy Link`
    6. `Hide Chapter`
  - Linux & Windows `MangaDetailPage` currently clicking a chapter opens it, and clicking the status cell downloads/reads it, but lacks `Mark Previous Read`, `Bookmark Chapter`, `Scanlator Filter`, `Chapter Search`, and `In-Chapter Progress Bar`.

### 4. Manga Reader (`Reader.tsx`, `ReaderHeader.tsx`, `ReaderSettingsSheet.tsx`, `ReaderViewport.tsx`, `PageScrubber.tsx`)
- **3-State UI Toggle**:
  - Clicking/tapping center cycles `full` (Header + Footer Scrubber) → `minimal` (subtle page indicator only) → `immersive` (100% distraction-free).
- **Ambilight Ambient Glow (`FastAverageColor`)**:
  - Samples the current manga page image and casts a soft radial ambient color glow behind the page in dark mode (with a header toggle button `Tv2`).
- **Desktop Filmstrip Thumbnail Page Scrubber (`PageScrubber.tsx`)**:
  - For chapters ≤ 60 pages on desktop, the bottom scrubber renders an interactive **live thumbnail filmstrip** (`36x52px` page previews, `42x60px` active page with red glow) instead of just a plain slider!
- **Reader Header Action Bar**:
  - Inline **Chapter Dropdown** (jump to any chapter without leaving the reader).
  - Inline **Layout Mode Picker** (`Webtoon`, `L→R Paged`, `R←L Paged`, `Vertical Paged`).
  - **Export Current Chapter to PDF** (`FileText` icon) and **Export to EPUB3** (`BookOpen` icon).
  - **Enhance / Upscale toggle** (`Sparkles` icon — sharpens local archives).
  - **Save Local Archive to Cloud** (`CloudUpload` icon).
  - **Open Chapter Comments Sheet** (`MessageCircle` icon).
- **Advanced Reader Settings Sheet (`ReaderSettingsSheet.tsx`)**:
  - **4 Image Scale Modes**: `Fit Screen`, `Fit Width`, `Fit Height`, `Original`.
  - **Zoom Level Stepper**: `50%` to `200%` (`ZoomIn` / `ZoomOut`).
  - **5 Image Filters + Reset**: `Brightness` (50–150%), `Contrast` (50–150%), `Grayscale`, `Invert`, `Sepia` (Linux/Windows lack `Sepia` and `Reset Filters` button).
  - **Webtoon Controls**: `Gapless Mode` (remove vertical gap between strips), `Webtoon Side Padding` slider (`0–80px`), `Crop Borders (Webtoon)` independent of paged `Crop Borders`.
  - **Navigation & Behavior Toggles**: `Tap Zone Layouts` (`Default`, `L-Nav`, `Edge`, `Disabled`), `Skip Read Chapters` (auto-jumps over already-read chapters), `Show Footer`, `Show Page Number in Header`.
- **End-of-Chapter Transition Card & Sibling Archive Batch Detector (`batchDetector.ts`)**:
  - When finishing the last chapter of a local batch archive (e.g. `Berserk Ch. 1-10.cbz`), automatically detects `Berserk Ch. 11-20.cbz` in the same folder and offers a seamless 1-click transition!

### 5. Novel Reader (`NovelReader.tsx`, `NovelViewport.tsx`)
- **Typography & Layout Controls**:
  - **Font Family**: `Serif`, `Sans-Serif`, `Mono` (Linux/Windows only have Serif & Sans).
  - **Font Size** (`14–26px`), **Line Height** (`1.4–2.2`), **Max Content Width** (`500–900px`) sliders (Linux/Windows lack Line Height and Max Width sliders).
  - **4 Themes**: `Dark`, `Amoled Black`, `Sepia`, `Light` (Linux/Windows only have 3).
- **Novel Header Actions**:
  - **Top Reading Progress Bar** (`0–100%` scroll progress line at top of viewport).
  - **Download Current Novel Chapter as EPUB** (`D` shortcut / Download button).
  - **Chapter Comments Drawer** (`M` shortcut / Comments button).
  - **Keyboard Shortcuts Guide Modal** (`?` shortcut).
  - **Exact Scroll Position Persistence** per chapter (`novel-pos-{provider}-{novelId}-{chapterId}`).

### 6. Sources & Extensions (`Sources.tsx`)
- **Custom Extension Repositories**:
  - Input field to add custom 3rd-party extension repo URLs (`https://your-repo.github.io/index.json`), fetch their sources, and remove custom repos.
- **Enable / Disable Toggle per Installed Extension**:
  - Power button to temporarily disable an installed extension without uninstalling it.
- **18+ Adult Content (NSFW) Filter Toggle** + **Language Pills** + **Installed vs. Available Split Tabs**.

### 7. Statistics (`Stats.tsx`, `readingSession.ts`)
- **Real Reading Session Tracker (`readingSession.ts`)**:
  - Records actual time spent inside the Reader (`durationSecs`, `pagesRead`) and computes:
    - **Today's Reading Time**
    - **This Week's Reading Time**
    - **Average Session Length**
    - **Reading Speed (`Pages / Min`)**
    - **Reading Pace (`Chapters / Week`)**
    - **Estimated Total Time (`pages × 45s`)**
- **30-Day Daily Activity Bar Chart** (in addition to the 52-week GitHub heatmap).
- **Interactive Reading Goals Editor**:
  - Inline edit form on `StatsPage` to set custom `Monthly Chapter Goal` and `Yearly Manga Goal` with progress bars (Linux currently only has a "Refresh Goals" button with hardcoded `40` and `25` targets; Windows shows a stub toast `"Reading goals dialog opens here"`).

### 8. Settings Pages (`Settings/General.tsx`, `System.tsx`, `Trackers.tsx`, `Profile.tsx`, `Help.tsx`)
- **Theme Modes**:
  - Web & Android support **3 Theme Modes**: `Dark`, `AMOLED Pure Black (#000000)`, and `Light`, plus **6 Accent Colors** (`Red`, `Blue`, `Purple`, `Green`, `Orange`, `Pink`).
  - Linux & Windows currently only expose `Dark` (with 5 accents: Crimson, Indigo, Emerald, Amber, Violet) and do not have `AMOLED Black` or `Light` mode toggles.
- **Incognito Mode Quick Toggle**:
  - Available in Settings → General AND as a quick toggle in Sidebar/More, pausing all history recording with an incognito banner on `HistoryPage`.
- **Self-Hosted Server Connectors (Komga & Suwayomi)**:
  - `Settings/System.tsx` has dedicated cards for **Komga** (`URL`, `Username`, `Password`) and **Suwayomi** (`URL`).
  - On Windows, clicking "Add Server" just fires a stub toast: `"Server configuration dialog opens here"`.
- **Cloud Backup & Restore (Supabase Storage Bucket `manga-backups`)**:
  - `Settings/System.tsx` has 1-click **Cloud Backup** and **Cloud Restore** buttons in addition to local `.mangadl` and `.tachibk` file export/import.
- **6 Trackers in Settings (`Settings/Trackers.tsx`)**:
  - Web supports configuring **AniList**, **MyAnimeList (PKCE)**, **Kitsu (email/password token)**, **MangaUpdates**, **Shikimori**, and **Bangumi**, plus expandable setup instructions and custom Client ID override for self-hosters.
  - Linux & Windows currently only connect AniList and MAL; clicking Kitsu or MangaUpdates shows `"tracking will be available in an upcoming update"`.
- **Help Center (`Help.tsx`)**:
  - Web `Help.tsx` includes:
    1. **Searchable FAQ Accordion**
    2. **Visual Icon Glossary / Legend** (explaining every icon across Navigation, Reader, Library, Detail, Queue)
    3. **Featured Import Guide Banner**
    4. **In-App Support Ticket Form** (`Name`, `Email`, `Category` dropdown, `Message`) that POSTs directly to `/api/support/ticket` (Linux/Windows only open `mailto:`).

---

## Part 3: Cross-Platform Sync & Compatibility Hazards (CRITICAL)

We inspected `SupabaseSyncService` and backup/storage formats across all 4 codebases. Here are the **cross-platform sync discrepancies** that will cause data loss or missing sync between Web, Android, Linux, and Windows if not aligned:

### Hazard 1: Milestones vs. Hunter Ranks Gamification Divergence
- **Web (`lib/milestones.ts` & `Profile.tsx`)** uses a **24–31 Badge Milestone system** across 3 categories (`chapters`, `library`, `streak` — Bronze/Silver/Gold/Platinum/Diamond/Mythic tiers like *First Scroll*, *Page Turner*, *Archive Sovereign*, *Eternal Flame*) and allows pinning up to **4 badges** on your public profile.
- **Linux (`GamificationService.cpp`), Windows (`GamificationService.cs`), and Android (`GamificationEngine.kt`)** use a **Solo Leveling 8-Rank Hunter System** (`E-Rank` → `National-Level` + **10 Hunter Achievements** like `first_blood`, `shadow_monarch`).
- **Impact**: When a user views their Profile on Web vs. Desktop/Android, the badge IDs and titles don't match!
- **Fix**: Unify both systems so Desktop & Android include both the **Hunter Rank Standing Badge** (which Web's `Profile.tsx` already renders as *"Permanent Hunter Standing Badge"*) AND the full **Milestones Gallery**, using shared badge IDs.

### Hazard 2: Supabase Tables Synced on Web vs. Desktop/Android
| Supabase Table | Web Frontend | Linux (`SupabaseSyncService`) | Windows (`SupabaseSyncService`) | Android (`SupabaseSyncManager`) |
|----------------|:------------:|:-----------------------------:|:-------------------------------:|:-------------------------------:|
| `subscriptions` (`id = "{provider}/{manga_id}"`) | ✅ | ✅ | ✅ | ❌ (**BUG A-1**: queries legacy `manga` table!) |
| `read_tracking` (`chapter_ids` array) | ✅ | ✅ | ✅ | ✅ |
| `reading_progress` (`last_page`, `manga_title`, `chapter_title`) | ✅ | ✅ | ✅ | ✅ |
| `user_categories` (`custom_categories`, `manga_assignments`) | ✅ | ❌ Not synced | ⚠️ Model exists (`SupabaseUserCategoriesRecord`), never called in `SyncAllAsync`! | ❌ Not synced |
| `manga_notes` (`note`, `rating` 0–5) | ✅ | ❌ Not synced | ❌ Not synced | ❌ Not synced |
| `comments` (chapter & manga comments) | ✅ | ❌ Not wired | ❌ Not wired | ✅ |

- **Critical Fix Needed**:
  1. **Android (`A-1`)** must be updated from `/rest/v1/manga` to `/rest/v1/subscriptions` so Android library sync works with Web, Linux, and Windows.
  2. **Linux & Windows** `SupabaseSyncService` should also pull/push `user_categories` and `manga_notes` during `syncAll()` so custom categories, category assignments, 5-star ratings, and personal notes sync seamlessly with Web!

### Hazard 3: Category Assignment Key Format (`title.toLowerCase()` vs `provider/mangaId`)
- **Web (`lib/categories.ts`)** keys `manga_assignments` in `user_categories` by `mangaTitle.toLowerCase().trim()`.
- **Linux & Windows (`MangaDatabase`)** map categories in SQLite `manga_categories` by `manga_id` (`{provider}/{mangaId}`).
- **Fix**: When syncing `user_categories` with Supabase on Linux/Windows, resolve both `title.toLowerCase().trim()` and `{provider}/{mangaId}` so category assignments created on Web appear on Desktop and vice versa.

### Hazard 4: Extension Source Count Mismatch on Android (`A-2`)
- **Web, Linux, and Windows** all execute the **38 JS extensions** (`sources/*.js`).
- **Android** uses native Kotlin scrapers and only implements **24 sources** (missing 14 sources including `flamecomics`, `manhuaplus`, `manganato`, `mangakakalot`, `readlightnovels`, `scribblehub`, `freewebnovel`, `boxnovel`).
- **Impact**: If a user subscribes to a series from one of those 14 sources on Linux/Windows/Web and syncs to Android, Android cannot load chapters for that series.

---

## Part 4: Unimplemented Stubs & Bugs in Other Codebases

### A. Windows (`manga-dl-windows`) — Still Has `Sample.*` Fallbacks & Unwired UI Stubs!
Unlike Linux (where we completely eradicated `SampleData` and wired every controller in Phases 1–9), **Windows still has `SampleData.cs` (`Sample.*`) wired into 11 pages and multiple stub buttons**:
1. **`SampleData.cs` Still Active in Production Pages**:
   - `LibraryPage.xaml.cs`: Initializes `Items = new(Sample.Library)` and `Continue = new(Sample.ContinueReading)`, and falls back to `Sample.HollowCrown` when opening manga/reader!
   - `MangaDetailPage.xaml.cs`: Initializes `Chapters = new(Sample.Chapters)` and `_currentManga = Sample.HollowCrown`.
   - `SearchPage.xaml.cs`: Initializes `Groups = new(Sample.SearchResults)` and resets to `Sample.SearchResults` when search box is cleared!
   - `BrowseSourcePage.xaml.cs`: Initializes `Items = new(Sample.BrowseCatalog)`.
   - `ExtensionsPage.xaml.cs`: Uses `Sample.LastUsedSource`, `Sample.Sources`, `Sample.Extensions`, and `Sample.Library`! **And `OnMigrate` is a 1-line stub toast**: `Nav.Toast("Pick a target source to migrate to")`!
   - `DownloadsPage.xaml.cs`, `HistoryPage.xaml.cs`, `NotificationsPage.xaml.cs`, `UpdatesPage.xaml.cs`: All initialize with `Sample.*` collections before async DB load or fall back to `Sample.*` when empty!
   - `StatsPage.xaml.cs`: Uses hardcoded `Sample.Activity`, `Sample.ByCategory`, `Sample.BySource` whenever DB stats are empty, and **`OnEditGoals` is a stub toast**: `Nav.Toast("Reading goals dialog opens here")`!
2. **Unimplemented Stub Buttons in Windows Settings**:
   - `SystemSettingsPage.xaml.cs`:
     - `OnClearCache` → only shows `Nav.Toast("Image cache cleared")` without deleting any files!
     - `OnAddServer` → only shows `Nav.Toast("Server configuration dialog opens here")`!
     - `OnInstall` (Update button) → hardcoded version string instead of real GitHub release check!
   - `ReaderSettingsPage.xaml.cs`:
     - `OnCustomiseShortcuts` → only shows a toast!
   - `RegisterPage.xaml.cs`:
     - `OnTermsClick` → only shows a 1-line toast!
   - `BrowseSourcePage.xaml.cs`:
     - `OnOpenBrowser` and `OnOpenFilters` → only show a toast (`Nav.Toast("Opening in browser")` / `Nav.Toast("Filter settings")`) without actually opening the browser or filter dialog!
3. **Windows Core Bugs (`W-1` to `W-13` from `FINDINGS-OTHER-CODEBASES.md`)**:
   - **`W-1` (Deadlock)**: `ExtensionBridge.cs` uses `.GetAwaiter().GetResult()` inside Jint callbacks (`fetch`, `fetchJson`, `descrambleImage`), which deadlocks if ever invoked from the UI thread.
   - **`W-2` (Invalid CSS Selectors)**: `DomShim.cs` passes `:contains(...)` and `img[src^=http]` directly to AngleSharp `QuerySelectorAll`, throwing `DomException` on ` flamecomics`, `kissmanga`, `mangago`, `ReadLightNovels`.
   - **`W-3` (Missing `globalThis.__ext`)**: `ExtensionBridge.cs` only checks `_engine.GetValue("__ext")`, breaking IIFE extensions wrapped by the Web backend.

### B. Web Frontend & Backend (`manga-dl`) Bugs (`X-1` to `X-9`)
1. **`X-1` (`FlameComics` broken on Web)**: `flamecomics/index.js` uses `div:contains("Status")`, which throws `SyntaxError` in browser `DOMParser.querySelectorAll` and Python `selectolax`. (Already fixed in Linux's `Selector.cpp`!).
2. **`X-2` (`WeebCentral` cover broken)**: `weebcentral/index.js` uses unquoted `img[src^=http]` in `getMangaDetail`. (Already fixed in Linux's `Selector.cpp`!).
3. **`X-3` (`NovelFire` chapters broken)**: `novelfire/index.js` expects JSON from `/ajax/chapter-archive` when NovelFire returns HTML. (Already fixed in Linux!).
4. **`X-4` (26 extensions broken on Web Backend Python runner)**: `backend/app/services/js_extensions.py` injects `dom_shim.js` (`__parseHtml`), which is missing from the repo!

### C. Android (`manga-dl-android`) Bugs (`A-1`, `A-2`)
1. **`A-1` (Broken Library Cloud Sync)**: Queries `/rest/v1/manga` instead of `/rest/v1/subscriptions`.
2. **`A-2` (Missing 14 Sources)**: Only 24 native Kotlin scrapers vs 38 JS extensions on Desktop/Web.
