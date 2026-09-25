# Outils de développement du jeu

Cette page réunit ce que le **jeu** offre à qui le développe, et à personne d'autre : le menu de
développement ouvert par <kbd>F9</kbd>, les options de la ligne de commande — tapées au lancement
ou dans le menu —, le lanceur de cartes, la racine de contenu d'essai, la galerie des assets, le
compteur de diagnostic, les captures, les journaux. Chacun est décrit ailleurs dans le guide, à l'endroit du mécanisme qu'il sert ; ici, on
les trouve **tous**, avec leur mode d'emploi et la règle qui les tient hors d'un binaire livré.

## Ce qu'est un outil de développement, ici

Un outil de développement est un chemin qui mène à un état du jeu **sans le jouer** : ouvrir
l'inventaire sans partie, paraître sur une carte à la case voulue, engager un combat sans parler au
maître d'arène, photographier un écran sans main devant le clavier. Il existe parce que plusieurs
écrans et mécanismes ne sont atteignables par aucun chemin de jeu tant que le contenu n'est pas là,
et qu'un mécanisme qu'on ne peut pas atteindre ne se vérifie pas.

Ce n'est **jamais une fonctionnalité**, et le code le garantit plutôt qu'une consigne :

- **Côté C++**, `core::DEVELOPER_BUILD` (`Source/Core/BuildConfig.h`) est vrai dans un binaire de
  développement et faux dans un binaire livré (`NDEBUG`). Les options de la ligne de commande qui
  changent la partie sont lues sous `if constexpr (core::DEVELOPER_BUILD)` : le code n'existe pas
  dans la release, pas seulement désactivé.
- **Côté QML**, `hmi::ScreenRouter` publie cette constante en `ScreenRouter.developerBuild`. Le
  menu de développement y lie son `visible`, le raccourci <kbd>F9</kbd> son `enabled` : un binaire
  livré n'a ni le panneau ni la touche. Les vues-modèles des outils refusent de leur côté
  (`hmi::DebugConsoleModel::run`, `hmi::ScreenRouter::jumpToGame` sont sans effet en release).

Les outils vivent du côté **développeur** du module `Jadg.App` (`Source/App/Game/Qml/Logic/` et
`Tools/`) et dans `Source/App/Game/Main.cpp`. Aucun n'a de formulaire dans `Source/Ui` : ce sont des
interfaces de développeur, en contrôles Qt Quick ordinaires, que l'atelier n'a pas à dessiner. Ils
n'écrivent aucune couleur — ils empruntent la palette ambiante, comme tout contrôle Qt — si bien que
`EX-IHM-105` tient sans exception, et ce qui les distingue du jeu n'est pas leur teinte mais leur
libellé.

| Outil | Comment on l'atteint | Ce qu'il donne | Décrit en détail |
|---|---|---|---|
| Menu de développement | <kbd>F9</kbd>, dans le jeu | écrans, carte, dialogue, combat, fins, gel, diagnostic, journaux, ligne de commande, en un panneau | [ci-dessous](#le-menu-de-developpement-f9) |
| Ligne de commande | `JustAnotherRpgGame.exe --…`, ou tapée dans le menu | l'écran, la carte, la case, les drapeaux, le contenu, la fenêtre, la capture | [ci-dessous](#la-ligne-de-commande), [Écrans](guide-ecrans.md#ce-que-la-ligne-de-commande-impose-hmilaunchoptions) |
| Lanceur de cartes | `--screen=MapLauncher`, ou le menu | toutes les cartes du contenu et des brouillons, ouvertes à la case et dans l'état voulus | [ci-dessous](#le-lanceur-de-cartes) |
| Racine de contenu d'essai | `--data=Source/Test/Fixtures/GameData` | jouer les mécanismes sans le contenu du jour | [ci-dessous](#la-racine-de-contenu-d-essai) |
| Galerie des assets | `--screen=AssetGallery`, ou le menu | tous les assets livrés, toutes leurs formes | [Rendu 2D](guide-rendu.md#la-galerie-des-assets-hmiassetgallery), [Données](guide-donnees.md#la-galerie-des-assets) |
| Compteur de diagnostic | options, onglet Graphismes, ou le menu | images par seconde, échelle, pas de simulation | [IHM Qt](guide-ihm-qt.md) |
| Captures | `--screenshot=` ; `capture_screens.py` | l'image d'un écran, sans main devant le clavier | [IHM Qt](guide-ihm-qt.md#vérifier-une-interface-sans-la-regarder) |
| Journaux et plantages | `Logs/`, `Crashes/`, `--log-level=`, `--crash-test` | ce qui s'est passé, et pourquoi ça s'est arrêté | [Journalisation](guide-journalisation.md) |

## Le menu de développement (F9) {#le-menu-de-developpement-f9}

<kbd>F9</kbd> ouvre, et referme, un panneau posé à droite de l'écran courant et un bandeau de
ligne de commande en bas (`Source/App/Game/Qml/Tools/DevMenu.qml`). Il réunit ce qu'on faisait jusque-là par la ligne de
commande, et le rend possible **en cours de partie**, sans relancer le jeu. <kbd>Échap</kbd> le
referme aussi. C'est le **seul** outil de navigation de développement : le sélecteur d'écrans du
bas de la fenêtre (◀ ▶) et la console de debug séparée ont été fondus dedans le 25 septembre 2026,
et l'écran du Colisée retiré (le combat se joue sur la carte, `LOT-118`).

| Section | Ce qu'on y fait | Ce que ça appelle |
|---|---|---|
| **Écrans** | choisir un écran dans la liste et l'ouvrir | `ScreenStack.showScreen(nom)` : l'écran reste **épinglé** jusqu'à ce que le jeu navigue de lui-même, exactement comme `--screen=` |
| **Carte** | lire la carte courante, son nom, la case du héros et son quartier ; entrer sur une carte par son identifiant, à un point d'arrivée facultatif ; ouvrir le lanceur de cartes ; geler ou dégeler la carte | `WorldModel.enterMap(carte, arrivée)` puis `ScreenRouter.jumpToGame()` ; `--screen=MapLauncher` ; `WorldModel.frozen` |
| **Dialogue** | choisir un dialogue du contenu (ou taper son identifiant) et l'ouvrir, comme un PNJ l'ouvrirait | `DialogueModel.dialogueIds`, `ScreenRouter.openDialogue(id)` ; les drapeaux sont ceux de la partie |
| **Combat** | lire la rencontre en cours et son issue ; engager une rencontre par son identifiant | `EncounterModel.begin(rencontre)` puis `ScreenRouter.openRpgScreen(CombatHud)` |
| **Fins** | ouvrir l'écran de mort, ou l'écran « Fin de la démo » sur la voie `arene` ou `parole` (`LOT-119`) | `ScreenRouter.jumpToGame()`, puis `openDeath()` ou `openDemoEnd(voie)` — ces écrans **finissent la partie** |
| **Affichage et journaux** | montrer le compteur de diagnostic ; écrire les journaux de la session | `OptionsModel.diagnostics`, `OptionsModel.saveLogs()` |
| **Ligne de commande** (bandeau en bas de l'écran) | taper les options du binaire, appliquées **à chaud** ; « Aide » les liste ; « Relancer avec » redémarre le jeu avec la ligne ; les flèches rappellent l'historique ; le champ prend le clavier dès que le menu s'ouvre | `DebugConsoleModel.run(ligne)`, `relaunch(ligne)` — voir [ci-dessous](#la-ligne-de-commande) |

En pied de panneau, la **réponse** de la dernière commande : la carte sur laquelle on est arrivé,
la raison d'un refus (`WorldModel.status`), le chemin du fichier de journaux écrit ; la ligne de
commande a son propre compte rendu, qui défile dans le bandeau au-dessus du champ. Une commande qui ne répondrait rien
laisserait croire qu'elle n'a rien fait, alors qu'elle a peut-être échoué.

Quatre règles gouvernent ce panneau, et chacune répond à un défaut qu'on aurait eu sans elle :

- **Il ne fait rien que les vues-modèles ne sachent déjà faire.** Chaque bouton appelle un
  invocable de `Jadg.Runtime` (`hmi::WorldModel::enterMap`, `hmi::EncounterModel::begin`,
  `hmi::DebugConsoleModel::run`, `hmi::OptionsModel::saveLogs`…) ; le menu n'a pas d'état au-delà
  de ce qu'on tape dans ses champs. Un chemin de développement qui contournerait les vues-modèles
  vérifierait autre chose que le jeu.
- **Il ne contourne la table des transitions qu'à un endroit nommé.** Ouvrir la vue de jeu depuis
  n'importe quel écran n'est pas une transition de `hmi::resolveTransition` : c'est
  `hmi::ScreenRouter::jumpToGame`, sans effet dans un binaire livré. Tout le reste passe par la
  table — l'écran de mort, par exemple, s'ouvre depuis le jeu, jamais depuis le menu principal.
- **Il prend le clavier tant qu'il est ouvert**, puisque ses champs se remplissent. La vue de jeu,
  qui perd le focus, relâche les directions tenues (le héros s'arrête) ; la carte, elle, ne gèle
  pas d'elle-même — l'interrupteur *Carte gelée* le fait à la demande, pour observer une scène
  arrêtée. Fermé, le menu rend le clavier à l'écran courant (`stack.forceActiveFocus()` dans
  `ScreenStack.qml`) : sans cela, plus aucune touche n'atteignait l'écran jusqu'au prochain clic.
- **<kbd>F9</kbd> vaut partout.** C'est un `Shortcut` de **contexte application**
  (`Qt.ApplicationShortcut`), posé dans `ScreenStack.qml` : il passe avant les `Keys` des écrans,
  donc un écran qui lit toutes les touches ne l'avale pas. Il est désactivé (`enabled`) dans un
  binaire livré, où le menu n'existe pas.

**Épingler un écran.** La pile (`ScreenStack.qml`) montre l'écran épinglé (`pinnedScreen`) s'il y
en a un, sinon celui que le routeur désigne. `--screen=` épingle au lancement, le menu en cours de
partie ; l'épingle tombe dès que le routeur change d'état (un geste du jeu, une transition), si bien
que la navigation — ce qu'on veut vérifier — n'est jamais figée par l'outil. Le lanceur de cartes
se désépingle lui-même en se fermant (`toolClosed`).

Engager une rencontre depuis le menu obéit à la règle du combat sur la carte (`LOT-118`) : la zone
de combat est celle de la case de la **dernière interaction** du héros, à défaut celle qu'il
regarde ([Combat tactique](guide-combat.md)). Le héros doit donc se tenir dans une zone de combat
de la carte, ou devant une entité qui en désigne une ; sinon le montage refuse, et le journal dit
pourquoi. La rencontre d'essai s'appelle `rats-du-donjon`, sur la carte `donjon` de la racine
d'essai.

## Le lanceur de cartes {#le-lanceur-de-cartes}

`--screen=MapLauncher`, ou *Carte* → *Lanceur de cartes…* dans le menu, ouvre un outil plein écran
(`Source/App/Game/Qml/Tools/MapLauncher.qml`, `hmi::MapLauncherModel`) : la liste de **toutes** les
cartes du `Levels/` du contenu, avec leur nom et leur taille — ou l'erreur qui les empêche de se
charger —, précédées des dossiers qu'on ajoute (les brouillons de l'éditeur, comme `--levels=`).
Les champs sont ceux de la ligne de commande : arrivée, case (`--at=`), drapeaux (`--flags=`),
figurine (`--hero-figure=`). « Lancer » passe par les **mêmes fonctions** que `Main.cpp`
(`hmi::WorldModel::setLevelDirectories`, `setStartCell`, `setStartFlags`, `setStartOverride`,
`startNewGame`), puis ouvre la vue de jeu ; <kbd>Échap</kbd> ou « Fermer » rendent la main au
routeur.

La liste vient de `hmi::scanLevelDirectories` (`HMI/Game/LevelScan.h`, sans Qt) : l'identifiant
d'une carte est son chemin sous `Levels/`, sans `.json`, avec des `/` — la règle du chargeur
(`LOT-124`) —, la première occurrence l'emporte quand deux dossiers portent la même carte, et les
fichiers annexes de l'éditeur (`<carte>.editor.json`) sont écartés.

## La ligne de commande {#la-ligne-de-commande}

Toutes les options sont lues par `app::commandLineOption` dans `Source/App/Game/Main.cpp` et
`Source/App/Common/Bootstrap.cpp`, à la forme `--nom=valeur`. Une valeur illisible est ignorée et
**signalée au journal**, jamais fatale (`EX-NFR-040`) : le jeu s'ouvre quand même.

**Les mêmes options se tapent dans le menu**, dans le bandeau du bas de l'écran, et s'appliquent à chaud,
dans l'ordre du lancement : `--levels=`, `--flags=`, `--at=`, `--hero-figure=`, puis `--map=`. Une
case seule (`--at=`) déplace le héros sur la carte ouverte. `--screen=` épingle un écran,
`--window-size=` redimensionne la fenêtre (sauf en plein écran), `--screenshot=` capture les
écrans sans le menu, `--log-level=` change le niveau du journal. Ce qui ne se lit qu'au lancement
— `--data=`, `--crash-test`, `--map-*` — est dit tel, et « Relancer avec » redémarre le jeu avec la
ligne tapée. Les guillemets doubles gardent un chemin avec espaces en un seul mot.

Le catalogue de ces options est `hmi::debugOptionCatalog` (`HMI/Game/DebugCommands.h`), et un test
(`DebugCommands.LeCatalogueSuitLesSourcesDuJeu`) le recoupe avec les `"--xxx="` que `Main.cpp`,
`Bootstrap.cpp` et `WorldMap.qml` lisent : **une option nouvelle du binaire s'ajoute au catalogue**
— sinon le test échoue —, et s'applique dans `hmi::DebugConsoleModel::apply` si elle peut l'être à
chaud.

| Option | Ce qu'elle fait | Build |
|---|---|---|
| `--screen=<Nom>` | épingle l'écran nommé, par-dessus le routeur ; les noms sont ceux de `ScreenStack.screenNames` (`MainMenu`, `GameView`, `Inventory`, `Death`, `DemoEnd`, `AssetGallery`, `MapLauncher`…) | tous |
| `--window-size=<L>x<H>` | impose la taille de la fenêtre, sans passer par le plein écran (qui écrirait le réglage du joueur) | tous |
| `--screenshot=<fichier>` | rend l'interface, l'écrit dans le fichier 1,2 s après, et **quitte** ; sortie forcée à 45 s si rien n'a été produit | tous |
| `--log-level=<niveau>` | le niveau du journal — `trace`, `info`, `warning` ou `error` —, prioritaire sur `JADG_LOG_LEVEL` | tous |
| `--crash-test` | plante volontairement au démarrage, pour prouver que le minidump s'écrit sous `Crashes/` (le test de fumée de la release) | tous |
| `--map=<carte>[@<arrivée>]` | ouvre le jeu **sur la carte**, sans passer par le menu, au point d'arrivée nommé (`hmi::WorldModel::setStartOverride`) ; les options suivantes s'y accrochent : sans carte imposée, aucune n'est lue | développement |
| `--at=<colonne>,<ligne>` | pose le héros sur la case voulue (`hmi::parseStartCell`) | développement |
| `--flags=<a>,<b>,<c=valeur>` | pose des drapeaux de monde avant le premier pas : la carte **après** une quête, sans la jouer (`hmi::parseWorldFlags`, `LOT-116`) | développement |
| `--levels=<dossier>;<dossier>` | sert ces dossiers de cartes **devant** celles du binaire, dans l'ordre — l'essai complet de l'éditeur (`LOT-EDITOR-10`) ; séparateur `;` (`hmi::LAUNCH_PATH_SEPARATOR`) | développement |
| `--hero-figure=<dossier>` | remplace la figurine du héros, par un dossier depuis `Assets/` — voir une figurine de l'atelier marcher | développement |
| `--data=<racine>` | lit **tout le contenu** — cartes, assets, monde, dialogues, rencontres, créatures, libellés — sous cette racine (`hmi::dataDirectory`), comme `LevelEditor --data` (`LOT-118`) ; à lire **avant** tout modèle, donc posée en tête de `main` | développement |
| `--map-region=`, `--map-city=`, `--map-district=`, `--map-block=` | ouvrent l'écran « Carte » sur la région, la ville, le quartier ou l'îlot nommé, chaque niveau supposant le précédent (`LOT-96`) | tous |

Deux variables d'environnement complètent la ligne : `JADG_LOG_LEVEL`, le niveau du journal par
défaut, et `JADG_QML_FROM_SOURCE=1`, qui fait relire les formulaires de `Jadg.Ui` depuis les
**sources** au lieu de la ressource — retoucher un écran sans rien reconstruire ([IHM Qt — deux
applications, deux technologies](guide-ihm-qt.md)). Les tests QML ont la leur,
`JADG_UPDATE_REFERENCES=1`, qui régénère les captures de référence ([Build, tests et intégration
continue](guide-outils.md)).

Les options de développement ne sont pas seulement ignorées en release : le code qui les lit est
sous `if constexpr (core::DEVELOPER_BUILD)`, avec sa branche `else` — un retour anticipé laisserait
en Release un code inatteignable, que `/W4 /WX` refuse (C4702).

## La racine de contenu d'essai {#la-racine-de-contenu-d-essai}

`Source/Test/Fixtures/GameData` a la forme de `Source/Elements/` mais **sans un seul asset ni une
seule carte du jeu** : des aplats à la géométrie du losange, des silhouettes numérotées, trois
cartes reliées, un kit d'arène, une ville, une rencontre, un maître d'arène d'essai. Les tests la
lisent à la place du contenu livré (`JADG_TEST_DATA_DIR`), et le jeu la joue de même, ce qui est la
façon la plus courte de voir un **mécanisme** tourner quel que soit le contenu du jour :

```
JustAnotherRpgGame.exe --data=Source/Test/Fixtures/GameData --map=donjon@sable --at=24,19
```

Le héros paraît devant le maître d'arène d'essai (24, 20), dans la zone « salle » ; <kbd>E</kbd>
lui parle, « Qu'on les lâche » engage les rats sur la carte — ou, depuis le menu de développement,
*Combat* → `rats-du-donjon` → *Engager*, le héros étant dans la salle. Le `README.md` de la racine
dit ce qu'elle contient et comment son art est fait.

## Ce que le menu ne fait pas

- **Il ne triche pas sur les règles** : pas de points de vie infinis, pas de jet forcé. Un combat
  engagé depuis le menu se joue comme un combat engagé par un dialogue ; ce qu'on veut vérifier,
  c'est le combat.
- **Il ne pose de drapeau de monde que par `--flags=`**, tapé dans sa ligne de commande : les
  drapeaux sont posés sur la session, et les quêtes avancent aussitôt ; mais la carte se compose à
  l'entrée, si bien qu'un drapeau posé en cours de partie ne se voit sur la carte qu'à la
  prochaine entrée — on le tape donc avec un `--map=`.
- **Il n'écrit rien dans les réglages du joueur**, à une exception nommée : le compteur de
  diagnostic est le même réglage que l'onglet Graphismes des options, retenu d'un lancement à
  l'autre, comme si on l'avait coché là.
- **Il n'est pas traduit** : ses textes sont écrits en français dans le fichier, sans `qsTr`, comme
  ceux du lanceur de cartes et de la galerie des assets. `check_translations.py` ne les demande pas, et
  l'atelier ne les voit pas.

## Ajouter un outil

Un outil nouveau va dans `Source/App/Game/Qml/Tools/` (un écran entier, comme la galerie des
assets) ou dans une section du menu de développement (une commande). Dans les deux cas :

1. il **appelle** une vue-modèle de `Jadg.Runtime`, il n'en réécrit pas la logique ; s'il faut un
   invocable nouveau, il se pose sur la vue-modèle, documenté, et un test unitaire le garde ;
2. il se lie à `ScreenRouter.developerBuild` (ou vit sous `core::DEVELOPER_BUILD` en C++) ;
3. il n'écrit **aucune** couleur, police ni taille de police (`check_ui_layers.py`, règle 6) :
   la palette ambiante suffit ;
4. il ne prend le clavier que s'il en a besoin, et le **rend** en se fermant ;
5. un écran nouveau s'ajoute à `ScreenStack.screenNames` et à `byName`, dans la liste de
   `Source/App/CMakeLists.txt` (un fichier non listé n'existe pas pour le jeu), et ici.

## Voir aussi

- `hmi::ScreenRouter`, `hmi::WorldModel`, `hmi::EncounterModel`, `hmi::OptionsModel`,
  `hmi::DebugConsoleModel`, `hmi::MapLauncherModel`, `hmi::debugOptionCatalog`,
  `hmi::scanLevelDirectories`, `hmi::GameLaunchOptions`, `hmi::dataDirectory`,
  `core::DEVELOPER_BUILD`.
- [`Source/App/Game/Qml/Tools/DevMenu.qml`](../../Source/App/Game/Qml/Tools/DevMenu.qml),
  [`Source/App/Game/Qml/Tools/MapLauncher.qml`](../../Source/App/Game/Qml/Tools/MapLauncher.qml),
  [`Source/App/Game/Qml/Logic/ScreenStack.qml`](../../Source/App/Game/Qml/Logic/ScreenStack.qml),
  [`Source/App/Game/Main.cpp`](../../Source/App/Game/Main.cpp).
- [Écrans, navigation et boucle de jeu](guide-ecrans.md) — le routeur, la pile, ce que la ligne de
  commande impose à la partie.
- [IHM Qt — deux applications, deux technologies](guide-ihm-qt.md) — les modules QML, les
  vues-modèles, les captures.
- [Rendu 2D : de la scène à l'écran](guide-rendu.md) — la galerie des assets.
- [Journalisation et assertions](guide-journalisation.md) — les niveaux, les journaux de session,
  le rapport de plantage.
- [Build, tests et intégration continue](guide-outils.md) — construire le binaire de développement
  qui porte ces outils.
