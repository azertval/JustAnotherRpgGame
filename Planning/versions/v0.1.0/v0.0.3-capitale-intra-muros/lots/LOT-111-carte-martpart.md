+++
id = "LOT-111"
titre = "Carte — Martpart"
version = "0.0.3"
filiere = "cartes"
statut = "a-faire"
taille = "M"
resume = "Martpart se parcourt."
prerequis = ["LOT-110", "LOT-103", "LOT-124", "LOT-125", "LOT-128", "LOT-146"]
livrables = [
  "`Levels/central-empire/capital/martpart.json`, dessinée **dans l'éditeur**.",
  "Portails, points d'apparition nommés, zones (combat, déclencheurs de quête).",
]
criteres = [
  "`LevelEditor --check` passe : aucune case inatteignable, aucun portail sans arrivée, aucune référence morte.",
  "La carte tient 60 images par seconde à 1080p sur le poste de référence.",
]
maquettes = ["../../v0.0.1-demo/maquettes/plan-martpart.svg"]
+++

## Reporté à la `0.0.3` (décision [D-25](../../../../vision/decisions.md), 25 septembre 2026)

Ce lot servait la démo. C'est un lot de *world building*, trop complexe pour elle : il rejoint la
`0.0.3`, où il s'inscrit avec les six autres quartiers intra-muros. Dans la démo, Martpart est une
**carte de principe** ([LOT-146](../../v0.0.1-demo/lots/LOT-146-cartes-de-principe-de-la-demo.md))
de 24 × 11 cases. Ce lot livre la carte définitive, qui la remplace sous le même identifiant.

## Conception

La carte comprend l'entrée du marché où le joueur apparaît ; la place des étals, où la mère interpelle le joueur ; Stravian Avenue jusqu'au portail d'Arenarea ; une ruelle. L'Illu Die Arena n'est **pas** dans la démo (version `0.0.4`).

![Plan de principe](../../v0.0.1-demo/maquettes/plan-martpart.svg)

Le plan ci-dessus est un **schéma de principe** : il fixe ce que la carte contient et comment on y
circule, pas son dessin.
