+++
id = "LOT-135"
titre = "Classe — Scoundrel"
version = "0.0.2"
filiere = "regles"
statut = "a-faire"
taille = "M"
resume = "Le Scoundrel se joue du niveau 1 au niveau 5, capacités en main."
prerequis = ["LOT-131"]
livrables = [
  "`Rpg/classes/scoundrel.json` : la table complète des niveaux 1 à 20, saisie de la page.",
  "Les capacités et les sorts des niveaux 1 à 5, avec leur effet en combat et leur ligne de journal.",
  "Les icônes de capacité et de sort, à la charte v2.",
]
criteres = [
  "La fiche préfabriquée du Scoundrel se joue en combat avec toutes ses capacités de niveau 1.",
  "Chaque capacité a un test qui la déclenche et vérifie son effet.",
  "Monter du niveau 1 au niveau 5 donne ce que dit la table.",
]
sources = ["Player's Guide to Tanares, p. 204-207"]
+++

## Les capacités des niveaux 1 à 5

- N1 **Sneak Attack Simplified** : +1d8 à la première touche du tour contre une cible **adjacente à un allié** ; **Scoundrel's Agility** : pas d'attaque d'opportunité, vitesse +10 ft.
- N3 +2d8, **Adventurer's Aptitude**.
- N5 +3d8, +2 CA, **Precise Striker**.

L'attaque sournoise lit l'adjacence d'un allié : c'est le calcul de la prise en tenaille du `LOT-23`, réemployé. Elle n'a de sens qu'en **groupe** — le Scoundrel seul n'en profite jamais.

La progression complète est dans [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).
