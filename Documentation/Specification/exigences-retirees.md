# Exigences retirées

> Deux familles d'exigences sont retirées **en entier** par le `LOT-88` : aucune ne décrit plus
> rien du jeu ni du dépôt. Leurs ancres restent ici, jamais renumérotées : les lots livrés s'y
> réfèrent, et un numéro réutilisé ferait mentir leur histoire. Les exigences retirées une à une,
> dans une famille qui vit encore, restent à la fin de leur propre spécification.
>
> Une exigence retirée n'est citée par aucun code : `scripts/checks/lint_exigences.py` le vérifie.

## Solveur d'apprentissage automatique (`EX-IA-*`)

Un agent entraîné sans framework d'apprentissage, capable de terminer un niveau seul et de rejouer
sa solution en jeu. Le programme et son code ont été retirés au `LOT-01`.

- **EX-IA-001** *(retirée au `LOT-88`)* — calcul tensoriel et générateur
  pseudo-aléatoire maison.
- **EX-IA-002** *(retirée au `LOT-88`)* — différentiation automatique.
- **EX-IA-003** *(retirée au `LOT-88`)* — réseaux de neurones.
- **EX-IA-004** *(retirée au `LOT-88`)* — optimiseurs de descente de gradient.
- **EX-IA-005** *(retirée au `LOT-88`)* — jeu sans fenêtre pour l'entraînement.
- **EX-IA-006** *(retirée au `LOT-88`)* — observation de l'état du jeu.
- **EX-IA-007** *(retirée au `LOT-88`)* — espace d'action de l'agent.
- **EX-IA-008** *(retirée au `LOT-88`)* — séquence d'actions gagnante.
- **EX-IA-009** *(retirée au `LOT-88`)* — signal de récompense.
- **EX-IA-010** *(retirée au `LOT-88`)* — journal d'entraînement.
- **EX-IA-011** *(retirée au `LOT-88`)* — algorithme évolutionniste.
- **EX-IA-012** *(retirée au `LOT-88`)* — entraînement niveau par niveau.
- **EX-IA-013** *(retirée au `LOT-88`)* — gradient de politique.
- **EX-IA-014** *(retirée au `LOT-88`)* — réduction de variance.
- **EX-IA-015** *(retirée au `LOT-88`)* — apprentissage par valeur.
- **EX-IA-016** *(retirée au `LOT-88`)* — harnais de benchmark.
- **EX-IA-017** *(retirée au `LOT-88`)* — taux de réussite d'un modèle.
- **EX-IA-018** *(retirée au `LOT-88`)* — validation d'un fichier de rejeu.
- **EX-IA-019** *(retirée au `LOT-88`)* — rejeu en partie réelle.
- **EX-IA-020** *(retirée au `LOT-88`)* — exécutable en ligne de commande.
- **EX-IA-021** *(retirée au `LOT-88`)* — garde-fou d'intégration continue.
- **EX-IA-022** *(retirée au `LOT-88`)* — écran « Mode IA ».
- **EX-IA-023** *(retirée au `LOT-88`)* — champ de distances de la récompense de
  progression.

## Décors et plans picturaux (`EX-DEC-*`)

Des images peintes couvrant un niveau entier, avec densité, profondeur et parallaxe, peintes dans
l'éditeur ; et avant elles des décors-sprites manipulables. Le lieu d'une carte se dessine
désormais avec les pièces de sa planche (`EX-VIS-008`, `EX-REN-010`).

- **EX-DEC-001** *(retirée au `LOT-88`)* — décor posé librement.
- **EX-DEC-002** *(retirée au `LOT-88`)* — décors devant ou derrière le
  personnage.
- **EX-DEC-003** *(retirée au `LOT-88`)* — plan affiché fidèle à l'asset.
- **EX-DEC-004** *(retirée au `LOT-88`)* — décors en entités de la simulation.
- **EX-DEC-005** *(retirée au `LOT-88`)* — collision propre à un décor.
- **EX-DEC-006** *(retirée au `LOT-88`)* — parallaxe d'un décor.
- **EX-DEC-010** *(retirée au `LOT-88`)* — édition des décors.
- **EX-DEC-020** *(retirée au `LOT-88`)* — manipulation des décors en jeu.
- **EX-DEC-021** *(retirée au `LOT-88`)* — déterminisme de cette manipulation.
- **EX-DEC-030** *(retirée au `LOT-88`)* — conversion d'une photo en pixel art.
- **EX-DEC-031** *(retirée au `LOT-88`)* — paramètres de cette conversion.
- **EX-DEC-032** *(retirée au `LOT-88`)* — enregistrement de l'image convertie.
- **EX-DEC-040** *(retirée au `LOT-88`)* — plan pictural couvrant le niveau.
- **EX-DEC-041** *(retirée au `LOT-88`)* — densité d'un plan.
- **EX-DEC-042** *(retirée au `LOT-88`)* — profondeur d'un plan.
- **EX-DEC-043** *(retirée au `LOT-88`)* — parallaxe d'un plan.
- **EX-DEC-044** *(retirée au `LOT-88`)* — coût borné des plans d'un niveau.
- **EX-DEC-045** *(retirée au `LOT-88`)* — peinture des plans dans l'éditeur.

## Textes remplacés au `LOT-1001` — le passage à la 3D

Ces cinq exigences **vivent encore**, sous leur numéro, dans leur spécification : le
passage à la 3D (décision D-29) en a réécrit le texte. Les textes qu'elles portaient depuis
les `LOT-101`, `LOT-103` et `LOT-129` sont gardés ici, tels qu'ils étaient — chemins de
fichiers compris, dont certains sont depuis passés sous `Planning/standards/archives/` —,
parce que les lots livrés ont été jugés sur eux. Ils ne prescrivent plus rien.

### `EX-VIS-008`, texte remplacé

> La **scène** — sols, murs, objets du monde et figurines —
> doit être **peinte en isométrie haute définition** : losange de sol de **256 × 159 pixels d'art**
> (rapport 0,62, celui d'`core::IsoProjection`), figurine humanoïde de **170 px** dans une cellule
> de **192 × 256** dont la ligne de sol est à `y = 252` (cellule large **384 × 256** pour l'attaque
> et le sort), grande créature en **384 × 384**. L'échelle de l'art est une **donnée du lieu** — le
> champ `"tile"` de son manifeste de pièces — et non une constante du rendu : un lieu peut être
> livré plus fin ou plus grossier sans toucher au code. Les images sont en sRGB, à **alpha continu**
> prémultiplié au chargement, sans palette imposée par image, et échantillonnées en **bilinéaire
> avec mipmaps** (`EX-ARCH-022`). Le style est **écrit** et non laissé au générateur : le standard
> 2D HD (`Planning/standards/style-2d-hd.md`) et la consigne du générateur
> (`Planning/standards/consigne-2d-hd.md`), tirés de la **planche de référence approuvée par
> l'auteur** (`Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png`), fixent le trait (un
> contour sombre et fin, bronze foncé, jamais noir pur), la lumière (clé douce en haut à gauche,
> ombre propre peinte, **aucune ombre portée** dans la pièce), la projection orthographique, la
> palette du lieu et les **dix familles de pièces** dont une zone fait l'inventaire. Une pièce
> **tient seule** : fond transparent, pas de sol sous un mur, pas de décor autour d'un meuble.
> Les figurines de l'atelier des PNJ (`LOT-91`) sont de la scène, à l'échelle de son sol.
> > **Tranché au `LOT-112`.** Une animation compte **huit images** — l'essai de marche en six et
> > en huit l'a décidé —, une bande par diagonale isométrique, et la marche va à deux cases par
> > seconde (`EX-REN-012`, `EX-EXP-011`). Aucune figurine ne se produit hors de ce gabarit.

### `EX-VIS-009`, texte remplacé

> L'**interface** — écrans, panneaux, HUD, et tout ce qui
> **renseigne le joueur par-dessus la scène** (curseur, chemin, portées, texte ancré) — doit porter
> la **charte v2** (`EX-IHM-070`) : images produites à 1080p et échantillonnées à tout facteur,
> polices vectorielles embarquées (`EX-REN-032`). Aucun élément de l'interface n'est de l'art de
> scène et aucun élément du monde ne porte la charte : pas de filet d'or ni de `Cinzel` dans la
> scène, pas de pièce isométrique ni de palette de lieu dans les écrans. Les deux couches se
> **mesurent** différemment, et c'est là que la frontière se vérifie : une pièce de scène se met à
> l'échelle du **lieu** (`EX-VIS-008`), une image d'interface à celle de la **fenêtre**. Le seul
> point de contact est le **viewport** de la scène, qu'un écran de l'interface encadre sans le
> peindre.

### `EX-REN-013`, texte remplacé

> La **caméra** du lieu doit suivre le héros, bornée à la scène
> (un axe plus étroit que la vue est centré), à un facteur d'affichage **libre** — l'agrandissement
> entier du pixel art n'a plus d'objet (`EX-ARCH-022`) — et **déduit de la définition de la
> fenêtre** : la largeur d'une case à l'écran vaut la hauteur de la fenêtre divisée par **10,8**,
> soit 100 px à 1080p et 200 px à 2160p. Toutes les définitions cadrent donc la **même étendue de
> monde** — 19,2 losanges de large, 17,4 de haut — et un écran plus fin montre le même jeu plus
> finement, jamais plus de jeu. La « fenêtre » est ici la **scène 16:9** qu'`EX-REN-019` y
> inscrit.

### `EX-REN-014`, texte remplacé

> Le rendu doit gérer un ordre de dessin par **calques**,
> défini par un **ordonnancement unique et explicite** (`hmi::RenderLayer`) dont aucun calque
> concurrent ne peut s'écarter : sol, objets et figurines, puis interface et aides d'édition.

### `EX-REN-018`, texte remplacé

> Dans la scène isométrique, l'ordre de dessin des acteurs et du décor
> traversé doit venir de leur **profondeur**, et non de leur calque : une entité passe devant ce qui
> est plus haut qu'elle à l'écran, derrière ce qui est plus bas. La profondeur se lit au **pied** du
> sprite — le bord bas, point de contact avec le sol — et non à son coin haut. Ces calques forment
> une **bande de profondeur** commune, à l'intérieur de laquelle le tri par profondeur passe
> **avant** le regroupement par texture. Le tri doit rester **stable** et quantifié au pixel : à
> profondeur égale, deux sprites gardent un ordre constant d'une image à l'autre. Hors de cette
> bande, l'ordre des calques reste souverain (`EX-REN-014`). Concrétisé en `LOT-07`. Depuis le
> `LOT-129`, la bande compte **six rangs** par profondeur — le relief, la figurine, puis les étages
> un à quatre (`EX-LVL-025`) — et les jetons de maquette n'y sont plus : ils renseignent, donc
> ils sont de l'interface en scène (`EX-REN-023`).
