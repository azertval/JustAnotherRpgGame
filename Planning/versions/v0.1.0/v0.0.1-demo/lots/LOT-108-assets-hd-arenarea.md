+++
id = "LOT-108"
titre = "Assets HD — Arenarea"
version = "0.0.1"
filiere = "assets"
statut = "a-faire"
taille = "L"
resume = "Les pièces propres à Arenarea, produites au standard et installées dans `capital/arenarea/Scene/`."
prerequis = ["LOT-105", "LOT-129"]
livrables = [
  "`Regions/central-empire/capital/arenarea/Scene/` : pièces, `manifest.json`, `appearance.json`.",
  "La commande de la zone (`Tools/AssetsHD/`) : les dix familles passées en revue, ce qui vient du kit, ce qui est propre.",
  "La page de galerie de la zone.",
]
criteres = [
  "Toutes les pièces de l'inventaire ci-dessous paraissent dans la galerie, à l'échelle du standard.",
  "Aucune pièce ne double une pièce du kit commun.",
  "Le sol de la zone est posé sur douze cases sur douze sans qu'aucun motif régulier n'apparaisse.",
  "La zone pèse moins de 40 Mio.",
]
sources = [
  "Tanares Sourcebook, p. 99 ; plan VTT (référence seule) : Herofate Ave, Gauntlet St, Golden Chalice Casino, Dusk of Justice, Hippodrome",
]
+++

## Le lieu

Le quartier des nobles : manoirs à colonnes de marbre, jardins de façade, fontaines de pierre, Herofate Avenue.

## Inventaire des pièces propres

- Tout ce que montre la planche de référence, famille par famille : façades de manoir, colonnes et colonnade, portes à bannières, balustrades, **la fontaine**, jardins, boutique à auvent, parvis à arche.
- Le **parvis de l'arène** : l'escalier monumental, l'Arena Gate.
- Les enseignes : Golden Chalice Casino, Dusk of Justice.

Le détail du quartier — texte du livre et lieux nommés sur le plan — est dans
[le référentiel de la Capitale](../../../../referentiels/central-empire/capitale.md).

## Le risque du sol

La maquette du [LOT-101](LOT-101-standard-2d-hd.md) a montré le poste qui coûte : répétée sur tout
l'écran, la dalle bordée de la planche de référence dessine un **treillis** sombre qui n'existe dans
aucune ville. Le sol se produit donc en premier et se juge en premier — d'abord une dalle de fond
sans bordure en trois variantes au moins (règle du [§4 du standard](../../../../standards/style-2d-hd.md)),
les panneaux bordés seulement ensuite, pour border une place ou tracer une allée. Le critère
ci-dessus se vérifie à l'œil, sur douze cases sur douze : c'est là que le moiré se voit, pas sur
quatre.

## Les toits : un entrant du LOT-129

La commande a fait paraître un besoin que le moteur ne couvre pas : les **toits**, et plus
largement les décors à plusieurs niveaux (étage de manoir posé sur le rez, toiture au sommet). Une
pièce ne se pose que sur une case au sol. Le besoin passe au [LOT-129](LOT-129-etages-et-toits.md),
qui sert aussi le Colisée et Martpart ; la production de ce lot est **suspendue** jusqu'à sa
livraison (décision de l'auteur, 23 septembre 2026). Les pièces d'étage propres à Arenarea restent
ici ; la toiture commune de la Capitale est au LOT-129.

## Périmètre

Les **pièces de scène** seulement. Les figurines sont au lot des PNJ, la carte au lot de la carte.
