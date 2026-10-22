import QtQuick
import QtQuick.Layouts
import MangaDl

// "Update ready" modal over the current page.
Item {
    id: dialog
    visible: opacity > 0
    opacity: 0
    Behavior on opacity { NumberAnimation { duration: Theme.fade } }

    readonly property string releasesChannelUrl: "https://github.com/zenmisan/manga-dl/releases"
    readonly property string latestApiUrl: "https://api.github.com/repos/zenmisan/manga-dl/releases/latest"

    property bool checking: false
    property bool updateAvailable: false
    property string latestVersion: ""
    property string releaseNotes: ""
    property string releaseUrl: releasesChannelUrl
    property string statusMessage: "You're running version " + AppInfo.version + " on Linux. System is up to date."

    function open() {
        opacity = 1
        closeButton.forceActiveFocus()
        if (AppSettings.autoCheckUpdates && !checking) {
            checkForUpdates()
        }
    }

    function close() {
        opacity = 0
    }

    function compareVersions(v1, v2) {
        var s1 = (v1 || "").replace(/^v/, "").split('.').map(function(x) { return parseInt(x, 10) || 0; })
        var s2 = (v2 || "").replace(/^v/, "").split('.').map(function(x) { return parseInt(x, 10) || 0; })
        for (var i = 0; i < Math.max(s1.length, s2.length); i++) {
            var n1 = s1[i] || 0
            var n2 = s2[i] || 0
            if (n1 > n2) return 1
            if (n1 < n2) return -1
        }
        return 0
    }

    function checkForUpdates() {
        checking = true
        statusMessage = "Checking GitHub Releases..."

        var xhr = new XMLHttpRequest()
        xhr.open("GET", latestApiUrl)
        xhr.setRequestHeader("Accept", "application/vnd.github.v3+json")
        xhr.setRequestHeader("User-Agent", "manga-dl-linux/" + AppInfo.version)

        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                checking = false
                if (xhr.status === 200) {
                    try {
                        var data = JSON.parse(xhr.responseText)
                        var tag = data.tag_name || ""
                        latestVersion = tag
                        releaseNotes = data.body || ""
                        releaseUrl = data.html_url || releasesChannelUrl

                        if (compareVersions(tag, AppInfo.version) > 0) {
                            updateAvailable = true
                            statusMessage = "A new version of manga-dl is available: " + tag
                        } else {
                            updateAvailable = false
                            statusMessage = "You're running the latest version (" + AppInfo.version + ")."
                        }
                    } catch (e) {
                        statusMessage = "Unable to parse update data from GitHub."
                    }
                } else if (xhr.status === 403) {
                    statusMessage = "GitHub rate limit reached. Check releases directly on GitHub."
                } else {
                    statusMessage = "Could not check updates. Check your connection or GitHub."
                }
            }
        }
        xhr.send()
    }

    Keys.onEscapePressed: close()

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        MouseArea { anchors.fill: parent; onClicked: dialog.close() }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(540, dialog.width - 32)
        height: content.implicitHeight + 56
        radius: 20
        color: Theme.sheet
        border.color: Theme.track
        border.width: 1
        MouseArea { anchors.fill: parent }   // swallow clicks so the scrim doesn't close it

        ColumnLayout {
            id: content
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 28 }
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                Eyebrow { text: "Software version"; Layout.fillWidth: true }
                Btn {
                    kind: "link"
                    text: "GitHub Releases"
                    fontSize: 12
                    iconName: "Globe"
                    onClicked: Qt.openUrlExternally(dialog.releasesChannelUrl)
                }
            }

            DisplayText {
                text: dialog.updateAvailable ? "Update Available!" : ("manga-dl v" + AppInfo.version)
                font.pixelSize: dialog.width < 500 ? 28 : 36
                color: dialog.updateAvailable ? Theme.accentLight : Theme.fg
            }

            BodyMuted {
                Layout.fillWidth: true
                text: dialog.statusMessage
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                RowLayout {
                    Caption {
                        text: dialog.checking ? "Checking..." : dialog.updateAvailable ? "New version ready" : "Status"
                        Layout.fillWidth: true
                    }
                    Caption {
                        text: dialog.updateAvailable ? dialog.latestVersion : "Up to date"
                        color: dialog.updateAvailable ? Theme.accentLight : Theme.fgSubtle
                    }
                }
                Progress {
                    Layout.fillWidth: true
                    height: 6
                    value: 1
                    fill: dialog.updateAvailable ? Theme.accent : Theme.success
                }
            }

            // Changelog card if update is available, or Highlights if up to date
            Card {
                Layout.fillWidth: true
                kind: "list"
                padding: 16
                spacing: 8
                Eyebrow {
                    text: dialog.updateAvailable ? ("What's new in " + dialog.latestVersion) : "Highlights"
                    muted: true
                }
                Txt {
                    visible: !dialog.updateAvailable
                    text: "• Native Qt 6 Quick client with C++20 core engine"
                    font.pixelSize: 13
                    color: Theme.fgStrong
                }
                Txt {
                    visible: !dialog.updateAvailable
                    text: "• High-performance QuickJS runtime supporting 38+ sources"
                    font.pixelSize: 13
                    color: Theme.fgStrong
                }
                Txt {
                    visible: !dialog.updateAvailable
                    text: "• Local-first library, CBZ exports, and Supabase cloud sync"
                    font.pixelSize: 13
                    color: Theme.fgStrong
                }
                Txt {
                    visible: dialog.updateAvailable && dialog.releaseNotes.length > 0
                    text: dialog.releaseNotes.substring(0, 300) + (dialog.releaseNotes.length > 300 ? "..." : "")
                    font.pixelSize: 13
                    color: Theme.fgStrong
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }

            CheckRow {
                text: "Check for updates automatically"
                checked: AppSettings.autoCheckUpdates
                onToggled: AppSettings.autoCheckUpdates = checked
                font.pixelSize: 13
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 10

                Btn {
                    kind: "ghost"
                    text: dialog.checking ? "Checking..." : "Check Now"
                    iconName: "Refresh"
                    enabled: !dialog.checking
                    implicitHeight: 42
                    fontSize: 13
                    onClicked: dialog.checkForUpdates()
                }

                Item { Layout.fillWidth: true }

                Btn {
                    visible: dialog.updateAvailable
                    text: "Download Update"
                    iconName: "Download"
                    implicitHeight: 42
                    fontSize: 13
                    onClicked: Qt.openUrlExternally(dialog.releaseUrl)
                }

                Btn {
                    id: closeButton
                    kind: dialog.updateAvailable ? "ghost" : "primary"
                    text: "Close"
                    implicitHeight: 42
                    fontSize: 13
                    onClicked: dialog.close()
                }
            }
        }
    }
}
