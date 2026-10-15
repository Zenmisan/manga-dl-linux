import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Checkbox with a label, accent box when checked.
CheckBox {
    id: control
    font.family: Theme.body
    font.pixelSize: 14
    spacing: 10
    padding: 0
    implicitHeight: 32
    indicator: Rectangle {
        width: 20; height: 20; radius: 6
        x: control.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        color: control.checked ? Theme.accent : "transparent"
        border.width: control.checked ? 0 : 1.5
        border.color: control.visualFocus ? Theme.accentLight : Theme.lineStrong
        Icon { anchors.centerIn: parent; name: "Check"; size: 14; color: Theme.white; visible: control.checked }
    }
    contentItem: Text {
        text: control.text
        font: control.font
        color: Theme.fgStrong
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }
}
