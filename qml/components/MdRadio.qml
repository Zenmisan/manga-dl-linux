import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Round radio button with a label.
RadioButton {
    id: control
    font.family: Theme.body
    font.pixelSize: 14
    spacing: 10
    padding: 0
    implicitHeight: 34
    indicator: Rectangle {
        width: 20; height: 20; radius: 10
        anchors.verticalCenter: parent.verticalCenter
        color: "transparent"
        border.width: control.checked ? 6 : 1.5
        border.color: control.checked ? Theme.accent : control.visualFocus ? Theme.accentLight : Theme.lineStrong
    }
    contentItem: Text {
        text: control.text
        font: control.font
        color: Theme.fgStrong
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
    }
}
