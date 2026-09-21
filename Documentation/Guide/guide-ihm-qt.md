# IHM Qt — deux applications, deux technologies

> Statut : **refondu** (`LOT-86`). Le **jeu** est une application **Qt Quick** ; l'**éditeur de
> niveaux** reste en **Qt Widgets**, dans son propre binaire. Le rendu de scène passe par **QRhi**
> — Direct3D 11 par défaut sous Windows — des deux côtés. L'apparence des écrans du jeu et le mode
> d'emploi de la conception sont en [Concevoir les écrans dans Qt Design Studio](guide-conception-qds.md), que cette page laisse de côté.

## Pourquoi deux binaires

L'éditeur et le jeu vivaient dans une seule application, et c'est de là que venaient les **2 472
lignes** de `MainWindow.cpp` : une seule technologie d'IHM devait servir deux besoins opposés.

- L'**éditeur** est un outil d'auteur : docks détachables, arbres, disposition persistée
  (`EX-IHM-011`). Qt Widgets y est le bon outil, et QML n'y apporterait rien. Depuis le
  `LOT-EDITOR-01`, c'est un module à part (`Source/Editor`), outil interne : style Fusion, textes
  anglais écrits dans le code, widgets construits en code.
- Le **jeu** est l'inverse : une image agrandie d'un facteur entier, dont l'apparence doit pouvoir
  changer **sans compiler** (`EX-IHM-100`).

| Cible | Technologie | Point d'entrée |
|---|---|---|
| `JustAnotherRpgGame` | Qt Quick, `QGuiApplication` | `Source/App/Game/Main.cpp` |
| `LevelEditor` | Qt Widgets, `QApplication` | `Source/App/Editor/Main.cpp` |

Elles partagent `Core`, `HMI/Graphics`, `HMI/Game`, `HMI/Input`, `HMI/Audio` et l'amorçage
(`App/Common/Bootstrap`) — tout ce qui n'est pas de la présentation. Elles ne partagent **aucune**
technologie d'IHM, et le jeu **ne lie pas `Qt6::Widgets`** (`EX-IHM-102`). Ce n'est pas une
convention : un widget qui y réapparaîtrait ferait échouer l'édition de liens.

## Les trois couches du jeu

```
Core                     règles et état, SANS Qt
  ↑
HMI/Presentation         logique de présentation pure (table de transitions, échelle, valeurs)
HMI/Runtime              vues-modèles exposées au QML : ce que le jeu SAIT DIRE (module Jadg.Runtime)
  ↑                      Qt6::Qml seulement — jamais Quick ni Widgets (EX-IHM-101)
Source/Ui (QML)          ce que ça DONNE À VOIR (module Jadg.Ui, sans C++)
Source/App/Game/Qml      le câblage entre les deux (module Jadg.App)
```

`HMI/Runtime` transforme l'état du jeu en propriétés et en modèles de liste, et **ne dessine
rien**. Un écran lui demande *ce que le jeu sait dire*, jamais *comment le montrer*. Un seul en-tête
d'IHM qui y entrerait signalerait que la logique de vue a commencé à redescendre dans la couche de
données — et c'est ainsi que `MainWindow.cpp` s'était épaissi.

`scripts/check_ui_layers.py` vérifie les six règles de cette séparation à chaque *Pull Request*.
Elles sont écrites en `EX-IHM-100` à `EX-IHM-105`. **Une règle qui n'est pas vérifiée n'est pas une
règle : c'est une intention** — le dépôt l'a appris deux fois, avec un défaut de taille d'écran
corrigé *trois* fois et une palette écrite *deux* fois.

## Les trois modules QML

Depuis le `LOT-87`, le jeu est fait de **trois** modules QML, et chacun est déclaré **dans le
répertoire de ses fichiers** :

| Module | Répertoire, et son CMakeLists.txt | Contenu | Cible |
|---|---|---|---|
| `Jadg.Ui` | `Source/Ui` | formulaires, contrôles, jetons, galerie — **QML pur** | bibliothèque statique `JadgUi`, `designersupported` |
| `Jadg.Runtime` | `Source/HMI/Runtime` | les types C++ exposés au QML (`QML_ELEMENT`) | bibliothèque statique `JadgRuntime` |
| `Jadg.App` | `Source/App` (fichiers sous `Game/Qml/`) | la fenêtre, la pile d'écrans, les jumeaux | l'exécutable `JustAnotherRpgGame` |

**Pourquoi trois, et pourquoi là.** Qt Design Studio ne charge aucun plugin C++ du projet : un
module qui mêle formulaires et types C++ est résolvable par le jeu et pas par l'atelier. `Jadg.Ui`
est donc pur, et n'importe jamais `Jadg.Runtime` — seuls les jumeaux le font, et l'atelier le
remplace par les doublures de `Source/Ui/Mocks/`. Quant au « là » : `qt_add_qml_module` calcule le
chemin de ressource de chaque fichier relativement au CMakeLists.txt qui l'appelle. Un module
déclaré depuis un autre répertoire oblige à réécrire ces chemins un par un, par des alias, et c'est
cette plomberie qui a coûté 125 commits à une branche abandonnée. **Aucun alias de ressource dans
ce dépôt**, et `check_qml_designer_compat.py` le vérifie.

La découverte de Qt et `QT_VERSION_MINIMUM` vivent dans `Source/CMakeLists.txt` : les cibles
importées d'un `find_package` ne sont visibles que sous le répertoire qui l'a appelé, et trois
répertoires frères en dépendent.

Les deux bibliothèques statiques sont liées **avec leurs plugins** (`JadgUiplugin`,
`JadgRuntimeplugin`) : le `qmldir` embarqué de chaque module les désigne (`optional plugin`,
`linktarget`), et c'est le plugin lié qui enregistre les types quand l'engine rencontre l'import.
Aucune macro d'import dans le C++.

Quatre pièges, tous silencieux, tous consignés dans les CMake :

- un **singleton** doit être déclaré (`QT_QML_SINGLETON_TYPE`) : CMake ne déduit pas
  `pragma Singleton`. Non déclaré, le type se charge quand même — mais chaque `import` en construit
  une instance neuve, et le facteur d'agrandissement posé par la fenêtre n'est vu par aucun écran ;
- le fichier d'enregistrement des types est **engendré** et inclut les en-têtes par **nom de base**,
  dans un `__has_include` qui échoue sans bruit : le répertoire doit être dans les chemins
  d'inclusion ;
- `windeployqt` sans `--qmldir` n'embarque **aucun** module QML, et le jeu se lance alors sans
  interface, sans message — il en faut deux, un par répertoire QML ;
- `target_link_libraries` doit **précéder** `qt_add_qml_module` sur l'exécutable : la cible
  `all_qmllint` compose ses chemins d'import depuis les modules déjà liés à cet instant. Après,
  qmllint ne résout ni `Jadg.Ui` ni `Jadg.Runtime` et signale chaque écran en erreur.

### Éditer un écran sans rien reconstruire

La ressource imposerait une reconstruction à chaque retouche. Un **second `qmldir`** est donc
engendré sous `Source/Ui`, dont les chemins désignent les **sources** ; `JADG_QML_FROM_SOURCE=1` le
place en tête des chemins d'import.

Il est engendré depuis **la même liste** que la ressource : ajouter un écran ne crée pas un second
endroit à synchroniser — ce serait exactement la surcouche que ce lot supprime ailleurs. C'est aussi
lui qui rend `Source/Ui` importable tel quel, donc ouvrable par Qt Design Studio. Seul `Jadg.Ui`
se relit ainsi : `Jadg.App` et `Jadg.Runtime` restent ceux du binaire, et c'est voulu — ce qu'un
artiste change ne demande jamais de les toucher.

### Ouvrir les écrans pour les dessiner

**Ce n'est pas Qt Designer.** Qt Designer dessine des *widgets* et n'ouvre que des `.ui` (XML) — le
dépôt n'en a plus aucun depuis le `LOT-EDITOR-01`, qui construit les widgets de l'éditeur en code. Les écrans du jeu sont du Qt Quick : ils
s'ouvrent dans **Qt Design Studio**, qui est un programme distinct.

```
D:/Qt/Tools/QtDesignStudio/bin/qtdesignstudio.exe Source/Ui/JadgUi.qmlproject
```

Trois règles, dont deux se paient par un mode *Design* vide plutôt que par un message :

- **ouvrir le `.qmlproject`, jamais le fichier seul.** Un `.ui.qml` ouvert par « File > Open File »
  n'a pas de chemin d'import : `import Jadg.Ui` échoue, et la vue 2D reste blanche ;
- **le projet doit avoir été configuré une fois par CMake.** Le `qmldir` de `Source/Ui/Jadg/Ui/`
  est *engendré* (ci-dessus) et ignoré par git : sur un dépôt fraîchement cloné il n'existe pas
  encore, et aucun type du module ne se résout. `scripts/build.ps1` suffit à le poser ;
- **le mode Design reste grisé si `qt6Project: true` manque** du `.qmlproject`. Sans ce booléen,
  Design Studio suppose un projet Qt 5, ne trouve aucun kit Qt 5, et désactive le mode — sans
  message ni trace dans le journal ;
- **on dessine le `*Form.ui.qml`, jamais son jumeau.** Un `.ui.qml` est déclaratif, donc réversible :
  Design Studio le réenregistre sans le casser. Le jumeau `.qml` contient du JavaScript ; Design
  Studio l'ouvre — grâce aux doublures de `Source/Ui/Mocks/` — pour le *voir* avec les valeurs du
  jeu, mais ne l'édite qu'en texte, et c'est voulu — c'est la frontière du lot, rendue littérale
  par l'outil lui-même ;
- **la bibliothèque de composants reste « (vide) »** pour les dossiers du projet, avec ou sans le
  mot `designersupported` que le `qmldir` engendré porte, et quelle que soit la disposition des
  fichiers (deux essayées en phase 1 du `LOT-87`). Les briques se posent depuis la galerie
  `DesignStudio/Main.ui.qml` ou par le code ; le point reste à instruire.

Aucune version de Qt n'est écrite dans le `.qmlproject`, et c'est délibéré. Le projet se construit
avec la version épinglée par `QT_VERSION_MINIMUM` (`Source/CMakeLists.txt`) — **6.11.2**, que
`check_qt_version_pin.py` tient identique en CMake et en CI. Design Studio, lui, dessine toujours avec le Qt qu'il **embarque**,
quel que soit le Qt installé : un numéro de plus dans le fichier de conception ne commanderait ni
l'un ni l'autre, et ne servirait qu'à faire croire à un troisième épinglage. Tous les imports du
module étant sans version, le choix ne se pose pas.

Ce Qt embarqué se voit dans le nom du programme qui dessine : `qmlpuppet-4.8.3.exe`, versionné par
**Design Studio** (4.8.3 sur le poste de référence) et non par Qt. C'est le seul exemplaire de la machine — une installation Qt
ordinaire n'en fournit aucun — donc la « couche d'émulation QML » des préférences n'a nulle part
ailleurs où pointer. Voir « Qt 6.8.7 » dans l'atelier alors que le jeu se construit en 6.11.2 n'est
pas un défaut d'installation : c'est la conception de l'outil, et seule une version plus récente de
Design Studio la déplacera.

Les types **C++** (`OptionsModel`, `ScreenRouter`, `PendingData`, `WorldModel`,
`CharacterSheetModel`, `InventoryModel`, `WorldViewport`…) sont invisibles à Design Studio ; `Source/Ui/Mocks/Jadg/Runtime/`
en porte des doublures QML aux mêmes noms et propriétés, que le `.qmlproject` place dans ses
`importPaths`. Les quatorze formulaires et les quatorze jumeaux se résolvent ainsi dans l'atelier —
vérifié avec le `qmllint` du Qt 6.8.7 embarqué. Seuls `Main.qml` et `ScreenStack.qml` restent
irrésolus, parce qu'ils importent `Jadg.App` lui-même : ce sont des fichiers de câblage, et rien
ne s'y dessine. `check_qml_designer_compat.py` tient les doublures alignées sur le C++ : chaque
`Q_PROPERTY` et chaque `Q_INVOKABLE` doit y avoir son pendant.

## La surface de rendu

Le jeu pose ses surfaces QRhi comme des items Qt Quick (`QQuickRhiItem`, dans `HMI/Runtime`) :
`hmi::WorldViewportItem` (`WorldViewport` en QML) pour la carte explorée,
`hmi::ArenaViewportItem` pour le Colisée, `hmi::GameViewportItem` sous le HUD de combat. C'est le
jumeau (`GameView.qml`, `Arena.qml`…) qui les pose, dans l'hôte que le formulaire lui réserve : un
type C++ n'a pas sa place dans un formulaire. Tous rendent dans une **texture d'appui** que leur
hôte compose : la cible technique ne change pas (`EX-ARCH-050`), seul l'hôte change. Un
recouvrement redevient donc un enfant ordinaire — plus aucun empilement de fenêtres natives.
L'éditeur, lui, ne parle plus au GPU depuis le `LOT-EDITOR-02` : son canevas est une
`QGraphicsView` qui peint par `QPainter` la scène que le jeu compose (`hmi::EditorViewport`).

**La différence qui compte** : `QQuickRhiItem` peint sur le **fil de rendu**, pas sur le fil
graphique. Toute donnée que la simulation produit doit traverser `synchronize()`, appelée pendant
que le fil graphique est **bloqué** — le seul instant où les deux fils peuvent se parler sans verrou.

C'est pour cela que les surfaces n'échangent que des **valeurs** : `hmi::WorldViewportItem` reçoit
la couleur d'effacement, le point suivi par la caméra et, si la scène a changé (compté par
`WorldModel::sceneRevision`), un instantané de la scène — des primitives **pures et sans GPU**
(`EX-NFR-004`/`005`), que le fil de rendu soumet par `hmi::WorldSceneRenderer`. La frontière que
le projet s'était donnée pour tester le rendu sans GPU sert ici une seconde fois.

`hmi::SceneResources` regroupe ce que les surfaces créent à l'identique — lot de sprites, atlas,
cache de textures. Le regroupement tient moins à l'économie qu'à l'**ordre de libération** : ce
qui tient une texture doit mourir avant elle, et la texture avant le pipeline qui l'échantillonne.
Le désordre ne produit pas une erreur nette mais un plantage à la fermeture, intermittent selon le
pilote.

## La navigation

`hmi::ScreenRouter` ne **décide rien**. Toute la règle vit dans `hmi::resolveTransition` — table
pure, sans Qt, couverte par ses tests — et le routeur ne fait que l'appeler et diffuser le résultat.
Une transition non déclarée est **refusée**, jamais silencieusement acceptée (`EX-GP-041`) : sans
cette discipline, un `openOptions()` appelé depuis un écran d'où les options ne s'ouvrent pas
produirait un état que la table ne décrit pas, et dont personne ne saurait revenir.

Il publie un **état**, jamais un chemin de fichier. La correspondance entre état et écran vit dans
`Source/App/Game/Qml/Logic/ScreenStack.qml` — côté développeur, mais du bon côté de la frontière : la
conception peut réorganiser `Screens/` sans qu'une ligne de C++ ne s'en aperçoive.

`--screen=<Nom>` court-circuite le routeur et ouvre un écran directement. C'est un outil de
vérification, pas un chemin de jeu.

`Source/App/Game/Qml/Logic/ScreenProbe.qml` fait la même chose **en cours d'exécution** : deux
boutons posés en bas de la fenêtre font défiler les écrans. Ils existent parce que plusieurs écrans
sont dessinés mais pas encore alimentés — sans eux, ils ne seraient atteignables par aucun chemin
de jeu, et ne se vérifieraient donc pas.

Deux choses le distinguent d'une fonctionnalité :

- il se lie à `ScreenRouter.developerBuild` et **n'existe pas** dans un binaire livré — garanti par
  la construction, non par une consigne de relecture ;
- il **rend la main au routeur** dès que le jeu navigue de lui-même. Sans cela, l'écran choisi
  restait épinglé : `Échap` ne fermait plus rien, et la navigation aurait paru cassée par l'outil
  censé permettre de la vérifier.

## Les réglages, et ce qu'ils atteignent

`hmi::OptionsModel` ne fait que **persister et prévenir** ; c'est `App/Game/Main.cpp` qui branche
chaque signal sur ce qu'il atteint. La vue-modèle ignore ainsi le moteur audio, la fenêtre et les
traducteurs — c'est précisément la frontière que le lot établit.

| Réglage | Atteint | Quand |
|---|---|---|
| plein écran | la fenêtre, par **liaison** sur `visibility` | immédiatement |
| volume | `hmi::AudioEngine::setVolume` | immédiatement |
| langue | le `QTranslator` puis `QQmlEngine::retranslate()` | immédiatement |
| compteur de diagnostic | `Controls/DiagnosticsOverlay.ui.qml` | immédiatement |
| synchronisation verticale | `QSurfaceFormat::setDefaultFormat` | **au prochain lancement** |

La dernière ligne est dite **à l'écran** et non tue : `EX-IHM-083` exige qu'un réglage exposé
atteigne le moteur, et il l'atteint — mais l'utilisateur doit savoir quand. Elle se pose sur le
format de surface, donc avant la fenêtre ; la changer à chaud recréerait la surface de rendu sous
les yeux du joueur, pour un réglage qu'on modifie une fois.

Deux pièges consignés là où ils se posent :

- l'**identité de l'application** (`setOrganizationName`) doit précéder toute lecture de `QSettings`,
  sans quoi la synchronisation verticale serait lue dans une portée vide — le réglage paraîtrait
  absent et sa valeur par défaut s'appliquerait à chaque lancement, en silence ;
- le changement de langue à chaud **exige** `retranslate()` : sans lui, la nouvelle langue
  n'apparaîtrait qu'aux écrans construits ensuite, et la moitié de l'interface changerait.

## Vérifier une interface sans la regarder

`--screenshot=<chemin>` capture la fenêtre **par Qt lui-même**. Les API de capture de Windows rendent
une image **noire** d'une fenêtre Qt Quick, dessinée par le GPU : seul Qt sait relire son propre
graphe de scène. La vérification visuelle des écrans devient ainsi reproductible, au lieu de dépendre
d'un œil devant l'écran au bon moment.

`--window-size=<largeur>x<hauteur>` impose la taille de la fenêtre, sans passer par le plein écran
— qui écrirait le réglage du joueur et donnerait la taille de son moniteur. C'est ainsi que chaque
écran de la charte v2 se capture à 1920 × 1080 et à 1280 × 720, à côté de sa maquette :

```
JustAnotherRpgGame --screen=MainMenu --window-size=1920x1080 --screenshot=menu-1080p.png
```

## Voir aussi

- [Concevoir les écrans dans Qt Design Studio](guide-conception-qds.md) — le mode d'emploi de la **conception** : ce qu'on modifie sans code.
- [Système de design et architecture de l'information](guide-design-ihm.md) — la répartition de l'information dans l'éditeur.
- [Écrans, navigation et boucle de jeu](guide-ecrans.md) — la navigation entre écrans.
- [Boucle de jeu et pas de temps fixe](guide-boucle.md) — la boucle et le pas de temps fixe.
- [Rendu 2D : de la scène à l'écran](guide-rendu.md) — le pipeline QRhi, partagé par les deux applications.
