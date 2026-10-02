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
- `EditorViewport` — le canevas : une `QGraphicsView` au fond transparent, posée sur une
  `SceneSurface` (`LOT-1002`). En **vue iso** (défaut), la surface dessine le lieu par le rendu du
  jeu et la vue peint les aides d'édition par-dessus ; en **vue à plat** (`F9`), la vue peint la
  composition de `DraftRenderer`. En **essai** (`P`), la carte est jouée par `hmi::WorldPlay` et
  dessinée par le même rendu, caméra sur le héros comme en jeu (`EX-EDIT-055`). `F8` : reliefs en
  transparence.
- `SceneSurface` — le `QRhiWidget` qui porte `hmi::WorldSceneRenderer`, le rendu du jeu : carte,
  figurines, cadrage et opacité des calques se règlent sur lui.
- `MapRender` — `LevelEditor --render` : une carte rendue en PNG, en isométrie et sans fenêtre, par
  le même rendu, hors écran (`hmi::OffscreenRhi`) ; bandes et échelle au choix (`EX-EDIT-075`).
  `renderStamp` y rend la vignette d'un préfabriqué, posé sur une carte jetable de sa taille
  (`EX-EDIT-086`).
- `DraftRenderer` — la vue à plat, composée puis peinte par `QPainter` : une couleur par type de
  tuile, la collision en masque, les entités par leur marqueur ; le canevas y ajoute formes,
  étiquettes et poignées, comme en iso.
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
- `RunInGameDialog` — l'essai complet : d'où l'on part, et les drapeaux de monde posés avant le
  premier pas (`EX-EDIT-094`).
- `LayersPanel` — couche active, visibilité, opacité, grisé, verrou, ajout, retrait, ordre et
  nom.
- `EntityPanel` — famille à poser, liste filtrable des entités (identifiant, famille, étiquette,
  case ; sélection étendue), propriétés de l'entité principale, verdict d'une zone de combat et
  avertissements.

Chaque panneau garde ses widgets dans une `struct Widgets` privée, construite dans son `.cpp`.
