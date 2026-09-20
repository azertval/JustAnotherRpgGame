# L'arborescence des assets

Treize régions, une centaine de zones, des centaines de PNJ : le volume d'assets sera gigantesque.
L'arborescence doit répondre d'avance à une seule question — *cet asset, où va-t-il ?* — et la
réponse ne doit jamais être « ça dépend ». Elle est **proposée** ici et mise en place par le
[LOT-102](../versions/v0.1.0/v0.0.1-demo/lots/LOT-102-table-rase-assets-et-cartes.md).

## Le principe : du commun vers le propre

Un asset vit au niveau **le plus bas qui couvre tous ses usages**. Quatre niveaux, du plus
partagé au plus particulier :

| Niveau | Dossier | Ce qui y vit |
|---|---|---|
| **Monde** | `Common/` | ce qui existe partout : herbe, terre, eau, rochers, arbres, caisses, effets de sort, animaux, peuples génériques, héros |
| **Région** | `Regions/<région>/Common/` | ce qui fait l'identité d'une région : pierre calcaire et bourgogne de l'Empire, bannières au lion, soldats Ironhand |
| **Ville** | `Regions/<région>/<ville>/Common/` | ce que les quartiers d'une ville partagent : pavage de la Capitale, lampadaires, fontaines, citadins |
| **Zone** | `Regions/<région>/<ville>/<zone>/` | ce qu'on ne voit que là : l'enceinte de l'arène, l'étal de la mère, un PNJ nommé |

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
│       │   ├── arena-of-fate/
│       │   │   ├── Scene/       pièces propres + manifest.json + appearance.json
│       │   │   ├── Characters/  PNJ nommés et combattants de l'arène
│       │   │   └── Map/         l'image de la zone pour l'onglet « Carte »
│       │   ├── arenarea/
│       │   └── martpart/
│       ├── skybell-city/
│       └── great-forest-of-bak/
├── UI/                          l'interface (charte v2) — inchangée
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
   dans la zone, puis la ville, puis la région, puis le monde. Une zone peut donc **remplacer**
   une pièce commune par la sienne, sous la même clé.
3. **Un dossier, un manifeste.** Chaque dossier `Scene/` et `Characters/` porte un `manifest.json`
   qui liste ses pièces : clé, fichier, emprise, ancre, type tactique. Un fichier que le manifeste
   ne cite pas fait échouer la CI (`EX-CNT-042`, inchangée : tout asset livré paraît dans la
   galerie de débug).
4. **Les noms.** Identifiants en anglais, minuscules, tirets : `arena-of-fate`, `wall-arcade-u`.
   Les noms de lieux sont ceux de l'atlas (`World/locations/`), sans leur préfixe de région.
   Une pièce se nomme `<famille>-<objet>[-<variante>]` : `floor-sand-01`, `wall-arcade-u`,
   `prop-lamppost`.
5. **Un PNJ, un dossier** : `<pnj>/portrait.png`, `<pnj>/token.png`, une planche et un
   `.anim.json` par animation. Un PNJ **nommé** vit dans la zone où on le rencontre ; un
   archétype (citadin, marchande, garde) vit dans le commun de sa ville ou de sa région.
6. **Les sources ne sont pas versionnées.** Les sorties brutes du générateur, les planches de
   référence et les masters en pleine définition vivent dans `Tools/AssetsHD/` (ignoré par git),
   sous **le même arbre**. Le dépôt ne reçoit que l'asset **installé** : détouré, réduit à
   l'échelle du standard, ancré, inscrit au manifeste.

## Le budget

| Garde-fou | Valeur | État |
|---|---|---|
| Taille d'un fichier binaire | 5 Mio | contrôlé (`check_binary_files.py`) |
| Poids d'une zone | **40 Mio** visés | à contrôler — [LOT-104](../versions/v0.1.0/v0.0.1-demo/lots/LOT-104-chaine-de-production-hd.md) |
| Poids du dépôt | question ouverte au-delà de **1 Gio** : Git LFS, ou dépôt d'assets à part | à trancher avant la `0.1.0`, voir [les risques](../vision/risques.md) |

À quarante mébioctets par zone, l'Empire central (vingt zones) pèse 800 Mio et le monde complet
plusieurs gibioctets : la question du stockage **se posera**, et le découpage par région est ce
qui permettra d'y répondre sans tout déplacer.
