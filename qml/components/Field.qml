import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// 48px text field. Set password: true for a masked field, error: true for the red state.
TextField {
    id: control
    property bool password: false
    property bool error: false
    property bool large: true

    implicitHeight: large ? 48 : 42
    leftPadding: 14
    rightPadding: 14
    font.family: Theme.body
    font.pixelSize: large ? 15 : 14
    color: Theme.fg
    placeholderTextColor: Theme.placeholder
    selectionColor: Theme.accent
    selectedTextColor: Theme.white
    echoMode: password ? TextInput.Password : TextInput.Normal
    verticalAlignment: TextInput.AlignVCenter
    Accessible.name: placeholderText

    Keys.onReturnPressed: (event) => { accepted(); }
    Keys.onEnterPressed: (event) => { accepted(); }

    background: Rectangle {
        radius: 12
        color: control.error ? Theme.accentFaint : Theme.surface
        border.width: 1
        border.color: control.error ? Theme.errorBorder
                    : control.activeFocus ? Theme.accent
                    : control.hovered ? Theme.lineStrong : Theme.line
    }
}
