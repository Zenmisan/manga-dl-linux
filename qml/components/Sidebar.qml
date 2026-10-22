import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Window
import MangaDl

// Responsive left navigation: collapses to 68px icon rail on narrow windows / Wayland tiling splits.
Rectangle {
    id: bar
    property string current: ""
    readonly property bool isCompact: Window.window ? (Window.window.width < 960) : false
    width: isCompact ? 68 : 232
    color: Theme.navBg

    Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.InOutQuad } }

    Rectangle { anchors { top: parent.top; bottom: parent.bottom; right: parent.right } width: 1; color: Theme.dividerStrong }

    ColumnLayout {
        anchors { fill: parent; leftMargin: bar.isCompact ? 8 : 12; rightMargin: bar.isCompact ? 8 : 12; topMargin: 20; bottomMargin: 16 }
        spacing: 2

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            Layout.bottomMargin: 14

            DisplayText {
                visible: !bar.isCompact
                text: "manga-dl"
                font.pixelSize: 24
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                visible: bar.isCompact
                anchors.centerIn: parent
                width: 32; height: 32; radius: 8
                color: Theme.accent
                Txt {
                    anchors.centerIn: parent
                    text: "M"
                    font.pixelSize: 18
                    font.weight: Font.Black
                    color: Theme.white
                }
            }
        }

        NavItem { Layout.fillWidth: true; compact: bar.isCompact; text: "Library"; iconName: "Library"; selected: bar.current === "Library"; onClicked: Nav.go("Library") }
        NavItem {
            Layout.fillWidth: true; compact: bar.isCompact; text: "Updates"; iconName: "Updates"
            badge: UpdatesModel.totalNewChapters > 0 ? ("" + UpdatesModel.totalNewChapters) : ""
            selected: bar.current === "Updates"
            onClicked: Nav.go("Updates")
        }
        NavItem { Layout.fillWidth: true; compact: bar.isCompact; text: "History"; iconName: "History"; selected: bar.current === "History"; onClicked: Nav.go("History") }
        NavItem { Layout.fillWidth: true; compact: bar.isCompact; text: "Browse"; iconName: "Browse"; selected: bar.current === "Browse"; onClicked: Nav.go("Extensions") }
        NavItem {
            Layout.fillWidth: true; compact: bar.isCompact; text: "Downloads"; iconName: "Download"
            badge: DownloadsModel.queueCount > 0 ? ("" + DownloadsModel.queueCount) : ""
            selected: bar.current === "Downloads"
            onClicked: Nav.go("Downloads")
        }
        NavItem { Layout.fillWidth: true; compact: bar.isCompact; text: "Stats"; iconName: "Stats"; selected: bar.current === "Stats"; onClicked: Nav.go("Stats") }

        Item { Layout.fillHeight: true }
        NavItem { Layout.fillWidth: true; compact: bar.isCompact; text: "Notifications"; iconName: "Mail"; selected: bar.current === "Notifications"; onClicked: Nav.go("Notifications") }
        NavItem { Layout.fillWidth: true; compact: bar.isCompact; text: "Settings"; iconName: "Settings"; selected: bar.current === "Settings"; onClicked: Nav.go("Settings") }
        NavItem { Layout.fillWidth: true; compact: bar.isCompact; text: "Help"; iconName: "Help"; selected: bar.current === "Help"; onClicked: Nav.go("Help") }

        HairLine { Layout.fillWidth: true; Layout.topMargin: 8; color: Theme.dividerStrong }

        Clickable {
            Layout.fillWidth: true
            implicitHeight: 56
            radius: 10
            Accessible.name: "Open your profile"
            onClicked: Nav.go("Profile")
            ToolTip.visible: hovered && bar.isCompact
            ToolTip.text: ProfileController.displayName.length > 0 ? ProfileController.displayName : "Profile"

            Item {
                anchors.fill: parent
                Row {
                    visible: !bar.isCompact
                    anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                    spacing: 10
                    Initial {
                        size: 32
                        round: true
                        text: ProfileController.displayName.length > 0 ? ProfileController.displayName.substring(0, 1).toUpperCase() : "U"
                        color: Theme.accentMuted
                        fontSize: 13
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        Txt {
                            text: ProfileController.displayName.length > 0 ? ProfileController.displayName : "User"
                            font.pixelSize: 13
                            font.weight: Font.Bold
                            elide: Text.ElideRight
                            width: 140
                        }
                        Caption { text: AccountController.signedIn ? "Synced" : "Local profile"; font.pixelSize: 11 }
                    }
                }
                Initial {
                    visible: bar.isCompact
                    anchors.centerIn: parent
                    size: 32
                    round: true
                    text: ProfileController.displayName.length > 0 ? ProfileController.displayName.substring(0, 1).toUpperCase() : "U"
                    color: Theme.accentMuted
                    fontSize: 13
                }
            }
        }
    }
}
