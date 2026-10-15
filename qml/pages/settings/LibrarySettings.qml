import QtQuick
import QtQuick.Layouts
import MangaDl

SettingsSection {
    id: section
    title: "Library"
    caption: "Categories, how the grid looks and when to check for new chapters."

    property string editingId: "" // "" closed, "new" adding, or category id
    property string editingName: ""

    function openEditor(id, currentName) {
        editingId = id
        editingName = currentName
        nameField.text = currentName
        nameField.forceActiveFocus()
    }

    function save() {
        const name = nameField.text.trim()
        if (!name) return
        if (editingId === "new") {
            CategoryModel.addCategory(name)
            Nav.toast("Added \"" + name + "\"")
        } else if (editingId !== "") {
            CategoryModel.renameCategory(editingId, name)
            Nav.toast("Renamed to \"" + name + "\"")
        }
        editingId = ""
    }

    function remove(id, name) {
        CategoryModel.deleteCategory(id)
        Nav.toast("Deleted \"" + name + "\"")
    }

    TwoColumns {
        leftCards: [
            SettingsCard {
                spacing: 12
                RowLayout {
                    Layout.fillWidth: true
                    Eyebrow { text: "Categories"; Layout.fillWidth: true }
                    Btn { size: "small"; text: "Add"; iconName: "Plus"; iconSize: 14; onClicked: section.openEditor("new", "") }
                }
                RowLayout {
                    visible: section.editingId !== ""
                    Layout.fillWidth: true
                    spacing: 8
                    Field {
                        id: nameField
                        Layout.fillWidth: true
                        placeholderText: "Category name"
                        onAccepted: section.save()
                        Keys.onEscapePressed: section.editingId = ""
                    }
                    Btn { text: "Save"; onClicked: section.save() }
                    Btn { kind: "ghost"; text: "Cancel"; onClicked: section.editingId = "" }
                }
                Repeater {
                    model: CategoryModel
                    Rectangle {
                        required property string id
                        required property string name
                        required property int mangaCount
                        required property int index
                        Layout.fillWidth: true
                        height: 52
                        radius: 12
                        color: Theme.surface
                        RowLayout {
                            anchors { fill: parent; leftMargin: 12; rightMargin: 6 }
                            spacing: 10
                            Icon { name: "Grip"; size: 16; color: Theme.fgFaint }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Txt { Layout.fillWidth: true; text: name; font.weight: Font.DemiBold }
                                Caption { Layout.fillWidth: true; text: mangaCount + (mangaCount === 1 ? " manga" : " manga") }
                            }
                            Btn { kind: "icon"; iconName: "Edit"; iconSize: 16; implicitHeight: 36; Accessible.name: "Rename"; onClicked: section.openEditor(id, name) }
                            Btn { kind: "icon"; iconName: "Trash"; iconSize: 16; implicitHeight: 36; Accessible.name: "Delete"; onClicked: section.remove(id, name) }
                        }
                    }
                }
                Caption { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "Drag to reorder. Deleting a category moves its manga to Default." }
            }
        ]
        rightCards: [
            SettingsCard {
                heading: "Display"
                SettingRow {
                    label: "Grid columns"
                    description: "Auto fits as many covers as the window allows."
                    below: [
                        Segmented {
                            model: ["Auto", "5", "6", "7", "8"]
                            currentIndex: {
                                const c = AppSettings.gridColumns
                                const idx = ["Auto", "5", "6", "7", "8"].indexOf(c)
                                return idx >= 0 ? idx : 0
                            }
                            onActivated: (idx) => {
                                AppSettings.gridColumns = ["Auto", "5", "6", "7", "8"][idx]
                            }
                        }
                    ]
                }
                SettingRow {
                    label: "Unread badges"
                    MdSwitch {
                        checked: AppSettings.unreadBadges
                        onToggled: AppSettings.unreadBadges = checked
                    }
                }
                SettingRow {
                    label: "Downloaded badges"
                    MdSwitch {
                        checked: AppSettings.downloadedBadges
                        onToggled: AppSettings.downloadedBadges = checked
                    }
                }
                SettingRow {
                    label: "Continue reading row"
                    showDivider: false
                    MdSwitch {
                        checked: AppSettings.continueReadingRow
                        onToggled: AppSettings.continueReadingRow = checked
                    }
                }
            },
            SettingsCard {
                heading: "Updates"
                SettingRow {
                    label: "Check for new chapters"
                    MdCombo {
                        model: ["Manually", "Every 6 hours", "Every 12 hours", "Daily", "Weekly"]
                        currentIndex: {
                            const c = AppSettings.checkNewChapters
                            const idx = ["Manually", "Every 6 hours", "Every 12 hours", "Daily", "Weekly"].indexOf(c)
                            return idx >= 0 ? idx : 2
                        }
                        Accessible.name: "Check for new chapters"
                        onActivated: (index) => {
                            AppSettings.checkNewChapters = model[index]
                        }
                    }
                }
                SettingRow {
                    label: "Only on unmetered networks"
                    MdSwitch {
                        checked: AppSettings.onlyUnmetered
                        onToggled: AppSettings.onlyUnmetered = checked
                    }
                }
                SettingRow {
                    label: "Skip completed series"
                    showDivider: false
                    MdSwitch {
                        checked: AppSettings.skipCompletedSeries
                        onToggled: AppSettings.skipCompletedSeries = checked
                    }
                }
            },
            SettingsCard {
                heading: "Migration"
                SettingRow {
                    label: "Migrate between sources"
                    description: "Move manga to another source and keep your progress."
                    showDivider: false
                    Btn { kind: "ghost"; size: "small"; text: "Open"; onClicked: Nav.open("Extensions", { tab: "migrate" }) }
                }
            }
        ]
    }
}
