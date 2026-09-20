+++
id = "LOT-150"
titre = "La sauvegarde"
version = "0.0.3"
filiere = "moteur"
statut = "a-faire"
taille = "L"
resume = "Une partie se sauvegarde et se reprend : groupe, fiches, inventaire, drapeaux, position."
prerequis = ["LOT-142"]
reprend = ["LOT-17"]
livrables = [
  "Le format de sauvegarde, versionné, et sa migration.",
  "Emplacements, sauvegarde automatique au changement de carte.",
  "L'écran Charger / Sauvegarder.",
]
criteres = [
  "Une partie sauvegardée dans un quartier se reprend à l'identique, drapeaux de quête compris.",
  "Une sauvegarde d'une version antérieure se charge ou est refusée avec un message clair — jamais un plantage.",
]
+++

## Périmètre

Pas de sauvegarde **en combat** : on sauvegarde en exploration.
