import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Sidebar entry: icon, label, optional badge. Lit when `selected`.
AbstractButton {
    id: item
    property string iconName
    property string badge: ""
    property bool selected: false
    text: ""
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    implicitHeight: 44
    implicitWidth: 208
    Accessible.role: Accessible.PageTab
    Accessible.name: badge ? text + ", " + badge + " new" : text
    Accessible.selected: selected

    background: Rectangle {
        radius: 10
        color: item.selected ? Theme.accentMuted : item.hovered ? Theme.hover : "transparent"
        border.width: item.visualFocus ? 1 : 0
        border.color: Theme.accentLight
    }
    contentItem: Item {
        Icon {
            id: glyph
            name: item.iconName
            size: 20
            color: item.selected ? Theme.fg : Theme.fgMuted
            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
        }
        Txt {
            text: item.text
            color: item.selected ? Theme.fg : Theme.fgMuted
            font.weight: Font.DemiBold
            anchors { left: glyph.right; leftMargin: 12; verticalCenter: parent.verticalCenter }
        }
        Badge {
            visible: item.badge !== ""
            text: item.badge
            anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
        }
    }
}
