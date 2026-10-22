import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// The app's one button.
//   kind: primary | ghost | danger | dangerFill | soft | success | icon | link | plain
//   size: large (50) | normal (42) | small (36) | pill (36, round)
//   iconName: optional leading icon (or the only content when kind is "icon")
Button {
    id: control
    property string kind: "primary"
    property string size: "normal"
    property string iconName: ""
    property bool alignLeft: false
    property int iconSize: kind === "icon" ? 20 : 16
    property color textColor: {
        switch (kind) {
        case "primary": case "dangerFill": return Theme.white
        case "danger": return Theme.errorText
        case "soft": return Theme.accentSoft
        case "success": return Theme.successText
        case "link": return Theme.accentLight
        default: return Theme.fg
        }
    }
    property color fillColor: {
        switch (kind) {
        case "primary": return Theme.accent
        case "dangerFill": return Theme.danger
        case "soft": return Theme.accentMuted
        case "success": return Theme.successSoft
        default: return "transparent"
        }
    }
    property color borderColor: kind === "ghost" ? Theme.lineStrong : kind === "danger" ? Theme.errorBorder : "transparent"
    property int radius: kind === "link" ? 6 : size === "pill" ? height / 2 : size === "small" ? 10 : kind === "icon" ? 10 : 12
    property int fontSize: size === "large" ? 15 : (size === "small" || size === "pill") ? 12 : 13

    implicitHeight: kind === "link" ? implicitContentHeight + topPadding + bottomPadding
                  : kind === "icon" ? 44
                  : size === "large" ? 50 : (size === "small" || size === "pill") ? 36 : 42
    implicitWidth: kind === "icon" ? implicitHeight : implicitContentWidth + leftPadding + rightPadding
    leftPadding: kind === "icon" ? 0 : kind === "link" ? 4 : (size === "small" || size === "pill") ? 14 : 18
    rightPadding: leftPadding
    topPadding: kind === "link" ? 6 : 0
    bottomPadding: topPadding
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.name: text

    font.family: Theme.body
    font.pixelSize: fontSize
    font.weight: (kind === "ghost" || kind === "danger" || kind === "link") && size !== "small" ? Font.Bold : Font.ExtraBold

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            x: control.alignLeft ? 0 : Math.round((parent.width - width) / 2)
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Icon {
                visible: control.iconName !== ""
                name: control.iconName
                size: control.iconSize
                color: control.textColor
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                visible: control.text !== ""
                text: control.text
                font: control.font
                color: control.textColor
                opacity: control.enabled ? 1 : 0.5
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                width: control.alignLeft && control.width > 0
                    ? Math.max(0, control.width - control.leftPadding - control.rightPadding - (control.iconName !== "" ? (control.iconSize + 8) : 0))
                    : implicitWidth
            }
        }
    }

    background: Rectangle {
        radius: control.radius
        color: control.fillColor
        border.color: control.visualFocus ? Theme.accentLight : control.borderColor
        border.width: control.visualFocus || control.borderColor.a > 0 ? 1 : 0
        opacity: control.enabled ? (control.down ? 0.85 : 1) : 0.4
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Theme.hover
            opacity: control.down ? 0.6 : control.hovered ? 1 : 0
        }
    }
}
