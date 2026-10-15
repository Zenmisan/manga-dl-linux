import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Browse: Sources / Extensions / Migrate tabs. Open with { tab: "sources" | "migrate" }.
Item {
    id: page
    property string tab: "extensions"
    readonly property int tabIndex: tab === "sources" ? 0 : tab === "migrate" ? 2 : 1

    property var selectedItemToMigrate: null
    property var selectedCandidate: null

    ExtensionsController {
        id: controller
    }

    function extStatus(s) { return s === "update" ? "Update available" : s === "installed" ? "Installed" : s === "broken" ? "Failed to load" : "Not installed" }
    function extStatusColor(s) { return s === "update" ? Theme.accentSoft : s === "installed" ? Theme.successText : s === "broken" ? Theme.errorText : Theme.fgSubtle }
    function extAction(s) { return s === "update" ? "Update" : s === "installed" ? "Uninstall" : s === "broken" ? "Reinstall" : "Install" }

    component SourceRow: Item {
        property var source
        Layout.fillWidth: true
        width: parent ? parent.width : 0
        height: 62
        HairLine { width: parent.width }
        RowLayout {
            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
            spacing: 14
            Initial { size: 40; text: source.initial; color: source.color; radius: 10 }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Txt { Layout.fillWidth: true; text: source.name; font.pixelSize: 15; font.weight: Font.Bold }
                Caption { Layout.fillWidth: true; text: source.meta }
            }
            Btn { kind: "ghost"; size: "pill"; text: "Popular"; onClicked: Nav.open("BrowseSource", { source: source.id || source.name, tab: "popular" }) }
            Btn { kind: "ghost"; size: "pill"; text: "Latest"; onClicked: Nav.open("BrowseSource", { source: source.id || source.name, tab: "latest" }) }
            Btn {
                kind: "icon"
                iconName: "Pin"
                iconSize: 18
                textColor: source.pinned ? Theme.accentLight : Theme.fgSubtle
                Accessible.name: source.pinned ? "Unpin source" : "Pin source"
                onClicked: controller.togglePinSource(source.id)
            }
        }
    }

    PageScroll {
        anchors.fill: parent
        spacing: 18

        RowLayout {
            width: parent.width
            spacing: 12
            PageTitle { text: "Browse"; Layout.fillWidth: true }
            // Looks like a search field; opens global search.
            Clickable {
                implicitWidth: 340; implicitHeight: 42
                radius: 12
                fill: Theme.surfaceRaised
                Accessible.name: "Search all sources"
                onClicked: Nav.open("Search")
                Rectangle { anchors.fill: parent; radius: 12; color: "transparent"; border.color: Theme.track }
                Row {
                    x: 14; anchors.verticalCenter: parent.verticalCenter
                    spacing: 10
                    Icon { name: "Search"; size: 18; color: Theme.fgSubtle }
                    Txt { text: "Search all sources"; color: Theme.fgSubtle; anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }

        Tabs {
            width: parent.width
            model: ["Sources · " + controller.sources.length, "Extensions", "Migrate"]
            currentIndex: page.tabIndex
            onActivated: index => page.tab = ["sources", "extensions", "migrate"][index]
        }

        // ===== Sources =====
        Column {
            visible: page.tab === "sources"
            width: parent.width
            Eyebrow { text: "Last used"; topPadding: 4; bottomPadding: 4 }
            SourceRow { source: controller.lastUsedSource }
            Eyebrow { text: "All sources"; muted: true; topPadding: 18; bottomPadding: 4 }
            Repeater {
                model: controller.sources
                delegate: SourceRow {
                    required property var modelData
                    source: modelData
                }
            }
        }

        // ===== Extensions =====
        Column {
            visible: page.tab === "extensions"
            width: parent.width
            spacing: 18
            RowLayout {
                width: parent.width
                spacing: 8
                ButtonGroup { id: lang }
                Chip {
                    small: true; text: "English"
                    checked: controller.languageFilter === "English"
                    ButtonGroup.group: lang
                    onClicked: controller.languageFilter = "English"
                }
                Chip {
                    small: true; text: "All languages"
                    checked: controller.languageFilter === "All languages"
                    ButtonGroup.group: lang
                    onClicked: controller.languageFilter = "All languages"
                }
                Chip {
                    small: true; text: "Show 18+"
                    checked: controller.show18Plus
                    onClicked: controller.show18Plus = !controller.show18Plus
                }
                Item { Layout.fillWidth: true }
                Txt {
                    text: "Index: manga-dl · " + controller.installedCount + " installed"
                    font.pixelSize: 13
                    color: Theme.fgSubtle
                    Layout.rightMargin: 8
                }
                Btn {
                    text: "Update All"
                    implicitHeight: 38
                    radius: 10
                    onClicked: {
                        controller.updateAll()
                        Nav.toast("All extensions are up to date")
                    }
                }
            }
            Column {
                width: parent.width
                RowLayout {
                    width: parent.width
                    height: 36
                    spacing: 16
                    TableHeader { text: "Extension"; Layout.fillWidth: true; Layout.leftMargin: 12 }
                    TableHeader { text: "Language"; Layout.preferredWidth: 120 }
                    TableHeader { text: "Version"; Layout.preferredWidth: 140 }
                    TableHeader { text: "Status"; Layout.preferredWidth: 160 }
                    TableHeader { text: "Action"; Layout.preferredWidth: 140; horizontalAlignment: Text.AlignRight; Layout.rightMargin: 12 }
                }
                Repeater {
                    model: controller.extensions
                    delegate: Item {
                        required property var modelData
                        width: parent.width
                        height: 56
                        HairLine { width: parent.width }
                        RowLayout {
                            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                            spacing: 16
                            Row {
                                Layout.fillWidth: true
                                spacing: 12
                                Rectangle {
                                    width: 36; height: 36; radius: 9; color: modelData.color
                                    DisplayText { anchors.centerIn: parent; text: modelData.initial; font.pixelSize: 15 }
                                }
                                Txt { text: modelData.name; font.weight: Font.Bold; anchors.verticalCenter: parent.verticalCenter }
                            }
                            Txt { text: modelData.language; font.pixelSize: 13; color: Theme.fgSoft; Layout.preferredWidth: 120 }
                            Txt { text: modelData.version; font.pixelSize: 13; color: Theme.fgSoft; Layout.preferredWidth: 140 }
                            Txt { text: page.extStatus(modelData.state); font.pixelSize: 12; font.weight: Font.Bold; color: page.extStatusColor(modelData.state); Layout.preferredWidth: 160 }
                            Item {
                                Layout.preferredWidth: 140
                                implicitHeight: 36
                                Btn {
                                    anchors.right: parent.right
                                    size: "small"
                                    kind: modelData.state === "update" ? "primary" : modelData.state === "broken" ? "danger" : "ghost"
                                    text: page.extAction(modelData.state)
                                    onClicked: {
                                        controller.updateExtension(modelData.id)
                                        Nav.toast(modelData.name + " is up to date")
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ===== Migrate =====
        Column {
            visible: page.tab === "migrate"
            width: parent.width
            spacing: 8
            BodyMuted { width: parent.width; text: "Move a manga to another source and keep its read chapters, categories and tracking."; bottomPadding: 8 }

            // Empty library check
            Column {
                width: parent.width
                spacing: 12
                visible: controller.libraryItems.length === 0
                topPadding: 32
                bottomPadding: 32

                Icon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    name: "Library"
                    size: 40
                    color: Theme.fgSubtle
                }
                DisplayText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Library is empty"
                    font.pixelSize: 16
                }
                BodyMuted {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Add titles to your library first to enable source migration."
                }
            }

            Column {
                width: parent.width
                visible: controller.libraryItems.length > 0
                Repeater {
                    model: controller.libraryItems
                    delegate: Item {
                        required property var modelData
                        width: parent.width
                        height: 70
                        HairLine { width: parent.width }
                        RowLayout {
                            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                            spacing: 16
                            CoverArt { width: 36; height: 54; radius: 6; cover: modelData.cover }
                            Txt { text: modelData.title; font.weight: Font.Bold; Layout.fillWidth: true }
                            Txt { text: modelData.source; font.pixelSize: 13; color: Theme.fgSubtle; Layout.preferredWidth: 200 }
                            Btn {
                                kind: "ghost"
                                size: "small"
                                text: "Migrate"
                                onClicked: {
                                    page.selectedItemToMigrate = modelData
                                    page.selectedCandidate = null
                                    // Search candidate in a default target source
                                    const defaultTarget = (modelData.provider === "mangadex") ? "mangakatana" : "mangadex"
                                    targetSourceCombo.currentIndex = 0
                                    controller.searchMigrationCandidates(defaultTarget, modelData.title)
                                    migrationModal.visible = true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Migration Modal
    Rectangle {
        id: migrationModal
        anchors.fill: parent
        color: "#99000000"
        visible: false
        z: 100

        MouseArea {
            anchors.fill: parent
            onClicked: migrationModal.visible = false
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(680, parent.width - 48)
            height: Math.min(600, parent.height - 48)
            radius: 16
            color: Theme.surfaceRaised
            border.color: Theme.dividerStrong

            MouseArea {
                anchors.fill: parent // prevent clicks from closing modal
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true
                    SectionTitle { text: "Migrate Manga Source"; Layout.fillWidth: true }
                    Btn {
                        kind: "icon"
                        iconName: "Close"
                        iconSize: 16
                        onClicked: migrationModal.visible = false
                    }
                }

                HairLine { Layout.fillWidth: true }

                // Source Item info
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    CoverArt {
                        width: 44; height: 66; radius: 8
                        cover: page.selectedItemToMigrate ? page.selectedItemToMigrate.cover : ""
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Txt {
                            text: page.selectedItemToMigrate ? page.selectedItemToMigrate.title : ""
                            font.pixelSize: 16; font.weight: Font.Bold
                        }
                        Caption {
                            text: "Current source: " + (page.selectedItemToMigrate ? page.selectedItemToMigrate.source : "")
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Txt { text: "Target Source:"; font.weight: Font.Bold }
                    MdCombo {
                        id: targetSourceCombo
                        Layout.preferredWidth: 220
                        model: {
                            const list = []
                            for (let i = 0; i < controller.sources.length; ++i) {
                                const s = controller.sources[i]
                                if (page.selectedItemToMigrate && s.id !== page.selectedItemToMigrate.provider) {
                                    list.push(s.name)
                                }
                            }
                            return list
                        }
                        onActivated: index => {
                            page.selectedCandidate = null
                            const targetName = model[index]
                            let targetId = targetName.toLowerCase()
                            for (let i = 0; i < controller.sources.length; ++i) {
                                if (controller.sources[i].name === targetName) {
                                    targetId = controller.sources[i].id
                                    break
                                }
                            }
                            if (page.selectedItemToMigrate) {
                                controller.searchMigrationCandidates(targetId, page.selectedItemToMigrate.title)
                            }
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Caption {
                        text: controller.searchingMigration ? "Searching candidates…" : (controller.migrationCandidates.length + " candidates found")
                    }
                }

                // Candidates list
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 12
                    color: Theme.navBg
                    border.color: Theme.dividerStrong
                    clip: true

                    ScrollView {
                        anchors.fill: parent
                        ListView {
                            width: parent.width
                            model: controller.migrationCandidates
                            spacing: 8
                            delegate: Rectangle {
                                required property var modelData
                                width: parent.width - 16
                                x: 8
                                height: 64
                                radius: 10
                                color: (page.selectedCandidate && page.selectedCandidate.id === modelData.id) ? Theme.accentMuted : "transparent"
                                border.color: (page.selectedCandidate && page.selectedCandidate.id === modelData.id) ? Theme.accentLight : Theme.track

                                Clickable {
                                    anchors.fill: parent
                                    onClicked: page.selectedCandidate = modelData
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 12
                                    CoverArt { width: 32; height: 48; radius: 6; cover: modelData.cover }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Txt { text: modelData.title; font.weight: Font.Bold; elide: Text.ElideRight }
                                        Caption { text: modelData.source }
                                    }
                                    Rectangle {
                                        width: 20; height: 20; radius: 10
                                        color: (page.selectedCandidate && page.selectedCandidate.id === modelData.id) ? Theme.accent : "transparent"
                                        border.color: Theme.track
                                        Icon {
                                            anchors.centerIn: parent
                                            name: "Check"
                                            size: 12
                                            color: Theme.white
                                            visible: (page.selectedCandidate && page.selectedCandidate.id === modelData.id)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Action buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Btn {
                        kind: "ghost"
                        text: "Cancel"
                        Layout.preferredWidth: 100
                        implicitHeight: 40
                        onClicked: migrationModal.visible = false
                    }
                    Item { Layout.fillWidth: true }
                    Btn {
                        text: "Migrate Progress & Categories"
                        enabled: page.selectedCandidate !== null
                        implicitHeight: 40
                        onClicked: {
                            if (page.selectedItemToMigrate && page.selectedCandidate) {
                                const ok = controller.migrateManga(
                                    page.selectedItemToMigrate.provider,
                                    page.selectedItemToMigrate.mangaId,
                                    page.selectedCandidate.provider,
                                    page.selectedCandidate.mangaId,
                                    page.selectedCandidate.title,
                                    page.selectedCandidate.coverUrl
                                )
                                migrationModal.visible = false
                                if (ok) {
                                    Nav.toast("Migrated " + page.selectedItemToMigrate.title + " to " + page.selectedCandidate.source)
                                } else {
                                    Nav.toast("Migration failed")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
