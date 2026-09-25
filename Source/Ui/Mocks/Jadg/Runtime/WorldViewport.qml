import QtQuick

/*!
    Doublure de `hmi::WorldViewportItem` (type QML `WorldViewport`) pour Qt Design Studio (LOT-09).

    La vraie surface dessine la carte courante par QRhi et publie son cadrage ; dans l'atelier, un
    aplat de la couleur d'effacement et un cadrage fixe suffisent a voir ou elle se pose.
*/
Rectangle {
    property var model: null
    property color clearColor: "transparent"

    readonly property real tileWidth: 64
    readonly property real tileHeight: 40
    readonly property real originX: width / 2 - tileWidth / 2
    readonly property real originY: tileHeight

    function cellAt(x, y) {
        return Qt.point(-1, -1)
    }

    color: clearColor
}
