+++
id = "LOT-121"
titre = "L'onglet « Carte » : le plan de la Capitale"
version = "0.0.1"
filiere = "cartes"
statut = "a-faire"
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
