import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Manga detail: hero tinted with the cover colour (ambilight) and the chapter table.
// Open with { manga: <manga map> }.
Item {
    id: page
    property var manga: ({})

    MangaDetailController {
        id: controller
    }

    readonly property string titleText: controller.detail.title || (page.manga ? page.manga.title : "") || "Manga Details"
    readonly property string coverSrc: controller.detail.coverUrl || (page.manga ? page.manga.cover : "") || "#1A2433"
    readonly property string sourceText: controller.detail.provider || (page.manga ? (page.manga.source || page.manga.provider) : "")
    readonly property string authorsText: controller.detail.authors && controller.detail.authors.length > 0 ? controller.detail.authors.join(", ") : ""
    readonly property string statusText: controller.detail.status || "Ongoing"
    readonly property string synopsisText: controller.detail.description || "No synopsis available."
    property bool showFullSynopsis: false

    property bool filterUnread: false
    property bool filterDownloaded: false
    property bool sortAscending: false

    readonly property var filteredChapters: {
        let list = controller.chapters.slice()
        if (filterUnread) list = list.filter(ch => !ch.read)
        if (filterDownloaded) list = list.filter(ch => ch.downloaded)
        if (sortAscending) list.reverse()
        return list
    }

    Component.onCompleted: {
        if (page.manga && (page.manga.provider || page.manga.source) && (page.manga.id || page.manga.mangaId)) {
            const prov = page.manga.provider || page.manga.source
            const id = page.manga.id || page.manga.mangaId
            controller.load(prov, id)
        }
    }

    function shade(c, f) {
        if (!c || !c.startsWith("#") || c.length < 7) return Theme.surfaceRaised
        const k = Qt.color(c)
        return Qt.rgba(k.r * f, k.g * f, k.b * f, 1)
    }

    component HeroButton: Btn {
        kind: "ghost"; borderColor: "transparent"; fillColor: Theme.darkChip
        implicitHeight: 46; fontSize: 14
    }

    PageScroll {
        anchors.fill: parent
        padding: 0
        topPadding: 0
        spacing: 0

        // Hero band
        Rectangle {
            width: parent.width
            height: heroRow.implicitHeight + 52
            color: page.coverSrc.startsWith("#") ? page.coverSrc : "#161B22"

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: Theme.alpha(Theme.bg, 0x66) }
                }
            }

            RowLayout {
                id: heroRow
                anchors { left: parent.left; right: parent.right; top: parent.top; leftMargin: 40; rightMargin: 40; topMargin: 24 }
                spacing: 32

                CoverArt {
                    Layout.alignment: Qt.AlignBottom
                    width: 200; height: 300; radius: 14
                    cover: page.coverSrc
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom
                    spacing: 12

                    Btn {
                        kind: "link"; text: "Back"; iconName: "Back"; iconSize: 14;
                        textColor: Theme.fgSoft; leftPadding: 0; font.weight: Font.DemiBold;
                        onClicked: Nav.back()
                    }

                    DisplayText {
                        Layout.fillWidth: true
                        text: page.titleText
                        font.pixelSize: 52
                        wrapMode: Text.Wrap
                    }

                    Txt {
                        Layout.fillWidth: true
                        text: (page.authorsText.length > 0 ? (page.authorsText + " · ") : "") + page.statusText + (page.sourceText.length > 0 ? (" · " + page.sourceText) : "")
                        color: Theme.fgStrong
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 720
                        spacing: 4

                        Txt {
                            Layout.fillWidth: true
                            text: page.synopsisText
                            color: Theme.fgStrong
                            wrapMode: Text.Wrap
                            maximumLineCount: page.showFullSynopsis ? 1000 : 4
                            elide: page.showFullSynopsis ? Text.ElideNone : Text.ElideRight
                            lineHeightMode: Text.FixedHeight
                            lineHeight: 22
                        }

                        Btn {
                            kind: "link"
                            visible: page.synopsisText.length > 180
                            text: page.showFullSynopsis ? "Show less" : "Show more"
                            fontSize: 12
                            textColor: Theme.accentLight
                            leftPadding: 0
                            onClicked: page.showFullSynopsis = !page.showFullSynopsis
                        }
                    }

                    Row {
                        Layout.topMargin: 6
                        spacing: 10

                        Btn {
                            text: controller.resumeChapter.number ? ("Resume Ch. " + controller.resumeChapter.number) : "Start Reading"
                            iconName: "Play"
                            implicitHeight: 46; fontSize: 14; leftPadding: 22; rightPadding: 22
                            onClicked: {
                                const ch = controller.resumeChapter
                                const isNovel = (controller.detail.type === "novel" || (page.manga && page.manga.type === "novel"))
                                Nav.open(isNovel ? "NovelReader" : "Reader", {
                                    manga: Object.assign({}, page.manga, controller.detail),
                                    chapterId: ch.id || (controller.chapters.length > 0 ? controller.chapters[0].id : ""),
                                    chapterTitle: ch.title || "",
                                    chapterNumber: ch.rawNumber || 0
                                })
                            }
                        }

                        HeroButton {
                            text: controller.inLibrary ? "In Library" : "Add to Library"
                            iconName: controller.inLibrary ? "Check" : "Plus"
                            onClicked: {
                                controller.toggleLibrary()
                                LibraryModel.refresh()
                            }
                        }

                        HeroButton {
                            text: "Track"
                            iconName: "Track"
                            onClicked: Nav.toast("Trackers can be connected in Settings")
                        }

                        HeroButton {
                            text: "Download"
                            iconName: "Download"
                            onClicked: Nav.toast("Download queued for " + page.titleText)
                        }

                        HeroButton {
                            text: "Migrate"
                            iconName: "Refresh"
                            onClicked: Nav.open("Extensions", { tab: "migrate" })
                        }
                    }
                }
            }
        }

        // Loading state
        Item {
            x: 40
            width: parent.width - 80
            height: 120
            visible: controller.loading

            RowLayout {
                anchors.centerIn: parent
                spacing: 12
                Icon { name: "Refresh"; size: 20; color: Theme.accentLight }
                Txt { text: "Loading chapters from " + (page.sourceText || "source") + "..."; color: Theme.fgMuted }
            }
        }

        // Error state
        Item {
            x: 40
            width: parent.width - 80
            height: 140
            visible: !controller.loading && controller.error.length > 0 && controller.chapters.length === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12
                Txt { text: "Failed to load chapters: " + controller.error; color: Theme.danger; horizontalAlignment: Text.AlignHCenter }
                Btn {
                    text: "Retry"
                    iconName: "Refresh"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: controller.reload()
                }
            }
        }

        // Chapter filters
        RowLayout {
            x: 40
            width: parent.width - 80
            height: 56
            spacing: 12
            visible: !controller.loading || controller.chapters.length > 0

            SectionTitle {
                text: controller.totalChapters + " chapters"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBottom
            }

            Chip {
                small: true; text: "Unread"; Layout.alignment: Qt.AlignBottom
                checked: page.filterUnread
                onClicked: page.filterUnread = !page.filterUnread
            }

            Chip {
                small: true; text: "Downloaded"; Layout.alignment: Qt.AlignBottom
                checked: page.filterDownloaded
                onClicked: page.filterDownloaded = !page.filterDownloaded
            }

            Btn {
                kind: "ghost"; size: "pill"
                text: page.sortAscending ? "Oldest first" : "Newest first"
                iconName: "Sort"; iconSize: 14; Layout.alignment: Qt.AlignBottom
                onClicked: page.sortAscending = !page.sortAscending
            }
        }

        // Chapter table
        Column {
            x: 40
            width: parent.width - 80
            topPadding: 12
            bottomPadding: 40
            visible: !controller.loading || controller.chapters.length > 0

            RowLayout {
                width: parent.width
                height: 36
                spacing: 16
                TableHeader { text: "No."; Layout.preferredWidth: 90; Layout.leftMargin: 12 }
                TableHeader { text: "Title"; Layout.fillWidth: true }
                TableHeader { text: "Released"; Layout.preferredWidth: 160 }
                TableHeader { text: "Group"; Layout.preferredWidth: 160 }
                TableHeader { text: "Status"; Layout.preferredWidth: 120; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 12 }
            }

            Repeater {
                model: page.filteredChapters
                delegate: Clickable {
                    required property var modelData
                    width: parent.width
                    height: 48
                    radius: 0
                    Accessible.name: "Chapter " + (modelData.number || modelData.id)
                    onClicked: {
                        const isNovel = (controller.detail.type === "novel" || (page.manga && page.manga.type === "novel"))
                        Nav.open(isNovel ? "NovelReader" : "Reader", {
                            manga: Object.assign({}, page.manga, controller.detail),
                            chapterId: modelData.id,
                            chapterTitle: modelData.title || "",
                            chapterNumber: modelData.rawNumber || 0
                        })
                    }

                    HairLine { width: parent.width }

                    RowLayout {
                        width: parent.width; height: parent.height
                        spacing: 16
                        opacity: modelData.read ? 0.5 : 1

                        Txt {
                            text: modelData.number || modelData.id
                            font.weight: Font.Bold
                            Layout.preferredWidth: 90; Layout.leftMargin: 12
                        }

                        Txt {
                            text: modelData.title || ("Chapter " + (modelData.number || modelData.id))
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Txt {
                            text: modelData.released || ""
                            color: Theme.fgSubtle
                            Layout.preferredWidth: 160
                        }

                        Txt {
                            text: modelData.group || ""
                            color: Theme.fgSubtle
                            Layout.preferredWidth: 160
                        }

                        Txt {
                            text: modelData.status || ""
                            font.pixelSize: 12; font.weight: Font.Bold
                            color: modelData.status && modelData.status.startsWith("Page") ? Theme.accentLight : Theme.fgSoft
                            horizontalAlignment: Text.AlignRight
                            Layout.preferredWidth: 120; Layout.rightMargin: 12
                        }
                    }
                }
            }
        }
    }
}
