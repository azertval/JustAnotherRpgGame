pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
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
        ListElement { rowId: "rapiere"; label: "Rapière"; value: ""; iconKey: "" }
        ListElement { rowId: "arc"; label: "Arc court"; value: ""; iconKey: "" }
        ListElement { rowId: "dague"; label: "Dague"; value: ""; iconKey: "" }
        ListElement { rowId: "trait"; label: "Trait de feu"; value: ""; iconKey: "ui/icon/spell/fire-bolt" }
        ListElement { rowId: "projectile"; label: "Projectile magique"; value: "2"; iconKey: "ui/icon/spell/magic-missile" }
        ListElement { rowId: "esquiver"; label: "Esquiver"; value: ""; iconKey: "" }
        ListElement { rowId: "desengager"; label: "Se désengager"; value: ""; iconKey: "" }
        ListElement { rowId: "precipiter"; label: "Se précipiter"; value: ""; iconKey: "" }
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
        y: 316 * Tokens.uiScale
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

    // --- Le tour du personnage actif (LOT-140) : sous son nom, a la place de l'experience --------------
    PanelFrame {
        x: 216 * Tokens.uiScale
        y: 138 * Tokens.uiScale
        width: 384 * Tokens.uiScale
        height: 164 * Tokens.uiScale
        subpanel: true

        Column {
            anchors.fill: parent
            spacing: 4 * Tokens.uiScale
            clip: true

            Text {
                width: parent.width
                text: qsTr("Niv. %1 · CA %2").arg(root.activeLevel).arg(root.activeArmorClass)
                      + (root.activeConditions.length > 0 ? " · " + root.activeConditions : "")
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
                elide: Text.ElideRight
            }

            // Ce qu'il reste a depenser : une pastille par action, pleine tant qu'elle est la.
            Row {
                spacing: Tokens.gapSmall

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Action")
                    color: Tokens.textOnPanelMuted
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontCaption
                }

                Repeater {
                    model: root.activeActionsMax

                    Rectangle {
                        id: actionPip

                        required property int index

                        anchors.verticalCenter: parent.verticalCenter
                        width: 12 * Tokens.uiScale
                        height: 12 * Tokens.uiScale
                        radius: width / 2
                        color: actionPip.index < root.activeActions ? Tokens.goldLight : "transparent"
                        border.color: Tokens.goldLight
                        border.width: Tokens.strokeWidth
                    }
                }

                Item { width: Tokens.gapSmall; height: 1 }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Bonus")
                    color: Tokens.textOnPanelMuted
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontCaption
                }

                Repeater {
                    model: root.activeBonusActionsMax

                    Rectangle {
                        id: bonusPip

                        required property int index

                        anchors.verticalCenter: parent.verticalCenter
                        width: 12 * Tokens.uiScale
                        height: 12 * Tokens.uiScale
                        radius: width / 2
                        color: bonusPip.index < root.activeBonusActions ? Tokens.goldLight : "transparent"
                        border.color: Tokens.goldLight
                        border.width: Tokens.strokeWidth
                    }
                }

                Item { width: Tokens.gapSmall; height: 1 }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Déplacement %1").arg(root.activeMovement)
                    color: Tokens.textOnPanelMuted
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontCaption
                }
            }

            // Les capacites de classe, icone et nom : ce que la fiche apporte au combat.
            Repeater {
                model: root.activeCapacities

                Item {
                    id: capacity

                    required property string label
                    required property string value

                    width: parent.width
                    height: 28 * Tokens.uiScale

                    Rectangle {
                        id: capacityFallback

                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 24 * Tokens.uiScale
                        height: 24 * Tokens.uiScale
                        radius: width / 2
                        visible: !capacityIcon.delivered
                        color: Tokens.panelRaised
                        border.color: Tokens.goldLight
                        border.width: Tokens.strokeWidth

                        Text {
                            anchors.centerIn: parent
                            text: capacity.label.length > 0 ? capacity.label.charAt(0) : ""
                            color: Tokens.goldLight
                            font.family: Tokens.titleFamily
                            font.pixelSize: Tokens.fontCaption
                        }
                    }

                    FixedArt {
                        id: capacityIcon

                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 24 * Tokens.uiScale
                        height: 24 * Tokens.uiScale
                        key: capacity.value
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 32 * Tokens.uiScale
                        anchors.verticalCenter: parent.verticalCenter
                        text: capacity.label
                        color: Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    // --- Ordre d'initiative aux jetons (LOT-140), sous la boussole ---------------------------------------
    Item {
        x: 640 * Tokens.uiScale
        y: 118 * Tokens.uiScale
        width: 960 * Tokens.uiScale
        height: 96 * Tokens.uiScale

        Text {
            id: roundLabel

            anchors.left: parent.left
            anchors.top: parent.top
            text: qsTr("Round %1").arg(root.round)
            color: Tokens.goldLight
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontCaption
            font.weight: Font.DemiBold
            style: Text.Outline
            styleColor: Tokens.panel
        }

        Row {
            anchors.left: parent.left
            anchors.top: roundLabel.bottom
            anchors.topMargin: 2 * Tokens.uiScale
            spacing: Tokens.gapSmall

            Repeater {
                model: root.initiative

                Item {
                    id: combatant

                    required property int index
                    required property string label
                    required property string value
                    required property string token
                    required property string initials
                    required property bool down

                    readonly property bool current: combatant.index === root.activeIndex
                    readonly property bool enemy: combatant.value === "enemy"
                    readonly property color tint: combatant.enemy ? Tokens.textEnemy : Tokens.textAlly

                    width: 88 * Tokens.uiScale
                    height: 72 * Tokens.uiScale
                    opacity: combatant.down ? 0.45 : 1

                    // Le jeton : l'image du membre du groupe, sinon deux lettres sur un disque du
                    // camp. L'actif est cercle d'or (maquette : ①).
                    Rectangle {
                        id: tokenRing

                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        width: 48 * Tokens.uiScale
                        height: 48 * Tokens.uiScale
                        radius: width / 2
                        color: Tokens.panel
                        border.color: combatant.current ? Tokens.goldLight : combatant.tint
                        border.width: (combatant.current ? 3 : 2) * Tokens.strokeWidth

                        Rectangle {
                            id: tokenMask

                            anchors.fill: parent
                            anchors.margins: tokenRing.border.width
                            radius: width / 2
                            color: Tokens.panelRaised
                            layer.enabled: true
                        }

                        Image {
                            id: tokenImage

                            anchors.fill: tokenMask
                            visible: false
                            source: combatant.token
                            fillMode: Image.PreserveAspectCrop
                            smooth: true
                            mipmap: true
                        }

                        MultiEffect {
                            anchors.fill: tokenMask
                            visible: combatant.token.length > 0
                            source: tokenImage
                            maskEnabled: true
                            maskSource: tokenMask
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: combatant.token.length === 0
                            text: combatant.initials
                            color: combatant.current ? Tokens.goldLight : combatant.tint
                            font.family: Tokens.titleFamily
                            font.pixelSize: Tokens.fontCaption
                            font.weight: Font.DemiBold
                        }
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: tokenRing.bottom
                        anchors.topMargin: 2 * Tokens.uiScale
                        text: combatant.label
                        color: combatant.current ? Tokens.goldLight : Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        style: Text.Outline
                        styleColor: Tokens.panel
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

        Column {
            anchors.centerIn: parent
            width: parent.width * 0.64
            spacing: 2 * Tokens.uiScale

            Text {
                width: parent.width
                text: root.activeActionLabel
                color: Tokens.textOnPanel
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontBody
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            // Le jet et les des de l'action choisie (LOT-140).
            Text {
                width: parent.width
                visible: text.length > 0
                text: root.activeActionDetail
                color: Tokens.goldLight
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }
    }

    // --- Barre d'actions (maquette : 555, 778 -> 1090, 880) ------------------------------------------------
    // Huit cases : une fenetre glissante sur les actions du tour, que le jumeau fait suivre l'action
    // choisie (un mage de niveau 5 en a quatorze).
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
                    required property string iconKey
                    required label

                    quantity: actionCell.value
                    shortcut: "" + (actionCell.index + 1)
                    active: actionCell.index === root.activeAction

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.actionClicked(actionCell.index)
                    }
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

    // --- Previsualisation (LOT-140) : a la place des quetes, au-dessus de la cible -----------------------------
    PanelFrame {
        x: 1600 * Tokens.uiScale
        y: 312 * Tokens.uiScale
        width: 296 * Tokens.uiScale
        height: 428 * Tokens.uiScale
        subpanel: true

        Text {
            id: previewHeading

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            text: qsTr("Prévisualisation")
            color: Tokens.textOnPanel
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontSectionTitle
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        GoldDivider {
            id: previewDivider

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: previewHeading.bottom
            anchors.topMargin: Tokens.gapSmall
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: previewDivider.bottom
            anchors.bottom: parent.bottom
            anchors.topMargin: Tokens.gapSmall
            spacing: 4 * Tokens.uiScale
            clip: true
            opacity: root.previewValid ? 1 : 0.7

            Text {
                width: parent.width
                text: root.previewTitle
                color: Tokens.goldLight
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontBody
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Repeater {
                model: root.previewLines

                Item {
                    id: previewRow

                    required property string label
                    required property string value

                    width: parent.width
                    height: Math.max(previewLabel.implicitHeight, previewValue.implicitHeight)

                    Text {
                        id: previewLabel

                        anchors.left: parent.left
                        anchors.top: parent.top
                        width: 86 * Tokens.uiScale
                        text: previewRow.label
                        color: Tokens.textOnPanelMuted
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                        elide: Text.ElideRight
                    }

                    Text {
                        id: previewValue

                        anchors.left: previewLabel.right
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: Tokens.gapSmall
                        text: previewRow.value
                        color: Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }
                }
            }

            // La capacite qui joue -- et celle qui ne joue pas, avec sa raison (maquette : ④).
            Repeater {
                model: root.previewCapacities

                Rectangle {
                    id: previewCapacity

                    required property string label
                    required property string value
                    required property bool applies
                    required property string reason

                    width: parent.width
                    height: capacityColumn.implicitHeight + Tokens.gapSmall
                    color: previewCapacity.applies ? Tokens.panelRaised : "transparent"
                    border.color: previewCapacity.applies ? Tokens.goldLight : Tokens.panelEdge
                    border.width: Tokens.strokeWidth

                    Column {
                        id: capacityColumn

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Tokens.gapSmall / 2
                        spacing: 0

                        Text {
                            width: parent.width
                            text: (previewCapacity.applies ? "+ " : "") + previewCapacity.label + "  " + previewCapacity.value
                            color: previewCapacity.applies ? Tokens.goldLight : Tokens.textOnPanelMuted
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontCaption
                            font.weight: previewCapacity.applies ? Font.DemiBold : Font.Normal
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            visible: text.length > 0
                            text: previewCapacity.reason
                            color: Tokens.textOnPanelMuted
                            font.family: Tokens.loreFamily
                            font.italic: true
                            font.pixelSize: Tokens.fontCaption
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Text {
                width: parent.width
                visible: root.previewExpected.length > 0
                text: qsTr("Attendu  %1 dégâts").arg(root.previewExpected)
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
                elide: Text.ElideRight
            }
        }
    }

    // --- Fiche de la cible (maquette : 1100, 678 -> 1390, 828) ----------------------------------------------
    // Rien que ce que la table voit (LOT-23) : un ennemi n'a pas de chiffre de vie, seulement
    // « ensanglante », « a terre », « mort ».
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
            visible: root.targetLevel.length > 0
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
