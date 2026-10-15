import QtQuick
import QtQuick.Layouts
import MangaDl

// First run, step 2 of 3: optional backend connection.
Item {
    // Hero
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
            DisplayText { Layout.fillWidth: true; text: "Your manga, everywhere."; font.pixelSize: 104; wrapMode: Text.Wrap }
        }

        // Steps
        Column {
            anchors { left: parent.left; bottom: parent.bottom; leftMargin: 64; bottomMargin: 72 }
            spacing: 14
            Row {
                spacing: 14
                Rectangle {
                    width: 32; height: 32; radius: 16; color: Theme.successSoft
                    Icon { anchors.centerIn: parent; name: "Check"; size: 16; color: Theme.successText }
                }
                Txt { text: "Welcome"; font.pixelSize: 15; font.weight: Font.DemiBold; color: Theme.fgMuted; anchors.verticalCenter: parent.verticalCenter }
            }
            Row {
                spacing: 14
                Rectangle {
                    width: 32; height: 32; radius: 16; color: Theme.accent
                    Txt { anchors.centerIn: parent; text: "2"; font.pixelSize: 13; font.weight: Font.ExtraBold; color: Theme.white }
                }
                Txt { text: "Connect to backend"; font.pixelSize: 15; font.weight: Font.ExtraBold; anchors.verticalCenter: parent.verticalCenter }
            }
            Row {
                spacing: 14
                Rectangle {
                    width: 32; height: 32; radius: 16; color: "transparent"; border.color: Theme.switchOff
                    Txt { anchors.centerIn: parent; text: "3"; font.pixelSize: 13; font.weight: Font.DemiBold; color: Theme.fgSubtle }
                }
                Txt { text: "Library folder & account"; font.pixelSize: 15; font.weight: Font.DemiBold; color: Theme.fgSubtle; anchors.verticalCenter: parent.verticalCenter }
            }
        }
    }

    // Form
    Item {
        x: hero.width
        width: parent.width - hero.width
        height: parent.height

        Connections {
            target: AccountController
            function onConnectionTestResult(success, latencyMs, message) {
                connected.visible = true
                if (success) {
                    connIcon.name = "Check"
                    connIcon.color = Theme.successText
                    connStatus.text = "Connected (" + latencyMs + " ms)"
                    connStatus.color = Theme.successText
                    Nav.toast("Connected to backend in " + latencyMs + " ms")
                } else {
                    connIcon.name = "Close"
                    connIcon.color = Theme.errorText
                    connStatus.text = message !== "" ? message : "Failed to connect"
                    connStatus.color = Theme.errorText
                    Nav.toast("Connection failed: " + message)
                }
            }
        }

        ColumnLayout {
            anchors { left: parent.left; top: parent.top; leftMargin: 80; topMargin: 72 }
            width: Math.min(460, parent.width - 120)
            spacing: 24
            Txt { Layout.fillWidth: true; text: "Connect to backend"; font.pixelSize: 28; font.weight: Font.Black }
            BodyMuted {
                Layout.fillWidth: true
                font.pixelSize: 15; lineHeight: 23
                text: "The desktop app reads sources by itself. Add a backend only if you want sync and cloud backups. You can change this later in Settings."
            }
            LabeledField { id: backendUrlField; label: "Backend URL"; text: AppSettings.backendUrl || "https://gyivwfweldwvzccbpgoz.supabase.co" }
            LabeledField { id: backendKeyField; label: "API key (optional)"; text: AppSettings.apiKey; password: true }
            RowLayout {
                spacing: 12
                Btn {
                    kind: "ghost"; text: AccountController.isBusy ? "Testing..." : "Test Connection"; implicitHeight: 44
                    enabled: !AccountController.isBusy && backendUrlField.text.length > 0
                    onClicked: {
                        connected.visible = false
                        AccountController.testConnection(backendUrlField.text, backendKeyField.text)
                    }
                }
                Row {
                    id: connected
                    visible: false
                    spacing: 8
                    Icon { id: connIcon; name: "Check"; size: 16; color: Theme.successText; anchors.verticalCenter: parent.verticalCenter }
                    Txt { id: connStatus; text: "Connected"; font.pixelSize: 13; font.weight: Font.Bold; color: Theme.successText }
                }
            }
        }

        RowLayout {
            anchors { left: parent.left; bottom: parent.bottom; leftMargin: 80; bottomMargin: 72 }
            width: Math.min(460, parent.width - 120)
            spacing: 12
            Btn { kind: "link"; text: "Skip — Read Offline"; fontSize: 14; onClicked: Nav.home() }
            Item { Layout.fillWidth: true }
            Btn { kind: "ghost"; size: "large"; text: "Back"; fontSize: 14; onClicked: Nav.back() }
            Btn {
                size: "large"; text: "Continue"; fontSize: 14; leftPadding: 28; rightPadding: 28
                onClicked: {
                    AppSettings.backendUrl = backendUrlField.text
                    AppSettings.apiKey = backendKeyField.text
                    Nav.go("Login")
                }
            }
        }
    }
}
