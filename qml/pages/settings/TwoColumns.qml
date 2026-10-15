import QtQuick
import MangaDl

// Two equal columns of cards. Put cards in `leftCards` and `rightCards`.
Item {
    property alias leftCards: leftCol.data
    property alias rightCards: rightCol.data
    width: parent ? parent.width : 0
    implicitHeight: Math.max(leftCol.implicitHeight, rightCol.implicitHeight)
    Column { id: leftCol; width: (parent.width - 20) / 2; spacing: 20 }
    Column { id: rightCol; x: (parent.width + 20) / 2; width: (parent.width - 20) / 2; spacing: 20 }
}
