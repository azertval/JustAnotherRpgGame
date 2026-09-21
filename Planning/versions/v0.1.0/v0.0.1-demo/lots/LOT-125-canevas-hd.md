+++
id = "LOT-125"
titre = "Le canevas de l'éditeur en HD"
version = "0.0.1"
filiere = "editeur"
statut = "a-faire"
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
