import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Jadg.Ui

/*!
    Credits -- FORMULAIRE, cote conception (LOT-87, T3.3 ; maquette 07).

    Reprend les deux colonnes de la maquette `07_Credit_Mockup.png` sur la page claire du codex :
    titre grenat, panneau de parchemin, citation au pied, Retour en bas a droite et version a gauche.

    **Aucun nom propre ici.** Les attributions sont des donnees (`Source/Elements/Credits/credits.json`,
    lues par `CreditsModel`) : le jumeau pose les sections de chaque colonne dans `leftSections` et
    `rightSections`. Les valeurs ci-dessous sont celles de la conception.
*/
Item {
    id: root

    /// Sections de chaque colonne : listes de `sectionId`, `title`, `iconKey`, `lines`.
    property var leftSections: []
    property var rightSections: []

    /// La version du jeu, sans le « v », posee par le jumeau.
    property string version: "0.0.0"

    property alias backButton: backControl
    /// La barre de defilement des sections : le jumeau la fait avancer au clavier.
    property alias scrollBar: creditsScrollBar

    width: 1920
    height: 1080

    Rectangle {
        anchors.fill: parent
        color: Tokens.frameEdge
    }

    PanelFrame {
        anchors.fill: parent
        anchors.margins: 14 * Tokens.uiScale
        material: "parchment"
        bound: true
        padding: 0
    }

    // --- Panneau des credits (maquette : 385, 170 -> 1285, 825) ------------------------------------
    PanelFrame {
        id: creditsPanel

        x: 90 * Tokens.uiScale
        y: 226 * Tokens.uiScale
        width: 1740 * Tokens.uiScale
        height: 752 * Tokens.uiScale
        material: "parchment"
        subpanel: true
        padding: 0

        // Les sections defilent quand elles depassent le panneau : la barre se loge dans la marge
        // droite, et les colonnes gardent leur largeur qu'elle soit la ou non -- une largeur qui
        // suivrait la barre relancerait le retour a la ligne, donc la hauteur, donc la barre.
        Flickable {
            id: creditsScroll

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: quote.top
            anchors.leftMargin: 40 * Tokens.uiScale
            anchors.rightMargin: 14 * Tokens.uiScale
            anchors.topMargin: 80 * Tokens.uiScale
            anchors.bottomMargin: Tokens.gapMedium
            clip: true
            contentWidth: width
            contentHeight: creditsColumns.implicitHeight
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: OrnateScrollBar {
                id: creditsScrollBar
            }

            RowLayout {
                id: creditsColumns

                width: creditsScroll.width - 26 * Tokens.uiScale
                spacing: 56 * Tokens.uiScale

                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    spacing: Tokens.gapLarge

                    Repeater {
                        model: root.leftSections

                        CreditSection {
                            required property var modelData

                            Layout.fillWidth: true
                            title: modelData.title
                            iconKey: modelData.iconKey
                            lines: modelData.lines
                            material: "parchment"
                        }
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    spacing: Tokens.gapLarge

                    Repeater {
                        model: root.rightSections

                        CreditSection {
                            required property var modelData

                            Layout.fillWidth: true
                            title: modelData.title
                            iconKey: modelData.iconKey
                            lines: modelData.lines
                            material: "parchment"
                        }
                    }
                }
            }
        }

        // La citation, au pied du panneau (maquette : 660, 765 -> 990, 792).
        Text {
            id: quote

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 48 * Tokens.uiScale
            text: qsTr("« Une grande aventure ne se fait jamais seul. »")
            color: Tokens.textMuted
            font.family: Tokens.loreFamily
            font.italic: true
            font.pixelSize: Tokens.fontBody
        }
    }

    // La plaque de titre chevauche le bord haut du panneau (maquette : 620, 110 -> 1050, 205).
    TitlePlate {
        anchors.horizontalCenter: creditsPanel.horizontalCenter
        y: 16 * Tokens.uiScale
        width: 1170 * Tokens.uiScale
        material: "garnet"
        text: qsTr("Crédits")
    }

    FixedArt {
        x: 36 * Tokens.uiScale
        y: 20 * Tokens.uiScale
        width: 112 * Tokens.uiScale
        height: 180 * Tokens.uiScale
        key: "ui/ornament/crest-pennant"
    }

    // --- Retour (maquette : 40, 775 -> 300, 840) ---------------------------------------------------
    OrnateButton {
        id: backControl

        x: 1580 * Tokens.uiScale
        y: 1000 * Tokens.uiScale
        kind: "back"
        text: qsTr("Retour")
    }

    // --- Version ------------------------------------------------------------------------------------
    Text {
        x: 130 * Tokens.uiScale
        y: 1000 * Tokens.uiScale
        text: "v" + root.version
        color: Tokens.text
        font.family: Tokens.bodyFamily
        font.pixelSize: Tokens.fontBody
    }
}
