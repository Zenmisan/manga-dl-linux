#pragma once

#include <QString>

/// Application log: every qDebug/qInfo/qWarning/qCritical goes to stderr and to a rotated file.
///
/// The file is `<dir>/manga-dl.log`. When it grows past `maxBytes` it is renamed to `manga-dl.log.1`
/// (older ones shift up) and at most `keep` files in total are retained.
namespace AppLog {

constexpr qint64 kDefaultMaxBytes = 2 * 1024 * 1024;
constexpr int kDefaultKeep = 3;

/// Install the Qt message handler. Safe to call more than once (re-initialises).
/// Returns false if the log file could not be opened; messages still go to stderr in that case.
bool init(const QString &dir, qint64 maxBytes = kDefaultMaxBytes, int keep = kDefaultKeep);

/// Path of the current log file (empty before init()).
QString logFilePath();

/// Flush and close the file and restore the previous message handler.
void shutdown();

} // namespace AppLog
