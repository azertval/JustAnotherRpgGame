+++
id = "LOT-141"
titre = "La fiche et l'écran de groupe"
version = "0.0.2"
filiere = "interface"
statut = "a-faire"
taille = "M"
resume = "La fiche de personnage montre la classe : capacités, sorts, progression ; l'écran de groupe montre les quatre."
prerequis = ["LOT-138", "LOT-132", "LOT-133", "LOT-134", "LOT-135"]
reprend = ["LOT-38", "LOT-74 (progression, en partie)"]
livrables = [
  "La fiche : onglet **Classe** (capacités acquises, à venir), onglet **Sorts** (lancers restants).",
  "L'écran **Groupe** : les quatre profils côte à côte, le meneur, l'ordre de marche.",
  "La montée de niveau, du niveau 1 au niveau 5.",
]
criteres = [
  "Chacune des quatre fiches préfabriquées s'affiche comme sa page du livre, valeur pour valeur.",
]
+++

## Périmètre

La montée de niveau est **donnée** (par la quête, par un bouton de débogage) : l'expérience et sa
courbe arrivent en `0.2.0`.
