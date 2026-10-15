import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// A plain clickable surface with hover + focus ring. Put any content inside.
AbstractButton {
    id: control
    property int radius: 12
    property color fill: "transparent"
    default property alias content: holder.data
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    implicitWidth: holder.childrenRect.width
    implicitHeight: holder.childrenRect.height
    background: Rectangle {
        radius: control.radius
        color: control.fill
        border.width: control.visualFocus ? 1 : 0
        border.color: Theme.accentLight
        Rectangle { anchors.fill: parent; radius: parent.radius; color: Theme.hover; opacity: control.down ? 0.6 : control.hovered ? 1 : 0 }
    }
    contentItem: Item { id: holder }
}
