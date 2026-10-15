#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace mangadl {

struct EpubChapter {
    QString title;
    QString html;
};

struct EpubBook {
    QString title;
    QList<EpubChapter> chapters;
};

class EpubParser {
public:
    static EpubBook parse(const QString &epubFilePath);
    static EpubBook parseBytes(const QByteArray &epubBytes);

    static QString normalizeZipPath(const QString &path);
    static QString extractChapterTitle(const QString &html);
};

} // namespace mangadl
