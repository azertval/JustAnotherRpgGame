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
  "Le plan de la Capitale déjà peint par l'auteur, gardé tel quel, avec ses douze quartiers.",
  "Les deux quartiers de la démo cliquables, et l'Arena of Fate **dans** Arenarea (sous-zone, décision D-16) ; les dix autres quartiers grisés, avec leur nom.",
  "`World/cities/capital.json` rebranché sur les cartes des deux quartiers ; le plan de ville et le HUD de la carte sont **gardés** (décision D-15), seul le branchement change.",
]
criteres = [
  "Depuis l'onglet, on lit où l'on est et où l'on peut aller.",
  "`check_map_assets.py` et `check_rpg_data.py` sont verts.",
]
+++

## Périmètre

Pas de voyage rapide : le plan **montre**, il ne transporte pas.
