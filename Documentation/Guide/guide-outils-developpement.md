# Outils de développement du jeu

Cette page réunit ce que le **jeu** offre à qui le développe, et à personne d'autre : le menu de
développement ouvert par <kbd>F9</kbd>, le sélecteur d'écrans, les options de la ligne de commande,
la racine de contenu d'essai, la galerie des assets, le compteur de diagnostic, les captures, les
journaux. Chacun est décrit ailleurs dans le guide, à l'endroit du mécanisme qu'il sert ; ici, on
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
  sélecteur d'écrans et le menu de développement y lient leur `visible`, le raccourci
  <kbd>F9</kbd> son `enabled` : un binaire livré n'a ni les boutons ni la touche.

Les outils vivent du côté **développeur** du module `Jadg.App` (`Source/App/Game/Qml/Logic/` et
`Tools/`) et dans `Source/App/Game/Main.cpp`. Aucun n'a de formulaire dans `Source/Ui` : ce sont des
interfaces de développeur, en contrôles Qt Quick ordinaires, que l'atelier n'a pas à dessiner. Ils
n'écrivent aucune couleur — ils empruntent la palette ambiante, comme tout contrôle Qt — si bien que
`EX-IHM-105` tient sans exception, et ce qui les distingue du jeu n'est pas leur teinte mais leur
libellé.

| Outil | Comment on l'atteint | Ce qu'il donne | Décrit en détail |
|---|---|---|---|
| Menu de développement | <kbd>F9</kbd>, dans le jeu | écrans, carte, combat, gel, diagnostic, journaux, en un panneau | [ci-dessous](#le-menu-de-developpement-f9) |
| Sélecteur d'écrans | boutons ◀ ▶ en bas de la fenêtre | faire défiler les dix-sept écrans et galeries | [ci-dessous](#le-selecteur-d-ecrans), [Écrans](guide-ecrans.md#les-outils-de-vérification-et-pourquoi-ils-ne-sont-pas-des-chemins-de-jeu) |
| Ligne de commande | `JustAnotherRpgGame.exe --…` | l'écran, la carte, la case, les drapeaux, le contenu, la fenêtre, la capture | [ci-dessous](#la-ligne-de-commande), [Écrans](guide-ecrans.md#ce-que-la-ligne-de-commande-impose-hmilaunchoptions) |
| Racine de contenu d'essai | `--data=Source/Test/Fixtures/GameData` | jouer les mécanismes sans le contenu du jour | [ci-dessous](#la-racine-de-contenu-d-essai) |
| Galerie des assets | `--screen=AssetGallery`, ou le menu | tous les assets livrés, toutes leurs formes | [Rendu 2D](guide-rendu.md#la-galerie-des-assets-hmiassetgallery), [Données](guide-donnees.md#la-galerie-des-assets) |
| Compteur de diagnostic | options, onglet Graphismes, ou le menu | images par seconde, échelle, pas de simulation | [IHM Qt](guide-ihm-qt.md) |
| Captures | `--screenshot=` ; `capture_screens.py` | l'image d'un écran, sans main devant le clavier | [IHM Qt](guide-ihm-qt.md#vérifier-une-interface-sans-la-regarder) |
| Journaux et plantages | `Logs/`, `Crashes/`, `--log-level=`, `--crash-test` | ce qui s'est passé, et pourquoi ça s'est arrêté | [Journalisation](guide-journalisation.md) |

## Le menu de développement (F9) {#le-menu-de-developpement-f9}

<kbd>F9</kbd> ouvre, et referme, un panneau posé à droite de l'écran courant
(`Source/App/Game/Qml/Tools/DevMenu.qml`). Il réunit ce qu'on faisait jusque-là par la ligne de
commande ou par le sélecteur, et le rend possible **en cours de partie**, sans relancer le jeu.
<kbd>Échap</kbd> le referme aussi.

| Section | Ce qu'on y fait | Ce que ça appelle |
|---|---|---|
| **Écrans** | choisir un écran dans la liste et l'ouvrir | le sélecteur d'écrans (`ScreenProbe.select`) : l'écran reste épinglé jusqu'à ce que le jeu navigue de lui-même, exactement comme `--screen=` |
| **Carte** | lire la carte courante, son nom, la case du héros et son quartier ; entrer sur une carte par son identifiant, à un point d'arrivée facultatif ; geler ou dégeler la carte | `WorldModel.enterMap(carte, arrivée)`, puis la vue de jeu si elle n'est pas à l'écran ; `WorldModel.frozen` |
| **Combat** | lire la rencontre en cours et son issue ; engager une rencontre par son identifiant ; ouvrir le Colisée | `EncounterModel.begin(rencontre)` puis `ScreenRouter.openRpgScreen(CombatHud)` ; `ScreenRouter.openArena()` |
| **Affichage et journaux** | montrer le compteur de diagnostic ; écrire les journaux de la session | `OptionsModel.diagnostics`, `OptionsModel.saveLogs()` |
| **Ligne de commande** | un aide-mémoire des options ci-dessous | rien : c'est du texte |

En pied de panneau, la **réponse** de la dernière commande : la carte sur laquelle on est arrivé,
la raison d'un refus (`WorldModel.status`), le chemin du fichier de journaux écrit. Une commande
qui ne répondrait rien laisserait croire qu'elle n'a rien fait, alors qu'elle a peut-être échoué.

Trois règles gouvernent ce panneau, et chacune répond à un défaut qu'on aurait eu sans elle :

- **Il ne fait rien que les vues-modèles ne sachent déjà faire.** Chaque bouton appelle un
  invocable de `Jadg.Runtime` (`hmi::WorldModel::enterMap`, `hmi::EncounterModel::begin`,
  `hmi::OptionsModel::saveLogs`…) ; le menu n'a pas d'état au-delà de ce qu'on tape dans ses
  champs. Un chemin de développement qui contournerait les vues-modèles vérifierait autre chose que
  le jeu.
- **Il prend le clavier tant qu'il est ouvert**, puisque ses champs se remplissent. La vue de jeu,
  qui perd le focus, relâche les directions tenues (le héros s'arrête) ; la carte, elle, ne gèle
  pas d'elle-même — l'interrupteur *Carte gelée* le fait à la demande, pour observer une scène
  arrêtée. Fermé, le menu rend le clavier à l'écran courant (`stack.forceActiveFocus()` dans
  `ScreenStack.qml`) : sans cela, plus aucune touche n'atteignait l'écran jusqu'au prochain clic.
- **<kbd>F9</kbd> vaut partout.** C'est un `Shortcut` de **contexte application**
  (`Qt.ApplicationShortcut`), posé dans `ScreenStack.qml` : il passe avant les `Keys` des écrans,
  donc un écran qui lit toutes les touches ne l'avale pas. Il est désactivé (`enabled`) dans un
  binaire livré, où le menu n'existe pas.

Engager une rencontre depuis le menu obéit à la règle du combat sur la carte (`LOT-118`) : la zone
de combat est celle de la case de la **dernière interaction** du héros, à défaut celle qu'il
regarde ([Combat tactique](guide-combat.md)). Le héros doit donc se tenir dans une zone de combat
de la carte, ou devant une entité qui en désigne une ; sinon le montage refuse, et le journal dit
pourquoi. La rencontre d'essai s'appelle `rats-du-donjon`, sur la carte `donjon` de la racine
d'essai.

Une commande que le routeur refuse ne fait rien : le Colisée ne s'ouvre pas depuis le menu
principal, parce que la table de transitions ne le prévoit pas ([Écrans, navigation et boucle de
jeu](guide-ecrans.md)). Le menu ne contourne pas la table — c'est la table qu'on vérifie.

## Le sélecteur d'écrans {#le-selecteur-d-ecrans}

Deux boutons ◀ ▶ et un libellé « n/17 · NomDÉcran », en bas de la fenêtre
(`Source/App/Game/Qml/Logic/ScreenProbe.qml`), font défiler les dix-sept noms de
`ScreenStack.screenNames` : les quinze écrans du jeu, la galerie des briques de la charte
(`Gallery`) et la galerie des assets (`AssetGallery`). Effacé tant qu'on ne s'en approche pas, il ne
prend **jamais** le clavier — les écrans se pilotent au clavier et à la manette, et un outil qui
capterait les touches empêcherait de vérifier cela même.

Il **cède la main au routeur** : sa sélection est effacée dès que le jeu navigue de lui-même
(`Connections` sur `ScreenRouter.changed`). Le menu de développement passe par lui pour ouvrir un
écran (`select(name)`) : un seul endroit épingle un écran, un seul le désépingle. Le détail est dans
[Écrans, navigation et boucle de jeu](guide-ecrans.md#les-outils-de-vérification-et-pourquoi-ils-ne-sont-pas-des-chemins-de-jeu).

## La ligne de commande {#la-ligne-de-commande}

Toutes les options sont lues par `app::commandLineOption` dans `Source/App/Game/Main.cpp` et
`Source/App/Common/Bootstrap.cpp`, à la forme `--nom=valeur`. Une valeur illisible est ignorée et
**signalée au journal**, jamais fatale (`EX-NFR-040`) : le jeu s'ouvre quand même.

| Option | Ce qu'elle fait | Build |
|---|---|---|
| `--screen=<Nom>` | ouvre l'écran nommé directement, en court-circuitant le routeur ; les noms sont ceux de `ScreenStack.screenNames` (`MainMenu`, `GameView`, `Inventory`, `AssetGallery`…) | tous |
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
- **Il ne pose pas de drapeau de monde en cours de partie** : les drapeaux se posent au lancement
  (`--flags=`), avant le premier pas, parce que la carte se compose à l'entrée et qu'un drapeau
  posé après ne referait pas ce qu'un portail ou un PNJ en a déjà tiré.
- **Il n'écrit rien dans les réglages du joueur**, à une exception nommée : le compteur de
  diagnostic est le même réglage que l'onglet Graphismes des options, retenu d'un lancement à
  l'autre, comme si on l'avait coché là.
- **Il n'est pas traduit** : ses textes sont écrits en français dans le fichier, sans `qsTr`, comme
  ceux du sélecteur et de la galerie des assets. `check_translations.py` ne les demande pas, et
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
  `hmi::GameLaunchOptions`, `hmi::dataDirectory`, `core::DEVELOPER_BUILD`.
- [`Source/App/Game/Qml/Tools/DevMenu.qml`](../../Source/App/Game/Qml/Tools/DevMenu.qml),
  [`Source/App/Game/Qml/Logic/ScreenProbe.qml`](../../Source/App/Game/Qml/Logic/ScreenProbe.qml),
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
