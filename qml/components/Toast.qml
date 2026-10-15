import QtQuick
import QtQuick.Layouts
import MangaDl

// Bottom-right confirmation toast. show(text) fades it in for 3.5 s.
Rectangle {
    id: toast
    width: 340
    implicitHeight: 60
    radius: 14
    color: Theme.toast
    border.color: Theme.track
    border.width: 1
    opacity: 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: Theme.fade } }

    function show(text) { label.text = text; opacity = 1; timer.restart() }

    Timer { id: timer; interval: 3500; onTriggered: toast.opacity = 0 }

    RowLayout {
        anchors { fill: parent; leftMargin: 16; rightMargin: 12 }
        spacing: 12
        Rectangle {
            width: 32; height: 32; radius: 16; color: Theme.successSoft
            Icon { anchors.centerIn: parent; name: "Check"; size: 16; color: Theme.successText }
        }
        Txt { id: label; Layout.fillWidth: true; font.pixelSize: 13; font.weight: Font.DemiBold; elide: Text.ElideRight }
        Btn { kind: "link"; text: "Open"; fontSize: 12; onClicked: toast.opacity = 0 }
    }
}
