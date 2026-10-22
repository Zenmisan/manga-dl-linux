import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// One source's catalogue. Open with { source: "MangaDex" }.
Item {
    id: page
    property string source: "MangaDex"
    property string tab: "popular"

    BrowseSourceController {
        id: controller
    }

    Component.onCompleted: {
        if (page.tab === "latest" || page.tab === "popular") {
            controller.setBrowseMode(page.tab)
        }
        controller.load(page.source)
    }

    component FilterChip: Rectangle {
        property string label
        height: 32
        width: row.implicitWidth + 16
        radius: 16
        color: Theme.accentMuted
        Row {
            id: row
            x: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            Txt { text: label; font.pixelSize: 12; font.weight: Font.Bold; color: Theme.accentSoft; anchors.verticalCenter: parent.verticalCenter }
            Btn {
                kind: "icon"; iconName: "Close"; iconSize: 12; textColor: Theme.accentSoft
                implicitHeight: 24; radius: 12
                Accessible.name: "Remove " + label + " filter"
                onClicked: parent.parent.visible = false
            }
        }
    }

    PageScroll {
        anchors.fill: parent
        topPadding: 24
        spacing: 18

        Btn {
            kind: "link"; text: "Browse"; iconName: "Back"; iconSize: 14; textColor: Theme.fgMuted
            fontSize: 13; leftPadding: 0; onClicked: Nav.back()
        }

        // Header (Responsive wrapping layout)
        Flow {
            width: parent.width
            spacing: 10

            PageTitle {
                text: controller.sourceName
                width: parent.width < 620 ? parent.width : undefined
            }

            Item {
                visible: parent.width >= 620
                width: Math.max(0, parent.width - 550)
                height: 1
            }

            SearchField {
                id: searchInput
                width: parent.width < 620 ? Math.max(160, parent.width - 170) : 260
                placeholderText: "Search " + controller.sourceName
                onAccepted: controller.search(text)
            }

            Btn {
                kind: "ghost"
                text: parent.width < 540 ? "Browser" : "Open in Browser"
                iconName: "Globe"
                onClicked: controller.openInBrowser()
            }
        }

        Flow {
            width: parent.width
            spacing: 8
            ButtonGroup { id: order }
            Chip {
                text: "Popular"
                checked: controller.mode === "popular"
                ButtonGroup.group: order
                onClicked: controller.setBrowseMode("popular")
            }
            Chip {
                text: "Latest"
                checked: controller.mode === "latest"
                ButtonGroup.group: order
                onClicked: controller.setBrowseMode("latest")
            }
            Rectangle { width: 1; height: 24; color: Theme.track; anchors.verticalCenter: parent.verticalCenter; visible: parent.width >= 400 }
            FilterChip { label: "Ongoing"; anchors.verticalCenter: parent.verticalCenter }
        }

        // Empty state
        Column {
            width: parent.width
            spacing: 12
            visible: !controller.loading && controller.items.length === 0
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
                text: "No manga found"
                font.pixelSize: 18
            }
            BodyMuted {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "No results returned from " + controller.sourceName + "."
            }
        }

        // 8 across at 1440 px
        AutoGrid {
            width: parent.width
            minItemWidth: 120
            spacing: 18
            model: controller.items
            delegate: ResultTile { required property var modelData; width: parent.cellWidth; manga: modelData; dimInLibrary: true }
        }

        // Loading indicator
        Item {
            width: parent.width
            height: 48
            visible: controller.loading
            RowLayout {
                anchors.centerIn: parent
                spacing: 8
                Caption { text: "Loading " + controller.sourceName + "…" }
            }
        }

        // Load More button
        Btn {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !controller.loading && controller.hasMore && controller.items.length > 0
            kind: "ghost"
            text: "Load More"
            implicitHeight: 40
            onClicked: controller.loadMore()
        }
    }
}
