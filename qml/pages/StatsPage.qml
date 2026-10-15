import QtQuick
import QtQuick.Layouts
import MangaDl

// Reading statistics: tiles, a 52-week heatmap, goals and breakdowns.
Item {
    id: page

    function heat(level) {
        return level === 3 ? Theme.accent : level === 2 ? Theme.alpha(Theme.accent, 0xA6)
             : level === 1 ? Theme.alpha(Theme.accent, 0x59) : Theme.surface4
    }

    component BarRow: RowLayout {
        property var bar
        Layout.fillWidth: true
        spacing: 12
        Txt { text: bar.name; font.pixelSize: 13; font.weight: Font.DemiBold; Layout.preferredWidth: 100; elide: Text.ElideRight }
        Progress { Layout.fillWidth: true; height: 8; value: bar.fraction; fill: Theme.fg; color: Theme.surfaceHigh }
        Caption { text: bar.count.toString(); font.pixelSize: 12; Layout.preferredWidth: 32 }
    }

    PageScroll {
        anchors.fill: parent
        spacing: 22

        RowLayout {
            width: parent.width
            Column {
                Layout.fillWidth: true
                PageTitle { text: "Statistics" }
                Caption { text: "Your reading habits and Hunter rank, at a glance"; font.pixelSize: 14; topPadding: 4 }
            }

            // Hunter Rank pill
            Rectangle {
                radius: 12
                color: StatsController.rankBgColor !== "" ? StatsController.rankBgColor : Theme.surface
                border.width: 1
                border.color: StatsController.rankColor !== "" ? StatsController.rankColor : Theme.border
                implicitHeight: 38
                implicitWidth: rankRow.implicitWidth + 24

                RowLayout {
                    id: rankRow
                    anchors.centerIn: parent
                    spacing: 8
                    Rectangle {
                        width: 22; height: 22; radius: 4
                        color: StatsController.rankColor !== "" ? StatsController.rankColor : Theme.accent
                        Txt {
                            anchors.centerIn: parent
                            text: StatsController.rankCode
                            font.pixelSize: 12
                            font.weight: Font.Black
                            color: "#000000"
                        }
                    }
                    Txt {
                        text: StatsController.rankName
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        color: Theme.fg
                    }
                    Txt {
                        text: "· " + StatsController.score + " EXP"
                        font.pixelSize: 13
                        color: StatsController.rankColor !== "" ? StatsController.rankColor : Theme.accent
                    }
                }
            }
        }

        // Stat tiles
        RowLayout {
            width: parent.width
            spacing: 14
            Repeater {
                model: [
                    ["Chapters read", StatsController.chaptersRead.toString()],
                    ["Time reading", StatsController.timeReading],
                    ["Current streak", StatsController.currentStreak + "d"],
                    ["Reading pace", StatsController.readingPace]
                ]
                Card {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    kind: "list"; radius: 16; padding: 18; spacing: 8
                    Txt { text: modelData[0]; font.pixelSize: 13; font.weight: Font.DemiBold; color: Theme.fgSubtle }
                    DisplayText { text: modelData[1]; font.pixelSize: 40; upper: false }
                }
            }
        }

        // 52-week heatmap (7 rows, one column per week)
        Card {
            width: parent.width
            kind: "faint"
            spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Eyebrow { text: "Activity · last 52 weeks"; Layout.fillWidth: true }
                Caption { text: "Chapters per day" }
            }
            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: heatGrid.height
                contentWidth: heatGrid.width
                clip: true
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds
                Grid {
                    id: heatGrid
                    rows: 7
                    flow: Grid.TopToBottom
                    spacing: 4
                    Repeater {
                        model: StatsController.activityHeatmap
                        Rectangle { required property int modelData; width: 16; height: 16; radius: 3; color: page.heat(modelData) }
                    }
                }
            }
            Row {
                Layout.alignment: Qt.AlignRight
                spacing: 6
                Caption { text: "Less"; font.pixelSize: 11 }
                Repeater {
                    model: [0, 1, 2, 3]
                    Rectangle { required property int modelData; width: 10; height: 10; radius: 2; color: page.heat(modelData); anchors.verticalCenter: parent.verticalCenter }
                }
                Caption { text: "More"; font.pixelSize: 11 }
            }
        }

        // Goals and breakdowns
        RowLayout {
            width: parent.width
            spacing: 14
            Card {
                Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                kind: "faint"; spacing: 14
                Eyebrow { text: "Reading goals"; muted: true }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    RowLayout {
                        Txt { text: "Monthly chapters"; font.pixelSize: 13; font.weight: Font.DemiBold; Layout.fillWidth: true }
                        Txt { text: StatsController.monthlyGoalText; font.pixelSize: 13; color: Theme.fgSubtle }
                    }
                    Progress { Layout.fillWidth: true; height: 6; value: StatsController.monthlyGoalProgress }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    RowLayout {
                        Txt { text: "Yearly manga"; font.pixelSize: 13; font.weight: Font.DemiBold; Layout.fillWidth: true }
                        Txt { text: StatsController.yearlyGoalText; font.pixelSize: 13; color: Theme.fgSubtle }
                    }
                    Progress { Layout.fillWidth: true; height: 6; value: StatsController.yearlyGoalProgress }
                }

                Btn {
                    kind: "ghost"
                    size: "small"
                    text: "Refresh Goals"
                    onClicked: {
                        StatsController.refresh()
                        Nav.toast("Goals refreshed")
                    }
                }
            }
            Card {
                Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                kind: "faint"; spacing: 12
                Eyebrow { text: "By category"; muted: true }
                Repeater {
                    model: StatsController.byCategory
                    BarRow { required property var modelData; bar: modelData }
                }
                Txt {
                    visible: StatsController.byCategory.length === 0
                    text: "No categorized series yet"
                    font.pixelSize: 13
                    color: Theme.fgSubtle
                }
            }
            Card {
                Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.fillHeight: true
                kind: "faint"; spacing: 12
                Eyebrow { text: "By source"; muted: true }
                Repeater {
                    model: StatsController.bySource
                    BarRow { required property var modelData; bar: modelData }
                }
                Txt {
                    visible: StatsController.bySource.length === 0
                    text: "No series in library yet"
                    font.pixelSize: 13
                    color: Theme.fgSubtle
                }
            }
        }
    }
}
