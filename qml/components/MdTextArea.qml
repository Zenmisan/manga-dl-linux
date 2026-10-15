import QtQuick
import QtQuick.Controls.Basic as C
import MangaDl

// Multi-line field with the same look as Field.
C.TextArea {
    id: control
    implicitHeight: 110
    padding: 14
    font.family: Theme.body
    font.pixelSize: 14
    color: Theme.fg
    placeholderTextColor: Theme.placeholder
    selectionColor: Theme.accent
    wrapMode: TextEdit.Wrap
    Accessible.name: placeholderText
    background: Rectangle {
        radius: 12
        color: Theme.surface
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.line
    }
}
