import QtQuick
import Jadg.Ui

/*!
    Chassis d'un ecran de la charte v2 sans maquette propre : fond, grand panneau, plaque de titre et
    mention des donnees en attente (LOT-87, T3.9).

    Remplace le cadre Qt Widgets de la v1 (`RpgScreenFrame`, retire) pour le journal, le dialogue
    et le marchand. La matiere suit la charte : `dark` (panneau sombre, plaque grenat) pour ce qui
    se superpose au jeu, `parchment` (parchemin relie, plaque grenat) pour les documents. Le contenu
    se pose dedans comme dans un `Item`, sous la plaque de titre.

    `pending` affiche, au pied, que l'ecran est dessine sans etre alimente : un ecran pas encore
    branche ressemble sinon a un ecran casse (`scripts/i18n/list_pending_bindings.py` en fait l'inventaire).
*/
Item {
    id: root

    property string title: "Titre"

    /// `dark` ou `parchment`.
    property string material: "parchment"

    /// Vrai tant qu'aucune donnee reelle n'alimente l'ecran.
    property bool pending: false

    default property alias content: contentArea.data

    readonly property bool dark: root.material !== "parchment"

    width: 1920
    height: 1080

    Rectangle {
        anchors.fill: parent
        color: root.dark ? Tokens.panel : Tokens.frameEdge
    }

    PanelFrame {
        id: frame

        anchors.fill: parent
        anchors.margins: 16 * Tokens.uiScale
        material: root.material
        bound: !root.dark
        padding: 0
    }

    FixedArt {
        x: 36 * Tokens.uiScale
        y: 20 * Tokens.uiScale
        width: 112 * Tokens.uiScale
        height: 180 * Tokens.uiScale
        key: "ui/ornament/crest-pennant"
        visible: !root.dark
    }

    TitlePlate {
        id: plate

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 16 * Tokens.uiScale
        width: root.dark ? implicitWidth : 1170 * Tokens.uiScale
        material: "garnet"
        text: root.title
    }

    Item {
        id: contentArea

        anchors.left: frame.left
        anchors.right: frame.right
        anchors.top: plate.bottom
        anchors.bottom: pendingNote.top
        anchors.leftMargin: 48 * Tokens.uiScale
        anchors.rightMargin: 48 * Tokens.uiScale
        anchors.topMargin: Tokens.gapLarge
        anchors.bottomMargin: root.dark ? Tokens.gapMedium : 2 * Tokens.gapLarge
    }

    Text {
        id: pendingNote

        anchors.right: frame.right
        anchors.bottom: frame.bottom
        anchors.rightMargin: 48 * Tokens.uiScale
        anchors.bottomMargin: Tokens.gapLarge
        height: root.pending ? implicitHeight : 0
        visible: root.pending
        text: qsTr("Écran dessiné, données à venir")
        color: root.dark ? Tokens.textOnPanelMuted : Tokens.textMuted
        font.family: Tokens.loreFamily
        font.italic: true
        font.pixelSize: Tokens.fontCaption
    }
}
