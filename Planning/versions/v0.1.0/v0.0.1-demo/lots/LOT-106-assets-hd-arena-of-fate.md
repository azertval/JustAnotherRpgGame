+++
id = "LOT-106"
titre = "Assets HD — Arena of Fate (donjon d'Arenarea)"
version = "0.0.1"
filiere = "assets"
statut = "a-faire"
taille = "L"
resume = "Les pièces propres à Arena of Fate, produites au standard et installées dans `capital/arenarea/arena-of-fate/Scene/`."
prerequis = ["LOT-105", "LOT-108"]
livrables = [
  "`Regions/central-empire/capital/arenarea/arena-of-fate/Scene/` : pièces, `manifest.json`, `appearance.json`.",
  "La commande de la zone (`Tools/AssetsHD/`) : les dix familles passées en revue, ce qui vient du kit, ce qui est propre.",
  "La page de galerie de la zone.",
]
criteres = [
  "Toutes les pièces de l'inventaire ci-dessous paraissent dans la galerie, à l'échelle du standard.",
  "Aucune pièce ne double une pièce du kit commun.",
  "La zone pèse moins de 40 Mio.",
]
sources = [
  "Tanares Sourcebook, p. 91-92, 99 : seule arène où l'on conteste un décret impérial",
]
+++

## Le lieu

L'amphithéâtre ovale en bord de baie : sable, enceinte à arcades, gradins rouges, feux sur le pourtour, vestiaires.

## Inventaire des pièces propres

- Sable (3 variantes), pavé d'enceinte, bordures sable / pavé.
- Enceinte : mur à arcades en U et en V, angle rentrant, angle sortant, porte de l'arène, herse.
- **Gradins et foule en HD** — les bandes de foule existantes sont en pixel art et se refont.
- Les deux gardiens de l'entrée, les braseros du pourtour, la loge impériale.
- Vestiaire A et vestiaire B : râtelier d'armes, banc, paillasse, grille.

Le détail du quartier — texte du livre et lieux nommés sur le plan — est dans
[le référentiel de la Capitale](../../../../referentiels/central-empire/capitale.md).

## Une sous-zone, pas un quartier

L'Arena of Fate est un **donjon d'Arenarea** (décision D-16) : un lieu clos, à plusieurs salles —
vestiaire A, vestiaire B, couloir, sable —, où l'on entre **depuis le quartier**, par la porte de
l'arène au bout du parvis. Elle n'a pas d'entrée sur le plan de la Capitale : l'onglet « Carte »
la montre **dans** Arenarea. Ses assets et sa carte se rangent sous `capital/arenarea/arena-of-fate/`,
et elle puise d'abord dans le kit d'Arenarea, puis dans celui de la Capitale.

## Périmètre

Les **pièces de scène** seulement. Les figurines sont au lot des PNJ, la carte au lot de la carte.
