+++
id = "LOT-133"
titre = "Classe — Mage"
version = "0.0.2"
filiere = "regles"
statut = "a-faire"
taille = "M"
resume = "Le Mage se joue du niveau 1 au niveau 5, capacités en main."
prerequis = ["LOT-131"]
livrables = [
  "`Rpg/classes/mage.json` : la table complète des niveaux 1 à 20, saisie de la page.",
  "Les capacités et les sorts des niveaux 1 à 5, avec leur effet en combat et leur ligne de journal.",
  "Les icônes de capacité et de sort, à la charte v2.",
]
criteres = [
  "La fiche préfabriquée du Mage se joue en combat avec toutes ses capacités de niveau 1.",
  "Chaque capacité a un test qui la déclenche et vérifie son effet.",
  "Monter du niveau 1 au niveau 5 donne ce que dit la table.",
]
sources = ["Player's Guide to Tanares, p. 196-199"]
+++

## Les capacités des niveaux 1 à 5

- N1 tours *fire bolt*, *light* ; sorts *detect magic*, *magic missile* (2 par jour chacun).
- N2 **Arcane Protection** : sans armure, CA = 13 + Dex.
- N3 *mage hand*, *invisibility*, *scorching ray*.
- N5 *fireball*, *fly*.

*Fireball* demande les zones d'effet du `LOT-22` ; *fly* demande le vol, que la grille tactique connaît déjà comme attribut.

La progression complète est dans [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).
