import QtQuick
import QtQuick.Layouts
import MangaDl

// Interactive Quick Tour modal introducing new or returning users to key features.
Item {
    id: tourModal
    anchors.fill: parent
    z: 20
    visible: opacity > 0
    opacity: 0

    Behavior on opacity { NumberAnimation { duration: Theme.fade } }

    property string tourId: "library" // "library" or "settings"
    property int currentStep: 0
    property var steps: []

    function open() {
        currentStep = 0
        opacity = 1
    }

    function close() {
        opacity = 0
        if (tourId === "library") {
            AppSettings.libraryTourCompleted = true
        } else if (tourId === "settings") {
            AppSettings.settingsTourCompleted = true
        }
        finished()
    }

    signal finished()

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        MouseArea { anchors.fill: parent } // block background clicks
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 48)
        height: cardContent.implicitHeight + 56
        radius: 20
        color: Theme.sheet
        border.color: Theme.track
        border.width: 1

        ColumnLayout {
            id: cardContent
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 28 }
            spacing: 20

            // Top Header: Category / Tour tag + step indicator
            RowLayout {
                Layout.fillWidth: true
                Eyebrow {
                    text: tourId === "settings" ? "SETTINGS TOUR" : "QUICK TOUR"
                    color: Theme.accentLight
                    Layout.fillWidth: true
                }
                Txt {
                    text: (tourModal.currentStep + 1) + " of " + tourModal.steps.length
                    color: Theme.fgSubtle
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
            }

            // Step Card
            Item {
                Layout.fillWidth: true
                implicitHeight: 180

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        spacing: 14
                        Rectangle {
                            width: 44
                            height: 44
                            radius: 12
                            color: Theme.accentSoft
                            border.color: Theme.accent
                            border.width: 1

                            Icon {
                                anchors.centerIn: parent
                                name: (tourModal.steps[tourModal.currentStep] && tourModal.steps[tourModal.currentStep].icon) || "Play"
                                size: 20
                                color: Theme.accentLight
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            DisplayText {
                                Layout.fillWidth: true
                                text: (tourModal.steps[tourModal.currentStep] && tourModal.steps[tourModal.currentStep].title) || ""
                                font.pixelSize: 28
                                wrapMode: Text.Wrap
                            }
                            Caption {
                                Layout.fillWidth: true
                                text: (tourModal.steps[tourModal.currentStep] && tourModal.steps[tourModal.currentStep].subtitle) || ""
                                color: Theme.accentLight
                            }
                        }
                    }

                    BodyMuted {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        text: (tourModal.steps[tourModal.currentStep] && tourModal.steps[tourModal.currentStep].description) || ""
                        font.pixelSize: 14
                        lineHeight: 22
                        wrapMode: Text.Wrap
                    }
                }
            }

            // Progress dots
            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8
                Repeater {
                    model: tourModal.steps.length
                    Rectangle {
                        required property int index
                        width: index === tourModal.currentStep ? 24 : 8
                        height: 8
                        radius: 4
                        color: index === tourModal.currentStep ? Theme.accent : Theme.track
                        Behavior on width { NumberAnimation { duration: 160 } }
                        Behavior on color { ColorAnimation { duration: 160 } }
                    }
                }
            }

            // Bottom action row
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                spacing: 12

                Btn {
                    kind: "ghost"
                    text: "Skip Tour"
                    onClicked: tourModal.close()
                }

                Item { Layout.fillWidth: true }

                Btn {
                    kind: "ghost"
                    text: "Previous"
                    visible: tourModal.currentStep > 0
                    onClicked: {
                        if (tourModal.currentStep > 0) {
                            tourModal.currentStep--
                        }
                    }
                }

                Btn {
                    text: tourModal.currentStep === tourModal.steps.length - 1 ? "Get Started" : "Next"
                    leftPadding: 22
                    rightPadding: 22
                    onClicked: {
                        if (tourModal.currentStep < tourModal.steps.length - 1) {
                            tourModal.currentStep++
                        } else {
                            tourModal.close()
                        }
                    }
                }
            }
        }
    }
}
