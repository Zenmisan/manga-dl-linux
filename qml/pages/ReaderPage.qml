import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import MangaDl

// Manga reader: single page, spread, and webtoon continuous vertical scroll.
// Keys: ← → / Space turn pages, ↑ ↓ scroll webtoon, F fullscreen, Esc leaves fullscreen or closes.
Rectangle {
    id: page
    property var manga: ({})
    property string chapterId: ""
    property string chapterTitle: ""
    property double chapterNumber: 0

    property string layout: AppSettings.readerMode || "Single"
    readonly property bool isSpread: layout === "Spread"
    readonly property bool isWebtoon: layout === "Webtoon" || layout === "Vertical"
    readonly property bool isSingle: !isSpread && !isWebtoon

    property int current: controller.currentPage
    readonly property int total: controller.totalPages
    property bool bookmarked: false
    property bool showSettings: false

    ReaderController {
        id: controller
        onIsNovelChanged: {
            if (controller.isNovel) {
                Nav.replace("NovelReader", {
                    manga: page.manga,
                    chapterId: page.chapterId,
                    chapterTitle: page.chapterTitle,
                    chapterNumber: page.chapterNumber
                })
            }
        }
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

    color: Theme.readerBg || "#050505"
    focus: true

    Component.onCompleted: {
        forceActiveFocus()
        const prov = (page.manga && (page.manga.provider || page.manga.source || "")).toLowerCase()
        const isWebtoonSource = prov.indexOf("asura") >= 0 || prov.indexOf("flame") >= 0 || prov.indexOf("webtoon") >= 0 || prov.indexOf("omega") >= 0 || (page.manga && (page.manga.type === "webtoon" || page.manga.layout === "webtoon"))
        if (isWebtoonSource) {
            page.layout = "Webtoon"
            AppSettings.readerMode = "Webtoon"
        }

        if (page.manga) {
            let chId = page.chapterId
            let chTitle = page.chapterTitle
            let chNum = page.chapterNumber
            const rawProv = page.manga.provider || page.manga.source || ""
            const mId = page.manga.id || page.manga.mangaId || ""
            const mTitle = page.manga.title || ""
            const mCover = page.manga.cover || page.manga.coverUrl || ""

            if ((!chId || chId.length === 0) && page.manga.chapters && page.manga.chapters.length > 0) {
                const firstCh = page.manga.chapters[0]
                chId = firstCh.id || ""
                chTitle = firstCh.title || ""
                chNum = firstCh.rawNumber || firstCh.number || 0
                page.chapterId = chId
                page.chapterTitle = chTitle
                page.chapterNumber = chNum
            }

            if (chId && chId.length > 0) {
                controller.loadChapter(rawProv, mId, chId, mTitle, mCover, chTitle, chNum)
            }
        }
    }

    Component.onDestruction: {
        if (controller.totalPages > 0) {
            controller.saveProgress(page.current, page.total, page.current >= page.total)
        }
    }

    function go(delta) {
        if (total <= 0) return
        if (isWebtoon) {
            webtoonView.flick(0, -delta * 800)
            return
        }
        const step = isSpread ? 2 : 1
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
        if (e.key === Qt.Key_Right || e.key === Qt.Key_D || e.key === Qt.Key_L) {
            go(1); e.accepted = true
        } else if (e.key === Qt.Key_Left || e.key === Qt.Key_A || e.key === Qt.Key_H) {
            go(-1); e.accepted = true
        } else if (e.key === Qt.Key_Down || e.key === Qt.Key_J || e.key === Qt.Key_Space) {
            if (isWebtoon) { webtoonView.flick(0, -600); e.accepted = true }
            else { go(1); e.accepted = true }
        } else if (e.key === Qt.Key_Up || e.key === Qt.Key_K) {
            if (isWebtoon) { webtoonView.flick(0, 600); e.accepted = true }
            else { go(-1); e.accepted = true }
        } else if (e.key === Qt.Key_F) {
            toggleFullscreen(); e.accepted = true
        } else if (e.key === Qt.Key_Escape) {
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
                id: modeSegmented
                model: ["Single", "Spread", "Webtoon"]
                currentIndex: page.isWebtoon ? 2 : (page.isSpread ? 1 : 0)
                onActivated: index => {
                    const m = index === 2 ? "Webtoon" : (index === 1 ? "Spread" : "Single")
                    page.layout = m
                    AppSettings.readerMode = m
                }
            }
            Binding {
                target: modeSegmented
                property: "currentIndex"
                value: page.isWebtoon ? 2 : (page.isSpread ? 1 : 0)
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
                    ? (page.isSpread ? page.current + "–" + Math.min(page.current + 1, page.total) + " / " + page.total : page.current + " / " + page.total)
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

            Icon {
                name: "Refresh"; size: 28; color: Theme.accentLight; anchors.horizontalCenter: parent.horizontalCenter
                NumberAnimation on rotation { from: 0; to: 360; duration: 1000; loops: Animation.Infinite; running: controller.loading }
            }
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
            visible: page.isSpread && !controller.loading && controller.pages.length > 0
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
            visible: page.isSpread && !controller.loading && controller.pages.length > 0
            anchors.fill: parent
            onClicked: m => page.go(m.x < width / 2 ? 1 : -1)
        }

        // Single page layout
        Item {
            visible: page.isSingle && !controller.loading && controller.pages.length > 0
            anchors.fill: parent

            RowLayout {
                anchors { fill: parent; topMargin: 12; bottomMargin: 96; leftMargin: 16; rightMargin: 16 }
                spacing: 16

                Btn {
                    kind: "icon"; iconName: "Back"; iconSize: 24; textColor: Theme.fgSoft; fillColor: Theme.surface
                    implicitWidth: 48; implicitHeight: 80; radius: 12
                    Accessible.name: "Previous page"; onClicked: page.go(-1)
                }

                PagePlaceholder {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: "Page " + page.current
                    pageUrl: controller.pages.length >= page.current ? controller.pages[page.current - 1] : ""
                }

                Btn {
                    kind: "icon"; iconName: "ChevronRight"; iconSize: 24; textColor: Theme.fgSoft; fillColor: Theme.surface
                    implicitWidth: 48; implicitHeight: 80; radius: 12
                    Accessible.name: "Next page"; onClicked: page.go(1)
                }
            }
        }

        // Webtoon continuous vertical scroll layout
        ListView {
            id: webtoonView
            visible: page.isWebtoon && !controller.loading && controller.pages.length > 0
            anchors { fill: parent; bottomMargin: 72 }
            clip: true
            spacing: 0
            boundsBehavior: Flickable.StopAtBounds
            model: controller.pages
            delegate: Item {
                id: webtoonItem
                required property string modelData
                required property int index
                width: webtoonView.width
                height: webtoonImg.status === Image.Ready && webtoonImg.implicitWidth > 0
                    ? Math.round(webtoonInner.width * (webtoonImg.implicitHeight / webtoonImg.implicitWidth))
                    : Math.round(webtoonInner.width * 1.4)

                Item {
                    id: webtoonInner
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(parent.width, 860)
                    height: parent.height

                    Rectangle {
                        anchors.fill: parent
                        color: "#0C0C0E"
                        visible: webtoonImg.status !== Image.Ready
                        Column {
                            anchors.centerIn: parent
                            spacing: 8
                            Icon {
                                name: "Refresh"; size: 20; color: Theme.accentLight
                                NumberAnimation on rotation { from: 0; to: 360; duration: 1000; loops: Animation.Infinite; running: webtoonImg.status === Image.Loading }
                            }
                            Caption { text: "Page " + (index + 1) }
                        }
                    }

                    Image {
                        id: webtoonImg
                        anchors.fill: parent
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit
                        source: {
                            if (!modelData) return ""
                            if (modelData.startsWith("image://") || modelData.startsWith("file:") || modelData.startsWith("qrc:"))
                                return modelData
                            return "image://page/" + encodeURIComponent(modelData)
                        }
                    }
                }
            }
            onContentYChanged: {
                if (count > 0 && contentHeight > 0) {
                    const approxPage = Math.min(count, Math.max(1, Math.round((contentY / contentHeight) * count) + 1))
                    if (approxPage !== page.current) {
                        page.current = approxPage
                        controller.currentPage = approxPage
                        saveTimer.restart()
                    }
                }
            }
        }

        // Floating Bottom Scrubber (Visible in all modes)
        Rectangle {
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 20 }
            width: Math.min(720, parent.width - 48)
            height: 52
            radius: 26
            color: "#F00E0E0E"
            border.color: Theme.dividerStrong
            z: 3

            RowLayout {
                anchors { fill: parent; margins: 6; leftMargin: 12; rightMargin: 12 }
                spacing: 12

                Btn {
                    kind: "ghost"; size: "pill"; text: "Prev Ch."; borderColor: "transparent"; fillColor: Theme.surfaceHigh; implicitHeight: 36
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
                        if (page.isWebtoon && webtoonView.count > 0) {
                            webtoonView.positionViewAtIndex(page.current - 1, ListView.Beginning)
                        }
                        saveTimer.restart()
                    }
                    Accessible.name: "Page"
                }

                Txt { text: page.total; Layout.minimumWidth: 24; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 13; font.weight: Font.Bold; color: Theme.fgSubtle }

                Btn {
                    size: "pill"; text: "Next Ch."; implicitHeight: 36
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

    // ===== Settings panel =====
    Rectangle {
        id: panel
        visible: page.showSettings
        anchors { right: parent.right; top: topBar.bottom; bottom: parent.bottom }
        width: 340
        color: Theme.panel
        z: 4
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
                        model: ["Single", "Spread", "Webtoon", "Vertical"]
                        OptionButton {
                            required property string modelData
                            width: (parent.width - 8) / 2
                            text: modelData
                            checked: modelData === page.layout
                            ButtonGroup.group: mode
                            onClicked: {
                                page.layout = modelData
                                AppSettings.readerMode = modelData
                            }
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
                        checked: page.isSpread
                        onToggled: {
                            page.layout = checked ? "Spread" : "Single"
                            AppSettings.readerMode = page.layout
                        }
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
