+++
id = "LOT-143"
titre = "Éditeur — des zones de combat pour un groupe"
version = "0.0.2"
filiere = "editeur"
statut = "a-faire"
taille = "S"
resume = "En dessinant une zone de combat, on voit si quatre personnages et la rencontre entière y tiennent, et ce que pèse la rencontre."
prerequis = ["LOT-139"]
livrables = [
  "Le verdict tactique d'une zone de combat compte le **groupe de quatre** et tous les adversaires de la rencontre : places d'entrée, formation, cases libres autour.",
  "Le **budget de difficulté** de la rencontre (LOT-139) affiché à côté de l'entité `encounter`, pour un groupe de niveau donné.",
  "`--check` : une rencontre dont le groupe ne peut pas se déployer est une erreur.",
]
criteres = [
  "La zone de combat de l'Arena of Fate porte un quatre contre quatre ; réduite de moitié au canevas, son verdict passe au rouge pendant qu'on la tire.",
  "Un scénario `--apply` couvre le cas ; aucun code par famille n'est ajouté à l'éditeur.",
]
+++

## Pourquoi

Le verdict d'une zone de combat (`LOT-EDITOR-05`) a été écrit pour un duel : il vérifie que la
formation **adverse** tient sur le terrain. Avec le [LOT-139](LOT-139-combat-de-groupe.md), c'est
le groupe du joueur qu'il faut aussi placer. Toutes les zones de combat des versions suivantes en
dépendent.
