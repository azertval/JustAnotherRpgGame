import QtQuick
import Jadg.Ui

/*!
    Carte -- vue QUARTIER -- FORMULAIRE, cote conception (LOT-96).

    Quatrieme niveau de l'ecran « Carte » : un quartier de la ville, et ses ilots. *Provisoire,
    decision de l'auteur du 18 septembre 2026* : le quartier n'a pas encore de carte peinte, et la
    vue AGRANDIT le plan de la ville sur son cadre (`frame`). Le jour ou l'auteur peint le quartier,
    `districtImage` le nomme et la vue le montre entier.

    Les reperes sont les ILOTS du quartier, numerotes, poses au centre de leur rectangle sur la carte
    du quartier -- tracee nord en haut, comme le plan. « Vous etes ici » marque le heros quand il
    est dans ce quartier.

    Les proprietes portent des VALEURS D'EXEMPLE ; le jumeau (`WorldMap.qml`) les remplace.
*/
Item {
    id: root

    property string cityImage: "city-central-empire-the-capital-city.jpg"
    /// La carte peinte du quartier, vide tant qu'il n'en a pas : on agrandit alors le plan.
    property string districtImage: ""
    property rect frame: Qt.rect(0.612, 0.415, 0.18, 0.18)
    property string districtName: "Martpart"
    /// La sous-zone montree (LOT-121), vide pour le quartier lui-meme : son nom suit le quartier.
    property string zoneName: ""
    property string cityName: "The Capital City"
    property string regionName: "Central Empire"

    /// Les ilots : `{ name, x, y, kind, number, gateway }`, en fractions du quartier.
    property var blocks: [
        { name: "La place du marché", x: 0.45, y: 0.49, kind: "point-of-interest", number: 1,
          gateway: true },
        { name: "L'arène Illu Die", x: 0.83, y: 0.37, kind: "point-of-interest", number: 2,
          gateway: true }
    ]
    property int selectedBlock: 0
    property string blockDescription: ""

    /// Le heros, en fractions du quartier ; `x < 0` s'il n'y est pas.
    property point here: Qt.point(0.9, 0.7)

    property alias canvas: mapCanvas
    property alias hud: mapHud
    property alias panel: sidePanel

    width: 1920
    height: 1080

    MapCanvas {
        id: mapCanvas

        anchors.fill: parent
        image: root.districtImage !== "" ? root.districtImage : root.cityImage
        frame: root.districtImage !== "" ? Qt.rect(0, 0, 1, 1) : root.frame
        markers: root.blocks
        labels: []
        activeIndex: root.selectedBlock
        here: root.here
        namesAlways: true
        leftInset: sidePanel.x + sidePanel.width
    }

    MapSidePanel {
        id: sidePanel

        x: 24 * Tokens.uiScale
        y: 172 * Tokens.uiScale
        width: 460 * Tokens.uiScale
        height: root.height - 272 * Tokens.uiScale
        title: root.zoneName !== "" ? root.zoneName : root.districtName
        lore: ""
        facts: [root.cityName]
        grades: []
        entriesTitle: qsTr("Îlots")
        entries: root.blocks
        selectedIndex: root.selectedBlock
        description: root.blockDescription
    }

    MapHud {
        id: mapHud

        anchors.fill: parent
        title: root.zoneName !== "" ? root.zoneName : root.districtName
        trail: qsTr("Tanares  ›  %1  ›  %2  ›  %3").arg(root.regionName).arg(root.cityName)
                                                     .arg(root.districtName)
               + (root.zoneName !== "" ? "  ›  " + root.zoneName : "")
        parentImage: root.cityImage
        parentFrame: root.frame
        canGoBack: true
    }
}
