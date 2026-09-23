# La consigne du générateur — 2D HD

Le style de la scène est **écrit**, pas laissé au générateur : c'est ce que dit
[`EX-VIS-008`](../../Documentation/Specification/vision.md). Cette page est cette écriture. Elle
remplace le bloc A de l'atelier des textures (`LOT-92`), qui décrivait du pixel art.

Une commande d'image se compose de **trois blocs**, toujours dans cet ordre, toujours en anglais —
c'est la langue où les générateurs sont les plus dociles :

| Bloc | Ce qu'il dit | Qui l'écrit |
|---|---|---|
| **A — le style** | la facture, la lumière, les matières, la palette, les interdits | figé ici, jamais réécrit à la main |
| **B — la vue** | la projection, l'échelle, l'emprise, le fond, le cadrage | figé ici, ses trous se remplissent |
| **C — la pièce** | l'objet demandé, ses matières, ses variantes | écrit à chaque commande |

La **planche de référence** `Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png` est jointe à
chaque envoi comme image de référence : elle vaut mieux que trois paragraphes de plus.

> **Le générateur reste un outil manuel.** Aucune génération ne tourne en CI, et Claude ne dessine
> pas : il écrit la commande, l'auteur l'envoie, la chaîne du [LOT-104](../versions/v0.1.0/v0.0.1-demo/lots/LOT-104-chaine-de-production-hd.md)
> installe le résultat. Règle du `LOT-91`, inchangée.

## Bloc A — le style

Pour chaque prochain lot, intégrer aussi les [critères de qualité validés par l’utilisateur](style-2d-hd.md#critères-de-qualité-validés-par-lutilisateur) : matières propres sans grain, relief lisible, volumes des balustres et buissons, grille exacte et continuité des modules jusque dans les angles. La référence acceptée est le LOT-105 V4 après sa reprise du relief. Ces contraintes complètent les blocs de commande ci-dessous et se vérifient dans une map rendue par le moteur.

Invariant pour tout l'Empire central. Une autre région change **la palette et les matières**, rien
d'autre.

```
STYLE (match the attached reference sheet): hand-painted 2D game art for an
isometric RPG. Painted, not pixel art: modelled flat colours, soft gradients,
crisp edges, no visible pixel grid, no dithering, no photographic texture, no
3D render look.

LINE: one thin dark outline in deep bronze (#5c4a2a) around the silhouette,
never pure black, never a uniform stroke — it thins where the light hits.

LIGHT: a single soft key light from the TOP-LEFT, cool ambient fill. Shading is
painted ON the object; the object casts NO shadow on the ground and carries no
ground plane of its own.

MATERIALS (measured on the reference sheet): ivory marble with subtle veining;
warm limestone ashlar in regular courses; noble paving with a geometric border;
patinated bronze, green-blue with gold edges; dark oiled timber; deep green
foliage.

PALETTE (Central Empire — keep to these, one accent per piece):
ivory #efe6d2, sand #d9c7a3, warm grey #9c948a, deep bronze #5c4a2a,
burgundy #8e2335 (the imperial accent), old gold #c9a45c, foliage #3f6b34,
still water #2f7f86. Contained saturation, nothing neon.

READABILITY: the piece will be seen at about 100 screen pixels per floor tile.
Marble veining, stone coursing and bronze patina must read at that size; finer
detail than that is lost, so do not spend it.

FORBIDDEN: any text, letters, numbers, labels, captions, user interface,
panels, borders, colour swatches, logo, watermark, grid lines, drop shadow,
ground plane, perspective vanishing point, multiple pieces in one image.
```

## Bloc B — la vue

Les accolades se remplissent depuis la commande. `{EMPRISE_L}` et `{EMPRISE_H}` sont l'emprise de la
pièce en cases ; le reste s'en déduit et figure au [standard](style-2d-hd.md).

```
VIEW: fixed isometric camera, orthographic — no perspective, no vanishing
point. The ground is a flat diamond {LOSANGE_L} pixels wide and {LOSANGE_H}
tall (ratio 0.62). The piece stands on a footprint of {EMPRISE_L} by
{EMPRISE_H} such diamonds; its faces run along the two diagonal directions of
that grid.

FRAMING: ONE piece, centred, alone, on a FULLY TRANSPARENT background. Nothing
under it, nothing around it, no shadow, no base plate, no cropping — the whole
piece is inside the image with a small margin. Draw it as large as the canvas
allows: the piece is reduced when it is installed, never enlarged.

OUTPUT: square canvas, at least 1024 px, PNG with alpha.
```

> **Pourquoi « dessine-la aussi grande que possible ».** Le standard veut un losange de 256 px ; les
> générateurs rendent une pièce dans un canevas carré où l'art flotte. Une pièce dessinée grande puis
> **réduite** est nette ; une pièce dessinée petite puis agrandie ne l'est jamais. C'est la règle
> « l'art est toujours réduit, jamais agrandi », et la maquette du `LOT-101` montre ce qu'il en coûte
> de l'enfreindre.

### Le cas d'une planche de sols

Les variantes d'une dalle de fond (standard, §4) se commandent **ensemble**, sur une planche : une
par envoi, trois dalles « du même pavé » sortent de trois teintes. C'est l'exception à « une pièce
par image », et la seule : l'installateur découpe la planche dans l'ordre de lecture et réduit
chaque morceau au losange exact du lieu. La commande remplace le paragraphe `FRAMING` du bloc B par
celui-ci, `{N}` étant le nombre de dalles :

```
FRAMING (floor sheet): {N} floor tiles in a single horizontal row, on a FULLY
TRANSPARENT background, with a clear transparent gap between two tiles. Each
tile is exactly ONE ground diamond seen from the isometric camera, perfectly
FLAT: no thickness, no side faces, no raised rim, no shadow. Draw the row as
large as the canvas allows.
```

La ligne `OUTPUT` devient celle d'une bande : `landscape canvas, as wide as available, PNG with alpha.`
Né du `LOT-105` : les sols du Colisée (`LOT-104`) étaient déjà livrés en planches de six.

### Les pièces d'un même kit

Une pièce qui doit s'accorder à une autre déjà produite — une fenêtre au mur plein, une haie
d'angle à la haie droite — la joint après la planche de référence, et son bloc C commence par :

```
REFERENCE: the image attached after the reference sheet shows another piece of
the SAME kit. Match its stone, its colours, its outline and its light exactly;
draw only the piece described below.
```

### Le cas d'une planche d'animation

Une animation se commande en **une seule fois**, en planche : demander les images une par une, c'est
garantir que le personnage dérive. La commande remplace alors les deux premières phrases du cadrage
ci-dessus par celles-ci, et rien d'autre du bloc B ne bouge :

```
FRAMING (animation strip): ONE character, drawn once per frame, in a single
horizontal strip on a FULLY TRANSPARENT background. Same figure, same colours,
same proportions and same light in every frame; only the pose changes. Each
frame is the same size, evenly spaced, with 8 px of margin between frames and
around the strip. No shadow, no base plate, no separating line, no frame
number.
```

La marge de 8 px est celle du [standard](style-2d-hd.md) : sans elle, le niveau de mipmap d'une
image déborde sur sa voisine.

L'essai de cadence du [LOT-112](../versions/v0.1.0/v0.0.1-demo/lots/LOT-112-heros-de-la-demo.md) a
ajouté deux choses à cette variante. La phrase « Draw it as large as the canvas allows » se **garde**
à la fin du cadrage : c'est la règle « réduit, jamais agrandi ». Et la ligne `OUTPUT` du bloc B, qui
demande un canevas carré, devient :

```
OUTPUT: landscape canvas, as wide as available, PNG with alpha.
```

Une image de la bande doit sortir à 192 px de large au moins (384 pour l'attaque et le sort) : huit
images demandent un canevas de 1600 px. Le standard fixe **huit images** par animation.

### Les envois qui suivent le premier

Une figurine se commande en **plusieurs** envois — une bande par animation et par orientation — et
chacun redessine le personnage. Le premier envoi fixe le personnage ; tous les suivants joignent sa
bande et commencent leur bloc C par :

```
REFERENCE: the attached strip shows THIS character — same face, same hair,
same colours, same equipment, same proportions. Draw him again, only the pose
and the direction change.
```

Une bande d'une autre orientation joint en plus la même animation vue du sud-est. La commande du
héros (`Tools/AssetsHD/Common/Characters/Heroes/brawler/commande.md`) est l'exemple complet, et
`scripts/prepare_envois_figure.py` en tire les envois prêts à copier.

### Le cas d'un portrait

Le portrait (512 × 512, standard §5) garde le bloc A et remplace le bloc B entier :

```
FRAMING (portrait): ONE character, head and shoulders, three-quarter view
facing the lower left, centred in a square canvas. A plain, softly lit
background in dark bronze (#5c4a2a) fading to warm grey (#9c948a) — no scenery,
no border, no text. The face and the upper chest fill most of the canvas.

OUTPUT: square canvas, at least 1024 px, PNG.
```

Il se commande **avant** les bandes : c'est lui que joint le premier envoi de bande.

## Bloc C — la pièce

Un gabarit, à remplir pour chaque commande. Une pièce par envoi.

```
PIECE: {objet, en une phrase : ce que c'est, en quoi c'est fait, ce qu'il porte}.
FAMILY: {une des dix familles du standard}.
PLACE: {le lieu, et sa couleur d'accent}.
VARIANTS: {ce qui doit changer d'une variante à l'autre, ou "none"}.
```

**Exemple**, la fontaine d'Arenarea :

```
PIECE: a three-tiered public fountain in ivory marble, octagonal basin with a
carved frieze, still water, four low planters of white flowers at its corners,
a thin band of patinated bronze at each tier.
FAMILY: 06 — centrepiece, footprint 3 by 3.
PLACE: Arenarea, the arena quarter of the Capital; accent burgundy.
VARIANTS: none.
```

## Ce que la maquette a appris à la consigne

Trois règles ci-dessus ne viennent pas de la planche mais du montage de la maquette
([LOT-101](../versions/v0.1.0/v0.0.1-demo/lots/LOT-101-standard-2d-hd.md)) :

1. **« une pièce par image »** — la planche livre ses pièces par panneaux de trois ou quatre ; les
   séparer ensuite coûte un détourage par pièce et laisse des franges.
2. **« pas de plaque de sol sous la pièce »** — les sols de la planche sont des dalles **avec leur
   épaisseur** : posées côte à côte, elles se marchent dessus.
3. **« dessine-la aussi grande que possible »** — les pièces de la planche tiennent dans un losange
   de 136 px, soit la moitié de ce que le standard demande.
