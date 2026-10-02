# Gabarit de la commande d'une zone

Un lot d'assets de zone commence par **sa commande** : une page qui passe en revue les dix familles
du [standard](style-3d.md#6-les-familles-de-pièces-dun-lieu), dit pour chacune ce qui vient du
**commun** et ce que la zone produit en **propre** (voir l'[arborescence](arborescence-assets.md)),
et suit chaque pièce de la commande à l'asset installé. Livrée par le
[LOT-104](../versions/v0.1.0/v0.0.1-demo/lots/LOT-104-chaine-de-production-hd.md).

> **Depuis le `LOT-1001`, une pièce est un maillage ou une image.** L'inventaire ci-dessous vaut
> pour les deux. Le chemin et le descripteur décrits ensuite sont ceux d'une **image tolérée**
> ([standard 3D, §7](style-3d.md#7-les-images-tolérées)) — mobilier, pièces maîtresses, kits non
> encore repris. Le chemin d'un **maillage** de décor (sa source, son script, son entrée `"mesh"`)
> s'écrit au [LOT-1004](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1004-kit-de-la-capitale-en-maillages.md),
> sur le kit de la Capitale : cette page le recevra alors. Les lots de zone de la `0.0.3` se
> réécrivent à la recette de la `0.0.2.5` ([D-35](../vision/decisions.md)).

## Où elle vit

À côté des sources qu'elle commande, **sous le même arbre** que la zone :
`Tools/AssetsHD/Regions/<région>/<ville>/<zone>/commande.md`, avec le descripteur d'installation
`install.json` du même dossier. Comme tout `Tools/`, ils restent **locaux** : rien n'y est livré
(décision de l'auteur, 23 septembre 2026, qui retire l'exception du LOT-104). Le dossier
`Tools/AssetsHD/Colisee/`, antérieur à la règle, garde son nom : son descripteur dit où il installe.

## Le chemin d'une pièce en image

| Étape | Qui | Ce qui en sort |
|---|---|---|
| 1. **Commander** | Claude écrit le bloc C, avec les blocs A et B de [la consigne archivée](archives/consigne-2d-hd.md), figés | la commande, dans la page |
| 2. **Générer** | l'auteur l'envoie au générateur, avec la planche de référence | la source, dans le dossier de la zone |
| 3. **Décrire** | Claude ajoute la pièce au descripteur : nom, famille, emprise, type tactique | une entrée d'`install.json` |
| 4. **Installer** | `python scripts/assetsGeneration/install_hd_asset.py <dossier>/install.json` | l'image et son entrée de manifeste, dans `Source/Elements/Assets/…/Scene/` |
| 5. **Voir** | la galerie de débug, `--screen=AssetGallery` | la pièce à l'échelle, dans son emprise |

La commande ne dessine pas et ne retouche rien : une pièce qui ne s'installe pas se **recommande**
(règle « un asset trop petit se refait »), ou se corrige dans le descripteur (`scale`,
`anchorOffset`, `align`) quand c'est la mesure qui se trompe, pas le dessin.

## Le descripteur

```json
{
  "version": 1,
  "target": "Regions/central-empire/capital/arenarea/arena-of-fate/Scene",
  "pieces": [
    {"source": "Sols/sable.png", "family": "01", "tactical": "open",
     "sheet": ["floor-sand-01", "floor-sand-02", "floor-sand-03"]},
    {"source": "Murs/mur-U.png", "name": "wall-arcade-u", "family": "02",
     "footprint": [3, 1], "align": "north"}
  ]
}
```

| Champ | Ce qu'il dit |
|---|---|
| `source` | l'image, relative au dossier du descripteur |
| `name` / `sheet` | le nom de la pièce (`<famille>-<objet>[-<variante>]`) ; une **planche** nomme ses morceaux dans l'ordre de lecture, et un compte faux est une erreur |
| `family` | `01` à `10` ; `01` est un sol, réduit au losange exact du lieu |
| `footprint` | l'emprise en cases, colonnes puis rangées (`[3, 1]` : un mur le long des colonnes) |
| `tactical` | `open`, `difficult`, `cover`, `obstacle`, `solid` ; à défaut, un sol passe et une pièce debout arrête la vue |
| `align` | `centre` (défaut) ou `north` : sur l'axe qu'il ne remplit pas, le socle est centré dans son emprise ou collé à son bord nord — celui d'un mur qui doit rejoindre un angle rentrant |
| `scale`, `anchorOffset` | corrections, quand la pièce ne touche pas ses pointes (un lampadaire, une statue au bras tendu) |
| `folders` (au niveau du descripteur) | le rangement en sous-dossiers : `[{"match": "^roof-l-d(\d)", "folder": "roofs/l/d\1"}]`, la première règle dont le motif prend le nom de la pièce donne son dossier sous `target` (`LOT-129`) |

## La page

Recopier ce qui suit dans `commande.md`, puis le remplir.

````markdown
# Commande — <zone>

Lot : LOT-NNN. Lieu : `Regions/<région>/<ville>/<zone>/`. Accent : <couleur>.
Direction artistique : <lien vers le référentiel>.

## Inventaire

| # | Famille | Forme | Du commun | Propre | État |
|---|---|---|---|---|---|
| 01 | Sols | maillage | `capital/Common` : pavage de fond | sable ×3 (dalle de fond), bordures | commandé |
| 02 | Façades | | | | |
| 03 | Colonnes | | | | |
| 04 | Accès | | | | |
| 05 | Balustrades | maillage | | | |
| 06 | Pièces maîtresses | image tolérée | | | |
| 07 | Végétal | | | *néant : écarté, et pourquoi* | écarté |
| 08 | Mobilier | image tolérée | | | |
| 09 | Bâtiments | | | | |
| 10 | Seuils | | | | |

Une famille ne reste jamais vide : elle est remplie, prise au commun, ou **écartée avec sa
raison**. La colonne « Forme » dit `maillage` ou `image tolérée` ; une pièce d'architecture en
image est une dette, nommée avec le lot qui la retire. La famille 01 livre d'abord sa dalle de fond répétable, en trois variantes au moins.

## Commandes

### <nom-de-la-pièce>

```
PIECE: …
FAMILY: …
PLACE: …
VARIANTS: …
```

Emprise : C × R. Type tactique : … Source : `<fichier>` (reçue le …). Installée : oui / non.

## Poids

<poids de la zone, tel que le résumé du job CI l'affiche> — pour mémoire : une zone n'a pas de budget (D-23).

## Publication

Dernière étape de la zone, après la revue de l'auteur : `python scripts/release/publish_asset_kit.py
Regions/<région>/<ville>/<zone>` publie le kit et met à jour `kits.lock.json` ; les images ne se
commitent pas (`Planning/standards/arborescence-assets.md`, « Le stockage »).
````

## Les états d'une pièce

`commandé` (bloc C écrit) → `reçu` (source dans le dossier) → `installé` (dans le manifeste, vue dans
la galerie) ; ou `écarté`, avec sa raison. Une pièce installée qui double une pièce du commun est
une faute : elle se **promeut** ou se retire (arborescence, règle 1).
