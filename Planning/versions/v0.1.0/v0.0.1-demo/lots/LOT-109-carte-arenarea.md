+++
id = "LOT-109"
titre = "Carte — Arenarea"
version = "0.0.1"
filiere = "cartes"
statut = "a-faire"
taille = "M"
resume = "Arenarea se parcourt, et figure dans l'onglet « Carte »."
prerequis = ["LOT-108", "LOT-103"]
livrables = [
  "`Levels/central-empire/capital/arenarea.json`, dessinée **dans l'éditeur**.",
  "`capital/arenarea/Map/` : l'image de la zone pour l'onglet « Carte », et son entrée dans `world-maps.json`.",
  "Portails, points d'apparition nommés, zones (combat, déclencheurs de quête).",
]
criteres = [
  "`LevelEditor --check` passe : aucune case inatteignable, aucun portail sans arrivée, aucune référence morte.",
  "La carte tient 60 images par seconde à 1080p sur le poste de référence.",
  "L'onglet « Carte » montre la zone et la position du joueur.",
]
maquettes = ["../maquettes/plan-arenarea.svg"]
+++

## Conception

La carte comprend Herofate Avenue du portail de Martpart au parvis de l'Arena of Fate ; le parvis, où le garde et l'enfant sont interceptés ; deux îlots de manoirs et leurs jardins ; la façade du casino.

![Plan de principe](../maquettes/plan-arenarea.svg)

Le plan ci-dessus est un **schéma de principe** : il fixe ce que la carte contient et comment on y
circule, pas son dessin.
