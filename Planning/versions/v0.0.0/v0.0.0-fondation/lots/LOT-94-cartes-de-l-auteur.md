+++
id = "LOT-94"
titre = "Les images du corpus quittent le dépôt ; l'écran « Carte » revient sur les cartes de l'auteur : monde, région, ville"
version = "0.0.0"
filiere = "interface"
statut = "livre"
taille = "L"
resume = "Plus aucune image du corpus n'est dans le dépôt, et l'écran « Carte » se consulte à trois niveaux — monde, région, ville — sur seize cartes peintes par l'auteur."
prerequis = ["LOT-87", "LOT-39", "LOT-37"]
livrables = [
  "`check_ui_assets.py` refuse toute illustration d'interface dont la provenance n'est pas `produced` ; les deux cartes du corpus et la commande `sourcebook illustrations` retirées.",
  "Le fond du menu principal, produit : `ui/background/menu-scene` (`Source/Elements/Assets/UI/background/`, `illustrations.json`, `Artwork.qml`).",
  "Les seize cartes de l'auteur, `Source/Elements/Assets/Maps/` et leur `manifest.json` (provenance `author`), contrôlées par `scripts/check_map_assets.py` en CI.",
  "`Source/Elements/Maps/world-maps.json` : repères et cadres des régions, 61 lieux placés, 19 entrées `omitted`, 103 noms de géographie, quartiers de la Capitale et sites de Fisherman's Wharf.",
  "`hmi::readWorldMaps`, `hmi::joinWorldMaps` (`Source/HMI/Presentation/WorldMaps.*`), `hmi::WorldMapModel` ; `RpgScreenId::WorldMap` et `ScreenRouter.WorldMap` rétablis.",
  "L'écran à trois niveaux : briques `MapCanvas`, `MapHud`, `MapSidePanel`, `MapMarker` ; `WorldMapForm`, `RegionMapForm`, `CityMapForm` et leurs jumeaux ; `--map-region=<id>`, `--map-city=<id>`.",
  "La section 12 de `interface-ihm.md` : `EX-IHM-106`, `EX-IHM-107`, et `EX-IHM-076` amendée.",
]
criteres = [
  "Aucun fichier du dépôt n'a la provenance `tanares`, et `check_ui_assets.py` le refuse en CI.",
  "Le menu principal et les crédits s'affichent sur le fond produit, à la capture de référence près.",
  "L'écran « Carte » s'ouvre depuis le cadre de jeu, descend du monde à une région puis à une ville et remonte, au clavier, à la manette et à la souris ; une capture de référence QML par niveau.",
  "`check_map_assets.py` est vert : « 16 carte(s) conforme(s) au manifeste, à l'atlas et à world-maps.json ».",
  "`test_world_maps.cpp` couvre la lecture et la jointure ; `test_rpg_screens.cpp` compte neuf écrans.",
  "`lint_lots.py` et `lint_exigences.py` verts ; le `LOT-42`, le `LOT-96`, le [LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27) et le [LOT-28](../../../../vision/archives/feuille-de-route-jeu.md#lot-28) citent ce lot.",
]
sources = ["Tanares Sourcebook, p. 44 (la carte du monde, retirée du dépôt par ce lot)"]
+++

## Pourquoi

Le §3 de la [feuille de route](../../../../vision/archives/feuille-de-route-jeu.md) laisse les licences « en sommeil » pour la **donnée**
et le vocabulaire du corpus (projet privé), pas pour ses **images** : le plan de la ville, la carte
du monde et les planches sont des œuvres, et le jeu ne les affiche pas. Deux l'étaient pourtant, les
seules images du corpus commises : `UI/world-map.jpg` (Sourcebook, page 44) et
`UI/world-map-hd.jpg` (`VTT/Map - World.jpg`), fond de l'écran de carte du monde, mais aussi du
menu principal, des crédits et des options, faute d'une pièce `menu-scene` jamais produite.

Ce lot les retire, met le dépôt en règle avec lui-même — et, le même jour, rend au jeu son écran
« Carte », sur des cartes qui sont **celles de l'auteur**.

### Deux temps, le même jour

**1. Le retrait (17 septembre 2026, matin).** Les deux cartes partent, et l'écran avec elles :
formulaire, jumeau, `WorldMapModel`, `WorldMapRegions`, `world-map-regions.json`, test, capture de
référence, l'entrée de `RpgScreenId` et de `ScreenRouter`, la commande `sourcebook illustrations`
qui les extrayait. `check_ui_assets.py` refuse toute provenance autre que `produced` ;
`EX-IHM-076` est refondue : une illustration d'interface est **produite**, jamais extraite. La page
écrivait alors « la carte du monde est supprimée pour le moment », et renvoyait au `LOT-42` le soin
d'en **générer** une.

**2. Le retour (17 septembre 2026, soir).** L'auteur livre **seize cartes qu'il a peintes
lui-même**, en 1 920 × 1 080 et **sans lettrage** : le monde, les treize régions de l'atlas, et deux
plans de ville — la Capitale impériale et Fisherman's Wharf. *Décision de l'auteur* : « cela
correspond au `LOT-94` + une refonte totale de l'interface de carte, avec une vue complète, une vue
zoom région et une vue zoom ville (deux villes prêtes) ; cela couvre également le `LOT-95` ». La
phrase du matin est donc **remplacée** : l'écran « Carte » revient, sur les cartes de l'auteur —
jamais sur une image du corpus, règle qui reste entière (`EX-IHM-076`).

## Périmètre

### Ce que ce lot livre

- **Plus une image du corpus dans le dépôt**, et un lint qui le tient : `check_ui_assets.py`
  refuse toute illustration d'interface dont la provenance n'est pas `produced`.
- **Le fond du menu principal, produit** : la pièce `ui/background/menu-scene` du cahier
  (`Source/Elements/Assets/UI/background/`, `illustrations.json`, `Artwork.qml`) ; les captures de
  référence du menu principal et des crédits sont régénérées.
- **Les seize cartes de l'auteur** (`Source/Elements/Assets/Maps/`) : seize JPEG à la qualité 88,
  convertis depuis les PNG de l'auteur (15,9 Mo au lieu de 66), et un `manifest.json` — provenance
  `author`, fichier source, date, taille, empreinte SHA-256. `scripts/check_map_assets.py`, en CI
  (étape `map_assets`), recoupe les images, le manifeste, `world-maps.json` et l'atlas.
- **Les positions, à part de l'atlas** (`Source/Elements/Maps/world-maps.json`) : le repère et le
  cadre approché de chaque région sur le monde ; **61 lieux** de l'atlas placés sur les cartes de
  région, **19 entrées** de l'atlas déclarées `omitted` (règles, objets, personnages, doublons),
  **103 noms de géographie** ; les **douze quartiers** de la Capitale (lieux de l'atlas) et les
  **douze sites numérotés** de Fisherman's Wharf (légende du livre) sur leur plan.
- **Le modèle** : `hmi::readWorldMaps` et `hmi::joinWorldMaps`
  (`Source/HMI/Presentation/WorldMaps.*`, purs, testés par `test_world_maps.cpp`),
  `hmi::WorldMapModel` (module `Jadg.Runtime`) ; `RpgScreenId::WorldMap` et
  `ScreenRouter.WorldMap` rétablis — les écrans du RPG sont de nouveau **neuf** ; le bouton
  « Carte » du cadre de jeu (`GameView`, `CombatHud`) est rallumé.
- **L'écran, à trois niveaux** : les briques `MapCanvas`, `MapHud`, `MapSidePanel` et `MapMarker`
  (refondue) ; trois formulaires, `WorldMapForm`, `RegionMapForm`, `CityMapForm`, et leurs jumeaux
  `WorldMap.qml` (qui orchestre les trois niveaux), `RegionMap.qml`, `CityMap.qml` ; une capture de
  référence QML par niveau. `--map-region=<id>` et `--map-city=<id>` ouvrent un niveau directement.
- **La spécification** : la section 12 de `interface-ihm.md`, `EX-IHM-106` et `EX-IHM-107` ;
  `EX-IHM-076` amendée — les cartes peintes par l'auteur ont leur propre manifeste.

#### Les commandes, les mêmes aux trois niveaux

| Geste | Clavier | Manette | Souris |
|---|---|---|---|
| Repère voisin | flèches | croix directionnelle | — |
| Lieu suivant de la liste | Tab, Page préc., Page suiv. | LB, RB | — |
| Ouvrir le niveau suivant | Entrée | A | clic |
| Remonter d'un niveau | Retour arrière, Échap | B | clic droit |
| Agrandir, réduire | +, − | X, Y | molette |
| Déplacer la carte | — | — | glisser |

Depuis le niveau monde, Échap ferme l'écran.

### Ce qui reste hors du lot, nommément

Le `LOT-95` prévoyait plus que ce que ce lot a absorbé. Le plan de la ville, l'écran de zoom et la
Capitale dans l'atlas (elle y est depuis le [LOT-92](LOT-92-atelier-textures.md) :
`central-empire-the-capital-city`) sont faits ici. Le reste passe au **`LOT-96`**, qui remplit déjà
`capital.json` :

- `Source/Elements/World/capital.json` — les douze quartiers, la porte de départ, la carte de
  niveau que chaque quartier désigne ;
- les niveaux **quartier** et **îlot** du plan de la Capitale, sous le niveau ville ;
- la **position du héros** et les **quartiers visités** sur le plan, persistés au
  [LOT-17](../../../../vision/archives/feuille-de-route-jeu.md#lot-17).

Au **`LOT-42`** (voyage) : les lieux **découverts**, la position de la compagnie, et le choix d'une
destination depuis l'écran. Il bâtit sur l'écran et les cartes livrés ici ; il n'a plus de carte du
monde à produire.

**Relevé à reprendre au fil de l'eau** : les positions ont été relevées **à l'œil**. Ctrl+clic dans
l'écran journalise la fraction de l'image sous le pointeur, pour retoucher `world-maps.json`.

## Conception

### Les décisions

| Sujet | Décision |
|---|---|
| **Images du corpus** | Des références, jamais des assets : ni affichées, ni décalquées. *Décision de l'auteur, 16 septembre 2026* ; elle reste entière après le retour de l'écran. |
| **Retour de l'écran « Carte »** | Sur les cartes **peintes par l'auteur**, à trois niveaux — monde, région, ville. Remplace « la carte du monde est supprimée pour le moment, le `LOT-42` en génèrera une ». *Décision de l'auteur, 17 septembre 2026.* |
| **`LOT-95`** | **Retiré par fusion** dans ce lot : le plan de la Capitale n'est plus à régénérer par l'atelier, l'auteur l'a peint. Le numéro n'est pas réattribué. *Même décision.* |
| **On ne se déplace pas sur la carte** | La carte sert à s'orienter et à choisir ; le déplacement se fait sur les cartes de niveau (`EX-IHM-106`). Inchangé depuis le 16 septembre. |
| **Aucun nom peint** | Les cartes sont sans lettrage ; tout nom est posé par le jeu, dans sa police et sa langue (`EX-IHM-107`). |
| **Positions à part de l'atlas** | L'atlas est **extrait** du livre, qui ne donne aucune coordonnée, et sa chaîne d'extraction effacerait un champ qu'elle n'a pas produit : les positions vivent dans `world-maps.json`. |
| **Ne jamais inventer une position** | Un lieu que le livre ne situe pas, ou qui sort du cadre peint, reste dans la **liste** de sa région, **sans repère**. Les îles de la Tempête n'ont ainsi aucun repère posé : la carte de l'auteur y est une composition libre. |
| **La carte de référence** | La carte lettrée du corpus (`VTT/Map - World.jpg`) a servi à **relever** les positions, à l'œil, région par région — lue sur le poste, jamais affichée, jamais commise. Les cartes de région sont repeintes, pas recadrées : aucune transformation mécanique de la référence vers elles ne tient. |
| **Une entrée de l'atlas qui n'est pas un lieu** | Écartée **nommément** (`omitted`) — une règle, un objet, un personnage, un doublon — plutôt que tue : le lint sait ainsi que chaque entrée de l'atlas a été traitée. |
| **JPEG, qualité 88** | Les seize PNG de l'auteur pèsent 66 Mo ; convertis, 15,9 Mo. Le manifeste garde le nom du fichier source de chaque carte. |
| **Provenance `author`** | Les cartes ne passent pas par le cahier des assets ni par `illustrations.json`, qui n'admet que `produced` : elles ont leur manifeste et leur lint (`EX-IHM-076`, amendée). |

## Exigences couvertes

- `EX-IHM-076` — une illustration d'interface est produite, jamais extraite du corpus ; les cartes
  peintes par l'auteur ont leur propre manifeste.
- `EX-IHM-106` — l'écran « Carte » a trois niveaux, les mêmes commandes aux trois, et l'on ne s'y
  déplace pas.
- `EX-IHM-107` — aucun nom peint, des positions tenues à part de l'atlas, jamais inventées.

## Journal

- **16 septembre 2026** — lot ajouté à la feuille de route à la relecture du plan d'intégration de
  la Capitale : les images du corpus sont des références, jamais des assets.
- **17 septembre 2026, le retrait.** Les deux cartes, l'écran de carte du monde et la commande
  d'extraction partent ; le cycle des écrans du RPG tombe à huit ; le bouton « Carte » reste à sa
  place, éteint ; menu, crédits et options retombent sur l'aplat `panel` de la charte.
- **17 septembre 2026, le retour.** L'auteur livre ses seize cartes et décide la refonte à trois
  niveaux, `LOT-95` compris. Conversion en JPEG et manifeste ; relevé des positions à l'œil contre
  la carte de référence ; modèle, briques, trois formulaires et leurs jumeaux ; section 12 de la
  spécification ; le cycle des écrans revient à neuf et le bouton « Carte » se rallume.
- **17 septembre 2026, le fond du menu.** `ui/background/menu-scene` livré ; captures du menu
  principal et des crédits régénérées.
- **17 septembre 2026, la feuille de route.** Le lot quitte la page pour ce dossier ; le `LOT-95`
  rejoint l'encart des numéros retirés ; `capital.json`, les niveaux quartier et îlot et la
  position du héros passent au `LOT-96` ; le `LOT-42` bâtit sur l'écran livré.

## Vérification

Les six critères de l'en-tête sont l'« acceptation, telle que vérifiée » à la livraison.

## Bilan

Statut : **livré le 17 septembre 2026** (ajouté à la feuille de route le 16 ; PR #65).
Alimente : [LOT-28](../../../../vision/archives/feuille-de-route-jeu.md#lot-28), `LOT-42`, `LOT-96`.
Absorbe : `LOT-95` (le plan de la Capitale), retiré par fusion le 17 septembre 2026.
Exigences couvertes : `EX-IHM-076` (refondue), `EX-IHM-106`, `EX-IHM-107` (détail plus haut).
