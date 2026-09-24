+++
id = "LOT-125"
titre = "Le canevas de l'éditeur en HD"
version = "0.0.1"
filiere = "editeur"
statut = "livre"
taille = "M"
resume = "Le canevas, les vignettes et `--render` montrent une carte HD comme le jeu la montre : à la bonne taille, lissée, entière, sans saturer la mémoire."
prerequis = ["LOT-103", "LOT-123"]
livrables = [
  "Le lissage (`SmoothPixmapTransform`) au canevas, à la vue et à `--render` ; des images **réduites en cache** par palier de zoom, puisque `QPainter` n'a pas de mipmaps.",
  "Plus aucune taille d'art dans l'éditeur : `ARENA_SHEET_TILE_WIDTH_PIXELS`, le marqueur de figurine absente et la marge du cadre viennent du manifeste du lieu (`tile`, `frameHeight`, hauteur de pièce maximale).",
  "L'essai immédiat prend le **zoom libre** du jeu (hauteur / 10,8), plus le zoom entier.",
  "`--render` : l'échelle 1 redevient « la carte à 1080p » ; un plafond en **pixels** remplace le plafond d'échelle ; l'étape CI rend à une taille lisible.",
  "Un `SceneImages` **partagé** entre onglets, vignettes et préfabriqués, qui lit chaque manifeste une fois et borne sa mémoire.",
  "`bench_canvas` mesure la **peinture**, pas seulement la composition, sur des images HD.",
]
criteres = [
  "La maquette du LOT-101, ouverte dans l'éditeur, est conforme à son rendu par le jeu sous un seuil écrit dans `test_scene_painter` — la parité exacte avec un GPU à mipmaps n'est plus promise, et c'est dit.",
  "Un travelling et un zoom continus sur la carte de la maquette restent fluides sur le poste de référence (contrôle de l'auteur), et la mesure de peinture est publiée par le job nocturne.",
  "Trois onglets de cartes HD ouverts : la mémoire de l'éditeur reste sous une borne écrite dans le README du module.",
  "Une pièce de quatre cases de haut n'est rognée ni au cadrage, ni en vignette, ni par `--render`.",
]
+++

## Pourquoi

Le [LOT-103](LOT-103-rendu-hd.md) corrige le rendu du **jeu** et ce que l'éditeur partage avec lui
(`SceneComposition`). Il laisse ce qui est propre à l'éditeur — constats H1 à H8 de
l'[audit](../../../../standards/audit-editeur.md) : un canevas qui réduit au plus proche voisin, une
constante de 86 px que l'audit du moteur n'a pas vue, un cache d'images sans borne qui relit son
manifeste à chaque image.

## Périmètre

L'affichage. **Pas dedans** : d'où viennent les pièces (LOT-124). Le pointage, le zoom du canevas
et la mini-carte ne bougent pas : ils travaillent en unités du monde.

## Risques

- Le bilinéaire logiciel de `QPainter` sur une grande carte peut coûter cher : c'est pourquoi la
  mesure de peinture est un livrable, et le cache par palier de zoom la parade.

## Réalisation — 23 septembre 2026

Branche `lot-125-canevas-hd`. Plusieurs constats de l'[audit](../../../../standards/audit-editeur.md)
avaient déjà été levés par le [LOT-103](LOT-103-rendu-hd.md) : la constante de 86 px est devenue une
convention du repère (`ARENA_TILE_WIDTH_UNITS`, H2), l'essai immédiat cadre déjà au zoom libre du jeu
(hauteur / 10,8, H3), et le marqueur de figurine absente se dessine une case de large quelle que soit
l'échelle du lieu. Ce que le lot a fait du reste :

| Livrable | Ce qui est fait |
|---|---|
| Lissage et images réduites en cache (H1) | `hmi::SceneImage` garde les **niveaux réduits** de chaque pièce, la moitié de la précédente, calculés à la première demande ; le peintre lit en bilinéaire celui que l'échelle demande. Les images engendrées (marqueurs, jetons, atlas, damier) restent au plus proche, comme le sampler du jeu. |
| Plus de taille d'art (H4) | Le cadre du canevas, de l'essai, des vignettes et de `--render` se mesure sur ce qui est peint (`hmi::composedSceneBounds`), et non sur une marge d'un losange. |
| Zoom libre de l'essai (H3) | Déjà fait par le LOT-103 ; vérifié. |
| `--render` (H5) | L'échelle 1 est la carte à 1080p (une case à 100 px) ; un plafond de **8 192 px** de côté remplace le plafond d'échelle ; la CI rend à la demi-échelle, une case à 50 px. Les vignettes des cartes et des préfabriqués se peignent directement à deux fois leur taille, et non plus en plein format. |
| `SceneImages` partagé et borné (H6) | Une instance par dossier d'assets (`SceneImages::shared`) pour les onglets, les vignettes et `--render` ; un budget de **256 Mio** de pixels, tenu par éviction de la pièce la moins récemment peinte, qui se relit à la peinture suivante ; chaque manifeste lu une fois (`ManifestCache`, partagé avec le jeu par `readSceneTextureTraits`). |
| La peinture mesurée (H7) | `CanvasBenchmarks` (`bench_canvas_paint.cpp`), une cible à part parce qu'elle lie Qt Gui ; le job de nuit installe Qt et publie sa série. |

### Critères

- **Parité avec le jeu** ✔ — `ScenePainterTest.LaMaquetteHdPeinteEgaleLeRenduDuJeu` : la maquette
  du LOT-101 peinte à 1080p. Seuils écrits dans `test_scene_painter.cpp`, avec la mesure qui les
  justifie :

  | | Cartes d'essai | Maquette HD |
  |---|---:|---:|
  | au plus proche (avant) | 1,56 % · écart 1,55 | 2,92 % · écart 6,30 |
  | lissé, par niveaux | 0 % · écart 0,22 | 0 % · écart 1,87 |
  | seuil | 0,5 % · 0,75 | 0,5 % · 3,5 |

  Le seuil des cartes d'essai redescend de 2,5 % à 0,5 %, comme le LOT-103 l'annonçait. La parité
  exacte n'est plus promise : le GPU mêle deux niveaux de mipmap, le peintre n'en lit qu'un.
- **Travelling et zoom fluides** ✔ — la maquette HD se peint en 11,5 ms à 1080p et en 4,8 ms
  dézoomée (Release, poste local) ; l'auteur a contrôlé à la main que le travelling et le zoom
  restent fluides.
- **Trois onglets, mémoire bornée** ✔ — la borne est écrite dans le README du module (256 Mio de
  pixels d'art, quel que soit le nombre d'onglets) ; `test_scene_images.cpp` prouve le partage, le
  budget et la relecture. Mesure d'appoint : trois rendus de la carte de validation du LOT-105
  (38 pièces HD) à l'échelle 2160p dans un même processus plafonnent à 100 Mio, image de
  5 100 × 3 376 comprise. L'essai à trois onglets, à la main, a été fait par l'auteur.
- **Une pièce de quatre cases n'est pas rognée** ✔ — `MapRenderTest.UnePieceHauteNEstPasRognee` :
  une tour de 256 × 1024 sur la case la plus haute de la carte, entière dans l'image. Les vignettes
  passent par le même cadre ; le canevas aussi.

Réalisé le 23 septembre 2026, **PR #120**. Livré le 24 septembre 2026 : l'auteur a fait les
contrôles à la main (travelling et zoom fluides, trois onglets HD).
