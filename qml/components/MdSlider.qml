import QtQuick
import QtQuick.Controls.Basic
import MangaDl

// Accent slider with a white knob.
Slider {
    id: control
    implicitHeight: 28
    stepSize: 1
    snapMode: Slider.SnapAlways
    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: 4
        radius: 2
        color: Theme.track
        Rectangle { width: control.visualPosition * parent.width; height: parent.height; radius: 2; color: Theme.accent }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: 18; height: 18; radius: 9
        color: Theme.white
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.accentLight
    }
}
