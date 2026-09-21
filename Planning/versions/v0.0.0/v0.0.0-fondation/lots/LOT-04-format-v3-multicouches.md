+++
id = "LOT-04"
titre = "Format de carte `version: 3`, multi-couches"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "Une carte porte N couches typées et une liste d'entités, avec propriétés libres et tolérance aux champs inconnus : toute production de carte peut démarrer sans dette de format."
prerequis = ["LOT-03"]
livrables = [
  "`struct TileLayer` (nom, `LayerKind`, `TileMap`, `PropertyMap`) ; `core::Level` porte un `std::vector<TileLayer>`.",
  "`struct MapEntity` (type, `GridPosition`, `PropertyMap`) ; `core::Level` porte un `std::vector<MapEntity>`.",
  "Migration ascendante dans `LevelLoader.cpp` : la grille racine promue en couche de tête (`Collision` ou `Legacy`), version inconnue refusée.",
  "`LevelScene::buildLevelScene` boucle sur les couches ; `TileAutotile` s'applique par couche.",
  "`LevelDraft` transporte couches, entités et propriétés ; `LevelWriter::buildJson` prend l'agrégat `LevelData`.",
  "Exigences `EX-LVL-*` (multi-couches, migration ascendante, tolérance aux champs inconnus) et extension `EX-EDIT-*`.",
]
criteres = [
  "Aller-retour chargement → écriture → rechargement **à l'identique** sur une carte à trois couches et une liste d'entités.",
  "Une carte `version: 2` se charge sans erreur, promue en couche unique.",
  "Une `version: 4` est refusée avec un message explicite (`EX-NFR-040`).",
  "Un champ inconnu dans une couche ou une entité est **ignoré sans erreur**, et **préservé** à la réécriture.",
]
+++

## Pourquoi

Passer le format de carte d'une **grille plate unique** à **N couches typées**, plus une liste
d'entités — la seule évolution de format structurante du programme.

### Pourquoi maintenant, et pas plus tard

Un RPG en vue de dessus a besoin d'au moins trois couches là où un platformer se contentait d'une :

- **sol** (herbe, dalle, eau) — ce qu'on voit sous les pieds ;
- **décor** (arbre, tonneau, tapis) — dessiné au-dessus du sol, et **devant ou derrière** le
  personnage selon sa position (cf. `LOT-07`) ;
- **collision** — masque indépendant du visuel : un tapis se traverse, un tonneau non, et les deux
  peuvent partager la même image de sol.

Plus une couche **`objects`** qui n'est pas une grille mais une **liste** : PNJ, coffres,
panneaux, portails, déclencheurs de rencontre.

Ce lot doit précéder toute production de carte. Une carte dessinée sur le format plat serait à
refaire — et c'est le genre de dette qu'on ne repaie jamais.

## Périmètre

- `struct TileLayer { std::string name; LayerKind kind; TileMap tiles; PropertyMap properties; }` ;
  `Level` porte un `std::vector<TileLayer>`. **Pas de champ `renderLayer`** : le rang de la couche
  dans le vecteur *est* son ordre de superposition, un second ordre l'aurait contredit.
- `struct MapEntity { std::string type; GridPosition position; PropertyMap properties; }` ;
  `Level` porte un `std::vector<MapEntity>`.
- **Migration ascendante** : le chargeur promeut la grille racine en couche de tête —
  `kind = Collision` quand la carte déclare des couches visibles, `kind = Legacy` quand elle n'en
  déclare aucune. Le mécanisme existe déjà — `LevelLoader.cpp` traite l'absence de champ `version`
  comme la version initiale, et refuse proprement une version qu'il ne connaît pas.
- `TileAutotile` (raccords 16 voisinages) s'applique **par couche**, sans modification.
- `LevelScene::buildLevelScene` boucle sur les couches.
- La grille de **collision** devient la source de vérité du balayage AABB — et, au `LOT-19`, de la
  grille de combat tactique.

## Risques et questions ouvertes

### Le point à ne pas rater

**Prévoir dès maintenant un dictionnaire de propriétés libres** par couche et par entité, et la
**tolérance aux champs inconnus**. Sans cela, les besoins du combat (terrain difficile, couverture,
hauteur — découverts en phase D) imposeraient un `version: 4` en plein milieu du programme, avec
migration de tout le contenu déjà produit. C'est le risque numéro un de ce lot.

## Décisions de réalisation

Ce que la réalisation a tranché.

**La collision n'est pas une couche du tableau `layers` : c'est le tableau racine `tiles`.** Le
périmètre initial la voyait comme l'une des trois couches déclarées, à égalité avec le sol et le
décor. Elle ne peut pas l'être : la grille racine porte déjà l'entrée, la sortie et les cases de
mécanismes, que la validation exige et qu'aucune couche ne réplique. Une carte à deux grilles se
serait désynchronisée dès le premier aller-retour d'éditeur — l'écriture repart de la grille du
niveau, qui aurait alors perdu son entrée. Une couche `collision` déclarée est donc **refusée**,
avec un message qui renvoie à la racine ; le chargeur promeut la grille racine en couche de tête
pour que les consommateurs bouclent quand même sans cas particulier.

**Le brouillon d'édition transporte ce qu'il ne sait pas éditer.** L'éditeur multi-couches est le
`LOT-11`, mais `LevelDraft` porte dès maintenant couches, entités et propriétés : sans cela, ouvrir
puis enregistrer une carte `version: 3` l'aurait vidée en silence. Le redimensionnement emporte
toutes les couches et abandonne les entités sorties de la grille, faute de quoi le niveau devenait
irrécupérable à l'enregistrement.

**`LevelWriter::buildJson` prend l'agrégat `LevelData`.** Les couches et les entités auraient porté
sa liste positionnelle à onze paramètres, dont trois `std::vector` voisins interchangeables sans
que le compilateur bronche — la dette même que le `LOT-03` venait de solder côté `Level`.

## Exigences couvertes

`EX-LVL-*` (multi-couches, migration ascendante, tolérance aux champs inconnus), extension
`EX-EDIT-*`.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à
914/914, lint d'exigences, cahier de test, Doxygen et `clang-format` verts. Le prérequis `LOT-03`
valait parce que `LevelData` accueille les nouveaux champs sans repasser à 17 paramètres
positionnels.
