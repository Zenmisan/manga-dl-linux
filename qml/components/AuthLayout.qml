import QtQuick
import QtQuick.Layouts
import MangaDl

// Two-column first-run layout: big hero on the left (bottom-aligned, with a glow),
// form on the right. Put the form's fields in as children.
Item {
    id: root
    property string eyebrow: ""
    property string title: ""
    property int titleSize: 120
    property string subtitle: ""
    property bool bloomTop: false
    property alias heroExtra: heroExtraSlot.data
    default property alias form: formColumn.data
    property alias formSpacing: formColumn.spacing

    // Hero
    Item {
        id: hero
        width: parent.width / 2
        height: parent.height
        clip: true
        Bloom {
            anchors.fill: parent
            strength: root.bloomTop ? 0.22 : 0.2
            cx: root.bloomTop ? 0.1 : 0.15
            cy: root.bloomTop ? 0.08 : 1.0
            rx: root.bloomTop ? 0.8 : 0.85
            ry: root.bloomTop ? 0.6 : 0.65
        }
        Rectangle { anchors { top: parent.top; bottom: parent.bottom; right: parent.right } width: 1; color: Theme.dividerStrong }

        ColumnLayout {
            anchors { left: parent.left; right: parent.right; top: root.bloomTop ? parent.top : undefined; bottom: root.bloomTop ? undefined : parent.bottom
                      leftMargin: 64; rightMargin: 64; topMargin: 72; bottomMargin: 72 }
            spacing: 16
            Eyebrow { visible: root.eyebrow !== ""; text: root.eyebrow }
            DisplayText {
                Layout.fillWidth: true
                text: root.title
                font.pixelSize: root.titleSize
                wrapMode: Text.Wrap
            }
            BodyMuted {
                visible: root.subtitle !== ""
                Layout.maximumWidth: 440
                Layout.fillWidth: true
                text: root.subtitle
                font.pixelSize: 17
                lineHeight: 26
            }
        }
        Item {
            id: heroExtraSlot
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 64; rightMargin: 64; bottomMargin: 72 }
            height: childrenRect.height
        }
    }

    // Form
    ColumnLayout {
        id: formColumn
        x: hero.width + 96
        width: Math.min(420, parent.width - hero.width - 96 - 40)
        anchors.verticalCenter: parent.verticalCenter
        spacing: 16
    }
}
