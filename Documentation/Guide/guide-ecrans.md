# Écrans, navigation et boucle de jeu

Cette page explique comment le jeu passe du menu à la carte, à la pause, aux options, aux écrans
du RPG ou au Colisée. Les écrans sont des fichiers QML ([IHM Qt — deux applications, deux technologies](guide-ihm-qt.md)) ; la **table de
transitions** qui décide où l'on peut aller est, elle, du C++ pur, testé sans fenêtre, que
`hmi::ScreenRouter` se contente d'appeler pour le compte du QML. L'éditeur de niveaux est un
binaire séparé ([Éditeur de niveaux](guide-editeur.md)) : aucun chemin du jeu n'y mène.

## La machine à états : `hmi::ScreenFlow`

`Source/HMI/Presentation/ScreenFlow.h` porte la navigation comme une **table pure**, sans
dépendance Qt — testable hors instance d'application (`EX-NFR-010`), même patron que
`hmi::PanelFocus`. Deux fonctions :

- `hmi::resolveTransition(current, event)` : résout un `hmi::ScreenEvent` depuis l'état courant
  (`hmi::ScreenState`) vers le nouvel écran, ou `std::nullopt` si la transition est **interdite**
  depuis cet écran — jamais de bascule silencieuse (`EX-GP-041`).
- `hmi::dressingFor(screen)` : l'habillage attendu (`hmi::ScreenDressing`), repris de l'époque où
  une même fenêtre hébergeait jeu et éditeur.

`hmi::ScreenId` compte huit états : `Menu`, `Editor`, `Game`, `Options`, `Pause`, `Credits`,
`RpgScreen` (l'un des écrans du RPG — fiche, inventaire, carte… —, dont
`Source/HMI/Presentation/RpgScreens.h` tient la liste) et `Arena` (le Colisée). `Editor` n'est plus jamais atteint par le jeu.

**La provenance est un attribut de l'état**, jamais une variable « écran précédent » posée à côté :
`ScreenState::optionsReturnTo` dit si `CloseOptions` revient au menu ou à la pause,
`rpgReturnTo` d'où un écran du RPG a été ouvert, `arenaReturnTo` si le Colisée revient au menu ou
à la carte (quand le héraut y envoie). Un seul événement `OpenOptions` sert ainsi deux origines.

## Le routeur et la pile d'écrans

`hmi::ScreenRouter` (`Source/HMI/Runtime/ScreenRouter.h`) est un singleton QML. Il **ne décide
rien** : chaque méthode (`openGame`, `openPause`, `resume`, `openOptions`, `openArena`,
`openRpgScreen`…) appelle `resolveTransition`, garde l'état courant si la transition est refusée,
et publie sinon `currentScreen` et `currentRpgScreen`.

Il publie un **état**, jamais un nom de fichier : la correspondance entre état et écran vit en
QML, dans `Source/App/Game/Qml/Logic/ScreenStack.qml`. Chaque écran y est enveloppé dans un
`Component`, construit par un `Loader` seulement une fois choisi — les écrans ne vivent jamais tous
en même temps. La conception peut ainsi renommer ou réorganiser ses formulaires sans qu'une ligne
de C++ ne s'en aperçoive.

Aucun écran ne bascule lui-même vers un autre en manipulant la pile : il appelle le routeur
(`ScreenRouter.openPause()` depuis la vue de jeu sur `Échap`, `ScreenRouter.resume()` depuis la
pause…), et la pile suit. `--screen=<Nom>` et le sélecteur de développement
(`Logic/ScreenProbe.qml`, absent des binaires livrés) court-circuitent le routeur : ce sont des
outils de vérification, pas des chemins de jeu.

## La vue de jeu et la session qui lui survit

`Screens/GameView.qml` pose la surface de rendu QRhi (`WorldViewport`, [Rendu 2D : de la scène à l'écran](guide-rendu.md)) sur le
singleton `hmi::WorldModel`, qui porte la **session d'exploration** (`core::ExplorationSession`,
via `hmi::WorldPlay`). La session est un singleton précisément parce que la pile **détruit** la vue
de jeu quand un autre écran la remplace : une session possédée par l'écran mourrait avec lui, et
l'on reviendrait de la pause, du dialogue ou du sable sur une carte neuve. `GameView` ne lance donc
`WorldModel.startNewGame()` que si aucune carte n'est chargée.

Le **gel** suit le focus : ce qui recouvre la vue de jeu (dialogue, Colisée, pause, écran du RPG)
lui prend le focus, et la vue relâche alors les directions tenues ; le dialogue pose en plus
`WorldModel.frozen`. Un seul chemin pour tous les écrans plutôt qu'un par écran. Quel écran du RPG
suspend la simulation est dit par `hmi::pausesGame` (`EX-IHM-091`), pas par la table de
transitions, qui ne connaît pas ces écrans un par un.

L'**essai immédiat** de l'éditeur joue la même exploration, par le même `hmi::WorldPlay`
(`EX-EDIT-055`, [Éditeur de niveaux](guide-editeur.md)) : l'essai montre donc exactement ce que le jeu montrera.

## Pause

`Échap` sur la vue de jeu ouvre l'écran de pause (`Screens/Pause.qml`) : *Reprendre*, *Options*,
*Quitter vers le menu*. Clavier et pointeur pilotent le même `currentIndex` ; `Échap` y reprend la
partie. Les options ouvertes depuis la pause y reviennent (`optionsReturnTo`), et reprendre ramène
sur la carte là où on l'avait laissée — la session n'a pas bougé (`EX-IHM-004`).

## Ce que le `LOT-67` a retiré

Ce guide décrivait, jusqu'au `LOT-67`, trois mécanismes de plus : un **écran de fin de niveau**,
une **sélection de niveau** et une **progression persistée** au tableau. Les trois supposaient une
séquence ordonnée de tableaux, que le jeu n'a pas — et ils ont été retirés avec elle, code,
écrans et exigences (`EX-LVL-010` → `EX-LVL-015`, `EX-IHM-005`, `EX-GP-030` → `EX-GP-032`, toutes
consignées « retirées » dans leur spécification plutôt que supprimées ; `EX-GP-040`, `EX-IHM-003`
et `EX-IHM-004`, elles, sont **refondues** — elles avaient un objet au-delà du niveau discret).

Le passage d'une carte à l'autre est désormais le **graphe de cartes** du `LOT-09`
(`core::WorldGraph`, `core::WorldTravel`). Ce qu'on retrouve en revenant sera la **sauvegarde
riche** du `LOT-17`, et « Continuer » reviendra au menu avec elle — pas avant : une entrée de menu
qui ne mène nulle part coûte plus de confiance qu'elle n'apporte d'information (`EX-IHM-072`).

## Où ça s'insère dans la boucle

L'**event loop Qt** (`QGuiApplication::exec`) possède la navigation. La simulation de la carte
avance à **pas fixe** dans `hmi::WorldModel` : un `QTimer` précis de `STEP_MILLISECONDS` (16 ms)
appelle `WorldModel::step`, qui transmet l'intention courante (direction, interaction) à
`WorldPlay::step` avec une durée constante — jamais le temps réel écoulé. Le rendu, lui, est
cadencé par le graphe de scène Qt Quick ; le détail est dans [IHM Qt — deux applications, deux technologies](guide-ihm-qt.md) et
[La boucle de jeu](guide-boucle.md).

## Voir aussi
- `hmi::ScreenRouter`, `hmi::WorldModel`, `hmi::WorldPlay`.
- `hmi::ScreenFlow`, `hmi::ScreenId`, `hmi::ScreenEvent`, `hmi::ScreenState`, `hmi::ScreenDressing`.
- [IHM Qt — deux applications, deux technologies](guide-ihm-qt.md) — le socle Qt Quick : module QML, surface de rendu QRhi.
- [Entrées et actions logiques](guide-entrees.md) — le clavier et la manette dans les écrans.
- [Niveaux : modèle, couches, entités, chargement](guide-niveaux.md), [Éditeur de niveaux](guide-editeur.md) — le format des cartes, et comment l'éditeur réutilise
  `hmi::WorldPlay` pour l'essai immédiat.
- [Boucle de jeu et pas de temps fixe](guide-boucle.md) — le pas fixe.
