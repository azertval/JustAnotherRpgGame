+++
id = "LOT-EDITOR-02"
titre = "Le canevas montre le lieu"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "L"
resume = "On édite sur le lieu rendu en isométrie comme dans le jeu, avec une bascule vers la vue à plat et un pointage juste partout, y compris sous un mur haut."
prerequis = ["LOT-EDITOR-01"]
livrables = [
  "La cible `SceneComposition` (`ComposedScene`, `PlaceAppearance`, `WorldSceneComposer` sans Qt ni GPU), `hmi::WorldSceneSource` et `hmi::npcFigures`.",
  "`core::ScenePieceManifest` et ses tests : le manifeste des pièces descend dans `Core`, la galerie des assets le lit par là.",
  "`EditorViewport` en `QGraphicsView`, `ScenePainter`, `SceneImages`, `DraftRenderer` réduit à la composition à plat.",
  "Le pointage (`CanvasPicking`) et les bandes (`CanvasScene`), testés ; hauteur en paramètre (`ELEVATION_STEP_DIAMONDS`).",
  "Calques grisés et verrouillés, reliefs en transparence (`F8`), bascule iso / à plat (`F9`), mini-carte (dock « Overview »), barre d'état.",
  "`ScenePainterTest` (image du canevas comparée au rendu GPU du jeu) et la mesure `ComposeMartpart` (`Source/Benchmark/bench_canvas.cpp`).",
  "`EX-EDIT-059`, `EX-EDIT-060`, `EX-EDIT-061`.",
]
criteres = [
  "Martpart ouverte dans l'éditeur produit la même liste de primitives que dans le jeu.",
  "Son rendu hors écran égale, à une tolérance près, celui du jeu.",
  "Le pointage est juste aux quatre coins de la carte et sous un mur haut.",
  "Les gestes et l'essai du `LOT-11` marchent en iso.",
]
+++

## Pourquoi

On édite sur le lieu rendu en isométrie comme dans le jeu, avec une bascule vers la vue à plat
(décisions D1 et D2 de la feuille de route). Le pointage est juste partout, y compris sous un mur
haut, et prend la hauteur en paramètre (D11).

Feuille de route : [éditeur](@ref roadmap-editeur). Le lot alimente `LOT-EDITOR-03` et
`LOT-EDITOR-05`.

## Ce que le dépôt contenait à l'ouverture (18 septembre 2026)

- **Le canevas était un `QRhiWidget`** qui dessinait le brouillon **à plat** — une couleur par type
  de tuile — par `hmi::SpriteBatch`. Le lieu ne se voyait que pendant l'essai, par
  `hmi::WorldSceneRenderer` : on éditait des couleurs et on vérifiait en jouant.
- **La composition vivait dans `HmiLib`** (constat A8), la bibliothèque qui porte aussi la
  localisation, les écrans et les entrées ; elle était déjà pure, sans GPU ni Qt.
- **Le manifeste des pièces n'était lu que par la galerie des assets**, dans `HMI` (A9) ; l'emprise
  d'une pièce n'était connue d'aucune règle.
- **La composition ne lisait qu'une `core::Level` validée** ; le brouillon de l'éditeur, souvent
  invalide en cours de tracé, ne pouvait pas être composé.

## Périmètre

### Livraison

| Commit | Contenu |
|---|---|
| 1 — phase 1 | cible `SceneComposition`, `core::ScenePieceManifest` et ses tests, `WorldSceneSource`, `npcFigures` ; la galerie lit le manifeste par `Core` ; le jeu ne change pas |
| 2 — phases 2 et 3 | `EditorViewport` en `QGraphicsView`, `ScenePainter`, `SceneImages`, `DraftRenderer` réduit à la composition à plat, pointage (`CanvasPicking`) et bandes (`CanvasScene`) testés, calques grisés et verrouillés, reliefs en transparence, mini-carte, barre d'état ; `ScenePainterTest` ; mesure `ComposeMartpart` |
| 3 — documentation | le dossier du lot, la feuille de route, `editeur-niveaux.md`, les guides et les README |

### Ce qui reste hors du lot, nommément

- **Les entités comme éléments de la scène**, leurs poignées et leurs liens : `LOT-EDITOR-05`.
- **La palette des pièces** et la pose d'une pièce en un geste : `LOT-EDITOR-03`. Le canevas montre
  les pièces, il ne les pose pas encore.
- **Un coup de pinceau reste un pas d'annulation par case** : les outils du `LOT-EDITOR-04`.
- **La composition n'est pas bornée au cadrage** : toute la carte est recomposée après un geste
  (0,5 ms sur Martpart), seule la peinture est bornée au visible. À revoir si la borne de 128 × 128
  cases (§5, règle 5) le demande.
- **L'agrandissement n'est pas entier** en édition : à un facteur fractionnaire, le pixel art
  s'échantillonne au plus proche, inégalement. L'essai, lui, prend l'agrandissement entier du jeu.

## Conception

- **La composition sort de `HmiLib` par la cible, pas par les chemins.** `SceneComposition` compile
  `ComposedScene`, `PlaceAppearance` et `WorldSceneComposer` sans Qt ni GPU ; `HmiLib` la lie en
  `PUBLIC`, le jeu ne change pas. Les fichiers restent sous `HMI/Graphics`, dans l'espace de noms
  `hmi` : déplacer quarante inclusions n'aurait rien apporté au lot (même parti que l'espace de noms
  au `LOT-EDITOR-01`).
- **Le manifeste descend dans `Core`** (`core::ScenePieceManifest`) : classe, emprise, ancre,
  taille, miroir, par nom court de pièce. La galerie le lit par là. `PlaceAppearance` reste dans la
  composition : c'est la table que le composeur lit, et le `LOT-EDITOR-12` en fera le défaut des
  cartes générées.
- **La composition lit une carte ou un brouillon par les mêmes accesseurs**
  (`hmi::WorldSceneSource`, `hmi::worldSceneSource`). Le canevas compose donc le brouillon tel
  qu'il est, sans validation, par le chemin même du jeu. Les figurines des PNJ sortent de
  `WorldPlay` (`hmi::npcFigures`) : le canevas les montre, sans le héros ni l'animation.
- **Une `QGraphicsView`, un seul élément peint.** Il parcourt la `ComposedScene` triée et ne peint
  que les primitives qui coupent le rectangle exposé ; les aides d'édition (quadrillage en
  losanges, case survolée, masque de collision, aperçu des outils, terrain de rencontre) sont
  peintes par-dessus, sur les seules cases visibles. **Écart à D2, nommé** : les entités sont
  encore des marqueurs peints par cet élément, pas des `QGraphicsItem` ; elles en deviennent au
  `LOT-EDITOR-05`, qui leur donne figurines et poignées.
- **L'éditeur ne parle plus au GPU.** L'essai immédiat compose la carte jouée par `WorldPlay` et le
  même peintre la dessine, caméra sur le héros, à l'agrandissement entier du jeu. `SpriteBatch`,
  `TextureAtlas`, `TextureCache`, `SceneResources`, `WorldSceneRenderer`, les shaders et
  `Qt6::GuiPrivate` quittent `LevelEditor`. `EX-EDIT-055` tient : la mise en scène est celle du
  jeu, et l'image est prouvée égale à celle du GPU (ci-dessous).
- **Remplissage texturé plutôt que `drawImage`.** `QPainter::drawImage` agrandi décale
  l'échantillonnage d'un demi-pixel et ouvrait des jours entre les losanges du sol : 2 % des pixels
  différaient du jeu sur la place de Martpart. Un `fillRect` avec un pinceau texturé échantillonne
  au centre des pixels, comme le GPU : 0,06 % au pire.
- **L'égalité avec le jeu se prouve contre le rendu du jeu, pas contre un PNG gelé.**
  `ScenePainterTest` rend Martpart (trois cadrages) et le Colisée (un) par
  `hmi::WorldSceneRenderer` hors écran, puis par le peintre de l'éditeur avec la même caméra, et
  compare : moins de 0,5 % des pixels peuvent différer de plus de 48 sur un canal. Une référence
  commitée vieillirait à chaque planche réextraite ; le rendu du jeu est la référence qui ne ment
  pas. Les deux images sont écrites dans `editor-captures/` pour être relues.
- **La hauteur (D11)** : un niveau d'élévation vaut une hauteur de losange
  (`ELEVATION_STEP_DIAMONDS`). Le pointage et le losange d'une case la prennent en paramètre ; tout
  appelant passe 0.
- **Le masque de collision, en iso, ne se montre que quand on peint la collision**, et à la moitié
  de l'opacité de la vue à plat : ailleurs, il couvrirait le lieu qu'on vient voir. La vue à plat
  reste celle qui lit la collision (D1).
- **Calques** : une couche **grisée** se peint à 30 % de son opacité ; une couche **verrouillée**
  se voit mais refuse pinceau, rectangle et collage, et la barre d'état le dit. Les réglages d'une
  couche agissent sur sa bande de la scène (sol, relief), comme la composition les fond. Aides
  d'édition : rien n'est enregistré.
- **Mini-carte** (dock « Overview ») : la matière de chaque case (première couche de sol, à défaut
  la grille racine), les obstacles foncés, et le cadre de la vue — un losange en iso. Un clic ou un
  glisser y recentre la vue.
- **Carte sans lieu** : ses types en couleurs, en losanges. C'est le repli que le
  `LOT-EDITOR-03` généralisera.
- **Raccourcis** : `F9` bascule iso / à plat, `F8` les reliefs en transparence. Le canevas prend le
  clavier au lancement : sans cela, la recherche au clavier de la palette avalait `P`.
- **Agrandissement continu** par crans de 1,25 à la molette, ancré sous le pointeur, de la carte
  entière jusqu'à 8 × ; le bouton droit déplace la vue. La barre d'état montre les pièces de la case
  survolée (`street · wall-left`) et la vue.

## Vérification

- **Martpart ouverte dans l'éditeur produit la même liste de primitives que dans le jeu.** ✔
  `CanvasSceneTest.MartpartSeComposeCommeDansLeJeu` : mêmes instantanés, héros mis à part ; mêmes
  primitives, dans le même ordre.
- **Son rendu hors écran égale, à une tolérance près, celui du jeu.** ✔
  `ScenePainterTest.MartpartPeinteEgaleLeRenduDuJeu` et `…LeColiseePeintEgaleLeRenduDuJeu` :
  0,06 % des pixels au pire hors tolérance, ancres, échelle des pièces et miroirs compris.
- **Le pointage est juste aux quatre coins de la carte et sous un mur haut.** ✔
  `CanvasPickingTest.LePointageEstJusteAuxQuatreCoins`, `…SousUnMurHautOnPointeLaCaseDerriere`,
  `…LaHauteurSePrendEnParametre`.
- **Les gestes et l'essai du `LOT-11` marchent en iso.** ✔ à l'écran pour l'essai (lancé depuis
  le canevas iso : héros à la porte du Colisée, caméra qui suit) ; ✔ en automatique pour ce que les
  gestes lisent (pointage, bandes, verrou). **À vérifier à la main**, les clics simulés n'atteignant
  pas Qt sur ce poste : peindre, rectangle, sélection, copier/coller, poser et déplacer une entité
  en vue iso, puis la même chose en vue à plat ; peindre une couche verrouillée (refus) ; cliquer la
  mini-carte.

**Mesure** (`Source/Benchmark/bench_canvas.cpp`, Release) : instantané et composition triée de
Martpart, 1 172 primitives, **0,5 ms** — ce que le canevas refait après chaque geste.

## Bilan

**Livré le 18 septembre 2026** (ouvert le même jour), sur la branche
`lot-editor-02-canevas-iso`, en trois commits : la composition hors de `HmiLib` et le manifeste
dans `Core` (phase 1), le canevas iso avec ses calques et sa mini-carte (phases 2 et 3), puis la
documentation. Vérification automatisée : construction `/W4 /WX` sans avertissement, `ctest`
vert, image du canevas comparée au rendu GPU du jeu sur quatre cadrages. Relu à l'écran :
`LevelEditor` lancé et capturé (vue iso, zoom, vue à plat, reliefs en transparence, essai). Reste
la vérification des gestes à la souris (voir « Vérification »).

Exigences : `EX-EDIT-059`, `EX-EDIT-060`, `EX-EDIT-061` (nouvelles).
