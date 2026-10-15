import QtQuick
import QtQuick.Layouts
import MangaDl

// One settings row: label + description on the left, a control on the right,
// optional full-width control underneath, and a hairline at the bottom.
//   SettingRow { label: "Launch on startup"; description: "…"; MdSwitch { checked: true } }
//   SettingRow { label: "Theme"; below: [ Segmented { model: ["Dark", "AMOLED"] } ] }
Item {
    id: row
    property string label
    property string description
    property bool showDivider: true
    default property alias control: trailing.data
    property alias below: belowSlot.data

    Layout.fillWidth: true
    implicitHeight: Math.max(56, grid.implicitHeight + 12)

    ColumnLayout {
        id: grid
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Txt { Layout.fillWidth: true; text: row.label; font.weight: Font.DemiBold; wrapMode: Text.Wrap }
                Caption {
                    Layout.fillWidth: true
                    visible: row.description !== ""
                    text: row.description
                    wrapMode: Text.Wrap
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 17
                }
            }
            Item {
                id: trailing
                implicitWidth: childrenRect.width
                implicitHeight: childrenRect.height
                visible: children.length > 0
            }
        }
        Item {
            id: belowSlot
            Layout.fillWidth: true
            implicitHeight: childrenRect.height
            visible: children.length > 0
        }
    }

    HairLine {
        visible: row.showDivider
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        color: Theme.divider
    }
}
