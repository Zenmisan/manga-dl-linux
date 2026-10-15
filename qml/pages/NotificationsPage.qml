import QtQuick
import QtQuick.Layouts
import MangaDl

// Notification inbox. Unread rows get a faint accent wash.
Item {
    id: root

    PageScroll {
        anchors.fill: parent
        spacing: 18
        maxContentWidth: 880

        RowLayout {
            width: parent.width
            PageTitle { text: "Notifications"; Layout.fillWidth: true }
            Btn {
                kind: "ghost"
                text: "Mark All Read"
                iconName: "Check"
                enabled: NotificationsController.unreadCount > 0
                onClicked: NotificationsController.markAllRead()
            }
        }

        Tabs {
            width: parent.width
            model: NotificationsController.tabs
            onActivated: (index) => {
                const rawTab = model[index]
                if (rawTab.indexOf("Unread") >= 0) {
                    NotificationsController.selectedTab = "Unread"
                } else {
                    NotificationsController.selectedTab = rawTab
                }
            }
        }

        // Empty state
        Column {
            width: parent.width
            visible: NotificationsController.notices.length === 0
            spacing: 16
            topPadding: 48
            bottomPadding: 48

            Icon {
                name: "Bell"
                size: 48
                color: Theme.accentMuted
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Txt {
                text: "No notifications"
                font.pixelSize: 20
                font.weight: Font.Bold
                color: Theme.fg
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Caption {
                text: "You're all caught up on new chapter releases and downloads."
                font.pixelSize: 14
                color: Theme.fgSubtle
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Column {
            width: parent.width
            Repeater {
                model: NotificationsController.notices
                delegate: Rectangle {
                    required property var modelData
                    width: parent.width
                    height: Math.max(72, row.implicitHeight + 32)
                    color: modelData.unread ? Theme.accentWash : "transparent"
                    HairLine { anchors.bottom: parent.bottom; width: parent.width }
                    RowLayout {
                        id: row
                        anchors { fill: parent; margins: 16 }
                        spacing: 16
                        Rectangle {
                            width: 40; height: 40; radius: 20
                            color: modelData.kind === "error" ? Theme.errorSoft : modelData.kind === "success" ? Theme.successSoft : Theme.surfaceHigh
                            Icon {
                                anchors.centerIn: parent; name: modelData.icon !== "" ? modelData.icon : "Bell"; size: 20
                                color: modelData.kind === "error" ? Theme.errorText : modelData.kind === "success" ? Theme.successText : Theme.fg
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Txt { Layout.fillWidth: true; text: modelData.title; font.pixelSize: 15; font.weight: Font.Bold }
                            Txt { Layout.fillWidth: true; text: modelData.body; font.pixelSize: 13; color: Theme.fgSoft; wrapMode: Text.Wrap }
                        }
                        Txt { text: modelData.when; font.pixelSize: 12; color: Theme.fgFaint }
                        Btn {
                            visible: modelData.action !== ""
                            kind: "ghost"; size: "small"; text: modelData.action
                            onClicked: {
                                if (modelData.action === "View") {
                                    Nav.go("Updates")
                                } else {
                                    NotificationsController.handleAction(modelData.action, modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
