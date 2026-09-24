+++
id = "LOT-111"
titre = "Carte — Martpart"
version = "0.0.1"
filiere = "cartes"
statut = "a-faire"
taille = "M"
resume = "Martpart se parcourt."
prerequis = ["LOT-110", "LOT-103", "LOT-124", "LOT-125", "LOT-128"]
livrables = [
  "`Levels/central-empire/capital/martpart.json`, dessinée **dans l'éditeur**.",
  "Portails, points d'apparition nommés, zones (combat, déclencheurs de quête).",
]
criteres = [
  "`LevelEditor --check` passe : aucune case inatteignable, aucun portail sans arrivée, aucune référence morte.",
  "La carte tient 60 images par seconde à 1080p sur le poste de référence.",
]
maquettes = ["../maquettes/plan-martpart.svg"]
+++

## Conception

La carte comprend l'entrée du marché où le joueur apparaît ; la place des étals, où la mère interpelle le joueur ; Stravian Avenue jusqu'au portail d'Arenarea ; une ruelle. L'Illu Die Arena n'est **pas** dans la démo (version `0.0.4`).

![Plan de principe](../maquettes/plan-martpart.svg)

Le plan ci-dessus est un **schéma de principe** : il fixe ce que la carte contient et comment on y
circule, pas son dessin.
