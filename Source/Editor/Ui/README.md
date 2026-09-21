# Source/Editor/Ui/

Les **widgets** de l'éditeur (`LevelEditor`, Qt Widgets), tous construits en code : style Fusion,
textes anglais, aucun formulaire `.ui` (`LOT-EDITOR-01`).

- `MainWindow` — la fenêtre : les **onglets des cartes ouvertes** au centre (`LOT-EDITOR-09`, un
  canevas par onglet, celui de l'onglet actif étant branché sur les panneaux), cinq docks
  (palette, cartes, couches, entités, mini-carte) dont la disposition est persistée
  (`EX-IHM-011`), les menus et la barre d'état. Elle tient aussi le filet de sécurité : sauvegarde
  automatique et reprise **par onglet**, garde du fichier modifié sur disque, question à la
  fermeture d'un onglet comme de la fenêtre.
- `EditorActions` — les commandes comme `QAction` uniques, partagées par la barre d'outils, les
  menus et les raccourcis remappables.
- `EditorViewport` — le canevas (`LOT-EDITOR-02`) : une `QGraphicsView` et un seul élément peint.
  En **vue iso** (défaut), il peint la scène que compose le jeu, puis les aides d'édition ; en **vue
  à plat** (`F9`), la composition de `DraftRenderer`. En **essai** (`P`), la carte est jouée par
  `hmi::WorldPlay` et peinte de même, caméra sur le héros (`EX-EDIT-055`). `F8` : reliefs en
  transparence.
- `ScenePainter` — peint une `hmi::ComposedScene` par `QPainter`, comme le GPU la dessine
  (échantillonnage au plus proche, remplissage texturé) ; rend aussi hors écran.
- `MapRender` — `LevelEditor --render` : une carte rendue en PNG, en isométrie et sans fenêtre, par
  le même peintre ; bandes et échelle au choix (`EX-EDIT-075`). `renderStamp` y rend la vignette
  d'un préfabriqué, posé sur une carte jetable de sa taille (`EX-EDIT-086`).
- `SceneImages` — les images des planches et des figurines, chargées à la demande selon les règles
  du rendu du jeu (marqueur d'une figurine absente, damier d'une pièce absente), l'atlas des types
  et les marqueurs d'entité.
- `DraftRenderer` — la vue à plat composée : une couleur par type de tuile, la collision en masque,
  les entités par leur marqueur ; le canevas y ajoute formes, étiquettes et poignées, comme en
  iso.
- `MiniMap` — toute la carte, un pixel par case, et le cadre de la vue ; un clic y recentre la vue.
- `PalettePanel` — la palette : l'onglet « Pieces » (la planche du lieu, `hmi::pieceCatalog`,
  vignettes et recherche), l'onglet « Types » (`hmi::tileTaxonomy`) et l'onglet « Prefabs » (la
  bibliothèque du lieu, `LOT-EDITOR-08`, vignettes générées par `hmi::renderStamp`). Choisir un
  préfabriqué arme le tampon du canevas.
- `LevelBrowserPanel` — la liste des cartes (recherche, état de chaque carte, filtre par état,
  vignettes), le graphe du monde et la vue de ville.
- `WorldGraphView` — le graphe du monde : les cartes, leurs portails, et **tirer d'une carte à une
  autre** pour les relier (`EX-EDIT-089`).
- `CityMapView` — une ville jouable sur son plan : les cadres de ses quartiers, ce qu'ils ouvrent,
  et ceux dont la carte manque (`EX-EDIT-090`).
- `MapPropertiesDialog` — le lieu, la région, l'ambiance et l'état d'une carte (`EX-EDIT-091`,
  `EX-EDIT-092`).
- `LayersPanel` — couche active, visibilité, opacité, grisé, verrou, ajout, retrait, ordre et
  nom.
- `EntityPanel` — famille à poser, liste filtrable des entités (identifiant, famille, étiquette,
  case ; sélection étendue), propriétés de l'entité principale, verdict d'une zone de combat et
  avertissements.

Chaque panneau garde ses widgets dans une `struct Widgets` privée, construite dans son `.cpp`.
