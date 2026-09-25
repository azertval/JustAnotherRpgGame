import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    HUD de combat -- CABLAGE, cote developpeur (LOT-86, LOT-87 T4.1, LOT-118).

    LE COMBAT SUR LA CARTE. `EncounterModel` tient la rencontre montee sur la zone de combat de la
    carte courante ; `WorldViewport` dessine la carte gelee et, a la place des figurines de
    l'exploration, les combattants que le modele publie a chaque pas (`WorldModel.figures`) --
    le meme pipeline, la meme camera, qui suit le heros de la grille. Le calque tactique du
    formulaire (`TacticalLayer`) se cale sur le cadrage que la surface publie, decale de l'origine
    de la zone.

    Les gestes sont ceux du Colisee (`Arena.qml`, LOT-24), au clavier, a la manette et a la souris,
    par les memes touches :

    | Geste | Clavier | Manette |
    |---|---|---|
    | Deplacer le curseur | fleches | croix ou stick gauche |
    | Confirmer (attaquer, se deplacer, l'action choisie) ; quitter une fois fini | Entree | A |
    | Cible suivante, precedente | Tab, Maj+Tab | X |
    | Action suivante, precedente | Page suivante, Page precedente, 1 a 9 | RB, LB |
    | Recentrer sur le combattant actif | Retour arriere | B |
    | Fin du tour (aussi le bouton sous la fiche de la cible) | Espace | Y |
    | Fuir (si la rencontre le permet) | F | -- |

    Tant qu'un mouvement se joue (`busy`), les gestes attendent : le modele les ignore, et
    `Entree` saute l'animation. Une fois l'issue publiee, `Entree` ou le bouton du panneau rendent
    l'exploration (`leave`). Une DEFAITE ouvre l'ecran de mort des qu'elle est publiee (LOT-119) :
    le combat n'est pas quitte, pour que l'ecran de mort montre la scene ou le heros est tombe.

    Les valeurs du HUD qui ne viennent pas encore d'un lot (groupe, quetes, horloge, minicarte)
    restent des donnees en attente (`PendingData`, cles `hud.*`), comme dans la vue de jeu.
*/
CombatHudForm {
    id: root

    focus: true
    pending: !EncounterModel.active

    characterName: EncounterModel.active ? EncounterModel.heroName : PendingData.value("hud.character.name")
    level: PendingData.value("hud.character.level")
    hitPointsText: EncounterModel.active ? EncounterModel.heroHitPoints : PendingData.value("hud.character.hit_points")
    hitPointsRatio: EncounterModel.active ? EncounterModel.heroHitPointsRatio : 0
    experienceText: PendingData.value("hud.character.experience")
    experienceRatio: 0
    portrait: PendingData.image("hud.character.portrait")
    party: PendingData.rows("hud.party", 4)
    activeMember: -1
    quests: PendingData.rows("hud.quests", 2)
    clock: PendingData.value("hud.clock")
    location: WorldModel.loaded ? WorldModel.mapName : PendingData.value("hud.location")
    minimap: PendingData.image("hud.minimap")

    // L'ordre, le journal et les actions se lisent du modele ; les roles sont ceux du formulaire
    // (`label`, `value`).
    initiative: root.initiativeRows(EncounterModel.turnOrder)
    activeIndex: root.activeRow(EncounterModel.turnOrder)
    combatLog: root.logRows(EncounterModel.journal)
    actions: root.actionRows(EncounterModel.turnActions)
    activeAction: root.selectedRow(EncounterModel.turnActions)
    activeActionLabel: root.selectedLabel(EncounterModel.turnActions)
    targetName: EncounterModel.target.name !== undefined ? EncounterModel.target.name : ""
    targetLevel: ""
    targetHitPoints: EncounterModel.target.hitPoints !== undefined ? EncounterModel.target.hitPoints : ""
    targetHitPointsRatio: EncounterModel.target.hitPointsRatio !== undefined ? EncounterModel.target.hitPointsRatio : 0
    targetArmorClass: EncounterModel.target.armorClass !== undefined ? EncounterModel.target.armorClass : ""
    targetInitiative: ""
    targetSpeed: EncounterModel.target.speed !== undefined ? EncounterModel.target.speed : ""
    targetConditions: EncounterModel.target.conditions !== undefined ? EncounterModel.target.conditions : ""
    targetPortrait: ""

    // Le calque tactique (LOT-118)
    fighters: EncounterModel.fighters
    reachableCells: EncounterModel.reachableCells
    pathCells: EncounterModel.pathCells
    cursorColumn: EncounterModel.cursorColumn
    cursorRow: EncounterModel.cursorRow
    ended: EncounterModel.ended
    zoneColumn: EncounterModel.zoneColumn
    zoneRow: EncounterModel.zoneRow
    gridTileWidth: viewport.tileWidth
    gridTileHeight: viewport.tileHeight
    gridOriginX: viewport.originX
    gridOriginY: viewport.originY
    status: EncounterModel.status
    busy: EncounterModel.busy
    outcome: EncounterModel.outcome

    // La scene du combat : la surface de rendu de l'exploration, sur la carte gelee, dont les
    // figurines sont les combattants tant que la rencontre dure.
    WorldViewport {
        id: viewport

        parent: root.viewportHost
        anchors.fill: parent
        model: WorldModel
        clearColor: Tokens.panel
    }

    onGridHovered: (x, y) => {
        const cell = viewport.cellAt(x, y)
        if (cell.x >= 0) {
            EncounterModel.pointCursor(cell.x - EncounterModel.zoneColumn, cell.y - EncounterModel.zoneRow)
        }
    }
    onGridClicked: (x, y) => {
        const cell = viewport.cellAt(x, y)
        if (cell.x >= 0) {
            EncounterModel.tapCell(cell.x - EncounterModel.zoneColumn, cell.y - EncounterModel.zoneRow)
        }
        root.forceActiveFocus()
    }
    onLeaveRequested: root.leave()
    onEndTurnRequested: {
        EncounterModel.endTurn()
        root.forceActiveFocus()
    }

    /// Quitter le combat fini : l'exploration reprend. Une defaite ne se quitte pas ainsi -- elle
    /// mene a l'ecran de mort, qui quittera le combat lui-meme (LOT-119).
    function leave() {
        if (EncounterModel.outcome === "defeat") {
            ScreenRouter.openDeath()
            return
        }
        EncounterModel.leave()
        ScreenRouter.closeRpgScreen()
    }

    // La defaite n'attend pas de geste : le heros tombe, la file a fini de le montrer, l'ecran de
    // mort prend la place (LOT-119).
    Connections {
        target: EncounterModel

        function onChanged() {
            if (EncounterModel.outcome === "defeat") {
                ScreenRouter.openDeath()
            }
        }
    }

    /// Le geste « confirmer » : sauter l'animation, quitter une fois fini, sinon l'action choisie.
    function confirm() {
        if (EncounterModel.busy) {
            EncounterModel.skipAnimations()
        } else if (EncounterModel.outcome.length > 0) {
            root.leave()
        } else {
            EncounterModel.confirm()
        }
    }

    function gamepad(button) {
        switch (button) {
        case "up": EncounterModel.moveCursor(0, -1); break
        case "down": EncounterModel.moveCursor(0, 1); break
        case "left": EncounterModel.moveCursor(-1, 0); break
        case "right": EncounterModel.moveCursor(1, 0); break
        case "a": root.confirm(); break
        case "b": EncounterModel.centerCursor(); break
        case "x": EncounterModel.cycleTarget(1); break
        case "y": EncounterModel.endTurn(); break
        case "lb": EncounterModel.cycleAction(-1); break
        case "rb": EncounterModel.cycleAction(1); break
        }
    }

    GamepadNavigator {
        id: pad

        active: root.visible
        onPressed: (button) => root.gamepad(button)
    }

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_Up: EncounterModel.moveCursor(0, -1); break
        case Qt.Key_Down: EncounterModel.moveCursor(0, 1); break
        case Qt.Key_Left: EncounterModel.moveCursor(-1, 0); break
        case Qt.Key_Right: EncounterModel.moveCursor(1, 0); break
        case Qt.Key_Return:
        case Qt.Key_Enter: root.confirm(); break
        case Qt.Key_Tab: EncounterModel.cycleTarget(1); break
        case Qt.Key_Backtab: EncounterModel.cycleTarget(-1); break
        case Qt.Key_PageDown: EncounterModel.cycleAction(1); break
        case Qt.Key_PageUp: EncounterModel.cycleAction(-1); break
        case Qt.Key_Backspace: EncounterModel.centerCursor(); break
        case Qt.Key_Space: EncounterModel.endTurn(); break
        case Qt.Key_F: EncounterModel.withdraw(); break
        default:
            if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                EncounterModel.selectAction(event.key - Qt.Key_1)
                break
            }
            return
        }
        event.accepted = true
    }

    // --- Les listes du formulaire, tirees des listes du modele ---------------------------------
    function initiativeRows(order) {
        const rows = []
        for (let i = 0; i < order.length; ++i) {
            rows.push({ rowId: "" + i, label: order[i].name, value: order[i].side === "allies" ? "ally" : "enemy" })
        }
        return rows
    }

    function activeRow(order) {
        for (let i = 0; i < order.length; ++i) {
            if (order[i].active) {
                return i
            }
        }
        return -1
    }

    function logRows(journal) {
        const rows = []
        const first = Math.max(0, journal.length - 5)
        for (let i = first; i < journal.length; ++i) {
            const line = journal[i]
            const turn = line.indexOf("tour ") === 0 ? "ally" : ""
            rows.push({ rowId: "" + i, label: line, value: turn })
        }
        return rows
    }

    function actionRows(actions) {
        const rows = []
        for (let i = 0; i < actions.length && i < 8; ++i) {
            rows.push({ rowId: "" + i, label: actions[i].label, value: actions[i].enabled ? "" : "0" })
        }
        return rows
    }

    function selectedRow(actions) {
        for (let i = 0; i < actions.length; ++i) {
            if (actions[i].selected) {
                return i
            }
        }
        return 0
    }

    function selectedLabel(actions) {
        const row = root.selectedRow(actions)
        return row < actions.length ? actions[row].label : ""
    }

    Connections {
        target: root.inventoryButton
        function onClicked() { ScreenRouter.openRpgScreen(ScreenRouter.Inventory) }
    }
    Connections {
        target: root.journalButton
        function onClicked() { ScreenRouter.openRpgScreen(ScreenRouter.QuestJournal) }
    }
    Connections {
        target: root.mapButton
        function onClicked() { ScreenRouter.openRpgScreen(ScreenRouter.WorldMap) }
    }
    Connections {
        target: root.optionsButton
        function onClicked() { ScreenRouter.openOptions() }
    }
}
