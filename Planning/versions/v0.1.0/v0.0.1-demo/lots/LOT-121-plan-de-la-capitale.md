+++
id = "LOT-121"
titre = "L'onglet « Carte » : le plan de la Capitale"
version = "0.0.1"
filiere = "cartes"
statut = "livre"
taille = "S"
resume = "Le plan de la Capitale montre ses douze quartiers ; trois s'ouvrent, les autres s'annoncent."
prerequis = ["LOT-146"]
reprend = ["LOT-94", "LOT-96"]
livrables = [
  "Le plan de la Capitale déjà peint par l'auteur, gardé tel quel, avec ses douze quartiers.",
  "Les deux quartiers de la démo cliquables, et l'Arena of Fate **dans** Arenarea (sous-zone, décision D-16) ; les dix autres quartiers grisés, avec leur nom.",
  "`World/cities/capital.json` rebranché sur les cartes des deux quartiers ; le plan de ville et le HUD de la carte sont **gardés** (décision D-15), seul le branchement change.",
  "L'image de chaque carte de principe pour l'onglet « Carte » — le rendu `--render` de la carte (LOT-128), à défaut d'une image peinte par l'auteur — et son entrée dans `world-maps.json` : `capital/arenarea/Map/`, `capital/martpart/Map/`, `capital/arenarea/arena-of-fate/Map/`. Les images peintes des zones définitives viennent avec leurs cartes, à la `0.0.3` (LOT-147, LOT-111, LOT-107 ; décision D-25).",
]
criteres = [
  "Depuis l'onglet, on lit où l'on est et où l'on peut aller.",
  "`check_map_assets.py` et `check_rpg_data.py` sont verts.",
  "L'onglet « Carte » montre chaque zone de la démo et la position du joueur ; l'Arena of Fate, à l'intérieur d'Arenarea.",
]
+++

## Périmètre

Pas de voyage rapide : le plan **montre**, il ne transporte pas.

## Décisions de réalisation

Livré le 25 septembre 2026, sur la branche des cartes habillées de la démo (PR #139), dont les
images sont le rendu.

1. **La carte d'une zone est son rendu, rangé avec elle.** `check_map_assets.py` réserve
   `Assets/Maps/` aux cartes **peintes par l'auteur** (`provenance: author`) : un rendu de
   l'éditeur n'y a pas sa place. Il se range dans le `Map/` de sa zone, dans le kit de la zone, et
   `world-maps.json` le nomme par un **chemin relatif à `Assets/`** ; un nom seul reste une carte
   peinte de `Maps/`. Le contrôle exige du rendu un JPEG 1920 × 1080 dans un `Map/` de `Regions/`.
2. **La grille plutôt qu'un plan à plat.** La vue d'un quartier posait le héros et les îlots en
   fractions de colonnes et de lignes, carte tracée nord en haut ; un rendu isométrique les aurait
   posés à faux. L'isométrie est affine : `--render --canvas 1920x1080` écrit, avec l'image, trois
   vecteurs (`origin`, `column`, `row`) que `world-maps.json` recopie (`grid`), et l'onglet pose
   par eux le héros, les îlots et les entrées. Sans grille, la lecture d'avant demeure.
3. **La sous-zone se nomme par son dossier.** `zones: { "arena-of-fate": … }` sous le quartier
   d'Arenarea : sa carte est `central-empire/capital/arenarea/arena-of-fate`, l'arborescence le
   dit (D-16) ; aucun identifiant de carte n'est recopié. Son repère se pose à son **entrée** sur
   la carte du quartier (le portail de l'arène, en cases), et s'ouvre sur sa carte dans la même
   vue ; « remonter » rend le quartier.
4. **Le quartier d'une sous-zone est le sien.** `CityPlan::districtOfMap` rendait vide dans
   l'Arena of Fate : le plan perdait le héros. Une carte rangée sous celle d'un quartier est dans
   ce quartier ; le héros de l'arène ou du niveau −1 est marqué à l'entrée de l'arène sur Arenarea,
   et à sa case sur la carte de l'arène quand il y marche.
5. **Grisé se lit de `capital.json`.** Un quartier du plan sans carte dans la ville est grisé,
   son nom toujours affiché (`MapMarker.locked`) : le schéma des villes n'a pas à les lister.
   `capital.json` reste provisoire — ses dix quartiers attendent carte ou porte gardée de la
   Capitale intra-muros (0.0.3).
6. **`--render` écrit le JPEG.** Sans application Qt, les greffons d'image ne se trouvaient pas :
   l'éditeur leur donne le dossier de l'exécutable avant ses commandes sans fenêtre.

## Livraison

Les trois rendus (Martpart, Arenarea, l'Arena of Fate) dans les kits de leurs zones, leurs grilles
et la sous-zone dans `world-maps.json`, les écrans du plan et du quartier, `check_map_assets.py`
étendu. Critères : `check_map_assets.py` et `check_rpg_data.py` verts ; tests `WorldMapsTest`
(grille, sous-zone, images livrées) et `CityPlanTest` (sous-zone) ; la vérification à l'écran,
souris et manette, reste due par l'auteur.
