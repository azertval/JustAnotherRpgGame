import QtQuick

/*!
    Doublure de `hmi::AssetGalleryItem` (type QML `AssetGalleryViewport`) pour Qt Design Studio.

    La vraie surface est la galerie des assets, un outil de debug qui dessine les assets livres par
    QRhi. Dans l'atelier, un aplat de la couleur d'effacement et une galerie vide suffisent.
*/
Rectangle {
    property real zoom: 1
    property real offsetX: 0
    property real offsetY: 0
    readonly property real cellSize: 68 * zoom
    property bool showGrid: true
    property bool showFootprint: true
    property bool playing: true
    property real speed: 1
    property color clearColor: "transparent"
    property int sampleCount: 1

    property int selectedIndex: -1
    readonly property var selected: ({})
    readonly property int selectedFrame: 0
    readonly property var siblings: []

    readonly property int blocCount: 0
    readonly property int drawnCount: 0
    readonly property int preloadedCount: 0
    readonly property int unloadedCount: 0
    readonly property var labels: []
    readonly property var bands: []
    readonly property var minimap: []
    readonly property int layoutColumns: 0
    readonly property int layoutRows: 0
    readonly property real viewColumn: 0
    readonly property real viewRow: 0
    readonly property real viewColumns: 0
    readonly property real viewRows: 0
    readonly property real ringCells: 3
    readonly property var errors: []

    function panBy(dx, dy) {}
    function zoomAt(zoom, x, y) {}
    function blocAt(x, y) {
        return -1
    }
    function select(index) {}
    function centerOn(index) {}
    function centerOnCell(column, row) {}
    function showBand(band) {}
    function step(frames) {}

    color: clearColor
}
