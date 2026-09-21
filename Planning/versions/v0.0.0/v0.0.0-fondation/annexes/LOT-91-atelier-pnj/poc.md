# LOT-91 — Preuve de concept : cinq planches à la main

> Tâche **T0** du [LOT-91](../../lots/LOT-91-atelier-pnj.md). Ce document est un **protocole** : chaque étape est
> exécutée à la main, dans l'ordre, et son résultat est noté dans le journal en fin de page. Rien
> de la chaîne n'est codé avant que ce protocole ait rendu son verdict — c'est lui qui dit ce
> qu'il faut coder, et s'il faut le coder.
>
> Ce qu'il doit prouver, en cinq planches : que le générateur tient le style d'une planche à
> l'autre avec les mêmes références ; que le fond transparent est exploitable ; que le pas de
> pixel se mesure et se ramène à la grille ; que le compte d'images par rangée est tenu assez
> souvent pour qu'une boucle automatique ait un sens ; et ce que coûte une planche retenue.

## 0. Le poste

- Python **3.13** par le lanceur `py -3.13` (le `python` du Bash de Claude Code est celui
  d'Inkscape, sans pip). `numpy`, `Pillow`, `pymupdf` sont installés ; il manque le SDK :

```
py -3.13 -m pip install openai
```

- La clé d'API dans l'environnement de la session PowerShell (jamais dans un fichier du dépôt) :

```
$env:OPENAI_API_KEY = "sk-..."
```

- Un dossier de travail **hors dépôt**, où tout ce que le protocole produit se range :
  `D:\JustAnotherDnDGame-npc-poc\` avec un sous-dossier par PNJ. Rien de ce dossier n'est
  versionné avant la clôture du protocole.
- Le modèle courant de l'API Images, lu sur la doc du jour ; ce document écrit `gpt-image-1`.

## 1. Les cinq PNJ du pilote

Tous *Medium*, donc tous en 48 × 64 ; choisis pour couvrir des silhouettes, des tenues et des
jeux d'animations différents. (Le Tigre impérial et Morlogh, prévus par l'étude, sont *Large* :
ils attendent la classe 96 × 96.)

| Slug | Fiche | Page | Pourquoi lui |
|---|---|---|---|
| `anariel` | Anariel, the Swordmage | 10 | La référence : lanceuse et rapière, six rangées, la maquette existe. |
| `xorius` | Xorius, the Archers' General | 148 | Archer : la rangée `attack` est un tir, pas une frappe. |
| `lizz` | Lizz, the Medusa | 83 | Silhouette non humaine (chevelure de serpents), sans armure de plates. |
| `nakral` | Nakral, the Death Knight | 92 | Tenue très sombre : le test du fond transparent et des contours. |
| `jade` | Jade, the Bard | 68 | Humaine sans armure, instrument à la place de l'arme ; `cast` = jouer du luth, pas un sort de la main libre. |

## 2. Les références (étape E1, à la main)

Pour chaque PNJ, quatre images dans `D:\JustAnotherDnDGame-npc-poc\<slug>\ref\`.

**2.1 — Portrait et figurine dorée**, rendus depuis la page (jamais extraits par `xref`). Le
Compendium est une double page à pagination **simple** : page PDF 10 = fiche 10, largeur
1 224 points, portrait à gauche, figurine à droite.

```
py -3.13 - <<'EOF'
import pymupdf
PAGE = 10                      # numéro de fiche = numéro de page PDF
SLUG = "anariel"
d = pymupdf.open(r"Documentation/SourceBook/VTT/Character Compendium - High.pdf")
p = d[PAGE - 1]; W, H = p.rect.width, p.rect.height
out = rf"D:\JustAnotherDnDGame-npc-poc\{SLUG}\ref"
import os; os.makedirs(out, exist_ok=True)
p.get_pixmap(dpi=300, clip=pymupdf.Rect(0, 0, W/2, H)).save(rf"{out}\ref3_portrait.png")
p.get_pixmap(dpi=300, clip=pymupdf.Rect(W/2, 0, W, H)).save(rf"{out}\ref4_figurine_page.png")
EOF
```

Recadrer `ref4_figurine_page.png` à la main sur la figurine dorée seule (tout éditeur d'image) ;
la découpe automatique de la moitié droite est une tâche de T2, pas du protocole.

**2.2 — Le portrait, généré en pixel art.** Les jetons du corpus (`Tokens VTT PNG/`) ne sont
**pas** employés (décision du 16 septembre) : le portrait est demandé au générateur, au style du
buste en haut à gauche de la maquette d'Anariel. Un appel par PNJ, deux candidats, trois
références (ancre de style, portrait peint, figurine), prompt = `portrait_style.md` suivi des
lignes `CHARACTER`, `LOOK` et `WEAPON` du bloc B :

```
py -3.13 portrait.py anariel          # -> anariel\token\candidat1.png, candidat2.png, journal.json
```

Le portrait est fait **avant** la planche : il fixe le visage et les couleurs à petite échelle,
et la planche le reçoit en référence 5. On retient un candidat à l'œil (visage, coiffe, couleurs
du portrait peint ; grille de 8 px nette ; fond navy uni) et on note le choix dans le journal.

**2.3 — La palette, tirée du portrait retenu.**

```
py -3.13 palette.py anariel anariel\token\candidat1.png
```

Le script copie le candidat en `ref\ref5_token.png`, quantifie en sept couleurs **hors fond
navy**, écrit `ref\palette.txt` et remplace la ligne `PALETTE` de `prompt_b.txt`. Le pixel art a
peu de couleurs, la quantification y est fiable, ce qu'elle n'était ni sur un jeton peint ni sur
un portrait peint (voir le journal). `tour.py` joint `ref5_token.png` dès qu'il existe.

**2.4 — L'ancre de style, commune à tous** : `Documentation/SourceBook/VTT/Anariel_MockUp_SpriteSheet.png`,
copiée en `ref1_style.png` à la racine du dossier de travail.

**2.5 — La bande d'échelle, commune à tous** : les quatre héros du Colisée à 2 px par pixel sur
fond transparent, image 1 de leur `idle`.

```
py -3.13 - <<'EOF'
from PIL import Image
heroes = ["kaelith_voss", "elira", "darin", "bram"]
strip = Image.new("RGBA", (4 * 192, 170), (0, 0, 0, 0))
for i, h in enumerate(heroes):
    band = Image.open(rf"Source/Elements/Assets/Coliseum/characters/{h}/idle.png").convert("RGBA")
    frame = band.crop((0, 0, 48, 64)).resize((96, 128), Image.NEAREST)
    strip.paste(frame, (i * 192 + 48, 170 - 20 - 128), frame)   # pieds 20 px au-dessus du bas
strip.save(r"D:\JustAnotherDnDGame-npc-poc\ref2_scale.png")
EOF
```

## 3. Le style commun v1 (étape E2, bloc A)

Écrire `D:\JustAnotherDnDGame-npc-poc\style.md`, **en anglais**, en recopiant le bloc A de la
page d'étude, tel quel, avec en tête une ligne `STYLE v1 — 2026-09-16`. Il ne change pas pendant
le protocole : si une planche montre qu'il doit changer, on note la correction dans le journal et
on ne l'applique qu'à la version v2, après les cinq PNJ. C'est ainsi qu'on sait si un défaut vient
du style ou du PNJ.

Le bloc B de chaque PNJ (cinq lignes : `CHARACTER`, `LOOK`, `WEAPON`, `PALETTE`, `ANIMATION SET`)
est rédigé à la main dans `<slug>\prompt_b.txt`, à partir des rendus de l'étape 2 : nommer l'arme,
la tenue, la coiffe, ce qui distingue ce PNJ d'un autre. Pour Jade, `ANIMATION SET` décrit
`cast` comme un chant de barde (les deux mains au luth, notes magiques). Pour Xorius, la ligne `WEAPON` dit que la rangée 3 est un tir à l'arc.

## 4. La référence canonique : Anariel (étape R)

La maquette `Anariel_MockUp_SpriteSheet.png` a prouvé que le style est atteignable, mais elle
**n'est pas au format** que le prompt exige : fond bleu nuit peint, cartouches, légendes, numéros
d'images, nuancier, bande d'échelle, images réparties à l'œil et non sur la grille. Envoyée comme
ancre de style à chaque PNJ, elle enseigne au générateur ces défauts autant que le style. L'étape R
produit donc, **avant les quatre autres PNJ**, une Anariel qui est exactement ce que la chaîne doit
rendre ; elle remplace ensuite la maquette comme référence 1.

Anariel ne compte pas dans les chiffres du verdict (§10) : elle a droit à plus de tours, et à la
retouche.

**R1 — Le portrait.** Pendant R seulement, l'ancre de portrait `ref1_portrait_style.png` est le
buste recadré de la maquette (`maquette\anariel_buste.png`).

```
py -3.13 portrait.py anariel
py -3.13 normalise_portrait.py anariel\token\candidatN.png ancre\anariel_portrait.png
py -3.13 palette.py anariel ancre\anariel_portrait.png
```

`normalise_portrait.py` met le candidat retenu au format exact : 1 024 × 1 024, grille de
128 × 128 pixels d'art de 8 px (médiane de chaque bloc), au plus 32 couleurs, fond `#0B141E`
exact ; il écrit aussi la version 128 × 128.

**R2 — La planche.** Un PNJ se génère en **une seule planche de 1 536 × 2 048**
(`disposition_planche.md`), un seul appel, donc un seul personnage : générés en deux planches,
corps et effets divergeaient (proportions, jambières, diadème, taille rendue ; décision du
16 septembre, voir le journal). La taille n'est pas celle de `gpt-image-1` : les modèles depuis
`gpt-image-2` acceptent toute taille multiple de 16, rapport entre 1:3 et 3:1, éprouvée jusqu'à
2 560 × 1 440 pixels de surface.

| Rangées | Cellules | Contenu | Bandes |
|---|---|---|---|
| 1-3 | 192 × 256, pieds centrés | `idle` 6, `walk` 8, `hit` 4 | 48 × 64, pieds à x = 24 |
| 4 | 256 × 256, pieds centrés | `death` 6 | **96 × 64**, pieds à x = 48 |
| 5-8 | 384 × 256, pieds au tiers gauche | `attack` 1-4, 5-8, `cast` 1-4, 5-8 | **96 × 64**, pieds à x = 32 |

Le prompt est : disposition, puis `style.md` (style, vue, échelle, fond, interdits), puis le bloc B. Pendant R, `ref1_style.png` est la maquette.

**On ne corrige jamais une planche, on corrige le prompt** (décision du 16 septembre) : un défaut
constaté devient une phrase de la disposition, de `style.md` ou du bloc B, et le tour suivant est
une **génération complète** depuis ce prompt. C'est ce qui rend le résultat reproductible sur les
autres PNJ ; une planche réparée par édition ne l'est pas. Pendant R, les tours n'ont **pas de
limite de trois** : on s'arrête quand la planche passe tout. La retouche à la main dans un éditeur de pixel art est **permise ici, et seulement ici**
(image manquante copiée et ajustée, pixel parasite, couleur hors palette) ; chaque retouche est
notée au journal.

```
py -3.13 tour.py anariel planche K
py -3.13 normalise.py anariel\tourK\planche\candidatN.png ancrenariel planche --hauteur 45
```

`normalise.py` lit rangées et images par profil, binarise l'alpha, ramène le personnage à
**45 pixels d'art**, la hauteur des héros du Colisée (un facteur par planche, pris sur la pose de
garde : `idle` au corps, `attack` 8 aux effets), puis écrit trois choses : la planche au format
exact du prompt (`corps.png`, `effets.png` : pixels de 2 px, pieds centrés à 40 px du bas de la
cellule), les **bandes du jeu** (`bandes\idle.png`… : pieds en bas, y = 63, à x = 24 dans une
cellule de 48, à x = 32 dans une cellule de 96, les effets partant à droite) et un rapport JSON.
Il rend 1 si un compte diffère, si une image sort de sa cellule de planche, ou si plus de 4 pixels
d'art sortent de la bande ; de 1 à 4, il rogne et avertit. `--vides 3,4` déclare les rangées
`cast` vides (Jade).

La planche est **acceptée** quand : `normalise.py` rend 0 ; les neuf questions du
§7 ont neuf « oui » ; et R3 est bon.

**R3 — En jeu.** Les bandes d'Anariel remplacent celles d'un héros au Colisée (§9) : juger
l'échelle et la lisibilité à l'écran. Une ancre illisible à l'écran n'est pas une ancre.

**R4 — La bascule.** Une fois R accepté (fait le 16 septembre, sur le composite tour 4 + tour 6) :

```
py -3.13 compose.py anariel 4 6                     # anariel\final\ : planche, bandes, rapport
copy anariel\final\* ancre\anariel\                 # l'ancre du tour 3 est rangée dans ancre\_tour3\
copy ancre\anariel\planche.png ref1_style_planche.png
copy ancre\anariel_portrait.png ref1_portrait_style.png
```

La maquette et son buste sont rangés dans `maquette\` et **ne sont plus jamais envoyés**. `style.md`,
`portrait_style.md` et les dispositions sont **gelés** en v1 à ce moment : les corrections découvertes
pendant R y entrent avant le gel, celles découvertes sur les quatre autres PNJ iront en v2. Les
quatre autres PNJ reçoivent donc des références au format exact, et leur écart à l'ancre mesure la
chaîne, plus la maquette.

Ce qui est entré au gel, pour que le prompt d'Anariel devienne celui de tous (les versions d'avant
sont dans `_v0\`) :

- la référence 1 d'une planche montre **un autre personnage** : en copier la grille, la taille, le
  pixel, le contour et le mouvement de chaque rangée, jamais le visage, la tenue, l'arme ni les
  couleurs (`disposition_planche.md`) ; de même pour l'ancre de portrait (`portrait_style.md`) ;
- la main d'arme est **celle de la ligne `WEAPON`** du bloc B, plus « la main droite » (Xorius tient
  son arc de la main gauche) ; la main droite du personnage reste à gauche du spectateur ;
- les mots propres à Anariel sortent des dispositions (diadème, jambières, cape, rapière, flamme) ;
- la description de la marche sort des dispositions en `{MARCHE}`, remplie par `marche_jambes.md`,
  ou par `<slug>\marche.md` quand le PNJ n'a pas de jambes (Lizz : `lizz\marche.md`, une ondulation
  de queue en quatre silhouettes, sur le même patron).

## 4bis. La méthode standard, pour un PNJ {#lot-91-methode}

Ce qu'Anariel a appris, écrit pour qu'un PNJ quelconque passe par les mêmes étapes, avec les mêmes
fichiers. C'est ce que T2 à T4 coderont ; pendant T0, les quatre autres PNJ la suivent telle quelle,
et c'est elle que le verdict (§10) valide.

| Étape | Appel | Références, dans l'ordre | Sortie retenue | Contrôle |
|---|---|---|---|---|
| **P** portrait | `portrait` (1024²) | ancre de portrait d'Anariel, portrait peint, figurine | `<slug>\token\portrait.png` (normalisé) | revue à l'œil : visage, coiffe, couleurs ; puis `palette.py` |
| **S** planche | `planche` (1536 × 2048) | planche d'Anariel, échelle, portrait peint, figurine, portrait pixel art | `<slug>\tourK\norm\` | `normalise.py … planche --hauteur 45` à 0, puis les neuf questions (§7) **sauf la marche** |
| **M** marche, **si la rangée `walk` bouge peu** | `marche` (1536 × 256) | **la planche normalisée du PNJ**, échelle, portrait peint, figurine, portrait pixel art | `<slug>\tourK\norm\` | `normalise.py … marche --hauteur 45` à 0 ; huit silhouettes distinctes, même personnage |
| **X** passe par animation, **si une seule animation est fausse** | `idle`, `hit`, `death` (1536 × 256), `attack`, `cast` (1536 × 512) | **la planche normalisée du PNJ**, échelle, portrait peint, figurine, portrait pixel art | `<slug>\tourK\norm\` | `normalise.py … <anim> --hauteur 45` à 0, puis les questions du §7 sur cette animation |
| **A** assemblage | — | — | `<slug>\final\` | `compose.py <slug> <tour S> [<tour M>] [<anim>=<tour>…]` à 0 |
| **J** en jeu | — | — | `Source/Elements/Assets/Npc/<slug>/` | R3 : lisible à l'échelle du Colisée |

- **La marche n'est un second appel que si la planche l'a manquée.** Pour Anariel, trois tours de
  prompt (3, 4, 5) n'ont pas tiré une marche de la planche entière ; le rang seul, avec la planche du
  personnage en référence 1, l'a fait du premier coup. Mais Lizz l'a réussie dans sa planche (décision
  de l'utilisateur, 16 septembre) : l'étape M est donc **conditionnelle**. `normalise.py` écrit
  `rapport_walk_idle`, l'écart moyen d'alpha entre images consécutives de `walk` divisé par celui
  d'`idle`, et avertit sous **4** ; calibré sur Anariel (figée 2,1 au tour 4, à moitié lue 3,1 au tour 5,
  retenue 4,2 au tour 6) ; Lizz 4,7. Le chiffre trie, l'œil tranche. Avec l'étape M, c'est une
  entorse assumée à « une planche, un appel ».
- **Un prompt, deux modes.** `prompts.py` assemble prompt et références ; `portrait.py` et `tour.py`
  les envoient par l'API, `chatgpt.py` les range pour l'interface. Le prompt collé à la main est
  donc, au caractère près, celui que l'API recevra.
- **Une animation fausse se refait seule, jamais par emprunt (décision de sortie du PoC,
  16 septembre).** Quand une planche passe tout sauf une animation, on génère **cette animation
  seule**, sur le modèle de la marche : `disposition_reprise.md` (gabarit : un personnage copié de la
  référence 1, ses images de cette animation déclarées fausses) suivi des lignes `Row` de l'animation
  **reprises de `disposition_planche.md`**, qui reste la seule source des consignes. On n'emprunte pas
  les images d'une autre planche : pour Nakral, aucune des trois n'avait `attack` 4-5 juste (l'épée
  dédoublée revient à chaque tour), et d'un tour à l'autre le personnage change de dessin (épée tenue
  haute aux tours 1-2, basse au tour 3 ; épaulière, proportions), la divergence qui avait fait
  abandonner corps et effets séparés. La marche (M) est le cas particulier de cette règle.
- **Coût** : une planche, plus une passe par animation fausse (marche comprise). Au-delà de deux
  passes pour un PNJ, la planche elle-même est à refaire.
- **Les tours comptent par étape** : au plus trois pour P, trois pour S, trois pour M (§8). Une étape
  qui échoue trois fois met le PNJ en file humaine ; les étapes suivantes n'ont pas lieu.

En mode interface, les PNJ d'un lot passent **ensemble, étape par étape** :

```
py -3.13 chatgpt.py portrait 1 xorius lizz nakral jade
#   pour chaque chatgpt\portrait-tour1\<slug>\ : coller prompt.txt, joindre 1_…, 2_…, 3_… dans l'ordre,
#   enregistrer l'image au chemin de A_ENREGISTRER_SOUS.txt
py -3.13 normalise_portrait.py xorius\token\tour1\candidat1.png xorius\token\portrait.png
py -3.13 palette.py xorius xorius\token\portrait.png                   # ref5_token.png + ligne PALETTE
#   … idem pour chaque PNJ retenu

py -3.13 chatgpt.py planche 1 xorius lizz nakral jade
py -3.13 normalise.py xorius\tour1\planche\candidat1.png xorius\tour1\norm planche --hauteur 45

py -3.13 chatgpt.py marche 2 xorius lizz nakral jade                   # tour 2 : la marche suit la planche du tour 1
py -3.13 normalise.py xorius\tour2\marche\candidat1.png xorius\tour2\norm marche --hauteur 45
py -3.13 compose.py xorius 1 2
```

Le numéro de tour n'est qu'un dossier : la marche prend la planche normalisée du **tour le plus
récent** (`tourK\norm\planche.png`), d'où un numéro de tour au-dessus de celui de la planche.

## 5. Le tour 1 (étape E3)

Un appel par PNJ, trois candidats, les références dans l'ordre fixe : style, échelle, portrait peint, figurine, portrait pixel art retenu. `py -3.13 tour.py <slug> 1` fait l'appel ci-dessous.

```
py -3.13 - <<'EOF'
import base64, json, datetime, hashlib, pathlib
from openai import OpenAI
SLUG = "anariel"; ROOT = pathlib.Path(r"D:\JustAnotherDnDGame-npc-poc"); D = ROOT / SLUG
refs = [ROOT/"ref1_style.png", ROOT/"ref2_scale.png", D/"ref"/"ref3_portrait.png", D/"ref"/"ref4_figurine.png"]
prompt = (ROOT/"style.md").read_text(encoding="utf-8") + "\n\n" + (D/"prompt_b.txt").read_text(encoding="utf-8")
client = OpenAI()
r = client.images.edit(model="gpt-image-1", image=[open(p, "rb") for p in refs], prompt=prompt,
                       n=3, size="1536x1024", quality="high", output_format="png", background="transparent")
tour = D / "tour1"; tour.mkdir(exist_ok=True)
for i, img in enumerate(r.data, 1):
    (tour / f"candidat{i}.png").write_bytes(base64.b64decode(img.b64_json))
(tour / "journal.json").write_text(json.dumps({
    "date": datetime.datetime.now().isoformat(timespec="seconds"), "model": "gpt-image-1",
    "prompt": prompt, "refs": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in refs},
    "n": 3, "size": "1536x1024", "quality": "high", "background": "transparent"}, indent=2, ensure_ascii=False), encoding="utf-8")
print("ok", tour)
EOF
```

Noter dans le journal le **coût** affiché sur le tableau de bord OpenAI après les cinq appels du
tour 1 : c'est le seul chiffre de coût fiable.

## 6. Le contrôle mécanique (étape E4, en mode manuel)

Pour chaque candidat, six mesures. Elles seront le cœur de `receive.py` ; ici on les fait
tourner à la main et on lit les nombres.

```
py -3.13 - <<'EOF'
import numpy as np
from PIL import Image
P = r"D:\JustAnotherDnDGame-npc-poc\anariel\tour1\candidat1.png"
im = np.asarray(Image.open(P).convert("RGBA")).astype(int); H, W, _ = im.shape
a = im[:, :, 3]
print("1. taille", W, H, "| alpha unique ?", np.unique(a).size, "valeurs ; semi-transparents :", int(((a > 0) & (a < 255)).sum()))
mask = a > 127
rows = mask.sum(axis=1) > 0
# rangées : plages de lignes non vides
bands, inr = [], False
for y, v in enumerate(rows):
    if v and not inr: y0, inr = y, True
    if not v and inr: inr = False; bands.append((y0, y))
if inr: bands.append((y0, H))
print("2. rangées trouvées", len(bands), [b[1]-b[0] for b in bands])
# cellules par rangée : colonnes non vides, coupées aux vallées de largeur >= 6 px
for k, (y0, y1) in enumerate(bands, 1):
    cols = mask[y0:y1].sum(axis=0) > 0
    cells, inc, gap = [], False, 0
    for x, v in enumerate(cols):
        if v:
            if not inc: x0, inc = x, True
            gap = 0
        elif inc:
            gap += 1
            if gap >= 6: inc = False; cells.append((x0, x-gap))
    if inc: cells.append((x0, W))
    feet = [int(np.where(mask[y0:y1, c0:c1].any(axis=1))[0].max()) + y0 for c0, c1 in cells]
    print(f"3. rangée {k}: {len(cells)} images, largeurs {[c1-c0 for c0,c1 in cells]}, pieds {feet}")
# pas de pixel : autocorrélation du gradient horizontal sur la première rangée
y0, y1 = bands[0]; crop = im[y0:y1, :, :3].sum(axis=2)
g = np.abs(np.diff(crop, axis=1)).sum(axis=0); g = g - g.mean()
ac = np.correlate(g, g, "full")[len(g)-1:]
print("4. pas de pixel (candidats par autocorrélation)", sorted(range(2, 9), key=lambda k: -ac[k])[:3])
print("5. hauteur de la 1re image", int(np.where(mask[y0:y1, cells[0][0]:cells[0][1]].any(axis=1))[0].ptp()) + 1, "px (cible ~110 à pas 2)")
EOF
```

Ce qu'on lit, et le verdict :

| Mesure | Attendu | Sinon |
|---|---|---|
| 1. Alpha | 1536 × 1024 ; peu de valeurs intermédiaires, aucune plage semi-opaque hors figurine | Si le modèle a peint un damier ou un fond, **repli chroma** : refaire le tour 1 avec `background="opaque"` et le bloc A en magenta. C'est le verdict le plus important du protocole. |
| 2. Rangées | 6 | Candidat éliminé. |
| 3. Images par rangée | 6 / 8 / 8 / 4 / 6 / 8 ; pieds à ± 2 px dans la rangée (sauf `death` 5-6) | Candidat éliminé ; si les trois candidats échouent sur la même rangée, c'est le prompt de la rangée qui est en cause : noter. |
| 4. Pas | 2 en tête, net | Un pas non entier ou flou : le style doit dire plus fort « crisp square pixels » ; noter pour v2. |
| 5. Hauteur | 100 à 120 px | Trop grand ou trop petit : la bande d'échelle n'est pas lue ; noter. |

Un candidat qui passe les cinq mesures est **survivant**. S'il n'y en a aucun sur les trois, on
passe directement au tour 2 en partant du candidat le moins mauvais.

## 7. La revue par vision, contre une liste fermée

Sur chaque survivant, ouvrir la planche et répondre **oui ou non** à ces neuf questions ; une
réponse « non » et sa raison vont au journal. Claude peut faire cette relecture (joindre la
planche, le portrait et la figurine dorée à la demande) ; l'humain tranche.

1. Le visage, la coiffure et les couleurs sont ceux du portrait.
2. La silhouette, l'arme et la garde sont celles de la figurine dorée.
3. Le personnage est **le même** sur les quarante images (tenue, proportions, arme).
4. Le style est celui de la planche d'Anariel : contour, ombrage, densité, pas d'anticrénelage.
5. La lumière vient de la même direction sur toutes les images.
6. `idle` boucle (l'image 6 ramène vers la 1) ; `attack`, `hit`, `cast` finissent près de `idle` 1.
7. `death` 5 et 6 sont couchées, sur la ligne.
8. Les effets (arc, flamme, flèche) restent dans la cellule et dans la palette.
9. Aucun texte, cadre, légende, ombre au sol, décor.

Neuf « oui » : la planche est **retenue**. Sinon, chaque « non » est noté au journal avec la phrase de prompt qu'il appelle.

## 8. Les tours 2 et 3 (nouvelle génération)

Pas de correction par édition (décision du 16 septembre ; l'essai du tour 2 d'Anariel l'a montré :
l'interface régénère tout et ne rattrape pas un compte d'images). Le tour K+1 est une **génération
complète avec le même prompt** : pendant le protocole, `style.md` et les dispositions sont gelés,
et un défaut qui revient d'un tour à l'autre est une correction pour la v2, pas une retouche du PNJ.
Seul le bloc B du PNJ peut être précisé (arme, tenue mal lues), et cela se note. Au plus trois
tours ; au-delà, le PNJ passe en **file humaine** : on note pourquoi,
on passe au suivant, on ne retouche pas l'image à la main pendant le protocole.

## 9. La découpe d'une planche retenue

Pour chaque planche retenue, produire les bandes 48 × 64 pour les voir dans le jeu. À ce stade
la découpe est manuelle-assistée : les rangées et cellules de l'étape 6, un rééchantillonnage au
pas mesuré, l'alpha binarisé, chaque image recentrée sur son ancre au pied.

```
py -3.13 - <<'EOF'
import numpy as np
from PIL import Image
P = r"D:\JustAnotherDnDGame-npc-poc\anariel\tour1\candidat1.png"; OUT = r"D:\JustAnotherDnDGame-npc-poc\anariel\bands"
PAS = 2; ROWS = [("idle", 6), ("walk", 8), ("attack", 8), ("hit", 4), ("death", 6), ("cast", 8)]
im = Image.open(P).convert("RGBA")
im = im.resize((im.width // PAS, im.height // PAS), Image.NEAREST)          # 768 x 512 à pas 1
a = np.asarray(im)[:, :, 3]; mask = a > 127
# ... reprendre ici les rangées/cellules de l'étape 6 (divisées par PAS) ...
# pour chaque cellule : bbox, recadrage, collage dans un canevas 48 x 64, pieds à y = 60, centre x = 24
# alpha binarisé : im.putalpha(Image.fromarray((a > 127).astype("uint8") * 255))
# bandes : une image par colonne, `idle.png` = 6 x 48 de large, 64 de haut
EOF
```

Le corps de cette découpe est ce que T4 écrira proprement ; pendant le protocole, l'objectif est
d'obtenir `idle.png` et `walk.png` d'Anariel dans `Source/Elements/Assets/Coliseum/characters/anariel/`
avec un `idle.anim.json` copié de Bram et adapté (`frames: [0..5]`, `frameDuration: 0.15`,
`loop: true`), puis de la voir au Colisée à la place d'un héros pour juger l'échelle et la lisibilité
**en jeu**. Si une figurine qui passe toutes les mesures est illisible à l'écran, c'est le style
qui est faux, pas la chaîne.

## 10. Le verdict, et ce qu'il déclenche

Le protocole est clos quand Anariel est acceptée (§4) et que les quatre autres PNJ ont un statut :
retenu (tour 1, 2 ou 3) ou file humaine. Les chiffres portent sur ces quatre-là.
Le journal donne alors les cinq chiffres qui décident de la suite :

| Chiffre | Seuil pour coder la chaîne | Si le seuil n'est pas atteint |
|---|---|---|
| Fond transparent exploitable (mesure 1) sur les planches retenues | 4 sur 4 | Repli chroma dans `style.md` v2 et dans E4 ; le protocole ne se rejoue pas pour ça. |
| Taux de rejet mécanique au tour 1 | ≤ 2 candidats sur 3 en moyenne | Le bloc A ne tient pas la toile : le corriger en v2 et rejouer **deux** PNJ. |
| PNJ retenus en ≤ 3 tours | ≥ 3 sur 4 | Rejouer le protocole avec `style.md` v2 avant de coder E3. |
| Pas de pixel entier et net (mesure 4) | 4 sur 4 | Idem : c'est le style. |
| Coût par PNJ retenu (tous tours compris) | à noter, pas de seuil | Décide la taille des lots de la série, pas l'existence de la chaîne. |

Quatre seuils atteints : `style.md` v1 est **confirmé**, l'ancre et les planches retenues deviennent les
figurines de remplacement du projet, et T1 à T5 commencent sur ces planches. Le journal est joint
à la PR de T0.

## Journal

| PNJ | Tour | Candidats survivants (mesures) | « Non » de la revue | Statut | Coût cumulé |
|---|---|---|---|---|---|
| `anariel` | 1 (interface ChatGPT, 1 candidat) | 0 — alpha : OK après binarisation (aucun pixel à 255, 290 k pixels de halo sous 32) ; rangées 6 ; images 5 / 7 / 8 / 4 / 6 / 8 (idle et walk en défaut) ; pas 2 ; hauteur 142 px au lieu de ~110 ; pieds à ± 3 px, 6 px en rangée 3 | 3 (image 6 d'idle absente), 6 (walk 8 absente) — style, visage, arme, mort, effets : oui | **ancre acceptée** (R4, 16 septembre) : planche du tour 4 + marche du tour 6, `anariel\final\` = `ancre\anariel\` | interface, non mesuré |
| `xorius` | S 3 + M 4 : **retenu** | M 4 : 8 images, code 0 ; `compose.py xorius 3 4` : code 0 | aucun (visage gris et yeux visibles dans la marche, sombres dans la planche : sans effet à 45 px) | **terminé** : `xorius\final\` | interface, non mesuré |
| `lizz` | S 1 **retenu**, sans étape M | 1 — 6/8/4/6/4/4/4/4, pieds ± 3, pas 2, garde 118 px ; code 0 ; walk/idle 4,7 | aucun (queue sans le vert du portrait) | **terminée** : `lizz\final\` (`compose.py lizz 1`) | interface, non mesuré |
| `nakral` | S 3 + X `attack` 5 + M 5 : **retenu** | X 5 et M 5 : code 0 ; `compose.py nakral 3 5 attack=5` : code 0 | aucun : une seule épée, tenue basse hors frappe | **terminé** : `nakral\final\` | interface, non mesuré |
| `jade` | S 3 + M 4 : **retenue** | M 4 : 8 images, code 0 ; `compose.py jade 3 4` : code 0 ; walk/idle 4,9 | aucun | **terminée** : `jade\final\` | interface, non mesuré |

Notes libres (portraits retenus, corrections proposées pour `style.md` v2, ce que l'écran a montré) :

- **16 septembre, Anariel tour 1.** Le PNG rendu par l'interface ChatGPT **est** transparent :
  le fond brun flou qu'on voit dans un visualiseur est un halo de pixels semi-transparents
  (alpha < 32), et aucun pixel n'atteint 255. La binarisation au seuil 127 donne des figurines
  propres sur magenta ; la mesure 1 est bonne pour ce candidat, mais le seuil est à garder à 127,
  pas plus bas.
- La grille de 8 colonnes n'est pas respectée : les images sont réparties régulièrement sur la
  largeur de la rangée (5 images sur 1 536 px en `idle`). La découpe par profil s'en accommode ;
  la consigne de cellule fixe ne sert donc qu'à borner la largeur d'une image.
- Hauteur 142 px pour ~110 attendus : la bande d'échelle n'était pas jointe (elle n'existait pas
  encore). À rejouer avec la référence 2 avant de conclure sur l'échelle.
- **Correction pour `style.md` v2** : le bloc A dit « weapon hand on the viewer's right », mais
  sur la maquette (et sur ce résultat, qui a suivi l'image) la rapière est dans la main droite du
  personnage, donc à **gauche** du spectateur. Écrire « sword in the character's right hand, on the
  viewer's left, flame in the left hand ». Le modèle a eu raison contre le texte.
- Les arcs d'attaque (images 4-5) font 190 px de large : à la limite de la cellule, pas au-delà.
- **16 septembre, E1 et E2 pour les cinq PNJ.** Références rendues dans le dossier de travail
  (portrait, moitié droite, figurine recadrée à la main, bande d'échelle, ancre de style) ;
  `style.md` v1 recopié tel quel de la page ; `prompt_b.txt` rédigés. Deux scripts s'ajoutent au
  dossier de travail pour ne pas recopier les blocs du protocole : `tour.py <slug> <K> [meilleur]`
  (§5 et §8, journalise aussi `usage` rendu par l'API) et `mesure.py <candidat> [comptes]` (§6,
  qui ignore les miettes de moins de 8 lignes ; `ndarray.ptp()` n'existe plus sous NumPy 2).
  `mesure.py` redonne sur le candidat ChatGPT les chiffres notés ci-dessus (hauteur lue 144).
- **Jetons du corpus abandonnés.** L'appariement à l'œil a donné Anariel = `Token 118` (sûr),
  Nakral = `Token 77` (probable), Jade = `Token 108` (incertain), et rien pour Xorius ni Lizz ;
  la quantification d'un jeton rendait surtout son fond et son biseau. **Décision du
  16 septembre** : ne pas utiliser les jetons, faire générer le portrait en pixel art au style
  de la maquette (§2.2), puis en tirer la palette (§2.3). L'appariement des jetons sort de T1.
  Les palettes des blocs B actuels, choisies à l'œil sur les portraits peints, sont provisoires :
  `palette.py` les remplace.
- **Lizz** n'a pas de jambes : le bloc B dit que la queue repose sur la ligne et que `walk` est
  une reptation. La mesure « pieds à ± 2 px » lira la queue.
- **Tour 2 d'Anariel** : un bloc C avait été préparé (image 6 d'`idle`, image 8 de `walk`, réduction à ~110 px) ; abandonné avec la correction par édition.
- **16 septembre, étape R ajoutée** (demande de l'utilisateur). La maquette n'a pas le style final
  (fond coloré, légendes, grille non tenue) : Anariel est d'abord produite au format exact et
  devient l'ancre des quatre autres. Scripts ajoutés : `normalise.py`, `normalise_portrait.py`,
  `mesure.py --grille`. `portrait.py` lit désormais `ref1_portrait_style.png` ; la maquette et son
  buste sont rangés dans `maquette\`.
- **Main d'arme corrigée dans `style.md` pendant R** : « the weapon is held in the character's
  right hand, which is on the viewer's left », conforme à la maquette.
- **Essai de `normalise.py` sur le candidat ChatGPT** (comptes forcés à 5 / 7 / 8 / 4 / 6 / 8) :
  38 images placées, pas réaligné. Centrées sur leurs pieds, trois images sortent de leur cellule
  de 192 px : `attack` 4 (190 px de large) et 5 (188), `cast` 6 (170, trait de flamme). À 144 px
  de haut, l'arc ne tient pas ; à la hauteur visée (~110 px) il devrait rentrer. Sinon, le bloc A
  doit borner les effets (« effects extend at most 70 px beyond the body »).
- **16 septembre, mode interface.** Faute de crédit API, les générations passent par l'interface
  ChatGPT, à la main : pour chaque appel, le dossier de travail prépare `chatgpt\<étape>\` avec
  le prompt exact (`prompt.txt`) et les pièces jointes numérotées dans l'ordre des références.
  Un seul candidat par envoi, ni `n`, ni `size`, ni `background` réglables : on redemande pour
  un deuxième candidat, et le coût n'est pas mesuré. Les mesures et la normalisation, elles,
  restent les mêmes.
- **16 septembre, R1 : portrait d'Anariel, candidat 1 (interface) retenu.** Rendu en 1254 × 1254,
  opaque, fond navy (coin haut droit `#0B1725`), environ 200 000 couleurs : c'est une peinture en
  gros pixels, pas du pixel art. Pas lu par autocorrélation : **≈ 12 px**, irrégulier, au lieu des
  8 demandés. Revue : visage, chevelure blanche, diadème d'or à gemme rouge, armure dorée et cape
  cramoisie conformes ; rapière enflammée dans la main droite, à gauche du spectateur ; flamme dans
  la main gauche ; aucun texte. Écart au cadrage : buste élargi jusqu'aux mains, accepté.
- **Normalisation du portrait réglée sur ce candidat.** Plus proche voisin sur 128 : points gris
  parasites sur le visage ; moyenne par bloc : couleurs délavées. Retenu : **médiane de chaque
  bloc**, puis quantification octree + k-means **sans tramage**. À 16 couleurs (coupe médiane),
  l'or et les flammes tournaient au brun : le plafond passe à **32 couleurs**, dans
  `portrait_style.md` aussi (R n'est pas gelé). Même cause dans `palette.py` : octree + k-means au
  lieu de la coupe médiane, qui noyait le cramoisi. Palette d'Anariel tirée du portrait :
  `#D3AC93, #F7E7C9, #9D5A29, #581C20, #E89633, #F9DD6A, #F26823` ; le bleu nuit des gantelets n'y
  entre pas (sept couleurs).
- **16 septembre, R2 tour 1 (interface, candidat 1).** Mesures : 1 536 × 1 024 ; halo brun
  semi-transparent (aucun pixel à 255, 259 k sous 32), propre après binarisation ; rangées 6 ;
  images **6 / 8 / 7 / 4 / 6 / 8** (`attack` n'en a que 7) ; pieds à ± 2 px sauf `death` (5) ;
  pas 2 ; hauteur **142 px**. Revue : 1-7 oui, 9 oui ; **8 non** (arcs d'`attack` de 208 à
  264 px, au-delà de la cellule). La grille de 8 colonnes est cette fois tenue à peu près.
- **L'échelle était fausse dans le prompt, pas seulement ignorée.** Les héros du Colisée font
  **45 pixels d'art** de haut (44 pour Darin) et 16 à 22 de large, pieds à y = 63 dans leur
  cellule 48 × 64 ; la bande d'échelle à 2 px les montre donc à 90 px, et le bloc A disait
  « about 110 px ». Corrigé en « about 90 px » dans `style.md` (R n'est pas gelé), et la cible
  de `mesure.py` avec. Deux planches sur deux sont sorties à ~142 px malgré la bande :
  **`normalise.py --hauteur 45`** ramène désormais le personnage à la taille des héros, même
  facteur pour toutes les images (0,316 ici), au plus proche voisin. À 45 px, Anariel reste
  lisible (visage, diadème, rapière et flamme).
- **Les effets ne tiennent pas dans une bande de 48.** À 45 px de haut, 7 images dépassent
  48 pixels d'art : `attack` 3 à 5 (66, 83, 68), `death` 5-6 couchée (51), `cast` 6 et 8 (55, 57).
  C'est une question de format, pas de génération : à trancher avant T4.
- **Tour 2 préparé** (`anariel/tour2r/bloc_c.txt`) : la huitième image d'`attack` seule.
- **16 septembre, R2 tour 2 (interface) : échec.** Le bloc C demandait la seule 8e image
  d'`attack` ; l'interface a **régénéré toute la planche** (hauteur 147 px, légèrement différente)
  et `attack` a toujours 7 images, dont les arcs 3 à 5 se touchent au point que la découpe n'en
  lit que 6. Le correctif par édition ne corrige pas un compte d'images dans l'interface.
- **Décision de l'utilisateur, 16 septembre : cellules de 96 × 64 pour `attack` et `cast`**
  (option 1 sur trois : cellules élargies, calque d'effets séparé, effets contenus). Conséquence
  tirée pour la génération : à ~145 px de haut, huit images avec leurs arcs ne tiennent pas dans
  1 536 px, d'où le compte raté deux fois ; un PNJ se génère désormais en **deux planches**,
  `corps` (8 × 4 cellules de 192 × 256) et `effets` (4 × 4 cellules de 384 × 256). `style.md` perd
  sa grille et ses rangées au profit de `disposition_corps.md` et `disposition_effets.md` ; les
  blocs B ne parlent plus de numéros de rangée ; `tour.py` et `normalise.py` prennent la
  disposition en argument ; l'ancienne planche unique est archivée dans `chatgpt\_archives\`.
- **Essai de la nouvelle normalisation** sur les images du tour 1, remontées aux deux
  dispositions : 40 images, bandes 6 / 8 / 4 / 6 × 48 et 8 / 8 × 96. Avec l'ancre des effets au
  centre (48), l'arc d'`attack` 4 (83 px de large, pieds à 19) sortait de 16 px : **ancre des
  effets à x = 32**, qui lui laisse 64 px à droite, et tout tient. Au corps, 1 px rogné à `walk` 6
  (pointe de rapière) et 3-4 px à `death` 5-6 (corps couché de 51 px) : tolérés, mais une morte
  couchée ne tient pas dans 48 à cette hauteur.
- **Pour T5** : les bandes d'effets ont leur ancre à x = 32 sur 96, pas au centre ; le miroir (PNJ
  tourné à gauche) la met à x = 64. Le catalogue d'apparence doit porter la largeur de cellule et
  l'ancre par animation.
- **Décisions de l'utilisateur, 16 septembre** : (1) **`death` en 96 × 64** aussi, pieds au centre
  (x = 48) ; la rangée `death` de la planche `corps` passe à 6 cellules de 256 px. (2) **Corriger le
  prompt initial plutôt que la planche** : fini le bloc C et l'édition, chaque tour est une
  génération complète (§4 R2, §8), pour que le prompt se reproduise sur les autres PNJ.
- **Ce que les deux premiers tours d'Anariel ont fait entrer dans le prompt initial** :
  - compte d'images : « EXACTLY N separate figures », au moins 16 px de vide entre deux figures,
    rien ne touche la cellule voisine (les arcs collés faisaient lire 6 ou 7 images au lieu de 8) ;
  - `walk` : cycle « contact, passing », corps qui balance, « must not look like the idle row »
    (les huit images du tour 1 ressemblaient à `idle`) ;
  - effets : pieds au tiers gauche de la cellule d'`effets`, effet au plus à 1,4 fois la hauteur du
    personnage vers la droite et 0,7 vers la gauche : c'est la bande de 96 à ancre 32 exprimée en
    hauteurs, pour qu'elle tienne quelle que soit l'échelle rendue ;
  - `death` : corps couché centré sur l'emplacement des pieds, au plus deux fois la hauteur debout ;
  - fond : « every pixel fully opaque or fully transparent, no soft halo » (les deux planches
    avaient un halo brun semi-transparent, sans aucun pixel à 255).
- **Essai de normalisation** sur les images du tour 1 remontées aux nouvelles dispositions : code 0
  sur les deux planches ; `death` tient dans 96 ; seul 1 px rogné à `walk` 6.
- **16 septembre, R2 tour 1 aux deux planches (interface, un candidat chacune).**
  - `corps` : 1 536 × 1 024, 4 rangées, **6 / 8 / 4 / 6**, pieds à ± 2 px (`death` couchée à part),
    pas 2, garde à **178 px**. `normalise.py --hauteur 45` : **code 0**, rien de rogné.
  - `effets` : 4 rangées de 4, pieds à ± 2 px, pas 2, garde (`attack` 8) à **200 px**.
    `mesure.py` lit 5 images en rangée 4 : les étincelles détachées de `cast` 7 font un morceau
    sans pixel au sol. `normalise.py` : **code 0** après correction de la lecture.
  - Le halo brun semi-transparent est toujours là malgré « no soft halo » (aucun pixel à 255) :
    la consigne ne sert à rien dans l'interface ; la binarisation suffit.
- **Trois corrections de `normalise.py` apprises sur ces planches** :
  - un morceau sans aucun pixel au pied de sa rangée rejoint son voisin le plus proche
    (étincelles, éclats) ;
  - **un facteur par planche** : le générateur rend le personnage à 178 px sur l'une et 200 sur
    l'autre ; le repère est la pose de garde (`idle` au corps, `attack` 8 aux effets, que le prompt
    veut presque identique à `idle`). `--facteur-de` ne doit plus servir entre corps et effets ;
  - `death` : l'ancre suit la position dans la cellule du générateur, recalée sur les pieds de
    l'image 1 ; sans recalage l'entrée dans la mort sautait de 5 px, et à l'ancre « pieds » la chute
    glissait vers la gauche.
- **Revue des neuf questions.** `corps` : neuf « oui » ; `walk` alterne enfin les jambes.
  `effets` : **3 non** — pendant `attack` 2 à 6, la rapière **change de main** (main droite du
  personnage au repos, main gauche pour frapper vers la droite) et la flamme de la main libre
  disparaît ; 1, 2, 4 à 9 oui. Correction entrée dans `disposition_effets.md` : l'arme ne change
  jamais de main, la frappe vers la droite traverse le corps, la main libre garde ce qu'elle tient.
  Tour 2 des effets à générer (`chatgpt\R2-effets-tour2\`) ; le corps est retenu, sous réserve de R3.
- **16 septembre, R2 tour 2 des effets.** Revue : **la rapière ne change plus de main** et la
  flamme reste dans la main libre (`attack` 1-3 et 6-8 ; cachée par le bras en 4-5) : la correction
  du prompt a porté. Nouveau défaut : le rayon de `cast` 5 traverse la cellule et touche `cast` 6
  (question 8 : non). Garde rendue à ~210 px, pieds bien au tiers de la cellule : la règle
  « 1,4 fois la hauteur à droite » était juste pour la bande (64 / 45) mais **fausse pour la
  cellule du générateur** (256 / 210 ≈ 1,2), puisque le générateur ne rend pas l'échelle demandée.
  Corrigé : 1,2 fois (et 0,6 à gauche), l'effet finit « well before » le bord, et `cast` 5-6 est
  une « short burst », pas un long rayon. Tour 3 des effets : `chatgpt\R2-effets-tour3\`.
- **Lecture des images, `normalise.py`** : des éclats flottant entre deux rangées faisaient lire
  5 rangées ; on revient alors à la grille de 256 px. Quand un effet touche la figure voisine, la
  rangée est recoupée à partir des pieds (groupes de colonnes au sol, fondus au plus proche jusqu'au
  compte attendu, coupe au milieu du plus long vide). Les planches déjà lues redonnent le même
  résultat (code 0).
- **16 septembre, corps et effets en deux planches : abandonné.** L'utilisateur relève que deux
  prompts font diverger le style. Vérifié sur la pose de garde : corps (178 px) et effets des tours
  1 et 2 (200 et 213 px) n'ont ni les mêmes proportions, ni les mêmes jambières (grises ou sombres),
  ni le même diadème. La contrainte de 1 536 × 1 024 venait de `gpt-image-1`, **pas du besoin** :
  depuis `gpt-image-2`, l'API accepte des tailles libres. Retour à **une planche**, 1 536 × 2 048,
  qui empile les deux dispositions validées (corps en rangées 1-4, effets en 5-8), avec une
  consigne explicite « same character, same size, in every frame of every row ». `tour.py` passe
  par défaut à `gpt-image-2` et à la disposition `planche` (à vérifier sur la doc du jour :
  `background="transparent"` avec ce modèle) ; `normalise.py planche` rend les six bandes en un
  passage, un seul facteur (essai sur les deux planches du tour 1 empilées : code 0, 4 px rognés à
  `cast` 6). Les envois séparés sont archivés dans `chatgpt\_archives\`.
- **16 septembre, R2 tour 3 : planche unique (interface) — retenue.** L'interface a rendu
  **1 086 × 1 448** au lieu des 1 536 × 2 048 demandés (même rapport 3:4, surface divisée par deux) :
  elle ne suit pas la taille. `normalise.py` ramène désormais la grille du prompt à la taille reçue
  (sans quoi l'ancre de `death`, calculée sur la grille, sortait de 21 à 96 px). Mesures : 8 rangées,
  **6 / 8 / 4 / 6 / 4 / 4 / 4 / 4**, pieds à ± 3 px, pas 2, garde à 145 px ; un éclat isolé de
  `cast` 7 rejoint son image ; `normalise.py planche --hauteur 45` : **code 0**, rien de rogné.
  Halo rouge semi-transparent autour des figures, propre après binarisation.
  Revue : **neuf « oui »**. Surtout, **le personnage est le même d'un bout à l'autre** : taille,
  diadème, jambières, cape, rapière et flamme identiques du repos au sort ; la rapière ne change
  pas de main ; les arcs d'`attack` et les gerbes de `cast` tiennent dans la bande de 96 ; `death`
  couchée tient dans 96. Réserve : `walk` reste proche d'`idle` à 45 px, à juger en jeu (R3).
  Planche et bandes copiées dans `ancre\anariel\`. Reste R3 avant la bascule R4.
- **16 septembre, `walk` précisé par l'utilisateur.** Les jambes doivent bouger d'une image à
  l'autre : jambe gauche à gauche, au centre, à droite, au centre, à gauche…, la droite à
  l'inverse. Écrit tel quel, image par image, dans `disposition_planche.md` (et `_corps`) : un pas
  complet en 4 images, deux fois sur les 8. Tour 4 de la planche : `chatgpt\R2-planche-tour4\`.
- **16 septembre, `walk` : cycle classique en 8 poses** (proposé, validé par l'utilisateur), à la
  place de « gauche, centre, droite, centre » qui ne donnait que deux poses répétées : contact,
  réception (corps au plus bas), passage, élan (corps au plus haut), puis la même chose sur l'autre
  jambe ; le bras libre balance à l'opposé de la jambe de son côté, le bras armé reste en garde.
  Écrit image par image dans les dispositions ; `chatgpt\R2-planche-tour4\prompt.txt` régénéré.
- **16 septembre, planche d'Anariel, tour 4 (interface) : rejeté, `walk` toujours figée.**
  `anariel\tour4\planche\candidat1.png` (1086 × 1448). Mesure : 8 rangées, comptes
  **6 / 8 / 4 / 6 / 4 / 4 / 4 / 5** (l'éclat de `cast` rejoint son image à la normalisation),
  pieds à ± 2 px, pas 2, garde à 141 px ; `normalise.py planche --hauteur 45` : **code 0**, 40
  images, rien de rogné. Mécaniquement équivalent au tour 3. Mais les huit images de `walk` sont
  la pose de garde répétée : jambes plantées au même écart, aucun genou levé, seul le balancement
  de la cape et des cheveux change (différence entre images consécutives de 118 à 312 pixels
  d'alpha, du même ordre qu'au tour 3). Le personnage y est aussi dessiné plus petit que la garde
  (41-42 px d'art au lieu de 45). Le cycle en 8 poses décrit par la mécanique (contact, réception,
  passage, élan) n'a pas été lu : deux tours de suite, le générateur recopie `idle`.
- **16 septembre, tour 5 préparé : `walk` par silhouettes.** Plutôt que la mécanique du pas, le rang
  décrit quatre silhouettes de jambes franches, chacune deux fois en miroir : STRIDE (grand écart
  avant-arrière), CROUCH (corps au plus bas), KNEE UP (pieds joints, genou levé haut, cuisse
  horizontale), PUSH (sur la pointe, corps au plus haut) ; « the ONLY row where the legs move »,
  hauteur de la garde imposée. Réécrit dans `disposition_planche.md` et `disposition_corps.md` ;
  `chatgpt\R2-planche-tour5\` prêt (mêmes cinq références). Si le tour 5 échoue encore, la voie
  suivante n'est plus le prompt : générer le rang `walk` seul (1536 × 256) avec la planche du tour 4
  en référence 1, le mécanisme prévu pour R4, ou donner une sixième référence de poses de marche.
- **16 septembre, planche d'Anariel, tour 5 (interface) : rejeté, `walk` encore en garde.**
  `anariel\tour5\planche\candidat1.png`. Mesure brute : comptes 6 / **6** / 4 / 6 / 4 / 5 / 4 / 6 (les
  trois premières figures de `walk` se touchent par leur halo, un éclat à `attack` 6 et deux à
  `cast`) ; `normalise.py planche --hauteur 45` : code 0, 40 images, 1 px rogné à `walk` 6, mais
  `walk` 5-6 se chevauchent dans la source et la bande 6 récupère une flamme et un pied détachés.
  Les silhouettes ont été à moitié lues : accroupissement aux images 2 et 6, genou esquissé à
  la 7, hauteur qui varie de 40 à 46 px ; tout le reste est la garde à pieds écartés. Aucune
  image STRIDE ni KNEE UP. Trois tours de prompt sur `walk` (3, 4, 5) : la voie du prompt seul
  est épuisée pour ce rang. La planche du **tour 4** reste la meilleure référence corps + effets.
- **16 septembre, tour 6 préparé : le rang `walk` seul, planche du tour 4 en référence 1.**
  Nouvelle disposition `disposition_marche.md` (1536 × 256, 8 cellules de 192, mêmes huit
  silhouettes) : la référence 1 est `anariel\tour4\norm\planche.png`, la planche normalisée au format
  exact, avec la consigne de copier le personnage et non ses poses. `normalise.py` et `tour.py`
  connaissent la disposition `marche` (bande `walk` 8 × 48). Dossier `chatgpt\R2-walk-tour6\`
  prêt, à enregistrer sous `anariel\tour6\marche\candidat1.png`. C'est le mécanisme prévu pour R4
  (la planche d'Anariel comme référence de style), essayé un tour plus tôt. Si la marche
  tient, la planche finale = corps et effets du tour 4 + rang `walk` du tour 6, assemblés à la
  normalisation ; à consigner comme entorse à « une planche, un appel ».
- **16 septembre, marche d'Anariel, tour 6 (interface, rang seul) : retenue.**
  `anariel\tour6\marche\candidat1.png`, rendu en 2098 × 750 (l'interface ignore 1536 × 256) ; garde à
  303 px, facteur 0,149. Mesure : 1 rangée, **8 images**, pieds à 0 px d'écart, aucune figure qui
  se touche ; `normalise.py marche --hauteur 45` : code 0, une seule composante par image (un
  pixel isolé à `walk` 8). Les huit silhouettes demandées sont là, dans l'ordre : grand pas,
  accroupi, genou levé, élan, puis en miroir ; le personnage est celui de la planche (visage,
  diadème, armure, cape, rapière dans la même main, flamme). Réserve : le corps varie de **37 à
  49 px d'art** (accroupi très bas, élan très haut) là où la disposition demandait quelques pixels ;
  à juger en jeu (R3), la marche cadencée à 0,12 s peut le rendre sautillant. La planche de style
  en référence 1 a fait ce que trois tours de prompt n'ont pas fait : c'est le mécanisme R4.
  Planche composite écrite dans `anariel\tour6\norm\planche.png` (tour 4 rangées 1 et 3-8, tour 6
  rangée 2) avec ses bandes et `planche.json` (sources et facteurs) ; **entorse assumée à
  « une planche, un appel »** : le rang `walk` vient d'un second appel ancré sur la planche.
  `ancre\anariel\` tient toujours le tour 3 : à remplacer par le composite si R3 l'accepte.
  R3 reste à faire : les bandes ont 6 et 8 images (les héros du Colisée en ont 5), le manifeste
  du Colisée est généré par `extract_coliseum_atlas.py` et ne connaît pas `anariel` ; voir §9.
- **16 septembre, R3 : Anariel branchée au Colisée à la place de Kaelith.** Les six bandes du
  composite (tour 4 + `walk` du tour 6) et leurs `.anim.json` (durées de la table du jeu
  d'animations) sont dans `Source/Elements/Assets/Npc/anariel/`, avec `Npc/manifest.json`
  (`npcs`, `replaces : kaelith_voss → anariel`) et un README. Le dossier `Coliseum/` n'est pas
  touché. Côté moteur : `ArenaAppearanceCatalog` porte un dossier par héros (`sheetDirectory`,
  `replaceHero`, `applyNpcManifest`) ; `ArenaSceneRenderer` lit `Assets/Npc/manifest.json` à côté
  du Colisée ; `ArenaTexture::frameWidth` vient du `frameWidth` de chaque `.anim.json` et le
  composeur en déduit le nombre d'images (6 au repos, 6 à terre sur 96 px) au lieu du compte fixe
  du manifeste. Trois tests ajoutés, 1 176 tests unitaires verts. Capture hors écran (rendu QRhi
  réel, `anariel\tour6\r3-colisee.png`) : Anariel debout est lisible, cape, flamme et rapière
  reconnaissables à l'échelle du Colisée, un peu plus haute et plus large que les héros de la
  planche (45 px d'art contre ~40) ; à terre, l'image 6 de `death` s'affiche entière sur sa
  cellule de 96 px. Le jeu lancé sur l'écran Arène journalise « 1 heros remplace(s) par un PNJ ».
  Reste : le jugement à l'écran par l'utilisateur (échelle, lisibilité, marche non visible car la
  scène ne joue que `idle`/`death`), et la bascule de l'ancre sur le composite (R4).
- **16 septembre, Anariel acceptée par l'utilisateur ; R4 faite, méthode standard écrite (§4bis).**
  `compose.py` rejoue l'assemblage fait à la main : `compose.py anariel 4 6` redonne le composite
  **au pixel près** (planche et six bandes identiques à celles de `Npc/anariel/`). L'ancre du tour 3
  est rangée dans `ancre\_tour3\`, la maquette et le buste dans `maquette\`. Gel de la v1 avec les
  corrections du §4 (autre personnage en référence 1, main d'arme lue dans `WEAPON`, `{MARCHE}`).
  `prompts.py` assemble prompt et références pour l'API comme pour l'interface ; `chatgpt.py` prépare
  un envoi par PNJ et par étape. Bloc B de Xorius précisé : `ATTACK:` devient une rubrique, sans
  quoi elle partait dans le prompt du portrait. Premier envoi groupé préparé :
  `chatgpt\portrait-tour1\{xorius,lizz,nakral,jade}\`.
- **16 septembre, étape P des quatre PNJ (interface, un envoi chacun) : quatre portraits retenus au
  premier tour.** Rendus en 1 254 × 1 254, opaques, fond navy ; normalisés (`<slug>	oken\portrait.png`,
  32 couleurs). Revue : Xorius (capuche violette, yeux ambrés, arc doré en main gauche, carquois,
  trait violet), Lizz (chevelure de serpents, écailles d'or, griffes, queue), Nakral (capuche vide à
  lueur violette, plates à pointes et crânes, lame cramoisie en main droite), Jade (cheveux à mèches
  violettes, bijou au front, collier d'or, luth, notes turquoise) : conformes au portrait peint et à
  la figurine. Le premier envoi de Jade a rendu un fichier de 190 octets, un message d'erreur
  réseau d'OpenAI enregistré en `.png` : **vérifier la taille du fichier avant de normaliser**.
- **`palette.py` : accents ajoutés.** Les sept couleurs les plus étendues perdaient ce qui fait le
  personnage sans couvrir de surface : la lame cramoisie de Nakral, la magie violette de Xorius, les
  notes turquoise de Jade (vérifié : absentes des sept, présentes dans le portrait). Or `PALETTE` dit
  « use only these ». Le script ajoute au plus trois accents : couleurs vives (saturation > 0,45) à
  plus de 90 des sept, couvrant au moins 0,5 % du personnage ; la ligne `PALETTE` les nomme
  « accent colours (weapon, magic, effects) ». Nakral : `#EF5D52, #D32F31` ; Jade : trois verts ;
  Xorius : `#A456DE` et deux tons du bois de l'arc ; Lizz : aucun, sa palette tenait déjà. La palette
  d'Anariel n'est pas recalculée (ancre gelée). Envoi S préparé : `chatgpt\planche-tour1\`.
- **16 septembre, Jade a un `cast` (décision de l'utilisateur).** En tant que barde, elle lance ses
  sorts en jouant : les deux mains au luth, des notes turquoise qui tournent autour de l'instrument
  (rangée 7), une vague de notes courte vers la droite puis retour à la garde (rangée 8). Écrit dans
  sa ligne `ANIMATION SET`, qui précise la consigne générique « gather energy in the free hand » ;
  plus de `--vides` pour elle. Envoi `chatgpt\planche-tour1\jade\` régénéré.
- **16 septembre, étape S des quatre PNJ, tour 1 (interface, un candidat chacun).** Tous rendus en
  1 086 × 1 448, halo semi-transparent propre après binarisation, pas 2, garde de 118 à 146 px.
  **Lizz retenue** (code 0, neuf « oui »). **Xorius** : la flèche d'`attack` 4-5 traverse la cellule
  (code 1). **Jade** : `hit` 3 images, `death` 5 (code 1) ; le reste est le meilleur des quatre.
  **Nakral** : mécaniquement bon, mais la revue voit l'épée changer de main et se dédoubler pendant
  la frappe, le défaut du tour 1 des effets d'Anariel, alors que la disposition l'interdit
  (« the weapon never changes hand ») : la consigne générique ne suffit pas pour une arme à deux
  mains tenue haute. Rejet au tour 1 : **3 sur 4**.
- **Tour 2, blocs B précisés (§8)** : Xorius, « SHORT violet arrow streak… no longer than his height » ;
  Nakral, « only ONE sword… the fiery arc is the trail of that single blade ». Jade : même prompt, le
  défaut est un compte. Envois `chatgpt\planche-tour2\{xorius,nakral,jade}\` et
  `chatgpt\marche-tour2\lizz\` (référence 1 : `lizz\tour1\norm\planche.png`).
- **Pour `style.md` v2** : si la main d'arme dérive encore au tour 2 de Nakral, la phrase « only ONE
  weapon; effects are the trail of that weapon » passe dans la disposition, pour tous.
- **16 septembre, étape S, tour 2 : trois rejets.** `hit` rendu à **3 images** sur les trois planches
  (Xorius, Nakral, Jade), comme au tour 1 de Jade : **4 planches sur 7** hors Anariel. Défaut qui
  revient d'un tour et d'un PNJ à l'autre : c'est la disposition, pas le PNJ (§8). Nakral a en plus
  un `idle` à 5 ; Xorius une mort couchée de 107 px d'art, plus longue que la bande de 96.
- **`normalise.py` rendait un faux 0.** Quand le compte manque, la relecture par les pieds prenait
  les deux pieds écartés d'une même figure pour deux images et coupait la figure (images de 7 à 15 px
  d'art). Une coupe qui produit un morceau de moins de la moitié de la largeur médiane est désormais
  refusée : Nakral et Jade passent à code 1. Les planches acceptées (Anariel tours 3, 4, 6 ; Lizz ;
  Nakral tour 1) redonnent code 0 et des sorties identiques. **Pour T4** : un code 0 ne dispense pas
  de compter à l'œil tant que cette lecture n'est pas testée.
- Marche de Lizz (M 2) : pas encore rendue.
- **Décision de l'utilisateur, 16 septembre : disposition v2 tout de suite**, sans tour 3 au prompt
  gelé, puisque le défaut est commun. `disposition_planche.md` v2 (la v1 est dans `_v1\`) : `hit`
  image par image (choc, recul, reprise, garde ; « four separate figures… never three ») ; `idle`
  image par image ; « only ONE weapon in each frame », l'arc est la traînée de cette arme (le
  dédoublement de Nakral persiste au tour 2 malgré son bloc B) ; mort couchée « cloak, hair and
  weapon included » dans deux hauteurs. `style.md` et `portrait_style.md` restent en v1. Les lignes de
  version (« LAYOUT v2 — … ») ne partent plus dans le prompt (`prompts.lire`). **Le verdict (§10)
  notera que la v1 a échoué sur `hit`** : les tours 3 de Xorius, Nakral et Jade mesurent la v2.
  Envois `chatgpt\planche-tour3\{xorius,nakral,jade}\` ; `chatgpt\marche-tour2\lizz\` régénéré (même
  prompt, sans ligne de version).
- **16 septembre, Lizz terminée sans étape M (décision de l'utilisateur).** Sa rangée `walk` du tour 1
  ondule nettement (queue en S d'un côté, de l'autre, ramassée, tendue) : rapport walk/idle **4,7**,
  au-dessus d'Anariel retenue (4,2). La marche devient conditionnelle (§4bis) ; `compose.py` accepte
  une planche seule ; l'envoi `chatgpt\marche-tour2\lizz\` est supprimé. Rapports des autres planches
  lues : Nakral t1 2,6, Xorius t1 1,7 et t2 3,8, Jade t2 6,0.
- **16 septembre, étape S, tour 3 (disposition v2).** **La v2 a porté sur `hit` et `idle`** : 4 et 6
  images sur les trois planches. Xorius et Jade retenus (code 0, revue sans « non ») ; leur marche
  bouge peu (2,7 et 3,7). Nakral : `idle`, `hit`, `death`, `cast` justes, mais l'épée dédoublée revient
  à `attack` 4-5 malgré « only ONE weapon » dans la disposition **et** dans son bloc B ; et sa rangée
  `walk`, épée tenue basse, touche ses voisines (code 1).
- **Décision de sortie du PoC (utilisateur, 16 septembre) : passe par animation** plutôt qu'emprunt
  d'images à une autre planche (§4bis, étape X). Outillée : `disposition_reprise.md`, `prompts.reprise`
  (lignes `Row` reprises de la disposition de planche), dispositions `idle|hit|death|attack|cast` dans
  `normalise.py` (facteur : image 1 pour `death`, `attack` 8 pour `attack`), `compose.py <slug> <S>
  [<M>] [anim=tour]`, qui écarte les erreurs de la planche portant sur une animation remplacée. Essai
  à blanc : les rangées `attack` d'Anariel tour 4, recoupées, normalisées seules et recomposées :
  code 0 ; Anariel se recompose toujours à l'identique sans passe.
- Envois du tour 4 : `chatgpt\attack-tour4\nakral\`, `chatgpt\marche-tour4\{xorius,nakral,jade}\`.
- **16 septembre, tour 4 : passes X et M.** **La passe par animation a corrigé l'épée de Nakral du
  premier coup** : une seule lame sur les huit images d'`attack`, là où trois planches entières avaient
  échoué. L'image 5 (lame tendue, arc de feu) sort de la bande de 5 px, 1 de plus que la tolérance :
  code 1, même prompt au tour 5. Marches de Xorius et de Jade : huit poses nettes, même personnage,
  échelle tenue (hauteurs 44-46 et 38-50 px d'art, garde à 45) ; **Xorius et Jade terminés**. Marche de
  Nakral : jambes justes, mais l'épée est **levée**, comme aux tours 1-2, alors que sa planche du
  tour 3 la tient basse. La référence 1 (sa planche normalisée) avait la rangée `walk` illisible, donc
  vide : le générateur a repris son idée de l'épée plutôt que la planche. Bloc B précisé : épée tenue
  basse, pointe vers le bas à gauche, levée seulement pour frapper. Envois
  `chatgpt\{marche,attack}-tour5\nakral\`.
- **Rapport walk/idle après la marche** : Xorius 3,1 malgré des poses nettes (l'arc, qui dépasse la tête,
  pèse dans l'écart d'`idle`). Le seuil de 4 sert à décider s'il faut l'étape M sur une planche, pas à
  juger une marche déjà refaite : elle se juge à l'œil.
- **Bilan provisoire** : Anariel (ancre), Lizz (S 1), Xorius (S 3 + M 4), Jade (S 3 + M 4) terminés ;
  Nakral en passes au tour 5.
- **16 septembre, tour 5 de Nakral : retenu.** Passe `attack` : une seule épée, arc dans la bande
  (code 0). Marche : épée tenue basse comme sur la planche, grâce à la précision du bloc B. **Les cinq
  PNJ sont produits.** Bandes et `.anim.json` copiés dans `Source/Elements/Assets/Npc/` ; manifeste :
  `kaelith_voss` → `anariel`, `elira` → `jade`, `darin` → `xorius`, `bram` → `nakral` (Lizz n'a pas
  de héros à remplacer). Aperçu à l'échelle : même taille entre eux, lisibles ; plus massifs que les
  héros du Colisée, plus fins. Jugement à l'écran (étape J) : à l'utilisateur.

### Verdict (16 septembre)

Appels d'image par PNJ, portrait compris (interface, un candidat par envoi) :

| PNJ | Portrait | Planche | Marche | Passes | Total |
|---|---|---|---|---|---|
| `lizz` | 1 | 1 | — | — | **2** |
| `xorius` | 1 | 3 | 1 | — | **5** |
| `jade` | 1 (+1 envoi perdu, erreur réseau) | 3 | 1 | — | **5** |
| `nakral` | 1 | 3 | 2 | 2 (`attack`) | **8** |

| Chiffre (§10) | Seuil | Mesuré | Lecture |
|---|---|---|---|
| Fond transparent exploitable | 4 sur 4 | **4 sur 4** | Toujours un halo semi-transparent, jamais un pixel à 255 : la binarisation à 127 suffit ; la consigne « no soft halo » est sans effet et peut sortir du style. Pas de repli chroma. |
| Rejet mécanique au tour 1 | ≤ 2 sur 3 | **3 planches sur 4** (un candidat par envoi) | Non atteint en v1 : `hit` à 3 images et arme dédoublée venaient de la disposition. Corrigé en v2 (décision de l'utilisateur) ; au tour 3, `hit` et `idle` justes sur 3 sur 3. |
| PNJ retenus en ≤ 3 tours | ≥ 3 sur 4 | **4 sur 4** pour la planche ; Nakral avec deux passes de plus | Atteint, en comptant les passes à part. |
| Pas de pixel entier et net | 4 sur 4 | **4 sur 4** (pas 2 en tête partout) | Le générateur ne tient pas l'échelle demandée (garde de 118 à 167 px au lieu de 90) : `--hauteur 45` est indispensable. |
| Coût | à noter | **5 appels en moyenne** (2 à 8), non chiffré en euros | Mode interface : le coût réel se mesurera par l'API. |

**Décisions de sortie, qui font la méthode standard (§4bis) :**
1. **Portrait avant la planche**, palette tirée du portrait (sept couleurs et au plus trois accents).
2. **Une planche, un appel**, disposition **v2** ; l'ancre est la planche d'Anariel, présentée comme un
   autre personnage.
3. **Marche conditionnelle** : second appel si `walk/idle` < 4 ou si l'œil la juge figée.
4. **Une animation fausse se refait seule** (passe par animation), jamais par emprunt d'images d'une
   autre planche ; au-delà de deux passes, la planche entière est à refaire.
5. **On corrige le prompt, jamais l'image** : disposition si le défaut est commun, bloc B s'il est
   propre au PNJ.
6. **Un code 0 ne dispense pas de l'œil** : `normalise.py` a rendu un faux 0 (corrigé), et la revue du
   §7 a trouvé seule le dédoublement de l'épée.

**Pour T1 à T5** : `prompts.py`, `chatgpt.py`, `normalise.py`, `compose.py` et `palette.py` sont les
prototypes à reprendre ; la lecture par profil de `normalise.py` doit recevoir des tests (les planches
de ce journal en sont les cas) avant d'être branchée dans une boucle automatique.
- **16 septembre, après le verdict (décision de l'utilisateur).** Le PoC prouve la méthode PNJ par PNJ,
  avec trop de choix au cas par cas pour coder une chaîne automatique : la suite fait confiance à la
  méthode, **sans essai en jeu de chaque modèle**. L'[epic](../../lots/LOT-91-atelier-pnj.md) est réécrit autour de la méthode
  (étapes E, P, S, M, X, A, I) et de la liste des 160 fiches (nom, page, case cochée à l'intégration).
  L'atelier entre dans le dépôt, sous `atelier/` : prompts (en `.txt`, Doxygen lirait des `.md`),
  ancres, scripts et références par PNJ (`prompt_b.txt`, `portrait.png`, `palette.txt`) ; le dossier
  de travail garde les rendus du corpus et les tours. `integre.py` écrit les bandes, les `.anim.json`
  et le portrait pixel art dans `Source/Elements/Assets/Npc/<slug>/`, ajoute le PNJ au manifeste et
  coche sa ligne ; rejoué sur les cinq PNJ du pilote, il redonne les mêmes fichiers.

