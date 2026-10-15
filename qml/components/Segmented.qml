import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Segmented control: a dark track with one white selected segment.
//   Segmented { model: ["Dark", "AMOLED", "System"]; currentIndex: 0 }
Rectangle {
    id: root
    property var model: []
    property int currentIndex: 0
    signal activated(int index)

    implicitWidth: row.implicitWidth + 6
    implicitHeight: 38
    radius: 10
    color: Theme.surfaceRaised

    Row {
        id: row
        anchors.centerIn: parent
        Repeater {
            model: root.model
            delegate: Button {
                id: seg
                required property int index
                required property string modelData
                readonly property bool selected: index === root.currentIndex
                text: modelData
                height: 32
                leftPadding: 12; rightPadding: 12
                hoverEnabled: true
                font.family: Theme.body
                font.pixelSize: 12
                font.weight: Font.Bold
                Accessible.role: Accessible.RadioButton
                Accessible.checked: selected
                onClicked: { root.currentIndex = index; root.activated(index) }
                contentItem: Text {
                    text: seg.text; font: seg.font
                    color: seg.selected ? Theme.ink : seg.hovered ? Theme.fg : Theme.fgSoft
                    verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                }
                background: Rectangle {
                    radius: 8
                    color: seg.selected ? Theme.fg : "transparent"
                    border.width: seg.visualFocus ? 1 : 0
                    border.color: Theme.accentLight
                }
            }
        }
    }
}
