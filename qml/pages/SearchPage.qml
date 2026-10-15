import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Global search across sources, with a filter panel on the right.
Item {
    id: root

    SearchController {
        id: controller
    }

    PageScroll {
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom; right: filters.left }
        spacing: 16

        Item {
            width: parent.width
            height: search.height
            SearchField {
                id: search
                width: parent.width
                large: true
                placeholderText: "Search all sources"
                onAccepted: controller.search(text)
            }
            Caption {
                text: controller.statusText
                anchors { right: parent.right; rightMargin: 18; verticalCenter: parent.verticalCenter }
            }
        }

        Tabs {
            width: parent.width
            model: ["Manga", "Web Novels", "Readers"]
            currentIndex: controller.activeTab
            onActivated: index => controller.activeTab = index
        }

        // Empty state
        Column {
            width: parent.width
            spacing: 12
            visible: !controller.searching && controller.groups.length === 0 && search.text.length > 0
            topPadding: 48
            bottomPadding: 48

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: "Search"
                size: 48
                color: Theme.fgSubtle
            }
            DisplayText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "No results found"
                font.pixelSize: 18
            }
            BodyMuted {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Try searching for another title or adjust your source filters."
            }
        }

        // Initial welcome state
        Column {
            width: parent.width
            spacing: 12
            visible: !controller.searching && controller.groups.length === 0 && search.text.length === 0
            topPadding: 48
            bottomPadding: 48

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: "Browse"
                size: 48
                color: Theme.fgSubtle
            }
            DisplayText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Discover Manga & Novels"
                font.pixelSize: 18
            }
            BodyMuted {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Type a title above and press Enter to search across all enabled sources."
            }
        }

        // Search Results
        Repeater {
            model: controller.groups
            delegate: Column {
                required property var modelData
                width: parent.width
                spacing: 10
                bottomPadding: 8
                RowLayout {
                    width: parent.width
                    spacing: 12
                    SectionTitle { text: modelData.source }
                    Caption { text: modelData.meta; Layout.fillWidth: true }
                    Btn {
                        kind: "link"
                        text: "See All"
                        fontSize: 13
                        onClicked: Nav.open("BrowseSource", { source: modelData.sourceId || modelData.source })
                    }
                }
                AutoGrid {
                    width: parent.width
                    minItemWidth: 110
                    maxColumns: 7
                    spacing: 16
                    model: modelData.results
                    delegate: ResultTile { required property var modelData; width: parent.cellWidth; manga: modelData }
                }
            }
        }

        // Still searching skeleton
        Column {
            width: parent.width
            spacing: 10
            visible: controller.searching
            RowLayout {
                width: parent.width
                SectionTitle { text: "Searching sources…"; Layout.fillWidth: true }
                Caption { text: "Pumping network…" }
            }
            Row {
                spacing: 16
                Repeater {
                    model: 6
                    Rectangle {
                        width: 110; height: 165; radius: 10
                        color: Theme.surfaceRaised
                        NumberAnimation on opacity {
                            from: 0.4; to: 0.9; duration: 800; loops: Animation.Infinite; running: controller.searching
                        }
                    }
                }
            }
        }
    }

    // Filters
    Rectangle {
        id: filters
        width: 300
        anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
        color: Theme.navBg
        Rectangle { width: 1; height: parent.height; color: Theme.dividerStrong }

        ColumnLayout {
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 24; topMargin: 28 }
            spacing: 20
            SectionTitle { Layout.fillWidth: true; text: "Search filters" }
            ColumnLayout {
                spacing: 2
                Eyebrow { text: "Sources"; muted: true; Layout.bottomMargin: 6 }
                Repeater {
                    model: controller.availableSources.slice(0, 6)
                    delegate: CheckRow {
                        required property var modelData
                        text: modelData.name
                        checked: controller.selectedSources.length === 0 || controller.selectedSources.indexOf(modelData.id) !== -1
                        onToggled: controller.toggleSource(modelData.id)
                    }
                }
            }
            ColumnLayout {
                spacing: 10
                Eyebrow { text: "Status"; muted: true }
                Row {
                    spacing: 6
                    ButtonGroup { id: status }
                    Chip {
                        small: true; leftPadding: 12; text: "Any"
                        checked: controller.selectedStatus === "Any"
                        ButtonGroup.group: status
                        onClicked: controller.selectedStatus = "Any"
                    }
                    Chip {
                        small: true; leftPadding: 12; text: "Ongoing"
                        checked: controller.selectedStatus === "Ongoing"
                        ButtonGroup.group: status
                        onClicked: controller.selectedStatus = "Ongoing"
                    }
                    Chip {
                        small: true; leftPadding: 12; text: "Completed"
                        checked: controller.selectedStatus === "Completed"
                        ButtonGroup.group: status
                        onClicked: controller.selectedStatus = "Completed"
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8
                Eyebrow { text: "Sort by"; muted: true }
                MdCombo {
                    Layout.fillWidth: true
                    model: ["Relevance", "Latest update", "Title A–Z"]
                    Accessible.name: "Sort by"
                    onActivated: index => controller.sortBy = model[index]
                }
            }
        }
        RowLayout {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 24; bottomMargin: 28 }
            spacing: 8
            Btn {
                Layout.fillWidth: true
                kind: "ghost"
                text: "Reset"
                implicitHeight: 44
                onClicked: controller.resetFilters()
            }
            Btn {
                Layout.fillWidth: true
                text: "Apply"
                implicitHeight: 44
                onClicked: controller.search(search.text)
            }
        }
    }
}
