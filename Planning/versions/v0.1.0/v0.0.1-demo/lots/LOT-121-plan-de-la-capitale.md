+++
id = "LOT-121"
titre = "L'onglet « Carte » : le plan de la Capitale"
version = "0.0.1"
filiere = "cartes"
statut = "a-faire"
taille = "S"
resume = "Le plan de la Capitale montre ses douze quartiers ; trois s'ouvrent, les autres s'annoncent."
prerequis = ["LOT-107", "LOT-109", "LOT-111"]
reprend = ["LOT-94", "LOT-96"]
livrables = [
  "Le plan de la Capitale repeint par l'auteur, avec ses douze quartiers et ses six portes.",
  "Les trois zones de la démo cliquables ; les neuf autres grisées, avec leur nom.",
  "`World/cities/capital.json` rebranché sur les trois cartes.",
]
criteres = [
  "Depuis l'onglet, on lit où l'on est et où l'on peut aller.",
  "`check_map_assets.py` et `check_rpg_data.py` sont verts.",
]
+++

## Périmètre

Pas de voyage rapide : le plan **montre**, il ne transporte pas.
