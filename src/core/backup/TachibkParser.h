#pragma once

#include <QByteArray>
#include <QString>
#include <QMap>
#include "TachibkModels.h"

namespace mangadl {

class TachibkParser {
public:
    static TachiyomiBackupResult parseTachiyomiBackup(const QByteArray &data, const QString &filename = QString());
    static TachiyomiBackupResult decodeTachibk(const QByteArray &data);
    static TachiyomiBackupResult parseJsonBackup(const QByteArray &data);

    static QString resolveProvider(const QString &sourceName, const QString &mangaUrl = QString());
    static QString cleanMangaId(const QString &url, const QString &provider = QString());
    static QString cleanChapterId(const QString &url, double chapterNumber = 0.0);

    static QByteArray gunzip(const QByteArray &data);
    static bool isGzip(const QByteArray &data);
    static bool looksLikeJson(const QByteArray &data);
};

} // namespace mangadl
