import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Sidebar entry: icon, label, optional badge. Lit when `selected`.
AbstractButton {
    id: item
    property string iconName
    property string badge: ""
    property bool selected: false
    property bool compact: false
    text: ""
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    implicitHeight: 44
    implicitWidth: compact ? 44 : 208
    Accessible.role: Accessible.PageTab
    Accessible.name: badge ? text + ", " + badge + " new" : text
    Accessible.selected: selected

    ToolTip.visible: item.hovered && item.compact
    ToolTip.delay: 400
    ToolTip.text: item.badge !== "" ? (item.text + " (" + item.badge + ")") : item.text

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
            anchors.centerIn: item.compact ? parent : undefined
            anchors.left: item.compact ? undefined : parent.left
            anchors.leftMargin: item.compact ? 0 : 12
            anchors.verticalCenter: parent.verticalCenter
        }
        Txt {
            visible: !item.compact
            text: item.text
            color: item.selected ? Theme.fg : Theme.fgMuted
            font.weight: Font.DemiBold
            anchors {
                left: glyph.right
                leftMargin: 12
                verticalCenter: parent.verticalCenter
                right: badgeItem.visible ? badgeItem.left : parent.right
                rightMargin: 8
            }
            elide: Text.ElideRight
        }
        Badge {
            id: badgeItem
            visible: item.badge !== ""
            text: item.badge
            anchors.right: parent.right
            anchors.rightMargin: item.compact ? 4 : 12
            anchors.top: item.compact ? parent.top : undefined
            anchors.topMargin: item.compact ? 4 : 0
            anchors.verticalCenter: item.compact ? undefined : parent.verticalCenter
        }
    }
}
