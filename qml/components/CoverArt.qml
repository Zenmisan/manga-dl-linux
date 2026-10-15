import QtQuick
import MangaDl

// Real cover art with asynchronous image loading via image://cover/ provider,
// maintaining color fallback while loading or on error.
Rectangle {
    id: root
    property string cover: "#22222A"
    property int unread: 0
    property bool downloaded: false
    radius: 12
    color: cover && cover.startsWith("#") ? cover : "#22222A"
    clip: true

    readonly property bool isUrl: !!root.cover && !root.cover.startsWith("#")

    Image {
        id: img
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        source: {
            if (!root.isUrl) return ""
            if (root.cover.startsWith("image://") || root.cover.startsWith("qrc:") || root.cover.startsWith("file:"))
                return root.cover
            return "image://cover/" + encodeURIComponent(root.cover)
        }
        opacity: status === Image.Ready ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        gradient: Gradient {
            GradientStop { position: 0.55; color: "transparent" }
            GradientStop { position: 1.0; color: "#40000000" }
        }
    }

    Badge {
        visible: root.unread > 0
        text: root.unread
        height: 22; radius: 11
        anchors { left: parent.left; top: parent.top; margins: 8 }
        z: 2
    }

    Rectangle {
        visible: root.downloaded
        width: 24; height: 24; radius: 12
        color: "#99000000"
        anchors { right: parent.right; top: parent.top; margins: 8 }
        Icon { anchors.centerIn: parent; name: "Download"; size: 13 }
        z: 2
    }
}
