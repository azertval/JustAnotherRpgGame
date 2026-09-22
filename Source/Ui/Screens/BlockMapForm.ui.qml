import QtQuick
import Jadg.Ui

/*!
    Carte -- vue ILOT -- FORMULAIRE, cote conception (LOT-96).

    Cinquieme et dernier niveau de l'ecran « Carte » : un ilot d'un quartier. *Decision de l'auteur
    du 18 septembre 2026* : l'ilot n'a pas d'image a lui. `blockImage` est la carte du quartier
    telle que le jeu la dessine, cadree sur l'ilot -- les memes planches, les memes figurines, le
    heros a sa case s'il y est. Le jumeau la demande au fournisseur `image://cityblock/`.

    Le plan sert a s'orienter : on ne s'y deplace pas.

    Les proprietes portent des VALEURS D'EXEMPLE ; le jumeau (`WorldMap.qml`) les remplace.
*/
Item {
    id: root

    property url blockImage: ""
    property string blockName: "La place du marché"
    property string districtName: "Martpart"
    property string cityName: "The Capital City"
    property string regionName: "Central Empire"
    property string cityImage: "city-central-empire-the-capital-city.jpg"
    property rect districtFrame: Qt.rect(0.612, 0.415, 0.18, 0.18)

    /// Vrai quand le heros est dans cet ilot.
    property bool heroHere: true
    /// Vrai tant que l'image de l'ilot se dessine.
    property bool loading: blockView.status === Image.Loading

    property alias hud: mapHud
    property alias panel: sidePanel
    property alias view: blockView

    width: 1920
    height: 1080

    Rectangle {
        anchors.fill: parent
        color: Tokens.panel
    }

    Image {
        id: blockView

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: sidePanel.right
        anchors.margins: 96 * Tokens.uiScale
        source: root.blockImage
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        // L'îlot est peint en 2D HD (LOT-103) : réduit à la taille du panneau, il se lisse.
        smooth: true
        mipmap: true
    }

    Text {
        anchors.centerIn: blockView
        visible: root.loading
        text: qsTr("L'îlot se dessine…")
        color: Tokens.textOnPanel
        font.family: Tokens.loreFamily
        font.italic: true
        font.pixelSize: Tokens.fontCaption
    }

    MapSidePanel {
        id: sidePanel

        x: 24 * Tokens.uiScale
        y: 172 * Tokens.uiScale
        width: 460 * Tokens.uiScale
        height: root.height - 272 * Tokens.uiScale
        title: root.blockName
        lore: ""
        facts: [root.districtName, root.cityName]
        grades: []
        entriesTitle: ""
        entries: []
        selectedIndex: -1
        description: root.heroHere ? qsTr("Vous êtes ici.") : ""
    }

    MapHud {
        id: mapHud

        anchors.fill: parent
        title: root.blockName
        trail: qsTr("Tanares  ›  %1  ›  %2  ›  %3  ›  %4").arg(root.regionName).arg(root.cityName)
                                                           .arg(root.districtName).arg(root.blockName)
        parentImage: root.cityImage
        parentFrame: root.districtFrame
        canGoBack: true
    }
}
