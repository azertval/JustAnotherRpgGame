# L'arborescence des assets

Treize régions, une centaine de zones, des centaines de PNJ : le volume d'assets sera gigantesque.
L'arborescence doit répondre d'avance à une seule question — *cet asset, où va-t-il ?* — et la
réponse ne doit jamais être « ça dépend ». Elle est **proposée** ici et mise en place par le
[LOT-102](../versions/v0.1.0/v0.0.1-demo/lots/LOT-102-table-rase-assets-et-cartes.md).

## Le principe : du commun vers le propre

Un asset vit au niveau **le plus bas qui couvre tous ses usages**. Cinq niveaux, du plus
partagé au plus particulier :

| Niveau | Dossier | Ce qui y vit |
|---|---|---|
| **Monde** | `Common/` | ce qui existe partout : herbe, terre, eau, rochers, arbres, caisses, effets de sort, animaux, peuples génériques, héros |
| **Région** | `Regions/<région>/Common/` | ce qui fait l'identité d'une région : pierre calcaire et bourgogne de l'Empire, bannières au lion, soldats Ironhand |
| **Ville** | `Regions/<région>/<ville>/Common/` | ce que les quartiers d'une ville partagent : pavage de la Capitale, lampadaires, fontaines, citadins |
| **Zone** | `Regions/<région>/<ville>/<zone>/` | ce qu'on ne voit que là : la fontaine d'Arenarea, l'étal de la mère, un PNJ nommé |
| **Sous-zone** | `…/<zone>/<sous-zone>/` | un **donjon** : un lieu clos où l'on entre depuis la zone — l'Arena of Fate dans Arenarea, Phantom Fortress et ses étages |

Une zone hors d'une ville (une forêt, une forteresse) se range directement sous sa région.

## L'arbre

```
Source/Elements/Assets/
├── Common/
│   ├── Terrain/                 sols naturels
│   ├── Nature/                  arbres, rochers, buissons
│   ├── Props/                   mobilier et objets génériques
│   ├── Fx/                      effets de combat et de sort
│   └── Characters/
│       ├── Heroes/<classe>/     les personnages jouables
│       ├── Peoples/<espèce>/<archétype>/    PNJ neutres génériques
│       ├── Beasts/<bête>/       les animaux
│       └── Monsters/<monstre>/  les monstres sans région
├── Regions/
│   └── central-empire/
│       ├── region.json          palette, échelle d'art, crédits de la région
│       ├── Common/
│       │   ├── Scene/           kit impérial : architecture, mobilier, bannières
│       │   └── Characters/      gardes, soldats Ironhand, fonctionnaires
│       ├── capital/
│       │   ├── Common/
│       │   │   ├── Scene/       kit de la Capitale
│       │   │   └── Characters/  citadins de la Capitale
│       │   ├── arenarea/
│       │   │   ├── Scene/       pièces propres + manifest.json + appearance.json
│       │   │   ├── Characters/  PNJ nommés du quartier
│       │   │   ├── Map/         l'image de la zone pour l'onglet « Carte »
│       │   │   └── arena-of-fate/   sous-zone (donjon) : mêmes trois dossiers
│       │   └── martpart/
│       ├── skybell-city/
│       └── great-forest-of-bak/
├── Maps/                        les cartes peintes de l'onglet « Carte » — inchangées, déjà au standard
├── UI/                          le HUD et l'interface (charte v2) — inchangés, déjà au standard
├── Fonts/                       inchangé
└── Entities/                    familles d'icônes RPG — inchangé
```

Les **cartes jouables** suivent le même découpage, dans `Source/Elements/Levels/` :
`Levels/central-empire/capital/arenarea.json`.

## Les règles

1. **Naître propre, monter par promotion.** Un asset naît dans sa zone. Le jour où une deuxième
   zone en a besoin, il **monte** au premier niveau commun aux deux — il n'est **jamais copié**.
   Monter un asset est un renommage outillé (`LevelEditor --replace-piece`, livré au
   `LOT-EDITOR-14`, qui réécrit les cartes qui le citent), pas une retouche à la main.
2. **La résolution descend, puis remonte.** Une scène déclare son lieu ; le moteur cherche une clé
   dans la sous-zone, puis la zone, la ville, la région, le monde. Une zone peut donc **remplacer**
   une pièce commune par la sienne, sous la même clé.
3. **Un dossier, un manifeste.** Chaque dossier `Scene/` et `Characters/` porte un `manifest.json`
   qui liste ses pièces : clé, fichier, emprise, ancre, type tactique. Un fichier que le manifeste
   ne cite pas fait échouer la CI (`EX-CNT-042`, inchangée : tout asset livré paraît dans la
   galerie de débug).
   Un kit volumineux se **range en sous-dossiers** sous son `Scene/`, sans second manifeste : le
   manifeste du dossier cite chaque pièce par son chemin (`"file": "roofs/l/d3/roof-l-d3-ne-c0r0.png"`),
   la clé ne change pas, les cartes non plus. Les sous-dossiers suivent les familles, en anglais :
   `floors/`, `walls/`, `columns/`, `balustrades/`, `plants/`, `props/`, `roofs/<sorte>/d<largeur>/`.
   Le rangement s'écrit dans le descripteur (`"folders"`, règles sur le nom de la pièce) : une
   réinstallation le garde. Le kit de la Capitale est le premier rangé ainsi (`LOT-129`).
4. **Les noms.** Identifiants en anglais, minuscules, tirets : `arena-of-fate`, `wall-arcade-u`.
   Les noms de lieux sont ceux de l'atlas (`World/locations/`), sans leur préfixe de région.
   Une pièce se nomme `<famille>-<objet>[-<variante>]` : `floor-sand-01`, `wall-arcade-u`,
   `prop-lamppost`.
5. **Un PNJ, un dossier** : `<pnj>/portrait.png`, `<pnj>/token.png`, une planche et un
   `.anim.json` par animation. Un PNJ **nommé** vit dans la zone où on le rencontre ; un
   archétype (citadin, marchande, garde) vit dans le commun de sa ville ou de sa région.
6. **`Tools/` n'est jamais livré.** C'est le répertoire de travail **local** de l'auteur : sorties
   brutes du générateur, planches de référence, masters en pleine définition, et aussi la
   [commande de la zone](gabarit-commande-zone.md) et le descripteur `install.json` — sous
   `Tools/AssetsHD/`, rangés **sous le même arbre**. Rien n'en est versionné (`.gitignore`, décision
   de l'auteur du 23 septembre 2026, qui retire l'exception du `LOT-104`). Le dépôt ne reçoit que
   l'asset **installé** : détouré, réduit à l'échelle du standard, ancré, inscrit au manifeste —
   par `scripts/assetsGeneration/install_hd_asset.py`, jamais à la main.

## Le poids

| Garde-fou | Valeur | État |
|---|---|---|
| Taille d'un fichier binaire | 5 Mio | contrôlé (`check_binary_files.py`) |
| Poids d'une zone | **pas de budget** ([D-23](../vision/decisions.md)) | pesé, pas borné (`check_hd_assets.py`, [LOT-104](../versions/v0.1.0/v0.0.1-demo/lots/LOT-104-chaine-de-production-hd.md)) : chaque zone et chaque sous-zone, sans ses sous-zones ; le poids s'affiche dans le résumé du job CI |
| Poids du dépôt | question ouverte au-delà de **1 Gio** : Git LFS, ou dépôt d'assets à part | à trancher avant la `0.1.0`, voir [les risques](../vision/risques.md) |

**Un kit ne se bride pas pour tenir un poids** (décision de l'auteur, 24 septembre 2026) : un jeu
lourd mais riche et immersif vaut mieux qu'une zone pauvre. Le commun de la Capitale pèse
101 Mio après le LOT-108 ; l'Empire central dépassera le gibioctet, et la question du stockage (Q-08) **se posera
tôt**. Le découpage par région est ce qui permettra d'y répondre sans tout déplacer.
