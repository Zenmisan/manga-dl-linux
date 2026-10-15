import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Library: continue-reading cards, category chips and the cover grid.
Item {
    id: libraryRoot

    PageScroll {
        anchors.fill: parent

        // Header
        RowLayout {
            width: parent.width
            spacing: 16
            PageTitle { text: "Library"; Layout.fillWidth: true }
            SearchField {
                Layout.preferredWidth: 320
                placeholderText: "Search library   Ctrl K"
                text: LibraryModel.searchFilter
                onTextChanged: LibraryModel.searchFilter = text
            }
            Btn {
                kind: "ghost"
                text: {
                    switch (LibraryModel.sortMode) {
                    case 1: return "Sort: Title"
                    case 2: return "Sort: Unread"
                    default: return "Sort: Last read"
                    }
                }
                iconName: "Sort"
                onClicked: LibraryModel.sortMode = (LibraryModel.sortMode + 1) % 3
            }
            Btn {
                text: "Update Library"
                iconName: "Refresh"
                onClicked: {
                    LibraryModel.refresh()
                    Nav.toast("Checking library for new chapters")
                }
            }
        }

        // Continue reading
        Column {
            width: parent.width
            spacing: 12
            visible: LibraryModel.continueReading.length > 0

            Eyebrow { text: "Continue reading" }
            AutoGrid {
                width: parent.width
                minItemWidth: 280
                maxColumns: 3
                spacing: 16
                model: LibraryModel.continueReading
                delegate: Clickable {
                    required property var modelData
                    width: parent.cellWidth
                    height: 110
                    radius: 16
                    fill: Theme.surfaceRaised
                    Accessible.name: "Resume " + (modelData.manga ? modelData.manga.title : "")
                    onClicked: Nav.open("Reader", { manga: modelData.manga })
                    Rectangle {   // border
                        width: parent.width; height: parent.height; radius: 16
                        color: "transparent"; border.color: Theme.dividerStrong
                    }
                    RowLayout {
                        width: parent.width; height: parent.height
                        anchors.margins: 12
                        spacing: 14
                        CoverArt {
                            Layout.leftMargin: 12
                            width: 56; height: 84; radius: 8
                            cover: modelData.manga && modelData.manga.cover ? modelData.manga.cover : "#1A2433"
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.rightMargin: 12
                            spacing: 6
                            ItemTitle { Layout.fillWidth: true; text: modelData.manga ? modelData.manga.title : "" }
                            Caption { Layout.fillWidth: true; text: modelData.where || "" }
                            Progress { Layout.fillWidth: true; value: modelData.progress || 0 }
                        }
                    }
                }
            }
        }

        // Categories
        Row {
            spacing: 8
            ButtonGroup { id: categories }
            Repeater {
                model: CategoryModel.categoryChips
                Chip {
                    required property string modelData
                    required property int index
                    text: modelData
                    checked: index === 0
                    ButtonGroup.group: categories
                    onClicked: {
                        if (index === 0) {
                            LibraryModel.categoryFilter = ""
                        } else {
                            const names = CategoryModel.names
                            if (index - 1 < names.length) {
                                LibraryModel.categoryFilter = names[index - 1]
                            }
                        }
                    }
                }
            }
        }

        // Covers: 7 across at 1440 px, reflows with the window.
        AutoGrid {
            width: parent.width
            minItemWidth: 140
            spacing: 20
            visible: LibraryModel.count > 0
            model: LibraryModel
            delegate: CoverTile {
                required property var modelData
                width: parent.cellWidth
                manga: modelData
            }
        }

        // Empty state
        Column {
            width: parent.width
            spacing: 16
            visible: LibraryModel.count === 0
            topPadding: 48
            bottomPadding: 48

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: "Bookmark"
                size: 48
                color: Theme.fgFaint
            }
            DisplayText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: LibraryModel.searchFilter.length > 0 ? "No matching manga found" : "Your library is empty"
                font.pixelSize: 22
                color: Theme.fgSoft
            }
            Txt {
                anchors.horizontalCenter: parent.horizontalCenter
                text: LibraryModel.searchFilter.length > 0
                    ? "Try a different search term or clear the filter."
                    : "Discover manga and novels from your favourite sources to add them here."
                color: Theme.fgMuted
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 12
                Btn {
                    text: "Discover Titles"
                    iconName: "Search"
                    onClicked: Nav.open("Search")
                }
                Btn {
                    kind: "ghost"
                    text: "Clear Search"
                    visible: LibraryModel.searchFilter.length > 0
                    onClicked: LibraryModel.searchFilter = ""
                }
            }
        }
    }
}
