import QtQuick
import MangaDl

// Responsive uniform grid: as many columns of at least `minItemWidth` as fit (capped by
// `maxColumns`); items stretch to fill the row. Delegates size themselves with
// `width: parent.cellWidth`.
Item {
    id: root
    property alias model: rep.model
    property alias delegate: rep.delegate
    property real minItemWidth: 140
    property int maxColumns: 0          // 0 = no cap
    property real spacing: 20
    property real rowSpacing: spacing
    readonly property int columns: {
        const n = Math.max(1, Math.floor((width + spacing) / (minItemWidth + spacing)))
        return maxColumns > 0 ? Math.min(n, maxColumns) : n
    }

    implicitHeight: grid.implicitHeight

    Grid {
        id: grid
        readonly property real cellWidth: (root.width - root.spacing * (root.columns - 1)) / root.columns
        width: root.width
        columns: root.columns
        columnSpacing: root.spacing
        rowSpacing: root.rowSpacing
        Repeater { id: rep }
    }
}
