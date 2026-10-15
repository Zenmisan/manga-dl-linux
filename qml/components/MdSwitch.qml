import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// 44×26 on/off switch, accent when on.
AbstractButton {
    id: control
    checkable: true
    implicitWidth: 44
    implicitHeight: 26
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1 : 0.4

    background: Rectangle {
        radius: 13
        color: control.checked ? Theme.accent : Theme.switchOff
        border.width: control.visualFocus ? 1 : 0
        border.color: Theme.accentLight
        Rectangle {
            width: 20; height: 20; radius: 10
            color: Theme.white
            anchors.verticalCenter: parent.verticalCenter
            x: control.checked ? parent.width - width - 3 : 3
        }
    }
}
