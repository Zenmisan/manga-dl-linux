import QtQuick
import MangaDl

// Soft accent glow behind hero text, as a radial gradient filling this item.
// cx/cy place the centre (0–1); rx/ry are the radii as fractions of width/height.
Canvas {
    id: bloom
    property color color: Theme.accent
    property real strength: 0.22
    property real cx: 0.1
    property real cy: 0.08
    property real rx: 0.8
    property real ry: 0.6

    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        if (width <= 0 || height <= 0) return
        // Draw a circle gradient, then stretch it to the ellipse.
        const r = Math.max(width * rx, 1)
        ctx.save()
        ctx.translate(width * cx, height * cy)
        ctx.scale(1, (height * ry) / r)
        const g = ctx.createRadialGradient(0, 0, 0, 0, 0, r)
        g.addColorStop(0, Qt.rgba(color.r, color.g, color.b, strength))
        g.addColorStop(1, Qt.rgba(color.r, color.g, color.b, 0))
        ctx.fillStyle = g
        ctx.fillRect(-r, -r, r * 2, r * 2)
        ctx.restore()
    }
}
