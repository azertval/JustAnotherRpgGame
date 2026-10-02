import QtQuick
import Jadg.Ui

/*!
    Carte -- vue COMPLETE, le monde -- FORMULAIRE, cote conception (LOT-94, LOT-95 ; maquette 08).

    Premier des trois niveaux de l'ecran « Carte » : le monde, puis une region (`RegionMapForm`),
    puis le plan d'une ville (`CityMapForm`). La carte est celle de l'auteur, peinte sans lettrage ;
    les treize regions de l'atlas y sont posees a leurs reperes (`world-maps.json`).

    Choisir un repere ouvre la fiche breve de sa region, a quai sous le titre ; l'activer zoome sur la
    region. On ne se DEPLACE pas sur cette carte : elle sert a s'orienter (feuille de route, §8).

    Les proprietes portent des VALEURS D'EXEMPLE ; le jumeau (`WorldMap.qml`) les remplace.
*/
Item {
    id: root

    property string worldImage: "world.jpg"

    /// Les regions : `{ name, x, y, kind, number, gateway, government, faction, places }`.
    property var regions: [
        { name: "Central Empire", x: 0.634, y: 0.42, kind: "city", number: 0, gateway: true,
          government: "The center of power of the Tanarean Empire.", faction: "Tanarean Empire",
          places: [] },
        { name: "Kingdom of Kolbjörn", x: 0.257, y: 0.236, kind: "city", number: 0, gateway: true,
          government: "Monarchy", faction: "Allied Forces", places: [] },
        { name: "Seashores", x: 0.557, y: 0.563, kind: "city", number: 0, gateway: true,
          government: "Pirate Lords of the Parliament of the Seas", faction: "Tanarean Empire",
          places: [] }
    ]

    /// La region choisie (indice dans `regions`), ou -1.
    property int selectedRegion: 0

    property alias canvas: mapCanvas
    property alias hud: mapHud

    readonly property bool hasSelection: root.selectedRegion >= 0
                                         && root.selectedRegion < root.regions.length
    readonly property var selected: root.hasSelection ? root.regions[root.selectedRegion] : ({})

    width: 1920
    height: 1080

    MapCanvas {
        id: mapCanvas

        anchors.fill: parent
        image: root.worldImage
        markers: root.regions
        activeIndex: root.selectedRegion
        namesAlways: true
    }

    // La fiche breve de la region choisie : a quai sous la plaque de titre, sur la marge de
    // parchemin de la carte -- posee pres du repere, elle recouvrait les reperes voisins.
    PanelFrame {
        id: regionCard

        visible: root.hasSelection
        x: 24 * Tokens.uiScale
        y: 172 * Tokens.uiScale
        width: 400 * Tokens.uiScale
        height: cardColumn.implicitHeight + 2 * Tokens.gapMedium
        material: "parchment"
        subpanel: true

        Column {
            id: cardColumn

            width: parent.width
            spacing: Tokens.gapSmall

            Text {
                width: parent.width
                text: root.hasSelection ? root.selected.name : ""
                color: Tokens.gem
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontSectionTitle
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }

            Text {
                width: parent.width
                text: root.hasSelection ? root.selected.government : ""
                color: Tokens.text
                font.family: Tokens.loreFamily
                font.italic: true
                font.pixelSize: Tokens.fontCaption
                wrapMode: Text.WordWrap
                maximumLineCount: 4
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: root.hasSelection ? root.selected.faction : ""
                color: Tokens.textMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }

            GoldDivider {
                width: parent.width
            }

            Text {
                width: parent.width
                text: root.hasSelection ? qsTr("Lieux : %1").arg(root.selected.places.length) : ""
                color: Tokens.text
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontCaption
            }

            Text {
                width: parent.width
                text: qsTr("Entrée ou clic : ouvrir la région")
                color: Tokens.textMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
            }
        }
    }

    MapHud {
        id: mapHud

        anchors.fill: parent
        title: qsTr("Tanares")
        trail: qsTr("Le monde connu")
        canGoBack: false
    }
}
