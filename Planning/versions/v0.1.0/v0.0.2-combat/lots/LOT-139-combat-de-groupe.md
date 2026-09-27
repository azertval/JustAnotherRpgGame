+++
id = "LOT-139"
titre = "Le combat de groupe"
version = "0.0.2"
filiere = "moteur"
statut = "a-faire"
taille = "L"
resume = "Quatre contre plusieurs : initiative mêlée, chaque personnage joué à son tour, alliés et ennemis qui se gênent et s'entraident."
prerequis = ["LOT-137", "LOT-138", "LOT-132", "LOT-133", "LOT-134", "LOT-135"]
reprend = ["LOT-29", "LOT-41 (rencontres, en partie)"]
livrables = [
  "L'entrée en combat du groupe : placement des quatre sur la zone de combat.",
  "Le tour de chaque personnage joué ; la fin de tour ; l'attente.",
  "Les effets d'**allié** : attaque sournoise, *bless*, soins, tenaille.",
  "Les rencontres **à plusieurs adversaires**, décrites en données, et leur budget de difficulté.",
]
criteres = [
  "Un combat à quatre contre quatre se joue de bout en bout dans l'Arena of Fate, au clavier comme à la manette.",
  "Le rejeu à graine fixée donne le même combat.",
  "L'IA répartit ses attaques : elle ne s'acharne pas sur un personnage à terre quand un autre menace.",
]
+++

## Pourquoi

Le moteur de combat sait déjà jouer plusieurs combattants par camp : le tour par tour du `LOT-20`
et l'IA du `LOT-23` n'ont jamais supposé un duel. Ce qui manque, c'est **le joueur à quatre** :
l'entrée en combat, la sélection, les effets entre alliés.

## Ce que le `LOT-138` lui laisse

- Le groupe existe en exploration, mais **seul le meneur** combat sur la carte : l'entrée en
  combat des quatre, sur la zone, est ici.
- Ce que le combat laisse **aux fiches** : les points de vie perdus, les lancers de sorts
  dépensés (`LOT-131`), un membre mort (`Dead`, `LOT-137`) qui ne suit plus le groupe. Aujourd'hui
  chaque rencontre relit les fiches pré-tirées, pleines.
