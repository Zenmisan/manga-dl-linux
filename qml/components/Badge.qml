import QtQuick
import MangaDl

// Red count pill (sidebar, tabs).
Rectangle {
    property alias text: label.text
    property color fill: Theme.accent
    implicitWidth: Math.max(20, label.implicitWidth + 12)
    implicitHeight: 20
    radius: 10
    color: fill
    Txt { id: label; anchors.centerIn: parent; font.pixelSize: 11; font.weight: Font.ExtraBold; color: Theme.white }
}
