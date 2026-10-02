import QtQuick
import QtQuick.Layouts
import Jadg.Ui

/*!
    Journal de quetes -- FORMULAIRE, cote conception (LOT-86, restyle LOT-87 T3.9).

    Pas de maquette : les briques et les jetons de la charte v2, sur la structure v1 -- les quetes a
    gauche, le detail et les objectifs de la quete choisie a droite. Panneau sombre : le journal se
    consulte par-dessus le jeu, comme la carte.

    Les proprietes portent des VALEURS D'EXEMPLE : sans elles, l'atelier montrerait un ecran vide et
    la mise en page ne se jugerait pas. Le jumeau les remplace par l'ancre `PendingData`, en
    attendant le lot des quetes (`LOT-16`).
*/
ScreenPage {
    id: root

    property var quests: exampleQuests
    property var objectives: exampleObjectives
    property string detail: "Le tavernier jure que les tonneaux descendent seuls à la cave, et qu'aucun ne remonte. Il propose trois pièces d'or et le gîte pour la nuit à qui voudra bien y descendre voir."

    readonly property ListModel exampleQuests: ListModel {
        ListElement { rowId: "a"; label: "Les caves de Bourg-la-Rive"; value: "En cours" }
        ListElement { rowId: "b"; label: "La cargaison disparue"; value: "En cours" }
        ListElement { rowId: "c"; label: "Le convoi de Val-Morne"; value: "Terminée" }
    }
    readonly property ListModel exampleObjectives: ListModel {
        ListElement { rowId: "a"; label: "Descendre à la cave"; value: "✓" }
        ListElement { rowId: "b"; label: "Trouver l'origine du bruit"; value: "" }
    }

    title: qsTr("Journal de quêtes")
    material: "parchment"

    RowLayout {
        anchors.fill: parent
        spacing: Tokens.gapLarge

        LedgerList {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            material: "parchment"
            title: qsTr("Quêtes")
            rows: root.quests
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            spacing: Tokens.gapLarge

            PanelFrame {
                Layout.fillWidth: true
                Layout.preferredHeight: 360 * Tokens.uiScale
                material: "parchment"
                subpanel: true

                SectionBanner {
                    id: detailBanner

                    anchors.left: parent.left
                    anchors.right: parent.right
                    material: "parchment"
                    text: qsTr("Détail")
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: detailBanner.bottom
                    anchors.bottom: parent.bottom
                    anchors.topMargin: Tokens.gapMedium
                    text: root.detail
                    color: Tokens.text
                    font.family: Tokens.loreFamily
                    font.italic: true
                    font.pixelSize: Tokens.fontBody
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                }
            }

            LedgerList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                material: "parchment"
                title: qsTr("Objectifs")
                rows: root.objectives
            }
        }
    }
}
