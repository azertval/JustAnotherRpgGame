import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    HUD de combat -- CABLAGE, cote developpeur (LOT-86, LOT-87 T4.1, LOT-118, LOT-140).

    LE COMBAT SUR LA CARTE. `EncounterModel` tient la rencontre montee sur la zone de combat de la
    carte courante ; `WorldViewport` dessine la carte gelee et, a la place des figurines de
    l'exploration, les combattants que le modele publie a chaque pas (`WorldModel.figures`) --
    le meme pipeline, la meme camera, qui suit le heros de la grille. Le calque tactique du
    formulaire (`TacticalLayer`) se cale sur le cadrage que la surface publie, decale de l'origine
    de la zone.

    Les gestes sont herites de l'ecran du Colisee (LOT-24), retire au profit du combat sur la carte :
    au clavier et a la souris, par les memes commandes :

    | Geste | Clavier |
    |---|---|
    | Deplacer le curseur | fleches |
    | Confirmer (attaquer, se deplacer, l'action choisie) ; quitter une fois fini | Entree |
    | Cible suivante, precedente | Tab, Maj+Tab |
    | Action suivante, precedente | Page suivante, Page precedente, 1 a 8 (la case visible) |
    | Recentrer sur le combattant actif | Retour arriere |
    | Fin du tour (aussi le bouton sous la fiche de la cible) | Espace |
    | Fuir (si la rencontre le permet) | F |

    LE GROUPE (LOT-139, LOT-140). Les quatre entrent en combat ; chacun est joue par ces memes
    gestes a son tour d'initiative. Le portrait en avant est celui du membre dont c'est le tour
    (sinon le meneur), le panneau du tour lit `activeProfile` (niveau, CA, etats, ressources,
    capacites), l'ordre d'initiative ses jetons (`turnOrder`), la previsualisation `preview` --
    l'action choisie sur la case du curseur, telle qu'elle serait jetee. Tout ce que le panneau
    montre se pilote par les gestes ci-dessus : la previsualisation suit le curseur et l'action
    choisie, rien n'y demande la souris.

    LA BARRE D'ACTIONS montre huit cases : une fenetre glissante sur `turnActions`, qui suit
    l'action choisie (`windowStart`). Les touches 1 a 8 choisissent la case VISIBLE de ce rang.

    Tant qu'un mouvement se joue (`busy`), les gestes attendent : le modele les ignore, et
    `Entree` saute l'animation. Une fois l'issue publiee, `Entree` ou le bouton du panneau rendent
    l'exploration (`leave`). Une DEFAITE ouvre l'ecran de mort des qu'elle est publiee (LOT-119) :
    le combat n'est pas quitte, pour que l'ecran de mort montre la scene ou le heros est tombe.

    Les valeurs du HUD qui ne viennent pas encore d'un lot (horloge, minicarte) restent des
    donnees en attente (`PendingData`, cles `hud.*`), comme dans la vue de jeu.
*/
CombatHudForm {
    id: root

    focus: true
    pending: !EncounterModel.active

    /// La barre montre huit cases a la fois.
    readonly property int actionWindow: 8
    playerTurn: EncounterModel.activeMember >= 0
    readonly property var filteredActions: EncounterModel.turnActions.map((row, index) => Object.assign({}, row, {sourceIndex: index}))
        .filter(row => root.actionFilter === 0 || (root.actionFilter === 2) === (row.iconKey || "").startsWith("ui/icon/spell/"))
    actionPaging: root.filteredActions.length > root.actionWindow
    onFilterRequested: (filter) => {
        root.actionFilter = filter
        if (root.filteredActions.length > 0) EncounterModel.selectAction(root.filteredActions[0].sourceIndex)
    }
    onActionPageRequested: (direction) => root.cycleDisplayed(direction * root.actionWindow)
    onDetailsToggleRequested: root.detailsExpanded = !root.detailsExpanded
    onLogToggleRequested: root.logExpanded = !root.logExpanded
    function selectVisible(index) {
        const row = root.filteredActions[root.windowStart + index]
        if (row) EncounterModel.selectAction(row.sourceIndex)
    }
    function cycleDisplayed(step) {
        const rows = root.filteredActions
        if (rows.length === 0) return
        const next = (root.selectedRow(rows) + step % rows.length + rows.length) % rows.length
        EncounterModel.selectAction(rows[next].sourceIndex)
    }
    /// La premiere action visible dans la barre : la fenetre suit l'action choisie.
    readonly property int windowStart: root.windowStartFor(root.filteredActions, root.selectedRow(root.filteredActions))

    // Le personnage en avant : le membre du groupe dont c'est le tour, sinon le meneur (LOT-139).
    readonly property var focusMember: root.memberInFront(EncounterModel.partyMembers, EncounterModel.activeMember)
    readonly property var profile: EncounterModel.activeProfile
    readonly property var previewMap: EncounterModel.preview
    characterName: EncounterModel.active ? (focusMember ? focusMember.label : EncounterModel.heroName) : PendingData.value("hud.character.name")
    level: EncounterModel.active && profile.level !== undefined && profile.level > 0 ? "" + profile.level : PendingData.value("hud.character.level")
    hitPointsText: EncounterModel.active ? (focusMember ? focusMember.value : EncounterModel.heroHitPoints) : PendingData.value("hud.character.hit_points")
    hitPointsRatio: EncounterModel.active ? (focusMember ? focusMember.ratio : EncounterModel.heroHitPointsRatio) : 0
    experienceText: PendingData.value("hud.character.experience")
    experienceRatio: 0
    portrait: EncounterModel.active && focusMember ? focusMember.portrait : PendingData.image("hud.character.portrait")
    party: EncounterModel.active ? EncounterModel.partyMembers : PendingData.rows("hud.party", 4)
    activeMember: EncounterModel.active ? EncounterModel.activeMember : -1
    quests: PendingData.rows("hud.quests", 2)
    clock: PendingData.value("hud.clock")
    location: WorldModel.loaded ? WorldModel.mapName : PendingData.value("hud.location")
    minimap: PendingData.image("hud.minimap")

    // L'ordre, le journal et les actions se lisent du modele ; les roles sont ceux du formulaire
    // (`label`, `value`).
    initiative: root.initiativeRows(EncounterModel.turnOrder)
    activeIndex: root.activeRow(EncounterModel.turnOrder)
    round: "" + EncounterModel.round
    combatLog: root.logRows(EncounterModel.journal)
    actions: root.actionRows(root.filteredActions, root.windowStart)
    activeAction: root.selectedRow(root.filteredActions) - root.windowStart
    activeActionLabel: root.selectedField(EncounterModel.turnActions, "label")
    activeActionDetail: root.selectedField(EncounterModel.turnActions, "detail")

    // Le tour du personnage actif (LOT-140).
    activeLevel: profile.level !== undefined && profile.level > 0 ? "" + profile.level : "—"
    activeArmorClass: profile.armorClass !== undefined ? "" + profile.armorClass : ""
    activeConditions: profile.conditions !== undefined ? profile.conditions.join(", ") : ""
    activeActions: profile.action !== undefined ? profile.action : 0
    activeActionsMax: profile.actionMax !== undefined ? profile.actionMax : 0
    activeBonusActions: profile.bonusAction !== undefined ? profile.bonusAction : 0
    activeBonusActionsMax: profile.bonusActionMax !== undefined ? profile.bonusActionMax : 0
    activeMovement: profile.movement !== undefined ? profile.movement + " / " + profile.movementMax : ""
    activeCapacities: root.capacityRows(profile.capacities)

    // La previsualisation (LOT-140) : l'action choisie sur la case du curseur.
    previewTitle: previewMap.title !== undefined ? previewMap.title : ""
    previewLines: root.previewRows(previewMap.lines)
    previewCapacities: root.previewCapacityRows(previewMap.capacities)
    previewExpected: previewMap.expected !== undefined ? previewMap.expected : ""
    previewValid: previewMap.valid !== undefined ? previewMap.valid : true

    // La cible : rien que ce que la table voit (LOT-23).
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
        // Les reglages de rendu des options, qui atteignent ainsi le moteur (EX-IHM-083).
        sampleCount: OptionsModel.antialiasing
        renderScalePercent: OptionsModel.renderScalePercent
        shadowSize: OptionsModel.shadows
    }

    onGridHovered: (x, y) => {
        root.pointerX = x
        root.pointerY = y
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
    onActionClicked: (index) => {
        root.selectVisible(index)
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
            if (root.filteredActions.length === 0) root.actionFilter = 0
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
        case Qt.Key_PageDown: root.cycleDisplayed(1); break
        case Qt.Key_PageUp: root.cycleDisplayed(-1); break
        case Qt.Key_Backspace: EncounterModel.centerCursor(); break
        case Qt.Key_Space: EncounterModel.endTurn(); break
        case Qt.Key_F: EncounterModel.withdraw(); break
        case Qt.Key_Escape: ScreenRouter.openOptions(); break
        default:
            // Les touches 1 a 8 : la case VISIBLE de ce rang dans la fenetre de la barre.
            if (event.key >= Qt.Key_1 && event.key <= Qt.Key_1 + root.actionWindow - 1) {
                root.selectVisible(event.key - Qt.Key_1)
                break
            }
            return
        }
        event.accepted = true
    }

    // --- Les listes du formulaire, tirees des listes du modele ---------------------------------
    /// Le membre du groupe en avant : celui dont c'est le tour, sinon le meneur ; null hors combat.
    function memberInFront(members, active) {
        if (!members || members.length === 0) {
            return null
        }
        return members[active >= 0 && active < members.length ? active : 0]
    }

    function initiativeRows(order) {
        const rows = []
        for (let i = 0; i < order.length; ++i) {
            rows.push({ rowId: "" + i,
                        label: order[i].name,
                        value: order[i].side === "allies" ? "ally" : "enemy",
                        token: order[i].token !== undefined ? order[i].token.toString() : "",
                        initials: order[i].initials !== undefined ? order[i].initials : "",
                        down: order[i].down === true })
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

    /// La premiere action visible : la fenetre glisse pour que l'action choisie y soit.
    function windowStartFor(actions, selected) {
        const count = actions ? actions.length : 0
        if (count <= root.actionWindow) {
            return 0
        }
        const last = count - root.actionWindow
        return Math.max(0, Math.min(last, selected - Math.floor(root.actionWindow / 2)))
    }

    function actionRows(actions, start) {
        const rows = []
        for (let i = start; i < actions.length && i < start + root.actionWindow; ++i) {
            // `value` : les lancers restants d'un sort, ou « 0 » pour une action grisee.
            const uses = actions[i].uses !== undefined ? actions[i].uses : -1
            const value = !actions[i].enabled ? "0" : (uses >= 0 ? "" + uses : "")
            rows.push({ rowId: "" + i, label: actions[i].label, value: value,
                        iconKey: actions[i].iconKey !== undefined ? actions[i].iconKey : "" })
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

    function selectedField(actions, field) {
        const row = root.selectedRow(actions)
        return row < actions.length && actions[row][field] !== undefined ? actions[row][field] : ""
    }

    function capacityRows(capacities) {
        const rows = []
        if (!capacities) {
            return rows
        }
        for (let i = 0; i < capacities.length; ++i) {
            rows.push({ rowId: capacities[i].id, label: capacities[i].name, value: capacities[i].iconKey })
        }
        return rows
    }

    function previewRows(lines) {
        const rows = []
        if (!lines) {
            return rows
        }
        for (let i = 0; i < lines.length; ++i) {
            rows.push({ rowId: "" + i, label: lines[i].label, value: lines[i].value })
        }
        return rows
    }

    function previewCapacityRows(capacities) {
        const rows = []
        if (!capacities) {
            return rows
        }
        for (let i = 0; i < capacities.length; ++i) {
            rows.push({ rowId: "" + i, label: capacities[i].name, value: capacities[i].dice,
                        applies: capacities[i].applies === true, reason: capacities[i].reason })
        }
        return rows
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
    Connections {
        target: root.characterButton
        function onClicked() {
            WorldModel.showCharacter(root.focusMember ? root.focusMember.id : "")
            ScreenRouter.openCharacterTab(0)
        }
    }
}
