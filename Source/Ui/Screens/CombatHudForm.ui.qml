pragma ComponentBehavior: Bound
import QtQuick
import Jadg.Ui

/*!
    HUD de combat -- FORMULAIRE, cote conception (LOT-86, LOT-87 T4.1 ; maquette 01).

    Le seul ecran qui ne s'ouvre PAS par-dessus le jeu : il EST le jeu pendant un combat. Il ne
    suspend donc rien, et le tour par tour decidera de son rythme. Le cadre commun a la vue de jeu
    vient de `HudFrame`, en mode `combat` ; ce formulaire y ajoute ce qui n'appartient qu'au combat :
    le journal, l'ordre d'initiative, la roue et la barre d'actions, la fiche de la cible.

    Chaque action porte son RACCOURCI, ecrit sous sa case : un combat doit se jouer entierement au
    clavier et entierement a la manette, le critere que la feuille de route dit « souvent oublie ».

    Les listes ont les roles de `SheetRowModel` (`rowId`, `label`, `value`) :
    - `initiative` : `label` le combattant, `value` son camp (`ally`, `enemy`) ;
    - `combatLog` : `label` la ligne, `value` le tour qu'elle ouvre (`ally`, `enemy`) ou vide ;
    - `actions` : `label` l'action, `value` ses charges restantes ou vide.

    Le combat sur la carte (LOT-118) y ajoute le CALQUE TACTIQUE par-dessus la surface -- cases
    atteignables, curseur, chemin, combattants (`TacticalLayer`, le meme qu'au Colisee) --, le
    statut du dernier geste, et le panneau de l'ISSUE : victoire, fuite ou mort, avec le geste
    qui rend l'exploration. La grille tactique est une zone de la carte : `zoneColumn` et
    `zoneRow` ramenent ses cases sur la carte.

    Les proprietes portent des VALEURS D'EXEMPLE ; le jumeau les remplace.
*/
HudFrame {
    id: root

    // --- Le calque tactique (LOT-118) -----------------------------------------------------------
    property var fighters: []
    property var reachableCells: []
    property var pathCells: []
    property int cursorColumn: 0
    property int cursorRow: 0
    property bool ended: false
    property int zoneColumn: 0
    property int zoneRow: 0
    property real gridTileWidth: 64
    property real gridTileHeight: 40
    property real gridOriginX: 0
    property real gridOriginY: 0
    /// Ce que le dernier geste a donne : un jet, un refus.
    property string status: ""
    /// Vrai pendant qu'un mouvement se joue : les gestes attendent.
    property bool busy: false
    /// L'issue : vide tant que le combat dure ; `victory`, `flight`, `defeat`.
    property string outcome: ""

    signal gridHovered(real x, real y)
    signal gridClicked(real x, real y)
    /// Le joueur quitte le combat fini : retour a l'exploration, ou fin de la demo.
    signal leaveRequested()
    /// Le joueur rend la main (le geste « Espace » / « Y », a la souris).
    signal endTurnRequested()

    property var initiative: exampleInitiative
    /// Le combattant dont c'est le tour (indice dans `initiative`).
    property int activeIndex: 0

    property var combatLog: exampleLog

    property var actions: exampleActions
    /// L'action choisie (indice dans `actions`).
    property int activeAction: 0
    property string activeActionLabel: "Arc long"

    property string targetName: "Bandit"
    property string targetLevel: "3"
    property string targetHitPoints: "18 / 32"
    property real targetHitPointsRatio: 18 / 32
    property string targetArmorClass: "13"
    property string targetInitiative: "+2"
    property string targetSpeed: "9 m"
    property string targetConditions: "À terre"
    property url targetPortrait: ""

    readonly property ListModel exampleInitiative: ListModel {
        ListElement { rowId: "brenna"; label: "Brenna"; value: "ally" }
        ListElement { rowId: "bandit-1"; label: "Bandit"; value: "enemy" }
        ListElement { rowId: "sarre"; label: "Sarre"; value: "ally" }
        ListElement { rowId: "bandit-2"; label: "Bandit"; value: "enemy" }
        ListElement { rowId: "ourse"; label: "Ourse"; value: "ally" }
    }

    readonly property ListModel exampleLog: ListModel {
        ListElement { rowId: "1"; label: "Tour de Brenna"; value: "ally" }
        ListElement { rowId: "2"; label: "Brenna se déplace de 4 cases."; value: "" }
        ListElement { rowId: "3"; label: "Brenna tire sur Bandit : 12 dégâts."; value: "" }
        ListElement { rowId: "4"; label: "Bandit est à terre."; value: "" }
        ListElement { rowId: "5"; label: "Tour de l'ennemi"; value: "enemy" }
    }

    readonly property ListModel exampleActions: ListModel {
        ListElement { rowId: "arc"; label: "Arc long"; value: "" }
        ListElement { rowId: "trait"; label: "Trait de givre"; value: "3" }
        ListElement { rowId: "cacher"; label: "Se cacher"; value: "" }
        ListElement { rowId: "potion"; label: "Potion"; value: "3" }
        ListElement { rowId: "parade"; label: "Parade"; value: "" }
        ListElement { rowId: "feu"; label: "Flèche de feu"; value: "3" }
        ListElement { rowId: "piege"; label: "Piège"; value: "1" }
        ListElement { rowId: "passer"; label: "Passer"; value: "" }
    }

    mode: "combat"

    // --- Le calque tactique, par-dessus la surface (LOT-118) ---------------------------------------------
    // Le contenu d'un HudFrame remplit le cadre, comme l'hote de la surface : meme rectangle.
    TacticalLayer {
        anchors.fill: parent
        visible: root.fighters.length > 0
        fighters: root.fighters
        reachableCells: root.reachableCells
        pathCells: root.pathCells
        cursorColumn: root.cursorColumn
        cursorRow: root.cursorRow
        ended: root.ended
        zoneColumn: root.zoneColumn
        zoneRow: root.zoneRow
        gridTileWidth: root.gridTileWidth
        gridTileHeight: root.gridTileHeight
        gridOriginX: root.gridOriginX
        gridOriginY: root.gridOriginY
        onGridHovered: (x, y) => root.gridHovered(x, y)
        onGridClicked: (x, y) => root.gridClicked(x, y)
    }

    // --- Le statut du dernier geste, sous l'ordre d'initiative ------------------------------------------
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 172 * Tokens.uiScale
        width: 900 * Tokens.uiScale
        visible: text.length > 0
        text: root.busy ? "" : root.status
        color: Tokens.goldLight
        font.family: Tokens.bodyFamily
        font.pixelSize: Tokens.fontBody
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideMiddle
        style: Text.Outline
        styleColor: Tokens.panel
    }

    // --- L'issue (LOT-118) : ce qui reste a l'ecran quand le combat est fini -----------------------------
    PanelFrame {
        anchors.centerIn: parent
        width: 520 * Tokens.uiScale
        height: 200 * Tokens.uiScale
        visible: root.outcome.length > 0

        Column {
            anchors.centerIn: parent
            spacing: Tokens.gapMedium

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.outcome === "victory" ? qsTr("Victoire")
                    : root.outcome === "flight" ? qsTr("Vous avez pris la fuite")
                    : qsTr("Vous êtes mort")
                color: Tokens.goldLight
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontSectionTitle
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 460 * Tokens.uiScale
                text: root.status
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            OrnateButton {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.outcome === "defeat" ? qsTr("Fin de la démo") : qsTr("Reprendre l'exploration")
                onClicked: root.leaveRequested()
            }
        }
    }

    // --- Ordre d'initiative, sous la boussole ------------------------------------------------------------
    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 128 * Tokens.uiScale
        spacing: Tokens.gapSmall

        Repeater {
            model: root.initiative

            Rectangle {
                id: combatant

                required property int index
                required property string label
                required property string value

                readonly property bool current: combatant.index === root.activeIndex

                width: chipRow.implicitWidth + 2 * Tokens.gapMedium
                height: 36 * Tokens.uiScale
                color: combatant.current ? Tokens.panelRaised : Tokens.panel
                border.color: combatant.current ? Tokens.goldLight : Tokens.panelEdge
                border.width: (combatant.current ? 2 : 1) * Tokens.strokeWidth

                Row {
                    id: chipRow

                    anchors.centerIn: parent
                    spacing: Tokens.gapSmall

                    // Le camp se lit a la marque autant qu'a la teinte : losange pour un allie,
                    // rond pour un ennemi.
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 10 * Tokens.uiScale
                        height: 10 * Tokens.uiScale
                        rotation: combatant.value === "enemy" ? 0 : 45
                        radius: combatant.value === "enemy" ? width / 2 : 0
                        color: combatant.value === "enemy" ? Tokens.textEnemy
                               : (combatant.value === "ally" ? Tokens.textAlly : Tokens.textOnPanelMuted)
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: combatant.label
                        color: combatant.current ? Tokens.goldLight : Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                    }
                }
            }
        }
    }

    // --- Journal de combat (maquette : 20, 678 -> 400, 835) ------------------------------------------------
    PanelFrame {
        x: 24 * Tokens.uiScale
        y: 776 * Tokens.uiScale
        width: 440 * Tokens.uiScale
        height: 188 * Tokens.uiScale
        subpanel: true

        Column {
            anchors.fill: parent
            spacing: 2 * Tokens.uiScale
            clip: true

            Repeater {
                model: root.combatLog

                Item {
                    id: entry

                    required property string label
                    required property string value

                    readonly property bool turn: entry.value === "ally" || entry.value === "enemy"

                    width: parent.width
                    height: 28 * Tokens.uiScale

                    Rectangle {
                        id: entryMark

                        anchors.left: parent.left
                        anchors.leftMargin: 4 * Tokens.uiScale
                        anchors.verticalCenter: parent.verticalCenter
                        width: (entry.turn ? 14 : 8) * Tokens.uiScale
                        height: width
                        rotation: entry.value === "ally" ? 45 : 0
                        radius: entry.value === "ally" ? 0 : width / 2
                        color: entry.value === "ally" ? Tokens.textAlly
                               : (entry.value === "enemy" ? Tokens.textEnemy : "transparent")
                        border.color: entry.turn ? Tokens.panel : Tokens.panelEdge
                        border.width: Tokens.strokeWidth
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 32 * Tokens.uiScale
                        anchors.verticalCenter: parent.verticalCenter
                        text: entry.label
                        color: entry.value === "ally" ? Tokens.textAlly
                               : (entry.value === "enemy" ? Tokens.textEnemy : Tokens.textOnPanel)
                        font.family: entry.turn ? Tokens.titleFamily : Tokens.bodyFamily
                        font.pixelSize: Tokens.fontBody
                        font.weight: entry.turn ? Font.DemiBold : Font.Normal
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    // --- Roue d'action (maquette : 425, 758 -> 562, 892) ---------------------------------------------------
    Item {
        x: 476 * Tokens.uiScale
        y: 872 * Tokens.uiScale
        width: 156 * Tokens.uiScale
        height: 156 * Tokens.uiScale

        Rectangle {
            anchors.fill: parent
            visible: !wheelArt.delivered
            radius: width / 2
            color: Tokens.gem
            border.color: Tokens.panelEdge
            border.width: 3 * Tokens.strokeWidth
        }

        FixedArt {
            id: wheelArt

            anchors.fill: parent
            key: "ui/medallion/action-wheel"
        }

        Text {
            anchors.centerIn: parent
            width: parent.width * 0.64
            text: root.activeActionLabel
            color: Tokens.textOnPanel
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontBody
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
    }

    // --- Barre d'actions (maquette : 555, 778 -> 1090, 880) ------------------------------------------------
    PanelFrame {
        x: 640 * Tokens.uiScale
        y: 884 * Tokens.uiScale
        width: 612 * Tokens.uiScale
        height: 136 * Tokens.uiScale
        subpanel: true

        Row {
            anchors.centerIn: parent
            spacing: Tokens.gapSmall

            Repeater {
                model: root.actions

                ActionSlot {
                    id: actionCell

                    required property int index
                    required property string value
                    required label

                    quantity: actionCell.value
                    shortcut: "" + (actionCell.index + 1)
                    active: actionCell.index === root.activeAction
                }
            }
        }
    }

    // --- Fin du tour, sous la fiche de la cible -----------------------------------------------------------
    // Le seul geste du tour qui n'avait ni case ni bouton : sans lui, la souris ne rendait jamais la main.
    OrnateButton {
        x: 1256 * Tokens.uiScale
        y: 968 * Tokens.uiScale
        text: qsTr("Fin du tour")
        enabled: !root.ended && !root.busy && root.outcome.length === 0
        onClicked: root.endTurnRequested()
    }

    // --- Fiche de la cible (maquette : 1100, 678 -> 1390, 828) ----------------------------------------------
    PanelFrame {
        x: 1256 * Tokens.uiScale
        y: 756 * Tokens.uiScale
        width: 332 * Tokens.uiScale
        height: 204 * Tokens.uiScale
        subpanel: true

        Text {
            id: targetTitle

            anchors.left: parent.left
            anchors.right: targetLevelLabel.left
            anchors.top: parent.top
            anchors.rightMargin: Tokens.gapSmall
            text: root.targetName
            color: Tokens.textOnPanel
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontBody
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Text {
            id: targetLevelLabel

            anchors.right: parent.right
            anchors.baseline: targetTitle.baseline
            text: qsTr("Niv. %1").arg(root.targetLevel)
            color: Tokens.textOnPanel
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontBody
        }

        PortraitFrame {
            id: targetPortraitFrame

            anchors.left: parent.left
            anchors.top: targetTitle.bottom
            anchors.topMargin: Tokens.gapSmall
            shape: "square"
            size: 96 * Tokens.uiScale
            source: root.targetPortrait
        }

        Column {
            anchors.left: targetPortraitFrame.right
            anchors.right: parent.right
            anchors.top: targetPortraitFrame.top
            anchors.leftMargin: Tokens.gapMedium
            spacing: 2 * Tokens.uiScale

            Gauge {
                width: parent.width
                height: 26 * Tokens.uiScale
                kind: "health"
                value: root.targetHitPointsRatio
                label: root.targetHitPoints
            }

            Item { width: 1; height: 4 * Tokens.uiScale }

            Repeater {
                model: [
                    { label: qsTr("CA"), value: root.targetArmorClass },
                    { label: qsTr("Initiative"), value: root.targetInitiative },
                    { label: qsTr("Vitesse"), value: root.targetSpeed },
                    { label: qsTr("États"), value: root.targetConditions }
                ]

                Item {
                    id: stat

                    required property var modelData

                    width: parent.width
                    height: 24 * Tokens.uiScale

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: stat.modelData.label
                        color: Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: stat.modelData.value
                        color: Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                    }
                }
            }
        }
    }
}
