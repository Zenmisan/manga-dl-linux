# manga-dl-linux — Current Project Status

> **Last Updated**: 2026-10-22  
> **Target**: Qt 6.4+ · C++20 · Qt Quick (QML) · QuickJS-ng v0.17.0 · SQLite WAL  
> **Status**: Feature-complete, 20/20 test targets passing, full Windows/Web parity achieved.

---

## 🚀 Overview

`manga-dl-linux` is the high-performance native desktop client for the manga-dl ecosystem on Linux. It operates 100% locally on-device without requiring a running Render backend or external proxy servers for scraping.

---

## ✅ Feature Implementation Matrix

| Area | Component | Implementation Status | Notes |
|---|---|:---:|---|
| **Core Storage** | SQLite WAL & Migrations | ✅ Complete | Versioned migrations, connection pooling, full library/progress/tracker/history models |
| **Extension Engine** | QuickJS-ng v0.17.0 Worker | ✅ Complete | Dedicated worker thread, async non-blocking `apiFetch`, microtask event loop, 38+ bundled sources |
| **DOM Engine** | `HtmlParser` & `Selector` | ✅ Complete | Live DOM tree mutation, `.remove()`, `classList`, live `innerHTML`/`textContent`, `:contains`, `:has` |
| **Descrambling** | Comix.to & Asura Scans | ✅ Complete | Native C++ QImage pixel descrambling (PRNG XOR/LCG + 5×5 tile reassembly) |
| **Manga Reader** | Single, Spread, Webtoon | ✅ Complete | Wayland/X11 responsive, delegate virtualization (`reuseItems`), bounded memory cache, Ambilight tint |
| **Novel Reader** | Text / EPUB Reader | ✅ Complete | Dynamic font size/family, line height, dark/AMOLED/sepia/paper themes |
| **Downloads** | `DownloadManager` & Builders | ✅ Complete | Parallel downloads, uncompressed `STORE` CBZ archives with `ComicInfo.xml`, EPUB 3.0 generation |
| **Local Import** | `ArchiveInspector` & Importer | ✅ Complete | Drag-and-drop CBZ/ZIP/EPUB, natural chapter sort, non-destructive reading via `image://cbz/` |
| **Authentication** | Supabase & Firebase Auth | ✅ Complete | Email/password via Supabase GoTrue; Google Sign-In via Firebase Auth loopback flow |
| **Cloud Sync** | PostgREST Sync Engine | ✅ Complete | Bidirectional sync for subscriptions, read tracking, reading progress, categories, and manga notes |
| **Trackers** | AniList & MyAnimeList | ✅ Complete | PKCE OAuth loopback listeners, GraphQL/REST mutations, auto-sync upon chapter completion |
| **Gamification** | Hunter Ranks & Badges | ✅ Complete | 21 achievement milestones, 52-week activity heatmap, EXP progression system |
| **Desktop / OS** | Tray & Notifications | ✅ Complete | FreeDesktop D-Bus notifications, system tray icon, autostart toggle, storage analytics |
| **Responsiveness** | Wayland / Tiling WM Parity | ✅ Complete | Adaptive compact sidebar rail (68px), wrapping action flows, zero-width preferred layouts, and responsive grids for Sway, Hyprland, and variable split viewports |
| **Backups** | Universal Backup Service | ✅ Complete | Full cross-platform `.mangadl` JSON schema parity + Tachiyomi `.tachibk` protobuf restore |

---

## 🧪 Verification & Test Suite

All 20 CMake/CTest test suites execute with a 100% pass rate:

```bash
Test project /home/zenmi/Projects/manga-dl-linux/build
      Start  1: tst_smoke ........................   Passed
      Start  2: tst_apppaths .....................   Passed
      Start  3: tst_applog .......................   Passed
      Start  4: tst_quickjs ......................   Passed
      Start  5: tst_selector .....................   Passed
      Start  6: tst_selector_extra ...............   Passed
      Start  7: tst_extension_selectors ..........   Passed
      Start  8: tst_jsworker .....................   Passed
      Start  9: tst_descramble ...................   Passed
      Start 10: tst_extensions ...................   Passed
      Start 11: tst_r3 ...........................   Passed
      Start 12: tst_database .....................   Passed
      Start 13: tst_settings .....................   Passed
      Start 14: tst_models .......................   Passed
      Start 15: tst_imageprovider ................   Passed
      Start 16: tst_controllers ..................   Passed
      Start 17: tst_discovery ....................   Passed
      Start 18: tst_downloads ....................   Passed
      Start 19: tst_sync .........................   Passed
      Start 20: tst_services .....................   Passed

100% tests passed out of 20
```

---

## 🛠️ Build & Development Commands

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Run test suite
QT_QPA_PLATFORM=offscreen QT_QPA_PLATFORMTHEME=generic ctest --test-dir build --output-on-failure

# Launch application
./build/manga-dl
```
