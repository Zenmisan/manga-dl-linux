import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Smaller cover used in search results and source catalogues, with an "In Library" tag.
AbstractButton {
    id: tile
    property var manga: ({})
    property bool dimInLibrary: false
    hoverEnabled: true
    implicitHeight: art.height + 6 + 18
    Accessible.name: manga.title || ""
    onClicked: Nav.open("MangaDetail", { manga: tile.manga })

    Item {
        id: art
        width: parent.width
        height: Math.round(width * 1.5)
        opacity: tile.dimInLibrary && tile.manga.inLibrary ? 0.55 : 1

        CoverArt {
            anchors.fill: parent
            radius: 10
            cover: tile.manga.coverUrl || tile.manga.cover || "#22222A"
        }
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "transparent"
            border.width: tile.visualFocus ? 2 : 0
            border.color: Theme.accentLight
        }
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: Theme.hover
            visible: tile.hovered
        }
    }
    Rectangle {
        visible: !!tile.manga.inLibrary
        x: 6; y: 6
        width: tag.implicitWidth + 14; height: tag.implicitHeight + 6
        radius: 6
        color: Theme.accent
        Txt { id: tag; anchors.centerIn: parent; text: "In Library"; font.pixelSize: 10; font.weight: Font.ExtraBold; color: Theme.white }
    }
    Txt {
        y: art.height + 6
        width: parent.width
        text: tile.manga.title || ""
        font.pixelSize: 12
        font.weight: Font.DemiBold
        elide: Text.ElideRight
    }
}
