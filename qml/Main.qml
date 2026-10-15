import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window
import MangaDl

// App window: sidebar + page stack, with the update dialog and toast on top.
// The window keeps the desktop's own title bar (decorations are left to the system).
ApplicationWindow {
    id: window
    width: 1440
    height: 900
    minimumWidth: 1100
    minimumHeight: 700
    visible: true
    title: "manga-dl"
    color: Theme.bg
    font.family: Theme.body

    // First screen. Pass --page <Name> on the command line to start elsewhere (handy for screenshots).
    readonly property string startPage: {
        const args = Qt.application.arguments
        const i = args.indexOf("--page")
        return i >= 0 && i + 1 < args.length ? args[i + 1] : "Onboarding"
    }

    // --param key=value (repeatable) passes page parameters to the start page, e.g. --page Extensions --param tab=sources
    readonly property var startParams: {
        const args = Qt.application.arguments, out = {}
        for (let i = 0; i + 1 < args.length; i++)
            if (args[i] === "--param") { const kv = args[i + 1].split("="); out[kv[0]] = kv.slice(1).join("=") }
        return out
    }

    readonly property bool fullBleed: Nav.fullBleed.indexOf(Nav.current) >= 0

    function urlFor(page) {
        return Qt.resolvedUrl(page === "Settings" ? "pages/settings/SettingsPage.qml" : "pages/" + page + "Page.qml")
    }

    function show(page, params, push) {
        const props = Object.assign({ objectName: page }, params || {})
        if (push) stack.push(urlFor(page), props)
        else if (stack.depth === 0) stack.push(urlFor(page), props)
        else stack.replace(null, urlFor(page), props)   // sidebar jumps reset the history
        Nav.current = page
    }

    Connections {
        target: Nav
        function onNavigate(page, params, push) { window.show(page, params, push) }
        function onGoBack() {
            if (stack.depth > 1) { stack.pop(); Nav.current = stack.currentItem.objectName }
            else window.show("Library", {}, false)
        }
        function onToastRequested(text) { toast.show(text) }
        function onUpdateDialogRequested() { updateDialog.open() }
    }

    Component.onCompleted: show(startPage, startParams, false)

    Shortcut { sequences: ["Alt+Left", "Back"]; onActivated: if (!window.fullBleed) Nav.back() }

    Sidebar {
        id: sidebar
        visible: !window.fullBleed
        anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
        current: Nav.sidebarKey[Nav.current] || ""
    }

    StackView {
        id: stack
        anchors { top: parent.top; bottom: parent.bottom; right: parent.right; left: sidebar.visible ? sidebar.right : parent.left }
        clip: true
        // design.md: fade only, no slides.
        pushEnter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.fade } }
        pushExit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.fade } }
        popEnter: pushEnter
        popExit: pushExit
        replaceEnter: pushEnter
        replaceExit: pushExit
    }

    UpdateDialog { id: updateDialog; anchors.fill: parent; z: 10 }

    Toast { id: toast; z: 11; anchors { right: parent.right; bottom: parent.bottom; margins: 24 } }
}
