import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Global search across sources, responsive for all window sizes and Wayland.
Item {
    id: root
    property bool showFilters: width >= 1150

    SearchController {
        id: controller
    }

    readonly property var currentTabSources: {
        const all = controller.availableSources
        if (controller.activeTab === 0) {
            return all.filter(s => s.type !== "novel")
        } else if (controller.activeTab === 1) {
            return all.filter(s => s.type === "novel")
        }
        return all
    }

    PageScroll {
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            right: filters.visible ? filters.left : parent.right
        }
        spacing: 16

        // Search Input Header
        RowLayout {
            width: parent.width
            spacing: 12

            SearchField {
                id: search
                Layout.fillWidth: true
                large: true
                placeholderText: "Search all sources..."
                onAccepted: controller.search(text)
            }

            Btn {
                kind: "ghost"
                iconName: "Sliders"
                text: "Filters"
                implicitHeight: 48
                onClicked: root.showFilters = !root.showFilters
            }
        }

        // Status text / count
        RowLayout {
            width: parent.width
            visible: controller.statusText.length > 0
            spacing: 8
            Caption {
                text: controller.statusText
                font.pixelSize: 13
                color: Theme.fgSubtle
            }
            Item { Layout.fillWidth: true }
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

    // Collapsible Filters Panel
    Rectangle {
        id: filters
        visible: root.showFilters
        width: Math.min(280, root.width * 0.35)
        anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
        color: Theme.navBg
        Rectangle { width: 1; height: parent.height; color: Theme.dividerStrong }

        Flickable {
            anchors.fill: parent
            contentHeight: filterCol.implicitHeight + 48
            clip: true

            ColumnLayout {
                id: filterCol
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 20; topMargin: 24 }
                spacing: 18

                RowLayout {
                    Layout.fillWidth: true
                    SectionTitle { text: "Search filters"; Layout.fillWidth: true }
                    Btn {
                        kind: "icon"; iconName: "Close"; iconSize: 14
                        onClicked: root.showFilters = false
                    }
                }

                // Sources list filtered by active tab
                ColumnLayout {
                    spacing: 4
                    Eyebrow { text: "Sources"; muted: true; Layout.bottomMargin: 4 }
                    Repeater {
                        model: root.currentTabSources.slice(0, 10)
                        delegate: CheckRow {
                            required property var modelData
                            text: modelData.name
                            checked: controller.selectedSources.length === 0 || controller.selectedSources.indexOf(modelData.id) !== -1
                            onToggled: controller.toggleSource(modelData.id)
                        }
                    }
                }

                // Status
                ColumnLayout {
                    spacing: 8
                    Eyebrow { text: "Status"; muted: true }
                    Row {
                        spacing: 6
                        ButtonGroup { id: status }
                        Chip {
                            small: true; leftPadding: 10; text: "Any"
                            checked: controller.selectedStatus === "Any"
                            ButtonGroup.group: status
                            onClicked: controller.selectedStatus = "Any"
                        }
                        Chip {
                            small: true; leftPadding: 10; text: "Ongoing"
                            checked: controller.selectedStatus === "Ongoing"
                            ButtonGroup.group: status
                            onClicked: controller.selectedStatus = "Ongoing"
                        }
                        Chip {
                            small: true; leftPadding: 10; text: "Completed"
                            checked: controller.selectedStatus === "Completed"
                            ButtonGroup.group: status
                            onClicked: controller.selectedStatus = "Completed"
                        }
                    }
                }

                // Sort By
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Eyebrow { text: "Sort by"; muted: true }
                    MdCombo {
                        Layout.fillWidth: true
                        model: ["Relevance", "Latest update", "Title A–Z"]
                        Accessible.name: "Sort by"
                        onActivated: index => controller.sortBy = model[index]
                    }
                }

                // Buttons
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    spacing: 8
                    Btn {
                        Layout.fillWidth: true
                        kind: "ghost"
                        text: "Reset"
                        implicitHeight: 40
                        onClicked: controller.resetFilters()
                    }
                    Btn {
                        Layout.fillWidth: true
                        text: "Apply"
                        implicitHeight: 40
                        onClicked: controller.search(search.text)
                    }
                }
            }
        }
    }
}
