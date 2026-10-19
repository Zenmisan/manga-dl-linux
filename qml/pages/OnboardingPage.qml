import QtQuick
import QtQuick.Layouts
import MangaDl

// First run: clean welcome & account selection. All scrapers run locally.
Item {
    id: root

    // Left Hero
    Item {
        id: hero
        width: parent.width / 2
        height: parent.height
        clip: true
        Bloom { anchors.fill: parent }
        Rectangle { anchors { top: parent.top; bottom: parent.bottom; right: parent.right } width: 1; color: Theme.dividerStrong }

        ColumnLayout {
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 64; topMargin: 72 }
            spacing: 20
            Eyebrow { text: "Welcome to manga-dl" }
            DisplayText { Layout.fillWidth: true; text: "Your manga, everywhere."; font.pixelSize: 84; wrapMode: Text.Wrap }
            BodyMuted {
                Layout.fillWidth: true
                font.pixelSize: 16; lineHeight: 26
                text: "A native, high-performance desktop manga and light novel reader. 100% local scrapers, zero clutter, and seamless reading."
            }
        }

        // Feature Highlights
        Column {
            anchors { left: parent.left; bottom: parent.bottom; leftMargin: 64; bottomMargin: 72 }
            spacing: 16
            Row {
                spacing: 14
                Rectangle {
                    width: 32; height: 32; radius: 16; color: Theme.accentMuted
                    Icon { anchors.centerIn: parent; name: "Check"; size: 16; color: Theme.accentLight }
                }
                Txt { text: "100% Local Engine & Direct Scrapers"; font.pixelSize: 14; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
            }
            Row {
                spacing: 14
                Rectangle {
                    width: 32; height: 32; radius: 16; color: Theme.accentMuted
                    Icon { anchors.centerIn: parent; name: "Download"; size: 16; color: Theme.accentLight }
                }
                Txt { text: "Offline Reading & CBZ Archiving"; font.pixelSize: 14; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
            }
            Row {
                spacing: 14
                Rectangle {
                    width: 32; height: 32; radius: 16; color: Theme.accentMuted
                    Icon { anchors.centerIn: parent; name: "Sync"; size: 16; color: Theme.accentLight }
                }
                Txt { text: "Optional Cloud Backup & Progress Sync"; font.pixelSize: 14; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
            }
        }
    }

    // Right Action Panel
    Item {
        x: hero.width
        width: parent.width - hero.width
        height: parent.height

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(420, parent.width - 96)
            spacing: 20

            DisplayText { text: "Get Started"; font.pixelSize: 36; font.weight: Font.Black }
            BodyMuted {
                Layout.fillWidth: true
                font.pixelSize: 14; lineHeight: 22
                text: "Jump straight into your local library, or sign in to sync your progress across devices."
            }

            Item { height: 12 }

            // Primary: Start without account
            Btn {
                Layout.fillWidth: true; size: "large"
                text: "Start Reading (Offline / Local)"
                fontSize: 15
                onClicked: {
                    AppSettings.onboardingCompleted = true
                    Nav.home()
                }
            }

            // Secondary: Sign In
            Btn {
                Layout.fillWidth: true; kind: "ghost"; size: "large"
                text: "Sign In to Sync"
                fontSize: 15
                onClicked: {
                    AppSettings.onboardingCompleted = true
                    Nav.go("Login")
                }
            }

            // Tertiary: Register
            Btn {
                Layout.fillWidth: true; kind: "ghost"; size: "large"
                text: "Create an Account"
                fontSize: 15
                onClicked: {
                    AppSettings.onboardingCompleted = true
                    Nav.go("Register")
                }
            }

            Item { height: 8 }

            Txt {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: "You can change servers, cloud sync, and accounts anytime in Settings."
                color: Theme.fgSubtle
                font.pixelSize: 12
            }
        }
    }
}
