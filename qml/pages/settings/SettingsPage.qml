import QtQuick
import QtQuick.Layouts
import MangaDl

// Settings host: search bar, section navigation, and settings tour.
Item {
    id: page
    property string section: "General"
    property string searchQuery: ""

    Component.onCompleted: {
        if (!AppSettings.settingsTourCompleted) {
            settingsTour.open()
        }
    }

    readonly property var sections: [
        ["Account", "Account", "User"],
        ["General", "General", "Settings"],
        ["Reader", "Reader", "Pages"],
        ["Library", "Library", "Library"],
        ["Trackers", "Trackers", "Track"],
        ["System", "System & Backup", "Backup"],
        ["Advanced", "Advanced", "Apps"]
    ]

    readonly property var searchableSettings: [
        { name: "Theme (Dark/Light/AMOLED)", section: "General", keywords: "theme dark light amoled colors appearance" },
        { name: "Accent Color", section: "General", keywords: "accent color red blue green crimson purple" },
        { name: "Ambilight Glow", section: "General", keywords: "ambilight glow cover background effect" },
        { name: "Desktop Notifications", section: "General", keywords: "notifications alerts desktop popups" },
        { name: "Discord Rich Presence", section: "General", keywords: "discord rpc activity status playing" },
        { name: "Incognito Mode", section: "General", keywords: "incognito private history reading tracking" },
        { name: "Reader Layout Direction", section: "Reader", keywords: "reading direction right to left left to right vertical webtoon" },
        { name: "Two-Page Spread", section: "Reader", keywords: "two page double spread dual pages" },
        { name: "Webtoon Side Padding", section: "Reader", keywords: "webtoon padding side margin width" },
        { name: "Image Fit Mode", section: "Reader", keywords: "fit height width screen contain cover" },
        { name: "Image Quality & Filter", section: "Reader", keywords: "sharpen invert grayscale brightness contrast" },
        { name: "Light Novel Typography", section: "Reader", keywords: "novel font size line height font family" },
        { name: "Library Grid Columns", section: "Library", keywords: "columns grid size covers auto" },
        { name: "Unread Badges", section: "Library", keywords: "unread badges count numbers" },
        { name: "Downloaded Badges", section: "Library", keywords: "downloaded badges indicator offline" },
        { name: "Continue Reading Row", section: "Library", keywords: "continue reading row resume recent" },
        { name: "Auto Check New Chapters", section: "Library", keywords: "updates check schedule interval frequency" },
        { name: "AniList Tracker", section: "Trackers", keywords: "anilist tracker sync progress oauth" },
        { name: "MyAnimeList Tracker", section: "Trackers", keywords: "myanimelist mal tracker sync oauth" },
        { name: "Kitsu & MangaUpdates Tokens", section: "Trackers", keywords: "kitsu mangaupdates shikimori bangumi token" },
        { name: "Download Location", section: "System", keywords: "download path folder storage directory disk" },
        { name: "Clear Image Cache", section: "System", keywords: "cache image clear storage free disk" },
        { name: "Backup & Restore", section: "System", keywords: "backup restore tachiyomi export import json" },
        { name: "Automatic Backups", section: "System", keywords: "auto backup daily weekly schedule" },
        { name: "Komga Server", section: "Advanced", keywords: "komga server self-hosted comic url username password" },
        { name: "Suwayomi / Tachidesk", section: "Advanced", keywords: "suwayomi tachidesk server url sync" },
        { name: "App Lock & Security PIN", section: "Advanced", keywords: "app lock security pin password protect" },
        { name: "Parallel Downloads", section: "Advanced", keywords: "parallel concurrent download threads connections" },
        { name: "CBZ Archive Format", section: "Advanced", keywords: "cbz archive format zip package" },
        { name: "Supabase & Cloud Sync Endpoint", section: "Advanced", keywords: "supabase backend url api key cloud sync" }
    ]

    readonly property var filteredSearch: {
        if (!page.searchQuery || page.searchQuery.trim().length === 0) return []
        const q = page.searchQuery.trim().toLowerCase()
        return page.searchableSettings.filter(item => {
            return item.name.toLowerCase().indexOf(q) >= 0 ||
                   item.keywords.toLowerCase().indexOf(q) >= 0 ||
                   item.section.toLowerCase().indexOf(q) >= 0
        })
    }

    Column {
        anchors { fill: parent; leftMargin: 40; rightMargin: 40; topMargin: 28 }
        spacing: 20

        RowLayout {
            width: parent.width
            spacing: 16

            PageTitle { text: "Settings"; Layout.fillWidth: true }

            SearchField {
                id: settingsSearchInput
                Layout.preferredWidth: 320
                placeholderText: "Search all settings..."
                text: page.searchQuery
                onTextChanged: page.searchQuery = text
            }

            Btn {
                kind: "ghost"
                size: "small"
                text: "Quick Tour"
                iconName: "Help"
                iconSize: 14
                onClicked: settingsTour.open()
            }
        }

        // Search Results Drawer (if active)
        Card {
            width: parent.width
            visible: page.searchQuery.trim().length > 0
            kind: "faint"
            padding: 16
            spacing: 8

            RowLayout {
                width: parent.width
                Eyebrow { text: "Search Results (" + page.filteredSearch.length + ")" }
                Item { Layout.fillWidth: true }
                Btn {
                    kind: "link"
                    text: "Clear Search"
                    fontSize: 12
                    onClicked: {
                        page.searchQuery = ""
                        settingsSearchInput.text = ""
                    }
                }
            }

            Txt {
                visible: page.filteredSearch.length === 0
                text: "No settings matched \"" + page.searchQuery + "\"."
                color: Theme.fgSubtle
            }

            Flow {
                width: parent.width
                spacing: 8
                Repeater {
                    model: page.filteredSearch
                    Clickable {
                        required property var modelData
                        width: itemRow.implicitWidth + 24
                        height: 38
                        radius: 8
                        fill: Theme.surfaceRaised
                        onClicked: {
                            page.section = modelData.section
                            page.searchQuery = ""
                            settingsSearchInput.text = ""
                        }

                        RowLayout {
                            id: itemRow
                            anchors.centerIn: parent
                            spacing: 8
                            Txt { text: modelData.name; font.pixelSize: 13; font.weight: Font.DemiBold }
                            Rectangle { width: 1; height: 12; color: Theme.track }
                            Txt { text: modelData.section; font.pixelSize: 11; color: Theme.accentLight }
                        }
                    }
                }
            }
        }

        Row {
            width: parent.width
            height: parent.height - y
            spacing: 32
            visible: page.searchQuery.trim().length === 0

            Column {
                width: 200
                spacing: 2
                Repeater {
                    model: page.sections
                    NavItem {
                        required property var modelData
                        width: 200
                        height: 42
                        text: modelData[1]
                        iconName: modelData[2]
                        selected: page.section === modelData[0]
                        onClicked: page.section = modelData[0]
                    }
                }
            }

            Loader {
                width: parent.width - 232
                height: parent.height
                source: Qt.resolvedUrl(page.section + "Settings.qml")
                opacity: status === Loader.Ready ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Theme.fade } }
            }
        }
    }

    QuickTourModal {
        id: settingsTour
        tourId: "settings"
        steps: [
            {
                title: "Settings Search",
                subtitle: "Instant configuration lookup",
                icon: "Search",
                description: "Type in the new search bar to quickly jump to any configuration option across Appearance, Reader, Library, Trackers, and Servers."
            },
            {
                title: "Reading Ergonomics",
                subtitle: "Custom-fitted canvas",
                icon: "Pages",
                description: "Fine-tune reading direction, dual-page spread, webtoon scroll margins, dark ambilight glow, and novel reader font sizes."
            },
            {
                title: "Trackers & Progress",
                subtitle: "Automated list synchronization",
                icon: "Track",
                description: "Connect AniList and MyAnimeList via OAuth, or add personal access tokens for Kitsu, MangaUpdates, Shikimori, and Bangumi."
            },
            {
                title: "Advanced & Media Servers",
                subtitle: "For power users & self-hosters",
                icon: "Apps",
                description: "Configure self-hosted Komga and Suwayomi servers, set up App Lock PIN protection, tune parallel download threads, and test Supabase cloud endpoints."
            }
        ]
    }
}
