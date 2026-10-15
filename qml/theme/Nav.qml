pragma Singleton
import QtQuick

// App-wide navigation. Main.qml listens and drives the page stack.
//   Nav.go("Library")                      replace the current section (sidebar)
//   Nav.open("MangaDetail", { manga: m })  push a page on top (Back returns)
//   Nav.back(), Nav.home()
// Page names are the file names in qml/pages (and qml/pages/settings for "Settings").
QtObject {
    signal navigate(string page, var params, bool push)
    signal goBack()
    signal toastRequested(string text)
    signal updateDialogRequested()

    property string current: ""

    // Pages that hide the sidebar.
    readonly property var fullBleed: ["Onboarding", "Login", "Register", "ForgotPassword", "Reader", "NovelReader"]

    // Which sidebar item lights up for each page.
    readonly property var sidebarKey: ({
        Library: "Library", MangaDetail: "Library",
        Updates: "Updates", History: "History",
        Extensions: "Browse", Search: "Browse", BrowseSource: "Browse", Import: "Browse",
        Downloads: "Downloads", Stats: "Stats",
        Notifications: "Notifications", Settings: "Settings", Help: "Help",
        AllPages: "AllPages", Profile: ""
    })

    function go(page, params) { navigate(page, params || {}, false) }
    function open(page, params) { navigate(page, params || {}, true) }
    function back() { goBack() }
    function home() { navigate("Library", {}, false) }
    function toast(text) { toastRequested(text) }
    function showUpdateDialog() { updateDialogRequested() }
}
