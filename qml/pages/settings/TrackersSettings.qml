import QtQuick
import QtQuick.Layouts
import MangaDl

SettingsSection {
    title: "Trackers"
    caption: "Keep your lists on other sites in step with what you read here."

    Connections {
        target: TrackerController
        function onTrackerConnected(trackerName, username) {
            Nav.toast("Connected to " + trackerName + (username ? " as @" + username : ""))
        }
        function onErrorMessageChanged() {
            if (TrackerController.errorMessage !== "") {
                Nav.toast(TrackerController.errorMessage)
            }
        }
    }

    AutoGrid {
        width: parent.width
        minItemWidth: 300
        maxColumns: 3
        spacing: 16
        model: TrackerController.trackers
        delegate: Card {
            required property var modelData
            width: parent.cellWidth
            height: 176
            border.color: modelData.connected ? Theme.successLine : Theme.dividerStrong
            spacing: 12
            RowLayout {
                spacing: 12
                Initial { size: 44; radius: 12; color: modelData.color; text: modelData.short; fontSize: 13 }
                ColumnLayout {
                    spacing: 0
                    Txt { Layout.fillWidth: true; text: modelData.name; font.pixelSize: 16; font.weight: Font.ExtraBold }
                    Txt {
                        text: modelData.connected ? ("Signed in as @" + (modelData.username || "user")) : "Not connected"
                        font.pixelSize: 12; font.weight: Font.DemiBold
                        color: modelData.connected ? Theme.successText : Theme.fgSubtle
                    }
                }
            }
            Txt { Layout.fillWidth: true; text: modelData.description; font.pixelSize: 13; color: Theme.fgMuted; wrapMode: Text.Wrap }
            Btn {
                size: "small"
                kind: modelData.connected ? "ghost" : "primary"
                text: modelData.connected ? "Log Out" : "Connect"
                enabled: !TrackerController.isConnecting
                onClicked: {
                    if (modelData.connected) {
                        TrackerController.disconnectTracker(modelData.id)
                        Nav.toast("Disconnected from " + modelData.name)
                    } else {
                        TrackerController.connectTracker(modelData.id)
                        Nav.toast("Opening browser for " + modelData.name + " authorization...")
                    }
                }
            }
        }
    }

    SettingsCard {
        width: Math.min(parent.width, 620)
        heading: "Sync"
        SettingRow {
            label: "Update progress after reading"; description: "Send the chapter number when you finish a chapter."
            MdSwitch {
                checked: AppSettings.autoSyncTrackers
                onToggled: AppSettings.autoSyncTrackers = checked
            }
        }
        SettingRow {
            label: "Mark as reading on first chapter"
            MdSwitch {
                checked: AppSettings.markTrackerReadingOnFirstChapter
                onToggled: AppSettings.markTrackerReadingOnFirstChapter = checked
            }
        }
        SettingRow {
            label: "Mark as completed on last chapter"
            MdSwitch {
                checked: AppSettings.markTrackerCompletedOnFinish
                onToggled: AppSettings.markTrackerCompletedOnFinish = checked
            }
        }
        SettingRow {
            label: "Pull progress from trackers"; description: "Mark chapters read here when a tracker is ahead."; showDivider: false
            MdSwitch {
                checked: AppSettings.pullProgressFromTrackers
                onToggled: AppSettings.pullProgressFromTrackers = checked
            }
        }
    }
}
