import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Menu principal -- CABLAGE, cote developpeur (LOT-86, LOT-87 T3.1).

    La navigation est REELLE : chaque entree appelle le routeur, qui suit la table de transitions
    pure. Un geste que la table n'autorise pas est refuse, jamais silencieusement accepte.

    Le CLAVIER et le POINTEUR pilotent le meme etat, `currentIndex` : survoler une entree la rend
    courante, et `Entree` active donc toujours celle que la marque designe. C'est la raison pour
    laquelle la souris ne s'active pas directement -- deux notions de « l'entree choisie » auraient
    fini par diverger, et l'ecran aurait ouvert autre chose que ce qu'il montrait.

    **Les entrees desactivees ne sont jamais courantes.** « Continuer » et « Charger une partie »
    attendent la sauvegarde (LOT-17) : les fleches les sautent, et le pointeur ne les designe pas
    (un bouton desactive n'emet ni survol ni clic). L'ecran s'ouvre donc sur « Nouvelle partie ».

    Le profil est une donnee en attente (`PendingData`, cles `main_menu.profile.*`) : aucun lot ne
    tient encore de profil de joueur.

    « Nouvelle partie » ouvre la VUE DE JEU sur la carte de depart de la ville (`WorldModel`,
    LOT-09) : le personnage parait a la porte, et l'on parcourt le lieu. Le combat ne se joue pas
    depuis le menu : c'est un PNJ de la carte qui y engage, et l'on en revient au meme endroit.
*/
MainMenuForm {
    id: root

    focus: true

    profileName: PendingData.value("main_menu.profile.name")
    profilePortrait: PendingData.image("main_menu.profile.portrait")
    // Posee par Main.cpp depuis core::Engine::version() : le numero n'existe qu'a un endroit.
    version: Qt.application.version

    /// Les entrees, dans l'ordre de l'ecran : l'indice est celui de `currentIndex`.
    readonly property var entries: [root.continueEntry, root.newGameEntry, root.loadGameEntry,
                                    root.optionsEntry, root.creditsEntry, root.quitEntry]

    Keys.onUpPressed: root.step(-1)
    Keys.onDownPressed: root.step(1)
    Keys.onReturnPressed: root.activate()
    Keys.onEnterPressed: root.activate()

    /// Avance d'une entree ACTIVABLE dans le sens `delta`, en bouclant. Il y en a toujours au
    /// moins une (Quitter), donc la boucle termine.
    function step(delta) {
        let index = root.currentIndex;
        do {
            index = (index + delta + root.entries.length) % root.entries.length;
        } while (!root.entries[index].enabled);
        root.currentIndex = index;
    }

    /// Le pointeur DESIGNE une entree, sans l'ouvrir : passer la souris en travers du menu ne doit
    /// rien declencher.
    function point(index) {
        if (root.entries[index].enabled)
            root.currentIndex = index;
    }

    /// Le pointeur CHOISIT : on designe d'abord, on ouvre ensuite, par le meme chemin que le
    /// clavier. Un clic ne peut ainsi jamais ouvrir une entree que l'ecran ne montrait pas.
    function choose(index) {
        root.point(index);
        if (root.currentIndex === index)
            root.activate();
    }

    function activate() {
        if (!root.entries[root.currentIndex].enabled)
            return;
        switch (root.currentIndex) {
        case 1: ScreenRouter.openGame(); break
        case 3: ScreenRouter.openOptions(); break
        case 4: ScreenRouter.openCredits(); break
        case 5: Qt.quit(); break
        }
    }

    Connections {
        target: root.continueEntry
        function onHoveredChanged() { if (root.continueEntry.hovered) root.point(0) }
        function onClicked() { root.choose(0) }
    }
    Connections {
        target: root.newGameEntry
        function onHoveredChanged() { if (root.newGameEntry.hovered) root.point(1) }
        function onClicked() { root.choose(1) }
    }
    Connections {
        target: root.loadGameEntry
        function onHoveredChanged() { if (root.loadGameEntry.hovered) root.point(2) }
        function onClicked() { root.choose(2) }
    }
    Connections {
        target: root.optionsEntry
        function onHoveredChanged() { if (root.optionsEntry.hovered) root.point(3) }
        function onClicked() { root.choose(3) }
    }
    Connections {
        target: root.creditsEntry
        function onHoveredChanged() { if (root.creditsEntry.hovered) root.point(4) }
        function onClicked() { root.choose(4) }
    }
    Connections {
        target: root.quitEntry
        function onHoveredChanged() { if (root.quitEntry.hovered) root.point(5) }
        function onClicked() { root.choose(5) }
    }
}
