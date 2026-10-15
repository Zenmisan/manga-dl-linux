import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import MangaDl

// Import local files: drop zone, this session's queue, and options.
Item {
    id: page

    function importText(k) { return k === "done" ? Theme.successText : k === "error" ? Theme.errorText : Theme.fgMuted }
    function importBar(k) { return k === "done" ? Theme.success : k === "error" ? Theme.errorBar : k === "queued" ? "transparent" : Theme.accent }

    ListModel {
        id: sessionModel
    }

    function processPaths(urls) {
        if (!urls || urls.length === 0) return
        let targetId = ""
        if (destExisting.checked && seriesSelect.currentIndex >= 0 && seriesSelect.count > 0) {
            const currentItem = LibraryModel.data(LibraryModel.index(seriesSelect.currentIndex, 0), LibraryModel.IdRole)
            if (currentItem)
                targetId = currentItem.toString()
        }

        const results = LocalImportService.importPaths(urls, targetId, comicInfoCheck.checked)
        for (let i = 0; i < results.length; ++i) {
            sessionModel.insert(0, results[i])
        }
        Nav.toast(results.length + " item(s) processed")
    }

    FileDialog {
        id: filePicker
        title: "Choose archives to import"
        nameFilters: ["Comic & novel archives (*.cbz *.zip *.epub)", "All files (*)"]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            let list = []
            for (let i = 0; i < selectedFiles.length; ++i) {
                list.push(selectedFiles[i].toString())
            }
            page.processPaths(list)
        }
    }

    FolderDialog {
        id: folderPicker
        title: "Choose folder to import"
        onAccepted: {
            page.processPaths([selectedFolder.toString()])
        }
    }

    PageScroll {
        anchors.fill: parent

        RowLayout {
            width: parent.width
            spacing: 32

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 20
                PageTitle { text: "Import files" }

                // Drop zone
                Rectangle {
                    Layout.fillWidth: true
                    height: 300
                    radius: 20
                    color: drop.containsDrag ? Theme.accentMuted : Theme.accentWash
                    border.color: Theme.accent
                    border.width: 2

                    DropArea {
                        id: drop
                        anchors.fill: parent
                        onDropped: d => {
                            let list = []
                            for (let i = 0; i < d.urls.length; ++i) {
                                list.push(d.urls[i].toString())
                            }
                            page.processPaths(list)
                        }
                    }
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 14
                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 64; height: 64; radius: 18; color: Theme.accentMuted
                            Icon { anchors.centerIn: parent; name: "Upload"; size: 30; color: Theme.accentLight }
                        }
                        Txt { Layout.fillWidth: true; Layout.alignment: Qt.AlignHCenter; text: "Drop files to import"; font.pixelSize: 22; font.weight: Font.Black }
                        Txt { Layout.fillWidth: true; Layout.alignment: Qt.AlignHCenter; text: "CBZ, ZIP, EPUB or folders of images"; color: Theme.fgSoft }
                        Row {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 6
                            spacing: 10
                            Btn {
                                text: "Choose Files"; implicitHeight: 44
                                onClicked: filePicker.open()
                            }
                            Btn {
                                kind: "ghost"; text: "Choose Folder"; implicitHeight: 44
                                onClicked: folderPicker.open()
                            }
                        }
                    }
                }

                Eyebrow { text: "This session"; muted: true; Layout.topMargin: 6 }

                Item {
                    Layout.fillWidth: true
                    height: 70
                    visible: sessionModel.count === 0
                    Txt {
                        anchors.centerIn: parent
                        text: "No files imported this session"
                        color: Theme.fgMuted
                        font.pixelSize: 13
                    }
                }

                Repeater {
                    model: sessionModel
                    delegate: Rectangle {
                        required property var model
                        Layout.fillWidth: true
                        height: 64
                        radius: 12
                        color: Theme.surface4
                        border.color: Theme.dividerStrong
                        RowLayout {
                            anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                            spacing: 14
                            Rectangle {
                                width: 44; height: 36; radius: 8; color: Theme.surfaceRaised
                                Txt { anchors.centerIn: parent; text: model.ext; font.pixelSize: 10; font.weight: Font.Black }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                RowLayout {
                                    Txt { text: model.name; font.weight: Font.Bold; Layout.fillWidth: true; elide: Text.ElideMiddle }
                                    Txt { text: model.message || model.status; font.pixelSize: 12; font.weight: Font.Bold; color: page.importText(model.kind) }
                                }
                                Progress { Layout.fillWidth: true; value: model.progress; fill: page.importBar(model.kind); color: Theme.dividerStrong }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: false
                Layout.preferredWidth: 320
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 72
                spacing: 14
                Card {
                    Layout.fillWidth: true
                    kind: "list"; color: Theme.surface4; radius: 16; padding: 20; spacing: 8
                    Eyebrow { text: "Import into"; Layout.bottomMargin: 4 }
                    ButtonGroup { id: dest }
                    MdRadio { id: destNew; text: "New series per folder"; checked: true; ButtonGroup.group: dest }
                    MdRadio { id: destExisting; text: "Add to existing series"; ButtonGroup.group: dest }
                    ComboBox {
                        id: seriesSelect
                        Layout.fillWidth: true
                        visible: destExisting.checked
                        model: LibraryModel
                        textRole: "title"
                    }
                    CheckRow { id: comicInfoCheck; text: "Read ComicInfo.xml"; checked: true }
                }
                Card {
                    Layout.fillWidth: true
                    kind: "list"; color: Theme.surface4; radius: 16; padding: 20; spacing: 8
                    Eyebrow { text: "Tips"; muted: true }
                    Txt {
                        Layout.fillWidth: true
                        text: "Name folders after the series. Files are sorted by volume and chapter numbers in their names."
                        font.pixelSize: 13; color: Theme.fgSoft; wrapMode: Text.Wrap
                        lineHeightMode: Text.FixedHeight; lineHeight: 20
                    }
                    Btn {
                        kind: "link"; text: "Read the import guide"; fontSize: 13; leftPadding: 0
                        onClicked: Nav.toast("Supported: .cbz, .zip, .epub, and folders of images. Subfolders named 'Chapter N' are grouped automatically.")
                    }
                }
            }
        }
    }
}
