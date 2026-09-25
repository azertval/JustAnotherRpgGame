import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Jadg.Runtime

/*!
    Le menu de développement -- un OUTIL DE DEBUG, côté développeur, ouvert et fermé par F9.

    Un panneau posé par-dessus l'écran courant, qui réunit ce qu'on faisait jusque-là par la ligne
    de commande ou par le sélecteur d'écrans : ouvrir un écran par son nom, entrer sur une carte à
    un point d'arrivée, engager une rencontre ou le Colisée, geler la carte, montrer le compteur de
    diagnostic, écrire les journaux de la session. Il ne fait rien que les vues-modèles ne sachent
    déjà faire : chaque bouton appelle un invocable de `Jadg.Runtime`, et le menu n'a pas d'état
    propre au-delà de ce qu'on tape dans ses champs.

    **Ce n'est pas une fonctionnalité, et le code le garantit** : `visible` se lie à
    `ScreenRouter.developerBuild`, faux dans un binaire livré, et le raccourci qui l'ouvre
    (`ScreenStack.qml`) est désactivé par la même propriété.

    Il vit dans Tools/ et non dans Screens/ : sans formulaire dans Source/Ui, l'atelier n'a rien à
    y dessiner. C'est une interface de développeur, en contrôles Qt Quick ordinaires, qui emprunte
    la palette ambiante comme le sélecteur d'écrans -- aucune couleur n'y est écrite, et ce qui la
    distingue du jeu n'est pas sa teinte mais son libellé.

    **Il prend le clavier** tant qu'il est ouvert -- ses champs se remplissent --, et la vue de jeu,
    qui perd alors le focus, relâche les directions tenues : le héros s'arrête, la carte ne gèle
    pas (un interrupteur le fait à la demande). Échap ou F9 referment le menu, et la pile rend le
    clavier à l'écran.

    | Geste | Clavier |
    |---|---|
    | Ouvrir, fermer | F9 |
    | Fermer | Échap |
    | Valider le champ courant (carte, rencontre) | Entrée |
*/
Item {
    id: root

    /// Les écrans que le menu propose, dans l'ordre du sélecteur (`ScreenStack.screenNames`).
    required property var screenNames

    /// Vrai quand le panneau est à l'écran. Ne s'ouvre jamais dans un binaire livré.
    property bool open: false

    /// Un écran a été choisi par son nom : c'est la pile qui l'ouvre, par le sélecteur.
    signal screenChosen(string name)

    /// Le menu vient de se fermer : la pile rend le clavier à l'écran courant.
    signal closed()

    /// Ce que la dernière commande a répondu, affiché en pied de panneau.
    property string feedback: ""

    readonly property int panelWidth: 340

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
        if (ScreenRouter.currentScreen !== ScreenRouter.Game) {
            root.screenChosen("GameView");
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

    // Le panneau, à droite : le haut porte le bandeau de titre des écrans, le bas le sélecteur
    // d'écrans, et une colonne à droite laisse la scène visible pendant qu'on la commande.
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
                            root.feedback = "Écran « " + screenBox.currentText + " » ouvert par le sélecteur.";
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

                Switch {
                    id: frozenSwitch

                    text: "Carte gelée"
                    checked: WorldModel.frozen
                    onToggled: WorldModel.frozen = frozenSwitch.checked
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

                Button {
                    text: "Colisée"
                    onClicked: {
                        ScreenRouter.openArena();
                        root.feedback = "Colisée demandé au routeur.";
                        root.close();
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

                // --- Aide-mémoire ---------------------------------------------------------
                Label {
                    Layout.topMargin: 8
                    text: "Ligne de commande"
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    text: "--screen=<Nom>  --map=<carte>[@<arrivée>]  --at=<c>,<l>  --flags=<a>,<b>\n"
                          + "--levels=<dossier>;…  --data=<racine>  --hero-figure=<dossier>\n"
                          + "--window-size=<L>x<H>  --screenshot=<fichier>  --log-level=<niveau>\n"
                          + "Le guide « Outils de développement du jeu » les détaille."
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
