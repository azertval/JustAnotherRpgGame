# Commande — le kit commun de la Capitale

Lot : [LOT-105](../../../../../../Planning/versions/v0.1.0/v0.0.1-demo/lots/LOT-105-kit-commun-de-la-capitale.md).
Lieux : `Regions/central-empire/Common/` (le kit impérial) et `Regions/central-empire/capital/Common/`
(le kit de la Capitale). Accent : bourgogne `#8e2335`.
Direction artistique : [le référentiel de la Capitale](../../../../../../Planning/referentiels/central-empire/capitale.md) ;
le livre dit « propreté méticuleuse, rues pavées, lanternes » (SB p. 96-99).

Ce kit est la première application de la règle [du commun vers le propre](../../../../../../Planning/standards/arborescence-assets.md) :
Martpart, Arenarea et l'Arena of Fate le prennent tel quel et ne produisent que ce qui leur est
propre. Une pièce de ce kit ne porte donc **rien qui nomme un quartier** : ni enseigne, ni
marchandise, ni couleur de faction.

## Inventaire

| # | Famille | Du commun | Propre au kit | État |
|---|---|---|---|---|
| 01 | Sols | | pavé de rue ×3 (dalle de fond), dallage de place ×3, bordures de trottoir ×4 | commandé |
| 02 | Façades | | mur plein et mur à fenêtre (deux sens), angle rentrant, angle sortant ; mur à bannière (deux sens, kit impérial) | commandé |
| 03 | Colonnes | | colonne à l'emblème du lion (kit impérial) | commandé |
| 04 | Accès | | *néant : une porte dit la maison qu'elle ouvre — propre à chaque zone* | écarté |
| 05 | Balustrades | | balustrade droite (deux sens), pilier | commandé |
| 06 | Pièces maîtresses | | *néant : une fontaine, une statue font l'identité d'une zone (la fontaine d'Arenarea)* | écarté |
| 07 | Végétal | | cyprès, haie droite (deux sens), haie d'angle, massif fleuri | commandé |
| 08 | Mobilier | | lampadaire, banc (deux sens), vasque, tonneau, caisse, étal nu ; bannière au lion sur mât (kit impérial) | commandé |
| 09 | Bâtiments | | *néant : un bâtiment est toujours celui d'une zone* | écarté |
| 10 | Seuils | | *néant : parvis et escaliers se dessinent à la mesure d'un lieu* | écarté |

**Deux sens.** Une pièce allongée se commande dans les deux directions de la grille : **U** court le
long des colonnes (dans l'image, du haut-gauche vers le bas-droite), **V** le long des lignes (du
haut-droite vers le bas-gauche). Le moteur ne retourne pas les images : la lumière vient du
haut-gauche, et un miroir la ferait venir de la droite.

**Les « matières impériales »** du lot ne sont pas des pièces : ce sont la palette et les matières de
la région, écrites dans `region.json` (et déjà dans le bloc A de la consigne).

**Les bannières.** Le lion d'or sur pourpre est **redessiné** : l'image du livre n'est jamais jointe
au générateur, ni décalquée (standard 2D HD, §6). Le dossier `Tools/AssetsHD/Bannieres/`, extrait
du livre, n'est pas une source.

## L'ordre des envois

`python scripts/prepare_envois_scene.py Tools/AssetsHD/Regions/central-empire/capital/Common/commande.md`
écrit un dossier par envoi dans `envois/` : le texte assemblé (blocs A, B et C), la planche de
référence, les références déjà produites, et le nom sous lequel enregistrer la sortie.

Deux lignes des blocs C ci-dessous sont des **marqueurs** que le script remplace par le texte de
[la consigne](../../../../../../Planning/standards/consigne-2d-hd.md) : `FRAMING: <N>` par le cadrage
d'une planche de N sols, `REFERENCE` par la phrase des pièces d'un même kit. Le reste se colle tel
quel, après les blocs A et B.

L'ordre compte : la **première** pièce d'une série fixe la pierre, le bois ou le feuillage, et les
suivantes la **joignent** (ligne « Référence » de chaque pièce). Commencer par `floor-paving`,
`wall-limestone-u`, `plant-cypress`, `prop-barrel`, `prop-banner-lion`.

Chaque sortie s'enregistre sous le chemin de sa ligne « Source », relatif à ce dossier. Une pièce qui
ne va pas se **recommande** entière : on corrige la commande, jamais l'image.

## Commandes

### 01 — Sols

#### floor-paving

```
FRAMING: THREE
PIECE: street paving of the Imperial Capital, the background floor of every
street: small squared setts of warm grey and sand limestone laid in straight
courses, tight neutral joints, clean and well kept, very slightly worn. No
border, no pattern, no kerb, nothing that marks the edge of the tile.
FAMILY: 01 — floor, background tile, repeated over hundreds of tiles.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: three tiles of the SAME paving; only the arrangement of the setts
changes. The edges of every tile must match any other tile laid next to it:
seamless, no line along the border.
```

Pièces : `floor-paving-01`, `floor-paving-02`, `floor-paving-03`. Emprise : 1 × 1. Type : open. Source : `Sols/floor-paving.png`. État : commandé.

#### floor-flagstone

```
FRAMING: THREE
REFERENCE
PIECE: large square flagstones of pale sand limestone for a public square,
laid on the grid, fine joints, polished by footsteps, a few faint warm-grey
veins. No border, no pattern, no kerb.
FAMILY: 01 — floor, background tile for squares and forecourts.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: three tiles of the SAME flagstones; only the veining and the wear
change. Seamless edges, as for a background tile.
```

Pièces : `floor-flagstone-01`, `floor-flagstone-02`, `floor-flagstone-03`. Emprise : 1 × 1. Type : open. Source : `Sols/floor-flagstone.png`. Référence : `floor-paving`. État : commandé.

#### floor-paving-edge

```
FRAMING: FOUR
REFERENCE
PIECE: the street paving of the attached reference, bordered on ONE side by a
flush kerb: a band of long ivory limestone kerbstones, level with the paving
(a flat drawing, no step, no height), about a sixth of the tile wide.
FAMILY: 01 — floor, street border.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: four tiles, left to right, with the kerb along a different side of
the diamond: first along the upper-left side, then the upper-right side, then
the lower-right side, then the lower-left side. The three other sides are
seamless paving.
```

Pièces : `floor-paving-edge-nw`, `floor-paving-edge-ne`, `floor-paving-edge-se`, `floor-paving-edge-sw`. Emprise : 1 × 1. Type : open. Source : `Sols/floor-paving-edge.png`. Référence : `floor-paving`. État : commandé.

### 02 — Façades

#### wall-limestone-u

```
PIECE: a plain section of a townhouse ground floor in warm limestone ashlar,
regular courses, a moulded plinth at the foot and a simple cornice at the top,
about three tiles tall. Clean, well kept stone; nothing hangs on it.
FAMILY: 02 — facade, a straight wall running from the upper left to the lower
right of the image, seen from its lit face.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `wall-limestone-u`. Emprise : 2 × 1. Type : solid. Source : `Facades/wall-limestone-u.png`. État : commandé.

#### wall-limestone-v

```
REFERENCE
PIECE: the same plain limestone wall section as the attached piece — same
courses, plinth and cornice, same height.
FAMILY: 02 — facade, a straight wall running from the upper right to the lower
left of the image, seen from its face in half-shade.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `wall-limestone-v`. Emprise : 1 × 2. Type : solid. Source : `Facades/wall-limestone-v.png`. Référence : `wall-limestone-u`. État : commandé.

#### wall-limestone-window-u

```
REFERENCE
PIECE: the same limestone wall section as the attached piece, pierced by one
tall window with an ivory stone frame, dark glass behind a thin bronze grille,
and a small flower box of red geraniums on the sill.
FAMILY: 02 — facade, a straight wall running from the upper left to the lower
right of the image.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `wall-limestone-window-u`. Emprise : 2 × 1. Type : solid. Source : `Facades/wall-limestone-window-u.png`. Référence : `wall-limestone-u`. État : commandé.

#### wall-limestone-window-v

```
REFERENCE
PIECE: the same limestone wall section as the attached piece, pierced by one
tall window with an ivory stone frame, dark glass behind a thin bronze grille,
and a small flower box of red geraniums on the sill.
FAMILY: 02 — facade, a straight wall running from the upper right to the lower
left of the image.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `wall-limestone-window-v`. Emprise : 1 × 2. Type : solid. Source : `Facades/wall-limestone-window-v.png`. Référence : `wall-limestone-u`. État : commandé.

#### wall-limestone-corner-inner

```
REFERENCE
PIECE: an inner corner of the same limestone wall: two short arms meeting at
the back, one running to the lower left, one to the lower right, forming a V
that opens toward the viewer. Same courses, plinth, cornice and height.
FAMILY: 02 — facade, inner corner.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `wall-limestone-corner-inner`. Emprise : 1 × 1. Type : solid. Source : `Facades/wall-limestone-corner-inner.png`. Référence : `wall-limestone-u`. État : commandé.

#### wall-limestone-corner-outer

```
REFERENCE
PIECE: an outer corner of the same limestone wall: the edge of a building
pointing toward the viewer, both faces visible, the left one lit, the right
one in half-shade, with quoins of larger ivory stones along the edge. Same
courses, plinth, cornice and height.
FAMILY: 02 — facade, outer corner.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `wall-limestone-corner-outer`. Emprise : 1 × 1. Type : solid. Source : `Facades/wall-limestone-corner-outer.png`. Référence : `wall-limestone-u`. État : commandé.

### 05 — Balustrades

#### balustrade-limestone-u

```
REFERENCE
PIECE: a waist-high balustrade in the same limestone: a moulded base, a row of
turned ivory balusters, a flat handrail on top.
FAMILY: 05 — balustrade, straight, running from the upper left to the lower
right of the image.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `balustrade-limestone-u`. Emprise : 2 × 1. Type : cover. Source : `Balustrades/balustrade-limestone-u.png`. Référence : `wall-limestone-u`. État : commandé.

#### balustrade-limestone-v

```
REFERENCE
PIECE: the same waist-high limestone balustrade as the attached piece.
FAMILY: 05 — balustrade, straight, running from the upper right to the lower
left of the image.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `balustrade-limestone-v`. Emprise : 1 × 2. Type : cover. Source : `Balustrades/balustrade-limestone-v.png`. Référence : `balustrade-limestone-u`. État : commandé.

#### balustrade-limestone-pillar

```
REFERENCE
PIECE: a square limestone pillar that ends a balustrade run or turns its
corner: slightly taller than the handrail of the attached balustrade, a
moulded cap topped with a small stone ball.
FAMILY: 05 — balustrade pillar.
PLACE: the Capital of the Central Empire, every quarter; accent burgundy.
VARIANTS: none.
```

Pièces : `balustrade-limestone-pillar`. Emprise : 1 × 1. Type : obstacle. Source : `Balustrades/balustrade-limestone-pillar.png`. Référence : `balustrade-limestone-u`. État : commandé.

### 07 — Végétal

#### plant-cypress

```
PIECE: a tall, slender Italian cypress, deep green foliage in dense vertical
tufts, a short dark trunk at the foot, planted straight in the ground; about
three tiles tall.
FAMILY: 07 — vegetation.
PLACE: the Capital of the Central Empire, gardens and avenues; accent burgundy.
VARIANTS: none.
```

Pièces : `plant-cypress`. Emprise : 1 × 1. Type : obstacle. Source : `Vegetal/plant-cypress.png`. État : commandé.

#### plant-hedge-u

```
REFERENCE
PIECE: a clipped box hedge, waist high, flat top and straight sides, the same
deep green as the attached cypress, neatly trimmed.
FAMILY: 07 — vegetation, straight hedge running from the upper left to the
lower right of the image.
PLACE: the Capital of the Central Empire, gardens and avenues; accent burgundy.
VARIANTS: none.
```

Pièces : `plant-hedge-u`. Emprise : 2 × 1. Type : cover. Source : `Vegetal/plant-hedge-u.png`. Référence : `plant-cypress`. État : commandé.

#### plant-hedge-v

```
REFERENCE
PIECE: the same clipped box hedge as the attached piece, same height.
FAMILY: 07 — vegetation, straight hedge running from the upper right to the
lower left of the image.
PLACE: the Capital of the Central Empire, gardens and avenues; accent burgundy.
VARIANTS: none.
```

Pièces : `plant-hedge-v`. Emprise : 1 × 2. Type : cover. Source : `Vegetal/plant-hedge-v.png`. Référence : `plant-hedge-u`. État : commandé.

#### plant-hedge-corner

```
REFERENCE
PIECE: the corner of the same clipped box hedge: two short arms meeting at the
back, one running to the lower left, one to the lower right, same height.
FAMILY: 07 — vegetation, hedge corner.
PLACE: the Capital of the Central Empire, gardens and avenues; accent burgundy.
VARIANTS: none.
```

Pièces : `plant-hedge-corner`. Emprise : 1 × 1. Type : cover. Source : `Vegetal/plant-hedge-corner.png`. Référence : `plant-hedge-u`. État : commandé.

#### plant-flowerbed

```
REFERENCE
PIECE: a low flowerbed edged with ivory limestone, filled with dark soil and
dense clumps of burgundy and white flowers among green leaves, knee high.
FAMILY: 07 — vegetation, flowerbed.
PLACE: the Capital of the Central Empire, gardens and avenues; accent burgundy.
VARIANTS: none.
```

Pièces : `plant-flowerbed`. Emprise : 1 × 1. Type : difficult. Source : `Vegetal/plant-flowerbed.png`. Référence : `plant-cypress`. État : commandé.

### 08 — Mobilier

#### prop-lamppost

```
PIECE: a street lamppost of patinated bronze, green-blue with old gold edges:
a slender fluted post on a square foot, a curled bracket at the top holding a
glazed lantern with a warm, softly glowing flame; about two and a half tiles
tall.
FAMILY: 08 — street furniture.
PLACE: the Capital of the Central Empire, every street; accent burgundy.
VARIANTS: none.
```

Pièces : `prop-lamppost`. Emprise : 1 × 1. Type : obstacle. Source : `Mobilier/prop-lamppost.png`. État : commandé.

#### prop-bench-u

```
REFERENCE
PIECE: a public bench: a seat and a back of dark oiled timber slats on two cast
bronze feet in the same patina as the attached lamppost.
FAMILY: 08 — street furniture; the bench runs from the upper left to the lower
right of the image, its seat facing the viewer.
PLACE: the Capital of the Central Empire, every street; accent burgundy.
VARIANTS: none.
```

Pièces : `prop-bench-u`. Emprise : 1 × 1. Type : cover. Source : `Mobilier/prop-bench-u.png`. Référence : `prop-lamppost`. État : commandé.

#### prop-bench-v

```
REFERENCE
PIECE: the same public bench as the attached piece.
FAMILY: 08 — street furniture; the bench runs from the upper right to the lower
left of the image, its seat facing the viewer.
PLACE: the Capital of the Central Empire, every street; accent burgundy.
VARIANTS: none.
```

Pièces : `prop-bench-v`. Emprise : 1 × 1. Type : cover. Source : `Mobilier/prop-bench-v.png`. Référence : `prop-bench-u`. État : commandé.

#### prop-planter

```
PIECE: a large round planter urn in ivory marble on a short square pedestal, a
carved garland around its belly, planted with a small clipped box ball of deep
green; knee to waist high.
FAMILY: 08 — street furniture.
PLACE: the Capital of the Central Empire, every street; accent burgundy.
VARIANTS: none.
```

Pièces : `prop-planter`. Emprise : 1 × 1. Type : cover. Source : `Mobilier/prop-planter.png`. État : commandé.

#### prop-barrel

```
PIECE: a wooden barrel standing upright, dark oiled staves, three iron hoops
darkened to bronze, a closed lid; waist high.
FAMILY: 08 — street furniture, goods.
PLACE: the Capital of the Central Empire, every street; accent burgundy.
VARIANTS: none.
```

Pièces : `prop-barrel`. Emprise : 1 × 1. Type : cover. Source : `Mobilier/prop-barrel.png`. État : commandé.

#### prop-crate

```
REFERENCE
PIECE: a closed wooden crate of the same dark oiled timber as the attached
barrel, planks and corner battens, iron nails; knee high. No marking.
FAMILY: 08 — street furniture, goods.
PLACE: the Capital of the Central Empire, every street; accent burgundy.
VARIANTS: none.
```

Pièces : `prop-crate`. Emprise : 1 × 1. Type : cover. Source : `Mobilier/prop-crate.png`. Référence : `prop-barrel`. État : commandé.

#### prop-stall-empty

```
REFERENCE
PIECE: an empty market stall of the same dark oiled timber as the attached
crate: a waist-high wooden counter, four corner posts, a sloping canvas awning
in plain burgundy. Nothing on the counter, no sign, no goods.
FAMILY: 08 — street furniture, market stall.
PLACE: the Capital of the Central Empire, every market; accent burgundy.
VARIANTS: none.
```

Pièces : `prop-stall-empty`. Emprise : 1 × 1. Type : obstacle. Source : `Mobilier/prop-stall-empty.png`. Référence : `prop-crate`. État : commandé.

### Kit impérial — `Regions/central-empire/Common/`

Ces quatre pièces vont au commun de **la région** : le lion est celui de l'Empire, pas celui de la
Capitale. Leurs sources vivent dans `Tools/AssetsHD/Regions/central-empire/Common/`, avec leur
descripteur.

#### prop-banner-lion

```
PIECE: an imperial banner on a free-standing pole: a tall dark oiled timber
pole on a bronze foot, a crossbar with old gold finials, a long hanging banner
of deep burgundy with a golden lion rampant embroidered at its centre and a
gold fringe at the bottom; about three tiles tall. The lion is a simple, bold
heraldic silhouette.
FAMILY: 08 — street furniture, banner.
PLACE: the Central Empire, every imperial town; accent burgundy.
VARIANTS: none.
```

Pièces : `prop-banner-lion`. Emprise : 1 × 1. Type : obstacle. Source : `../../Common/Bannieres/prop-banner-lion.png`. État : commandé.

#### wall-limestone-banner-u

```
REFERENCE
PIECE: the limestone wall section of the first attached piece, with the
imperial banner of the second attached piece hanging flat against its face
from a bronze rod under the cornice: burgundy cloth, golden lion rampant,
gold fringe.
FAMILY: 02 — facade, a straight wall running from the upper left to the lower
right of the image.
PLACE: the Central Empire, every imperial town; accent burgundy.
VARIANTS: none.
```

Pièces : `wall-limestone-banner-u`. Emprise : 2 × 1. Type : solid. Source : `../../Common/Bannieres/wall-limestone-banner-u.png`. Référence : `wall-limestone-u`, `prop-banner-lion`. État : commandé.

#### wall-limestone-banner-v

```
REFERENCE
PIECE: the same wall with its hanging imperial banner as the attached piece.
FAMILY: 02 — facade, a straight wall running from the upper right to the lower
left of the image.
PLACE: the Central Empire, every imperial town; accent burgundy.
VARIANTS: none.
```

Pièces : `wall-limestone-banner-v`. Emprise : 1 × 2. Type : solid. Source : `../../Common/Bannieres/wall-limestone-banner-v.png`. Référence : `wall-limestone-banner-u`. État : commandé.

#### column-lion-emblem

```
REFERENCE
PIECE: a square ivory limestone column on a stepped base, about two tiles
tall, crowned by a seated lion in old gold, and on its front face a round
bronze medallion bearing the same lion rampant as the attached banner.
FAMILY: 03 — column, imperial emblem.
PLACE: the Central Empire, every imperial town; accent burgundy.
VARIANTS: none.
```

Pièces : `column-lion-emblem`. Emprise : 1 × 1. Type : obstacle. Source : `../../Common/Bannieres/column-lion-emblem.png`. Référence : `prop-banner-lion`. État : commandé.

## Budget

À mesurer à l'installation (résumé du job `hd_assets`) : 40 Mio par niveau, le kit de la Capitale
et le kit impérial comptés à part.
