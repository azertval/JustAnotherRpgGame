# Le standard 2D HD

Le jeu quitte le pixel art le 20 septembre 2026. Ce document fixe ce qui le remplace : **une scène
isométrique peinte, en haute définition**, dont la référence est la planche
`Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png`.

Il est **normatif** : le [LOT-101](../versions/v0.1.0/v0.0.1-demo/lots/LOT-101-standard-2d-hd.md)
l'a confronté à une [maquette de huit cases sur huit](../versions/v0.1.0/v0.0.1-demo/maquettes/maquette-2d-hd-reperes.png),
montée aux deux définitions où le jeu se joue, et en a réécrit les exigences `EX-VIS-008`,
`EX-VIS-009` et `EX-REN-013`. Ce que la maquette a mesuré est au [§7](#7-ce-que-la-maquette-a-mesuré) ;
ce qu'elle a appris à la commande d'images est dans [la consigne du générateur](consigne-2d-hd.md).

> **Le standard est complet.** Sa dernière valeur ouverte — le nombre d'images par animation
> (§5) — a été fixée par le [LOT-112](../versions/v0.1.0/v0.0.1-demo/lots/LOT-112-heros-de-la-demo.md),
> sur la première figurine produite : **huit**.

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
| Figurine humanoïde | **170 px** de haut, dans une cellule de **192 × 256** ; cellule large **384 × 256** pour l'attaque, le sort et la mort (un corps allongé ne tient pas dans 192 px, `LOT-112`) | quatre fois la cellule actuelle (48 × 64) : les ancres et le sol (`y = 252`) se transposent |
| Grande créature | cellule de **384 × 384** | |
| Alpha | **continu** (8 bits), bords adoucis, **prémultiplié** au chargement | le détourage binaire est ce qui signe le pixel art |
| Couleur | sRGB, 8 bits par canal, **pas de palette imposée** par image | la cohérence vient de la palette du lieu (§3), pas d'une quantification |
| Filtrage | **bilinéaire + mipmaps** pour tout l'art de scène | c'est la révision du moteur que demande le [LOT-103](../versions/v0.1.0/v0.0.1-demo/lots/LOT-103-rendu-hd.md) |
| Zoom | **libre**, et fixé par la définition : une case occupe **100 px à 1080p, 200 px à 2160p** | les deux définitions cadrent la **même étendue de monde** (§7) : un écran plus fin ne montre pas plus de jeu, il montre le même jeu plus finement |
| Fichier | PNG 32 bits ; une pièce = un fichier ; **5 Mio au plus** (contrôle existant) ; planche d'animation ≤ 4096 px de côté | |
| Planche d'animation | **8 px de marge** entre deux images, et autour de la planche | sans elle, le niveau de mipmap d'une image déborde sur sa voisine et la marche bave (risque relevé par le [LOT-103](../versions/v0.1.0/v0.0.1-demo/lots/LOT-103-rendu-hd.md)) |

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

### Critères de qualité validés par l’utilisateur

Préférences durables confirmées le **23 septembre 2026**, après validation du LOT-105 V4 et de sa reprise du relief. Elles s’appliquent aux prochains lots d’assets de décor.

- **Fidélité au style d’origine** : conserver la facture peinte et les références approuvées ; éviter les variantes génériques. Reprendre précisément l’emblème du lieu depuis sa référence, sans en inventer un autre. Pour la Capitale, la bannière doit rester dans la famille visuelle du lampadaire v1.
- **Grille et tailles standard** : sol de 256 × 159 px, emprises en cases entières, échelle cohérente et ancres exactes. Aucun ajustement manuel par placement pour masquer une mauvaise calibration.
- **Continuité entre modules** : les textures de deux pièces voisines d’une même catégorie doivent se prolonger, y compris entre variantes. Garder des joints alignés, une phase commune et des bords compatibles ; éviter les coutures visibles et l’effet de damier entre tuiles.
- **Vraies pièces d’angle** : employer et vérifier les angles rentrants et sortants des murs, balustrades et haies. Pas de coins abîmés, trous, chevauchements parasites ou étirements de texture.
- **Matière propre, mais relief lisible** : pas de bruit, grain aléatoire ou microdétails parasites. Ne pas confondre cette propreté avec un aplat lisse : les sols doivent montrer la pierre taillée, ses facettes, ses biseaux et ses joints creux, avec des variations maîtrisées.
- **Volume réel dans l’image** : petits piliers des balustrades et buissons doivent présenter une épaisseur, des côtés visibles et des ombres propres cohérentes. Éviter les éléments de face simplement plaqués sur un plan incliné. Balustres modelés ; haies avec dessus arrondi et raccords continus jusque dans les angles. La livraison reste en PNG isométriques.
- **Validation en assemblage** : fournir une map réunissant tous les assets, avec répétitions de sols et raccords droits/angles des différentes familles. Utiliser le moteur pour vérifier le chargement et inspecter le rendu à plusieurs échelles ; une planche de pièces isolées ne suffit pas. Distinguer validation visuelle et essai de gameplay.
- **Retouches ciblées et réversibles** : préserver le reste d’un kit déjà validé, garder les versions précédentes ainsi que les sources et les prompts. Le recalage géométrique précis par script a été explicitement autorisé ; il complète la génération des images pour garantir les dimensions, la projection et les raccords.

Référence acceptée : `Tools/AssetsHD/Regions/central-empire/capital/Common/V4/`, **après reprise du relief**, et ses rendus moteur. Les premières versions de ce lot ne constituent pas la référence qualité.

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

**La famille 01 a une règle de plus**, que la maquette a imposée : une zone livre d'abord une
**dalle de fond répétable** — sans bordure, aux joints neutres, en **trois variantes au moins** —
et seulement ensuite ses panneaux décoratifs. Une dalle bordée répétée sur cinq cents cases dessine
un treillis qui n'existe dans aucune ville, et une dalle unique répétée fait battre un moiré sur
tout le champ. Les deux sols de la planche de référence sont des panneaux : ils ne sont pas un fond.

## 5. Les figurines

| Règle | Valeur |
|---|---|
| Orientations | **quatre** (les diagonales de l'isométrie), comme aujourd'hui |
| Animations | repos, marche, attaque, sort, touché, mort |
| Images par animation | **huit**, pour toutes les animations. Tranché par le [LOT-112](../versions/v0.1.0/v0.0.1-demo/lots/LOT-112-heros-de-la-demo.md) : la même marche commandée deux fois, à six et à huit images, et jugée sur la maquette à la vitesse du jeu. À six, la planche décollait de 8 px et dérivait de 12 px d'une image à l'autre ; à huit, le générateur a tenu le même personnage, les pieds au sol. Le quart de coût en plus sur chaque PNJ est accepté |
| Cadence | un cycle de marche couvre **une case** ; à 2 cases par seconde il dure une demi-seconde, soit **62 ms par image**. La durée est écrite dans le `.anim.json` de chaque bande, jamais dans le code |
| Fichiers | une bande par animation **et par orientation** : `walk-se.png`, `walk-sw.png`, `walk-ne.png`, `walk-nw.png`, chacune avec son `.anim.json` ; une animation orientée l'est dans les quatre sens |
| Ligne de sol | **252** dans la cellule de 256, déclarée par le manifeste `Characters/` (`"ground"`) ; le moteur la pose au centre du losange de la position |
| Portrait | 512 × 512, même facture, pour les dialogues et la fiche |
| Jeton | 128 × 128, détouré en rond, pour la piste d'initiative |

## 6. Ce que le standard interdit

- **Aucune image du corpus** dans le jeu : ni affichée, ni décalquée, ni retouchée. Les livres sont
  une référence de contenu, jamais une source d'images (règle du `LOT-94`, inchangée).
- **Aucun asset pixel art** ne subsiste : pas de cohabitation des deux styles, même provisoire.
- **Aucun agrandissement** : un asset trop petit se refait.

## 7. Ce que la maquette a mesuré

La maquette du `LOT-101` monte huit cases sur huit d'Arenarea — sol, deux façades, une colonnade, la
fontaine, un lampadaire, un banc — à l'échelle du standard, et les cadre aux deux définitions. Elle
se reconstruit par `python scripts/build_hd_mockup.py`, et vit dans
[`maquettes/`](../versions/v0.1.0/v0.0.1-demo/maquettes/) :

| Image | Ce qu'elle montre |
|---|---|
| `maquette-2d-hd-1080.png` | la scène entière en 1920 × 1080, à 100 px par case |
| `maquette-2d-hd-2160.png` | une fenêtre au pixel près à la densité 2160p, à 200 px par case |
| `maquette-2d-hd-reperes.png` | la même scène annotée : grille des cases, cellule de figurine, échelle |

Les deux premières ne portent **aucun texte** : ce sont les références de non-régression que le
rendu du `LOT-103` doit reproduire.

**Quatre mesures**, et ce qu'elles décident :

1. **Une case de 256 px tient.** À 100 px d'écran, la fontaine, la colonnade et le pan de mur se
   lisent sans effort ; le lion d'une bannière et la frise d'un bassin ne se lisent plus. Produire
   plus fin que **le double de la taille d'écran** est dépensé pour rien — c'est ce que dit la ligne
   « lisibilité » de [la consigne](consigne-2d-hd.md).
2. **La vue cadre 19,2 losanges de large et 17,4 de haut**, aux deux définitions. Une place de huit
   cases sur huit n'occupe donc qu'**un cinquième de l'écran** : une zone jouable qui remplit la vue
   fait au moins **vingt cases sur dix-sept**, et c'est ce chiffre, non le mètre carré, qui donne le
   volume d'assets d'une zone.
3. **Le sol est le poste le plus lourd.** Il couvre tout l'écran et il se répète : c'est de lui que
   viennent le treillis et le moiré, d'où la règle des variantes au [§4](#4-les-familles-de-pièces-dun-lieu).
4. **La planche de référence n'est pas une source de production.** Elle dessine un losange de
   136 px là où le standard en demande 256 : la maquette l'agrandit d'un facteur 1,88, et à 2160p
   cela se voit. Elle juge donc la composition, l'emprise et la lisibilité — **pas la finesse du
   trait**. La finesse se juge sur un master de production (1254 px réduit à 256, facteur 0,20),
   et c'est la règle « toujours réduit, jamais agrandi » qui la garantit.
