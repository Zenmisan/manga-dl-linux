import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Dropdown in the app's style.  MdCombo { model: ["Daily", "Weekly"]; currentIndex: 0 }
ComboBox {
    id: control
    implicitHeight: 40
    implicitWidth: 160
    font.family: Theme.body
    font.pixelSize: 13
    font.weight: Font.DemiBold
    leftPadding: 14
    rightPadding: 36
    hoverEnabled: true

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: Theme.fg
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Icon {
        name: "ChevronDown"; size: 16; color: Theme.fgSubtle
        x: control.width - width - 14
        anchors.verticalCenter: parent.verticalCenter
    }
    background: Rectangle {
        radius: 10
        color: control.hovered ? Theme.surfaceHigh : Theme.surface
        border.width: 1
        border.color: control.visualFocus || control.popup.visible ? Theme.accent : Theme.line
    }
    delegate: ItemDelegate {
        id: option
        required property int index
        required property var modelData
        width: control.width - 8
        x: 4
        height: 36
        hoverEnabled: true
        contentItem: Text {
            text: option.modelData
            font: control.font
            color: option.index === control.currentIndex ? Theme.fg : Theme.fgSoft
            verticalAlignment: Text.AlignVCenter
            leftPadding: 6
        }
        background: Rectangle {
            radius: 8
            color: option.index === control.currentIndex ? Theme.accentMuted : option.hovered ? Theme.hover : "transparent"
        }
    }
    popup: Popup {
        y: control.height + 4
        width: control.width
        padding: 4
        implicitHeight: contentItem.implicitHeight + 8
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
        }
        background: Rectangle { radius: 12; color: Theme.sheet; border.color: Theme.track }
    }
}
