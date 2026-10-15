import QtQuick
import MangaDl

// Coloured square (or circle) with an initial: sources, extensions, trackers, avatars.
Rectangle {
    property alias text: label.text
    property int size: 44
    property bool round: false
    property int fontSize: Math.round(size * 0.36)
    width: size
    height: size
    radius: round ? size / 2 : Math.round(size * 0.27)
    color: "#22222A"
    Txt { id: label; anchors.centerIn: parent; font.pixelSize: parent.fontSize; font.weight: Font.Black }
}
