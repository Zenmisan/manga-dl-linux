import QtQuick
import MangaDl

// Thin rounded progress bar. value is 0–1.
Rectangle {
    property real value: 0
    property color fill: Theme.accent
    implicitHeight: 4
    radius: height / 2
    color: Theme.track
    Rectangle {
        width: parent.width * Math.max(0, Math.min(1, parent.value))
        height: parent.height
        radius: parent.radius
        color: parent.fill
    }
}
