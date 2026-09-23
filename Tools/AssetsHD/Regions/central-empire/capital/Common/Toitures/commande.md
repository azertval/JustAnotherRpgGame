# Commande — la toiture de la Capitale

Lot : [LOT-129](../../../../../../../Planning/versions/v0.1.0/v0.0.1-demo/lots/LOT-129-etages-et-toits.md).
Lieu : `Regions/central-empire/capital/Common/` (le kit de la Capitale, `LOT-105`). Accent :
bourgogne `#8e2335`. Direction artistique : [le référentiel de la Capitale](../../../../../../../Planning/referentiels/central-empire/capitale.md).

## Ce que l'auteur a tranché (23 septembre 2026)

- **Toits à deux pans, pignons de pierre**, « style romain » : tuiles creuses et plates (*tegula* et
  *imbrex*) de terre cuite bourgogne, pente douce, pignon de pierre calcaire au bout visible.
- **Profondeur libre** : un îlot se couvre quelle que soit sa profondeur ; le faîtage se calcule.

## Pourquoi quatre images, et pas quatre-vingts

La toiture est **modulaire**, comme les murs du kit : elle se pose case par case sur la couche
d'étage au-dessus des murs (`LOT-129`), et deux cases voisines doivent se raccorder au tuile près,
jusque dans les pignons. Aucun générateur ne tient cela sur des pièces dessinées une à une. La
méthode est celle de la V4 du `LOT-105`, écrite dans [la consigne](../../../../../../../Planning/standards/consigne-2d-hd.md#le-cas-dune-matière--surface-et-élévation) :
le générateur peint **la matière à plat**, et `scripts/build_capital_roofs.py` la projette sur la
géométrie exacte de chaque toit.

Le générateur peint donc **quatre matières** ; la pierre du pignon est celle des murs de la V4
(`../V4/Sources/wall-surface.png`), reprise telle quelle pour que le pignon prolonge le mur au joint
près. Le script en tire les pièces :

| Pièce | Ce que c'est |
|---|---|
| `roof-u-d<D>-r<r>` | le toit d'un îlot de `D` rangées dont le faîtage court le long des colonnes (U), sur une case de la rangée `r` (0 au nord), au milieu du toit |
| `…-start` | la même, sur la première colonne : la rive du bout caché |
| `…-gable` | la même, sur la dernière colonne : le pignon de pierre, visible |
| `…-single` | un îlot d'une seule colonne : rive et pignon |
| `roof-v-d<D>-c<c>…` | les mêmes, faîtage le long des rangées (V) |

`D` va de 2 à 5 : un îlot plus profond se couvre de deux toits. Soit **112 pièces** (deux sens,
14 rangées, quatre positions), en `open` : un étage ne compte pas dans la collision. Le toit part du
bord des cases au nord et à l'ouest — là où les murs droits du kit s'arrêtent — et déborde du nu des
façades vues au sud et à l'est.

## La géométrie, validée avant les matières

`python scripts/build_capital_roofs.py --provisional` projette des matières de remplacement, et
`python scripts/validate_capital_roofs.py --scale 2` rend par le moteur deux îlots de murs du kit sur
deux étages, coiffés de leur toit, faîtage dans les deux sens (`apercus/roofs.png`). Validé le
23 septembre 2026 : pièces jointives, pignons, égouts, ordre de tracé. Il ne reste que les matières.

## L'ordre des envois

`python scripts/prepare_envois_scene.py Tools/AssetsHD/Regions/central-empire/capital/Common/Toitures/commande.md`

Les tuiles d'abord : les trois bandes suivantes les joignent. Chaque sortie s'enregistre sous le
chemin de sa ligne « Source ». Une matière qui ne va pas se **recommande** entière.

## Commandes

#### roof-tiles

```
FRAMING: SURFACE
REFERENCE
PIECE: a Roman terracotta roof seen straight from above, laid in regular
alternating rows: flat tegula tiles with raised edges, the joints between two
tegulae covered by a half-round imbrex tile; the imbrex lines run from the TOP
edge to the BOTTOM edge of the image (the slope goes down the image). Exactly
FOUR imbrex lines across the width, evenly spaced, and exactly SIX tile courses
from top to bottom, each course overlapping the one below with a thin soft
shadow line. Burgundy terracotta #8e2335, a little warmer and lighter on the
imbrex crowns, darker in the channels; clean, well kept, no moss, no broken
tile. Each imbrex is modelled: lit on its left flank, shaded on its right.
FAMILY: 09 — building, roof covering material.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `roof-tiles`. Emprise : 1 × 1. Type : open. Source : `Sources/roof-tiles.png`. Référence : `../V4/Sources/wall-surface.png`. État : commandé.

#### roof-ridge

```
FRAMING: ELEVATION
REFERENCE
PIECE: the ridge of the same Roman roof seen from the side: a continuous row of
large half-round ridge tiles laid end to end, each overlapping the next, in the
same burgundy terracotta, bedded on a thin line of pale mortar. The band is
about four times wider than tall; only the ridge tiles, nothing below them.
FAMILY: 09 — building, roof ridge.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `roof-ridge`. Emprise : 1 × 1. Type : open. Source : `Sources/roof-ridge.png`. Référence : `roof-tiles`. État : commandé.

#### roof-eave

```
FRAMING: ELEVATION
REFERENCE
PIECE: the lower edge of the same Roman roof, seen from the front: the rounded
ends of the imbrex tiles in a regular row, each closed by a small ivory
antefix shaped like a palmette, the flat tegula ends between them, all resting
on a slim moulded cornice of ivory limestone. The band is about six times wider
than tall.
FAMILY: 09 — building, roof eave.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `roof-eave`. Emprise : 1 × 1. Type : open. Source : `Sources/roof-eave.png`. Référence : `roof-tiles`, `../V4/Sources/wall-surface.png`. État : commandé.

#### gable-cornice

```
FRAMING: ELEVATION
REFERENCE
PIECE: the raking cornice that edges a stone gable under the roof verge: a
simple moulded band of ivory limestone, a flat fascia, a small cyma and a
narrow drip, with the thin burgundy line of the verge tiles resting on top.
Same limestone as the attached wall. The band is about eight times wider than
tall.
FAMILY: 02 — facade, gable cornice.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `gable-cornice`. Emprise : 1 × 1. Type : open. Source : `Sources/gable-cornice.png`. Référence : `../V4/Sources/wall-surface.png`, `roof-tiles`. État : commandé.

## Budget

À mesurer à l'installation, avec le kit de la Capitale (≈ 2,6 Mio avant la toiture) : sous 40 Mio.
