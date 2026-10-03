import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Vue de jeu -- CABLAGE, cote developpeur (LOT-86, LOT-87 T4.1, LOT-09).

    L'ECRAN QUI SE PARCOURT. `WorldModel` tient la session d'exploration (`core::
    ExplorationSession`) et fait tourner son pas fixe ; `WorldViewport` dessine la carte courante
    par le meme pipeline QRhi et le meme composeur a calques que le Colisee en combat (LOT-86,
    LOT-92). Aucun second moteur de rendu.

    Le DEPLACEMENT est un etat de touches, pas une suite de pas : on cumule les touches enfoncees
    en une direction, et la session avance de son pas fixe a elle. Traduire chaque `onPressed` en
    un pas ferait dependre la vitesse du taux de repetition du clavier.

    Les valeurs du HUD qui ne viennent pas encore d'un lot (quetes, horloge, minicarte, experience)
    restent des donnees en attente (`PendingData`, cles `hud.*`), comme dans le cadre du LOT-87 ;
    le LIEU, lui, est celui de la carte chargee, et le GROUPE celui de la partie (LOT-138) : le
    meneur au portrait principal, les quatre membres dessous, le meneur marque.

    `Tab` passe la tete au suivant du groupe -- la figurine menee change sous la main --, `G` ouvre
    l'ecran du groupe.
*/
GameViewForm {
    id: root

    focus: true
    pending: !WorldModel.loaded
    // Rien a dire quand la carte est la : le statut ne parle que d'un echec.
    status: WorldModel.status

    // Le groupe de la partie (LOT-138) : les fiches se lisent une fois par ecran.
    readonly property PartyModel partyModel: PartyModel {}
    readonly property QuestJournalModel journal: QuestJournalModel {}
    readonly property var target: WorldModel.interactionTarget
    trackedObjective: journal.selected.length > 0 ? journal.detail : ""
    interactionText: root.target.prompt !== undefined ? qsTr("E — %1").arg(root.target.prompt) : ""
    interactionX: root.target.column !== undefined ? viewport.originX + (root.target.column - root.target.row) * viewport.tileWidth / 2 + viewport.tileWidth / 2 : 0
    interactionY: root.target.row !== undefined ? viewport.originY + (root.target.column + root.target.row) * viewport.tileHeight / 2 - viewport.tileHeight : 0
    onInteractRequested: WorldModel.interact()
    onObjectiveToggleRequested: root.objectiveExpanded = !root.objectiveExpanded
    onMemberClicked: (index) => {
        if (index < partyModel.members.length) WorldModel.setLeader(partyModel.members[index].id)
        root.forceActiveFocus()
    }

    characterName: partyModel.leaderName
    level: partyModel.leaderLevel
    hitPointsText: partyModel.leaderHitPoints
    hitPointsRatio: partyModel.members.length > 0 ? partyModel.members[0].ratio : 0
    experienceText: PendingData.value("hud.character.experience")
    experienceRatio: 0
    portrait: WorldModel.leaderPortrait
    party: partyModel.members
    activeMember: partyModel.members.length > 0 ? 0 : -1
    quests: PendingData.rows("hud.quests", 2)
    clock: ""
    location: WorldModel.loaded ? WorldModel.mapName : PendingData.value("hud.location")
    minimap: PendingData.image("hud.minimap")

    // La session d'exploration est un SINGLETON (`WorldModel`) : elle survit a l'ouverture du
    // dialogue et des ecrans RPG, que la pile d'ecrans construit a la place de cet ecran. Une
    // session possedee par l'ecran mourrait avec lui, et l'on reviendrait sur une carte neuve.
    Connections {
        target: WorldModel

        function onDialogueRequested(dialogueId) {
            // Le dialogue gele la carte : elle reste telle quelle, et l'on la reprendra ou on l'a
            // laissee -- c'est ce qui distingue un monde d'une suite de tableaux.
            WorldModel.frozen = true;
            ScreenRouter.openDialogue(dialogueId);
        }

        // Une entite `encounter` de la carte (LOT-118) : le combat se monte sur la zone de combat
        // et se joue par-dessus la carte gelee ; refuse, l'exploration continue.
        function onEncounterRequested(encounterId) {
            if (EncounterModel.begin(encounterId)) {
                ScreenRouter.openRpgScreen(ScreenRouter.CombatHud);
            }
        }

        // Le fondu du passage : a l'entree sur une carte, l'ecran revient de l'obscurite.
        function onMapEntered(mapId) { fondu.restart() }

        // Un portail qui ne s'ouvre pas le dit au joueur, un instant, par-dessus la vue : le
        // drapeau exige ou la carte visee ne le regardent pas, seul le refus compte.
        function onPortalLocked(flag) { root.notify(qsTr("Cette porte est fermée.")) }
        function onPortalBroken(mapId) { root.notify(qsTr("Ce passage est bloqué.")) }
        function onPortalSealed(mapId) { root.notify(qsTr("Ce passage est condamné.")) }
    }

    /// Pose `message` par-dessus la vue pour quelques secondes ; un nouveau message remplace le
    /// precedent et repart le compte.
    function notify(message) {
        notice.text = message;
        notice.visible = true;
        noticeTimer.restart();
    }

    // La partie ne recommence pas parce que l'ecran reparait : on revient du dialogue, du combat
    // ou de l'inventaire sur la carte qu'on a quittee.
    Component.onCompleted: {
        // « Nouvelle partie » : le joueur choisit d'abord son meneur, sur l'ecran Groupe
        // (LOT-142).
        if (!WorldModel.loaded && WorldModel.startNewGame() && WorldModel.choosingLeader)
            ScreenRouter.openRpgScreen(ScreenRouter.Party);
        // Et l'on revient de l'obscurite : au premier pas sur la carte comme au retour d'un
        // ecran, la carte se leve d'un fondu plutot que de paraitre d'un coup.
        fondu.restart();
    }

    // De retour du dialogue ou du combat : la carte reprend la ou elle s'etait arretee. Le gel
    // depend du focus et non d'un signal de fermeture : tout ce qui recouvre la vue de jeu --
    // dialogue, combat, pause, inventaire -- lui prend le focus, et un seul chemin vaut mieux
    // qu'un par ecran.
    onActiveFocusChanged: {
        if (root.activeFocus) {
            // Pendant un combat sur la carte, c'est le combat qui tient la carte gelee : entre la
            // fermeture du dialogue et l'ouverture de l'affichage de combat, la vue de jeu peut
            // reprendre le focus un instant sans que le heros doive repartir.
            WorldModel.frozen = EncounterModel.active;
        } else {
            held.up = false;
            held.down = false;
            held.left = false;
            held.right = false;
            root.pushMove();
        }
    }

    // La surface de rendu QRhi, posee dans l'hote que le formulaire reserve. Elle est ici et non
    // dans le formulaire parce que c'est un type C++ (`Jadg.Runtime`), invisible a l'atelier ; le
    // cadre du formulaire reste au-dessus d'elle, comme un enfant ordinaire.
    WorldViewport {
        id: viewport

        parent: root.viewportHost
        anchors.fill: parent
        model: WorldModel
        // Le vide autour du lieu n'est pas du parchemin : c'est la nuit hors des murs.
        clearColor: Tokens.panel
        // Les reglages de rendu des options, qui atteignent ainsi le moteur (EX-IHM-083).
        sampleCount: OptionsModel.antialiasing
        renderScalePercent: OptionsModel.renderScalePercent
        shadowSize: OptionsModel.shadows
    }

    // --- Le deplacement : un etat de touches, releve a chaque appui et a chaque relachement -----
    QtObject {
        id: held

        property bool up: false
        property bool down: false
        property bool left: false
        property bool right: false
    }

    /// Envoie a la session la direction que les touches enfoncees composent, normalisee : une
    /// diagonale ne doit pas aller plus vite qu'une ligne droite.
    function pushMove() {
        let x = (held.right ? 1 : 0) - (held.left ? 1 : 0);
        let y = (held.down ? 1 : 0) - (held.up ? 1 : 0);
        const length = Math.sqrt(x * x + y * y);
        if (length > 0) {
            x /= length;
            y /= length;
        }
        WorldModel.setMove(x, y);
    }

    Keys.onPressed: function (event) {
        switch (event.key) {
        case Qt.Key_Up: case Qt.Key_W: case Qt.Key_Z: held.up = true; break
        case Qt.Key_Down: case Qt.Key_S: held.down = true; break
        case Qt.Key_Left: case Qt.Key_A: case Qt.Key_Q: held.left = true; break
        case Qt.Key_Right: case Qt.Key_D: held.right = true; break
        case Qt.Key_E: case Qt.Key_Space: WorldModel.interact(); event.accepted = true; return
        case Qt.Key_Escape: ScreenRouter.openPause(); event.accepted = true; return
        // Le groupe (LOT-138) : passer la main au suivant, ou ouvrir l'ecran du groupe.
        case Qt.Key_Tab: WorldModel.rotateLeader(); event.accepted = true; return
        case Qt.Key_G: ScreenRouter.openRpgScreen(ScreenRouter.Party); event.accepted = true; return
        default: return
        }
        root.pushMove();
        event.accepted = true;
    }

    Keys.onReleased: function (event) {
        switch (event.key) {
        case Qt.Key_Up: case Qt.Key_W: case Qt.Key_Z: held.up = false; break
        case Qt.Key_Down: case Qt.Key_S: held.down = false; break
        case Qt.Key_Left: case Qt.Key_A: case Qt.Key_Q: held.left = false; break
        case Qt.Key_Right: case Qt.Key_D: held.right = false; break
        default: return
        }
        root.pushMove();
        event.accepted = true;
    }

    // Le fondu au noir du passage d'un portail et de l'entree sur une carte. Il vit ici, dans le
    // jumeau, et non dans le formulaire : c'est une transition de NAVIGATION, pas un ornement.
    Rectangle {
        id: voile

        parent: root.viewportHost
        anchors.fill: parent
        color: Tokens.panel
        opacity: 0

        NumberAnimation {
            id: fondu

            target: voile
            property: "opacity"
            from: 1
            to: 0
            duration: 320
        }
    }

    // Le mot d'un portail refuse (LOT-126) : une plaque sombre du HUD, INVISIBLE par defaut, que
    // `notify` montre et que le compte a rebours efface. Elle vit ici, dans le jumeau, parce
    // qu'elle depend d'un signal et d'un Timer -- ce qu'un formulaire ne porte pas.
    Rectangle {
        id: notice

        property alias text: noticeText.text

        parent: root.viewportHost
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.gapLarge * 4
        width: noticeText.implicitWidth + Tokens.gapLarge * 2
        height: noticeText.implicitHeight + Tokens.gapMedium * 2
        visible: false
        color: Tokens.panelRaised
        border.color: Tokens.panelEdge
        border.width: Tokens.strokeWidth
        opacity: 0.9

        Text {
            id: noticeText

            anchors.centerIn: parent
            color: Tokens.textOnPanel
            font.family: Tokens.loreFamily
            font.italic: true
            font.pixelSize: Tokens.fontBody
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Timer {
        id: noticeTimer

        interval: 2500
        onTriggered: notice.visible = false
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
        function onClicked() { ScreenRouter.openPause() }
    }
    Connections {
        target: root.characterButton
        function onClicked() { WorldModel.showCharacter(""); ScreenRouter.openCharacterTab(0) }
    }
    Connections {
        target: root.groupButton
        function onClicked() { ScreenRouter.openRpgScreen(ScreenRouter.Party) }
    }
}
