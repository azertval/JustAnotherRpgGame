import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    HUD de combat -- CABLAGE, cote developpeur (LOT-86, LOT-87 T4.1).

    Alimente par le lot du combat tactique (`LOT-24`) : initiative, journal, actions du tour,
    cible. Le cadre commun lit les memes cles `hud.*` que la vue de jeu (`GameView.qml`).

    Chaque liaison porte une CLE D'ATTRIBUTION nommee, qui aboutit a l'ancre `PendingData`. Le
    jour ou le lot fonctionnel arrive, il remplace ici `PendingData` par sa vraie vue-modele :
    le formulaire ne bouge pas, et la mise en page decidee aujourd'hui est conservee telle quelle.

    `python scripts/i18n/list_pending_bindings.py` releve ces cles depuis le QML : l'inventaire de ce
    qu'il reste a brancher est DERIVE du code, donc toujours exact.

    Les touches `1` a `8` choisissent l'action de la case correspondante : c'est l'etat de l'ecran,
    que la roue affiche, et que le tour par tour lira.
*/
CombatHudForm {
    id: root

    focus: true
    pending: true

    characterName: PendingData.value("hud.character.name")
    level: PendingData.value("hud.character.level")
    hitPointsText: PendingData.value("hud.character.hit_points")
    hitPointsRatio: 0
    experienceText: PendingData.value("hud.character.experience")
    experienceRatio: 0
    portrait: PendingData.image("hud.character.portrait")
    party: PendingData.rows("hud.party", 4)
    activeMember: -1
    quests: PendingData.rows("hud.quests", 2)
    clock: PendingData.value("hud.clock")
    location: PendingData.value("hud.location")
    minimap: PendingData.image("hud.minimap")

    initiative: PendingData.rows("combat_hud.initiative", 6)
    activeIndex: -1
    combatLog: PendingData.rows("combat_hud.log", 5)
    actions: PendingData.rows("combat_hud.actions", 8)
    activeActionLabel: PendingData.value("combat_hud.action.current")
    targetName: PendingData.value("combat_hud.target.name")
    targetLevel: PendingData.value("combat_hud.target.level")
    targetHitPoints: PendingData.value("combat_hud.target.hit_points")
    targetHitPointsRatio: 0
    targetArmorClass: PendingData.value("combat_hud.target.armor_class")
    targetInitiative: PendingData.value("combat_hud.target.initiative")
    targetSpeed: PendingData.value("combat_hud.target.speed")
    targetConditions: PendingData.value("combat_hud.target.conditions")
    targetPortrait: PendingData.image("combat_hud.target.portrait")

    // La scene du combat : la meme surface de rendu que la vue de jeu, sous le cadre.
    GameViewport {
        parent: root.viewportHost
        anchors.fill: parent
        clearColor: Tokens.background
    }

    Keys.onPressed: (event) => {
        const slot = event.key - Qt.Key_1;
        if (slot >= 0 && slot < 8) {
            root.activeAction = slot;
            event.accepted = true;
        }
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
