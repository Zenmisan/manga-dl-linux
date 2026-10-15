#include "IconProvider.h"

#include <QHash>
#include <QPainter>
#include <QStringList>
#include <QSvgRenderer>
#include <QUrlQuery>

namespace {

struct IconDef {
    QStringList paths;
    double stroke;
    bool filled;
};

// Lucide-style 24px icons, 2px round strokes. Same paths as the Android and Windows apps.
const QHash<QString, IconDef> &icons()
{
    static const QHash<QString, IconDef> map = {
        {"Search", {{"M11 4a7 7 0 1 0 0 14a7 7 0 1 0 0-14z", "M20 20l-3.5-3.5"}, 2.0, false}},
        {"Filter", {{"M4 6h16M7 12h10M10 18h4"}, 2.0, false}},
        {"More", {{"M12 5h.01M12 12h.01M12 19h.01"}, 3, false}},
        {"Library", {{"M5 4h11a3 3 0 0 1 3 3v13H8a3 3 0 0 1-3-3z", "M5 17a3 3 0 0 1 3-3h11"}, 2.0, false}},
        {"Updates", {{"M6 16v-5a6 6 0 0 1 12 0v5l2 2H4z", "M10 21h4"}, 2.0, false}},
        {"History", {{"M12 4a8 8 0 1 0 0 16a8 8 0 1 0 0-16z", "M12 8v4l3 2"}, 2.0, false}},
        {"Browse", {{"M12 3a9 9 0 1 0 0 18a9 9 0 1 0 0-18z", "M15.5 8.5l-2 5-5 2 2-5z"}, 2.0, false}},
        {"Menu", {{"M4 7h16M4 12h16M4 17h16"}, 2.0, false}},
        {"Back", {{"M15 5l-7 7 7 7"}, 2.0, false}},
        {"ChevronRight", {{"M9 5l7 7-7 7"}, 2.0, false}},
        {"ChevronDown", {{"M6 9l6 6 6-6"}, 2.0, false}},
        {"ChevronUp", {{"M6 15l6-6 6 6"}, 2.0, false}},
        {"Download", {{"M12 4v11M7 10l5 5 5-5M5 20h14"}, 2.0, false}},
        {"Upload", {{"M4 20h16M12 16V4M7 9l5-5 5 5"}, 2.0, false}},
        {"Check", {{"M5 12l5 5 9-10"}, 2.5, false}},
        {"Play", {{"M8 5l11 7-11 7z"}, 2.0, true}},
        {"Pause", {{"M8 5v14M16 5v14"}, 2.2, false}},
        {"Bookmark", {{"M7 4h10v16l-5-4-5 4z"}, 2.0, false}},
        {"BookmarkFilled", {{"M6 3h12v18l-6-4-6 4z"}, 2.0, true}},
        {"Sliders", {{"M4 7h10M18 7h2M4 17h4M12 17h8", "M16 5a2 2 0 1 0 0 4a2 2 0 1 0 0-4z", "M10 15a2 2 0 1 0 0 4a2 2 0 1 0 0-4z"}, 2.0, false}},
        {"Settings", {{"M4 7h10M18 7h2M4 17h4M12 17h8M16 5v4M10 15v4"}, 2.0, false}},
        {"Globe", {{"M12 3a9 9 0 1 0 0 18a9 9 0 1 0 0-18z", "M3 12h18M12 3a14 14 0 0 1 0 18M12 3a14 14 0 0 0 0 18"}, 2.0, false}},
        {"Sort", {{"M7 4v16M4 17l3 3 3-3M17 20V4M14 7l3-3 3 3"}, 2.0, false}},
        {"Close", {{"M6 6l12 12M18 6L6 18"}, 2.0, false}},
        {"Refresh", {{"M20 12a8 8 0 1 1-2.3-5.7M20 4v5h-5"}, 2.0, false}},
        {"Trash", {{"M4 7h16M9 7V4h6v3M6 7l1 13h10l1-13"}, 2.0, false}},
        {"Pin", {{"M12 17v5M8 3h8l-1 6 3 4H6l3-4z"}, 2.0, false}},
        {"Mail", {{"M4 6h16v12H4zM4 7l8 6 8-6"}, 2.0, false}},
        {"Stats", {{"M5 20V10M12 20V4M19 20v-7"}, 2.0, false}},
        {"Backup", {{"M4 6h16v4H4zM6 10v10h12V10M10 14h4"}, 2.0, false}},
        {"Help", {{"M12 3a9 9 0 1 0 0 18a9 9 0 1 0 0-18z", "M9.5 9.5a2.5 2.5 0 1 1 3.5 2.3c-.6.3-1 .9-1 1.6M12 17h.01"}, 2.0, false}},
        {"User", {{"M12 4a4 4 0 1 0 0 8a4 4 0 1 0 0-8z", "M4 20a8 8 0 0 1 16 0"}, 2.0, false}},
        {"Track", {{"M20 12a8 8 0 0 1-14 5.3M4 12a8 8 0 0 1 14-5.3", "M18 3v4h-4M6 21v-4h4"}, 2.0, false}},
        {"Share", {{"M12 4v12M7 9l5-5 5 5M5 14v6h14v-6"}, 2.0, false}},
        {"Warning", {{"M12 4l9 16H3z", "M12 10v4M12 17h.01"}, 2.0, false}},
        {"ArrowUp", {{"M12 20V6M6 12l6-6 6 6"}, 2.0, false}},
        {"Pages", {{"M6 3h12a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z", "M12 3v18"}, 2.0, false}},
        {"Crop", {{"M4 9V4h5M20 15v5h-5M4 4l6 6M20 20l-6-6"}, 2.0, false}},
        {"Sun", {{"M12 8a4 4 0 1 0 0 8a4 4 0 1 0 0-8z", "M12 2v2M12 20v2M2 12h2M20 12h2M5 5l1.5 1.5M17.5 17.5L19 19M5 19l1.5-1.5M17.5 6.5L19 5"}, 2.0, false}},
        {"PrevChapter", {{"M18 6l-6 6 6 6M8 6v12"}, 2.0, false}},
        {"NextChapter", {{"M6 6l6 6-6 6M16 6v12"}, 2.0, false}},
        {"List", {{"M9 6h11M9 12h11M9 18h11M4 6h.01M4 12h.01M4 18h.01"}, 2.0, false}},
        {"CloudCheck", {{"M7 18a5 5 0 0 1-.5-10A6 6 0 0 1 18 9a4.5 4.5 0 0 1-.5 9z", "M9 13l2 2 4-4"}, 2.0, false}},
        {"Edit", {{"M4 20h4L19 9l-4-4L4 16zM14 6l4 4"}, 2.0, false}},
        {"Plus", {{"M12 5v14M5 12h14"}, 2.0, false}},
        {"Fullscreen", {{"M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5"}, 2.0, false}},
        {"Folder", {{"M3 6a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v10a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"}, 2.0, false}},
        {"Grip", {{"M9 6h.01M15 6h.01M9 12h.01M15 12h.01M9 18h.01M15 18h.01"}, 3, false}},
    };
    return map;
}

} // namespace

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

bool IconProvider::has(const QString &name) { return icons().contains(name); }

// id is "<Name>?color=%23RRGGBB" (the color is URL-encoded by Icon.qml).
QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const auto q = id.indexOf(u'?');
    const QString name = q < 0 ? id : id.left(q);
    const QString color = q < 0 ? QStringLiteral("#FAFAFA")
                                : QUrlQuery(id.mid(q + 1)).queryItemValue(QStringLiteral("color"), QUrl::FullyDecoded);

    const QSize out = requestedSize.isValid() && !requestedSize.isEmpty() ? requestedSize : QSize(24, 24);
    if (size) *size = out;

    QImage image(out, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    const auto it = icons().constFind(name);
    if (it == icons().constEnd()) return image;

    QString svg = QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>");
    for (const QString &d : it->paths) {
        if (it->filled)
            svg += QStringLiteral("<path d='%1' fill='%2'/>").arg(d, color);
        else
            svg += QStringLiteral("<path d='%1' fill='none' stroke='%2' stroke-width='%3' stroke-linecap='round' stroke-linejoin='round'/>")
                       .arg(d, color, QString::number(it->stroke));
    }
    svg += QStringLiteral("</svg>");

    QSvgRenderer renderer(svg.toUtf8());
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    return image;
}
