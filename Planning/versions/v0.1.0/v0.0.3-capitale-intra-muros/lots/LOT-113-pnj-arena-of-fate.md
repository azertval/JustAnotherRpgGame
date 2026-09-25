+++
id = "LOT-113"
titre = "PNJ — Arena of Fate (donjon d'Arenarea)"
version = "0.0.3"
filiere = "pnj"
statut = "a-faire"
taille = "M"
resume = "Les habitants de Arena of Fate : figurines HD, portraits, fiches, placements."
prerequis = ["LOT-104", "LOT-112", "LOT-145", "LOT-158"]
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

## Reporté à la `0.0.3` (décision [D-25](../../../../vision/decisions.md), 25 septembre 2026)

Ce lot servait la démo. C'est un lot de *world building*, trop complexe pour elle : il rejoint la
`0.0.3`, où il s'inscrit avec les six autres quartiers intra-muros. Dans la démo, le maître d'arène
et le combattant de l'arène sont tenus par les **mannequins**
([LOT-145](../../v0.0.1-demo/lots/LOT-145-mannequins-de-remplacement.md)) ou par des **jetons**
(`LOT-128`), posés sur les cartes de principe du
[LOT-146](../../v0.0.1-demo/lots/LOT-146-cartes-de-principe-de-la-demo.md) ; leurs dialogues sont au
`LOT-120`. Ce lot leur donne leurs figurines, leurs portraits et la foule, avec le pinceau de foule
du `LOT-158`. Une figurine livrée remplace son mannequin sans toucher à la carte.

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
