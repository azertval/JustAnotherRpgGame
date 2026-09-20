# Le standard 2D HD

Le jeu quitte le pixel art le 20 septembre 2026. Ce document fixe ce qui le remplace : **une scène
isométrique peinte, en haute définition**, dont la référence est la planche
`Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png`. Il est **proposé** ici et devient
**normatif** à la livraison du
[LOT-101](../versions/v0.1.0/v0.0.1-demo/lots/LOT-101-standard-2d-hd.md), qui le confronte à une
maquette rendue dans le moteur et réécrit l'exigence `EX-VIS-008` en conséquence.

## 1. La géométrie — ce qui ne change pas

| Règle | Valeur | Pourquoi |
|---|---|---|
| Projection | isométrique, losange de rapport **0,62** (hauteur / largeur) | c'est celui du moteur (`IsoProjection.h`) et celui de la planche de référence : les cartes, la grille tactique et l'éditeur n'ont pas à bouger |
| Une case | 1,5 m de côté, comme la grille du *Manuel* | le combat est déjà réglé dessus |
| Lumière | du **haut à gauche**, douce ; ombre propre peinte, **pas d'ombre portée** dans la pièce | deux pièces voisines ne doivent pas se contredire ; l'ombre au sol est l'affaire du moteur |
| Point de vue | orthographique : aucune fuyante, aucune perspective | une pièce doit pouvoir se poser n'importe où sur la carte |

## 2. La définition — ce qui change

| Règle | Valeur | Pourquoi |
|---|---|---|
| Losange de sol | **256 × 159 px** | à 1080p une case occupe ~100 px d'écran, à 2160p ~200 : l'art est **toujours réduit, jamais agrandi** |
| Figurine humanoïde | **170 px** de haut, dans une cellule de **192 × 256** ; cellule large **384 × 256** pour l'attaque et le sort | quatre fois la cellule actuelle (48 × 64) : les ancres et le sol (`y = 252`) se transposent |
| Grande créature | cellule de **384 × 384** | |
| Alpha | **continu** (8 bits), bords adoucis, **prémultiplié** au chargement | le détourage binaire est ce qui signe le pixel art |
| Couleur | sRGB, 8 bits par canal, **pas de palette imposée** par image | la cohérence vient de la palette du lieu (§3), pas d'une quantification |
| Filtrage | **bilinéaire + mipmaps** pour tout l'art de scène | c'est la révision du moteur que demande le [LOT-103](../versions/v0.1.0/v0.0.1-demo/lots/LOT-103-rendu-hd.md) |
| Zoom | **libre** : la caméra cadre la scène à la fenêtre | l'agrandissement entier (`EX-REN-013`) n'a plus d'objet |
| Fichier | PNG 32 bits ; une pièce = un fichier ; **5 Mio au plus** (contrôle existant) ; planche d'animation ≤ 4096 px de côté | |

L'échelle de l'art devient une **donnée du lieu** : le manifeste d'une scène déclare
`"tile": [256, 159]`, et le moteur en déduit l'échelle de chaque pièce. Un lieu pourrait demain
être livré plus fin ou plus grossier sans toucher au code.

## 3. La facture

Ce que la planche de référence montre, et que toute commande d'asset doit reprendre :

- **Peint, pas pixellisé** : aplats modelés, dégradés doux, arêtes nettes mais sans crénelage.
- **Un contour sombre et fin** (bronze foncé, jamais noir pur) qui détache la pièce du sol.
- **Matières lisibles** à la taille du jeu : le veinage du marbre, l'appareil de la pierre, la
  patine du bronze se lisent à 100 px de case — le détail plus fin est perdu, donc inutile.
- **Saturation contenue**, une couleur d'accent par lieu.
- **Une pièce tient seule** : fond transparent, pas de sol sous un mur, pas de décor autour d'un
  meuble.

### La palette de l'Empire central

Relevée sur la planche. Chaque région aura la sienne, écrite dans son référentiel.

| Teinte | Usage |
|---|---|
| **Ivoire** `#efe6d2` | marbre, enduits, lumière |
| **Sable** `#d9c7a3` | pierre calcaire, sols |
| **Gris chaud** `#9c948a` | ombres de pierre, pavés |
| **Bronze foncé** `#5c4a2a` | contours, ferronnerie, bois sombre |
| **Bourgogne** `#8e2335` | l'accent impérial : bannières, tuiles, auvents |
| **Or vieilli** `#c9a45c` | emblèmes, chapiteaux, dorures |
| **Vert feuillage** `#3f6b34` | cyprès, haies, jardins |
| **Eau sourde** `#2f7f86` | fontaines, bassins |

## 4. Les familles de pièces d'un lieu

Les dix familles de la planche sont le **gabarit d'inventaire** d'une zone urbaine : un lot
d'assets de zone les passe en revue une à une et dit, pour chacune, ce qu'il prend au **commun**
et ce qu'il produit en **propre** (voir l'[arborescence](arborescence-assets.md)).

| # | Famille | Emprise type | Exemples |
|---|---|---|---|
| 01 | Sols | 1 × 1 | pavage, bordure, motif, sable |
| 02 | Façades | 2 × 1, 3 × 1 | pan de mur, fenêtre, lierre |
| 03 | Colonnes | 1 × 1, 3 × 1 | colonne, colonnade |
| 04 | Accès | 2 × 1 | porte, portail, grille |
| 05 | Balustrades | 2 × 1, angle | garde-corps, pilier |
| 06 | Pièces maîtresses | 3 × 3 et plus | fontaine, statue, estrade |
| 07 | Végétal | 1 × 1 à 3 × 1 | haie, massif, arbre, topiaire |
| 08 | Mobilier | 1 × 1 | banc, lampadaire, vasque, étal |
| 09 | Bâtiments | 3 × 2 et plus | boutique, maison, tour |
| 10 | Seuils | 3 × 3 et plus | parvis, escalier, arche |

## 5. Les figurines

| Règle | Valeur |
|---|---|
| Orientations | **quatre** (les diagonales de l'isométrie), comme aujourd'hui |
| Animations | repos, marche, attaque, sort, touché, mort — le nombre d'images par animation se **fixe au LOT-101**, sur un essai : en peint, huit images coûtent cher et six suffisent peut-être |
| Portrait | 512 × 512, même facture, pour les dialogues et la fiche |
| Jeton | 128 × 128, détouré en rond, pour la piste d'initiative |

## 6. Ce que le standard interdit

- **Aucune image du corpus** dans le jeu : ni affichée, ni décalquée, ni retouchée. Les livres sont
  une référence de contenu, jamais une source d'images (règle du `LOT-94`, inchangée).
- **Aucun asset pixel art** ne subsiste : pas de cohabitation des deux styles, même provisoire.
- **Aucun agrandissement** : un asset trop petit se refait.
