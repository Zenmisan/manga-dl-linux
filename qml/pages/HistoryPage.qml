import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Reading history: two columns of resume cards.
Item {
    PageScroll {
        anchors.fill: parent
        spacing: 18

        RowLayout {
            width: parent.width
            spacing: 12
            PageTitle { text: "History"; Layout.fillWidth: true }
            SearchField {
                Layout.preferredWidth: 300
                placeholderText: "Search history"
                text: HistoryModel.searchFilter
                onTextChanged: HistoryModel.searchFilter = text
            }
            Btn {
                kind: "danger"
                text: "Clear History"
                enabled: HistoryModel.count > 0
                opacity: enabled ? 1.0 : 0.4
                onClicked: HistoryModel.clear()
            }
        }

        Row {
            spacing: 8
            ButtonGroup { id: range }
            Repeater {
                model: ["This week", "Today", "This month", "All time"]
                Chip {
                    required property string modelData
                    text: modelData
                    checked: modelData === HistoryModel.timeRange
                    ButtonGroup.group: range
                    onClicked: HistoryModel.timeRange = modelData
                }
            }
        }

        AutoGrid {
            width: parent.width
            minItemWidth: 420
            maxColumns: 2
            spacing: 12
            visible: HistoryModel.count > 0
            model: HistoryModel
            delegate: Rectangle {
                required property var modelData
                width: parent.cellWidth
                height: 110
                radius: 14
                color: Theme.surface4
                border.color: Theme.dividerStrong

                RowLayout {
                    anchors { fill: parent; margins: 12 }
                    spacing: 16
                    CoverArt {
                        width: 56; height: 84; radius: 8
                        cover: modelData.manga && modelData.manga.cover ? modelData.manga.cover : "#1A2433"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        ItemTitle { Layout.fillWidth: true; text: modelData.manga ? modelData.manga.title : "" }
                        Txt { Layout.fillWidth: true; text: modelData.where || ""; font.pixelSize: 13; color: Theme.fgSoft }
                        Txt { Layout.fillWidth: true; text: modelData.when || ""; font.pixelSize: 12; color: Theme.fgFaint }
                    }
                    Btn {
                        kind: "icon"; iconName: "Close"; iconSize: 18; textColor: Theme.fgSubtle; implicitHeight: 40
                        Accessible.name: "Remove from history"
                        onClicked: HistoryModel.remove(modelData.provider, modelData.mangaId, modelData.chapterId)
                    }
                    Btn {
                        text: "Resume"; implicitHeight: 40; radius: 10; leftPadding: 16; rightPadding: 16
                        onClicked: Nav.open("Reader", {
                            manga: modelData.manga,
                            chapterId: modelData.chapterId,
                            chapterTitle: modelData.chapterTitle
                        })
                    }
                }
            }
        }

        // Empty state
        Column {
            width: parent.width
            spacing: 14
            visible: HistoryModel.count === 0
            topPadding: 48
            bottomPadding: 48

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: "History"
                size: 48
                color: Theme.fgFaint
            }
            DisplayText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: HistoryModel.searchFilter.length > 0 ? "No history matches found" : "No reading history yet"
                font.pixelSize: 22
                color: Theme.fgSoft
            }
            Txt {
                anchors.horizontalCenter: parent.horizontalCenter
                text: HistoryModel.searchFilter.length > 0
                    ? "Try clearing the search filter."
                    : "Chapters you read will appear here with your latest progress."
                color: Theme.fgMuted
            }
        }
    }
}
