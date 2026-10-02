+++
id = "LOT-1002"
titre = "Le canevas de l'éditeur sur le rendu du jeu"
version = "0.0.2.5"
filiere = "editeur"
statut = "livre"
taille = "L"
resume = "L'éditeur dessine la scène par le rendu du jeu lui-même : ce qu'on y voit est ce qu'on jouera, et le moteur peut changer de matière sans que l'éditeur prenne un lot de retard."
prerequis = ["LOT-142"]
livrables = [
  "`EditorViewport` : la vue iso et l'essai (`P`) dessinés dans un `QRhiWidget` par `hmi::WorldSceneRenderer`, le rendu du jeu ; les aides d'édition (grille, poignées, étiquettes, formes d'entité) en surcouche.",
  "`LevelEditor --render` et les vignettes de préfabriqués (`hmi::renderStamp`) rendus par le même rendu, **hors écran**.",
  "La mesure : `CanvasBenchmarks` mesure l'image du canevas par le rendu du jeu, sur la carte d'Arenarea.",
  "Le guide d'usage de l'éditeur et `Source/Editor/Ui/README.md` à jour.",
]
criteres = [
  "Une carte rendue par `--render` et la même carte rendue par le jeu, hors écran, sont **identiques au pixel** : il n'y a plus de seuil de parité, parce qu'il n'y a plus qu'un rendu.",
  "`git grep -n \"ScenePainter\\|SceneImages\" Source` ne trouve rien.",
  "Chaque scénario `--apply` livré donne le même fichier attendu qu'avant ; les gestes du canevas (pinceau, tampon, entités, zones) sont refaits à la main par l'auteur sur une carte d'essai.",
  "Un travelling sur la carte d'Arenarea dans le canevas ne descend pas sous la cadence mesurée avant le lot.",
  "La CI rend `--render` sans carte graphique (WARP), comme les tests de rendu du jeu.",
]
+++

## Pourquoi

Le canevas peint aujourd'hui la scène composée par `QPainter` (`ScenePainter`), à côté du rendu GPU
du jeu, et un test mesure que les deux se ressemblent à 2,5 % près. Un peintre ne dessine ni
maillage ni profondeur : si le moteur passait en 3D d'abord, l'éditeur ne montrerait plus le décor
pendant plusieurs lots.

Ce lot fait l'inverse. Il branche le canevas sur le rendu du jeu **tant que celui-ci est encore en
2D** : rien ne change à l'écran, et c'est vérifiable au pixel. Quand le
[LOT-1003](LOT-1003-maillages-et-profondeur.md) apprend les maillages au moteur, l'éditeur les
montre le même jour.

## Périmètre

Dedans : la vue iso, l'essai, `--render`, les vignettes.

Dehors, nommément :

- la **vue à plat** (`F9`, `DraftRenderer`) et la **mini-carte** : elles ne dessinent que des types
  de tuile, et restent peintes ;
- toute la logique d'édition (`Source/Editor/Logic`) : elle travaille sur la grille ;
- la 3D : pas un maillage dans ce lot.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Le peintre de scène | `Source/Editor/Ui/ScenePainter.{h,cpp}` | le rendu du jeu le remplace ; le garder serait tenir deux rendus |
| Le cache d'images du canevas (niveaux réduits, budget de 256 Mio) | `Source/Editor/Ui/SceneImages.{h,cpp}` | les textures sont celles du rendu du jeu |
| Les tests du peintre et la parité GPU / `QPainter` | `Source/Test/Unit/Editor/test_scene_painter.cpp`, `test_scene_images.cpp` ; le seuil de 2,5 % | une identité ne se mesure pas avec un seuil ; `test_storey_render.cpp` se **rebranche** sur le rendu hors écran |
| La mesure de peinture | `Source/Benchmark/bench_canvas_paint.cpp` | remplacée par la mesure de l'image |
| Le paragraphe `ScenePainter` et `SceneImages` | `Source/Editor/Ui/README.md`, guide de l'éditeur, spécification de l'éditeur (parité) | documentation d'un code retiré |
| Les images de sortie de test à la racine | `cave-renderer.png`, `donjon-renderer.png`, `place-renderer.png` (non suivies, réécrites par les tests lancés depuis la racine) | les tests de rendu écrivent désormais sous `build/` |

## Conception

- `QRhiWidget` (Qt 6.7 et suivants ; le projet exige 6.11.2) dans la `QGraphicsView` ou à sa place :
  à trancher à l'ouverture, selon ce que le zoom, le défilement et le pointage demandent.
- Le pointage reste celui du jeu : `screenToWorld`, puis `worldToTile`.
- Les aides d'édition restent peintes par `QPainter`, **au-dessus** : ce ne sont pas la scène.
- `F8` (reliefs en transparence) et l'opacité d'une couche deviennent des paramètres du rendu.

## Tranché à l'ouverture (2 octobre 2026)

- **Le `QRhiWidget` va sous la `QGraphicsView`, pas à sa place.** La vue garde le zoom, le
  défilement, l'ancrage sous le pointeur et le pointage, que rien n'obligeait à réécrire ; son fond
  devient transparent, et elle dit son cadrage à la surface (`hmi::WorldFraming`) à chaque
  changement. Les deux plans se repeignent dans la même image.
- **La vue à plat garde un peintre à elle** (`hmi::DraftRenderer::paint`) : des aplats et des
  marqueurs engendrés, rien que le jeu dessine. `ScenePainter` et `SceneImages` partent bien en
  entier.
- **Le budget commun de 256 Mio ne survit pas au cache qu'il bornait.** Chaque onglet tient les
  textures de sa carte sur la carte graphique, sans partage entre onglets ; seul le rendu hors
  écran gardé (vignettes, `--render`) reste borné à 256 Mio. À revoir si plusieurs grandes cartes
  ouvertes ensemble pèsent trop.
- **Les tuiles de `--render`** (4 096 pixels) ne servent qu'au-delà de cette taille ; une image en
  tuiles diffère de la même image d'un seul tenant sur moins d'un pixel sur mille, de deux niveaux
  au plus (l'arrondi du centre de chaque tuile). L'identité au pixel avec le rendu du jeu est tenue
  sur une image d'un seul tenant.

## Mesures (poste de l'auteur, Release, 2 octobre 2026)

Travelling sur la carte d'Arenarea, 1920 × 1080 :

| | Avant (`QPainter`) | Après (rendu du jeu, relecture comprise) |
|---|---|---|
| Une case à 100 px | 7,3 ms | 5,8 ms |
| Une case à 25 px | 2,6 ms | 5,7 ms |

La mesure d'après relit l'image pour attendre la carte graphique ; son plancher — effacer et relire
une image vide — est de 6,2 ms. La scène elle-même coûte donc moins que le bruit de la mesure, et
le canevas, qui ne relit rien, ne paie pas ce plancher.

## Risques et questions ouvertes

- Le rendu hors écran en CI : `test_rhi_offscreen` l'exerce déjà ; `--render` doit tenir le plafond
  de 8 192 px par découpage en tuiles.
- La réactivité au geste : le canevas ne recompose aujourd'hui qu'au geste et peint découpé à la
  vue ; le rendu du jeu le fait déjà (`hmi::StaticWorldScene`), à vérifier pendant un coup de pinceau.
