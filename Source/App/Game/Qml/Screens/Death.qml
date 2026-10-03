import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Ecran de mort -- CABLAGE, cote developpeur (LOT-119).

    Il s'ouvre quand le combat sur la carte s'acheve sur une defaite (`CombatHud.qml`) : le combat
    n'est pas encore quitte, si bien que la surface de rendu de la carte dessine toujours la scene
    ou le heros est tombe, figee, sous le voile du formulaire.

    Les deux issues ferment la partie : la rencontre est quittee, la session remise a zero
    (`WorldModel.endGame`), puis « Recommencer » rouvre une partie neuve a la porte de la ville et
    « Menu » rend le menu -- d'ou « Nouvelle partie » repartira de zero, elle aussi.

    Fleches gauche/droite pour choisir, Entree pour valider, Echap pour le menu.
*/
DeathForm {
    id: root

    focus: true

    // La scene du combat, gelee : la meme surface que la vue de jeu, sur la meme carte.
    WorldViewport {
        parent: root.sceneHost
        anchors.fill: parent
        model: WorldModel
        clearColor: Tokens.panel
        // Les reglages de rendu des options, qui atteignent ainsi le moteur (EX-IHM-083).
        sampleCount: OptionsModel.antialiasing
        renderScalePercent: OptionsModel.renderScalePercent
        shadowSize: OptionsModel.shadows
    }

    Keys.onLeftPressed: root.currentIndex = 0
    Keys.onRightPressed: root.currentIndex = 1
    Keys.onReturnPressed: root.activate()
    Keys.onEnterPressed: root.activate()
    Keys.onEscapePressed: root.choose(1)

    function point(index) {
        root.currentIndex = index;
    }

    function choose(index) {
        root.currentIndex = index;
        root.activate();
    }

    function activate() {
        EncounterModel.leave();
        WorldModel.endGame();
        if (root.currentIndex === 0) {
            ScreenRouter.openGame();
        } else {
            ScreenRouter.openMenu();
        }
    }

    Connections {
        target: root.restartEntry
        function onHoveredChanged() { if (root.restartEntry.hovered) root.point(0) }
        function onClicked() { root.choose(0) }
    }
    Connections {
        target: root.menuEntry
        function onHoveredChanged() { if (root.menuEntry.hovered) root.point(1) }
        function onClicked() { root.choose(1) }
    }
}
