+++
id = "LOT-117"
titre = "Un jet de compétence dans un dialogue"
version = "0.0.1"
filiere = "moteur"
statut = "a-faire"
taille = "S"
resume = "Une réponse de dialogue peut demander un jet : le joueur voit la compétence, le DD, le dé, le résultat."
prerequis = ["LOT-100"]
livrables = [
  "Le graphe de dialogue accepte une réponse **à jet** : compétence, DD, branche de réussite, branche d'échec.",
  "L'interface montre le jet : « Persuasion · DD 18 », le d20 lancé, le total, l'issue.",
  "Le jet passe par le d20 du `LOT-12` : graine, journal, rejeu.",
]
criteres = [
  "À graine fixée, le même dialogue donne la même issue.",
  "Une réponse déjà tentée et ratée ne se propose plus (drapeau).",
  "Le contrôle des dialogues refuse une réponse à jet sans branche d'échec.",
]
+++

## Périmètre

Un jet **simple** : pas d'avantage circonstanciel, pas d'aide d'un allié, pas de jet de groupe.
