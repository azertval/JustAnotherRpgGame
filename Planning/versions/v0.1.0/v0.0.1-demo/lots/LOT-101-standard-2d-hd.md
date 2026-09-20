+++
id = "LOT-101"
titre = "Le standard 2D HD"
version = "0.0.1"
filiere = "standard"
statut = "a-faire"
taille = "M"
resume = "Le style qui remplace le pixel art est écrit, chiffré et éprouvé sur une maquette rendue dans le moteur."
prerequis = ["LOT-100"]
reprend = ["LOT-92 (style de scène)", "LOT-66 (charte visuelle, pour la scène)"]
livrables = [
  "[`standards/style-2d-hd.md`](../../../../standards/style-2d-hd.md) passé de « proposé » à « normatif ».",
  "Une **maquette de validation** : un carré de 8 × 8 cases d'Arenarea (sol, deux façades, une colonnade, la fontaine, un lampadaire, une figurine) à l'échelle du standard, affichée à 1080p et à 2160p.",
  "`EX-VIS-008`, `EX-VIS-009` et `EX-REN-013` réécrites pour la 2D HD.",
  "La consigne de style du générateur (les trois blocs de prompt) réécrite, avec la planche de référence d'Arenarea pour ancre.",
  "Le nombre d'images par animation, tranché sur un essai de marche en six et en huit images.",
]
criteres = [
  "L'auteur approuve la maquette aux deux définitions : lisible à 1080p, nette à 2160p.",
  "Aucune valeur du standard ne reste « à fixer ».",
  "`lint_exigences.py` est vert après la réécriture des trois exigences.",
]
sources = [
  "Planche de référence `Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png`",
]
+++

## Pourquoi

Tout asset produit avant que le standard soit figé risque d'être refait. Ce lot est court, mais il
**bloque** toute la filière des assets : c'est lui qui dit la taille d'une case, la hauteur d'une
figurine, la lumière, l'alpha, la palette.

## Conception

Les valeurs proposées sont dans [le standard](../../../../standards/style-2d-hd.md) : losange de 256 × 159 px,
rapport 0,62 conservé, figurine de 170 px dans une cellule de 192 × 256, alpha continu, filtrage
bilinéaire avec mipmaps, zoom libre. La maquette sert à les **contredire** : si une case de 256 px
est trop lourde ou trop pauvre, c'est ici qu'on le voit, pas au vingtième quartier.

La maquette se monte à la main dans un outil d'image, à partir de sorties du générateur : elle ne
dépend ni du LOT-102 ni du LOT-103. Elle est ensuite la **référence de non-régression** du rendu HD.

## Risques

- La facture « peinte » varie d'une génération à l'autre bien plus que le pixel art quantifié :
  la consigne doit être éprouvée sur au moins trois familles de pièces avant d'être figée.
- Six ou huit images par animation : l'écart de coût est d'un quart sur **chaque** PNJ du jeu.
