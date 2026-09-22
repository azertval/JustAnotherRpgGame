+++
id = "LOT-103"
titre = "Le rendu HD"
version = "0.0.1"
filiere = "moteur"
statut = "en-cours"
taille = "M"
resume = "Le moteur affiche une pièce HD à la bonne taille, entière et sans scintillement : l'échelle de l'art devient une donnée du lieu."
prerequis = ["LOT-101"]
livrables = [
  "L'échelle de l'art lue dans le manifeste du lieu (`\"tile\": [256, 159]`) et propagée aux deux composeurs, au rendu des îlots et à la galerie — fin de `SCENE_TILE_WIDTH_PIXELS`.",
  "`frameHeight` ajouté à `SceneTexture` et employé par `WorldSceneComposer` et `ArenaSceneComposer` — fin de la figurine rognée à 64 px.",
  "Un échantillonneur **bilinéaire avec mipmaps** pour l'art de scène dans `SpriteBatch`, alpha prémultiplié.",
  "Le zoom libre : fin de l'agrandissement entier dans `WorldSceneRenderer` et `Camera2D::fitZoom` ; `FIGURE_SCALE` retiré.",
  "La hauteur de pièce maximale lue dans le manifeste (`CityBlockRender`), `smooth` et `mipmap` sur `BlockMapForm`.",
  "`EX-ARCH-022` enfin tenue par le code ; tests de rendu hors écran sur la maquette du LOT-101.",
]
criteres = [
  "La maquette du LOT-101, rendue par le moteur à 1080p et à 2160p, est conforme à la maquette montée à la main (écart moyen par pixel sous un seuil écrit dans le test).",
  "Une figurine de 192 × 256 et une créature de 384 × 384 s'affichent entières.",
  "Un travelling lent sur la maquette ne scintille pas (contrôle visuel de l'auteur).",
  "Aucune constante du rendu ne porte plus une taille d'art en pixels.",
]
+++

## Pourquoi

L'hypothèse de départ était qu'aucune révision du moteur n'était nécessaire.
L'[audit](../../../../standards/audit-passage-hd.md) dit le contraire, fichier et ligne à l'appui : quatre
hypothèses du pixel art sont compilées dans le rendu. Aucune n'est profonde — le sol est déjà
indépendant de la résolution, le manifeste porte déjà l'échelle sans que le C++ la lise — mais
sans ce lot une pièce HD s'affiche 3,8 fois trop grande, rognée et scintillante.

## Périmètre

Le **rendu** seulement. Pas de changement du format de carte, de la grille tactique, de la
projection (le rapport 0,62 est conservé) ni de l'éditeur, hormis ce qu'il partage avec le rendu du
jeu (`SceneComposition`, vignettes de la palette). Ce qui est propre à l'éditeur — lissage du canevas,
`--render`, cache d'images, mesure de peinture — est au [LOT-125](LOT-125-canevas-hd.md).

## Risques

- La parité GPU / QPainter de l'éditeur (`test_scene_painter.cpp`) doit tenir avec le filtrage
  bilinéaire : le seuil de comparaison est à revoir. **Revu** : voir D-103-7.
- Les mipmaps d'une planche d'animation débordent d'une image sur sa voisine : prévoir une marge
  entre images, à inscrire au standard. **Inscrit** au standard par le LOT-101 (8 px, §2) ; le
  moteur n'a rien à y faire.

## Ce que la réalisation a tranché

**D-103-1 — La cascade de résolution reste au LOT-124.** La [D-102-2](LOT-102-table-rase-assets-et-cartes.md)
avait laissé ici la résolution d'une clé à travers les niveaux (sous-zone, zone, ville, région,
monde). Elle n'est pas dans les livrables de cette fiche, et le [LOT-124](LOT-124-editeur-et-arborescence-par-niveaux.md)
la porte tout entière : son catalogue résolu dans `Core`, et le critère « `git grep '"Scene/"'
Source/Editor Source/HMI/Graphics` ne trouve plus de chemin composé à la main ». La faire deux fois
— une dans le rendu, une dans `Core` — aurait fait diverger le jeu et l'éditeur. Ce lot ne touche
donc pas aux chemins ; il fait en sorte que **chaque pièce** porte sa propre échelle, ce qui est la
condition pour qu'une carte puise demain dans plusieurs niveaux livrés à des échelles différentes.

**D-103-2 — L'échelle se lit par image, pas par lieu.** Le manifeste du dossier d'une image déclare
`tile` ; `hmi::readSceneTextureTraits` le reporte sur la texture (`SceneTexture::artTile`), avec la
cellule du `.anim.json` et l'ancre. Une figurine lit le manifeste de son atelier, un dossier plus
haut (`Characters/<pnj>/idle.png`) : les six manifestes `Characters/` livrés déclarent donc
`"tile": [256, 159]`, comme les données d'essai `[68, 42]`. Sans échelle déclarée, une pièce se
suppose d'une case de large, une bande d'une case de **haut** — sa cellule est commune à toutes ses
bandes, là où sa largeur double pour une attaque.

**D-103-3 — Une grande créature n'est plus agrandie.** L'arène grossissait d'un facteur *n* la
figurine d'une créature de *n* cases : l'art de 48 × 64 était le même pour toutes. Une créature HD
est livrée à sa taille (384 × 384, `EX-VIS-008`) : elle se centre sur son emprise, sans facteur.

**D-103-4 — La nature d'une texture se décide à sa création.** Tout fichier chargé est de l'art
peint (`TextureFiltering::Smooth` : mipmaps engendrées par le GPU, bilinéaire) ; une image engendrée
en code reste `Sharp`. `SpriteBatch` reconnaît la première à son drapeau `MipMapped`. Toutes sont
prémultipliées, et le pipeline mélange en `One`/`OneMinusSrcAlpha`.

**D-103-5 — Les dalles débordent d'1/256 de case.** Le premier rendu de la maquette s'écartait de
sa référence par un treillis sombre sur tout le sol : le bord adouci de deux dalles jointives laisse
passer le fond. La maquette débordait d'un pixel d'art ; le moteur en fait autant
(`FLOOR_SEAM_OVERLAP`, en fraction de case). C'est un défaut que le jeu aurait eu sur chaque zone.

**D-103-6 — La conformité se mesure sur des blocs de 4 × 4.** Pixel à pixel, l'écart est dominé par
le filtrage (Lanczos contre mipmaps sur le fin pavage) : 6,8 à 1080p, et les coutures ne donnaient
que 8,2. Moyenné par blocs, l'écart vaut 2,3 à 1080p et 3,1 à 2160p, et le seuil de 4 attrape chaque
défaut **provoqué** : échelle de 68 px (16,9 / 34,6), ancres ignorées (4,3 / 11,6), dalles jointives
(4,2 / 5,1), échelle fausse de 10 % (3,1 / 6,9). Le tableau est dans `test_hd_mockup_render.cpp`.

**D-103-7 — La parité avec le canevas de l'éditeur passe à 2,5 %.** Le GPU lisse, le peintre QPainter
de l'éditeur pas encore (son lissage est au [LOT-125](LOT-125-canevas-hd.md)) : chaque arête de pièce
porte un liseré d'un pixel qui diffère — 1,29 % au pire, et rien d'autre, la géométrie étant
identique. Le seuil passe de 0,5 à 2,5 % ; le LOT-125 le fera redescendre. Le test se cadre à
64 px par case, pour que l'art d'essai y soit réduit comme l'art HD l'est en jeu.

**D-103-8 — Aucune taille d'art dans le rendu, et ce qui reste en pixels.** `ARENA_SHEET_TILE_WIDTH_PIXELS`
(86) disparaît : `ARENA_TILE_WIDTH_UNITS` (5,375) n'est plus qu'une convention du repère, que ni
l'échelle de l'art ni la taille d'une case à l'écran ne lisent. Restent des **définitions** d'images
engendrées — un marqueur de figurine de 48 × 64, un jeton de 44 px, une image d'îlot à 100 px par
case, une case de galerie de 100 px à l'écran — qui ne disent rien de la taille d'un art ; le jeton
se pose à 0,65 case. Les commentaires « pixel art » de l'éditeur (`ScenePainter`, `PalettePanel`,
`ThumbnailGeometry`) décrivent encore son canevas tel qu'il est : ils partent avec le LOT-125.

**D-103-9 — Le travelling s'écrit à la demande.** Le critère du scintillement est un contrôle
visuel, et aucune carte HD ne se joue encore (les chemins sont au LOT-124). `JADG_TRAVELLING_DIR`
fait écrire au test 96 images de la maquette, la caméra glissant d'un quart de pixel par image, et
`scripts/build_hd_mockup.py --travelling <dossier>` les assemble en animation de 960 × 540 à
l'échelle 1.

## Ce qui reste

- **Le contrôle visuel du travelling** par l'auteur (critère 3) : le lot reste `en-cours` jusque-là
  ([définition de livré](../../../../standards/definition-de-livre.md)). Pour le refaire :
  `$env:JADG_TRAVELLING_DIR = "build\lot-103-travelling"`, lancer
  `UnitTests.exe --gtest_filter=HdMockupRender.WritesASlowTravellingForTheAuthor`, puis
  `python scripts/build_hd_mockup.py --travelling build\lot-103-travelling`.
- Les mesures de performance nocturnes : les mipmaps ajoutent un tiers de mémoire par texture
  chargée ; aucune série ne porte encore d'art HD (`bench_canvas` compose les données d'essai).
- `hmi::ArenaViewportItem` et le rendu de l'arène cadrent au rapport 0,62 ; une arène dont la carte
  déclarerait un autre rapport verrait son calque QML décalé. C'était déjà le cas (le calque écrivait
  42/68) ; ce lot l'aligne sur le défaut sans lire le rapport de la carte.
