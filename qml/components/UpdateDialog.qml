import QtQuick
import QtQuick.Layouts
import MangaDl

// "Update ready" modal over the current page.
Item {
    id: dialog
    visible: opacity > 0
    opacity: 0
    Behavior on opacity { NumberAnimation { duration: Theme.fade } }
    function open() { opacity = 1; closeButton.forceActiveFocus() }
    function close() { opacity = 0 }

    Keys.onEscapePressed: close()

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        MouseArea { anchors.fill: parent; onClicked: dialog.close() }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 540
        height: content.implicitHeight + 64
        radius: 20
        color: Theme.sheet
        border.color: Theme.track
        border.width: 1
        MouseArea { anchors.fill: parent }   // swallow clicks so the scrim doesn't close it

        ColumnLayout {
            id: content
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 32 }
            spacing: 18
            Eyebrow { text: "Software version" }
            DisplayText { text: "manga-dl v" + AppInfo.version; font.pixelSize: 40 }
            BodyMuted { Layout.fillWidth: true; text: "You're running version " + AppInfo.version + " on Linux. System is up to date." }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                RowLayout {
                    Caption { text: "Status"; Layout.fillWidth: true }
                    Caption { text: "Up to date" }
                }
                Progress { Layout.fillWidth: true; height: 6; value: 1; fill: Theme.success }
            }
            Card {
                Layout.fillWidth: true
                kind: "list"
                padding: 16
                spacing: 8
                Eyebrow { text: "Highlights"; muted: true }
                Txt { text: "• Native Qt 6 Quick client with C++20 core engine"; font.pixelSize: 13; color: Theme.fgStrong }
                Txt { text: "• High-performance QuickJS runtime supporting 38+ sources"; font.pixelSize: 13; color: Theme.fgStrong }
                Txt { text: "• Local-first library, CBZ exports, and Supabase cloud sync"; font.pixelSize: 13; color: Theme.fgStrong }
            }
            CheckRow {
                text: "Check for updates automatically"
                checked: AppSettings.autoCheckUpdates
                onToggled: AppSettings.autoCheckUpdates = checked
                font.pixelSize: 13
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                Btn { id: closeButton; text: "Close"; implicitHeight: 46; fontSize: 14; onClicked: dialog.close() }
            }
        }
    }
}
