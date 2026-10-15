import QtQuick
import QtQuick.Layouts
import MangaDl

SettingsSection {
    title: "Reader"
    caption: "Defaults for every manga. You can override them per series from the reader."

    TwoColumns {
        leftCards: [
            SettingsCard {
                heading: "Reading"
                SettingRow { label: "Reading mode"; below: [ Segmented { model: ["Left to right", "Right to left", "Vertical", "Webtoon"] } ] }
                SettingRow { label: "Two-page spread"; description: "Show pages side by side on wide windows."; MdSwitch { checked: true } }
                SettingRow { label: "Crop borders"; description: "Trim white or black margins around pages."; MdSwitch {} }
                SettingRow {
                    label: "Webtoon side padding"; description: "Narrow the strip on wide screens."; showDivider: false
                    below: [
                        RowLayout {
                            width: parent.width
                            spacing: 12
                            MdSlider { id: padding; Layout.fillWidth: true; from: 0; to: 40; stepSize: 5; value: 15; Accessible.name: "Webtoon side padding" }
                            Txt { text: Math.round(padding.value) + "%"; font.pixelSize: 13; font.weight: Font.ExtraBold; Layout.preferredWidth: 44; horizontalAlignment: Text.AlignRight }
                        }
                    ]
                }
            },
            SettingsCard {
                heading: "Controls"
                SettingRow { label: "Click zones"; description: "Click the left or right edge to turn pages."; MdSwitch { checked: true } }
                SettingRow { label: "Mouse wheel"; below: [ Segmented { model: ["Scroll", "Turn page", "Zoom"] } ] }
                SettingRow {
                    label: "Keyboard shortcuts"; description: "← → turn pages · F fullscreen · Esc exit"; showDivider: false
                    Btn { kind: "ghost"; size: "small"; text: "Customise" }
                }
            }
        ]
        rightCards: [
            SettingsCard {
                heading: "Display"
                SettingRow { label: "Page fit"; below: [ Segmented { model: ["Fit height", "Fit width", "Original"] } ] }
                SettingRow { label: "Background"; below: [ Segmented { model: ["Black", "Gray", "White"] } ] }
                SettingRow { label: "Show page number"; MdSwitch { checked: true } }
                SettingRow { label: "Open in fullscreen"; showDivider: false; MdSwitch {} }
            },
            SettingsCard {
                heading: "Image filters"
                SettingRow { label: "Brightness"; below: [ MdSlider { width: parent.width; from: -50; to: 50; stepSize: 5; value: 0; Accessible.name: "Brightness" } ] }
                SettingRow { label: "Grayscale"; MdSwitch {} }
                SettingRow { label: "Invert colours"; MdSwitch {} }
                SettingRow { label: "Sharpen scaled pages"; showDivider: false; MdSwitch { checked: true } }
            }
        ]
    }
}
