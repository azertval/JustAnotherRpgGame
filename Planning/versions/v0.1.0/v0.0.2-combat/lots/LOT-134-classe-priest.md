+++
id = "LOT-134"
titre = "Classe — Priest"
version = "0.0.2"
filiere = "regles"
statut = "a-faire"
taille = "M"
resume = "Le Priest se joue du niveau 1 au niveau 5, capacités en main."
prerequis = ["LOT-131"]
livrables = [
  "`Rpg/classes/priest.json` : la table complète des niveaux 1 à 20, saisie de la page.",
  "Les capacités et les sorts des niveaux 1 à 5, avec leur effet en combat et leur ligne de journal.",
  "Les icônes de capacité et de sort, à la charte v2.",
]
criteres = [
  "La fiche préfabriquée du Priest se joue en combat avec toutes ses capacités de niveau 1.",
  "Chaque capacité a un test qui la déclenche et vérifie son effet.",
  "Monter du niveau 1 au niveau 5 donne ce que dit la table.",
]
sources = ["Player's Guide to Tanares, p. 200-203"]
+++

## Les capacités des niveaux 1 à 5

- N1 tours *light*, *sacred flame* ; sorts *bless*, *cure wounds* (2 par jour chacun).
- N3 *spare the dying*, *lesser restoration*, *spiritual weapon*.
- N5 *daylight*, *revivify*.

*Bless* demande la concentration ; *revivify* demande les règles de mort du LOT-137.

La progression complète est dans [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).
