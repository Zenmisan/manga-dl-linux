pragma Singleton
import QtQuick

// manga-dl design tokens (design.md). One place for every colour, font and size.
// Accent colours are plain properties, so setAccent() recolours the whole app live.
QtObject {
    // ---- Fonts ----
    readonly property string display: "Anton"      // headings, always uppercase
    readonly property string body: "Inter"         // everything else
    readonly property string serif: "PT Serif"     // novel reader only

    // ---- Grounds ----
    readonly property color bg: "#050505"
    readonly property color navBg: "#080808"
    readonly property color titleBar: "#0A0A0A"
    readonly property color panel: "#0B0B0B"
    readonly property color sheet: "#111111"
    readonly property color toast: "#161616"
    readonly property color readerBg: "#000000"
    readonly property color novelBg: "#0D0D0D"
    readonly property color scrim: "#A6000000"

    // ---- Text ----
    readonly property color fg: "#FAFAFA"
    readonly property color fgStrong: "#D9FFFFFF"
    readonly property color fgSoft: "#BFFFFFFF"
    readonly property color fgMuted: "#B8FFFFFF"
    readonly property color fgSubtle: "#99FFFFFF"
    readonly property color fgFaint: "#8CFFFFFF"
    readonly property color placeholder: "#73FFFFFF"
    readonly property color white: "#FFFFFF"
    readonly property color ink: "#050505"

    // ---- Surfaces and lines ----
    readonly property color surface3: "#08FFFFFF"
    readonly property color surface4: "#0AFFFFFF"
    readonly property color surface: "#0DFFFFFF"
    readonly property color surfaceRaised: "#0FFFFFFF"
    readonly property color surfaceHigh: "#14FFFFFF"
    readonly property color track: "#1FFFFFFF"
    readonly property color line: "#24FFFFFF"
    readonly property color lineStrong: "#2EFFFFFF"
    readonly property color divider: "#0FFFFFFF"
    readonly property color dividerStrong: "#14FFFFFF"
    readonly property color switchOff: "#33FFFFFF"
    readonly property color hover: "#12FFFFFF"
    readonly property color darkChip: "#59000000"

    // ---- Accent (changed by setAccent) ----
    property color accent: "#DC2626"
    property color accentLight: "#EF4444"
    property color accentSoft: "#FCA5A5"
    readonly property color accentMuted: alpha(accent, 0x29)
    readonly property color accentFaint: alpha(accent, 0x14)
    readonly property color accentWash: alpha(accent, 0x0D)
    readonly property color accentNav: alpha(accent, 0x2E)
    property string accentName: "Red"

    // ---- Status ----
    readonly property color success: "#16A34A"
    readonly property color successText: "#4ADE80"
    readonly property color successSoft: "#2E16A34A"
    readonly property color successWash: "#0F16A34A"
    readonly property color successLine: "#5916A34A"
    readonly property color errorText: "#F87171"
    readonly property color errorBorder: "#99EF4444"
    readonly property color errorSoft: "#2EDC2626"
    readonly property color errorBar: "#7F1D1D"
    readonly property color danger: "#B91C1C"

    // ---- Reader page paper ----
    readonly property color paper: "#E9E6DF"
    readonly property color paperPanel: "#D4D0C6"
    readonly property color paperInk: "#1A1A1A"
    readonly property color paperText: "#3A3A3A"

    // ---- Motion: fade only ----
    readonly property int fade: 220

    // Accent options from Settings › General. Red is the brand default.
    readonly property var accents: [
        { name: "Red", main: "#DC2626", light: "#EF4444", soft: "#FCA5A5" },
        { name: "Blue", main: "#2563EB", light: "#3B82F6", soft: "#93C5FD" },
        { name: "Purple", main: "#7C3AED", light: "#8B5CF6", soft: "#C4B5FD" },
        { name: "Green", main: "#16A34A", light: "#22C55E", soft: "#86EFAC" },
        { name: "Orange", main: "#EA580C", light: "#F97316", soft: "#FDBA74" },
        { name: "Pink", main: "#DB2777", light: "#EC4899", soft: "#F9A8D4" }
    ]

    function setAccent(name) {
        for (const a of accents) {
            if (a.name === name) {
                accent = a.main; accentLight = a.light; accentSoft = a.soft; accentName = a.name
                if (typeof AppSettings !== "undefined" && AppSettings) {
                    AppSettings.accentColor = a.name
                }
                return
            }
        }
    }

    Component.onCompleted: {
        if (typeof AppSettings !== "undefined" && AppSettings && AppSettings.accentColor) {
            setAccent(AppSettings.accentColor)
        }
    }

    function alpha(c, a) { return Qt.rgba(c.r, c.g, c.b, a / 255) }
}
