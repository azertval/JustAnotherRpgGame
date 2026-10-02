import QtQuick
import QtQuick.Layouts
import Jadg.Ui

/*!
    Une section de credits : icone cerclee, titre, filet d'or, puis ses lignes role / noms
    (LOT-87, T3.3 ; maquette 07).

    Les donnees viennent d'un modele (`lines`, une liste de `role` et `names`, un nom par ligne) ;
    la brique ne dessine que leur mise en page. L'icone est la piece `iconKey`
    (`ui/icon/credits-section/<id>`) posee dans son anneau `ui/medallion/icon-ring` ; tant qu'aucune
    n'est livree, un anneau d'aplat tient la place.
*/
ColumnLayout {
    id: root

    property string title: "Section"
    property string iconKey: ""
    property var lines: []
    property string material: "dark"

    spacing: Tokens.gapSmall

    RowLayout {
        Layout.fillWidth: true
        spacing: Tokens.gapMedium

        Item {
            Layout.preferredWidth: 48 * Tokens.uiScale
            Layout.preferredHeight: 48 * Tokens.uiScale

            Rectangle {
                anchors.fill: parent
                visible: !ringArt.delivered
                radius: width / 2
                color: Tokens.panelRaised
                border.color: Tokens.panelEdge
                border.width: Tokens.strokeWidth
            }

            FixedArt {
                id: ringArt

                anchors.fill: parent
                key: "ui/medallion/icon-ring"
            }

            FixedArt {
                anchors.fill: parent
                anchors.margins: 8 * Tokens.uiScale
                key: root.iconKey
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.title
            color: root.material === "parchment" ? Tokens.gem : Tokens.goldLight
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontSectionTitle
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
    }

    GoldDivider {
        Layout.fillWidth: true
    }

    Repeater {
        model: root.lines

        RowLayout {
            id: creditLine

            required property var modelData

            Layout.fillWidth: true
            Layout.topMargin: Tokens.gapSmall
            spacing: Tokens.gapMedium

            Text {
                // Parts proportionnelles et non « parent.width » : un texte a retour a la ligne dont la
                // largeur suit sa rangee relance la mise en page de la rangee, qui boucle.
                Layout.fillWidth: true
                Layout.preferredWidth: 42
                Layout.alignment: Qt.AlignTop
                text: creditLine.modelData.role
                color: root.material === "parchment" ? Tokens.text : Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 58
                Layout.alignment: Qt.AlignTop
                text: creditLine.modelData.names
                color: root.material === "parchment" ? Tokens.text : Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                wrapMode: Text.WordWrap
            }
        }
    }
}
