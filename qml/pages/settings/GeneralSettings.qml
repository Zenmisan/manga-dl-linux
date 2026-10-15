import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

SettingsSection {
    title: "General"
    caption: "Look, desktop behaviour and your manga-dl server."

    TwoColumns {
        leftCards: [
            SettingsCard {
                heading: "Appearance"
                SettingRow {
                    label: "Theme"; description: "manga-dl is designed dark-first."
                    below: [ Segmented { model: ["Dark", "AMOLED", "System"] } ]
                }
                SettingRow {
                    label: "Accent colour"; description: "Buttons, progress and highlights."
                    below: [
                        Row {
                            spacing: 10
                            Repeater {
                                model: Theme.accents
                                AbstractButton {
                                    required property var modelData
                                    readonly property bool current: Theme.accentName === modelData.name
                                    width: 40; height: 40
                                    Accessible.name: modelData.name
                                    Accessible.checked: current
                                    ToolTip.visible: hovered
                                    ToolTip.text: modelData.name
                                    hoverEnabled: true
                                    onClicked: Theme.setAccent(modelData.name)
                                    background: Rectangle {
                                        radius: 20
                                        color: "transparent"
                                        border.width: parent.current || parent.visualFocus ? 2 : 0
                                        border.color: parent.modelData.main
                                        Rectangle { anchors.centerIn: parent; width: 30; height: 30; radius: 15; color: parent.parent.modelData.main }
                                    }
                                }
                            }
                        }
                    ]
                }
                SettingRow {
                    label: "Ambilight"; description: "Tint the reader and detail pages with the cover's colour."; showDivider: false
                    MdSwitch { checked: true }
                }
            },
            SettingsCard {
                heading: "Connection"
                spacing: 14
                LabeledField {
                    id: serverUrlField
                    label: "Server URL"
                    text: AppSettings.backendUrl || "https://manga-dl.onrender.com"
                    onTextChanged: AppSettings.backendUrl = text
                }
                LabeledField {
                    id: apiKeyField
                    label: "API key"
                    text: AppSettings.apiKey
                    password: true
                    onTextChanged: AppSettings.apiKey = text
                }
                Row {
                    spacing: 12
                    Btn {
                        kind: "ghost"
                        text: AccountController.isBusy ? "Testing..." : "Test connection"
                        enabled: !AccountController.isBusy && serverUrlField.text.length > 0
                        onClicked: {
                            result.visible = false
                            AccountController.testConnection(serverUrlField.text, apiKeyField.text)
                        }
                    }
                    Row {
                        id: result
                        visible: false
                        spacing: 6
                        anchors.verticalCenter: parent.verticalCenter
                        Icon { id: resIcon; name: "Check"; size: 16; color: Theme.successText }
                        Txt { id: resText; text: "Connected"; font.pixelSize: 13; font.weight: Font.DemiBold; color: Theme.successText }
                    }
                }
                Connections {
                    target: AccountController
                    function onConnectionTestResult(success, latencyMs, message) {
                        result.visible = true
                        if (success) {
                            resIcon.name = "Check"
                            resIcon.color = Theme.successText
                            resText.text = "Connected · " + latencyMs + " ms"
                            resText.color = Theme.successText
                            Nav.toast("Connected in " + latencyMs + " ms")
                        } else {
                            resIcon.name = "Close"
                            resIcon.color = Theme.errorText
                            resText.text = message !== "" ? ("Failed (" + message + ")") : "Failed to connect"
                            resText.color = Theme.errorText
                            Nav.toast("Connection test failed: " + message)
                        }
                    }
                }
            }
        ]
        rightCards: [
            SettingsCard {
                heading: "Desktop"
                SettingRow { label: "Launch on startup"; description: "Start manga-dl when you log in."; MdSwitch {} }
                SettingRow { label: "Keep running in tray"; description: "Closing the window keeps downloads going."; MdSwitch { checked: true } }
                SettingRow { label: "Desktop notifications"; description: "New chapters and finished downloads."; MdSwitch { checked: true } }
                SettingRow { label: "Discord Rich Presence"; description: "Show what you're reading on Discord."; showDivider: false; MdSwitch {} }
            },
            SettingsCard {
                heading: "Behaviour"
                SettingRow { label: "Incognito mode"; description: "Pause history and tracker sync while it's on."; MdSwitch {} }
                SettingRow { label: "Auto-download new chapters"; description: "For manga in your library."; showDivider: false; MdSwitch { checked: true } }
            }
        ]
    }
}
