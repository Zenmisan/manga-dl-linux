import QtQuick
import QtQuick.Layouts
import MangaDl

// Public reader profile.
Item {
    id: root

    PageScroll {
        anchors.fill: parent
        padding: 0
        topPadding: 0
        spacing: 0

        // Banner + avatar
        Rectangle {
            width: parent.width
            height: 180
            color: ProfileController.rankBgColor !== "" ? ProfileController.rankBgColor : "#2D1716"
        }

        RowLayout {
            x: 40
            width: parent.width - 80
            spacing: 16
            Rectangle {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: -64
                width: 140; height: 140; radius: 70
                color: Theme.bg
                border.width: 3
                border.color: ProfileController.rankColor !== "" ? ProfileController.rankColor : Theme.border
                Initial {
                    anchors.centerIn: parent
                    size: 124
                    round: true
                    color: ProfileController.rankBgColor !== "" ? ProfileController.rankBgColor : "#3A1518"
                    text: ProfileController.displayName !== "" ? ProfileController.displayName.charAt(0).toUpperCase() : "U"
                    fontSize: 44
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 16
                Layout.leftMargin: 8
                spacing: 2
                Txt {
                    Layout.fillWidth: true
                    text: ProfileController.displayName
                    font.pixelSize: 30
                    font.weight: Font.Black
                }
                Caption {
                    Layout.fillWidth: true
                    text: ProfileController.username + " · reading since " + ProfileController.readingSince
                    font.pixelSize: 14
                }
            }
            Btn {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 16
                kind: "ghost"
                text: "Share"
                iconName: "Share"
                onClicked: Nav.toast("Profile link copied to clipboard")
            }
            Btn {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 16
                text: "Edit Profile"
                iconName: "Edit"
                onClicked: Nav.go("Settings", { section: "Account" })
            }
        }

        RowLayout {
            x: 40
            width: parent.width - 80
            spacing: 32
            Layout.topMargin: 28
            y: 0

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 28
                spacing: 24

                Txt {
                    Layout.fillWidth: true
                    text: ProfileController.bio
                    font.pixelSize: 15
                    color: Theme.fgStrong
                    wrapMode: Text.Wrap
                }

                // Rank card
                Rectangle {
                    Layout.fillWidth: true
                    radius: 14
                    color: ProfileController.rankBgColor !== "" ? ProfileController.rankBgColor : Theme.surface
                    border.width: 1
                    border.color: ProfileController.rankColor !== "" ? ProfileController.rankColor : Theme.border
                    implicitHeight: rankLayout.implicitHeight + 32

                    ColumnLayout {
                        id: rankLayout
                        anchors { fill: parent; margins: 16 }
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Rectangle {
                                width: 36; height: 36; radius: 8
                                color: ProfileController.rankColor !== "" ? ProfileController.rankColor : Theme.accent
                                Txt {
                                    anchors.centerIn: parent
                                    text: ProfileController.rankCode
                                    font.pixelSize: 16
                                    font.weight: Font.Black
                                    color: "#000000"
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Txt {
                                    text: ProfileController.rankName
                                    font.pixelSize: 16
                                    font.weight: Font.Bold
                                    color: Theme.fg
                                }
                                Caption {
                                    text: ProfileController.rankTag + " · " + ProfileController.rankTier + " Tier"
                                    font.pixelSize: 12
                                    color: Theme.fgSoft
                                }
                            }

                            Txt {
                                text: ProfileController.score + " EXP"
                                font.pixelSize: 15
                                font.weight: Font.Bold
                                color: ProfileController.rankColor !== "" ? ProfileController.rankColor : Theme.accent
                            }
                        }

                        Progress {
                            Layout.fillWidth: true
                            height: 6
                            value: ProfileController.progressToNextRank
                        }

                        Caption {
                            Layout.fillWidth: true
                            text: ProfileController.nextRankInfo
                            font.pixelSize: 12
                            color: Theme.fgSubtle
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Eyebrow { text: "Pinned" }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        Repeater {
                            model: ProfileController.pinnedCovers
                            CoverArt {
                                required property string modelData
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                height: 250
                                radius: 12
                                cover: modelData
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 28
                spacing: 20

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Repeater {
                        model: [
                            [ProfileController.chaptersRead.toString(), "Chapters"],
                            [ProfileController.libraryCount.toString(), "Library"],
                            [ProfileController.currentStreak.toString(), "Day streak"]
                        ]
                        Rectangle {
                            required property var modelData
                            Layout.fillWidth: true; Layout.preferredWidth: 1
                            height: 86; radius: 14; color: Theme.surface
                            Column {
                                anchors.centerIn: parent
                                DisplayText { text: modelData[0]; font.pixelSize: 28; upper: false; anchors.horizontalCenter: parent.horizontalCenter }
                                Caption { text: modelData[1]; anchors.horizontalCenter: parent.horizontalCenter }
                            }
                        }
                    }
                }

                ColumnLayout {
                    spacing: 10
                    Eyebrow {
                        text: "Milestones (" + ProfileController.unlockedMilestonesCount + "/" + ProfileController.totalMilestonesCount + ")"
                        muted: true
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Repeater {
                            model: ProfileController.unlockedBadges
                            Rectangle {
                                required property var modelData
                                width: label.implicitWidth + 24; height: 32; radius: 16
                                color: modelData.color !== "" ? modelData.color : Theme.accentMuted
                                Txt {
                                    id: label
                                    anchors.centerIn: parent
                                    text: modelData.title
                                    font.pixelSize: 12
                                    font.weight: Font.Bold
                                    color: "#000000"
                                }
                            }
                        }
                        Repeater {
                            model: ProfileController.lockedBadges.slice(0, 4)
                            Rectangle {
                                required property var modelData
                                width: lockedLabel.implicitWidth + 24; height: 32; radius: 16
                                color: "transparent"
                                border.width: 1
                                border.color: Theme.switchOff
                                Txt {
                                    id: lockedLabel
                                    anchors.centerIn: parent
                                    text: modelData.title + " (" + modelData.currentValue + "/" + modelData.threshold + ")"
                                    font.pixelSize: 12
                                    font.weight: Font.DemiBold
                                    color: Theme.fgSubtle
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Eyebrow { text: "Recent activity"; muted: true; Layout.bottomMargin: 6 }
                    Repeater {
                        model: ProfileController.recentActivity
                        Item {
                            required property string modelData
                            required property int index
                            Layout.fillWidth: true
                            height: 36
                            Txt {
                                text: modelData
                                font.pixelSize: 13
                                color: Theme.fgStrong
                                elide: Text.ElideRight
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width
                            }
                            HairLine { visible: index < ProfileController.recentActivity.length - 1; anchors.bottom: parent.bottom; width: parent.width }
                        }
                    }
                }
            }
        }
        Item { width: 1; height: 40 }
    }
}
