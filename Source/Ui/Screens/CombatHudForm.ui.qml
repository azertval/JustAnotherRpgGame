pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import Jadg.Ui

/*!
    HUD de combat -- FORMULAIRE, cote conception (LOT-86, LOT-87 T4.1 ; maquette 01 ; LOT-140).

    Le seul ecran qui ne s'ouvre PAS par-dessus le jeu : il EST le jeu pendant un combat. Il ne
    suspend donc rien, et le tour par tour decidera de son rythme. Le cadre commun a la vue de jeu
    vient de `HudFrame`, en mode `combat` ; ce formulaire y ajoute ce qui n'appartient qu'au combat :
    le journal, l'ordre d'initiative, la roue et la barre d'actions, la fiche de la cible.

    Chaque action porte son RACCOURCI, ecrit sous sa case : un combat doit se jouer entierement au
    clavier et entierement a la manette, le critere que la feuille de route dit « souvent oublie ».

    LE GROUPE (LOT-140, maquette `combat-de-groupe.svg`). L'ordre d'initiative se lit AUX JETONS :
    le jeton rond d'un membre du groupe, deux lettres pour une creature, l'actif cercle d'or. Le
    PANNEAU DU TOUR, sous le nom du personnage actif, dit son niveau, sa CA, ses etats, ce qu'il
    lui reste a depenser (action, action bonus, deplacement) et ses capacites de classe ; la
    jauge d'experience du cadre lui laisse la place. La PREVISUALISATION prend la place des quetes :
    l'action choisie sur la case du curseur -- le jet, la CA vue, la chance, les des, LA CAPACITE
    QUI JOUE ou pourquoi elle ne joue pas, l'esperance. La barre d'actions montre HUIT cases, une
    fenetre glissante autour de l'action choisie, chaque sort avec son icone et ses lancers.

    Les listes ont les roles de `SheetRowModel` (`rowId`, `label`, `value`), et ce que le combat
    y ajoute :
    - `initiative` : `label` le combattant, `value` son camp (`ally`, `enemy`), `token` l'URL de
      son jeton ou vide, `initials` deux lettres, `down` s'il est a terre ;
    - `combatLog` : `label` la ligne, `value` le tour qu'elle ouvre (`ally`, `enemy`) ou vide ;
    - `actions` : `label` l'action, `value` ses charges restantes ou vide, `iconKey` son icone ;
    - `activeCapacities` : `label` la capacite, `value` la cle de son icone ;
    - `previewLines` : `label` la rubrique, `value` sa valeur ;
    - `previewCapacities` : `label` la capacite, `value` ses des, `applies`, `reason`.

    Le combat sur la carte (LOT-118) y ajoute le CALQUE TACTIQUE par-dessus la surface -- cases
    atteignables, curseur, chemin, combattants (`TacticalLayer`, le meme qu'au Colisee) --, le
    statut du dernier geste, et le panneau de l'ISSUE : victoire, fuite ou mort, avec le geste
    qui rend l'exploration. La grille tactique est une zone de la carte : `zoneColumn` et
    `zoneRow` ramenent ses cases sur la carte.

    Les proprietes portent des VALEURS D'EXEMPLE ; le jumeau les remplace.
*/
HudFrame {
    id: root
    property bool playerTurn: true
    property int actionFilter: 0
    property bool actionPaging: false
    property bool detailsExpanded: false
    property bool logExpanded: false
    property real pointerX: width * 0.6
    property real pointerY: height * 0.4
    signal filterRequested(int filter)
    signal actionPageRequested(int direction)
    signal detailsToggleRequested()
    signal logToggleRequested()

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
    /// Le joueur clique une case de la barre d'actions (indice dans `actions`).
    signal actionClicked(int index)

    // --- L'ordre d'initiative ------------------------------------------------------------------
    property var initiative: exampleInitiative
    /// Le combattant dont c'est le tour (indice dans `initiative`).
    property int activeIndex: 0
    property string round: "2"

    // --- Le tour du personnage actif (LOT-140) --------------------------------------------------
    property string activeLevel: "1"
    property string activeArmorClass: "14"
    /// Les etats, deja joints (« Ensanglante, Beni »), ou vide.
    property string activeConditions: ""
    property int activeActions: 1
    property int activeActionsMax: 1
    property int activeBonusActions: 1
    property int activeBonusActionsMax: 1
    property string activeMovement: "6 / 6"
    property var activeCapacities: exampleCapacities

    property var combatLog: exampleLog

    property var actions: exampleActions
    /// L'action choisie (indice dans `actions`).
    property int activeAction: 0
    property string activeActionLabel: "Rapière"
    property string activeActionDetail: "+5 · 1d8+3 perforant"

    // --- La previsualisation (LOT-140) ----------------------------------------------------------
    property string previewTitle: "Rapière › Bandit"
    property var previewLines: examplePreview
    property var previewCapacities: examplePreviewCapacities
    /// L'esperance de degats (« 6,1 »), ou vide.
    property string previewExpected: "6,1"
    /// Faux quand l'action ne peut pas se jouer sur cette case : la carte se grise.
    property bool previewValid: true

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
        ListElement { rowId: "nessa"; label: "Nessa"; value: "ally"; token: ""; initials: "Ne"; down: false }
        ListElement { rowId: "bandit-1"; label: "Bandit"; value: "enemy"; token: ""; initials: "B1"; down: false }
        ListElement { rowId: "grom"; label: "Grom"; value: "ally"; token: ""; initials: "Gr"; down: false }
        ListElement { rowId: "bandit-2"; label: "Bandit"; value: "enemy"; token: ""; initials: "B2"; down: true }
        ListElement { rowId: "helga"; label: "Helga"; value: "ally"; token: ""; initials: "He"; down: false }
        ListElement { rowId: "faelar"; label: "Faelar"; value: "ally"; token: ""; initials: "Fa"; down: false }
        ListElement { rowId: "archer"; label: "Archer"; value: "enemy"; token: ""; initials: "Ar"; down: false }
    }

    readonly property ListModel exampleCapacities: ListModel {
        ListElement { rowId: "sneak"; label: "Sneak Attack Simplified"; value: "ui/icon/capacity/sneak-attack-simplified" }
        ListElement { rowId: "agility"; label: "Scoundrel's Agility"; value: "ui/icon/capacity/scoundrels-agility" }
    }

    readonly property ListModel exampleLog: ListModel {
        ListElement { rowId: "1"; label: "Tour de Nessa"; value: "ally" }
        ListElement { rowId: "2"; label: "Nessa se déplace de 4 cases."; value: "" }
        ListElement { rowId: "3"; label: "Nessa frappe Bandit : 12 dégâts."; value: "" }
        ListElement { rowId: "4"; label: "Bandit est à terre."; value: "" }
        ListElement { rowId: "5"; label: "Tour de l'ennemi"; value: "enemy" }
    }

    readonly property ListModel exampleActions: ListModel {
        ListElement { rowId: "rapiere"; label: "Rapière"; value: ""; iconKey: "ui/icon/action/melee" }
        ListElement { rowId: "arc"; label: "Arc court"; value: ""; iconKey: "ui/icon/action/ranged" }
        ListElement { rowId: "dague"; label: "Dague"; value: ""; iconKey: "ui/icon/action/melee" }
        ListElement { rowId: "trait"; label: "Trait de feu"; value: ""; iconKey: "ui/icon/spell/fire-bolt" }
        ListElement { rowId: "projectile"; label: "Projectile magique"; value: "2"; iconKey: "ui/icon/spell/magic-missile" }
        ListElement { rowId: "esquiver"; label: "Esquiver"; value: ""; iconKey: "ui/icon/action/dodge" }
        ListElement { rowId: "desengager"; label: "Se désengager"; value: ""; iconKey: "ui/icon/action/disengage" }
        ListElement { rowId: "precipiter"; label: "Se précipiter"; value: ""; iconKey: "ui/icon/action/dash" }
    }

    readonly property ListModel examplePreview: ListModel {
        ListElement { rowId: "toucher"; label: "Toucher"; value: "d20 +5 contre CA 15 · 55 %" }
        ListElement { rowId: "degats"; label: "Dégâts"; value: "1d8+3 perforant" }
    }

    readonly property ListModel examplePreviewCapacities: ListModel {
        ListElement { rowId: "sneak"; label: "Sneak Attack Simplified"; value: "1d8"; applies: true; reason: "" }
    }

    mode: "combat"
    showExperience: false
    showQuests: false

    // --- Le calque tactique, par-dessus la surface (LOT-118) ---------------------------------------------
    // Le contenu d'un HudFrame remplit le cadre, comme l'hote de la surface : meme rectangle.
    TacticalLayer {
        id: tacticalLayer

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
    }

    Connections {
        target: tacticalLayer

        function onGridHovered(x, y) {
            root.gridHovered(x, y)
        }

        function onGridClicked(x, y) {
            root.gridClicked(x, y)
        }
    }


    // L'initiative reste consultable, même avec un grand nombre de combattants.
    PanelFrame {
        id: initiativeDock
        objectName: "combatInitiative"
        z: 5
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Tokens.gapSmall
        width: 700 * Tokens.uiScale
        height: 140 * Tokens.uiScale
        subpanel: true
        scale: Tokens.hudScale
        transformOrigin: Item.Top
        Text {
            id: roundLabel
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Round %1").arg(root.round)
            color: Tokens.goldLight
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontBody
        }
        ListView {
            anchors.top: roundLabel.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            orientation: ListView.Horizontal
            clip: true
            spacing: Tokens.gapSmall
            model: root.initiative
            currentIndex: root.activeIndex
            highlightRangeMode: ListView.ApplyRange
            preferredHighlightBegin: width / 3
            preferredHighlightEnd: width * 2 / 3
            delegate: Item {
                id: fighter
                required property int index
                required property string label
                required property string value
                required property string token
                required property string initials
                required property bool down
                width: 80 * Tokens.uiScale
                height: 86 * Tokens.uiScale
                opacity: fighter.down ? 0.5 : 1
                PortraitFrame {
                    anchors.horizontalCenter: parent.horizontalCenter
                    size: 76 * Tokens.uiScale
                    source: fighter.token
                }
                Text {
                    anchors.centerIn: parent
                    visible: fighter.token.length === 0
                    text: fighter.initials
                    color: Tokens.textOnPanel
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontBody
                }
                Text {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: (fighter.index === root.activeIndex ? "◆ " : fighter.value === "enemy" ? "● " : "◇ ") + fighter.label
                    elide: Text.ElideRight
                    color: fighter.index === root.activeIndex ? Tokens.goldLight : Tokens.textOnPanel
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontCaption
                }
            }
        }
    }

    PanelFrame {
        id: actionDock
        objectName: "combatActionDock"
        z: 5
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.gapMedium
        width: 1320 * Tokens.uiScale
        height: 230 * Tokens.uiScale
        subpanel: true
        padding: Tokens.gapMedium
        scale: Tokens.hudScale
        transformOrigin: Item.Bottom
        MouseArea { anchors.fill: parent }
        PortraitFrame {
            id: activePortrait
            anchors.left: parent.left
            anchors.leftMargin: 14 * Tokens.uiScale
            anchors.top: parent.top
            size: 140 * Tokens.uiScale
            source: root.portrait
        }
        Text {
            x: 8 * Tokens.uiScale
            y: 138 * Tokens.uiScale
            width: 152 * Tokens.uiScale
            text: root.characterName
            color: Tokens.textOnPanel
            horizontalAlignment: Text.AlignHCenter
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontBody
            elide: Text.ElideRight
        }
        Gauge {
            x: 8 * Tokens.uiScale
            y: 164 * Tokens.uiScale
            width: 152 * Tokens.uiScale
            kind: "health"
            value: root.hitPointsRatio
            label: root.hitPointsText
        }
        Text {
            x: 190 * Tokens.uiScale
            width: 860 * Tokens.uiScale
            text: qsTr("Action %1 / %2   ·   Bonus %3 / %4   ·   Déplacement %5")
                      .arg(root.activeActions).arg(root.activeActionsMax)
                      .arg(root.activeBonusActions).arg(root.activeBonusActionsMax).arg(root.activeMovement)
            color: Tokens.goldLight
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontSectionTitle
            horizontalAlignment: Text.AlignHCenter
        }
        Row {
            x: 250 * Tokens.uiScale
            y: 34 * Tokens.uiScale
            spacing: Tokens.gapSmall
            Repeater {
                model: [qsTr("Toutes"), qsTr("Actions"), qsTr("Sorts")]
                delegate: OrnateTab {
                    id: filterTab
                    required property int index
                    required property string modelData
                    width: 220 * Tokens.uiScale
                    height: 40 * Tokens.uiScale
                    text: modelData
                    checked: root.actionFilter === index
                    checkable: false
                    Connections {
                        target: filterTab
                        function onClicked() {
                            root.filterRequested(filterTab.index)
                        }
                    }
                }
            }
        }
        Row {
            x: 194 * Tokens.uiScale
            y: 82 * Tokens.uiScale
            spacing: 14 * Tokens.uiScale
            Repeater {
                model: root.actions
                delegate: Item {
                    id: actionEntry
                    required property int index
                    required property string label
                    required property string value
                    required property string iconKey
                    width: 88 * Tokens.uiScale
                    height: 110 * Tokens.uiScale
                    ActionSlot {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 66 * Tokens.uiScale
                        label: actionEntry.label
                        iconKey: actionEntry.iconKey
                        quantity: actionEntry.value
                        shortcut: "" + (actionEntry.index + 1)
                        active: actionEntry.index === root.activeAction
                        forcedState: actionEntry.value === "0" ? "disabled" : ""
                    }
                    Text {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        text: actionEntry.label
                        color: Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                    MouseArea {
                        id: actionPointer
                        anchors.fill: parent
                    }
                    Connections {
                        target: actionPointer
                        function onClicked() {
                            root.actionClicked(actionEntry.index)
                        }
                    }
                }
            }
        }
        Column {
            anchors.right: parent.right
            y: 54 * Tokens.uiScale
            spacing: Tokens.gapSmall
            OrnateButton {
                id: endTurnButton
                width: 250 * Tokens.uiScale
                text: qsTr("Fin du tour")
                enabled: !root.ended && !root.busy && root.playerTurn
            }
            Connections {
                target: endTurnButton
                function onClicked() {
                    root.endTurnRequested()
                }
            }
            Row {
                visible: root.actionPaging
                spacing: Tokens.gapSmall
                OrnateButton { id: previousPageButton; width: 120 * Tokens.uiScale; height: 42 * Tokens.uiScale; kind: "secondary"; text: "◀" }
                Connections { target: previousPageButton; function onClicked() { root.actionPageRequested(-1) } }
                OrnateButton { id: nextPageButton; width: 120 * Tokens.uiScale; height: 42 * Tokens.uiScale; kind: "secondary"; text: "▶" }
                Connections { target: nextPageButton; function onClicked() { root.actionPageRequested(1) } }
            }
            Text {
                width: 250 * Tokens.uiScale
                text: root.activeConditions.length > 0 ? root.activeConditions : qsTr("Espace · Fin du tour")
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // Une seule fiche contextuelle. Les règles détaillées se déplient à la demande.
    PanelFrame {
        id: previewPanel
        objectName: "combatPreview"
        z: 4
        visible: root.previewTitle.length > 0 && !root.ended
        x: Math.max(Tokens.gapMedium, Math.min(root.width - width * Tokens.hudScale - Tokens.gapMedium,
                                             root.pointerX + 28 * Tokens.uiScale))
        y: Math.max(170 * Tokens.uiScale, Math.min(root.pointerY - height * Tokens.hudScale / 2,
                                                  root.height - (260 * Tokens.uiScale + height) * Tokens.hudScale))
        width: 380 * Tokens.uiScale
        height: Math.min(520 * Tokens.uiScale, previewColumn.implicitHeight + 40 * Tokens.uiScale)
        subpanel: true
        scale: Tokens.hudScale
        transformOrigin: Item.TopLeft
        MouseArea { anchors.fill: parent }
        Flickable {
            anchors.fill: parent
            clip: true
            contentHeight: previewColumn.implicitHeight
            ScrollBar.vertical: ScrollBar {}
        Column {
            id: previewColumn
            width: parent.width
            spacing: Tokens.gapSmall
            Text {
                width: parent.width
                text: root.previewTitle
                color: root.previewValid ? Tokens.goldLight : Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontSectionTitle
                wrapMode: Text.WordWrap
            }
            Repeater {
                model: root.previewLines
                delegate: Text {
                    required property string label
                    required property string value
                    width: previewColumn.width
                    text: label + " : " + value
                    color: Tokens.textOnPanel
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontBody
                    wrapMode: Text.WordWrap
                }
            }
            Text {
                width: parent.width
                visible: root.targetConditions.length > 0
                text: root.targetConditions
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                wrapMode: Text.WordWrap
            }
            OrnateButton {
                id: detailsToggle
                width: parent.width
                height: 38 * Tokens.uiScale
                kind: "secondary"
                text: root.detailsExpanded ? qsTr("Masquer les détails") : qsTr("Détails du calcul")
            }
            Connections {
                target: detailsToggle
                function onClicked() {
                    root.detailsToggleRequested()
                }
            }
            Repeater {
                model: root.detailsExpanded ? root.previewCapacities : null
                delegate: Text {
                    required property string label
                    required property string value
                    required property bool applies
                    required property string reason
                    width: previewColumn.width
                    text: label + " : " + (applies ? value : reason)
                    color: Tokens.textOnPanel
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontCaption
                    wrapMode: Text.WordWrap
                }
            }
            Text {
                visible: root.detailsExpanded && root.previewExpected.length > 0
                text: qsTr("Dégâts moyens : %1").arg(root.previewExpected)
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
            }
        }
        }
    }
    Text {
        z: 5
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: actionDock.top
        anchors.bottomMargin: (Tokens.hudScale - 1) * actionDock.height + Tokens.gapSmall
        width: 900 * Tokens.uiScale
        text: root.busy ? qsTr("Action en cours…") : root.status
        color: Tokens.textOnPanel
        font.family: Tokens.bodyFamily
        font.pixelSize: Tokens.fontBody
        style: Text.Outline
        styleColor: Tokens.panel
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }
    OrnateButton {
        id: logToggle
        z: 5
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Tokens.gapMedium
        width: 144 * Tokens.uiScale
        height: 50 * Tokens.uiScale
        scale: Tokens.hudScale
        transformOrigin: Item.TopLeft
        kind: "secondary"
        text: qsTr("Historique")
    }
    Connections {
        target: logToggle
        function onClicked() {
            root.logToggleRequested()
        }
    }
    PanelFrame {
        z: 6
        visible: root.logExpanded
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: Tokens.gapMedium
        anchors.bottomMargin: 78 * Tokens.uiScale * Tokens.hudScale
        width: 540 * Tokens.uiScale
        height: 340 * Tokens.uiScale
        subpanel: true
        scale: Tokens.hudScale
        transformOrigin: Item.BottomLeft
        MouseArea { anchors.fill: parent }
        ListView {
            anchors.fill: parent
            clip: true
            model: root.combatLog
            delegate: Text {
                required property string label
                width: ListView.view.width
                text: label
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                wrapMode: Text.WordWrap
            }
        }
    }
    PanelFrame {
        z: 9
        anchors.centerIn: parent
        width: 620 * Tokens.uiScale
        height: 260 * Tokens.uiScale
        visible: root.outcome.length > 0
        Column {
            anchors.centerIn: parent
            spacing: Tokens.gapMedium
            Text {
                width: 540 * Tokens.uiScale
                horizontalAlignment: Text.AlignHCenter
                text: root.outcome === "victory" ? qsTr("Victoire") : root.outcome === "flight" ? qsTr("Vous avez pris la fuite") : qsTr("Vous êtes mort")
                color: Tokens.goldLight
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontSectionTitle
            }
            Text {
                width: 540 * Tokens.uiScale
                text: root.status
                wrapMode: Text.WordWrap
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
            }
            OrnateButton {
                id: leaveButton
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Reprendre l’exploration")
            }
            Connections {
                target: leaveButton
                function onClicked() {
                    root.leaveRequested()
                }
            }
        }
    }
}
