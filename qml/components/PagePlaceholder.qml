import QtQuick
import QtQuick.Layouts
import MangaDl

// Manga page view: loads real page images via image://page/ with fallback to paper outline.
Rectangle {
    id: root
    property string label: ""
    property string pageUrl: ""
    color: Theme.paper
    clip: true

    // Placeholder paper skeleton while loading or when no URL is available
    GridLayout {
        anchors.fill: parent
        anchors.margins: 18
        columns: 2
        rowSpacing: 10
        columnSpacing: 10
        visible: img.status !== Image.Ready
        Rectangle {
            Layout.columnSpan: 2; Layout.fillWidth: true; Layout.fillHeight: true
            color: "#D4D0C6"; border.color: Theme.paperInk; border.width: 2
            Txt { anchors.centerIn: parent; text: root.label; color: Theme.paperText; font.pixelSize: 13; font.weight: Font.Bold }
        }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#DCD8CF"; border.color: Theme.paperInk; border.width: 2 }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#CFCAC0"; border.color: Theme.paperInk; border.width: 2 }
        Rectangle { Layout.columnSpan: 2; Layout.fillWidth: true; Layout.fillHeight: true; color: "#D8D4CA"; border.color: Theme.paperInk; border.width: 2 }
    }

    Image {
        id: img
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: {
            if (!root.pageUrl) return ""
            if (root.pageUrl.startsWith("image://") || root.pageUrl.startsWith("file:") || root.pageUrl.startsWith("qrc:"))
                return root.pageUrl
            return "image://page/" + encodeURIComponent(root.pageUrl)
        }
        opacity: status === Image.Ready ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
}
