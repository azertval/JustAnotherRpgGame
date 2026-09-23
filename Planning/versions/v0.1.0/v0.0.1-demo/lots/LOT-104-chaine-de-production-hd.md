+++
id = "LOT-104"
titre = "La chaîne de production des assets HD"
version = "0.0.1"
filiere = "standard"
statut = "livre"
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
  "`scripts/assetsGeneration/install_hd_asset.py` : détourage (alpha continu, frange nettoyée), réduction à l'échelle du standard, mesure de l'ancre, écriture du manifeste.",
  "Le contrôle en CI : tout fichier cité par un manifeste existe, tout fichier d'image est cité, dimensions et poids dans les bornes du standard, **budget de 40 Mio par zone**.",
  "La galerie de débug (`--screen=AssetGallery`) qui lit la nouvelle arborescence (`EX-CNT-042` inchangée).",
  "Le gabarit de **commande d'une zone** : les dix familles du standard, ce qui vient du commun, ce qui est propre — une page par zone dans `Tools/AssetsHD/`.",
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

## Périmètre

La palette de l'éditeur qui propose les pièces du lieu **et** de ses niveaux communs était un livrable
de ce lot. L'[audit de l'éditeur](../../../../standards/audit-editeur.md) a montré que ce n'est pas une
palette mais tout ce qui, dans `Core` et l'éditeur, suppose un manifeste unique : c'est le
[LOT-124](LOT-124-editeur-et-arborescence-par-niveaux.md).

## Décisions de réalisation

### D1 — Le descripteur est versionné, les images non

La commande lit un `install.json` posé à côté des sources. Laissé sous `Tools/`, ignoré par git, il
ne se relirait pas et l'installation ne se rejouerait pas. Le `.gitignore` fait donc une exception
sous `Tools/AssetsHD/` pour `install.json` et les pages `*.md` de commande ; les images restent
hors du dépôt, et la règle 6 de l'[arborescence](../../../../standards/arborescence-assets.md) le dit.

### D2 — L'ancre se mesure aux deux pointes du socle

Une pièce debout n'a pas d'ancre dans sa source : l'art flotte dans un canevas carré. Le bord bas
d'un socle isométrique descend de sa pointe ouest vers le sud, puis remonte vers sa pointe est ; ces
deux pointes se lisent sur l'**enveloppe basse** de l'art (le point qui maximise `bas − m·x`, puis
`bas + m·x`), et de leur écart se déduit l'étendue du socle le long des deux axes de la grille. Le
socle remplit son emprise sur l'axe où il est le plus court par rapport à elle : c'est l'échelle ;
le sommet nord de l'emprise, la convention de `core::ScenePiece`, s'en déduit. Une première version
lisait la colonne la plus à gauche et la plus à droite : la corniche de l'angle rentrant, qui
déborde plus loin que son socle, la trompait (échelle 0,36 au lieu de 0,71).

Sur le Colisée, sans aucune correction, les murs tombent à 0,575, l'angle rentrant à 0,711 et
l'angle sortant à 0,678 : **un arc par case** dans les trois, alors que le générateur ne les a pas
dessinés à la même taille. La pente `m` est prise un peu plus raide que le losange (0,75 contre
0,62), parce que le générateur dessine plus plat (rapport des dalles : 0,57 à 0,61).

### D3 — Un mur se colle au bord nord de son emprise

Le mur du générateur n'a presque pas d'épaisseur (0,03 case). Sur l'axe qu'un socle ne remplit pas,
il est centré par défaut ; les murs du Colisée sont posés `align: north`, collés au bord nord de
leur rangée — là où sont, par construction, les bras d'un angle rentrant qu'ils doivent rejoindre.
Posées par leurs seules ancres sur une place d'essai de sept cases sur sept (montage hors dépôt),
les pièces ferment l'enclos sans jour.

### D4 — Une planche se découpe, et chaque morceau se nomme

Les deux sources de sol du Colisée portent six dalles chacune. Chaque morceau d'un seul tenant est
une pièce, dans l'ordre de lecture ; le descripteur les nomme toutes (`sheet`), et un compte qui ne
tombe pas juste est une erreur, jamais une affectation au hasard. L'étiquetage se fait sur une
grille de 4 px, en Python seul : scipy n'est pas sur le poste, et une planche de 1254 px se découpe
en moins d'une seconde.

### D5 — Le détourage ne touche pas la couleur

L'alpha du générateur porte un voile de 1 à 16 autour de la pièce et un intérieur à 252-254 : il est
étiré entre 16 et 248, les îlots de moins de 0,5 % de l'art sont effacés, et la réduction se fait en
**alpha prémultiplié**, pour que le bord ne tire pas vers le noir du fond transparent. Aucune
décontamination de couleur : le contour bronze foncé est voulu par [la consigne](../../../../standards/consigne-2d-hd.md),
et le « redresser » l'effacerait. Une dalle est étirée au losange exact du lieu (256 × 159) : l'écart
de rapport du générateur, jusqu'à 8 % en hauteur, ne se voit pas une fois le sol posé.

### D6 — Le manifeste garde la trace de la source

Chaque entrée installée porte sa `family` et sa `source` (chemin sous `Tools/AssetsHD/` et empreinte
SHA-256). `install_hd_asset.py --check` rejoue l'installation et compare, pixel à pixel : une image
retouchée à la main se voit. Ce contrôle demande les sources, donc il est local ; celui de la CI,
`check_hd_assets.py`, n'en a pas besoin.

### D7 — Le budget se pèse par lieu, sous-zone à part

Une zone pèse ses dossiers `Scene/`, `Characters/` et `Map/`, sans ceux de ses sous-zones, qui ont
leur propre ligne et leur propre budget de 40 Mio : c'est le critère du LOT-106 pour l'Arena of
Fate. Les communs sont pesés et affichés, sans budget.

### D8 — La galerie lit l'arborescence, et garde ses anciennes lectures

Chaque `manifest.json` sous `Common/` et `Regions/` y fait une famille (« Scène · central-empire/
capital/arenarea/arena-of-fate »). Les lectures de l'ancienne forme (`Npc/`, `Monsters/`,
`Coliseum/`, `Scene/<lieu>/`) restent : les données d'essai (`Fixtures/GameData`) les emploient
encore. Leur retrait va avec celui de ces données.

## Ce qui a été installé

Les 18 pièces de `Tools/AssetsHD/Colisee/`, dans
`Regions/central-empire/capital/arenarea/arena-of-fate/Scene/` : quatre dalles de sable, quatre de
pavé, quatre bordures sable / pavé (nord-ouest, nord-est), quatre murs à arcades (U et V, neufs et
usés), l'angle rentrant et l'angle sortant — 2,2 Mio, 6 % du budget de la sous-zone. Les **décors**
n'y sont pas : les bandes de foule sont en pixel art et se refont, et l'emprise des deux gardiens
se décide avec l'inventaire du [LOT-106](LOT-106-assets-hd-arena-of-fate.md).

Livré le 22 septembre 2026, **PR #111**.

## Ce qui reste

- Les sources du Colisée sont dans `Tools/AssetsHD/Colisee/`, pas sous le même arbre que leur zone
  (`definition-de-livre.md`, filière assets) : le LOT-106 les range en ouvrant la commande de la
  zone ; le descripteur dit déjà où elles s'installent.
- La galerie montre les pièces HD dans sa grille de 68 px : à leur taille d'art, donc trois à quatre
  fois plus grandes que les anciennes. Leur échelle à l'écran est l'affaire du
  [LOT-103](LOT-103-rendu-hd.md).

## Risques

- La mesure automatique de l'ancre sur un canevas carré où l'art flotte : prévoir une
  correction manuelle dans le descripteur. **Tenu** : `scale`, `anchorOffset` et `align` ; aucune
  n'a servi au Colisée hors `align`.
