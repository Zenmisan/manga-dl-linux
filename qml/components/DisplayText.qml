import QtQuick
import MangaDl

// Anton, always uppercase (headings, big numbers).
Text {
    property bool upper: true
    color: Theme.fg
    font.family: Theme.display
    font.pixelSize: 24
    font.capitalization: upper ? Font.AllUppercase : Font.MixedCase
    // Anton's line box is tall; headings use ~0.92 of the font size like the designs.
    lineHeightMode: Text.FixedHeight
    lineHeight: Math.round(font.pixelSize * 0.92)
    textFormat: Text.PlainText
}
