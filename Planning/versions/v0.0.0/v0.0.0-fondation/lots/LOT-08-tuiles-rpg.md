+++
id = "LOT-08"
titre = "Vocabulaire de tuiles RPG"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "La palette reçoit les neuf types de terrain d'un RPG en vue de dessus, chacun avec libellé, catégorie, rendu de repli et test : une carte se dessine sans aucun fichier d'image."
prerequis = ["LOT-04"]
livrables = [
  "Neuf types ajoutés à `core::TileType` : `Grass`, `Dirt`, `Sand`, `Water`, `DeepWater`, `Wall`, `Cliff`, `Bridge`, `Stairs` (`TileType.h`, `TileTypeName.cpp`).",
  "Catégories de palette à trois entrées dans `Editor/TileTaxonomy.cpp` et `TaxonomyLabels.cpp`, libellés `fr.lang` et `en.lang`.",
  "Rendu de repli déterministe de chaque type dans `ProceduralAtlas`.",
  "`DeepWater` porté par `core::isSolid` ; raccords de `Wall` et `Cliff` obtenus par `hmi::solidNeighborMask`.",
  "Tests d'aller-retour chargeur/écrivain pour chaque type.",
]
criteres = [
  "Chaque nouveau type a : un libellé fr/en, une classe de palette, un rendu procédural de repli, et un test d'aller-retour.",
  "Le jeu se lance et affiche une carte **sans aucun fichier d'image** présent.",
  "`TILE_TYPE_COUNT` reste dérivé du dernier énumérateur — aucune borne recopiée à la main.",
]
+++

## Pourquoi

Donner à la palette les types de terrain d'un RPG en vue de dessus, là où le `LOT-01` a laissé le
strict minimum hérité (11 types).

## Périmètre

Types à ajouter dans `core::TileType`, à peu près : `Grass`, `Dirt`, `Sand`, `Water`, `DeepWater`,
`Wall`, `Cliff`, `Bridge`, `Stairs`. La liste exacte se décide au contact du level design, pas
d'avance.

Pour **chaque** type ajouté, la chaîne complète — c'est la leçon la plus chère de l'héritage, où
ajouter un type touchait « exactement la même chaîne de huit fichiers » :

1. `TileType.h` (avant le dernier énumérateur, `TILE_TYPE_COUNT` suit tout seul) ;
2. `TileTypeName.cpp` (le `switch` est exhaustif et sans `default` : le compilateur désigne
   lui-même ce qu'il reste à faire) ;
3. `Editor/TileTaxonomy.cpp` (catégorie de palette) et `TaxonomyLabels.cpp` ;
4. libellés `fr.lang` **et** `en.lang` ;
5. `ProceduralAtlas` : **rendu de repli déterministe**, dans le même lot, jamais « plus tard » —
   c'est ce qui garde le jeu lançable sans aucun fichier d'image ;
6. `TileAutotile` si le type a des raccords ;
7. `TileSilhouette` si sa matière n'occupe pas toute la case (falaises, bords d'eau, ponts) — le
   `LOT-01` a **conservé ce mécanisme vidé** précisément pour ce lot ;
8. test d'aller-retour chargeur/écrivain.

## Conception

### Note de conception

Le `LOT-01` a supprimé les 25 types de plateforme mais gardé les 11 génériques (`Empty`, `Solid`,
`Danger`, `Entry`, `Exit`, `Switch`, `Door`, `PressurePlate`, `Block`, `Key`, `LockedDoor`) : le
vocabulaire de puzzle sert tel quel au RPG. Ce lot **ajoute**, il ne remplace pas.

## Décisions de réalisation

Ce que la réalisation a tranché.

**La liste retenue est exactement celle que le périmètre proposait** — `Grass`, `Dirt`, `Sand`,
`Water`, `DeepWater`, `Wall`, `Cliff`, `Bridge`, `Stairs` — faute de level design à consulter pour
en dévier. Elle couvre les trois questions qu'un auteur de carte se pose devant sa palette : ce qui
se marche, ce qui arrête, ce qui fait franchir. C'est aussi le découpage de catégories retenu, à
trois entrées, plutôt qu'une seule rubrique « Terrain » où l'on chercherait le pont parmi les sols.

**L'eau profonde arrête.** Ce n'est pas de la matière, mais rien ne permet encore de la franchir.
Le mettre dans `core::isSolid` plutôt que dans un test à part garantit que le jour où une règle de
nage existera, il n'y aura **qu'un** endroit à changer — l'alternative, des « sauf si c'est de
l'eau » disséminés, est exactement la dette que ce lot cherche à ne pas créer.

**Aucune silhouette n'est déclarée** (point 7 du périmètre). Le mécanisme, conservé vidé par le
`LOT-01`, découpe la matière qui n'occupe pas toute la case : il décrivait des pentes et des
arrondis de plateforme. Un terrain vu de dessus est carré par nature — falaise, rive et pont
occupent leur case entière, et c'est le raccord entre cases voisines (autotuilage), pas la découpe
d'une case, qui leur donnera leur forme. Leur inventer des silhouettes serait travailler contre le
genre ; le mécanisme reste disponible pour le jour où une tuile en aura vraiment besoin.

**L'autotuilage vient sans code.** `hmi::solidNeighborMask` interroge `core::isSolid` : `Wall` et
`Cliff` obtiennent donc leurs raccords à seize voisinages du seul fait d'arrêter le déplacement,
sans qu'aucune liste de types n'ait à les nommer.

**Les neuf types ont pris des cases restées libres** dans l'atlas procédural après le retrait des
types de plateforme au `LOT-01` — aucune couleur déjà posée dans un niveau livré n'a bougé, ce que
l'invariant de `tileColor` exige.

## Exigences couvertes

`EX-EXP-*`, `EX-EDIT-*`, `EX-REN-*`.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest`
à 927/927, lint d'exigences, cahier de test, Doxygen et `clang-format` verts. Le `LOT-04` était
requis parce que les couches donnent leur sens aux types.
