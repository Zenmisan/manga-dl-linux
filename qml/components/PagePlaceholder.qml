import QtQuick
import QtQuick.Layouts
import MangaDl

// Manga page view: loads real page images via image://page/ with smooth scroll for webtoon strips in Single mode,
// and a sleek, unobtrusive dark loading state.
Rectangle {
    id: root
    property string label: ""
    property string pageUrl: ""
    property int fillMode: Image.PreserveAspectFit
    color: "#050505"
    clip: true

    readonly property bool isStrip: img.status === Image.Ready && img.implicitWidth > 0 && (img.implicitHeight / img.implicitWidth) > 1.8

    // Clean, subtle dark loading state (no heavy borders or bulky placeholder boxes)
    Rectangle {
        anchors.fill: parent
        color: "#08080A"
        visible: img.status !== Image.Ready

        // Error message if loading failed
        Column {
            anchors.centerIn: parent
            spacing: 12
            visible: img.status === Image.Error

            Txt {
                text: "Failed to load page"
                color: Theme.danger
                font.pixelSize: 13
                font.weight: Font.DemiBold
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Btn {
                kind: "ghost"; size: "small"; text: "Retry"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    const u = root.pageUrl
                    root.pageUrl = ""
                    root.pageUrl = u
                }
            }
        }

        // Sleek subtle pulse bar while fetching page image
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            color: Theme.surfaceRaised
            visible: img.status === Image.Loading

            Rectangle {
                id: pulseBar
                width: parent.width * 0.35
                height: 2
                color: Theme.accentLight
                PropertyAnimation on x {
                    from: -pulseBar.width
                    to: root.width
                    duration: 1400
                    loops: Animation.Infinite
                    running: img.status === Image.Loading
                }
            }
        }
    }

    // Scrollable container for tall webtoon strips in Single mode
    Flickable {
        id: stripScroller
        visible: root.isStrip
        anchors.fill: parent
        contentWidth: width
        contentHeight: stripImg.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Image {
            id: stripImg
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width, 860)
            height: img.implicitWidth > 0 ? Math.round(width * (img.implicitHeight / img.implicitWidth)) : parent.height
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            source: img.source
            opacity: img.status === Image.Ready ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }

    // Regular aspect fit for standard manga pages
    Image {
        id: img
        visible: !root.isStrip
        anchors.fill: parent
        asynchronous: true
        fillMode: root.fillMode
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
