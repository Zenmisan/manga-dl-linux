# manga-dl for Linux (Qt 6 · C++20 + QML)

A high-performance, native Linux client for **manga-dl**, built with C++20 and Qt 6 (Qt Quick / QML). It achieves complete 1:1 feature and backend parity with `manga-dl-windows` and the web client.

---

## Features

- **Local-first SQLite Storage**: SQLite with WAL mode (`$XDG_DATA_HOME/manga-dl/manga.db`), versioned schema migrations, and full persistence across library items, reading progress, history, downloads, tracker binds, and categories.
- **Embedded Extension Engine**: Integrated `quickjs-ng` (v0.17.0) runtime running on a dedicated worker thread, featuring a complete DOM parser and CSS selector engine supporting all 38+ bundled manga and novel sources.
- **Native Image Descrambling**: In-memory descrambler supporting Comixto (PRNG XOR/LCG descramble) and Asura Scans (5×5 tile reassembly) with custom `image://` providers.
- **Manga & Novel Readers**:
  - Manga reader with single-page, two-page spread, vertical, and continuous webtoon modes; RTL/LTR reading direction; keyboard/mouse navigation; and ambilight backdrop tinting.
  - Webtoon mode features delegate virtualization (`reuseItems: true`, bounded cache buffer) for ultra-low memory footprints on 100+ page chapters.
  - Web novel reader with customizable fonts, themes (AMOLED, Dark, Paper, Sepia), font sizes, and line heights.
  - Direct reading from local archives without extraction to temp directories (`image://cbz/`).
- **Downloads & Local Archive Imports**:
  - High-concurrency download manager saving chapters directly to valid `.cbz` archives (uncompressed `STORE` mode with `ComicInfo.xml` metadata) or EPUB 3.0 files.
  - Local import service supporting `.cbz`, `.zip`, `.epub`, loose image folders, natural sorting, and drag-and-drop.
- **Cloud Accounts & Bidirectional Sync**:
  - Native authentication architecture: Supabase GoTrue for email/password and account lifecycle; Firebase Auth loopback listener for Google Sign-In (`z-manga-dl`).
  - Secure credential storage using `libsecret-1` (GNOME Keyring / KWallet) with fallback to 0600 file settings.
  - Bidirectional PostgREST sync engine synchronizing subscriptions, reading progress, user categories, manga notes, and profile details with Supabase.
- **Trackers**:
  - AniList (OAuth PKCE + GraphQL viewer/media search/mutation).
  - MyAnimeList (Plain PKCE + REST v2 `@me` status).
  - Automated chapter sync upon reading.
- **Gamification & Profile**:
  - Hunter rank system (E-Rank Novice through Shadow Monarch).
  - 21 achievement milestones tracking chapters read, library collections, and daily streaks.
  - 52-week activity heatmap (364-day bucketing) and reading goal counters.
- **Desktop & System Integration**:
  - Native desktop notifications via D-Bus (`org.freedesktop.Notifications`) with `notify-send` fallback.
  - System tray icon with minimize-to-tray and restore.
  - Automated background chapter update checker (30-minute timer).
  - Storage statistics (`QStorageInfo`), image cache clearing, and desktop autostart toggle (`~/.config/autostart`).
  - Universal backup & restore with 100% cross-platform schema parity supporting `.mangadl` (`library`, `progress`, `trackerBinds`, `categories`, `mangaCategories`, `bookmarks`, `settings`) and Tachiyomi `.tachibk` (gzipped protobuf).

---

## Dependencies

### Core Requirements
- C++20 compiler: GCC 11+ or Clang 14+
- CMake 3.21+
- Qt 6.4+ (Core, Gui, Qml, Quick, QuickControls2, Svg, Network, Sql, DBus, Widgets)
- `libzip` (>= 1.8)
- `zlib`
- `libsecret-1` (optional runtime library for keyring credential storage)
- `qt6-imageformats` (required for WebP page decoding)

### Distro Package Installation

#### Ubuntu / Debian (22.04+)
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config \
    qt6-base-dev qt6-declarative-dev qt6-svg-dev libqt6widgets6 \
    qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
    qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtqml-workerscript \
    qml6-module-qtquick-dialogs qt6-image-formats-plugins \
    libzip-dev zlib1g-dev libsecret-1-dev
```

#### Arch Linux / Manjaro
```bash
sudo pacman -S --needed base-devel cmake \
    qt6-base qt6-declarative qt6-svg qt6-imageformats \
    libzip zlib libsecret
```

#### Fedora (39+)
```bash
sudo dnf install cmake gcc-c++ pkgconf-pkg-config \
    qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel \
    qt6-qtimageformats libzip-devel zlib-devel libsecret-devel
```

---

## Build & Run

### Debug Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Run the app
./build/manga-dl

# Start directly on a specific page
./build/manga-dl --page Library
./build/manga-dl --page Search
./build/manga-dl --page Downloads
```

### Release Build & Install
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo cmake --install build
```

This installs:
- Binary: `/usr/bin/manga-dl`
- Desktop entry: `/usr/share/applications/io.github.zenmisan.MangaDl.desktop`
- Application icon: `/usr/share/icons/hicolor/256x256/apps/io.github.zenmisan.MangaDl.png`

---

## Running Tests

The test suite contains 20 ctest targets covering all core services, models, and controllers:

```bash
cd build
ctest --output-on-failure
```

---

## Architecture Layout

```
manga-dl-linux/
├── assets/                  # Icons, fonts (Anton, Inter, PT Serif), extensions
├── packaging/               # Desktop launcher and metadata
├── qml/
│   ├── components/          # Reusable controls (Btn, Field, MdSwitch, Tabs, AutoGrid, ...)
│   ├── pages/               # Application screens (Library, Search, Reader, Downloads, ...)
│   │   └── settings/        # Settings screens (General, Library, System, Trackers, ...)
│   └── theme/               # Theme tokens (Theme.qml) and router (Nav.qml)
├── src/
│   ├── core/
│   │   ├── backup/          # .mangadl and Tachiyomi protobuf backup parsers
│   │   ├── db/              # SQLite database, migrations, and QAbstractListModels
│   │   ├── downloads/       # CbzBuilder, NovelEpubBuilder, and DownloadManager
│   │   ├── extensions/      # QuickJS worker, DOMParser, and Descrambler
│   │   ├── gamification/    # Hunter ranks, badges, and activity heatmaps
│   │   ├── local/           # Archive inspector and local file importer
│   │   ├── settings/        # QSettings-backed AppSettings
│   │   ├── sync/            # Supabase GoTrue, SecretStorage, and DB sync
│   │   ├── tracking/        # AniList, MyAnimeList, and OAuth loopback listener
│   │   └── updates/         # D-Bus notifications and update checker
│   ├── *Controller.{h,cpp}  # QML controllers (Library, Reader, Search, Profile, ...)
│   └── main.cpp             # Entry point, singletons, and tray icon setup
├── tests/                   # 20 C++ unit and integration test targets
└── vendor/
    └── quickjs/             # Vendored quickjs-ng sources
```

---

## License

MIT License.
