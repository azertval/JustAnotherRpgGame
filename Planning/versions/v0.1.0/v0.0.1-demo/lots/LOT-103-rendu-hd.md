+++
id = "LOT-103"
titre = "Le rendu HD"
version = "0.0.1"
filiere = "moteur"
statut = "a-faire"
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
  bilinéaire : le seuil de comparaison est à revoir.
- Les mipmaps d'une planche d'animation débordent d'une image sur sa voisine : prévoir une marge
  entre images, à inscrire au standard.
