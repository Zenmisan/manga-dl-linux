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
    property bool filterBookmarked: false
    property bool sortAscending: true
    property string chapterSearch: ""
    property int bookmarkRevision: 0
    property int selectedRating: controller.personalRating
    property bool showNotesSection: false
    property bool showOverrideModal: false

    readonly property var filteredChapters: {
        const dummy = page.bookmarkRevision
        let list = controller.chapters.slice()
        if (page.chapterSearch.trim().length > 0) {
            const q = page.chapterSearch.trim().toLowerCase()
            list = list.filter(ch => {
                const titleMatch = ch.title && ch.title.toLowerCase().indexOf(q) >= 0
                const numMatch = ch.number && String(ch.number).toLowerCase().indexOf(q) >= 0
                return titleMatch || numMatch
            })
        }
        if (page.filterUnread) list = list.filter(ch => !ch.read)
        if (page.filterDownloaded) list = list.filter(ch => ch.downloaded)
        if (page.filterBookmarked) list = list.filter(ch => controller.isChapterBookmarked(ch.id))
        list.sort((a, b) => {
            let na = 0
            if (typeof a.rawNumber === 'number' && !isNaN(a.rawNumber) && a.rawNumber > 0) {
                na = a.rawNumber
            } else if (a.number !== undefined && a.number !== null) {
                na = parseFloat(String(a.number).replace(/[^0-9.]/g, '')) || 0
            }
            let nb = 0
            if (typeof b.rawNumber === 'number' && !isNaN(b.rawNumber) && b.rawNumber > 0) {
                nb = b.rawNumber
            } else if (b.number !== undefined && b.number !== null) {
                nb = parseFloat(String(b.number).replace(/[^0-9.]/g, '')) || 0
            }
            if (na !== nb) {
                return page.sortAscending ? (na - nb) : (nb - na)
            }
            const sa = String(a.number || a.id || "")
            const sb = String(b.number || b.id || "")
            return page.sortAscending ? sa.localeCompare(sb) : sb.localeCompare(sa)
        })
        return list
    }

    Connections {
        target: controller
        function onPersonalNoteChanged() {
            page.selectedRating = controller.personalRating
            if (personalNoteInput) {
                personalNoteInput.text = controller.personalNote
            }
        }
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
                                const isNovel = controller.isNovel || controller.detail.type === "novel" || (page.manga && (page.manga.type === "novel" || page.manga.contentType === "novel"))
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
                            text: controller.pinned ? "Pinned" : "Pin"
                            iconName: "Bookmark"
                            onClicked: {
                                controller.togglePinned()
                                Nav.toast(controller.pinned ? "Manga pinned to top" : "Manga unpinned")
                            }
                        }

                        HeroButton {
                            text: controller.muted ? "Muted" : "Mute"
                            iconName: "Close"
                            onClicked: {
                                controller.toggleMuted()
                                Nav.toast(controller.muted ? "Notifications muted" : "Notifications enabled")
                            }
                        }

                        HeroButton {
                            text: controller.hasOverride ? "Customized" : "Edit Info"
                            iconName: "Edit"
                            onClicked: {
                                overrideTitleField.text = controller.detail.title || (page.manga ? page.manga.title : "") || ""
                                overrideCoverField.text = controller.detail.coverUrl || (page.manga ? page.manga.cover : "") || ""
                                overrideSynopsisField.text = controller.detail.description || ""
                                page.showOverrideModal = true
                            }
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

        // Personal Rating & Notes
        Card {
            x: 40
            width: parent.width - 80
            kind: "faint"
            padding: 16
            spacing: 12
            visible: !controller.loading || controller.chapters.length > 0

            RowLayout {
                width: parent.width
                spacing: 16

                RowLayout {
                    spacing: 8
                    Txt { text: "Your Rating:"; font.pixelSize: 13; font.weight: Font.Bold; color: Theme.fgSoft }
                    Repeater {
                        model: 5
                        Clickable {
                            required property int index
                            width: 26; height: 26
                            onClicked: {
                                page.selectedRating = index + 1
                                controller.savePersonalNote(page.selectedRating, personalNoteInput ? personalNoteInput.text : "")
                                Nav.toast("Rated " + (index + 1) + " / 5 stars")
                            }
                            Icon {
                                anchors.centerIn: parent
                                name: "Star"
                                size: 18
                                color: index < page.selectedRating ? Theme.accentLight : Theme.fgFaint
                            }
                        }
                    }
                    Txt {
                        text: page.selectedRating > 0 ? (page.selectedRating + " / 5") : "Not rated"
                        font.pixelSize: 12
                        color: Theme.fgSubtle
                    }
                }

                Item { Layout.fillWidth: true }

                Btn {
                    kind: "link"
                    text: page.showNotesSection ? "Hide Notes" : (controller.personalNote.length > 0 ? "Edit Notes" : "+ Add Note")
                    fontSize: 13
                    textColor: Theme.accentLight
                    onClicked: page.showNotesSection = !page.showNotesSection
                }
            }

            ColumnLayout {
                width: parent.width
                visible: page.showNotesSection || (controller.personalNote.length > 0)
                spacing: 8

                Field {
                    id: personalNoteInput
                    Layout.fillWidth: true
                    placeholderText: "Personal notes, reading logs, or private reminders..."
                    text: controller.personalNote
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Item { Layout.fillWidth: true }
                    Btn {
                        size: "small"
                        text: "Save Note"
                        onClicked: {
                            controller.savePersonalNote(page.selectedRating, personalNoteInput.text)
                            Nav.toast("Personal note saved")
                        }
                    }
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
                text: page.filteredChapters.length + " of " + controller.totalChapters + " chapters"
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true; Layout.minimumWidth: 16 }

            SearchField {
                Layout.preferredWidth: 200
                Layout.alignment: Qt.AlignVCenter
                placeholderText: "Filter chapters..."
                text: page.chapterSearch
                onTextChanged: page.chapterSearch = text
            }

            Chip {
                small: true; text: "Unread"; Layout.alignment: Qt.AlignVCenter
                checked: page.filterUnread
                onClicked: page.filterUnread = !page.filterUnread
            }

            Chip {
                small: true; text: "Downloaded"; Layout.alignment: Qt.AlignVCenter
                checked: page.filterDownloaded
                onClicked: page.filterDownloaded = !page.filterDownloaded
            }

            Chip {
                small: true; text: "Bookmarked"; Layout.alignment: Qt.AlignVCenter
                checked: page.filterBookmarked
                onClicked: page.filterBookmarked = !page.filterBookmarked
            }

            Btn {
                kind: "ghost"; size: "pill"
                text: page.sortAscending ? "Oldest first" : "Newest first"
                iconName: "Sort"; iconSize: 14; Layout.alignment: Qt.AlignVCenter
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
                spacing: 12
                TableHeader { text: ""; Layout.preferredWidth: 36; Layout.leftMargin: 8 }
                TableHeader { text: "No."; Layout.preferredWidth: 80 }
                TableHeader { text: "Title"; Layout.fillWidth: true; Layout.minimumWidth: 160 }
                TableHeader { text: "Released"; Layout.preferredWidth: 160 }
                TableHeader { text: "Group"; Layout.preferredWidth: 160 }
                TableHeader { text: "Status"; Layout.preferredWidth: 120; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 12 }
            }

            Repeater {
                model: page.filteredChapters
                delegate: Rectangle {
                    id: chapterRow
                    required property var modelData
                    width: parent.width
                    height: 48
                    color: rowHover.containsMouse ? Theme.surfaceRaised : "transparent"
                    radius: 6
                    Accessible.name: "Chapter " + (modelData.number || modelData.id)

                    MouseArea {
                        id: rowHover
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                chapterMenu.targetChapter = modelData
                                chapterMenu.popup()
                            } else {
                                const isNovel = controller.isNovel || controller.detail.type === "novel" || (page.manga && (page.manga.type === "novel" || page.manga.contentType === "novel"))
                                Nav.open(isNovel ? "NovelReader" : "Reader", {
                                    manga: Object.assign({}, page.manga, controller.detail),
                                    chapterId: modelData.id,
                                    chapterTitle: modelData.title || "",
                                    chapterNumber: modelData.rawNumber || 0
                                })
                            }
                        }
                    }

                    HairLine { width: parent.width; anchors.bottom: parent.bottom }

                    RowLayout {
                        width: parent.width; height: parent.height
                        spacing: 12
                        opacity: modelData.read ? 0.5 : 1

                        Clickable {
                            Layout.preferredWidth: 36; height: 36
                            Layout.leftMargin: 8
                            radius: 6
                            onClicked: {
                                controller.toggleChapterBookmark(modelData.id)
                                page.bookmarkRevision++
                                Nav.toast(controller.isChapterBookmarked(modelData.id) ? "Chapter bookmarked" : "Bookmark removed")
                            }
                            Icon {
                                anchors.centerIn: parent
                                name: "Bookmark"
                                size: 16
                                color: controller.isChapterBookmarked(modelData.id) ? Theme.accentLight : (rowHover.containsMouse ? Theme.fgSoft : Theme.fgFaint)
                            }
                        }

                        Txt {
                            text: modelData.number || modelData.id
                            font.weight: Font.Bold
                            Layout.preferredWidth: 80
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

    // Context Menu for Chapter actions
    Menu {
        id: chapterMenu
        property var targetChapter: null

        MenuItem {
            text: chapterMenu.targetChapter && chapterMenu.targetChapter.read ? "Mark as Unread" : "Mark as Read"
            onTriggered: {
                if (chapterMenu.targetChapter) {
                    controller.markChapterRead(chapterMenu.targetChapter.id, !chapterMenu.targetChapter.read)
                    Nav.toast(chapterMenu.targetChapter.read ? "Marked as unread" : "Marked as read")
                }
            }
        }

        MenuItem {
            text: "Mark Previous Chapters as Read"
            onTriggered: {
                if (chapterMenu.targetChapter) {
                    controller.markPreviousChaptersRead(chapterMenu.targetChapter.id)
                    Nav.toast("Previous chapters marked as read")
                }
            }
        }

        MenuItem {
            text: chapterMenu.targetChapter && controller.isChapterBookmarked(chapterMenu.targetChapter.id) ? "Remove Bookmark" : "Bookmark Chapter"
            onTriggered: {
                if (chapterMenu.targetChapter) {
                    controller.toggleChapterBookmark(chapterMenu.targetChapter.id)
                    page.bookmarkRevision++
                    Nav.toast("Chapter bookmark updated")
                }
            }
        }
    }

    // Metadata Override Modal
    Item {
        id: overrideModal
        anchors.fill: parent
        z: 30
        visible: page.showOverrideModal
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.fade } }

        Rectangle {
            anchors.fill: parent
            color: Theme.scrim
            MouseArea { anchors.fill: parent; onClicked: page.showOverrideModal = false }
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(520, parent.width - 48)
            height: overrideCol.implicitHeight + 48
            radius: 20
            color: Theme.sheet
            border.color: Theme.track
            border.width: 1
            MouseArea { anchors.fill: parent }

            ColumnLayout {
                id: overrideCol
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 24 }
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true
                    DisplayText { text: "Edit Manga Metadata"; font.pixelSize: 28; Layout.fillWidth: true }
                    Btn {
                        kind: "icon"; iconName: "Close"; iconSize: 16
                        onClicked: page.showOverrideModal = false
                    }
                }

                Caption {
                    Layout.fillWidth: true
                    text: "Override title, cover, and synopsis locally without changing upstream source."
                }

                LabeledField {
                    id: overrideTitleField
                    label: "Custom Title"
                    placeholderText: "Title override"
                }

                LabeledField {
                    id: overrideCoverField
                    label: "Custom Cover URL"
                    placeholderText: "https://... or #color"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    FieldLabel { text: "Custom Synopsis" }
                    Field {
                        id: overrideSynopsisField
                        Layout.fillWidth: true
                        placeholderText: "Synopsis override..."
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    spacing: 12

                    Btn {
                        kind: "ghost"
                        text: "Reset Defaults"
                        visible: controller.hasOverride
                        onClicked: {
                            controller.resetOverride()
                            page.showOverrideModal = false
                            Nav.toast("Metadata override reset to source default")
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Btn {
                        kind: "ghost"
                        text: "Cancel"
                        onClicked: page.showOverrideModal = false
                    }

                    Btn {
                        text: "Save Changes"
                        onClicked: {
                            controller.saveOverride(overrideTitleField.text, overrideCoverField.text, overrideSynopsisField.text)
                            page.showOverrideModal = false
                            Nav.toast("Custom metadata saved")
                        }
                    }
                }
            }
        }
    }
}
