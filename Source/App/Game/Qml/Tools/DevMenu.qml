import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Jadg.Runtime

/*!
    Le menu de développement -- un OUTIL DE DEBUG, côté développeur, ouvert et fermé par F9.

    Un panneau posé par-dessus l'écran courant, qui réunit ce qu'on faisait jusque-là par la ligne
    de commande : ouvrir un écran par son nom (il remplace le sélecteur d'écrans du bas de la
    fenêtre), entrer sur une carte à un point d'arrivée ou la choisir dans le lanceur de cartes,
    ouvrir un dialogue, engager une rencontre, ouvrir les écrans de fin (LOT-119), geler la carte,
    montrer le compteur de diagnostic, écrire les journaux de la session -- et, dans la section
    « Ligne de commande », taper les options du binaire, appliquées à chaud (`DebugConsoleModel`,
    catalogue `hmi::debugOptionCatalog`). Il ne fait rien que les vues-modèles ne sachent déjà
    faire : chaque bouton appelle un invocable de `Jadg.Runtime`.

    **Ce n'est pas une fonctionnalité, et le code le garantit** : `visible` se lie à
    `ScreenRouter.developerBuild`, faux dans un binaire livré, et le raccourci qui l'ouvre
    (`ScreenStack.qml`) est désactivé par la même propriété.

    Il vit dans Tools/ et non dans Screens/ : sans formulaire dans Source/Ui, l'atelier n'a rien à
    y dessiner. C'est une interface de développeur, en contrôles Qt Quick ordinaires, qui emprunte
    la palette ambiante comme tout contrôle Qt -- aucune couleur n'y est écrite, et ce qui la
    distingue du jeu n'est pas sa teinte mais son libellé.

    **Il prend le clavier** tant qu'il est ouvert -- ses champs se remplissent --, et la vue de jeu,
    qui perd alors le focus, relâche les directions tenues : le héros s'arrête, la carte ne gèle
    pas (un interrupteur le fait à la demande). Échap ou F9 referment le menu, et la pile rend le
    clavier à l'écran.

    | Geste | Clavier |
    |---|---|
    | Ouvrir, fermer | F9 |
    | Fermer | Échap |
    | Valider le champ courant (carte, dialogue, rencontre, ligne de commande) | Entrée |
    | Rappeler une ligne de commande | flèches haut, bas (dans le champ) |
*/
Item {
    id: root

    /// Les écrans que le menu propose, dans l'ordre de la pile (`ScreenStack.screenNames`).
    required property var screenNames

    /// Vrai quand le panneau est à l'écran. Ne s'ouvre jamais dans un binaire livré.
    property bool open: false

    /// Ce que `--screenshot=` capture : la pile d'écrans, sans ce menu.
    property Item captureTarget: null

    /// Un écran a été choisi par son nom : c'est la pile qui l'épingle.
    signal screenChosen(string name)

    /// `--window-size=` : c'est la pile qui tient la fenêtre.
    signal windowSizeChosen(int width, int height)

    /// Le menu vient de se fermer : la pile rend le clavier à l'écran courant.
    signal closed()

    /// Ce que la dernière commande a répondu, affiché en pied de panneau.
    property string feedback: ""

    readonly property int panelWidth: 400

    visible: root.open && ScreenRouter.developerBuild

    /// Ouvre ou ferme le menu. Sans effet hors d'un build de développement.
    function toggle() {
        if (!ScreenRouter.developerBuild) {
            return;
        }
        if (root.open) {
            root.close();
        } else {
            root.open = true;
            panel.forceActiveFocus();
        }
    }

    /// Ferme le menu et rend le clavier.
    function close() {
        if (!root.open) {
            return;
        }
        root.open = false;
        root.closed();
    }

    /// Entre sur la carte tapée, au point d'arrivée tapé ; ouvre la vue de jeu si elle ne l'est pas.
    function enterMap() {
        const mapId = mapField.text.trim();
        if (mapId.length === 0) {
            root.feedback = "Carte : identifiant vide.";
            return;
        }
        if (!WorldModel.enterMap(mapId, arrivalField.text.trim())) {
            root.feedback = WorldModel.status.length > 0 ? WorldModel.status
                                                          : "Carte « " + mapId + " » : refusée.";
            return;
        }
        root.feedback = "Sur « " + WorldModel.mapName + " » (" + WorldModel.mapId + ").";
        ScreenRouter.jumpToGame();
    }

    /// Ouvre le dialogue choisi, comme un PNJ l'ouvrirait.
    function openDialogue() {
        const dialogueId = dialogueBox.editText.trim();
        if (dialogueId.length === 0) {
            root.feedback = "Dialogue : identifiant vide.";
            return;
        }
        ScreenRouter.openDialogue(dialogueId);
        root.feedback = "Dialogue « " + dialogueId + " » ouvert.";
        root.close();
    }

    /// L'écran de mort (LOT-119), par-dessus la partie ; depuis n'importe quel écran.
    function openDeath() {
        ScreenRouter.jumpToGame();
        ScreenRouter.openDeath();
        root.feedback = "Écran de mort ouvert.";
        root.close();
    }

    /// L'écran « Fin de la démo » (LOT-119), par la voie choisie.
    function openDemoEnd() {
        ScreenRouter.jumpToGame();
        ScreenRouter.openDemoEnd(endingBox.currentText);
        root.feedback = "Fin de la démo : « " + endingBox.currentText + " ».";
        root.close();
    }

    /// Exécute la ligne tapée, comme la ligne de commande du binaire.
    function runCommand() {
        DebugConsoleModel.run(commandField.text);
        commandField.text = "";
        root.historyIndex = DebugConsoleModel.history.length;
    }

    /// Rappelle la ligne précédente (`delta` < 0) ou suivante de l'historique.
    function recall(delta) {
        const history = DebugConsoleModel.history;
        const next = Math.max(0, Math.min(history.length, root.historyIndex + delta));
        root.historyIndex = next;
        commandField.text = next < history.length ? history[next] : "";
        commandField.cursorPosition = commandField.text.length;
    }

    /// Rang dans l'historique pendant qu'on le parcourt ; `history.length` = la ligne en cours.
    property int historyIndex: DebugConsoleModel.history.length

    /// La liste des dialogues du contenu, lue une fois.
    readonly property DialogueModel dialogueCatalog: DialogueModel {}

    // Ce que la ligne de commande demande à la fenêtre et au routeur : le modèle ne les connaît pas.
    Connections {
        target: DebugConsoleModel

        function onScreenRequested(name) {
            root.screenChosen(name);
        }

        function onWindowSizeRequested(width, height) {
            if (OptionsModel.fullscreen) {
                DebugConsoleModel.say("Fenetre en plein ecran : la taille ne s'applique pas (Options).");
                return;
            }
            root.windowSizeChosen(width, height);
        }

        function onScreenshotRequested(path) {
            if (root.captureTarget === null
                    || !root.captureTarget.grabToImage(function (result) {
                        const saved = result.saveToFile(path);
                        DebugConsoleModel.say((saved ? "Capture ecrite : " : "Echec de la capture : ") + path);
                    })) {
                DebugConsoleModel.say("Capture impossible : la scene n'est pas rendue.");
            }
        }

        function onGameRequested() {
            ScreenRouter.jumpToGame();
        }
    }

    /// Engage la rencontre tapée sur la carte courante, et ouvre l'affichage de combat.
    function beginEncounter() {
        const encounterId = encounterField.text.trim();
        if (encounterId.length === 0) {
            root.feedback = "Rencontre : identifiant vide.";
            return;
        }
        if (!WorldModel.loaded) {
            root.feedback = "Rencontre : aucune carte chargée, entrer d'abord sur une carte.";
            return;
        }
        if (!EncounterModel.begin(encounterId)) {
            root.feedback = "Rencontre « " + encounterId + " » : refusée (voir le journal).";
            return;
        }
        ScreenRouter.openRpgScreen(ScreenRouter.CombatHud);
        root.feedback = "Rencontre « " + EncounterModel.encounterName + " » engagée.";
        root.close();
    }

    // Le panneau, à droite : une colonne laisse la scène visible pendant qu'on la commande.
    Rectangle {
        id: panel

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: root.panelWidth
        color: root.palette.window
        border.color: root.palette.mid
        border.width: 1
        focus: true

        Keys.onEscapePressed: root.close()

        // Un clic sur le panneau ne doit pas atteindre l'écran qu'il recouvre.
        MouseArea {
            anchors.fill: parent
            onClicked: panel.forceActiveFocus()
        }

        ScrollView {
            id: scroll

            anchors.fill: parent
            anchors.margins: 8
            contentWidth: scroll.availableWidth
            clip: true

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 6

                Label {
                    Layout.fillWidth: true
                    text: "Menu de développement · F9"
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: "Version " + Qt.application.version + " · Échap ferme. Un binaire livré n'a pas ce menu."
                    opacity: 0.7
                }

                // --- Écrans ---------------------------------------------------------------
                Label {
                    Layout.topMargin: 8
                    text: "Écrans"
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    ComboBox {
                        id: screenBox

                        Layout.fillWidth: true
                        model: root.screenNames
                    }

                    Button {
                        text: "Ouvrir"
                        onClicked: {
                            root.screenChosen(screenBox.currentText);
                            root.feedback = "Écran « " + screenBox.currentText + " » épinglé.";
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    text: "Épinglé jusqu'à ce que le jeu navigue de lui-même, comme --screen=."
                }

                // --- Carte ----------------------------------------------------------------
                Label {
                    Layout.topMargin: 8
                    text: "Carte"
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: WorldModel.loaded
                          ? WorldModel.mapId + " · " + WorldModel.mapName
                            + " · héros (" + Math.floor(WorldModel.heroColumn) + ", "
                            + Math.floor(WorldModel.heroRow) + ")"
                            + (WorldModel.districtId.length > 0 ? " · " + WorldModel.districtId : "")
                          : (WorldModel.status.length > 0 ? WorldModel.status : "Aucune carte chargée.")
                }

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: mapField

                        Layout.fillWidth: true
                        placeholderText: "carte (ex. donjon)"
                        onAccepted: root.enterMap()
                    }

                    TextField {
                        id: arrivalField

                        Layout.preferredWidth: 100
                        placeholderText: "arrivée"
                        onAccepted: root.enterMap()
                    }

                    Button {
                        text: "Entrer"
                        onClicked: root.enterMap()
                    }
                }

                Button {
                    text: "Lanceur de cartes…"
                    onClicked: {
                        root.screenChosen("MapLauncher");
                        root.close();
                    }
                }

                Switch {
                    id: frozenSwitch

                    text: "Carte gelée"
                    checked: WorldModel.frozen
                    onToggled: WorldModel.frozen = frozenSwitch.checked
                }

                // --- Dialogue -------------------------------------------------------------
                Label {
                    Layout.topMargin: 8
                    text: "Dialogue"
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    ComboBox {
                        id: dialogueBox

                        Layout.fillWidth: true
                        editable: true
                        model: root.dialogueCatalog.dialogueIds
                        onAccepted: root.openDialogue()
                    }

                    Button {
                        text: "Ouvrir"
                        onClicked: root.openDialogue()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    text: "Les dialogues du contenu (World/dialogues) ; les drapeaux sont ceux de la partie."
                }

                // --- Combat ---------------------------------------------------------------
                Label {
                    Layout.topMargin: 8
                    text: "Combat"
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: EncounterModel.active
                          ? "Rencontre en cours : " + EncounterModel.encounterName
                            + (EncounterModel.outcome.length > 0 ? " · " + EncounterModel.outcome : "")
                          : "Aucune rencontre en cours."
                }

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: encounterField

                        Layout.fillWidth: true
                        placeholderText: "rencontre (ex. rats-du-donjon)"
                        onAccepted: root.beginEncounter()
                    }

                    Button {
                        text: "Engager"
                        onClicked: root.beginEncounter()
                    }
                }

                // --- Écrans de fin (LOT-119) ------------------------------------------------
                Label {
                    Layout.topMargin: 8
                    text: "Fins"
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: "Mort"
                        onClicked: root.openDeath()
                    }

                    ComboBox {
                        id: endingBox

                        Layout.fillWidth: true
                        model: ["arene", "parole"]
                    }

                    Button {
                        text: "Fin de la démo"
                        onClicked: root.openDemoEnd()
                    }
                }

                // --- Affichage et journaux --------------------------------------------------
                Label {
                    Layout.topMargin: 8
                    text: "Affichage et journaux"
                    font.bold: true
                }

                Switch {
                    id: diagnosticsSwitch

                    text: "Compteur de diagnostic"
                    checked: OptionsModel.diagnostics
                    // Le même réglage que l'onglet Graphismes des options : il est retenu d'un
                    // lancement à l'autre, comme si on l'avait coché là.
                    onToggled: OptionsModel.diagnostics = diagnosticsSwitch.checked
                }

                Button {
                    text: "Enregistrer les journaux de session"
                    onClicked: root.feedback = OptionsModel.saveLogs()
                }

                // --- Ligne de commande ------------------------------------------------------
                Label {
                    Layout.topMargin: 8
                    text: "Ligne de commande"
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    text: "Les options du binaire, appliquées à chaud : --map=, --at=, --flags=, "
                          + "--screen=, --window-size=, --screenshot=… « aide » les liste."
                }

                TextField {
                    id: commandField

                    Layout.fillWidth: true
                    placeholderText: "--map=donjon@sable --at=24,19"
                    onAccepted: root.runCommand()
                    Keys.onUpPressed: root.recall(-1)
                    Keys.onDownPressed: root.recall(1)
                }

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: "Exécuter"
                        onClicked: root.runCommand()
                    }

                    Button {
                        text: "Aide"
                        onClicked: DebugConsoleModel.run("aide")
                    }

                    Button {
                        // --data=, --crash-test, --map-* ne se lisent qu'au lancement.
                        text: "Relancer avec"
                        onClicked: DebugConsoleModel.relaunch(commandField.text)
                    }

                    Button {
                        text: "Effacer"
                        onClicked: DebugConsoleModel.clear()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    visible: DebugConsoleModel.transcript.length > 0
                    // Les dernières lignes seulement : le panneau n'est pas une console plein écran.
                    text: DebugConsoleModel.transcript.slice(-12).join("\n")
                    background: Rectangle {
                        color: root.palette.base
                        border.color: root.palette.mid
                        border.width: 1
                    }
                    padding: 4
                }

                // --- Retour de la dernière commande -----------------------------------------
                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    wrapMode: Text.Wrap
                    visible: root.feedback.length > 0
                    text: root.feedback
                    background: Rectangle {
                        color: root.palette.base
                        border.color: root.palette.mid
                        border.width: 1
                    }
                    padding: 4
                }
            }
        }
    }
}
