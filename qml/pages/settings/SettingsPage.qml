import QtQuick
import QtQuick.Layouts
import MangaDl

// Settings host: section nav on the left, the section on the right.
// Open with { section: "Account" | "General" | "Reader" | "Library" | "Trackers" | "System" }.
Item {
    id: page
    property string section: "General"

    readonly property var sections: [
        ["Account", "Account", "User"], ["General", "General", "Settings"], ["Reader", "Reader", "Pages"],
        ["Library", "Library", "Library"], ["Trackers", "Trackers", "Track"], ["System", "System & backup", "Backup"]
    ]

    Column {
        anchors { fill: parent; leftMargin: 40; rightMargin: 40; topMargin: 28 }
        spacing: 20

        PageTitle { text: "Settings" }

        Row {
            width: parent.width
            height: parent.height - y
            spacing: 32

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
}
