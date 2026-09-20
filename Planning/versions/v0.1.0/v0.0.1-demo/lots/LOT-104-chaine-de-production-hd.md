+++
id = "LOT-104"
titre = "La chaîne de production des assets HD"
version = "0.0.1"
filiere = "standard"
statut = "a-faire"
taille = "M"
resume = "Du brut du générateur à l'asset installé, un chemin outillé : détourer, réduire, ancrer, inscrire au manifeste, contrôler, montrer dans la galerie."
prerequis = ["LOT-101", "LOT-102"]
reprend = [
  "LOT-92 (atelier des textures)",
  "LOT-91 (atelier des PNJ)",
  "LOT-93 (atelier des monstres)",
  "LOT-CREATION-ASSETS",
]
livrables = [
  "`scripts/install_hd_asset.py` : détourage (alpha continu, frange nettoyée), réduction à l'échelle du standard, mesure de l'ancre, écriture du manifeste.",
  "Le contrôle en CI : tout fichier cité par un manifeste existe, tout fichier d'image est cité, dimensions et poids dans les bornes du standard, **budget de 40 Mio par zone**.",
  "La galerie de débug (`--screen=AssetGallery`) qui lit la nouvelle arborescence (`EX-CNT-042` inchangée).",
  "Le gabarit de **commande d'une zone** : les dix familles du standard, ce qui vient du commun, ce qui est propre — une page par zone dans `Tools/AssetsHD/`.",
  "La palette de l'éditeur qui propose les pièces du lieu **et** de ses niveaux communs.",
]
criteres = [
  "Les pièces de `Tools/AssetsHD/Colisee/` (sols, murs, angles) s'installent par la commande, sans retouche manuelle, et paraissent dans la galerie.",
  "Un fichier image non cité par un manifeste fait échouer la CI.",
  "Le poids de chaque zone s'affiche dans le résumé du job.",
]
+++

## Pourquoi

Cent zones, des centaines de PNJ : ce qui se fait à la main une fois se fera mal la centième. La
chaîne est le seul endroit où le standard est **appliqué** plutôt que rappelé.

## Conception

Les sources (sorties du générateur, masters à 1254 px et plus) restent dans `Tools/AssetsHD/`, hors
du dépôt, sous le même arbre que les assets installés. La commande lit une source et un descripteur
(famille, emprise, type tactique) ; elle écrit l'image réduite et l'entrée de manifeste.

Le générateur d'images reste un outil **manuel** : Claude ne dessine pas, et aucune génération ne
tourne en CI (règle du LOT-91, inchangée).

## Risques

- La mesure automatique de l'ancre sur un canevas carré où l'art flotte : prévoir une
  correction manuelle dans le descripteur.
