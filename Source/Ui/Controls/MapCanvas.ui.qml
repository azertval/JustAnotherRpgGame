pragma ComponentBehavior: Bound
import QtQuick
import Jadg.Ui

/*!
    Carte qu'on deplace et qu'on agrandit, avec ses reperes (LOT-94, LOT-95).

    Le fond des trois vues de l'ecran « Carte » -- le monde, une region, le plan d'une ville. Les
    cartes sont celles de l'auteur, peintes en 1920 x 1080 et SANS lettrage : tout nom est pose par
    le jeu, dans sa police et dans sa langue.

    `image` est un NOM DE FICHIER de `Source/Elements/Assets/Maps/` -- ou, pour la carte rendue d'une
    zone (LOT-121), un CHEMIN relatif a `Source/Elements/Assets/` (`Regions/.../Map/martpart.jpg`) ;
    le chemin relatif vaut dans l'atelier, depuis les sources et dans la ressource (meme mecanisme
    que les pieces de la charte).

    Sans panneau (`leftInset` nul), la carte COUVRE la zone : a `zoom` 1 elle la remplit sans bande,
    quitte a deborder d'un cote sur un ecran qui n'est pas en 16:9. Avec un panneau pose a gauche,
    elle TIENT ENTIERE a sa droite, centree en hauteur -- rien de la carte ne reste sous la fiche --,
    et l'agrandir lui fait remplir la zone. Les positions des reperes sont des fractions de l'image.

    - `markers` : `{ name, x, y, kind, number, gateway, visited, locked }` ; `activeIndex` designe
      le repere choisi.
    - `labels` : `{ name, kind, x, y }`, des noms de geographie sans repere ni fiche.
    - `frame` : la part de l'image qu'on montre, en fractions (tout, par defaut). Le niveau
      « quartier » du plan (LOT-96) agrandit ainsi le plan de la ville sur un quartier, faute de
      carte peinte du quartier ; les reperes sont alors des fractions de CE cadre.
    - `here` : le point ou se tient le heros, en fractions de la vue ; `x < 0` : il n'y est pas.
    - `markerHovered(index, inside)`, `markerActivated(index)`, `backRequested()` (clic droit) et
      `positionMarked(x, y)` (Ctrl+clic, la fraction sous le pointeur) remontent le pointeur ; le
      jumeau decide.

    `positionMarked` est detecte par une `MouseArea` posee DANS la `Flickable`, au meme niveau que
    le clic droit -- pas par un `TapHandler` sur un ancetre : la `Flickable` prend la main sur le
    bouton gauche a la pression pour son propre glisser-deposer, et un gestionnaire pose plus haut
    dans l'arbre ne fait pas partie de sa cooperation `childMouseEventFilter`, donc ne voit jamais
    le tap.
*/
Item {
    id: root

    property string image: "world.jpg"
    property var markers: [
        { name: "Central Empire", x: 0.634, y: 0.42, kind: "city", number: 0, gateway: true }
    ]
    property var labels: []
    property int activeIndex: -1

    /// La part de l'image montree, en fractions de l'image : `Qt.rect(0, 0, 1, 1)` la montre toute.
    property rect frame: Qt.rect(0, 0, 1, 1)

    /// Le heros, en fractions de la vue ; hors vue (`x < 0`), rien ne se pose.
    property point here: Qt.point(-1, -1)
    property string hereLabel: qsTr("Vous êtes ici")

    /// Agrandissement : 1 couvre la zone, `maximumZoom` est la butee.
    property real zoom: 1
    readonly property real maximumZoom: 3

    /// Vrai pour garder le nom des reperes affiche hors survol.
    property bool namesAlways: false

    /// Largeur reservee a gauche par un panneau pose sur l'ecran : la carte s'ajuste a sa droite.
    property real leftInset: 0

    readonly property real coverScale: root.leftInset > 0
        ? Math.min((root.width - root.leftInset) / 1920, root.height / 1080)
        : Math.max(root.width / 1920, root.height / 1080)
    readonly property real mapWidth: 1920 * root.coverScale * root.zoom
    readonly property real mapHeight: 1080 * root.coverScale * root.zoom

    property alias flick: mapFlick

    signal markerHovered(int index, bool inside)
    signal markerActivated(int index)
    signal backRequested()
    signal positionMarked(real x, real y)

    Rectangle {
        anchors.fill: parent
        color: Tokens.panel
    }

    Flickable {
        id: mapFlick

        anchors.fill: parent
        contentWidth: root.mapWidth
        contentHeight: root.mapHeight
        leftMargin: root.leftInset
        topMargin: Math.max(0, (root.height - root.mapHeight) / 2)
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Item {
            id: mapImage

            width: root.mapWidth
            height: root.mapHeight
            clip: true

            Image {
                width: root.mapWidth / root.frame.width
                height: root.mapHeight / root.frame.height
                x: -root.frame.x * width
                y: -root.frame.y * height
                // Un nom : une carte peinte de `Assets/Maps/` ; un chemin : la carte rendue d'une
                // zone, rangee avec elle et relative a `Assets/` (LOT-121).
                source: root.image === "" ? ""
                        : "../../Elements/Assets/" + (root.image.indexOf("/") >= 0
                                                      ? root.image : "Maps/" + root.image)
                fillMode: Image.Stretch
                smooth: true
                mipmap: true
            }
        }

        MouseArea {
            anchors.fill: mapImage
            acceptedButtons: Qt.RightButton
            onClicked: root.backRequested()
        }

        // Releve d'une position pour `world-maps.json` : Ctrl+clic. Ici, DANS la `Flickable`, pas
        // sur un ancetre -- voir la note en tete de fichier.
        MouseArea {
            anchors.fill: mapImage
            acceptedButtons: Qt.LeftButton
            onClicked: (mouse) => {
                if (mouse.modifiers & Qt.ControlModifier)
                    root.positionMarked(root.frame.x + mouse.x / root.mapWidth * root.frame.width,
                                        root.frame.y + mouse.y / root.mapHeight * root.frame.height)
            }
        }

        Repeater {
            model: root.labels

            Text {
                id: tag

                required property var modelData

                x: tag.modelData.x * root.mapWidth - tag.width / 2
                y: tag.modelData.y * root.mapHeight - tag.height / 2
                text: tag.modelData.name
                color: tag.modelData.kind === "lake" || tag.modelData.kind === "sea"
                       ? Tokens.textAlly : Tokens.textOnPanel
                style: Text.Outline
                styleColor: Tokens.panel
                font.family: Tokens.loreFamily
                font.italic: true
                font.pixelSize: Tokens.fontCaption
                opacity: 0.9
            }
        }

        Repeater {
            model: root.markers

            MapMarker {
                id: marker

                required property var modelData
                required property int index

                x: marker.modelData.x * root.mapWidth - marker.markerSize / 2
                y: marker.modelData.y * root.mapHeight - marker.markerSize / 2
                z: marker.active ? 1 : 0
                label: marker.modelData.name
                kind: marker.modelData.kind
                number: marker.modelData.number
                gateway: marker.modelData.gateway
                visited: marker.modelData.visited === true
                locked: marker.modelData.locked === true
                active: root.activeIndex === marker.index
                labelAlways: root.namesAlways
                onHoveredChanged: root.markerHovered(marker.index, marker.hovered)
                onActivated: root.markerActivated(marker.index)
            }
        }

        // « Vous etes ici » (LOT-96) : un anneau d'or, plus large qu'un repere -- il entoure le
        // quartier ou se tient le heros sans cacher son numero --, et son nom en permanence.
        Item {
            id: herePin

            visible: root.here.x >= 0 && root.here.y >= 0
            x: root.here.x * root.mapWidth - width / 2
            y: root.here.y * root.mapHeight - height / 2
            z: 2
            width: 60 * Tokens.uiScale
            height: 60 * Tokens.uiScale

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "transparent"
                border.color: Tokens.goldLight
                border.width: 2 * Tokens.strokeWidth
                antialiasing: true
            }

            Text {
                anchors.top: parent.bottom
                anchors.topMargin: Tokens.gapSmall
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.hereLabel
                color: Tokens.goldLight
                style: Text.Outline
                styleColor: Tokens.panel
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontCaption
                font.weight: Font.DemiBold
            }
        }
    }
}
