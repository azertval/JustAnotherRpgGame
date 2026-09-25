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

    Les valeurs du HUD qui ne viennent pas encore d'un lot (groupe, quetes, horloge, minicarte)
    restent des donnees en attente (`PendingData`, cles `hud.*`), comme dans le cadre du LOT-87 ;
    le LIEU, lui, est celui de la carte chargee.
*/
GameViewForm {
    id: root

    focus: true
    pending: !WorldModel.loaded
    // Rien a dire quand la carte est la : le statut ne parle que d'un echec.
    status: WorldModel.status

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
    }

    // La partie ne recommence pas parce que l'ecran reparait : on revient du dialogue, du combat
    // ou de l'inventaire sur la carte qu'on a quittee.
    Component.onCompleted: {
        if (!WorldModel.loaded)
            WorldModel.startNewGame();
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
