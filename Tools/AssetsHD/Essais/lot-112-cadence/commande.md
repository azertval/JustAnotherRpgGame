# LOT-112 — l'essai de cadence : six ou huit images

Deux envois, **identiques à un mot près** (`SIX` / `EIGHT`). Chacun joint la planche de référence
`Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png`, et rien d'autre.

Les sorties s'enregistrent ici, sous ces noms exacts :

| Envoi | Fichier |
|---|---|
| six images | `walk-6.png` |
| huit images | `walk-8.png` |

Le canevas : le plus large que le générateur propose, en paysage. Une image de la marche doit faire
**au moins 192 px de large** une fois sortie (l'art est réduit, jamais agrandi), soit un canevas
d'au moins 1208 px pour six et 1608 px pour huit.

Le texte ci-dessous est la [consigne](../../../../Planning/standards/consigne-2d-hd.md) assemblée :
bloc A inchangé, bloc B dans sa variante planche d'animation (losange 256 × 159, emprise 1 × 1), et
le bloc C de la [fiche du lot](../../../../Planning/versions/v0.1.0/v0.0.1-demo/lots/LOT-112-heros-de-la-demo.md).

## Envoi 1 — six images

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

VIEW: fixed isometric camera, orthographic — no perspective, no vanishing
point. The ground is a flat diamond 256 pixels wide and 159
tall (ratio 0.62). The piece stands on a footprint of 1 by
1 such diamonds; its faces run along the two diagonal directions of
that grid.

FRAMING (animation strip): ONE character, drawn once per frame, in a single
horizontal strip on a FULLY TRANSPARENT background. Same figure, same colours,
same proportions and same light in every frame; only the pose changes. Each
frame is the same size, evenly spaced, with 8 px of margin between frames and
around the strip. No shadow, no base plate, no separating line, no frame
number. Draw it as large as the canvas allows: the piece is reduced when it is
installed, never enlarged.

OUTPUT: landscape canvas, as wide as available, PNG with alpha.

PIECE: a walk cycle for a city guard of the Central Empire, seen from the
south-east isometric direction: burgundy tabard over a mail shirt, old gold
trim, a spear held upright in the right hand, no shield.
FAMILY: figure — humanoid, 170 px tall, ground line at the bottom of each frame.
PLACE: Arenarea, the arena quarter of the Capital; accent burgundy.
VARIANTS: SIX frames — one full walk cycle that loops, the contact,
down, passing and up poses evenly spread over the strip.
```

## Envoi 2 — huit images

Le même texte, dont la dernière ligne devient :

```
VARIANTS: EIGHT frames — one full walk cycle that loops, the contact,
down, passing and up poses evenly spread over the strip.
```

## Juger : les deux marches sur la maquette

Le descripteur `install.json` de ce dossier est un **essai** (`previewOnly`) : il se mesure et
s'aperçoit, il ne s'installe jamais. Une fois les deux images enregistrées :

```
python scripts/install_hd_asset.py Tools/AssetsHD/Essais/lot-112-cadence/install.json --measure
python scripts/preview_figure_walk.py Tools/AssetsHD/Essais/lot-112-cadence/install.json --figure essai-6 --loops 4 --out build/lot-112/marche-6.webp
python scripts/preview_figure_walk.py Tools/AssetsHD/Essais/lot-112-cadence/install.json --figure essai-8 --loops 4 --out build/lot-112/marche-8.webp
```

Le garde marche vers le sud-est, sur la vue 1080 de la maquette, à la vitesse du jeu (4 cases par
seconde), les pieds au centre du losange. `--frame-duration` essaie une autre cadence sans rien
toucher. Les trois questions de la [fiche](../../../../Planning/versions/v0.1.0/v0.0.1-demo/lots/LOT-112-heros-de-la-demo.md) :
la marche à six **saccade-t-elle** ? à huit, est-ce **le même personnage** d'une image à l'autre ?
le quart de coût en plus, sur tout le jeu, se paie-t-il ?

## Deux écarts à la consigne, et pourquoi

- **`OUTPUT`** : le bloc B dit « square canvas », ce qui contredit une planche en bande. La variante
  planche d'animation ne remplaçait que le cadrage ; elle remplace aussi cette ligne. À reporter
  dans la consigne si l'essai la confirme.
- **« Draw it as large as the canvas allows »** est gardé dans le cadrage de la planche : la variante
  l'écrasait avec les deux premières phrases, alors que c'est la règle « réduit, jamais agrandi ».
