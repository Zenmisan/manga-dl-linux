#pragma once

#include <QString>

/// Where manga-dl keeps its files. Follows the XDG Base Directory spec; no Qt application object is needed,
/// so it can be used from tests and before QGuiApplication exists.
///
///   data   $XDG_DATA_HOME/manga-dl    (default ~/.local/share/manga-dl)   database, user extensions
///   config $XDG_CONFIG_HOME/manga-dl  (default ~/.config/manga-dl)
///   cache  $XDG_CACHE_HOME/manga-dl   (default ~/.cache/manga-dl)         covers, page images
///   state  $XDG_STATE_HOME/manga-dl   (default ~/.local/state/manga-dl)   logs
namespace AppPaths {

QString dataDir();
QString configDir();
QString cacheDir();
QString stateDir();

QString extensionsDir();   ///< user-installed extension scripts: <data>/extensions
QString databasePath();    ///< <data>/manga.db
QString imageCacheDir();   ///< <cache>/images

/// Default download folder: the desktop's Downloads directory (XDG user-dirs) + "/manga-dl".
QString defaultDownloadDir();

/// Create every directory above. Returns false if any could not be created.
bool ensureDirs();

/// Redirect everything under `root` (used by tests so they never touch the real profile).
/// Pass an empty string to go back to the XDG locations.
void setRootOverride(const QString &root);

} // namespace AppPaths
