import QtQuick
import QtQuick.Layouts
import MangaDl

// Debug-only index of every screen, grouped like the desktop page on the design canvas.
Item {
    readonly property var groups: [
        { title: "First run", items: [["Onboarding", "Onboarding"], ["Sign in", "Login"], ["Create account", "Register"], ["Forgot password", "ForgotPassword"]] },
        { title: "Main", items: [["Library", "Library"], ["Updates", "Updates"], ["History", "History"], ["Notifications", "Notifications"], ["Downloads", "Downloads"], ["Statistics", "Stats"]] },
        { title: "Discover", items: [["Browse · Extensions", "Extensions"], ["Browse · Sources", "Extensions?tab=sources"], ["Browse · Migrate", "Extensions?tab=migrate"], ["Search", "Search"], ["Browse a source", "BrowseSource?source=MangaDex"], ["Import local files", "Import"]] },
        { title: "Reading & you", items: [["Manga detail", "MangaDetail"], ["Reader · two-page spread", "Reader"], ["Reader · single page", "Reader?layout=single"], ["Novel reader", "NovelReader"], ["Reader profile", "Profile"], ["Help center", "Help"]] },
        { title: "Settings", items: [["Account", "Settings?section=Account"], ["General", "Settings?section=General"], ["Reader", "Settings?section=Reader"], ["Library", "Settings?section=Library"], ["Trackers", "Settings?section=Trackers"], ["System & backup", "Settings?section=System"]] },
        { title: "Overlays", items: [["App update available", "!update"], ["Toast", "!toast"]] }
    ]

    function openTarget(target) {
        if (target === "!update") return Nav.showUpdateDialog()
        if (target === "!toast") return Nav.toast("4 chapters downloaded")
        const [name, query] = target.split("?")
        const params = {}
        if (query) { const [k, v] = query.split("="); params[k] = v }
        Nav.open(name, params)
    }

    PageScroll {
        anchors.fill: parent
        maxContentWidth: 1100
        spacing: 22

        Column {
            PageTitle { text: "All pages" }
            Caption { text: "Debug builds only. Jump to any screen to compare it with the design canvas."; font.pixelSize: 14; topPadding: 4 }
        }

        GridLayout {
            width: parent.width
            columns: 3
            columnSpacing: 24
            rowSpacing: 24
            Repeater {
                model: groups
                ColumnLayout {
                    id: group
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 8
                    Eyebrow { text: group.modelData.title }
                    Repeater {
                        model: group.modelData.items
                        Btn {
                            required property var modelData
                            Layout.fillWidth: true
                            kind: "ghost"; alignLeft: true
                            implicitHeight: 44; fontSize: 14; font.weight: Font.DemiBold
                            text: modelData[0]
                            onClicked: openTarget(modelData[1])
                        }
                    }
                }
            }
        }
    }
}
