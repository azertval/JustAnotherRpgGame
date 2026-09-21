+++
id = "LOT-EDITOR-13"
titre = "L'éditeur sans fenêtre"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "S"
resume = "Ce que les scripts des cartes apportaient — l'édition en masse, rejouable, relue en diff — sans les scripts : `--apply` rejoue les gestes de la main, `--render` rend une carte en PNG, et la CI montre les cartes qu'une PR change."
prerequis = ["LOT-EDITOR-04"]
livrables = [
  "`Editor/Logic` `GestureScript` : format, état (`hmi::GestureState`), exécuteur, `applyGestureFile` ; `--apply` dans `hmi::runMapCommand` ; `hmi::mapFiles` public.",
  "Les gestes du canevas descendus dans `EditorLogic` : `placeEntityOfKind`, `applyEntityDrag`, `removeEntities` (`EntityGesture`), `pieceBrush`, `copyTypeBlock` (`PaintTools`).",
  "`Editor/Ui` `MapRender` : `renderMap`, `parseRenderLayers`, `runRenderCommand` ; `Main.cpp` : `--render` avant toute construction Qt.",
  "`Source/Test/Fixtures/Gestures/` : la carte-témoin, onze scénarios et leurs fichiers attendus, l'annexe attendue des notes, la rue de Martpart.",
  "Tests : `test_gesture_script.cpp` (5), `test_map_render.cpp` (3).",
  "CI `build-ninja` : checkout à deux commits, étape « Rendre les cartes que la PR change », artefact `map-renders`.",
  "`editeur-niveaux.md` §14, README du module et de `Ui/` ; `EX-EDIT-074` à `EX-EDIT-076`.",
]
criteres = [
  "Refaire par `--apply` une rue de Martpart rend le même fichier que le geste à la souris.",
  "Un geste refusé rend une erreur lisible et ne touche pas au fichier.",
  "Un scénario `--apply` par outil, comparé à un fichier attendu.",
  "La CI publie le rendu PNG des cartes qu'une PR change.",
]
+++

## Pourquoi

Ce que les scripts des cartes apportaient — l'édition en masse, rejouable, relue en diff (constat
A11) —, sans les scripts (décision D9). `LevelEditor --apply gestes.json` rejoue des gestes par les
fonctions mêmes des outils ; `LevelEditor --render` rend une carte en PNG ; la CI montre dans la PR
les cartes qu'elle change.

Feuille de route : [éditeur](@ref roadmap-editeur). Le lot alimente `LOT-EDITOR-06` (fin des
scripts) et `LOT-EDITOR-09` (monde).

## Ce que le dépôt contenait à l'ouverture (19 septembre 2026)

- **Deux commandes sans fenêtre**, `--check` et `--migrate` (`LOT-EDITOR-12`), dans
  `hmi::runMapCommand`, appelée par `Main.cpp` avant toute construction Qt.
- **Des outils purs, mais pas tous.** Les outils du peintre étaient des fonctions pures
  (`applyStroke`, `applyRectangleStroke`, `applyBucket`, `pickBrush`, `paintTypeBlock`), comme la
  résolution et l'aperçu des gestes d'entité (`resolveEntityPress`, `dragEntities`). Mais le canevas
  écrivait lui-même, en ligne, l'entité posée, le résultat d'un glisser, le retrait d'un groupe, la
  copie d'une sélection et le pinceau d'une pièce : un exécuteur sans fenêtre aurait dû les
  recopier.
- **Le peintre hors écran existait** (`hmi::renderComposedScene`, `LOT-EDITOR-02`), cadré par une
  caméra du jeu ; rien ne rendait une carte entière.

## Périmètre

### Livraison

| Partie | Contenu |
|---|---|
| `Editor/Logic` | `GestureScript` (format, état, exécuteur, `applyGestureFile`) ; `--apply` dans `hmi::runMapCommand` ; `hmi::mapFiles` public ; `EntityGesture` : `placeEntityOfKind`, `applyEntityDrag`, `removeEntities` ; `PaintTools` : `pieceBrush`, `copyTypeBlock` |
| `Editor/Ui` | `MapRender` (`renderMap`, `parseRenderLayers`, `runRenderCommand`) ; le canevas appelle les fonctions descendues |
| `App/Editor` | `Main.cpp` : `--render` avant toute construction Qt |
| Fixtures | `Source/Test/Fixtures/Gestures/` : la carte-témoin, onze scénarios et leurs fichiers attendus, l'annexe attendue des notes, la rue de Martpart |
| Tests | `test_gesture_script.cpp` (5), `test_map_render.cpp` (3) |
| CI | `build-ninja` : checkout à deux commits, étape « Rendre les cartes que la PR change », artefact `map-renders` |
| Documentation | `editeur-niveaux.md` §14 ; README du module (commandes sans fenêtre) et de `Ui/` |

### Ce qui reste hors du lot, nommément

- **Enregistrer ses gestes à la souris** dans un fichier `--apply` (une macro) : utile pour les
  tampons (`LOT-EDITOR-08`), pas demandé ici.
- **Les commandes qui ne sont pas des outils** — ajouter, renommer ou réordonner une couche,
  redimensionner la carte, poser l'entrée, annuler — ne se rejouent pas : les scripts à remplacer
  au `LOT-EDITOR-06` n'en ont pas besoin, les cartes existent.
- **Un schéma JSON du fichier de gestes** : le format est décrit dans l'en-tête de
  `GestureScript.h` et montré par un exemple par outil ; un schéma viendra s'il sert à un outil.
- **Rendre la vue à plat** : `--render` ne rend que l'isométrie, ce que le jeu montre.

## Conception

- **Un geste est ce que fait la main.** Un fichier de gestes ne décrit pas des modifications de
  carte, mais la main dans la fenêtre : un outil, un appui (`at`), un glisser (`path`, ou `from` et
  `to`), un relâchement, et ce qu'on arme entre deux gestes — pièce ou type (la palette), couche
  active et verrous (le panneau des couches), miroir, famille d'entité, sélection d'entités. Ce qui
  est armé le reste, comme dans la fenêtre (`hmi::GestureState`). C'est ce qui rend un geste rejoué
  égal au geste à la souris : **les mêmes fonctions, dans le même ordre**. Un format de
  modifications abstraites (« poser telle pièce en telle case ») aurait été plus court à écrire, et
  aurait eu son propre chemin de code.
- **Ce que le canevas écrivait en ligne descend dans `EditorLogic`** : `hmi::placeEntityOfKind`,
  `hmi::applyEntityDrag`, `hmi::removeEntities` (`EntityGesture`), `hmi::pieceBrush`,
  `hmi::copyTypeBlock` (`PaintTools`). Le canevas les appelle désormais, et l'exécuteur aussi.
- **Le pinceau et la gomme se rejouent case par case** : l'appui sur la première case du `path`,
  puis une case par mouvement, chacune prolongeant le trait — exactement les appels de
  `EditorViewport::paintAt`, dans un seul geste du brouillon.
- **Un geste refusé arrête tout, et rien n'est écrit.** Ce que la fenêtre refuserait (couche
  verrouillée, seau d'une pièce large, déplacement qui sortirait une entité, identifiant absent,
  famille inconnue, rien à piquer, rien à coller) rend une erreur qui nomme le geste —
  `gesture 2 (paint): The decor layer is locked.` — et le fichier n'est pas touché. Un geste qui
  ne change rien n'est pas une erreur.
- **Une case hors de la carte est un refus**, alors que la souris, elle, n'y clique simplement pas :
  dans un fichier, c'est une faute de frappe, qu'il faut dire.
- **Une pièce que la planche ne connaît pas est refusée**, sauf si le geste dit `floor` : la palette
  permet de reposer une pièce absente de la planche (`EX-EDIT-063`), mais il faut alors dire sur
  quelle couche elle va.
- **Les couches se désignent par leur nom**, `collision` pour la grille racine ; les entités par
  leur identifiant (`e12`, décision D8), jamais par leur rang.
- **Un pas par geste qui change la carte**, compté par la révision du brouillon plutôt que par la
  profondeur de l'historique, plafonnée (`EX-EDIT-058`) : un long fichier ferait sinon mentir le
  compte.
- **Les notes vont dans l'annexe**, écrite seulement si elles ont changé ; une annexe vidée retire
  son fichier (`hmi::writeSidecar`, `EX-EDIT-068`). La mesure s'écrit dans le compte rendu.
- **`--apply` écrit en place**, comme `--migrate` ; `--output` écrit ailleurs, et l'annexe suit la
  carte écrite. Suivi de `--check`, il contrôle ensuite toutes les cartes.
- **`--render` n'a besoin ni de fenêtre ni de `QApplication`** : `QImage` et `QPainter` sur une
  image suffisent, et le peintre n'écrit aucun texte. L'échelle 1 est celle de l'art (un pixel
  d'image par pixel de planche, `Camera2D` au zoom 1) : Martpart fait 3 956 × 2 592 pixels. Les
  bandes par défaut sont celles que le jeu montre ; `--layers …,collision` ajoute le masque du
  canevas. Une carte sans lieu se peint par les couleurs de ses types, comme dans le canevas.
- **La CI rend les cartes que la PR ajoute ou change** (`git diff HEAD^1 HEAD`, d'où un checkout à
  deux commits), à demi-échelle, et les publie dans l'artefact `map-renders`, listées dans le
  résumé du job. Les annexes et les séquences n'en sont pas.
- **Les scénarios sont les tests d'IHM** (règle 4) : un par outil, rejoué sur une carte-témoin de
  12 × 10 cases sur la planche de Martpart (`Source/Test/Fixtures/Gestures/terrain.json`), comparé
  octet pour octet à `<outil>.attendu.json`. Un changement voulu se régénère par
  `LevelEditor --apply … --output <outil>.attendu.json`, puis se relit en diff.

## Vérification

- **Refaire par `--apply` une rue de Martpart rend le même fichier que le geste à la souris.** ✔
  `GestureScriptTest.UneRueDeMartpartRefaiteRendLaCarteLivree` : la rue d'Arenarea (lignes 2 à 5,
  colonnes 0 à 16) est gommée couche par couche, puis retracée en dix gestes — un rectangle de pavés,
  un trait par variante de pavé et pour les seuils, une ligne de façade, un trait de fenêtres, de
  portes et de lanternes. Le fichier rendu égale `martpart.json` **octet pour octet** : les outils
  posent la pièce, le type de sa case et la collision comme la carte livrée les porte. Que ce soit
  aussi le fichier du geste à la souris tient à la construction : la souris et `--apply` appellent
  les mêmes fonctions, dans le même ordre. La main de l'auteur ne se simule pas (les clics postés
  n'atteignent pas Qt, voir le `LOT-EDITOR-04`).
- **Un geste refusé rend une erreur lisible et ne touche pas au fichier.** ✔
  `UnGesteRefuseRendUneErreurLisible` (couche verrouillée, case hors de la carte, pièce inconnue,
  identifiant absent, groupe qui sortirait, outil inconnu, autre format) ;
  `UnGesteRefuseNeTouchePasAuFichier` (code 1, « nothing written », aucun fichier).
- **Un scénario `--apply` par outil, comparé à un fichier attendu.** ✔
  `UnScenarioParOutilRendLeFichierAttendu` : pinceau, gomme, rectangle, ligne au miroir, seau,
  pipette, sélection (copier, coller, `Suppr`), entité (poser, retirer, renommer, `Maj` + clic,
  groupe déplacé, zone tirée, glisser), forme (peindre, gommer, point ajouté, glissé, retiré),
  mesure, notes ; chacun avec son nombre de pas d'annulation.
- **La CI publie le rendu PNG des cartes qu'une PR change.** Étape écrite, commande éprouvée au
  poste (`--render Source/Elements/Levels/capital/martpart.json` rend `capital-martpart.png`) ;
  **à voir tourner à la première PR qui touche une carte** — celle du lot n'en touche aucune.

**Vérification à la souris, due** : les gestes que le canevas écrivait en ligne passent maintenant
par les fonctions descendues. Sur Martpart : outil Entité, poser un coffre, le glisser, tirer une
zone de combat, `Maj` + clic sur une deuxième entité et déplacer le groupe, `Suppr` ; outil
Sélection, `Ctrl+C` puis `Ctrl+V` sur la collision ; choisir une pièce dans la palette (la couche
active suit). Chaque geste se défait d'un `Ctrl+Z`.

## Bilan

**Livré le 19 septembre 2026** (ouvert le même jour), sur la branche
`lot-editor-13-sans-fenetre`. Vérification automatisée : construction `/W4 /WX` sans
avertissement, tests unitaires verts (848, dont deux ignorés comme avant), acceptation sur
Martpart, rendu des trois cartes livrées relu à l'œil. **Vérification à la souris due** sur les
gestes d'entité et la copie (voir « Vérification ») ; **étape CI de rendu éprouvée à la première
PR** qui change une carte.

Exigences : `EX-EDIT-074` à `EX-EDIT-076` (nouvelles).
