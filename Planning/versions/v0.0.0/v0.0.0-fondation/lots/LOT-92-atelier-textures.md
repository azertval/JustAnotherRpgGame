+++
id = "LOT-92"
titre = "Atelier des textures : le style de la scène par maquette, une planche par lieu"
version = "0.0.0"
filiere = "assets"
statut = "livre"
taille = "L"
resume = "La scène a un style écrit, fixé par une maquette approuvée, et une planche de textures se commande par lieu depuis sa seule fiche d'atlas."
prerequis = ["LOT-50", "LOT-91", "LOT-39", "LOT-37"]
livrables = [
  "La Capitale et ses douze quartiers dans l'atlas (`scripts/sourcebook/atlas.py`, 94 → 107 lieux), filigranes retirés de treize fiches.",
  "La maquette approuvée `atelier/ancres/maquette.png` et le bloc A `atelier/prompts/style.txt`.",
  "`scripts/extract_texture_sheet.py` (dispositions JSON, gabarit, bloc C, découpe par pièces, contrôle d'orientation, miroirs, palette de 64 couleurs, manifeste, `--check`) et ses tests `scripts/tests/test_extract_texture_sheet.py`.",
  "Les dispositions `atelier/dispositions/<id>.json` et le modèle `atelier/dispositions/modeles/quartier.json`, validés par `check_assets_brief.py`.",
  "Deux planches installées : `Source/Elements/Assets/Scene/coliseum/` (36 textures, dessinées par `hmi::composeArenaScene`) et `Source/Elements/Assets/Scene/martpart/` (22 textures).",
  "La spécification des deux identités : `EX-VIS-008` et `EX-VIS-009` dans `vision.md` ; `EX-IHM-070` et `EX-ARCH-022` précisées.",
]
criteres = [
  "T0 — `sourcebook atlas` régénère sans diff hors les treize fiches de la Capitale ; `test_atlas` vert.",
  "T1 — **l'auteur approuve** la maquette ; le risque de l'angle est tranché.",
  "T2 — le bloc A est relu par l'auteur.",
  "T3 — `extract_texture_sheet.py --check` reproduit le dossier ; `check_assets_brief.py` vert.",
  "T4 — la planche du Colisée est découpée par le script ; l'arène se dessine avec.",
  "T5 — l'envoi de Martpart est préparé sans rédaction à la main ; spécification relue.",
]
sources = ["Tanares Sourcebook, p. 98-99 (encart « The Capital City », région Central Empire)"]
+++

## Pourquoi

Les figurines ont une méthode (`LOT-91`), pas les textures. Ce lot donne à la scène un **style
écrit**, fixé par une maquette approuvée par l'auteur, puis une méthode pour commander une planche
de textures **par lieu**, depuis le descriptif du lieu dans l'atlas. Première planche : le Colisée
final du `LOT-09`, qui remplace celle du `LOT-50`.

## Périmètre

### Ce que ce lot livre

- **La Capitale dans l'atlas** : la Capitale et ses douze quartiers, lus dans l'encart du
  Sourcebook ; 94 → 107 lieux ; filigranes retirés des treize fiches qui les portaient.
- **Le style de la scène, écrit** : la maquette approuvée (`atelier/ancres/maquette.png`) et le
  bloc A (`atelier/prompts/style.txt`), qui en relève trait, lumière et palettes de matière.
- **L'atelier** (`scripts/extract_texture_sheet.py`) : une disposition JSON déclare les cellules,
  et en déduit planches, gabarit, bloc C, découpe par pièces, contrôle d'orientation, miroirs,
  palette de 64 couleurs commune au lieu et manifeste ; `--check` reproduit l'installation sans
  rien écrire ; `check_assets_brief.py` valide les dispositions. Un lieu sans cellules propres
  nomme un **modèle** (`dispositions/modeles/quartier.json`) et se commande depuis sa seule fiche.
- **Deux planches installées** : le Colisée (36 textures, `Assets/Scene/coliseum/`), que l'arène
  dessine désormais, et Martpart (22 textures, `Assets/Scene/martpart/`).
- **La spécification des deux identités** : `EX-VIS-008` (la scène en pixel art isométrique) et
  `EX-VIS-009` (l'interface à la charte v2, et leur frontière), `EX-IHM-070` et `EX-ARCH-022`
  précisées en renvoi.

### Ce qui reste hors du lot, nommément

- **Au [LOT-09](@ref lot-09)** : retirer la planche du `LOT-50` (ses figurines servent encore) ;
  poser une texture par l'`anchor` de son manifeste — le composeur de l'arène suppose
  (34, hauteur − 42), faux pour une pièce libre élargie ; poser les grandes pièces du Colisée
  (portes, loges) et ses gradins, que la grille de combat n'emploie pas encore.
- **Écart consigné dans `EX-VIS-008`, non tranché** : le facteur d'affichage de la scène n'est pas
  entier (cadrage à la fenêtre, figurines × 1,25), et les pixels d'art n'ont pas tous la même
  taille à l'écran.
- **Poids des planches recomposées** : la planche de Martpart pèse 1,5 Mio ; les allègements
  essayés n'étaient pas fidèles (journal).
- **Les autres lieux de la Capitale** (Arenarea, le bas-fond, la variante noble de l'Arène du
  Destin) se commandent au fil de l'eau par les lots qui les posent (`LOT-96`, [LOT-27](@ref lot-27)).

## Conception

### Les décisions

| Sujet | Décision |
|---|---|
| **Point de vue** | **Isométrique, losange au rapport 0,62** : celui d'`core::IsoProjection` et du composeur du `LOT-86`, que le `WorldViewportItem` du `LOT-09` reprend. La maquette fixe le trait, la palette, la lumière et la taille de tuile, **pas** la géométrie. *Décision de l'auteur, 16 septembre 2026.* |
| **Échelle** | Tuile de **68 × 42 pixels d'art** et personnage de **45 px** : au facteur `ARENA_FIGURE_SCALE` (1,25), la tuile tombe à 85 × 52,5 px, la tuile de 86 px de la planche du `LOT-50`. Figurine et sol sont donc au **même pas de pixel** — ce que la planche du `LOT-50` n'était pas. Proposition à confirmer par la maquette (voir *Risque*). |
| **Murs** | Un mur monte de `ARENA_WALL_RISE` (0,85) largeurs de losange au-dessus de sa case : 58 px d'art. |
| **Lieux de la Capitale** | Ils entrent dans l'atlas. Le *Tanares Sourcebook* décrit la Capitale et ses **douze quartiers** dans un encart de la région Central Empire (PDF p. 50, imprimées 98-99), hors « Places of Interest » : `atlas.py` ne l'a pas lu. Ce lot l'étend. *Décision de l'auteur, 16 septembre 2026.* |
| **Générateur** | Celui du `LOT-91` : génération d'images d'OpenAI, envois **à la main**, retours dans un dossier de travail hors dépôt (`TEXTURE_ATELIER`, par défaut `D:\JustAnotherDnDGame-textures`). Claude rédige, découpe, mesure et intègre ; il ne dessine pas. |
| **Identités** | La scène est en pixel art ; l'interface reste à la charte v2 (`LOT-87`). La spécification le dit (T5). |

### Les tâches

| Tâche | Contenu | Acceptation |
|---|---|---|
| **T0 — La Capitale dans l'atlas** | `scripts/sourcebook/atlas.py` lit l'encart « The Capital City » : la Capitale et ses douze quartiers (Sloghood, Uptown, Artisansquare, Scholarnest, Dweomer, **Martpart**, **Arenarea**, Oldtown, Neckoffoods, **Bloomburgs**, Downtown, Palacedomain) deviennent des lieux de `central-empire`, au schéma `location.schema.json` inchangé. `LIEUX_ATTENDUS` et `test_atlas.cpp` suivent. | `sourcebook atlas` régénère sans diff hors ces treize fiches ; `test_atlas` vert. |
| **T1 — La maquette de style** | `atelier/prompts/maquette.txt` et `atelier/scripts/maquette.py` (l'envoi : prompt + référence d'échelle composée des figurines d'Anariel et de Jade sur la grille de losanges). Tours jusqu'à approbation ; la maquette retenue va dans `atelier/ancres/maquette.png`. | **L'auteur approuve** ; le risque de l'angle est tranché. |
| **T2 — Le bloc A** | `atelier/prompts/style.txt`, écrit depuis la maquette approuvée : pas de pixel, trait, palette du sol et de la pierre, lumière, tuile. | Relu par l'auteur. |
| **T3 — La disposition et la découpe** | `atelier/dispositions/<id>.json` (cellules nommées : sols, pièces hautes, grandes pièces ; emprise en cases, hauteur) ; `scripts/extract_texture_sheet.py` en déduit la grille, le gabarit, le bloc C, la découpe et la clé `scene/<lieu>/<nom>` de chaque texture ; `check_assets_brief.py` valide les dispositions. | `extract_texture_sheet.py --check` reproduit le dossier ; `check_assets_brief.py` vert. |
| **T4 — La planche du Colisée** | Bloc B depuis la fiche d'Arenarea, réduit aux phrases de l'Arena of Fate, disposition « Colisée » : sable, gradins, loges, couloirs, vestiaires, portes, torches, bannières. Remplace la planche du `LOT-50` au rendu de l'arène. | Découpée par le script ; l'arène se dessine avec. |
| **T5 — Martpart et la spécification** | La planche de Martpart se **commande** (envoi prêt) depuis sa seule fiche d'atlas, bloc A intouché ; `Documentation/Specification/` dit les deux identités, scène et interface. | Envoi préparé sans rédaction à la main ; spécification relue. |

## Risques et questions ouvertes

### L'angle 0,62 en pixel art

Le pixel art isométrique trace ses arêtes en marches régulières de 2 pour 1 ; un rapport de 0,62
donne des marches de 21 pour 34, irrégulières, qui se voient sur un sol uni. La maquette dit si
cela se lit ; si non, deux sorties, à trancher avec elle sous les yeux : un losange 68 × 34
(2 : 1, `ARENA_DIAMOND_RATIO` à 0,5 et le cadrage à revoir) ou des sols texturés qui masquent
l'arête (pavés, sable).

## Exigences couvertes

- `EX-VIS-008` — la scène en pixel art isométrique, au style écrit par le bloc A (T1, T2, T4, T5).
- `EX-VIS-009` — l'interface à la charte v2, et la frontière entre les deux identités (T5).

## Journal

- **16 septembre 2026** — ouverture. Point de vue isométrique 0,62 gardé ; la Capitale entre dans
  l'atlas. Rédaction du prompt de maquette (T1).
- **16 septembre 2026, maquette tour 1** (prompt v1). *Tenu* : la grille — les dalles suivent
  exactement les losanges 136 × 84 de la référence, et le rapport 0,62 ne se voit pas en escalier
  sur un sol pavé ; l'ambiance (soir, lanternes, auvents rayés, bannières). *Écarts mesurés* :
  pas de pixel perdu (2 % de voisins identiques par paire, 252 548 couleurs, halos doux) ;
  façades et étals de face, pas le long des arêtes du losange ; porte de ~195 px pour une
  figurine de 90 (2,2 fois) ; figurines redessinées, pas recopiées. Prompt v2 (v1 gardé en
  `maquette_v1.txt`) : blocs 2 × 2 et 64 couleurs, faces le long des diagonales, tailles
  d'objets chiffrées, joints de dalle admis.
- **16 septembre 2026, maquette tour 2** (prompt v2). *Gagné* : un pixel art franc, trait et
  aplats lisibles, palette resserrée à l'œil ; les étals du fond suivent les diagonales. *Perdu* :
  la grille — dalles de ~62 × 35 px (période mesurée), non calées sur la référence ; figurines
  déplacées et agrandies (~105 px). *Toujours faux* : la façade gauche de face, la porte (~225 px).
  Ni l'un ni l'autre tour n'est sur un pas de 2 px (aucune phase dominante de la différence
  horizontale) : le générateur rééchantillonne, la mise au pas se fera à la réception, comme
  `normalise.py` au `LOT-91`. *Constat* : le générateur tient le style **ou** la métrique, pas
  les deux dans une scène.
- **16 septembre 2026, maquette approuvée : le tour 1.** *Décision de l'auteur*, pour son
  ambiance, plus riche et plus chaude que le tour 2. Copiée en `atelier/ancres/maquette.png`.
  Le risque de l'angle 0,62 est levé pour les sols texturés. Les écarts du tour 1 ne se corrigent
  pas dans la maquette : le pas de pixel se remet à la réception (quantification, comme au
  `LOT-91`) ; les faces le long des diagonales et la taille des objets sont des règles du bloc A,
  vérifiées planche par planche, où la cellule de la disposition impose le cadre. Bloc A v1
  écrit (`atelier/prompts/style.txt`), palettes de matière relevées sur la maquette (T2).
- **16 septembre 2026, T0 fait.** `atlas.py` lit l'encart de la Capitale (PDF index 49, déclaré
  dans `CAPITALE` : la typographie ne le distingue pas d'une ouverture de région) : treize lieux
  de `central-empire`, `central-empire-the-capital-city` et ses douze quartiers
  `central-empire-the-capital-city-<quartier>`. Quartiers repérés par leur fragment gras
  « N- Nom. », numérotation contrôlée ; numéro de page imprimé et filigrane de commande écartés.
  Régénération sans autre diff que ces treize fiches et `regions/central-empire.json`
  (`LIEUX_ATTENDUS` 12 → 25) ; 94 → 107 lieux. Nouveau test
  `AtlasTest.LaCapitaleEtSesDouzeQuartiersSontRejouesDepuisLeLivre` (phrases de Martpart et
  d'Arenarea recopiées du PDF) ; `check_rpg_data.py` vert, 1 263 tests unitaires verts.
- **16 septembre 2026, filigrane retiré de l'atlas** (accord de l'auteur). Treize fiches du
  `LOT-37` portaient le numéro de page et le filigrane de commande du livre (`Order #…`), en fin ou
  en milieu de description. `sans_filigrane` les retire **à l'assemblage de la description** et
  non à la lecture : retirée plus tôt, la ligne ne sépare plus deux intertitres, et la légende
  « Fisherman's Wharf » du plan de la République fusionnait avec « Locations in Fisherman's
  Wharf ». Le numéro de page n'est retiré que collé au filigrane : un filtre sur toute ligne de
  chiffres effaçait les cellules de la table d6 des rencontres de morts-vivants. Diff audité mot
  à mot : les treize filigranes, rien d'autre. *Reste* : `republic-of-freelands-fisherman-s-wharf`
  est cette légende, pas un lieu ; sa description est désormais vide (le schéma l'admet).
- **16 septembre 2026, T3 fait.** *Décisions* :
  - **La disposition vient avant l'image.** `extract_coliseum_atlas.py` relevait des coordonnées
    sur une planche déjà faite ; ici un JSON déclare chaque cellule par sa classe (`floor`,
    `tall`, `wide`), son emprise en cases (`footprint`, `[1, 2]` pour une pièce allongée sur
    l'arête haut-gauche) et sa hauteur (`rise`, en pixels d'art). La grille (rangées remplies dans
    l'ordre, marge de 8 px), le gabarit envoyé au générateur, le bloc C, la découpe et le
    manifeste s'en déduisent : aucune coordonnée écrite à la main.
  - **Géométrie d'`IsoProjection`** : une emprise a × b fait (a + b) · 34 × (a + b) · 21 px
    d'art ; l'ancre d'une texture est le sommet haut de son emprise, le coin (0, 0) de sa case.
  - **Le cahier des scènes est la disposition.** Le cahier du `LOT-87` est réservé à l'interface
    (clés `ui/…`, jetons, zones de maquette) ; une texture de scène n'y a ni jeton ni maquette
    d'écran. `check_assets_brief.py` fait donc valider les dispositions par
    `extract_texture_sheet.valider_tout` (sans Pillow, comme la CI) : clés `scene/…` au format du
    `LOT-39` et uniques, lieu présent dans l'atlas, grille qui tient dans la planche.
  - **Réception** : planche ramenée à 1536 × 1024, alpha binarisé à 127 (une planche sans
    transparence est refusée), 2 × 2 → 1 par moyenne des pixels opaques, **une palette de
    64 couleurs commune** à la planche, sols découpés au losange exact et refusés sous 97 % de
    couverture, débordement hors canevas signalé. Installés sous `installRoot` avec
    `planche.png` et `manifest.json` ; `--check` refait la découpe en mémoire et compare pixels et
    manifeste **sans rien écrire** (celui du `LOT-50` réécrivait ses PNG).
  - *Disposition du Colisée* (T3, alors sur une planche 1536 × 1024 au pas 2) : 36 cellules — 15 sols, 17 pièces hautes (murs, arches, gradins,
    escaliers, torches et bannières en deux orientations, pilier, brasero, banc, râtelier),
    4 grandes pièces (porte des combattants, loge) ; bloc B depuis la fiche de Martpart, sujet :
    l'Illu Die Arena (remplacé ensuite par l'Arena of Fate, voir plus bas). Tests : `scripts/tests/test_extract_texture_sheet.py` (13, dont la découpe
    d'une planche synthétique et son `--check`).
- **16 septembre 2026, T2 fait** : le bloc A (`style.txt` v1) est relu par l'auteur.
- **16 septembre 2026, planches plus grandes et au pas 4** (demande de l'auteur). Le 1536 × 1024
  de T3 venait d'une limite du générateur qui n'a plus cours : la documentation d'OpenAI
  (`gpt-image-2` et suivants) admet toute taille aux côtés multiples de 16, d'au plus 3840 px, au
  rapport d'au plus 3 : 1, entre 655 360 et 8 294 400 pixels, et donne le rendu pour
  expérimental au-delà de 2560 × 1440. *Décisions* : la disposition déclare `sheet.size` et
  `sheet.scale` (pixels d'écran par pixel d'art), contrôlés contre ces contraintes ; les cellules
  débordent sur **autant de planches que nécessaire** ; la palette de 64 couleurs reste
  **commune au lieu**. Le Colisée passe à **2560 × 1440 au pas 4** — la plus grande taille
  fiable, quatre fois plus de pixels par pixel d'art qu'au pas 2 : 2 planches (24 et 12
  cellules). La texture installée ne grandit pas (une tuile reste 68 × 42 px d'art, l'échelle des
  figurines) ; c'est la réduction qui gagne en netteté. Le bloc A porte désormais `{PAS}`,
  `{LOSANGE_L}` et `{LOSANGE_H}`, remplis depuis la disposition, sans autre changement ; le bloc C
  précise que la maquette est au pas 2 et la planche à son propre pas.
- **16 septembre 2026, le Colisée est l'Arena of Fate d'Arenarea** (*décision de l'auteur*). Le
  bloc B recopiait la fiche entière de Martpart : marché, ruelles, vie nocturne — autant d'étals
  et de lanternes que le générateur aurait dessinés sur une planche d'arène. Et le rattachement à
  l'Illu Die Arena était une supposition de Claude, que la feuille de route ne portait pas.
  *Décisions* : la disposition désigne `central-empire-the-capital-city-arenarea` ; le champ
  `locationExcerpt` restreint le bloc B à des phrases de la fiche, **vérifiées mot pour mot**
  par `valider` (le bloc B cite le livre, il ne le récrit pas) ; le Colisée en cite deux, celles
  de l'Arena of Fate, et son sujet exclut le quartier alentour.
- **16 septembre 2026, planche du Colisée, tour 1.** *Reçu* : deux planches à fond transparent,
  rendues en 1672 × 941 pour 2560 × 1440 demandés ; le style de la maquette est tenu, les 36
  pièces sont là, **dans l'ordre**. *Écarts* :
  1. **Le gabarit n'est pas suivi** : pièces réparties librement, planche 2 dessinée plus grande
     que la planche 1 ; la découpe aux cellules rendait 27 fautes. *Décision* : la découpe lit
     désormais les **pièces** (composantes connexes, éclats proches rattachés, éclats lointains
     effacés ; rangées par recouvrement vertical, lues de gauche à droite), les rapproche des
     cellules par leur rang, et met chacune à la largeur de son emprise — ou, pour une pièce
     plus petite qu'elle (`"fill": false` : pilier, brasero, banc, râtelier), au facteur médian
     des pièces pleines de sa planche. La planche n'est plus ramenée à la taille demandée.
     Résultat sur ce tour : 24 et 12 pièces lues, facteurs de 0,36 à 0,37 sur la planche 1.
  2. **Le seuil de couverture des sols** (97 %) refusait `stands-step` à 96,9 % ; les quinze
     sols, pleins à l'œil, couvrent de 96,9 à 99,8 % (arrondi de l'arête) : seuil à 95 %.
  3. **L'orientation n'est pas tenue**, et c'est ce qui écarte le tour : arches et gradins
     inversés, les deux torches dans le même sens, escaliers sans règle — « upper-left edge »
     écrit en toutes lettres ne suffit pas. *Décision* (on corrige le prompt, jamais l'image) :
     une cellule orientée déclare `"edge"` (`left`, `right`, `both`), le gabarit trace cette
     arête **en rouge épais**, le bloc C dit que la pièce se dresse contre elle (ou monte vers
     elle), et les descriptions de cellules perdent leurs « upper-left/right ».
  *Tour 2 préparé* ; rien n'est installé du tour 1. Tests : 24.
- **16 septembre 2026, planche du Colisée, tour 2 : installée.** *Reçu* : 1672 × 941 encore,
  36 pièces lues sans faute. L'arête rouge améliore l'orientation sans la fixer : murs justes,
  mais pour arches, torches, bannières, portes, loges, gradins et escaliers, la seconde pièce de
  la paire (« the same … ») est recopiée dans le sens de la première une fois sur deux.
  *Décision de l'auteur* : **miroir à la découpe**. Une paire ne se dessine qu'en « -left » ; la
  cellule « -right » déclare `mirrorOf` et n'est plus demandée au générateur (27 pièces
  dessinées, planches de 24 et 3) ; la découpe la produit par retournement horizontal (emprise
  inversée, ancre reportée), au prix d'une lumière venue d'en haut à droite sur la pièce
  retournée. *Contrôle d'orientation automatique* (`_orienter`), étalonné sur les tours 1 et 2
  et concordant avec l'œil : une pièce dressée le long de son arête a son point bas du côté de
  l'arête (indices mesurés 0,16–0,26 contre 0,74–0,85) ; une pièce qui monte vers elle
  (`"stance": "toward"`) penche sa moitié haute de ce côté (0,41–0,45 contre 0,58–0,60). Une
  pièce dessinée contre l'autre arête est retournée, avec un avertissement.
  *Installation* : le tour 2 a été généré sur la disposition du commit `74adbc0ee` ; il est
  découpé avec elle, et ses 27 pièces encore dessinées sont reposées ×4 dans la grille actuelle
  (script d'usage unique, hors dépôt). Ces **planches recomposées** sont les sources installées
  sous `Source/Elements/Assets/Scene/coliseum/` (`planche-1.png`, `planche-2.png`, 36 textures,
  `manifest.json`, 480 Kio) ; `--check` les reproduit. Retournées par le contrôle : torche,
  bannière, loge, gradins, escalier « -left ». Relu à l'œil sur une planche de contrôle (textures
  ×3, ancres marquées) : orientations, ancres et miroirs justes. `bench` est rogné de 2 px.
  *Reste pour T4* : que l'arène se dessine avec ces textures (composeur du `LOT-86`).
- **16 septembre 2026, T4 fait : l'arène se dessine avec l'atelier.** `hmi::composeArenaScene`
  lit ses sols et son enceinte sous `Assets/Scene/coliseum/` (chemins `../Scene/coliseum/<nom>.png`
  relatifs au dossier du Colisée, comme `../Npc`) ; les figurines restent celles du Colisée.
  *Décisions* :
  - **une pièce par case d'enceinte**, qui porte son mur : pan, pan à torche, pan à bannière,
    arche sur une porte ; l'angle du fond est `wall-corner`, les trois autres des piliers
    (14 quads d'enceinte sur la piste de test au lieu de 15 : la torche n'est plus posée sur un
    pan) ;
  - **l'arête** : les bords haut et bas de la grille courent comme l'arête droite d'une case
    (`-right`), les bords gauche et droit comme l'arête gauche (`-left`) ; un mur du bord de
    devant se dresse contre l'arête du fond de sa case, l'atelier ne dessinant que des pièces du
    fond ;
  - **la pose par l'ancre** : 68 px d'art pour la largeur du losange
    (`ARENA_SCENE_TILE_WIDTH_PIXELS`), le sommet haut du losange de la case sur le pixel
    (34, hauteur − 42) — l'`anchor` du manifeste pour une emprise d'une case ;
  - **les sols** : `stone-slab` sous l'enceinte, `gate-threshold` sous une porte, `sand` et ses
    trois variantes là où le catalogue semait une dalle claire.
  *Vérifié* : 1 265 tests unitaires (nouveau : `LeDecorSePoseParSonAncreDansLeBonSens`) ; le
  rendu QRhi charge les 22 textures de scène depuis les fichiers livrés ; capture d'une arène
  20 × 14 hors écran (`ArenaSceneRendererTest.CaptureDeLArenePourRelecture`, jouée si
  `JADG_ARENA_CAPTURE` nomme un fichier) relue à l'œil : enceinte orientée, portes, bannières,
  torches, figurines à l'échelle. *Reste au `LOT-09`* : retirer la planche du `LOT-50` (ses
  figurines servent encore) ; les grandes pièces (portes, loges) et les gradins ne sont pas
  encore posés, la grille de combat n'en a pas l'usage.
- **16 septembre 2026, T5 : Martpart se commande, la spécification dit les deux identités.**
  *Décisions* :
  - **Un modèle, pas une disposition rédigée.** Une disposition sans cellules écrites pour son
    lieu nomme un modèle (`atelier/dispositions/modeles/<nom>.json`) qui porte la taille des
    planches, les classes, le sujet et des cellules **neutres** — « un objet typique du lieu décrit
    plus haut » — : c'est le bloc B, la fiche d'atlas **entière**, qui dit au générateur ce qui rend
    le lieu reconnaissable. `resoudre` fusionne le fichier sur son modèle ; le titre vient du nom de
    la fiche, `keyPrefix` (`scene/<id>`) et `installRoot` de l'identifiant ; un champ écrit dans le
    fichier l'emporte. `martpart.json` tient en quatre champs : `version`, `id`, `location`,
    `model`. Le Colisée garde sa disposition propre.
  - **Modèle `quartier`** : 22 cellules, 18 dessinées, une planche 2560 × 1440 au pas 4 — six sols
    (rue et deux variantes, place, ruelle, seuil), mur, angle, porte, fenêtre éclairée (paires en
    miroir), éclairage de rue, quatre objets typiques, une façade sur deux cases (paire en miroir),
    deux grands éléments libres sur deux cases. Bloc A intouché.
  - *Préparé* : `commande martpart 1` → `<TEXTURE_ATELIER>/chatgpt/martpart-tour1/planche-1/`
    (prompt de 7 091 caractères, maquette, gabarit). *Risque vu au prompt* : la fiche nomme l'Illu
    Die Arena ; un « grand élément » peut la dessiner entière sur deux cases. Constat à faire au
    tour 1, pas de correction d'avance.
  - **Spécification** : `vision.md` gagne une section *Identités visuelles* — `EX-VIS-008` (la
    scène : losange 68 × 42, figurine 45 px, bloc A et maquette, planche par lieu, palette de 64
    couleurs, plus proche voisin) et `EX-VIS-009` (l'interface à la charte v2 ; ce qui renseigne le
    joueur par-dessus la scène en relève, ce qui est du monde relève du pixel art ; le viewport est
    le seul contact). `EX-IHM-070` et `EX-ARCH-022` sont *précisées* en renvoi. *Écart consigné,
    non tranché* : le facteur d'affichage de la scène n'est pas entier (cadrage, figurines × 1,25).
  Tests : 27 dans `test_extract_texture_sheet.py` (nouveaux : Martpart depuis sa seule fiche,
  modèle inconnu refusé, champ du fichier prioritaire).
- **16 septembre 2026, Martpart, tour 1.** *Reçu* : une planche à fond transparent, 1672 × 941 ;
  18 pièces dans l'ordre, style de la maquette tenu, contenu tiré de la fiche (étals à auvents,
  panneau d'avis, marchandises, étal d'armes, bannière sur la façade) ; l'Illu Die Arena n'est pas
  dessinée. Sols, murs, angle, lampadaire et façade justes ; le mur, dessiné contre l'autre arête,
  est retourné par `_orienter`. *Écarts* :
  1. `doorstep` dessiné en marches, refusé par la découpe (94 % du losange) ;
  2. porte et fenêtre **de face**, pas le long de l'arête (l'écart de la maquette) ; le contrôle
     d'orientation ne le voit pas, la pièce étant symétrique ;
  3. objets et grands éléments **plus larges que leur emprise** (étals de ~170 px d'art pour 102),
     que la découpe **rognait** ;
  4. porte et fenêtre ~11 % plus hautes que la classe (canevas agrandi, non bloquant).
  *Décisions* :
  - **une pièce libre (`"fill": false`) n'est plus rognée** (*décision de l'auteur*) : centrée sur
    le sommet bas de son emprise, elle élargit son canevas de part et d'autre et l'**ancre du
    manifeste suit** ; elle déborde sur les cases voisines, comme tout objet isométrique plus large
    que sa case. Une pièce pleine reste rognée à son emprise. Conséquence pour le `LOT-09` : poser
    une texture par l'`anchor` de son manifeste, pas par un (34, hauteur − 42) supposé ;
  - on corrige le modèle, jamais l'image : seuil « dalle plate, sans marche ni hauteur » ; porte et
    fenêtre décrites comme « le mur de la cellule 7, vu sous le même angle, percé d'une porte /
    d'une fenêtre ».
  *Constat, non corrigé* : les deux grands éléments sont dessinés sur une emprise carrée plutôt
  qu'allongée ; centrés sur le sommet bas de leur emprise de 2 × 1, ils la débordent surtout à
  droite (18 et 52 px d'art). *Rien n'est installé du tour 1* ; `commande martpart 2` préparé.
  Colisée redécoupé depuis ses planches installées : seul `bench` change (rogné de 2 px, il passe
  à 70 px d'art, ancre à 35) ; le composeur ne le pose pas ; `--check colisee` vert. Tests : 28.
- **16 septembre 2026, Martpart, tour 2.** *Reçu* : 1672 × 941, 18 pièces dans l'ordre. *Gagné* :
  porte et fenêtre **le long de l'arête**, dans le mur de la cellule 7 (la correction du modèle a
  porté) ; seuil plat ; objets et grands éléments dans leur emprise ou presque (85 et 77 px d'art
  pour 102). *Perdu* : les **sols**, dessinés en losanges de rapport **0,56 à 0,58** (mesuré :
  rang le plus large sur hauteur de la pièce) au lieu de 0,62 — au tour 1, 0,60 à 0,67 ; la
  découpe en refuse cinq (couverture de 90 à 94 %), à juste titre : ce n'est pas un arrondi.
  *Bug corrigé* : une pièce libre était centrée sur le sommet bas de son emprise, juste pour une
  case, décalé de 17 px d'art pour une emprise de 2 × 1 ; elle l'est désormais sur le milieu de
  l'emprise (le Colisée, aux pièces libres d'une case, ne change pas : `--check` vert). Rien
  n'est installé.
- **16 septembre 2026, Martpart : recomposition** (*décision de l'auteur*, plutôt qu'un tour 3
  complet ou un étirement des sols). Les cinq sols du tour 1, les douze autres pièces du tour 2,
  sur une planche recomposée par un script d'usage unique, hors dépôt
  (`<TEXTURE_ATELIER>/martpart/recomposition/recompose.py`). *Piège écarté* : reposer la texture
  découpée ×4 n'est pas fidèle — un sol dont la découpe a perdu un pixel de pointe est remesuré
  plus étroit, donc **étiré** à la redécoupe (le seuil du tour 2 passait ainsi de 90 à 96 %, sans
  être meilleur). On repose donc les **pixels source** de chaque pièce (sa boîte sur la planche
  reçue, masquée à ses composantes), mis à l'échelle de sa découpe : la redécoupe de la planche
  recomposée rend les mêmes tailles et ancres, alpha identique à 99,5 % au moins. *Reste* :
  **le seuil**, en marches au tour 1, trop plat au tour 2 (91 %). La classe `floor` du modèle
  précise « aussi large et aussi haut que le contour, jamais plus plat » ; `commande martpart 3`
  préparé, dont seul le seuil sera pris s'il passe. Rien n'est installé.
- **16 septembre 2026, Martpart, tour 3.** 18 pièces lues ; façade retournée par `_orienter`.
  **Les six sols encore aplatis** (270 × 154 px pour un losange de 272 × 168 : 86 à 91 % de
  couverture) malgré la classe « jamais plus plat » ; le seuil porte en outre une dalle en relief.
  Seul le tour 1 a tenu la forme des sols, dessinés plus petits (≈ 200 px de large). Rien n'est
  pris de ce tour. *Tour 4* (*décision de l'auteur*, plutôt que retirer le seuil) : le bloc C
  répète pour **chaque sol** sa largeur et sa hauteur en pixels, sommet à sommet ; le seuil devient
  « le pavé de la rue, une grande dalle lisse encastrée à niveau, rien en relief ».
- **16 septembre 2026, Martpart, tour 4 : installé.** *Reçu* : 18 pièces, **tout passe** — la
  taille répétée pour chaque sol a porté : losanges de 194 × 120 px (rapport 0,61 à 0,63), seuil
  plat, dalle encastrée. *Mais* le mur porte bannière, lanterne et jardinière (répétées sur chaque
  case de mur) et la « façade » est un étal. *Recomposition* (*décision de l'auteur*) : les **six
  sols du tour 4**, seuil compris — un seul tour, pavés plus réguliers — et les douze autres
  pièces du tour 2 ; les sols du tour 1 sont abandonnés. Installé sous
  `Source/Elements/Assets/Scene/martpart/` : 22 textures, `planche-1.png` recomposée,
  `manifest.json` ; `--check martpart` vert ; relu à l'œil sur une planche de contrôle (×3,
  ancres marquées). *Poids* : 1,6 Mio, dont 1,5 Mio de planche (le Colisée : 480 Kio). La
  planche garde les pixels source remis à l'échelle ; deux allègements essayés et écartés —
  256 couleurs (−20 %, la moitié des pixels de texture changent) et textures découpées reposées
  ×4 comme au Colisée (160 Kio, mais le masque de `street-3` bouge : pas fidèle). Les planches
  reçues pèsent elles-mêmes 1,4 à 1,6 Mio. **T5 : acceptation tenue** — la planche d'un lieu s'est
  commandée depuis sa seule fiche et un modèle, bloc A intouché ; reste la relecture de la
  spécification.

## Bilan

Statut : **livré le 16 septembre 2026** (ouvert le même jour). Vérification : build Debug et `ctest` à
**1280/1281** (la capture de l'arène, jouée sur demande, est sautée), `pytest` de l'atelier à 28/28,
`extract_texture_sheet.py --check` vert pour le Colisée et Martpart, `check_assets_brief.py`, lints
d'exigences et de lots verts, cahier de test régénéré (1275 cas) ; maquette approuvée et
spécification relue par l'auteur ; arène capturée hors écran et relue.

Alimente : [LOT-09](@ref lot-09), `LOT-93`, `LOT-95`, `LOT-42`.
Exigences couvertes : `EX-VIS-008`, `EX-VIS-009` (détail plus haut).
