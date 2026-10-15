import QtQuick
import QtQuick.Layouts
import MangaDl

// Scrolling body of one settings section: title, caption, then cards.
PageScroll {
    property string title
    property string caption
    default property alias cards: body.data

    padding: 0
    topPadding: 0
    spacing: 20
    maxContentWidth: 1040

    Column {
        SectionTitle { text: title }
        Caption { text: caption; font.pixelSize: 13; topPadding: 4 }
    }
    Column {
        id: body
        width: parent.width
        spacing: 20
    }
}
