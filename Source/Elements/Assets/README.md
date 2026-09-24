# Elements/Assets/

**Assets graphiques éditables hors code** (`LOT-39`) : images et polices, copiées à côté de
l'exécutable au build (patron `Levels`/`Localization`, sous `Assets/`) et chargées à l'exécution.
Chaque sous-dossier est une **famille** ; la plupart sont **engendrées** par un script ou un atelier
et ne se retouchent pas à la main — le README de la famille dit lequel.

## L'arborescence (`LOT-102`)

Le volume à venir — treize régions, une centaine de zones, des centaines de PNJ — impose que la
question *cet asset, où va-t-il ?* ait une réponse qui ne soit jamais « ça dépend ». Un asset vit
au **niveau le plus bas qui couvre tous ses usages**, du monde à la sous-zone :

```
Common/                                  ce qui existe partout
Regions/<région>/Common/                 l'identité d'une région
Regions/<région>/<ville>/Common/         ce que les quartiers d'une ville partagent
Regions/<région>/<ville>/<zone>/         ce qu'on ne voit que là
…/<zone>/<sous-zone>/                    un donjon : un lieu clos où l'on entre depuis la zone
```

Trois règles s'ensuivent : un asset **naît propre et monte par promotion** (jamais copié) ; le
moteur **résout une clé du plus propre au plus commun** (une zone peut donc remplacer une pièce
commune sous la même clé) ; **un dossier, un manifeste** — un fichier qu'aucun manifeste ne cite
fait échouer la CI. Le détail, les noms et le poids :
[l'arborescence](../../../Planning/standards/arborescence-assets.md).

**Les images ne sont pas suivies par Git** (`LOT-108`) : `Common/`, `Regions/`, `Maps/` et `UI/`
viennent d'archives publiées sur les releases du dépôt, dont `kits.lock.json` donne l'empreinte.
Après un clone ou un `git pull` qui change le verrou : `python scripts/fetch_assets.py`
(`setup_dev.ps1` et `build.ps1` le font d'eux-mêmes ; CMake refuse de configurer sans). Une
retouche d'image se **publie** — `python scripts/release/publish_asset_kit.py <chemin du kit>` —
et le nouveau verrou se commite ; l'image elle-même, jamais (`check_binary_files.py` la refuse).
Le détail : [le stockage](../../../Planning/standards/arborescence-assets.md#le-stockage).

Les **sources** ne sont pas versionnées : masters, planches de référence et sorties brutes du
générateur vivent dans `Tools/AssetsHD/` (ignoré par git), sous le même arbre. Le dépôt ne reçoit
que l'asset **installé** : détouré, réduit à l'échelle du standard, ancré, inscrit au manifeste.

## Contenu

- `Common/`, `Regions/` — l'arborescence ci-dessus : le kit de la Capitale (`LOT-105`, `LOT-129`,
  `LOT-108`), Arenarea (`LOT-108`), le héros de la démo (`LOT-112`).
- `Entities/` — familles des illustrations d'entité et leur contrat de dimensions
  (`families.json`, `core::loadAssetFamilies`) ; une donnée désigne son image par une **clé**, et
  un marqueur en tient lieu tant qu'aucune n'existe (voir `Entities/README.md`).
- `Fonts/` — polices TTF de l'interface et leurs licences (voir `Fonts/README.md`).
- `Maps/` — cartes peintes par l'auteur : le monde, les régions et les villes (`LOT-94`), décrites
  par `manifest.json`.
- `UI/` — illustrations de l'interface (`background/menu-scene.png`, le fond du menu), décrites par
  `illustrations.json`.

Le canevas de l'éditeur distingue les types de tuile par l'atlas **procédural** de
`hmi::TextureAtlas` (`EX-NFR-040`) : aucun fichier d'atlas n'est livré ni lu.

Tout asset livré ici doit paraître dans la galerie de débug (`--screen=AssetGallery`,
`EX-CNT-042`).

> Anciennement `Textures/` (réservé, jamais peuplé) — ce dossier est le point d'entrée réel des
> assets graphiques depuis le LOT-39, nommé `Assets/` pour suivre le même patron que `Levels/` et
> `Localization/`.
