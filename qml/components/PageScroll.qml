import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Vertical page scroller with a thin overlay scrollbar.
// Children go into a full-width column; set `padding` for the page gutter.
Flickable {
    id: flick
    property int padding: 40
    property int topPadding: 28
    property int bottomPadding: 40
    property alias spacing: column.spacing
    property int maxContentWidth: 0          // 0 = no limit
    default property alias content: column.data

    clip: true
    contentWidth: width
    contentHeight: column.implicitHeight + topPadding + bottomPadding
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick

    Column {
        id: column
        x: flick.padding
        y: flick.topPadding
        width: flick.maxContentWidth > 0 ? Math.min(flick.maxContentWidth, flick.width - flick.padding * 2)
                                         : flick.width - flick.padding * 2
        spacing: 24
    }

    ScrollBar.vertical: ScrollBar {
        id: bar
        policy: flick.contentHeight > flick.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        contentItem: Rectangle {
            implicitWidth: 6; radius: 3
            color: Theme.lineStrong
            opacity: bar.active || bar.hovered ? 1 : 0.5
        }
        background: Item {}
    }
}
