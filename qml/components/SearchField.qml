import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Rounded search box with a leading magnifier. large: 52px hero version (Search page).
TextField {
    id: control
    property bool large: false
    signal submitted(string query)

    implicitHeight: large ? 52 : 42
    leftPadding: large ? 48 : 40
    rightPadding: 14
    font.family: Theme.body
    font.pixelSize: large ? 16 : 14
    color: Theme.fg
    placeholderTextColor: Theme.placeholder
    selectionColor: Theme.accent
    verticalAlignment: TextInput.AlignVCenter
    Accessible.name: placeholderText
    onAccepted: submitted(text)

    background: Rectangle {
        radius: control.large ? 14 : 12
        color: Theme.surface
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.line
        Icon {
            name: "Search"
            size: control.large ? 20 : 18
            color: Theme.fgSubtle
            anchors { left: parent.left; leftMargin: control.large ? 16 : 14; verticalCenter: parent.verticalCenter }
        }
    }
}
