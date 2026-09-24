+++
id = "LOT-EDITOR-03"
titre = "Peindre avec les pièces du lieu"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "M"
resume = "La palette est la planche du lieu : poser une pièce écrit sa couche, sa pièce et sa collision en un seul geste, ce que les scripts des cartes faisaient à la place de l'éditeur."
prerequis = ["LOT-EDITOR-02", "LOT-EDITOR-12"]
livrables = [
  "`core::LevelDraft` : `placePiece`, `placePieceRegion`, `eraseLayerRegion`, `pieceAnchorAt`, `unforceCollision`, `isCollisionForced` ; la collision suit les gestes de couche, la peinture de la collision force ou libère.",
  "`PlaceAppearance::typeOfPiece`, réciproque de la table du lieu.",
  "`Editor/Logic` : `PieceCatalog` (catalogue, recherche, couche et type d'une pièce) et `BrushGesture` (pinceau de type, de pièce, gomme) ; barre d'état : pinceau armé, case forcée.",
  "`Editor/Ui` : palette à deux onglets (Pieces, Types) et gomme, canevas armé d'un pinceau, masque des cases forcées.",
  "Tests : `test_level_draft_pieces.cpp` (11), `test_piece_catalog.cpp` (6), `test_brush_gesture.cpp` (7), `test_editor_status.cpp` (+1).",
  "Le Colisée sans case forcée : `carte_colisee.py` trace la collision déduite.",
  "`EX-EDIT-063`, `EX-EDIT-064`, `EX-EDIT-065`.",
]
criteres = [
  "Repeindre une rue et une façade de Martpart sans toucher à la collision rend un fichier identique à l'original.",
  "Poser puis gommer un étal 2 × 1 occupe puis libère ses deux cases, collision comprise.",
  "Les écarts de collision forcés à la main sont montrés en masque.",
  "Une pièce absente de la planche reste dans la carte, en damier, jamais retirée.",
  "Carte sans lieu : repli sur les types en couleurs.",
]
+++

## Pourquoi

La palette est la planche du lieu. Poser une pièce écrit sa couche, sa `piece` et sa collision en
un seul geste — ce que les scripts des cartes faisaient jusqu'ici à la place de l'éditeur.

Feuille de route : [éditeur](../../../../vision/archives/feuille-de-route-editeur.md). Le lot alimente `LOT-EDITOR-04`.

## Ce que le dépôt contenait à l'ouverture (19 septembre 2026)

- **Une palette de types en couleurs** : l'éditeur peignait des types de tuile ; la pièce qu'une
  case nomme (format v4) ne se posait pas, elle ne faisait que traverser un cycle
  ouvrir/enregistrer.
- **Une collision peinte à part** : un geste sur une couche visuelle ne touchait pas la grille de
  collision, alors que `LevelEditor --check` exige depuis le `LOT-EDITOR-12` qu'elle égale la
  déduction hors cases forcées. Peindre une carte dans l'éditeur la rendait donc fausse au contrôle.
- **Des cases forcées invisibles** : le Colisée en garde 540 depuis sa migration, sans que rien ne
  les montre.

## Périmètre

### Livraison

| Partie | Contenu |
|---|---|
| `Core` | `LevelDraft` : `placePiece`, `placePieceRegion`, `eraseLayerRegion`, `pieceAnchorAt`, `unforceCollision`, `isCollisionForced` ; la collision suit les gestes de couche, la peinture de la collision force ou libère |
| Composition | `PlaceAppearance::typeOfPiece`, réciproque de la table du lieu |
| `Editor/Logic` | `PieceCatalog` (catalogue, recherche, couche et type d'une pièce), `BrushGesture` (pinceau de type, de pièce, gomme) ; barre d'état : pinceau armé, case forcée |
| `Editor/Ui` | palette à deux onglets (Pieces, Types) et gomme ; canevas armé d'un pinceau ; masque des cases forcées |
| Tests | `test_level_draft_pieces.cpp` (11), `test_piece_catalog.cpp` (6), `test_brush_gesture.cpp` (7), `test_editor_status.cpp` (+1) |
| Colisée | `carte_colisee.py` trace la collision déduite ; la carte n'a plus de case forcée (décision de l'auteur, plus bas) |

### Ce qui reste hors du lot, nommément

- **Seau, ligne, pipette, gomme en outil, miroir** : `LOT-EDITOR-04`. La gomme de ce lot est un
  pinceau de la palette, que le `LOT-EDITOR-04` fera outil.
- **Copier-coller des pièces** : le presse-papiers ne copie encore que des types ; les tampons sont
  au `LOT-EDITOR-08`.
- **Choisir la couche d'une pièce** quand une carte a plusieurs couches de décor : la composition
  du jeu ne lit que la première, la pièce y va.

## Conception

- **Le catalogue est le manifeste du lieu**, rangé par classe (sols, pièces debout, pièces larges,
  classe inconnue), sous le nom court que la carte écrit, avec une recherche sur le nom et la classe.
  Le manifeste ne porte pas d'autre libellé : « les libellés du lieu » sont ses noms de pièce, ceux
  de la disposition de l'atelier (`LOT-92`). Les consignes de dessin de la disposition, en anglais,
  ne sont pas livrées avec le jeu ; elles n'entrent pas dans la bulle d'aide, qui dit classe,
  emprise et type tactique.
- **Une pièce va sur sa couche, quelle que soit la couche active** : un sol sur la première couche
  de sol, le reste sur la première de décor — celles que la composition du jeu lit. Choisir une
  pièce rend sa couche active, pour qu'on voie où elle va et que le verrou s'y lise.
- **Le type de la case d'ancrage** est celui dont la table du lieu tire la pièce (réciproque de la
  table, `PlaceAppearance::typeOfPiece`) ; à défaut, `wall` pour une pièce debout — c'est ce que
  portent les reliefs des cartes livrées, et une pièce venue à manquer arrête encore la vue — et le
  vide pour un sol. Le type garde ainsi son seul sens de règle (décision D3).
- **Deux emprises ne se recouvrent pas sur une couche** : poser une pièce retire, entières, celles
  qu'elle couvrirait ; les autres cases de son emprise perdent leur type, qui dessinerait sinon sa
  pièce par défaut sous elle. Une pièce dont l'emprise déborderait de la carte n'est pas posée.
- **Glisser une pièce large ne la décale pas** : un pas de glisser sur une case que la même pièce
  couvre déjà ne la repose pas. Le rectangle pave au pas de l'emprise.
- **La collision suit le geste, sur ses seules cases.** Tout geste sur une couche visuelle — pièce,
  type, gomme, retrait d'une couche, cases gagnées par un agrandissement — redéduit la collision
  des cases qu'il touche, emprises comprises, hors cases forcées et hors entrée ; une case qui
  s'accorde déjà garde son écriture (`dirt` n'est pas réécrit en vide), pour qu'un geste sans effet
  ne change pas le fichier. Aucun geste ne « répare » une case qu'il n'a pas touchée.
- **Peindre la grille de collision force la case**, si la valeur peinte s'écarte de la déduction,
  et la **libère** si elle s'y accorde : forcer, c'est exactement « la main contre les pièces ».
  Une case forcée tient sous les gestes de couche.
- **La gomme libère les cases forcées** quand la collision est la couche active, plutôt qu'une
  commande de plus : gommer le forçage, c'est rendre la case à la déduction. Sur une couche
  visuelle, elle retire la pièce entière qui couvre la case, ou son type.
- **Le masque des cases forcées** — un aplat magenta et son contour, qu'aucune teinte de règle
  n'emploie — se montre quand on peint la collision, en iso comme à plat ; la barre d'état dit
  « forced collision » sur une case forcée.
- **Une pièce absente de la planche reste dans la carte** : déjà dessinée en damier par la
  composition (`LOT-EDITOR-12`), elle paraît maintenant dans la palette, groupe « Missing from the
  sheet », vignette en damier, et se pose encore. Aucun geste ne la retire que celui qui la vise.
- **Carte sans lieu** : l'onglet des pièces s'éteint et la palette des types reprend la main, en
  couleurs ; la collision se déduit alors des types, comme au contrôle.
- **La logique est pure, dans `Editor/Logic`** : `PieceCatalog` (le catalogue, la couche d'une
  pièce, son type) et `BrushGesture` (un coup de pinceau sur un rectangle : type, pièce ou gomme,
  refus compris). Le canevas ne fait que les appeler ; l'éditeur sans fenêtre (`LOT-EDITOR-13`) les
  appellera telles quelles. La pose et la collision qui suit vivent dans `core::LevelDraft`, un pas
  d'annulation par geste.
- **Le manifeste suit le brouillon** (`LevelDraft::setPieceManifest`), partagé et hors de
  l'historique : l'éditeur le lit avec la table du lieu par `loadPlaceAssets`, comme `--check`.
- **La palette prend la hauteur** à gauche dans la disposition par défaut (version de disposition
  12) : c'est l'outil qu'on regarde le plus.

## Les 540 cases forcées du Colisée — libérées

Le masque les a montrées d'un coup d'œil : l'anneau de **vide** autour de l'amphithéâtre (538
cases, ni sol ni pièce) et deux **piliers**, en (7, 10) et (7, 23). Un parcours depuis l'entrée (la
porte, en (19, 32)) les atteignait **toutes** : le héros pouvait sortir par la porte et marcher
dans le vide, héritage de la v3 que la migration avait gardé à l'identique. Aucune entité n'y était
posée. Les deux piliers se traversaient, là où ceux du couloir est arrêtent le pas.

*Décision de l'auteur, 19 septembre 2026* : **tout libérer**. Le vide est un mur, les quatre piliers
arrêtent le pas, et la collision du Colisée est exactement la déduction : plus aucune case forcée.
Le Colisée étant encore tracé par `carte_colisee.py` (jusqu'au `LOT-EDITOR-06`), c'est le script
qui trace désormais cette collision — le vide en mur, et sur une dalle la collision du type
tactique de la pièce qu'elle porte —, pour que `--migrate` ne force plus rien ; `--check` du
script et de l'éditeur sont verts. Un parcours depuis la porte atteint 632 cases, toutes sur un
sol : les 634 cases franchissables, moins les deux piliers.

## Vérification

- **Repeindre une rue et une façade de Martpart sans toucher à la collision rend un fichier
  identique à l'original.** ✔ `BrushGestureTest.RepeindreUneRueEtUneFacadeRendLeMemeFichier` :
  une case de rue (`street-2`) et un pan de façade (`window-left`) gommés puis reposés au pinceau de
  pièce ; le brouillon écrit le fichier livré, octet pour octet. Martpart ne pose pas de façade
  large (`front-left` n'y paraît pas) : le pan de mur est la façade de ses rues.
- **Poser puis gommer un étal 2 × 1 occupe puis libère ses deux cases, collision comprise.** ✔
  `BrushGestureTest.PoserPuisGommerUnEtalSurMartpart` (`feature-1` sur la place, gommé par sa
  deuxième case, fichier rendu intact) et
  `LevelDraftPiecesTest.PoserPuisGommerUnEtalOccupePuisLibereSesDeuxCases`.
- **Les écarts de collision forcés à la main sont montrés en masque.** ✔ capture de la fenêtre sur
  le Colisée, avant qu'ils ne soient libérés ;
  `LevelDraftPiecesTest.PeindreLaCollisionForceOuLibereLaCase`, `…UneCaseForceeTientPuisSeLibere`.
- **Une pièce absente de la planche reste dans la carte, en damier, jamais retirée.** ✔
  `PieceCatalogTest.LeCatalogueGroupeParClasseEtGardeLesAbsentes`,
  `…SansLieuSeulesLesPiecesCiteesRestent`.
- **Carte sans lieu : repli sur les types en couleurs.** ✔ l'onglet des pièces s'éteint (palette),
  le canevas peignait déjà les types en couleurs (`LOT-EDITOR-02`).

**Vérification à la souris, faite** au [LOT-127](../../../v0.1.0/v0.0.1-demo/lots/LOT-127-recette-de-l-editeur-a-la-main.md), le 24 septembre 2026 : sur Martpart, choisir `feature-1`, le poser sur la place,
glisser sur sa deuxième case (rien ne bouge), le gommer ; la collision active, peindre un mur sur
une rue (case magenta), puis la gommer (le magenta part). Les clics postés n'atteignent pas Qt :
cette vérification ne se fait pas sans la main de l'auteur.

## Bilan

**Livré le 19 septembre 2026** (ouvert le même jour), sur la branche `lot-editor-03-pieces`.
Vérification automatisée : construction `/W4 /WX` sans avertissement, tests unitaires verts (813),
acceptation sur Martpart, `LevelEditor --check` vert ; capture de la fenêtre (palette des pièces,
masque des cases forcées). **Vérifié à la souris** au [LOT-127](../../../v0.1.0/v0.0.1-demo/lots/LOT-127-recette-de-l-editeur-a-la-main.md) : poser, glisser et gommer dans le
canevas (voir « Vérification »).

Exigences : `EX-EDIT-063`, `EX-EDIT-064`, `EX-EDIT-065` (nouvelles) ; `EX-EDIT-002`,
`EX-EDIT-018` et `EX-EDIT-043` révisées.
