# Source/Editor/

Le **module éditeur de cartes** (`LOT-EDITOR-01`) : un outil interne, fait pour l'auteur seul, qui
sert à fabriquer les cartes du jeu. Son programme est la
[feuille de route de l'éditeur](../../Planning/vision/archives/feuille-de-route-editeur.md) ; sa spécification,
[`editeur-niveaux.md`](../../Documentation/Specification/editeur-niveaux.md).

Le module dépend de `Core` (modèle et validation de carte, manifeste des pièces), de
`SceneComposition` (la composition d'un lieu, sans GPU, partagée avec le jeu) et de `HmiLib` (carte
jouée par l'essai). **Rien ne dépend de lui** : ni le jeu, ni `HmiLib`. Depuis le `LOT-EDITOR-02`,
l'éditeur ne parle plus au GPU : il peint la composition du jeu par `QPainter`.

| Dossier | Contenu | Cible |
|---|---|---|
| [`Logic/`](Logic/) | La logique pure : sans Qt, testée sous `Source/Test/Unit/Editor` | `EditorLogic` (bibliothèque statique, liée par `UnitTests`) |
| [`Ui/`](Ui/README.md) | Les widgets, construits en code : fenêtre, canevas, panneaux | `LevelEditor` (point d'entrée : `Source/App/Editor/Main.cpp`) |

Faire une carte de bout en bout : le
[guide d'usage](../../Documentation/Guide/Manuel/utiliser-l-editeur.md).

## Règles du module

- **Outil interne.** Style Fusion de Qt, icônes standard ou libellés texte, **textes anglais écrits
  dans le code** : ni charte, ni thème, ni catalogue de traduction, ni formulaire `.ui`.
- **Le brouillon est la seule source.** `core::LevelDraft` porte toute la carte ; le canevas en est
  le seul propriétaire, les panneaux demandent et il applique.
- **Tout ce qui a une règle est pur et testé** dans `Logic/` ; l'IHM ne fait que l'afficher.
- **Aucun travail perdu.** Sauvegarde automatique et reprise (`EX-EDIT-056`), garde du fichier
  modifié sur disque (`EX-EDIT-057`), historique plafonné et « modifié » qui suit le contenu
  (`EX-EDIT-058`).
- **On édite ce qu'on jouera.** Le canevas peint la liste de primitives que compose le jeu, et son
  image égale celle du GPU à une tolérance près (`EX-EDIT-059`).
- **On pose des pièces, la collision suit.** La palette est la planche du lieu ; une pièce écrit sa
  couche, sa pièce et sa collision en un geste, et seule la main force une case (`EX-EDIT-063` à
  `EX-EDIT-065`).
- **Un geste, un pas.** Du clic au relâchement, tout outil — trait, ligne, seau, rectangle, reflet
  du miroir — se défait d'un seul `Ctrl+Z` (`core::GestureScope`, `EX-EDIT-066`).
- **La forme, pas le type.** Une entité se dessine et se manipule par la forme que sa famille
  déclare dans `core::knownEntityKinds` ; aucune famille n'a de code dans le module, et un test
  bloque toute famille lue par le jeu et absente de la table (`EX-EDIT-070`, `EX-EDIT-073`).
- **L'éditeur fait foi pour les cartes** (décision D4, `LOT-EDITOR-06`) : il ouvre l'arbre des
  sources, jamais la copie de la construction, et aucun script n'écrit plus dans `Levels/`
  (`EX-EDIT-078`).
- **Un contrôle, deux entrées.** `--check` et le panneau « Problems » appellent le même
  `hmi::checkAllMaps`, sur toutes les cartes ; un constat nomme sa carte, sa case et son entité, et
  le panneau y mène (`EX-EDIT-079`, `EX-EDIT-080`).
- **Ce qu'on a composé une fois se repose.** Une sélection se copie entière — couches, pièces
  ancrées, entités, cases forcées —, se pose en un pas d'annulation avec des identifiants neufs,
  se reflète, et s'enregistre comme préfabriqué du lieu (`EX-EDIT-085`, `EX-EDIT-086`). Une carte
  neuve part d'un modèle (`EX-EDIT-087`).
- **Plusieurs cartes à la fois** (`LOT-EDITOR-09`) : un onglet par carte, chacun avec son
  brouillon, son historique, sa sauvegarde automatique et sa garde du fichier ; ouvrir n'écrase
  rien, c'est **fermer** qui demande quoi faire d'un brouillon modifié (`EX-EDIT-088`).
- **Le monde s'écrit au geste** : tirer d'une carte à une autre sur le graphe pose la paire
  portail / point d'arrivée **des deux côtés**, par un plan montré avant d'être écrit
  (`EX-EDIT-089`) ; la ville se voit par quartiers (`EX-EDIT-090`) ; une carte porte sa région et
  son ambiance (`EX-EDIT-091`), et son annexe dit où elle en est (`EX-EDIT-092`).
- **L'essai complet** (`LOT-EDITOR-10`) : *Run in game* lance le **vrai jeu** sur la carte
  ouverte, à la case voulue et dans l'état de partie voulu (`EX-EDIT-093`, `EX-EDIT-094`) ; les
  brouillons de tous les onglets sont posés dans un dossier temporaire, que le jeu sert avant ses
  propres cartes (`EX-EDIT-095`).
- **Deux façons d'éditer, un seul chemin.** La souris et `--apply` appellent les mêmes fonctions
  pures, dans le même ordre ; un scénario `--apply` par outil, comparé à un fichier attendu, tient
  lieu de test d'IHM (`EX-EDIT-074`, `EX-EDIT-076`).

## Logique pure (`Logic/`)

- `TileTaxonomy` — l'arbre catégories/tuiles de la palette des types, libellés compris.
- `PieceCatalog` — le catalogue des pièces d'un lieu (groupes par classe, recherche, pièces
  absentes de la planche), la couche où va une pièce et le type qu'elle écrit (`EX-EDIT-063`).
- `BrushGesture` — un coup de pinceau sur un rectangle : un type, une pièce ou la gomme, refus
  compris ; ce que la souris appelle, et ce qu'appellera l'éditeur sans fenêtre (`EX-EDIT-064`,
  `EX-EDIT-065`).
- `PaintTools` — les outils du peintre : trait, rectangle, ligne, seau, pipette, miroir et mesure,
  chacun en un geste du brouillon (`EX-EDIT-066`, `EX-EDIT-067`, `EX-EDIT-069`).
- `EditorSidecar` — l'annexe d'une carte, `<carte>.editor.json`, et ses notes d'auteur
  (`EX-EDIT-068`).
- `MapFormat` — `--migrate` et `--check`, la garde du format v4 (`EX-EDIT-062`), et l'entrée des
  commandes sans fenêtre (`hmi::runMapCommand`).
- `GestureScript` — `--apply` : un fichier de gestes rejoué par les fonctions des outils, l'état
  que la fenêtre garde d'un geste à l'autre, et le refus lisible d'un geste (`EX-EDIT-074`).
- `LevelFileOperations`, `LevelNameValidation` — créer (avec son lieu, `EX-EDIT-077`), renommer,
  dupliquer, supprimer une carte ; son nom est une clé, que les catalogues reçoivent (`EX-EDIT-081`).
- `Stamps` — les **tampons** : découper un rectangle de carte entier, le reposer, le refléter, et
  la bibliothèque de préfabriqués et de modèles de carte du lieu (`EX-EDIT-085` à `EX-EDIT-087`).
- `MapDocuments` — les cartes ouvertes en onglets : le libellé d'un onglet, celui qu'une
  ouverture vise, celui qui revient après une fermeture (`EX-EDIT-088`).
- `WorldLinks` — relier deux cartes : le nom du point d'arrivée, les cases où poser la paire, et le
  plan des deux fichiers à récrire (`EX-EDIT-089`).
- `CityView` — la vue de ville : les quartiers d'une ville jouable joints à leurs cadres et à
  l'atlas, et celui que désigne un point du plan (`EX-EDIT-090`).
- `MapRefactor` — renommer et remplacer d'un bout à l'autre du projet : qui cite une carte, un
  point d'arrivée, une entité, une pièce ; le plan de chaque fichier à récrire, calculé avant d'en
  écrire un (`EX-EDIT-082`, `EX-EDIT-083`, `EX-EDIT-084`).
- `ContentCheck` — le contrôle du contenu, après celui du format : références, rencontres,
  atteignabilité, portails sans retour, points d'arrivée orphelins, textes (`EX-EDIT-079`).
- `MapTexts` — la clé du nom d'une carte, et les catalogues de traduction qu'on lit et complète
  (`EX-EDIT-081`).
- `GameLaunch` — l'essai complet : le dossier temporaire des brouillons joués, leur écriture, et
  le jeu cherché à côté de l'éditeur (`EX-EDIT-095`).
- `DataRoot` — la racine des données que l'éditeur ouvre : `--data`, sinon l'arbre des sources qui
  l'a construit, sinon le dossier de l'exécutable (`LOT-EDITOR-06`).
- `EditorTool`, `PanelFocus` — l'outil actif et le panneau qu'il met en avant.
- `EntityShapes` — les entités à forme : rectangle et poignées, zone peinte, trajet, entité sous
  le curseur, liste filtrable (`EX-EDIT-070`, `EX-EDIT-072`).
- `EntityGesture` — les gestes des outils « Entité » et « Forme » : prendre, basculer, poser, tirer
  une zone, déplacer un groupe, tirer une poignée, peindre une zone, tracer un trajet ; l'aperçu et
  l'écriture sont la même fonction (`hmi::dragEntities`).
- `EntityReferences`, `EditorDiagnostics` — les catalogues que les entités citent (dialogues,
  rencontres, cartes, figurines, drapeaux, lieux, objets, `carte#id`), et les avertissements rendus
  en anglais, verdict des zones de combat compris (`EX-EDIT-071`, `EX-EDIT-073`).
- `LayerView` — les couches telles que l'éditeur les montre : visibles, opacité, grisées,
  verrouillées (`EX-EDIT-061`), et l'étage d'une couche de décor (`EX-LVL-025`, `LOT-129`) : le
  panneau des couches le règle (« Floor »), le pinceau à pièces peint la couche d'étage active, et
  un préfabriqué garde l'étage de ses couches.
- `CanvasPicking` — le pointage du canevas, iso et à plat : la case sous un point, par son losange,
  hauteur en paramètre ; les cases visibles d'un cadrage (`EX-EDIT-060`).
- `CanvasScene` — l'instantané que le canevas compose (celui du jeu, PNJ compris, sans héros) et
  l'opacité de chaque bande de la scène selon les couches (`EX-EDIT-059`, `EX-EDIT-061`).
- `EditorStatus`, `EditContextTarget` — la barre d'état, la cible des commandes d'édition.
- `WorldGraphLayout` — la disposition du graphe du monde.
- `ThumbnailGeometry` — les vignettes à l'échelle d'affichage réelle.
- `EditorKeyBindings` — les raccourcis remappables, persistés en JSON.
- `Autosave` — les brouillons de reprise (`%LOCALAPPDATA%\JustAnotherRpgGame\Editor\autosave`) et
  les versions mises de côté (`conflicts\`).
- `DiskGuard` — l'empreinte d'un fichier et la réaction à son changement.

## Sans fenêtre

Ces commandes rendent la main aussitôt, sans `QApplication` ; elles tournent en CI. `--data`
désigne la racine des données ; par défaut, comme la fenêtre, l'éditeur ouvre le `Source/Elements`
de l'arbre qui l'a construit, et à défaut le dossier de l'exécutable (`hmi::resolveDataRoot`).

| Commande | Ce qu'elle fait |
|---|---|
| `LevelEditor --data Source/Elements --check` | Contrôle le format et le contenu de toutes les cartes ; sort en 1 s'il y a une erreur (`EX-EDIT-062`, `EX-EDIT-079`). |
| `LevelEditor --migrate [carte…] [--output f]` | Convertit en v4 canonique (`EX-EDIT-062`). |
| `LevelEditor --apply gestes.json [carte] [--output f]` | Rejoue les gestes du fichier ; un geste refusé n'écrit rien (`EX-EDIT-074`). |
| `LevelEditor --render [carte…] [--output f.png\|dossier] [--layers floors,relief,figures,collision] [--scale s]` | Rend en PNG, en isométrie (`EX-EDIT-075`). L'échelle 1 est la carte vue à 1080p (une case à 100 pixels), 2 à 2160p ; le cadre est ce qui est peint, reliefs hauts compris ; l'image ne dépasse jamais 8 192 pixels de côté, l'échelle se réduisant pour y tenir (`LOT-125`). |
| `LevelEditor --list-prefabs [lieu…]` | Liste les préfabriqués d'un lieu, de tous les lieux à défaut (`EX-EDIT-086`). |
| `LevelEditor --save-prefab <carte> <nom> --from <c,r> --to <c,r>` | Découpe le rectangle et l'écrit comme préfabriqué du lieu de la carte (`EX-EDIT-086`). |
| `LevelEditor --link-maps <carte> <carte>` | Relie deux cartes : le portail et le point d'arrivée des deux côtés ; refusé, n'écrit rien (`EX-EDIT-089`). |
| `LevelEditor --who-cites map <carte>` (ou `arrival <carte> <point>`, `entity <carte> <id>`, `piece <pièce>`) | Liste ce qui cite, sans rien écrire (`EX-EDIT-082`). |
| `LevelEditor --rename-map <ancien> <nouveau>` | Renomme une carte, dossier compris, et tout ce qui la cite ; refusé, n'écrit rien (`EX-EDIT-082`). |
| `LevelEditor --rename-arrival <carte> <ancien> <nouveau>`, `--rename-id <carte> <ancien> <nouveau>` | Renomme un point d'arrivée, un identifiant d'entité, et ce qui les cite (`EX-EDIT-082`). |
| `LevelEditor --replace-piece <ancienne> <nouvelle> [carte…]` | Remplace une pièce sur les cartes nommées, toutes celles qui la posent à défaut (`EX-EDIT-083`). |
| `LevelEditor --change-scene <carte> <lieu> [--table table.json]` | Fait passer une carte à une autre planche ; la table (`jadg-piece-table`, version 1, `"pieces": {"ancienne": "nouvelle"}`) donne les pièces sans homonyme (`EX-EDIT-084`). |

Suivie de `--check`, une commande de renommage ou de remplacement contrôle ensuite toutes les
cartes.

Un fichier de gestes (`jadg-editor-gestures`, version 1) décrit ce que la main ferait : l'outil,
l'appui (`at`), le glisser (`path`, ou `from` et `to`), et ce qu'on arme entre deux gestes (`piece`,
`type`, `layer`, `lock`, `mirror`, `kind`, `select`). Le format complet est dans l'en-tête de
`Logic/GestureScript.h` ; un exemple par outil dans `Source/Test/Fixtures/Gestures/`, et une rue de
Martpart entière dans `martpart-rue.json`.

## Le canevas en HD

Depuis le `LOT-125`, le canevas, les vignettes et `--render` peignent l'art HD comme le jeu :

- **Lissé** : l'art peint se lit en bilinéaire, sur le **niveau réduit** que l'échelle demande
  (`hmi::SceneImage::level`, la moitié de l'image à chaque niveau) — `QPainter` n'a pas de
  mipmaps. Les images engendrées (marqueurs, jetons, atlas, damier) restent au plus proche, comme
  en jeu. La parité exacte avec le GPU n'est plus promise : il mêle deux niveaux (trilinéaire), le
  peintre n'en lit qu'un. `test_scene_painter.cpp` en écrit les seuils, mesurés.
- **Entier** : le cadre du canevas, des vignettes et de `--render` se mesure sur ce qui est peint
  (`hmi::composedSceneBounds`), et non sur une marge d'un losange.
- **Borné** : un seul cache d'images par dossier d'assets (`hmi::SceneImages::shared`), que les
  onglets, les vignettes de la liste des cartes et celles des préfabriqués se partagent. Chaque
  manifeste s'y lit une fois.

### La borne de mémoire

**Les pixels de l'art peint, niveaux réduits compris, tiennent en 256 Mio**
(`SCENE_IMAGES_DEFAULT_BUDGET_BYTES`), quel que soit le nombre d'onglets ouverts : au-delà, la
pièce la moins récemment peinte est évincée, et se relit sur disque à la peinture suivante. Seules
les images engendrées, de quelques kibioctets chacune, restent hors budget. Un kit de zone HD pèse
moins de 40 Mio installé (budget de zone du `LOT-104`) ; décompressé en mémoire, il tient
largement dans la borne. `test_scene_images.cpp` prouve le partage, le budget et la relecture.

Mesure de la peinture : `CanvasBenchmarks` (`Source/Benchmark/bench_canvas_paint.cpp`), publiée
chaque nuit. Sur le poste de référence, en Release, la maquette HD se peint en 11,5 ms à 1080p et
en 4,8 ms dézoomée (une case à 25 pixels), le 23 septembre 2026.

## Fichiers du poste

| Chemin | Contenu |
|---|---|
| `%LOCALAPPDATA%\JustAnotherRpgGame\Editor\autosave\*.autosave.json` | Brouillon en attente de reprise, un par carte ; retiré à l'enregistrement et à la fermeture voulue. |
| `%LOCALAPPDATA%\JustAnotherRpgGame\Editor\autosave\conflicts\` | Versions écartées par un choix (reprise refusée, disque relu ou gardé de côté). Jamais nettoyé automatiquement. |

`LevelEditor --crash-test` plante juste après la première sauvegarde automatique : c'est la façon
d'éprouver la reprise.

## Fichiers à côté des cartes

| Chemin | Contenu |
|---|---|
| `Editor/Prefabs/<lieu>/<nom>.json` | Un **préfabriqué** du lieu (`jadg-editor-prefab`, version 1) : un morceau de carte gardé, que la palette montre et que `Ctrl+V` repose. |
| `Editor/Templates/<id>.json` | Un **modèle de carte** (`jadg-editor-map-template`, version 1) : couches, taille, entrée, et ce qu'il pose. Il ne nomme aucune pièce. |
| `Levels/<carte>.editor.json` | L'annexe de la carte : ses notes d'auteur, une par case. Le jeu ne la lit jamais, aucune liste de cartes ne la prend pour une carte ; elle suit la carte qu'on renomme, duplique ou supprime. |

## Touches des outils

| Touche | Outil |
|---|---|
| `B` · `R` · `L` · `G` | pinceau, rectangle, ligne, seau |
| `E` · `I` | gomme, pipette (`Alt` + clic : pipette depuis tout outil) |
| `S` · `O` | sélection (`Suppr` la gomme), entité (`Maj` + clic : sélection multiple, `Suppr` retire) |
| `Z` | forme : peindre la zone sélectionnée (`Ctrl` gomme), tracer le trajet sélectionné |
| `D` · `N` | mesure (5 pieds la case), note d'auteur |
| `M` | miroir, par la case survolée |
| `Ctrl+C` · `Ctrl+V` · `Ctrl+Maj+V` | copier la sélection en tampon, la poser, la poser reflétée |
| `Ctrl+Maj+S` | enregistrer la sélection comme préfabriqué |
| `P` · `Shift+P` | essai depuis l'entrée, depuis la case survolée |

Pour ouvrir le nouveau Colisée, utiliser
`LevelEditor --data <Source/Elements> --map=capital/arena-of-brave` (`--map=` remplace la carte de
départ dans l'onglet du démarrage).
`--screenshot=<fichier.png>` capture la fenêtre puis quitte.
Voir le [guide Arena of Brave](../../Documentation/arena-of-brave-map.md).
