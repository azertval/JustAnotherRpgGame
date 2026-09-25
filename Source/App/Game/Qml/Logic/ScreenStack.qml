pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
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

    `--screen=<Nom>` et le menu de développement (F9) **épinglent** un écran par son nom, par-dessus
    le routeur (`pinnedScreen`). Ce sont des outils de vérification, pas des chemins de jeu : les
    écrans dessinés mais pas encore alimentés ne sont atteignables par aucun autre moyen. L'écran
    épinglé cède la main dès que le jeu navigue de lui-même.
*/
Item {
    id: root

    /// Écran imposé au lancement (`--screen=`), ou chaîne vide pour laisser le routeur décider.
    property string forcedScreen: ""

    /// L'écran épinglé par `--screen=` ou le menu F9, vide tant que le routeur décide.
    property string pinnedScreen: root.forcedScreen

    /// Les écrans, dans l'ordre du menu de développement, puis la galerie des briques de la
    /// charte v2 (LOT-87) et les outils de debug -- que le routeur ne désigne jamais.
    /// Le même vocabulaire que `--screen=` : deux listes différentes auraient fini par diverger,
    /// et un écran serait devenu joignable par un chemin et pas par l'autre.
    readonly property var screenNames: [
        "MainMenu", "GameView", "Pause", "Options", "Credits",
        "CharacterSheet", "Skills", "Inventory", "Journal", "WorldMap", "Dialogue",
        "Merchant", "Company", "CombatHud", "Death", "DemoEnd", "Gallery", "AssetGallery",
        "MapLauncher"
    ]

    /// Épingle l'écran nommé, s'il est de la liste.
    function showScreen(name) {
        if (root.screenNames.indexOf(name) >= 0) {
            root.pinnedScreen = name;
        }
    }

    /// Rend le clavier à l'écran courant.
    function restoreFocus() {
        stack.forceActiveFocus();
    }

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
    // Les ecrans de fin (LOT-119).
    Component { id: deathScreen; Death {} }
    Component { id: demoEndScreen; DemoEnd {} }
    // La galerie est le point d'entrée de l'atelier (`DesignStudio/Main.ui.qml`), posée telle quelle :
    // aucun jumeau, rien à câbler -- c'est ce qui prouve que les briques se résolvent au jeu comme à
    // l'atelier.
    Component { id: galleryScreen; Ui.Main {} }
    // La galerie des ASSETS : un outil de debug, lui non plus pas un ecran du jeu.
    Component { id: assetGalleryScreen; AssetGallery {} }
    // Le lanceur de cartes : un outil de debug. Il se referme en rendant la main au routeur.
    Component {
        id: mapLauncherScreen

        MapLauncher {
            onToolClosed: root.pinnedScreen = ""
        }
    }

    Loader {
        id: stack

        anchors.fill: parent
        focus: true
        // Deux sources : l'écran épinglé (`--screen=`, menu F9), sinon le routeur.
        sourceComponent: root.pinnedScreen.length > 0 ? root.byName(root.pinnedScreen)
                                                      : root.byState()
    }

    /*!
        Le menu de développement, ouvert par F9. Absent des binaires livrés : il se lie lui-même
        à `ScreenRouter.developerBuild`, et le raccourci aussi. Il remplace le sélecteur d'écrans
        et la console de debug : un seul outil, une seule touche.

        Posé APRÈS la pile, donc au-dessus : c'est un recouvrement qui commande l'écran, il ne doit
        être masqué par aucun.
    */
    DevMenu {
        id: devMenu

        anchors.fill: parent
        screenNames: root.screenNames
        // Ce que `--screenshot=` capture : les écrans, sans le menu posé dessus.
        captureTarget: stack
        onScreenChosen: function (name) { root.showScreen(name) }
        onWindowSizeChosen: function (width, height) {
            root.Window.window.width = width;
            root.Window.window.height = height;
        }
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
    // au menu restait épinglé : `Échap` ne fermait plus rien, et la navigation -- ce
    // qu'on cherche justement à vérifier -- aurait paru cassée par l'outil de vérification.
    Connections {
        target: ScreenRouter

        function onChanged() {
            root.pinnedScreen = "";
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
        case "Death":          return deathScreen
        case "DemoEnd":        return demoEndScreen
        case "Gallery":        return galleryScreen
        case "AssetGallery":   return assetGalleryScreen
        case "MapLauncher":    return mapLauncherScreen
        }
        return menuScreen
    }
}
