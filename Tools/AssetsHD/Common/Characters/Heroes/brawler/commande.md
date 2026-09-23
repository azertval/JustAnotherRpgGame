# LOT-112 — la commande du héros : le Brawler pré-tiré

Le héros de la démo est la fiche pré-tirée du Brawler (*Player's Guide to Tanares*, p. 195) :
demi-orc, Dragon Hunter, grande hache. Le livre est une **référence de costume, en mots** : son
illustration n'est jamais jointe au générateur, ni décalquée (standard 2D HD, §6).

> **À envoyer après l'essai de cadence** (`Tools/AssetsHD/Essais/lot-112-cadence/`). `{N}` ci-dessous
> est le nombre d'images qu'il aura fixé ; il vaut pour les cinq animations.

## L'ordre des envois

La consigne du `LOT-91` tient : **le portrait d'abord**, puis une première bande qui fixe le
personnage, puis les autres **avec elle en référence** — demander vingt bandes sans référence, c'est
obtenir vingt demi-orcs.

| # | Envoi | Pièces jointes | Fichier |
|---|---|---|---|
| 1 | portrait | planche de référence | `portrait.png` |
| 2 | repos, sud-est | planche, `portrait.png` | `idle-se.png` |
| 3-6 | marche, attaque, touché, mort — sud-est | planche, `idle-se.png` | `walk-se.png`, `attack-se.png`, `hit-se.png`, `death-se.png` |
| 7-21 | les cinq animations — sud-ouest, nord-est, nord-ouest | planche, `idle-se.png`, la même animation en sud-est | `<animation>-<sw\|ne\|nw>.png` |

La planche de référence est `Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png`. Chaque
sortie s'enregistre **dans ce dossier**, sous le nom de la dernière colonne. Une bande qui ne va pas
se **recommande** entière, par la même commande : on corrige la commande, jamais l'image (règle du
`LOT-91`).

Le canevas : paysage, le plus large proposé. Une image de bande doit sortir à **192 px de large au
moins**, 384 pour l'attaque (l'art est réduit, jamais agrandi).

## Envoi 1 — le portrait

Le bloc A de la [consigne](../../../../../Planning/standards/consigne-2d-hd.md), inchangé, puis :

```
FRAMING (portrait): ONE character, head and shoulders, three-quarter view
facing the lower left, centred in a square canvas. A plain, softly lit
background in dark bronze (#5c4a2a) fading to warm grey (#9c948a) — no scenery,
no border, no text. The face and the upper chest fill most of the canvas.

OUTPUT: square canvas, at least 1024 px, PNG.

PIECE: portrait of the demo hero, a half-orc brawler: grey-green skin, a
heavy jaw with two short lower tusks, deep-set dark eyes, long black hair in
thick braids, bare muscular shoulders and chest slashed with burgundy war
paint, a leather shoulder strap with an old gold buckle; the dark oiled haft
of a greataxe rests against his right shoulder. Hard, watchful, not cruel.
FAMILY: portrait.
PLACE: none — a wanderer from beyond the Central Empire; accent burgundy.
VARIANTS: none.
```

## Envois 2 à 21 — les bandes

Le bloc A inchangé, le bloc B dans sa **variante planche d'animation** (losange 256 × 159, emprise
1 × 1 ; voir [la commande de l'essai](../../../../Essais/lot-112-cadence/commande.md), qui l'assemble
déjà, avec ses deux écarts), puis ce bloc C, où seules trois lignes changent : `{ANIMATION}`,
`{DIRECTION}` et `{N}`.

```
REFERENCE: the attached strip shows THIS character — same face, same braids,
same war paint, same axe, same proportions. Draw him again, only the pose and
the direction change.

PIECE: {ANIMATION} for the demo hero, a half-orc brawler, {DIRECTION}:
grey-green skin, heavy muscular build, bare chest and arms slashed with
burgundy war paint, long black hair in braids, two short lower tusks, dark
leather trousers and boots, a wide leather belt and a shoulder strap with an
old gold buckle, a two-handed greataxe with a dark oiled haft and a steel head,
always held in his RIGHT hand.
FAMILY: figure — humanoid, 170 px tall, ground line at the bottom of each frame.
PLACE: none — a wanderer from beyond the Central Empire; accent burgundy.
VARIANTS: {N} frames — {TIMING}.
```

L'envoi 2 n'a pas encore de bande à joindre : il omet la ligne `REFERENCE` et joint le portrait,
avec `REFERENCE: the attached portrait shows THIS character.` à la place.

### `{ANIMATION}` et `{TIMING}`

| Animation | `{ANIMATION}` | `{TIMING}` |
|---|---|---|
| `idle` | `an idle stance` | `a calm breathing loop, the greataxe resting on his right shoulder, the weight shifting slightly; the last frame leads back into the first` |
| `walk` | `a walk cycle` | `one full walk cycle that loops, the contact, down, passing and up poses evenly spread over the strip, the greataxe carried on his right shoulder` |
| `attack` | `an overhead greataxe attack` | `one two-handed overhead swing played once: wind-up, strike, follow-through, recovery. Each frame is TWICE AS WIDE as a walk frame, so that the whole swing fits inside it` |
| `hit` | `a hit reaction` | `recoiling from a blow and recovering, played once, feet staying on the ground` |
| `death` | `a death` | `collapsing to the ground, played once; in the last frame he lies still, flat on the ground line` |

### `{DIRECTION}`

| Suffixe | `{DIRECTION}` |
|---|---|
| `se` | `seen from the south-east isometric direction: three-quarter front view, he faces the lower right of the image` |
| `sw` | `seen from the south-west isometric direction: three-quarter front view, he faces the lower left of the image` |
| `ne` | `seen from the north-east isometric direction: three-quarter back view, he faces the upper right of the image` |
| `nw` | `seen from the north-west isometric direction: three-quarter back view, he faces the upper left of the image` |

## Ce que deviennent les sources

Le descripteur `install.json` de ce dossier se remplit quand les sources arrivent (le nombre
d'images de chaque bande est celui de l'essai) : `scripts/install_hd_asset.py` les découpe, les pose
sur la ligne de sol, et installe le tout dans `Source/Elements/Assets/Common/Characters/Heroes/brawler/`.
La mort est prévue en cellule normale ; si le corps allongé n'y tient pas, l'installateur le dit et
la bande passe en cellule large.
