import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import MangaDl

// Help center: FAQ accordion and a contact form.
Item {
    id: page
    property int openIndex: 0

    readonly property var faq: [
        ["Where are downloads saved?", "As CBZ files with ComicInfo.xml. Change the folder in Settings › System › Download location."],
        ["How do I add a source?", "Open Browse › Extensions and install one, or connect Komga or Suwayomi in Settings › System."],
        ["How does cloud sync work?", "Sign in and your library, history and progress sync through your backend automatically."],
        ["Why won’t a chapter load?", "The source may be down or rate-limiting. Retry, or open it in the browser to check."]
    ]

    PageScroll {
        anchors.fill: parent
        maxContentWidth: 1120

        RowLayout {
            width: parent.width
            spacing: 40

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 3
                Layout.alignment: Qt.AlignTop
                spacing: 18
                PageTitle { text: "Help center" }
                Eyebrow { text: "Frequently asked"; Layout.topMargin: 8 }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Repeater {
                        model: page.faq
                        Column {
                            id: entry
                            required property var modelData
                            required property int index
                            readonly property bool expanded: page.openIndex === index
                            Layout.fillWidth: true
                            Clickable {
                                width: parent.width
                                height: 56
                                radius: 0
                                Accessible.name: entry.modelData[0]
                                Accessible.role: Accessible.Button
                                onClicked: page.openIndex = entry.expanded ? -1 : entry.index
                                Txt {
                                    width: parent.width - 30
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    text: entry.modelData[0]
                                    font.pixelSize: 15
                                    font.weight: entry.expanded ? Font.Bold : Font.DemiBold
                                }
                                Icon { x: parent.width - width; y: (parent.height - height) / 2; name: entry.expanded ? "ChevronUp" : "ChevronDown"; size: 18 }
                            }
                            BodyMuted {
                                visible: entry.expanded
                                width: parent.width
                                text: entry.modelData[1]
                                bottomPadding: 16
                            }
                            HairLine { width: parent.width; color: Theme.dividerStrong }
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 64
                kind: "faint"
                spacing: 14
                Eyebrow { text: "Contact support"; muted: true }
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    ButtonGroup { id: category }
                    Repeater {
                        model: ["Bug Report", "Feature Request", "Account", "Source / Extension"]
                        Chip { required property string modelData; small: true; leftPadding: 12; text: modelData; checked: modelData === "Bug Report"; ButtonGroup.group: category }
                    }
                }
                FieldLabel { text: "Message" }
                MdTextArea {
                    id: messageField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    placeholderText: "What happened? Include the source and chapter if it's a loading issue."
                }
                Btn {
                    Layout.fillWidth: true
                    text: "Send Message"
                    implicitHeight: 46
                    fontSize: 14
                    onClicked: {
                        const cat = category.checkedButton ? category.checkedButton.text : "Support"
                        const subject = encodeURIComponent("[manga-dl Linux] " + cat)
                        const body = encodeURIComponent(messageField.text)
                        Qt.openUrlExternally("mailto:support@manga-dl.app?subject=" + subject + "&body=" + body)
                        Nav.toast("Opening email client to send message...")
                    }
                }
            }
        }
    }
}
