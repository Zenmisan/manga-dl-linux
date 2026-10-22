import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import MangaDl

// Download queue with a "save to" side panel.
Item {
    id: page

    function stateText(s) { return s === "failed" ? Theme.errorText : s === "done" ? Theme.successText : Theme.fgMuted }
    function stateBar(s) { return s === "failed" ? Theme.errorBar : s === "done" ? Theme.success : s === "paused" ? Theme.fgFaint : Theme.accent }
    function stateAction(s) { return s === "failed" ? "Retry" : s === "paused" ? "Resume" : s === "done" ? "Remove" : "Pause" }
    function stateIcon(s) { return s === "failed" ? "Refresh" : s === "paused" ? "Play" : s === "done" ? "Close" : "Pause" }

    FolderDialog {
        id: folderDialog
        title: "Select download folder"
        currentFolder: AppSettings.downloadPath ? ("file://" + AppSettings.downloadPath) : ""
        onAccepted: {
            let path = selectedFolder.toString()
            if (path.startsWith("file://"))
                path = path.substring(7)
            AppSettings.downloadPath = path
        }
    }

    PageScroll {
        anchors.fill: parent

        Flow {
            width: parent.width
            spacing: 24

            ColumnLayout {
                width: parent.width >= 780 ? (parent.width - 324) : parent.width
                spacing: 20

                Flow {
                    Layout.fillWidth: true
                    spacing: 10

                    PageTitle {
                        text: "Downloads"
                        width: parent.width < 500 ? parent.width : undefined
                    }

                    Item {
                        visible: parent.width >= 500
                        width: Math.max(0, parent.width - 380)
                        height: 1
                    }

                    Btn {
                        kind: "ghost"; text: "Clear Finished"
                        onClicked: DownloadsModel.clearFinished()
                    }
                    Btn {
                        text: DownloadsModel.isPaused ? "Resume All" : "Pause All"
                        iconName: DownloadsModel.isPaused ? "Play" : "Pause"
                        onClicked: DownloadsModel.isPaused ? DownloadsModel.resumeAll() : DownloadsModel.pauseAll()
                    }
                }

                Tabs {
                    Layout.fillWidth: true
                    model: ["Queue · " + DownloadsModel.queueCount, "Finished · " + DownloadsModel.finishedCount, "Failed · " + DownloadsModel.failedCount]
                    currentIndex: DownloadsModel.filterTab === "finished" ? 1 : DownloadsModel.filterTab === "failed" ? 2 : 0
                    onActivated: index => {
                        if (index === 0) DownloadsModel.filterTab = "queue"
                        else if (index === 1) DownloadsModel.filterTab = "finished"
                        else DownloadsModel.filterTab = "failed"
                    }
                }

                Item {
                    Layout.fillWidth: true
                    height: 140
                    visible: DownloadsModel.count === 0
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        Icon { Layout.alignment: Qt.AlignHCenter; name: "Download"; size: 32; color: Theme.fgMuted }
                        Txt {
                            Layout.alignment: Qt.AlignHCenter
                            text: DownloadsModel.filterTab === "finished" ? "No finished downloads" :
                                  DownloadsModel.filterTab === "failed" ? "No failed downloads" : "Download queue is empty"
                            color: Theme.fgMuted
                            font.pixelSize: 14
                        }
                    }
                }

                Repeater {
                    model: DownloadsModel
                    delegate: Rectangle {
                        required property var model
                        Layout.fillWidth: true
                        height: 88
                        radius: 14
                        color: Theme.surface
                        border.color: Theme.dividerStrong
                        RowLayout {
                            anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                            spacing: 16
                            CoverArt { width: 40; height: 60; radius: 6; cover: model.manga.cover }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 0
                                spacing: 8
                                RowLayout {
                                    Layout.fillWidth: true
                                    Txt {
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 0
                                        elide: Text.ElideRight
                                        textFormat: Text.StyledText
                                        text: model.manga.title + "<font color='" + Theme.fgSubtle + "'> · " + model.chapter + "</font>"
                                        font.weight: Font.Bold
                                    }
                                    Txt { text: model.status; font.pixelSize: 12; font.weight: Font.Bold; color: page.stateText(model.state) }
                                }
                                Progress { Layout.fillWidth: true; value: model.progress; fill: page.stateBar(model.state) }
                            }
                            Btn {
                                kind: "icon"; iconName: page.stateIcon(model.state); iconSize: 18
                                fillColor: Theme.surfaceRaised
                                Accessible.name: page.stateAction(model.state)
                                onClicked: {
                                    if (model.state === "failed") DownloadsModel.retryDownload(model.id)
                                    else if (model.state === "paused") DownloadsModel.resumeDownload(model.id)
                                    else if (model.state === "done") DownloadsModel.cancelDownload(model.id)
                                    else DownloadsModel.pauseDownload(model.id)
                                }
                            }
                            Btn {
                                kind: "icon"; iconName: "Close"; iconSize: 18; textColor: Theme.fgMuted; Accessible.name: "Cancel download"
                                visible: model.state !== "done"
                                onClicked: DownloadsModel.cancelDownload(model.id)
                            }
                        }
                    }
                }
            }

            Card {
                width: parent.width >= 780 ? 300 : parent.width
                kind: "list"
                padding: 20
                radius: 16
                spacing: 14
                Eyebrow { text: "Save to" }
                Txt {
                    text: AppSettings.downloadPath ? AppSettings.downloadPath : "~/Downloads/manga-dl"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                }
                Btn {
                    Layout.fillWidth: true
                    kind: "ghost"
                    text: "Change Folder"
                    iconName: "Folder"
                    implicitHeight: 40
                    radius: 10
                    onClicked: folderDialog.open()
                }
                Repeater {
                    model: [
                        ["Format", AppSettings.downloadFormat ? AppSettings.downloadFormat : "CBZ + ComicInfo"],
                        ["Parallel downloads", AppSettings.parallelDownloads ? AppSettings.parallelDownloads.toString() : "3"],
                        ["Storage used", DownloadsModel.storageUsed]
                    ]
                    RowLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        Txt { text: modelData[0]; font.pixelSize: 13; color: Theme.fgMuted; Layout.fillWidth: true }
                        Txt { text: modelData[1]; font.pixelSize: 13; font.weight: Font.DemiBold }
                    }
                }
            }
        }
    }
}
