import QtQuick
import MangaDl

// Small tracked-out uppercase label above a section.
Txt {
    property bool muted: false
    color: muted ? Theme.fgSubtle : Theme.accentLight
    font.pixelSize: 10
    font.weight: Font.Black
    font.letterSpacing: 2
    font.capitalization: Font.AllUppercase
}
