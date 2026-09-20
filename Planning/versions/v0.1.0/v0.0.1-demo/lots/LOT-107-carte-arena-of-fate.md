+++
id = "LOT-107"
titre = "Carte — Arena of Fate"
version = "0.0.1"
filiere = "cartes"
statut = "a-faire"
taille = "M"
resume = "Arena of Fate se parcourt, et figure dans l'onglet « Carte »."
prerequis = ["LOT-106", "LOT-103"]
livrables = [
  "`Levels/central-empire/capital/arena-of-fate.json`, dessinée **dans l'éditeur**.",
  "`capital/arena-of-fate/Map/` : l'image de la zone pour l'onglet « Carte », et son entrée dans `world-maps.json`.",
  "Portails, points d'apparition nommés, zones (combat, déclencheurs de quête).",
]
criteres = [
  "`LevelEditor --check` passe : aucune case inatteignable, aucun portail sans arrivée, aucune référence morte.",
  "La carte tient 60 images par seconde à 1080p sur le poste de référence.",
  "L'onglet « Carte » montre la zone et la position du joueur.",
]
maquettes = ["../maquettes/plan-arena-of-fate.svg"]
+++

## Conception

La carte comprend une **pré-carte** d'abord, montée avec les pièces déjà présentes dans `Tools/AssetsHD/Colisee/` (sols, murs, angles, gardiens) : elle éprouve la chaîne et le rendu HD avant que le reste des pièces existe. Puis la carte finale : le sable (zone de combat), l'enceinte, le vestiaire A où arrive le condamné, le vestiaire B, le couloir, la porte vers Arenarea.

![Plan de principe](../maquettes/plan-arena-of-fate.svg)

Le plan ci-dessus est un **schéma de principe** : il fixe ce que la carte contient et comment on y
circule, pas son dessin.
