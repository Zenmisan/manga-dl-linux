import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Pill filter chip. Checked = white fill with dark text.
// Put chips in a ButtonGroup (or set autoExclusive) for single choice.
Button {
    id: control
    property bool small: false
    checkable: true
    hoverEnabled: true
    implicitHeight: small ? 34 : 36
    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    leftPadding: small ? 14 : 16
    rightPadding: leftPadding
    font.family: Theme.body
    font.pixelSize: small ? 12 : 13
    font.weight: checked ? Font.Bold : Font.DemiBold

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.checked ? Theme.ink : Theme.fgSoft
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
    background: Rectangle {
        radius: height / 2
        color: control.checked ? Theme.fg : control.hovered ? Theme.hover : "transparent"
        border.width: control.checked ? 0 : 1
        border.color: control.visualFocus ? Theme.accentLight : Theme.line
    }
}
