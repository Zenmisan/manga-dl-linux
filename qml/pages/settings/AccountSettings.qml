import QtQuick
import QtQuick.Layouts
import MangaDl

SettingsSection {
    id: section
    title: "Account"
    caption: "Your profile, sync and sign-in."
    property bool confirmSignOut: false

    // Profile summary
    Card {
        width: parent.width
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            Initial {
                size: 64; round: true; color: "#3A1518"
                text: (AccountController.displayName || AccountController.email || "U").slice(0, 1).toUpperCase()
                fontSize: 24
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Txt {
                    Layout.fillWidth: true
                    text: AccountController.displayName !== "" ? AccountController.displayName : (AccountController.signedIn ? AccountController.email : "Guest User")
                    font.pixelSize: 18; font.weight: Font.ExtraBold
                }
                Caption {
                    Layout.fillWidth: true
                    text: AccountController.signedIn ? ("@" + (AccountController.displayName || "user") + " · " + AccountController.email) : "Not signed in · Local Mode"
                    font.pixelSize: 13
                }
            }
            Btn { kind: "ghost"; text: "View profile"; onClicked: Nav.go("Profile") }
        }
    }

    TwoColumns {
        leftCards: [
            SettingsCard {
                heading: "Public profile"
                spacing: 14
                LabeledField {
                    id: displayNameField
                    label: "Display name"
                    text: AccountController.displayName
                    placeholderText: "Display name"
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    FieldLabel { text: "Bio" }
                    MdTextArea {
                        id: bioField
                        text: AccountController.bio
                        Layout.fillWidth: true; Layout.preferredHeight: 96
                        placeholderText: "A short line about what you like to read"
                    }
                }
                SettingRow {
                    label: "Show reading activity"; description: "Let others see what you're reading and your stats."; showDivider: false
                    MdSwitch {
                        checked: AppSettings.showReadingActivity
                        onToggled: AppSettings.showReadingActivity = checked
                    }
                }
                Btn {
                    text: "Save changes"
                    onClicked: {
                        AccountController.saveProfile(displayNameField.text, bioField.text)
                        Nav.toast("Profile saved")
                    }
                }
            }
        ]
        rightCards: [
            Card {
                width: parent.width
                color: AccountController.signedIn ? Theme.successWash : Theme.cardBg
                border.color: AccountController.signedIn ? Theme.successLine : Theme.dividerStrong
                spacing: 10
                RowLayout {
                    spacing: 10
                    Rectangle {
                        width: 36; height: 36; radius: 10
                        color: AccountController.signedIn ? Theme.successSoft : Theme.fgFaint
                        Icon { anchors.centerIn: parent; name: AccountController.signedIn ? "CloudCheck" : "Cloud"; size: 18; color: AccountController.signedIn ? Theme.successText : Theme.fgMuted }
                    }
                    ColumnLayout {
                        spacing: 0
                        Txt {
                            Layout.fillWidth: true
                            text: AccountController.signedIn ? "Cloud sync is on" : "Cloud sync is off"
                            font.pixelSize: 15; font.weight: Font.ExtraBold
                        }
                        Caption {
                            Layout.fillWidth: true
                            text: AccountController.signedIn ? ("Last synced " + AccountController.lastSyncedText) : "Sign in to sync your library across devices"
                            color: AccountController.signedIn ? Theme.successText : Theme.fgMuted
                        }
                    }
                }
                BodyMuted {
                    Layout.fillWidth: true
                    text: "Library, progress, categories and settings follow you across Android, Windows and Linux."
                }
                Row {
                    spacing: 8
                    Btn {
                        kind: "success"
                        text: AccountController.isSyncing ? "Syncing..." : "Sync now"
                        enabled: AccountController.signedIn && !AccountController.isSyncing
                        onClicked: {
                            AccountController.syncNow()
                            Nav.toast("Syncing with cloud...")
                        }
                    }
                    Btn {
                        kind: "ghost"
                        text: AccountController.signedIn ? "Manage devices" : "Sign in"
                        onClicked: {
                            if (!AccountController.signedIn) {
                                Nav.go("Login")
                            } else {
                                Nav.toast("All your active devices stay automatically in sync.")
                            }
                        }
                    }
                }
            },
            SettingsCard {
                heading: "Sign-in"
                visible: AccountController.signedIn
                SettingRow {
                    label: "Email"
                    description: AccountController.email || "Not configured"
                }
                SettingRow {
                    label: "Account ID"
                    description: AccountController.userId.length > 12 ? AccountController.userId.slice(0, 12) + "..." : (AccountController.userId || "—")
                }
                SettingRow {
                    label: "Sign out"; description: "Downloads stay on this computer."; showDivider: false
                    Btn { kind: "danger"; size: "small"; text: "Sign out"; onClicked: section.confirmSignOut = true }
                }
                // Inline confirmation
                Rectangle {
                    visible: section.confirmSignOut
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    implicitHeight: confirm.implicitHeight + 28
                    radius: 12
                    color: Theme.errorSoft
                    border.color: Theme.errorBorder
                    ColumnLayout {
                        id: confirm
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
                        spacing: 10
                        Txt { Layout.fillWidth: true; text: "Sign out of manga-dl on this computer?"; font.weight: Font.Bold }
                        Row {
                            spacing: 8
                            Btn {
                                kind: "danger"; size: "small"; text: "Sign out"
                                onClicked: {
                                    AccountController.signOut()
                                    section.confirmSignOut = false
                                    Nav.toast("Signed out")
                                    Nav.go("Login")
                                }
                            }
                            Btn { kind: "ghost"; size: "small"; text: "Cancel"; onClicked: section.confirmSignOut = false }
                        }
                    }
                }
            },
            SettingsCard {
                heading: "Sign-in"
                visible: !AccountController.signedIn
                SettingRow {
                    label: "Not signed in"
                    description: "Sign in to keep your library in sync with your account."
                    showDivider: false
                    Btn {
                        size: "small"
                        text: "Sign in"
                        onClicked: Nav.go("Login")
                    }
                }
            }
        ]
    }
}
