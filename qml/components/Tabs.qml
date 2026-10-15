import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Underlined text tabs. model: list of strings; optional counts via `badges`.
Item {
    id: root
    property var model: []
    property var badges: []
    property int currentIndex: 0
    signal activated(int index)

    implicitHeight: 44
    implicitWidth: row.implicitWidth

    HairLine { anchors { left: parent.left; right: parent.right; bottom: parent.bottom } }

    Row {
        id: row
        spacing: 28
        height: parent.height
        Repeater {
            model: root.model
            delegate: AbstractButton {
                id: tab
                required property int index
                required property string modelData
                readonly property bool selected: index === root.currentIndex
                height: row.height
                implicitWidth: label.implicitWidth + (badge.visible ? badge.width + 8 : 0)
                hoverEnabled: true
                Accessible.name: modelData
                onClicked: { root.currentIndex = index; root.activated(index) }
                Txt {
                    id: label
                    text: tab.modelData
                    anchors.verticalCenter: parent.verticalCenter
                    font.weight: tab.selected ? Font.ExtraBold : Font.DemiBold
                    color: tab.selected || tab.hovered ? Theme.fg : Theme.fgSubtle
                }
                Badge {
                    id: badge
                    visible: (root.badges[tab.index] || "") !== ""
                    text: root.badges[tab.index] || ""
                    fill: Theme.accentMuted
                    anchors { left: label.right; leftMargin: 8; verticalCenter: parent.verticalCenter }
                }
                Rectangle {
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                    height: 2
                    color: Theme.accent
                    visible: tab.selected
                }
            }
        }
    }
}
