import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Web-novel reader: chapter list on the left, article in a comfortable column,
// "Reading Settings" popover for font, size and theme.
Rectangle {
    id: page
    property var manga: ({})
    property string chapterId: ""
    property string chapterTitle: ""
    property double chapterNumber: 0

    property string fontFamily: AppSettings.novelFontFamily || Theme.serif
    property int fontSize: AppSettings.novelFontSize || 20
    property color paper: AppSettings.novelTheme === "Black" ? "#000000" : (AppSettings.novelTheme === "Sepia" ? "#E8DCC4" : (AppSettings.novelTheme === "Light" ? "#F5F5F2" : "#0D0D0D"))
    property color ink: AppSettings.novelTheme === "Sepia" ? "#3B2F22" : (AppSettings.novelTheme === "Light" ? "#1F1F1F" : "#DBFFFFFF")
    color: Theme.novelBg

    readonly property var themes: [
        { name: "Dark", bg: "#0D0D0D", fg: "#DBFFFFFF" },
        { name: "Black", bg: "#000000", fg: "#DBFFFFFF" },
        { name: "Sepia", bg: "#E8DCC4", fg: "#3B2F22" },
        { name: "Light", bg: "#F5F5F2", fg: "#1F1F1F" }
    ]

    readonly property var chaptersList: (page.manga && page.manga.chapters && page.manga.chapters.length > 0)
        ? page.manga.chapters
        : [{ id: page.chapterId, number: page.chapterNumber || page.chapterId, title: page.chapterTitle }]

    readonly property var paragraphs: {
        if (controller.novelText.length > 0) {
            return controller.novelText.split(/\n\s*\n/)
        }
        return []
    }

    ReaderController {
        id: controller
    }

    focus: true
    Keys.onEscapePressed: Nav.back()

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

    // Top bar
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
                text: "<b>" + (page.manga ? page.manga.title : "") + "</b><font color='" + Theme.fgSubtle + "'> · Ch. " + (page.chapterNumber > 0 ? page.chapterNumber : page.chapterId) + (page.chapterTitle.length > 0 ? (" · " + page.chapterTitle) : "") + "</font>"
                elide: Text.ElideRight
            }

            Btn {
                id: settingsButton
                kind: "ghost"; size: "small"
                text: "Reading Settings"
                Accessible.name: "Reading settings"
                onClicked: settings.opened ? settings.close() : settings.open()
                leftPadding: 40
                Txt { text: "Aa"; font.family: Theme.serif; font.pixelSize: 14; x: 14; anchors.verticalCenter: parent.verticalCenter }
            }
        }
    }

    Popup {
        id: settings
        parent: settingsButton
        x: settingsButton.width - width
        y: settingsButton.height + 8
        width: 380
        padding: 20
        background: Rectangle { radius: 16; color: Theme.panel; border.color: Theme.track }

        contentItem: ColumnLayout {
            spacing: 18

            ColumnLayout {
                spacing: 10
                Eyebrow { text: "Font"; muted: true }
                RowLayout {
                    spacing: 8
                    ButtonGroup { id: fontGroup }
                    OptionButton {
                        Layout.fillWidth: true
                        text: "Serif"
                        font.family: Theme.serif
                        font.pixelSize: 15
                        checked: page.fontFamily === Theme.serif
                        ButtonGroup.group: fontGroup
                        onClicked: {
                            page.fontFamily = Theme.serif
                            AppSettings.novelFontFamily = Theme.serif
                        }
                    }
                    OptionButton {
                        Layout.fillWidth: true
                        text: "Sans"
                        checked: page.fontFamily === Theme.body
                        ButtonGroup.group: fontGroup
                        onClicked: {
                            page.fontFamily = Theme.body
                            AppSettings.novelFontFamily = Theme.body
                        }
                    }
                }
            }

            ColumnLayout {
                spacing: 6
                RowLayout {
                    Eyebrow { text: "Size"; muted: true; Layout.fillWidth: true }
                    Caption { text: page.fontSize + "px" }
                }
                MdSlider {
                    Layout.fillWidth: true; from: 14; to: 28; value: page.fontSize
                    Accessible.name: "Text size"
                    onMoved: {
                        page.fontSize = Math.round(value)
                        AppSettings.novelFontSize = page.fontSize
                    }
                }
            }

            ColumnLayout {
                spacing: 10
                Eyebrow { text: "Theme"; muted: true }
                Row {
                    spacing: 10
                    Repeater {
                        model: page.themes
                        AbstractButton {
                            required property var modelData
                            width: 44; height: 44
                            Accessible.name: modelData.name + " theme"
                            onClicked: {
                                page.paper = modelData.bg
                                page.ink = modelData.fg
                                AppSettings.novelTheme = modelData.name
                            }
                            background: Rectangle {
                                radius: 22
                                color: modelData.bg
                                border.width: Qt.colorEqual(page.paper, modelData.bg) ? 2 : 1
                                border.color: Qt.colorEqual(page.paper, modelData.bg) ? Theme.accent : "#33FFFFFF"
                            }
                        }
                    }
                }
            }
        }
    }

    // Chapter list
    Rectangle {
        id: list
        anchors { left: parent.left; top: topBar.bottom; bottom: parent.bottom }
        width: 300
        color: "#0A0A0A"
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.dividerStrong }

        PageScroll {
            anchors.fill: parent
            padding: 12
            topPadding: 20
            spacing: 2
            Eyebrow { text: "Chapters"; muted: true; leftPadding: 12; bottomPadding: 10 }

            Repeater {
                model: page.chaptersList
                NavItem {
                    required property var modelData
                    width: parent.width
                    height: 42
                    text: "Ch. " + (modelData.number || modelData.id) + (modelData.title ? (" · " + modelData.title) : "")
                    font.pixelSize: 13
                    selected: modelData.id === page.chapterId
                    onClicked: {
                        page.chapterId = modelData.id
                        page.chapterTitle = modelData.title || ""
                        page.chapterNumber = modelData.rawNumber || modelData.number || 0
                        const prov = page.manga.provider || page.manga.source || ""
                        const mId = page.manga.id || page.manga.mangaId || ""
                        controller.loadChapter(prov, mId, page.chapterId, page.manga.title || "", page.manga.cover || "", page.chapterTitle, page.chapterNumber)
                    }
                }
            }
        }
    }

    // Article
    Rectangle {
        anchors { left: list.right; right: parent.right; top: topBar.bottom; bottom: parent.bottom }
        color: page.paper

        PageScroll {
            anchors.fill: parent
            padding: Math.max(40, (width - 680) / 2)
            topPadding: 56
            bottomPadding: 56
            spacing: 22

            Txt {
                text: "Chapter " + (page.chapterNumber > 0 ? page.chapterNumber : page.chapterId) + (page.chapterTitle.length > 0 ? (" · " + page.chapterTitle) : "")
                font.pixelSize: 13; font.weight: Font.ExtraBold; font.letterSpacing: 2
                font.capitalization: Font.AllUppercase; color: Theme.fgSubtle; bottomPadding: 6
            }

            // Loading state
            Column {
                width: parent.width
                spacing: 12
                visible: controller.loading
                Icon { name: "Refresh"; size: 24; color: Theme.accentLight; anchors.horizontalCenter: parent.horizontalCenter }
                Txt { text: "Loading chapter text..."; color: Theme.fgMuted; anchors.horizontalCenter: parent.horizontalCenter }
            }

            // Error state
            Column {
                width: parent.width
                spacing: 12
                visible: !controller.loading && controller.error.length > 0 && page.paragraphs.length === 0
                Txt { text: "Failed to load chapter: " + controller.error; color: Theme.danger; anchors.horizontalCenter: parent.horizontalCenter }
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

            // Paragraphs
            Repeater {
                model: page.paragraphs
                Text {
                    required property string modelData
                    width: parent.width
                    text: modelData
                    wrapMode: Text.Wrap
                    font.family: page.fontFamily
                    font.pixelSize: page.fontSize
                    lineHeightMode: Text.FixedHeight
                    lineHeight: Math.round(page.fontSize * 1.75)
                    color: page.ink
                }
            }
        }
    }
}
