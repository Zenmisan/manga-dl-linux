import QtQuick
import QtQuick.Layouts
import MangaDl

// A faint card with an eyebrow heading; children are SettingRows (or anything).
Card {
    property string heading
    kind: "faint"
    width: parent ? parent.width : 0
    Eyebrow { text: heading; Layout.bottomMargin: 6; visible: heading !== "" }
}
