import QtQuick
import Jadg.Runtime
// Qualifié : `Jadg.Ui` et `Jadg.App` exportent tous deux un type `Main` (la galerie de l'atelier et
// la fenêtre du jeu). Sans préfixe, l'un masquerait l'autre selon l'ordre des imports.
import Jadg.Ui as Ui
// Les écrans vivent dans Screens/, un sous-dossier de ce module : l'import explicite les rend
// visibles d'ici, ce que l'import implicite du répertoire courant ne ferait pas.
import Jadg.App

/*!
    La pile d'écrans -- côté DÉVELOPPEUR (LOT-86).

    Elle traduit l'**état** publié par `ScreenRouter` en écran affiché. C'est le seul endroit du
    projet où cette correspondance existe, et elle vit ici plutôt qu'en C++ pour une raison
    précise : la présentation publie un état, jamais un chemin de fichier. La conception peut ainsi
    réorganiser `Screens/` sans qu'une ligne de C++ ne s'en aperçoive.

    **Des composants typés et non des chemins construits.** Assembler « ../Screens/ » + un nom
    produisait une URL que la ressource ne résolvait pas depuis un sous-répertoire — un écran
    introuvable, à l'exécution seulement. Les types, eux, sont vérifiés par `qmllint` : un écran
    renommé fait échouer le contrôle, pas le jeu.

    Chaque écran est enveloppé dans un `Component` : il n'est construit qu'une fois choisi. Les
    quinze écrans ne vivent jamais tous en même temps.

    `--screen=<Nom>` court-circuite le routeur. C'est un outil de vérification, pas un chemin de
    jeu : les écrans dessinés mais pas encore alimentés ne sont atteignables par aucun autre moyen.
*/
Item {
    id: root

    /// Écran imposé au lancement, ou chaîne vide pour laisser le routeur décider.
    property string forcedScreen: ""

    /// Les quinze écrans, dans l'ordre où le sélecteur de développement les fait défiler, puis
    /// la galerie des briques de la charte v2 (LOT-87) -- qui n'est pas un écran du jeu, et que le
    /// routeur ne désigne jamais.
    /// Le même vocabulaire que `--screen=` : deux listes différentes auraient fini par diverger,
    /// et un écran serait devenu joignable par un chemin et pas par l'autre.
    readonly property var screenNames: [
        "MainMenu", "GameView", "Pause", "Options", "Credits",
        "CharacterSheet", "Skills", "Inventory", "Journal", "WorldMap", "Dialogue",
        "Merchant", "Company", "CombatHud", "Arena", "Death", "DemoEnd", "Gallery", "AssetGallery"
    ]

    Component { id: menuScreen; MainMenu {} }
    Component { id: optionsScreen; Options {} }
    Component { id: creditsScreen; Credits {} }
    Component { id: pauseScreen; Pause {} }
    Component { id: gameScreen; GameView {} }
    Component { id: characterSheetScreen; CharacterSheet {} }
    Component { id: skillsScreen; Skills {} }
    Component { id: inventoryScreen; Inventory {} }
    Component { id: journalScreen; Journal {} }
    Component { id: worldMapScreen; WorldMap {} }
    Component { id: dialogueScreen; Dialogue {} }
    Component { id: merchantScreen; Merchant {} }
    Component { id: companyScreen; Company {} }
    Component { id: combatHudScreen; CombatHud {} }
    Component { id: arenaScreen; Arena {} }
    // Les ecrans de fin (LOT-119).
    Component { id: deathScreen; Death {} }
    Component { id: demoEndScreen; DemoEnd {} }
    // La galerie est le point d'entrée de l'atelier (`DesignStudio/Main.ui.qml`), posée telle quelle :
    // aucun jumeau, rien à câbler -- c'est ce qui prouve que les briques se résolvent au jeu comme à
    // l'atelier.
    Component { id: galleryScreen; Ui.Main {} }
    // La galerie des ASSETS : un outil de debug, lui non plus pas un ecran du jeu.
    Component { id: assetGalleryScreen; AssetGallery {} }

    Loader {
        id: stack

        anchors.fill: parent
        focus: true
        // Trois sources, dans cet ordre : le sélecteur de développement s'il a servi, puis
        // `--screen=`, puis le routeur. Le sélecteur passe DEVANT `--screen=` : sans cela,
        // ouvrir le jeu sur un écran précis aurait figé le sélecteur sur ce même écran.
        sourceComponent: probe.selectedScreen.length > 0
                         ? root.byName(probe.selectedScreen)
                         : (root.forcedScreen.length > 0 ? root.byName(root.forcedScreen)
                                                         : root.byState())
    }

    /*!
        Le sélecteur d'écrans de développement. Absent des binaires livrés -- il se lie lui-même à
        `ScreenRouter.developerBuild`.

        Il est posé APRÈS le `Loader`, donc au-dessus : c'est un recouvrement, et il doit le rester
        quel que soit l'écran regardé.
    */
    ScreenProbe {
        id: probe

        anchors.fill: parent
        screenNames: root.screenNames
        // Reprend là où `--screen=` a ouvert : sans cela, le premier clic sur ▶ aurait ramené au
        // menu depuis n'importe quel écran, au lieu de continuer la liste.
        Component.onCompleted: {
            if (root.forcedScreen.length > 0) {
                probe.index = root.screenNames.indexOf(root.forcedScreen);
                probe.selectedScreen = root.forcedScreen;
            }
        }
    }

    /*!
        Le menu de développement, ouvert par F9. Absent des binaires livrés, comme le sélecteur :
        il se lie lui-même à `ScreenRouter.developerBuild`, et le raccourci aussi.

        Posé APRÈS le sélecteur, donc au-dessus de tout : c'est un recouvrement qui commande
        l'écran, il ne doit être masqué par aucun.
    */
    DevMenu {
        id: devMenu

        anchors.fill: parent
        screenNames: root.screenNames
        // Un écran choisi au menu passe par le sélecteur : un seul chemin pour épingler un écran,
        // et un seul endroit où le routeur le désépingle.
        onScreenChosen: function (name) { probe.select(name) }
        // Le menu a pris le clavier ; fermé, il le rend à l'écran courant. Sans cela, plus aucune
        // touche n'atteignait l'écran jusqu'au prochain clic.
        onClosed: stack.forceActiveFocus()
    }

    // F9 partout, quel que soit l'écran qui a le clavier : un raccourci d'application passe avant
    // les `Keys` des écrans. Désactivé dans un binaire livré, où le menu n'existe pas.
    Shortcut {
        sequence: "F9"
        context: Qt.ApplicationShortcut
        enabled: ScreenRouter.developerBuild
        onActivated: devMenu.toggle()
    }

    // Le routeur reprend la main dès que le jeu navigue de lui-même. Sans cela, un écran choisi
    // dans le sélecteur restait épinglé : `Échap` ne fermait plus rien, et la navigation -- ce
    // qu'on cherche justement à vérifier -- aurait paru cassée par l'outil de vérification.
    Connections {
        target: ScreenRouter

        function onChanged() {
            probe.clear();
        }
    }

    /// L'écran que le routeur désigne.
    function byState() {
        switch (ScreenRouter.currentScreen) {
        case ScreenRouter.Menu:      return menuScreen
        case ScreenRouter.Options:   return optionsScreen
        case ScreenRouter.Credits:   return creditsScreen
        case ScreenRouter.Pause:     return pauseScreen
        case ScreenRouter.RpgScreen: return root.rpgScreen(ScreenRouter.currentRpgScreen)
        case ScreenRouter.Game:      return gameScreen
        case ScreenRouter.Arena:     return arenaScreen
        case ScreenRouter.Death:     return deathScreen
        case ScreenRouter.DemoEnd:   return demoEndScreen
        }
        return menuScreen
    }

    function rpgScreen(screen) {
        switch (screen) {
        case ScreenRouter.CharacterSheet: return characterSheetScreen
        case ScreenRouter.Skills:         return skillsScreen
        case ScreenRouter.Inventory:      return inventoryScreen
        case ScreenRouter.QuestJournal:   return journalScreen
        case ScreenRouter.WorldMap:       return worldMapScreen
        case ScreenRouter.Dialogue:       return dialogueScreen
        case ScreenRouter.Merchant:       return merchantScreen
        case ScreenRouter.Company:        return companyScreen
        case ScreenRouter.CombatHud:      return combatHudScreen
        }
        return characterSheetScreen
    }

    /// Résolution par nom, pour `--screen=`. Une table explicite : dériver le composant d'une
    /// chaîne par convention aurait marché sur douze écrans et cassé sur le treizième.
    function byName(name) {
        switch (name) {
        case "MainMenu":       return menuScreen
        case "Options":        return optionsScreen
        case "Credits":        return creditsScreen
        case "Pause":          return pauseScreen
        case "GameView":       return gameScreen
        case "CharacterSheet": return characterSheetScreen
        case "Skills":         return skillsScreen
        case "Inventory":      return inventoryScreen
        case "Journal":        return journalScreen
        case "WorldMap":       return worldMapScreen
        case "Dialogue":       return dialogueScreen
        case "Merchant":       return merchantScreen
        case "Company":        return companyScreen
        case "CombatHud":      return combatHudScreen
        case "Arena":          return arenaScreen
        case "Death":          return deathScreen
        case "DemoEnd":        return demoEndScreen
        case "Gallery":        return galleryScreen
        case "AssetGallery":   return assetGalleryScreen
        }
        return menuScreen
    }
}
