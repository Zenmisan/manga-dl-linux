#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

// ─── Source-level error ───────────────────────────────────────────────────────

enum class SourceErrorKind {
    Unknown,
    NetworkError,       ///< QNetworkReply error
    ParseError,         ///< JS exception or unexpected response shape
    AuthRequired,       ///< HTTP 401/403
    NotFound,           ///< HTTP 404
    Timeout,            ///< JS interrupt or network timeout
    ScriptLoadError,    ///< Extension script failed to evaluate
};

struct SourceError {
    SourceErrorKind kind = SourceErrorKind::Unknown;
    QString message;

    bool ok() const { return false; } // always an error
    QString toString() const;

    static SourceError fromText(const QString &text, SourceErrorKind k = SourceErrorKind::Unknown);
};

// ─── Extension metadata ───────────────────────────────────────────────────────

enum class ExtensionType { Manga, Novel };

struct ExtensionInfo {
    QString id;         ///< e.g. "mangadex"
    QString name;       ///< display name
    QString language;   ///< "en", "ja", ...
    ExtensionType type = ExtensionType::Manga;
    QString version;
    QString filePath;   ///< absolute path or "qrc:/..." for builtins
    bool isBuiltin = false;
    bool isInstalled = false;
};

// ─── Search / detail / chapter result types ───────────────────────────────────

struct MangaPreview {
    QString id;
    QString title;
    QString coverUrl;
    QString url;
    QString provider;   ///< extension id
};

struct ChapterInfo {
    QString id;
    QString title;
    double  number = 0.0;
    QString publishedAt;
    QString scanlator;
    QString url;
    bool    read = false;
    bool    downloaded = false;
};

struct MangaDetail {
    QString id;
    QString title;
    QString coverUrl;
    QString url;
    QString description;
    QString status;
    QStringList genres;
    QStringList authors;
    QStringList artists;
    QString provider;
    QList<ChapterInfo> chapters;
    QVariantMap raw;    ///< original JS object for fields not explicitly mapped
};

struct PageInfo {
    QString url;        ///< image URL (may start with comixto:// or use asura-descramble path)
    int width  = 0;
    int height = 0;
};
