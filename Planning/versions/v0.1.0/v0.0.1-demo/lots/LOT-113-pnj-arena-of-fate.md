+++
id = "LOT-113"
titre = "PNJ — Arena of Fate (donjon d'Arenarea)"
version = "0.0.1"
filiere = "pnj"
statut = "a-faire"
taille = "M"
resume = "Les habitants de Arena of Fate : figurines HD, portraits, fiches, placements."
prerequis = ["LOT-104", "LOT-112"]
livrables = [
  "`capital/arenarea/arena-of-fate/Characters/` pour les PNJ propres ; les archétypes de citadins dans `capital/Common/Characters/`.",
  "Pour chaque PNJ : figurine animée, portrait, jeton, et sa fiche (`Rpg/`) quand il en a une.",
  "Les placements sur la carte (entités), et les répliques d'ambiance.",
]
criteres = [
  "Chaque PNJ de la liste est visible à sa place sur la carte.",
  "Aucun portrait ni jeton n'est une image du corpus.",
  "Les archétypes communs resservent d'une zone à l'autre sans copie.",
]
sources = ["Référentiel : Arena of Fate dans `referentiels/central-empire/`"]
+++

## Les PNJ

- **Le maître d'arène** (vestiaire A) — il lance le combat. À tirer du livre s'il en nomme un, sinon à nommer.
- **Galender, the Weapon Master** (CC p. 48) et les **Twin Tigers** (CC p. 135) : présents, non combattus — trop forts pour la démo.
- Neutres : deux gardes Ironhand, un soigneur, des condamnés, la foule des gradins.
- **Hostile : le combattant de l'arène**, l'unique adversaire de la démo — gabarit Malfrat ou Berserker du *Manuel des Monstres*, à régler par simulation.

Proportions de la foule (livre, p. 90) : sur cent passants, 84 humains, 5 gnomes, 3 elfes d'été,
2 tieffelins, 2 nains, 2 soulborns. Voir [la population neutre](../../../../referentiels/central-empire/population-neutre.md)
et [les PNJ nommés](../../../../referentiels/central-empire/pnj-nommes.md).

## Une sous-zone, pas un quartier

L'Arena of Fate est un **donjon d'Arenarea** (décision D-16) : un lieu clos, à plusieurs salles —
vestiaire A, vestiaire B, couloir, sable —, où l'on entre **depuis le quartier**, par la porte de
l'arène au bout du parvis. Elle n'a pas d'entrée sur le plan de la Capitale : l'onglet « Carte »
la montre **dans** Arenarea. Ses assets et sa carte se rangent sous `capital/arenarea/arena-of-fate/`,
et elle puise d'abord dans le kit d'Arenarea, puis dans celui de la Capitale.

## Périmètre

Pas de routine de déplacement, pas de cycle jour / nuit : les PNJ de la démo sont **postés**.
