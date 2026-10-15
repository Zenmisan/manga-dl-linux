import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// 44px square icon button that stays lit while checked (reader toolbars).
Button {
    id: control
    property string iconName
    checkable: true
    hoverEnabled: true
    implicitWidth: 44
    implicitHeight: 44
    padding: 0
    contentItem: Item {
        Icon { anchors.centerIn: parent; name: control.iconName; size: 20; color: control.checked ? Theme.accentLight : Theme.fg }
    }
    background: Rectangle {
        radius: 10
        color: control.checked ? Theme.accentNav : control.hovered ? Theme.hover : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: Theme.accentLight
    }
}
