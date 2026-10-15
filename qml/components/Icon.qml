import QtQuick
import MangaDl

// A line icon from the C++ IconProvider, tinted to `color`.
//   Icon { name: "Search"; size: 20; color: Theme.fgSubtle }
Image {
    property string name
    property int size: 20
    property color color: Theme.fg

    width: size
    height: size
    sourceSize: Qt.size(size, size)
    fillMode: Image.PreserveAspectFit
    smooth: true
    // The SVG renderer wants #RRGGBB; alpha goes on the item instead.
    opacity: color.a
    source: name ? "image://icon/" + name + "?color=" + encodeURIComponent(hex(color)) : ""

    function hex(c) {
        const h = v => ("0" + Math.round(v * 255).toString(16)).slice(-2)
        return "#" + h(c.r) + h(c.g) + h(c.b)
    }
}
