+++
id = "LOT-122"
titre = "Recette et version 0.0.1"
version = "0.0.1"
filiere = "version"
statut = "a-faire"
taille = "M"
resume = "La démo est jouée, corrigée, empaquetée et taguée."
prerequis = ["LOT-120", "LOT-121"]
reprend = ["LOT-28 (version et tag ; l'audio part en 0.2.0)"]
livrables = [
  "Le numéro de version ramené à `0.0.1` dans `CMakeLists.txt` — il porte `0.1.0`, ce qui est un contresens.",
  "La recette : trois parties complètes par trois personnes, anomalies corrigées ou inscrites.",
  "L'installeur, les notes de version, le tag `v0.0.1`.",
  "Le bilan de la version dans `Planning/` : ce qui a coûté plus que prévu, ce que la `0.0.2` doit en retenir.",
]
criteres = [
  "Les quatre critères de sortie de la version sont tenus.",
  "L'installeur se lance sur un poste vierge (test de fumée de la release).",
]
+++

## Périmètre

Pas d'audio, pas d'effets : ils étaient au `LOT-28`, ils partent avec les dernières
fonctionnalités de la `0.2.0`.
