import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Fin de la demo -- CABLAGE, cote developpeur (LOT-119).

    Il s'ouvre quand un dialogue clot la demo (action `endDemo`, `Dialogue.qml`) : la voie suivie
    est celle que le dialogue a nommee, transportee par le routeur (`ScreenRouter.endingText`).

    La partie est finie : a la construction de l'ecran, la session est remise a zero
    (`WorldModel.endGame`), si bien que « Nouvelle partie » repart de la porte de la ville. Les
    credits se referment sur le menu.

    | Geste | Clavier | Manette |
    |---|---|---|
    | Changer de bouton | fleches gauche, droite | croix ou stick |
    | Valider | Entree | A |
    | Menu | Echap | B |
*/
DemoEndForm {
    id: root

    focus: true
    path: ScreenRouter.endingText

    Component.onCompleted: WorldModel.endGame()

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
        if (root.currentIndex === 0) {
            ScreenRouter.openCredits();
        } else {
            ScreenRouter.openMenu();
        }
    }

    GamepadNavigator {
        active: root.visible
        onPressed: (button) => {
            switch (button) {
            case "left": root.currentIndex = 0; break
            case "right": root.currentIndex = 1; break
            case "a": root.activate(); break
            case "b": root.choose(1); break
            }
        }
    }

    Connections {
        target: root.creditsEntry
        function onHoveredChanged() { if (root.creditsEntry.hovered) root.point(0) }
        function onClicked() { root.choose(0) }
    }
    Connections {
        target: root.menuEntry
        function onHoveredChanged() { if (root.menuEntry.hovered) root.point(1) }
        function onClicked() { root.choose(1) }
    }
}
