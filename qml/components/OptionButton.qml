import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Big selectable option tile (reader mode grid). Use in a ButtonGroup.
Button {
    id: control
    checkable: true
    hoverEnabled: true
    implicitHeight: 44
    font.family: Theme.body
    font.pixelSize: 13
    font.weight: checked ? Font.Bold : Font.DemiBold
    contentItem: Text {
        text: control.text; font: control.font
        color: control.checked ? Theme.fg : Theme.fgStrong
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 10
        color: control.checked ? Theme.accentMuted : control.hovered ? Theme.hover : "transparent"
        border.width: 1
        border.color: control.checked || control.visualFocus ? Theme.accent : Theme.line
    }
}
