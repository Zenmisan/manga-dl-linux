import QtQuick
import QtQuick.Layouts
import MangaDl

// Rounded panel whose children stack in a column.
// kind: "card" (raised), "faint" (settings sections) or "list".
Rectangle {
    id: card
    property string kind: "card"
    property int padding: kind === "list" ? 12 : 20
    property alias spacing: column.spacing
    default property alias content: column.data

    implicitWidth: column.implicitWidth + padding * 2
    implicitHeight: column.implicitHeight + padding * 2
    radius: kind === "list" ? 14 : 16
    color: kind === "faint" ? Theme.surface3 : kind === "list" ? Theme.surface : Theme.surfaceRaised
    border.color: Theme.dividerStrong
    border.width: 1

    ColumnLayout {
        id: column
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: card.padding }
        spacing: 0
    }
}
