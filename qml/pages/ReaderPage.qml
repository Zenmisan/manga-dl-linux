import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import MangaDl

// Manga reader: single page, spread, webtoon, or vertical with settings panel.
// Keys: ← → / Space turn pages, F fullscreen, Esc leaves fullscreen or closes.
Rectangle {
    id: page
    property var manga: ({})
    property string chapterId: ""
    property string chapterTitle: ""
    property double chapterNumber: 0

    property string layout: AppSettings.readerMode === "Single" ? "single" : "spread"
    readonly property bool spread: layout === "spread"
    property int current: controller.currentPage
    readonly property int total: controller.totalPages
    property bool bookmarked: false
    property bool showSettings: false

    ReaderController {
        id: controller
    }

    Timer {
        id: saveTimer
        interval: 500
        repeat: false
        onTriggered: {
            if (controller.totalPages > 0) {
                controller.saveProgress(page.current, page.total, page.current >= page.total)
            }
        }
    }

    color: Theme.readerBg
    focus: true

    Component.onCompleted: {
        forceActiveFocus()
        if (page.manga && page.chapterId) {
            const prov = page.manga.provider || page.manga.source || ""
            const mId = page.manga.id || page.manga.mangaId || ""
            const mTitle = page.manga.title || ""
            const mCover = page.manga.cover || page.manga.coverUrl || ""
            controller.loadChapter(prov, mId, page.chapterId, mTitle, mCover, page.chapterTitle, page.chapterNumber)
        }
    }

    Component.onDestruction: {
        if (controller.totalPages > 0) {
            controller.saveProgress(page.current, page.total, page.current >= page.total)
        }
    }

    function go(delta) {
        if (total <= 0) return
        const step = spread ? 2 : 1
        const next = Math.max(1, Math.min(total, page.current + delta * step))
        if (next !== page.current) {
            page.current = next
            controller.currentPage = next
            saveTimer.restart()
        }
    }

    function toggleFullscreen() {
        const w = Window.window
        if (w) {
            w.visibility = w.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
        }
    }

    Keys.onPressed: e => {
        if (e.key === Qt.Key_Right || e.key === Qt.Key_Space) { go(1); e.accepted = true }
        else if (e.key === Qt.Key_Left) { go(-1); e.accepted = true }
        else if (e.key === Qt.Key_F) { toggleFullscreen(); e.accepted = true }
        else if (e.key === Qt.Key_Escape) {
            const w = Window.window
            if (w && w.visibility === Window.FullScreen) toggleFullscreen()
            else Nav.back()
            e.accepted = true
        }
    }

    // ===== Top bar =====
    Rectangle {
        id: topBar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 56
        color: "#EB080808"
        z: 2
        HairLine { anchors.bottom: parent.bottom; width: parent.width; color: Theme.dividerStrong }

        RowLayout {
            anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
            spacing: 12

            Btn { kind: "icon"; iconName: "Back"; Accessible.name: "Close reader"; onClicked: Nav.back() }

            Txt {
                Layout.fillWidth: true
                textFormat: Text.StyledText
                text: "<b>" + (page.manga ? page.manga.title : "") + "</b><font color='" + Theme.fgSubtle + "'>  ·  Ch. " + (page.chapterNumber > 0 ? page.chapterNumber : page.chapterId) + (page.chapterTitle.length > 0 ? (" · " + page.chapterTitle) : "") + "</font>"
                elide: Text.ElideRight
            }

            Segmented {
                model: ["Single", "Spread"]
                currentIndex: page.spread ? 1 : 0
                onActivated: index => {
                    page.layout = index === 1 ? "spread" : "single"
                    AppSettings.readerMode = page.layout === "spread" ? "Spread" : "Single"
                }
            }

            Btn {
                kind: "ghost"; size: "small"; text: "Prev ch."; font.weight: Font.Bold
                enabled: controller.prevChapterId.length > 0
                opacity: enabled ? 1.0 : 0.4
                onClicked: {
                    if (controller.prevChapterId.length > 0) {
                        const prov = page.manga.provider || page.manga.source || ""
                        const mId = page.manga.id || page.manga.mangaId || ""
                        controller.loadChapter(prov, mId, controller.prevChapterId, page.manga.title || "", page.manga.cover || "")
                    }
                }
            }

            Txt {
                Layout.minimumWidth: 72
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 13; font.weight: Font.Bold
                text: page.total > 0
                    ? (page.spread ? page.current + "–" + Math.min(page.current + 1, page.total) + " / " + page.total : page.current + " / " + page.total)
                    : "-- / --"
                Accessible.role: Accessible.StaticText
            }

            Btn {
                kind: "ghost"; size: "small"; text: "Next ch."; font.weight: Font.Bold
                enabled: controller.nextChapterId.length > 0
                opacity: enabled ? 1.0 : 0.4
                onClicked: {
                    if (controller.nextChapterId.length > 0) {
                        const prov = page.manga.provider || page.manga.source || ""
                        const mId = page.manga.id || page.manga.mangaId || ""
                        controller.loadChapter(prov, mId, controller.nextChapterId, page.manga.title || "", page.manga.cover || "")
                    }
                }
            }

            Btn {
                kind: "icon"; iconName: page.bookmarked ? "BookmarkFilled" : "Bookmark"
                textColor: page.bookmarked ? Theme.accentLight : Theme.fg
                Accessible.name: "Bookmark page"
                onClicked: page.bookmarked = !page.bookmarked
            }

            Btn {
                kind: "icon"; iconName: "Fullscreen"
                Accessible.name: "Toggle fullscreen"
                onClicked: page.toggleFullscreen()
            }

            IconToggle {
                iconName: "Sliders"
                checked: page.showSettings
                onToggled: page.showSettings = checked
                Accessible.name: "Reader settings"
            }
        }
    }

    // ===== Stage =====
    Item {
        id: stage
        anchors { left: parent.left; top: topBar.bottom; bottom: parent.bottom; right: panel.visible ? panel.left : parent.right }

        // Loading state
        Column {
            anchors.centerIn: parent
            spacing: 12
            visible: controller.loading

            Icon { name: "Refresh"; size: 28; color: Theme.accentLight; anchors.horizontalCenter: parent.horizontalCenter }
            Txt { text: "Loading chapter pages..."; color: Theme.fgMuted; anchors.horizontalCenter: parent.horizontalCenter }
        }

        // Error state
        Column {
            anchors.centerIn: parent
            spacing: 12
            visible: !controller.loading && controller.error.length > 0 && controller.pages.length === 0

            Txt { text: "Failed to load pages: " + controller.error; color: Theme.danger; anchors.horizontalCenter: parent.horizontalCenter }
            Btn {
                text: "Retry"
                iconName: "Refresh"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    const prov = page.manga.provider || page.manga.source || ""
                    const mId = page.manga.id || page.manga.mangaId || ""
                    controller.loadChapter(prov, mId, page.chapterId, page.manga.title || "", page.manga.cover || "", page.chapterTitle, page.chapterNumber)
                }
            }
        }

        // Two-page spread (right-to-left: next page on left)
        Row {
            visible: page.spread && !controller.loading && controller.pages.length > 0
            anchors.centerIn: parent
            spacing: 2
            readonly property real scale: Math.min(1, (stage.width - 40) / 1042, (stage.height - 40) / 760)

            readonly property int leftIdx: Math.min(page.current + 1, page.total)
            readonly property int rightIdx: page.current

            PagePlaceholder {
                width: 520 * parent.scale; height: 760 * parent.scale
                label: "Page " + parent.leftIdx
                pageUrl: controller.pages.length >= parent.leftIdx ? controller.pages[parent.leftIdx - 1] : ""
            }
            PagePlaceholder {
                width: 520 * parent.scale; height: 760 * parent.scale
                label: "Page " + parent.rightIdx
                pageUrl: controller.pages.length >= parent.rightIdx ? controller.pages[parent.rightIdx - 1] : ""
            }
        }

        MouseArea {
            visible: page.spread && !controller.loading && controller.pages.length > 0
            anchors.fill: parent
            // Click zones: left half goes forward, right half goes back
            onClicked: m => page.go(m.x < width / 2 ? 1 : -1)
        }

        // Single page layout
        Item {
            visible: !page.spread && !controller.loading && controller.pages.length > 0
            anchors.fill: parent

            RowLayout {
                anchors { fill: parent; topMargin: 16; bottomMargin: 96 }
                spacing: 24

                Item { Layout.fillWidth: true }

                Btn {
                    kind: "icon"; iconName: "Back"; iconSize: 24; textColor: Theme.fgSoft; fillColor: Theme.surface
                    implicitWidth: 56; implicitHeight: 96; radius: 14
                    Accessible.name: "Previous page"; onClicked: page.go(-1)
                }

                PagePlaceholder {
                    readonly property real s: Math.min(1, parent.height / 684)
                    Layout.preferredWidth: 476 * s
                    Layout.preferredHeight: 684 * s
                    label: "Page " + page.current
                    pageUrl: controller.pages.length >= page.current ? controller.pages[page.current - 1] : ""
                }

                Btn {
                    kind: "icon"; iconName: "ChevronRight"; iconSize: 24; textColor: Theme.fgSoft; fillColor: Theme.surface
                    implicitWidth: 56; implicitHeight: 96; radius: 14
                    Accessible.name: "Next page"; onClicked: page.go(1)
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 24 }
                width: Math.min(720, parent.width - 48)
                height: 58
                radius: height / 2
                color: "#F00E0E0E"
                border.color: Theme.dividerStrong

                RowLayout {
                    anchors { fill: parent; margins: 8 }
                    spacing: 12

                    Btn {
                        kind: "ghost"; size: "pill"; text: "Prev Ch."; borderColor: "transparent"; fillColor: Theme.surfaceHigh; implicitHeight: 40
                        enabled: controller.prevChapterId.length > 0
                        onClicked: {
                            if (controller.prevChapterId.length > 0) {
                                const prov = page.manga.provider || page.manga.source || ""
                                const mId = page.manga.id || page.manga.mangaId || ""
                                controller.loadChapter(prov, mId, controller.prevChapterId, page.manga.title || "", page.manga.cover || "")
                            }
                        }
                    }

                    Txt { text: page.current; Layout.minimumWidth: 24; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 13; font.weight: Font.ExtraBold }

                    MdSlider {
                        Layout.fillWidth: true
                        from: 1
                        to: Math.max(1, page.total)
                        value: page.current
                        onMoved: {
                            page.current = Math.round(value)
                            controller.currentPage = page.current
                            saveTimer.restart()
                        }
                        Accessible.name: "Page"
                    }

                    Txt { text: page.total; Layout.minimumWidth: 24; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 13; font.weight: Font.Bold; color: Theme.fgSubtle }

                    Btn {
                        size: "pill"; text: "Next Ch."; implicitHeight: 40
                        enabled: controller.nextChapterId.length > 0
                        onClicked: {
                            if (controller.nextChapterId.length > 0) {
                                const prov = page.manga.provider || page.manga.source || ""
                                const mId = page.manga.id || page.manga.mangaId || ""
                                controller.loadChapter(prov, mId, controller.nextChapterId, page.manga.title || "", page.manga.cover || "")
                            }
                        }
                    }
                }
            }
        }
    }

    // ===== Settings panel =====
    Rectangle {
        id: panel
        visible: page.showSettings
        anchors { right: parent.right; top: topBar.bottom; bottom: parent.bottom }
        width: 340
        color: Theme.panel
        Rectangle { width: 1; height: parent.height; color: Theme.dividerStrong }

        PageScroll {
            anchors.fill: parent
            padding: 24
            topPadding: 24
            bottomPadding: 24

            DisplayText { text: "Reader"; font.pixelSize: 24 }

            Column {
                width: parent.width
                spacing: 10
                Eyebrow { text: "Reading mode"; muted: true }
                ButtonGroup { id: mode }
                Grid {
                    width: parent.width
                    columns: 2
                    spacing: 8
                    Repeater {
                        model: ["LTR pager", "RTL pager", "Webtoon", "Vertical"]
                        OptionButton {
                            required property string modelData
                            width: (parent.width - 8) / 2
                            text: modelData
                            checked: modelData === (AppSettings.readerMode || "RTL pager")
                            ButtonGroup.group: mode
                            onClicked: AppSettings.readerMode = modelData
                        }
                    }
                }
            }

            Column {
                width: parent.width
                Item {
                    width: parent.width
                    height: 44
                    Txt { text: "Dual-page spread"; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
                    MdSwitch {
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        Accessible.name: "Dual-page spread"
                        checked: page.spread
                        onToggled: page.layout = checked ? "spread" : "single"
                    }
                }
                Item {
                    width: parent.width
                    height: 44
                    Txt { text: "Crop borders"; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
                    MdSwitch {
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        Accessible.name: "Crop borders"
                        checked: AppSettings.readerCropBorders
                        onToggled: AppSettings.readerCropBorders = checked
                    }
                }
            }

            Column {
                width: parent.width
                spacing: 14
                Eyebrow { text: "Image filters"; muted: true }

                Column {
                    width: parent.width
                    spacing: 4
                    Item {
                        width: parent.width; height: 18
                        Txt { text: "Brightness"; font.pixelSize: 13 }
                        Txt { anchors.right: parent.right; text: Math.round(bSlider.value) + "%"; font.pixelSize: 13; color: Theme.fgSubtle }
                    }
                    MdSlider {
                        id: bSlider
                        width: parent.width; from: 50; to: 150
                        value: AppSettings.readerBrightness || 100
                        Accessible.name: "Brightness"
                        onMoved: AppSettings.readerBrightness = Math.round(value)
                    }
                }

                Column {
                    width: parent.width
                    spacing: 4
                    Item {
                        width: parent.width; height: 18
                        Txt { text: "Contrast"; font.pixelSize: 13 }
                        Txt { anchors.right: parent.right; text: Math.round(cSlider.value) + "%"; font.pixelSize: 13; color: Theme.fgSubtle }
                    }
                    MdSlider {
                        id: cSlider
                        width: parent.width; from: 50; to: 150
                        value: AppSettings.readerContrast || 100
                        Accessible.name: "Contrast"
                        onMoved: AppSettings.readerContrast = Math.round(value)
                    }
                }

                Row {
                    spacing: 8
                    Chip { small: true; text: "Grayscale"; checked: AppSettings.readerGrayscale; onClicked: AppSettings.readerGrayscale = !AppSettings.readerGrayscale }
                    Chip { small: true; text: "Invert"; checked: AppSettings.readerInvert; onClicked: AppSettings.readerInvert = !AppSettings.readerInvert }
                }
            }
        }
    }
}
