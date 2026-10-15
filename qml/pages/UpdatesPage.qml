import QtQuick
import QtQuick.Layouts
import MangaDl

// New chapters grouped by day.
Item {
    id: root

    PageScroll {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            width: parent.width
            spacing: 12
            Column {
                Layout.fillWidth: true
                PageTitle { text: "Updates" }
                Caption {
                    text: "Last checked " + UpdatesModel.lastCheckedText + " · background sync every 30 min"
                    font.pixelSize: 13
                    topPadding: 4
                }
            }
            Btn {
                kind: "ghost"
                text: "Download All New"
                iconName: "Download"
                enabled: UpdatesModel.totalNewChapters > 0
                onClicked: {
                    UpdateCheckerService.checkUpdates(true)
                    Nav.toast("Queued new chapters for download")
                }
            }
            Btn {
                text: UpdateCheckerService.isChecking ? "Checking..." : "Check Now"
                iconName: "Refresh"
                enabled: !UpdateCheckerService.isChecking
                onClicked: {
                    UpdateCheckerService.checkUpdates(false)
                    Nav.toast("Checking library for new chapters...")
                }
            }
        }

        // Empty state when there are no new updates
        Column {
            width: parent.width
            visible: UpdatesModel.totalNewChapters === 0
            spacing: 16
            topPadding: 48
            bottomPadding: 48

            Icon {
                name: "Sparkles"
                size: 48
                color: Theme.accentMuted
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Txt {
                text: "All caught up"
                font.pixelSize: 20
                font.weight: Font.Bold
                color: Theme.fg
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Caption {
                text: "No new chapters detected in your library. Check back later or run Check Now."
                font.pixelSize: 14
                color: Theme.fgSubtle
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Repeater {
            model: UpdatesModel
            delegate: Column {
                required property var modelData
                width: parent.width
                Eyebrow { text: modelData.title; muted: true; topPadding: 12; bottomPadding: 6 }
                Repeater {
                    model: modelData.items
                    delegate: Item {
                        required property var modelData
                        width: parent.width
                        height: 76
                        HairLine { width: parent.width }
                        RowLayout {
                            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                            spacing: 16
                            CoverArt { width: 40; height: 60; radius: 6; cover: modelData.manga.cover }
                            Btn {
                                kind: "link"; textColor: Theme.fg; fontSize: 15; leftPadding: 0; alignLeft: true
                                text: modelData.manga.title
                                Layout.preferredWidth: 260
                                onClicked: Nav.go("MangaDetail", {
                                    provider: modelData.provider,
                                    mangaId: modelData.manga.id,
                                    title: modelData.manga.title,
                                    coverUrl: modelData.manga.cover
                                })
                            }
                            Txt { text: modelData.chapter; color: Theme.fgSoft; Layout.fillWidth: true; elide: Text.ElideRight }
                            Txt { text: modelData.source; font.pixelSize: 13; color: Theme.fgSubtle; Layout.preferredWidth: 120 }
                            Btn {
                                kind: "ghost"; size: "small"; text: "Download"; font.weight: Font.Bold
                                onClicked: {
                                    DownloadsModel.enqueueChapter(
                                        modelData.provider,
                                        modelData.manga.id,
                                        modelData.manga.title,
                                        modelData.chapterId,
                                        modelData.chapter,
                                        modelData.chapterNumber
                                    )
                                    Nav.toast("Downloading " + modelData.chapter)
                                }
                            }
                            Btn {
                                kind: "soft"; size: "small"; text: "Read"
                                onClicked: Nav.go("Reader", {
                                    provider: modelData.provider,
                                    mangaId: modelData.manga.id,
                                    mangaTitle: modelData.manga.title,
                                    chapterId: modelData.chapterId,
                                    chapterTitle: modelData.chapter,
                                    chapterNumber: modelData.chapterNumber,
                                    coverUrl: modelData.manga.cover
                                })
                            }
                        }
                    }
                }
            }
        }
    }
}
