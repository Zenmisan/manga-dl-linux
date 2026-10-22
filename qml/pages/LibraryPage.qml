import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Library: continue-reading cards, category chips, content/status filters, and the cover grid/list.
Item {
    id: libraryRoot

    Component.onCompleted: {
        LibraryModel.contentType = AppSettings.libraryContentType || "all"
        LibraryModel.filterMode = AppSettings.libraryFilterMode || "all"
        if (!AppSettings.libraryTourCompleted) {
            quickTour.open()
        }
    }

    PageScroll {
        anchors.fill: parent

        // Header (Responsive wrapping layout)
        Flow {
            width: parent.width
            spacing: 10

            PageTitle {
                text: "Library"
                width: parent.width < 600 ? parent.width : undefined
            }

            Item {
                // Spacer for desktop wide views
                visible: parent.width >= 600
                width: Math.max(0, parent.width - 720)
                height: 1
            }

            SearchField {
                width: parent.width < 600 ? parent.width : 220
                placeholderText: "Search library   Ctrl K"
                text: LibraryModel.searchFilter
                onTextChanged: LibraryModel.searchFilter = text
            }

            // View Mode toggle: Grid vs List
            Btn {
                kind: "ghost"
                size: "small"
                text: AppSettings.libraryViewMode === "list" ? "List" : "Grid"
                iconName: AppSettings.libraryViewMode === "list" ? "PageSpread" : "Apps"
                iconSize: 14
                onClicked: {
                    AppSettings.libraryViewMode = AppSettings.libraryViewMode === "list" ? "grid" : "list"
                }
            }

            // Density toggle: Normal vs Compact
            Btn {
                kind: "ghost"
                size: "small"
                text: AppSettings.libraryDensity === "compact" ? "Compact" : "Normal"
                iconName: "Settings"
                iconSize: 14
                onClicked: {
                    AppSettings.libraryDensity = AppSettings.libraryDensity === "compact" ? "normal" : "compact"
                }
            }

            // Sort button
            Btn {
                kind: "ghost"
                size: "small"
                text: {
                    switch (LibraryModel.sortMode) {
                    case 1: return "Title"
                    case 2: return "Unread"
                    default: return "Recent"
                    }
                }
                iconName: "Sort"
                iconSize: 14
                onClicked: LibraryModel.sortMode = (LibraryModel.sortMode + 1) % 3
            }

            Btn {
                size: "small"
                text: "Update"
                iconName: "Refresh"
                iconSize: 14
                onClicked: {
                    LibraryModel.refresh()
                    Nav.toast("Checking library for new chapters")
                }
            }
        }

        // Secondary filter toolbar: Content Type pills + Filter mode pills
        Flow {
            width: parent.width
            spacing: 14

            // Content Type (All | Manga | Novels)
            Row {
                spacing: 6
                ButtonGroup { id: typeGroup }
                Repeater {
                    model: [
                        { label: "All Types", val: "all" },
                        { label: "Manga", val: "manga" },
                        { label: "Novels", val: "novel" }
                    ]
                    Chip {
                        required property var modelData
                        small: true
                        text: modelData.label
                        checked: (AppSettings.libraryContentType || "all") === modelData.val
                        ButtonGroup.group: typeGroup
                        onClicked: {
                            AppSettings.libraryContentType = modelData.val
                            LibraryModel.contentType = modelData.val
                        }
                    }
                }
            }

            Rectangle { width: 1; height: 18; color: Theme.track; anchors.verticalCenter: parent.verticalCenter; visible: parent.width >= 600 }

            // Filter Mode (All | Unread | Downloaded | Started | Completed)
            Row {
                spacing: 6
                ButtonGroup { id: filterGroup }
                Repeater {
                    model: [
                        { label: "All", val: "all" },
                        { label: "Unread", val: "unread" },
                        { label: "Downloaded", val: "downloaded" },
                        { label: "Started", val: "started" },
                        { label: "Completed", val: "completed" }
                    ]
                    Chip {
                        required property var modelData
                        small: true
                        text: modelData.label
                        checked: (AppSettings.libraryFilterMode || "all") === modelData.val
                        ButtonGroup.group: filterGroup
                        onClicked: {
                            AppSettings.libraryFilterMode = modelData.val
                            LibraryModel.filterMode = modelData.val
                        }
                    }
                }
            }
        }

        // Continue reading
        Column {
            width: parent.width
            spacing: 12
            visible: AppSettings.continueReadingRow && LibraryModel.continueReading.length > 0

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
                    onClicked: {
                        const isNovel = modelData.manga && (modelData.manga.type === "novel" || modelData.manga.contentType === "novel")
                        Nav.open(isNovel ? "NovelReader" : "Reader", {
                            manga: modelData.manga,
                            chapterId: modelData.chapterId || "",
                            chapterTitle: modelData.chapterTitle || "",
                            chapterNumber: modelData.chapterNumber || 0
                        })
                    }
                    Rectangle {
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
                            Layout.preferredWidth: 0
                            Layout.rightMargin: 12
                            spacing: 6
                            ItemTitle {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 0
                                text: modelData.manga ? modelData.manga.title : ""
                                elide: Text.ElideRight
                            }
                            Caption {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 0
                                text: modelData.where || ""
                                elide: Text.ElideRight
                            }
                            Progress { Layout.fillWidth: true; value: modelData.progress || 0 }
                        }
                    }
                }
            }
        }

        // Categories (Scrollable horizontal container to prevent offscreen overflow)
        Flickable {
            width: parent.width
            height: catRow.height + 4
            contentWidth: catRow.width
            clip: true
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds

            Row {
                id: catRow
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
        }

        // Grid View
        AutoGrid {
            width: parent.width
            minItemWidth: AppSettings.libraryDensity === "compact" ? 112 : 144
            spacing: AppSettings.libraryDensity === "compact" ? 12 : 20
            visible: LibraryModel.count > 0 && AppSettings.libraryViewMode !== "list"
            model: LibraryModel
            delegate: CoverTile {
                required property var modelData
                width: parent.cellWidth
                manga: modelData
                showBadges: AppSettings.unreadBadges
            }
        }

        // List View
        Column {
            width: parent.width
            spacing: 6
            visible: LibraryModel.count > 0 && AppSettings.libraryViewMode === "list"

            Repeater {
                model: LibraryModel
                delegate: Clickable {
                    required property var modelData
                    width: parent.width
                    height: AppSettings.libraryDensity === "compact" ? 56 : 72
                    radius: 10
                    fill: Theme.surfaceRaised
                    onClicked: Nav.open("MangaDetail", { manga: modelData })

                    RowLayout {
                        anchors { fill: parent; leftMargin: 10; rightMargin: 16 }
                        spacing: 14

                        CoverArt {
                            width: AppSettings.libraryDensity === "compact" ? 34 : 44
                            height: AppSettings.libraryDensity === "compact" ? 48 : 62
                            radius: 6
                            cover: modelData.cover || "#1A2433"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Txt {
                                text: modelData.title || ""
                                font.pixelSize: AppSettings.libraryDensity === "compact" ? 13 : 15
                                font.weight: Font.Bold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Txt {
                                text: (modelData.provider || modelData.source || "") + (modelData.type === "novel" ? " · Light Novel" : " · Manga")
                                font.pixelSize: 11
                                color: Theme.fgSubtle
                            }
                        }

                        // Chapters badge
                        Txt {
                            text: (modelData.totalChapters || 0) + " ch."
                            color: Theme.fgSubtle
                            font.pixelSize: 12
                            Layout.preferredWidth: 80
                            horizontalAlignment: Text.AlignRight
                        }

                        // Downloaded icon
                        Icon {
                            name: "Download"
                            size: 14
                            color: modelData.downloaded ? Theme.accentLight : "transparent"
                        }

                        // Unread badge
                        Rectangle {
                            visible: AppSettings.unreadBadges && (modelData.unread > 0)
                            width: Math.max(22, unreadTxt.implicitWidth + 10)
                            height: 20
                            radius: 10
                            color: Theme.accent
                            Txt {
                                id: unreadTxt
                                anchors.centerIn: parent
                                text: modelData.unread || 0
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: Theme.white
                            }
                        }
                    }
                }
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

    // Quick Tour for First Run / Library
    QuickTourModal {
        id: quickTour
        tourId: "library"
        steps: [
            {
                title: "Welcome to manga-dl",
                subtitle: "Fast, distraction-free reading",
                icon: "Play",
                description: "Your local-first manga and light novel center. Organize your reading library, sort by titles or unread chapters, and access your favorite stories anytime."
            },
            {
                title: "38+ Built-in Sources",
                subtitle: "Search, discover & read",
                icon: "Search",
                description: "Browse extensions across MangaDex, Comick, MangaFox, and community repositories. Search once across multiple sources simultaneously."
            },
            {
                title: "Supercharged Reader",
                subtitle: "Tailored reading ergonomics",
                icon: "Pages",
                description: "Seamless single/spread reading, webtoon continuous scroll, image sharpening, ambilight glow, and customizable keyboard shortcuts."
            },
            {
                title: "Cloud Sync & Backups",
                subtitle: "Cross-platform continuity",
                icon: "Backup",
                description: "Link your Supabase account or self-hosted backend, synchronize reading progress across devices, and connect external trackers like AniList and MyAnimeList."
            }
        ]
    }
}
