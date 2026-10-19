import QtQuick
import QtQuick.Layouts
import MangaDl

SettingsSection {
    title: "Advanced"
    caption: "Media servers, network tuning, security lock, and backend connections."

    TwoColumns {
        leftCards: [
            SettingsCard {
                heading: "Media Servers"
                spacing: 12

                // Komga Server
                SettingRow {
                    label: "Komga Server"
                    description: "Connect to your personal Komga comic & manga server."
                    MdSwitch {
                        checked: AppSettings.komgaEnabled
                        onToggled: AppSettings.komgaEnabled = checked
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: AppSettings.komgaEnabled
                    spacing: 10
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6

                    LabeledField {
                        label: "Komga URL"
                        placeholderText: "http://localhost:25600"
                        text: AppSettings.komgaUrl
                        onTextEdited: AppSettings.komgaUrl = text
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        LabeledField {
                            Layout.fillWidth: true
                            label: "Username"
                            placeholderText: "user@domain.com"
                            text: AppSettings.komgaUsername
                            onTextEdited: AppSettings.komgaUsername = text
                        }
                        LabeledField {
                            Layout.fillWidth: true
                            label: "Password"
                            password: true
                            placeholderText: "••••••••"
                            text: AppSettings.komgaPassword
                            onTextEdited: AppSettings.komgaPassword = text
                        }
                    }
                }

                SettingRow {
                    label: "Suwayomi / Tachidesk"
                    description: "Self-hosted Tachiyomi server for cross-device sync."
                    MdSwitch {
                        checked: AppSettings.suwayomiEnabled
                        onToggled: AppSettings.suwayomiEnabled = checked
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: AppSettings.suwayomiEnabled
                    spacing: 10
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6

                    LabeledField {
                        label: "Suwayomi URL"
                        placeholderText: "http://localhost:4567"
                        text: AppSettings.suwayomiUrl
                        onTextEdited: AppSettings.suwayomiUrl = text
                    }

                    LabeledField {
                        label: "API Key (optional)"
                        password: true
                        placeholderText: "API key or token"
                        text: AppSettings.suwayomiApiKey
                        onTextEdited: AppSettings.suwayomiApiKey = text
                    }
                }
            },

            SettingsCard {
                heading: "Security & App Lock"
                SettingRow {
                    label: "Enable app lock"
                    description: "Require a PIN when starting manga-dl or after waking."
                    MdSwitch {
                        checked: AppSettings.appLockEnabled
                        onToggled: AppSettings.appLockEnabled = checked
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: AppSettings.appLockEnabled
                    spacing: 8
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6

                    LabeledField {
                        label: "Security PIN"
                        password: true
                        placeholderText: "4-digit or alphanumeric PIN"
                        text: AppSettings.appLockPin
                        onTextEdited: AppSettings.appLockPin = text
                    }
                }
            }
        ]

        rightCards: [
            SettingsCard {
                heading: "Network & Download Engine"

                SettingRow {
                    label: "Parallel downloads"
                    description: "Simultaneous chapter downloads."
                    MdCombo {
                        model: ["1 (Safe)", "2 (Recommended)", "4 (Fast)", "8 (High throughput)"]
                        currentIndex: {
                            const p = AppSettings.parallelDownloads
                            if (p <= 1) return 0
                            if (p <= 2) return 1
                            if (p <= 4) return 2
                            return 3
                        }
                        Accessible.name: "Parallel downloads"
                        onActivated: (index) => {
                            const vals = [1, 2, 4, 8]
                            AppSettings.parallelDownloads = vals[index]
                        }
                    }
                }

                SettingRow {
                    label: "Download archive format"
                    description: "Package downloaded chapters into standard comic archives."
                    Segmented {
                        model: ["CBZ Archive", "Image Folder"]
                        currentIndex: AppSettings.downloadFormat === "folder" ? 1 : 0
                        onActivated: (idx) => {
                            AppSettings.downloadFormat = idx === 1 ? "folder" : "cbz"
                        }
                    }
                }
            },

            SettingsCard {
                heading: "Cloud Sync & Supabase Backend"

                SettingRow {
                    label: "Supabase cloud sync"
                    description: AppSettings.hasSupabase ? "Connected to Supabase project" : "Default cloud endpoint active"
                    showDivider: false
                    Btn {
                        kind: "ghost"
                        size: "small"
                        text: "Test"
                        onClicked: {
                            AccountController.testConnection(AppSettings.backendUrl, AppSettings.apiKey)
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    Layout.topMargin: 4

                    LabeledField {
                        label: "Backend URL"
                        placeholderText: "https://your-backend.example.com"
                        text: AppSettings.backendUrl
                        onTextEdited: AppSettings.backendUrl = text
                    }

                    LabeledField {
                        label: "Supabase Project URL"
                        placeholderText: "https://xyz.supabase.co"
                        text: AppSettings.supabaseUrl
                        onTextEdited: AppSettings.supabaseUrl = text
                    }
                }
            }
        ]
    }
}
