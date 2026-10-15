import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Library/browse cover: 2:3 art plus a two-line title. Clicking opens the manga.
AbstractButton {
    id: tile
    property var manga: ({})
    property bool showBadges: true
    property bool dimInLibrary: false
    hoverEnabled: true
    implicitWidth: 140
    implicitHeight: art.height + 8 + title.height
    Accessible.name: manga.title || ""
    onClicked: Nav.open("MangaDetail", { manga: tile.manga })

    CoverArt {
        id: art
        width: parent.width
        height: Math.round(parent.width * 1.5)
        cover: tile.manga.cover || "#22222A"
        unread: tile.showBadges ? (tile.manga.unread || 0) : 0
        downloaded: tile.showBadges && !!tile.manga.downloaded
        opacity: tile.dimInLibrary && tile.manga.inLibrary ? 0.55 : 1
        border.width: tile.visualFocus ? 2 : 0
        border.color: Theme.accentLight
        Rectangle { anchors.fill: parent; radius: parent.radius; color: Theme.hover; visible: tile.hovered }
    }
    Txt {
        id: title
        y: art.height + 8
        width: parent.width
        text: tile.manga.title || ""
        font.pixelSize: 13
        font.weight: Font.DemiBold
        wrapMode: Text.Wrap
        maximumLineCount: 2
        elide: Text.ElideRight
        height: 36
    }
}
