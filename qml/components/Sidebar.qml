import QtQuick
import QtQuick.Layouts
import MangaDl

// 232px left navigation. `current` is the Nav.sidebarKey of the visible page.
Rectangle {
    id: bar
    property string current: ""
    width: 232
    color: Theme.navBg

    Rectangle { anchors { top: parent.top; bottom: parent.bottom; right: parent.right } width: 1; color: Theme.dividerStrong }

    ColumnLayout {
        anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 20; bottomMargin: 16 }
        spacing: 2

        DisplayText { text: "manga-dl"; font.pixelSize: 24; Layout.leftMargin: 12; Layout.bottomMargin: 18 }

        NavItem { Layout.fillWidth: true; text: "Library"; iconName: "Library"; selected: bar.current === "Library"; onClicked: Nav.go("Library") }
        NavItem {
            Layout.fillWidth: true; text: "Updates"; iconName: "Updates"
            badge: UpdatesModel.totalNewChapters > 0 ? ("" + UpdatesModel.totalNewChapters) : ""
            selected: bar.current === "Updates"
            onClicked: Nav.go("Updates")
        }
        NavItem { Layout.fillWidth: true; text: "History"; iconName: "History"; selected: bar.current === "History"; onClicked: Nav.go("History") }
        NavItem { Layout.fillWidth: true; text: "Browse"; iconName: "Browse"; selected: bar.current === "Browse"; onClicked: Nav.go("Extensions") }
        NavItem {
            Layout.fillWidth: true; text: "Downloads"; iconName: "Download"
            badge: DownloadsModel.queueCount > 0 ? ("" + DownloadsModel.queueCount) : ""
            selected: bar.current === "Downloads"
            onClicked: Nav.go("Downloads")
        }
        NavItem { Layout.fillWidth: true; text: "Stats"; iconName: "Stats"; selected: bar.current === "Stats"; onClicked: Nav.go("Stats") }

        Item { Layout.fillHeight: true }
        NavItem { Layout.fillWidth: true; text: "Notifications"; iconName: "Mail"; selected: bar.current === "Notifications"; onClicked: Nav.go("Notifications") }
        NavItem { Layout.fillWidth: true; text: "Settings"; iconName: "Settings"; selected: bar.current === "Settings"; onClicked: Nav.go("Settings") }
        NavItem { Layout.fillWidth: true; text: "Help"; iconName: "Help"; selected: bar.current === "Help"; onClicked: Nav.go("Help") }

        HairLine { Layout.fillWidth: true; Layout.topMargin: 8; color: Theme.dividerStrong }

        Clickable {
            Layout.fillWidth: true
            implicitHeight: 56
            radius: 10
            Accessible.name: "Open your profile"
            onClicked: Nav.go("Profile")
            Row {
                x: 12; y: 12
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
                    Txt { text: ProfileController.displayName; font.pixelSize: 13; font.weight: Font.Bold }
                    Caption { text: AccountController.signedIn ? "Synced" : "Local profile"; font.pixelSize: 11 }
                }
            }
        }
    }
}
