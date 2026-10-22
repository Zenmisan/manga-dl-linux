import QtQuick
import QtQuick.Layouts
import MangaDl

// Label above a Field.
ColumnLayout {
    id: root
    property alias label: lbl.text
    property alias field: input
    property alias text: input.text
    property alias placeholderText: input.placeholderText
    property alias password: input.password
    property alias error: input.error
    signal accepted()

    function forceActiveFocus() {
        input.forceActiveFocus()
    }

    Layout.fillWidth: true
    spacing: 8
    FieldLabel { id: lbl }
    Field {
        id: input
        Layout.fillWidth: true
        Accessible.name: lbl.text
        onAccepted: root.accepted()
    }
}
