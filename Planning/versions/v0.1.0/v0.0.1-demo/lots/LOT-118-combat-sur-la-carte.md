+++
id = "LOT-118"
titre = "Le combat sur la carte"
version = "0.0.1"
filiere = "moteur"
statut = "a-faire"
taille = "L"
resume = "Une rencontre engagée sur une carte d'exploration se joue **sur place** : la carte se fige, la grille paraît, le combat se joue, l'exploration reprend."
prerequis = ["LOT-103"]
reprend = ["LOT-27 (le combat posé sur la carte)", "LOT-24 (hérité)"]
livrables = [
  "`core::beginEncounter` monte une session de combat sur la zone de combat de la carte, gelée.",
  "L'interface de combat du `LOT-24` branchée sur la carte d'exploration, et non plus sur le seul Colisée.",
  "Les issues : victoire (retour à l'exploration, drapeau posé), défaite (écran de mort), fuite.",
  "Un dialogue peut **déclencher** une rencontre.",
]
criteres = [
  "Un combat se joue sur une carte de test, du déclenchement par dialogue au retour à l'exploration, au clavier et à la manette.",
  "Le rejeu à graine fixée donne le même combat qu'en arène.",
  "Aucune régression du mode arène.",
]
+++

## Pourquoi

L'interface de combat a été livrée sur le Colisée parce que le jeu n'avait pas d'exploration.
Le `LOT-09` l'a donnée ; il reste à poser le combat **dessus**. C'est la pièce de moteur la plus
lourde de la démo.
