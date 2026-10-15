import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import MangaDl

SettingsSection {
    title: "System & backup"
    caption: "Storage, backups, and data synchronization."

    FolderDialog {
        id: downloadFolderDialog
        title: "Select download directory"
        currentFolder: "file://" + SystemController.downloadLocation
        onAccepted: {
            var path = selectedFolder.toString()
            if (path.startsWith("file://")) {
                path = path.substring(7)
            }
            SystemController.downloadLocation = path
            Nav.toast("Download location updated")
        }
    }

    FileDialog {
        id: backupSaveDialog
        title: "Save manga-dl backup"
        fileMode: FileDialog.SaveFile
        nameFilters: ["manga-dl Backup (*.mangadl)"]
        defaultSuffix: "mangadl"
        onAccepted: BackupController.createBackup(selectedFile.toString())
    }

    FileDialog {
        id: backupOpenDialog
        title: "Restore backup file"
        fileMode: FileDialog.OpenFile
        nameFilters: ["manga-dl / Tachiyomi Backup (*.mangadl *.tachibk *.proto.gz *.json)", "All files (*)"]
        onAccepted: BackupController.restoreBackup(selectedFile.toString())
    }

    Connections {
        target: BackupController
        function onBackupCreated(filePath) {
            Nav.toast("Backup saved: " + filePath.split('/').pop())
        }
        function onBackupRestored(m, p, c, t) {
            Nav.toast("Restored " + m + " manga, " + p + " progress records, " + c + " categories")
        }
        function onErrorMessageChanged() {
            if (BackupController.errorMessage !== "") {
                Nav.toast("Backup error: " + BackupController.errorMessage)
            }
        }
    }

    TwoColumns {
        leftCards: [
            SettingsCard {
                heading: "Storage"
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    Txt { text: SystemController.usedDiskFormatted + " used"; font.pixelSize: 20; font.weight: Font.ExtraBold; Layout.fillWidth: true }
                    Caption { text: "of " + SystemController.freeDiskFormatted + " free" }
                }
                Progress { Layout.fillWidth: true; Layout.topMargin: 8; height: 8; value: SystemController.usedDiskFraction }
                Row {
                    Layout.topMargin: 10
                    Layout.bottomMargin: 6
                    spacing: 16
                    Row { spacing: 6; Rectangle { width: 8; height: 8; radius: 4; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter } Caption { text: "Downloads " + SystemController.downloadDirFormatted } }
                    Row { spacing: 6; Rectangle { width: 8; height: 8; radius: 4; color: Theme.fgFaint; anchors.verticalCenter: parent.verticalCenter } Caption { text: "Cache " + SystemController.cacheDirFormatted } }
                }
                SettingRow {
                    label: "Download location"
                    description: SystemController.downloadLocation
                    Btn {
                        kind: "ghost"
                        size: "small"
                        text: "Change"
                        onClicked: downloadFolderDialog.open()
                    }
                }
                SettingRow {
                    label: "Clear image cache"
                    description: "Covers and pages you've read online."
                    showDivider: false
                    Btn {
                        kind: "ghost"
                        size: "small"
                        text: "Clear"
                        onClicked: {
                            SystemController.clearImageCache()
                            Nav.toast("Image cache cleared")
                        }
                    }
                }
            },
            SettingsCard {
                heading: "Sync"
                SettingRow {
                    label: "Sync library"; description: "Across every device signed in to your account."
                    MdSwitch {
                        checked: AppSettings.syncLibrary
                        onToggled: AppSettings.syncLibrary = checked
                    }
                }
                SettingRow {
                    label: "Sync reading progress"
                    MdSwitch {
                        checked: AppSettings.syncProgress
                        onToggled: AppSettings.syncProgress = checked
                    }
                }
                SettingRow {
                    label: "Sync settings"; showDivider: false
                    MdSwitch {
                        checked: AppSettings.syncSettings
                        onToggled: AppSettings.syncSettings = checked
                    }
                }
            }
        ]
        rightCards: [
            SettingsCard {
                heading: "Backup"
                SettingRow {
                    label: "Create backup"; description: "Library, categories, progress and settings."
                    Btn {
                        size: "small"
                        text: BackupController.isBusy ? "Creating..." : "Create"
                        enabled: !BackupController.isBusy
                        onClicked: backupSaveDialog.open()
                    }
                }
                SettingRow {
                    label: "Restore backup"; description: "From a .mangadl or Tachiyomi backup file."
                    Btn {
                        kind: "ghost"
                        size: "small"
                        text: BackupController.isBusy ? "Restoring..." : "Restore"
                        enabled: !BackupController.isBusy
                        onClicked: backupOpenDialog.open()
                    }
                }
                SettingRow {
                    label: "Automatic backups"
                    description: BackupController.lastBackupDate !== "" ? ("Last backup " + BackupController.lastBackupDate) : "No backups yet"
                    showDivider: false
                    MdCombo {
                        model: ["Off", "Daily", "Weekly", "Monthly"]
                        currentIndex: Math.max(0, model.indexOf(BackupController.autoBackupSchedule))
                        onActivated: (index) => BackupController.autoBackupSchedule = model[index]
                        implicitWidth: 130
                        Accessible.name: "Automatic backups"
                    }
                }
            }
        ]
    }

    // About
    Card {
        width: parent.width
        kind: "faint"
        RowLayout {
            Layout.fillWidth: true
            spacing: 20
            Image { source: "qrc:/qt/qml/MangaDl/assets/icons/manga-dl.png"; sourceSize: Qt.size(48, 48); Layout.preferredWidth: 48; Layout.preferredHeight: 48 }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                DisplayText { text: "manga-dl"; font.pixelSize: 22 }
                Caption { Layout.fillWidth: true; text: "Version " + AppInfo.version + " · Native Linux Client · Up to date"; font.pixelSize: 13 }
            }
            Btn {
                kind: "ghost"
                text: "GitHub"
                onClicked: Qt.openUrlExternally("https://github.com/zenmisan/manga-dl")
            }
        }
    }
}
