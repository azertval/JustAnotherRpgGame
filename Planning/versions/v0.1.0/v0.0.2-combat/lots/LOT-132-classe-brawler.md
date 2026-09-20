+++
id = "LOT-132"
titre = "Classe — Brawler"
version = "0.0.2"
filiere = "regles"
statut = "a-faire"
taille = "M"
resume = "Le Brawler se joue du niveau 1 au niveau 5, capacités en main."
prerequis = ["LOT-131"]
livrables = [
  "`Rpg/classes/brawler.json` : la table complète des niveaux 1 à 20, saisie de la page.",
  "Les capacités et les sorts des niveaux 1 à 5, avec leur effet en combat et leur ligne de journal.",
  "Les icônes de capacité et de sort, à la charte v2.",
]
criteres = [
  "La fiche préfabriquée du Brawler se joue en combat avec toutes ses capacités de niveau 1.",
  "Chaque capacité a un test qui la déclenche et vérifie son effet.",
  "Monter du niveau 1 au niveau 5 donne ce que dit la table.",
]
sources = ["Player's Guide to Tanares, p. 192-195"]
+++

## Les capacités des niveaux 1 à 5

- N1 **Tough as Nails** : sans armure, CA = 10 + Dex + Con ; **résistance à tous les types de dégâts**.
- N3 **Hit the Mark** : +2 aux jets d'attaque.
- N5 **Extra Attack**.

La résistance à tout est la capacité la plus forte du niveau 1 : l'équilibrage des rencontres en dépend.

La progression complète est dans [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).
