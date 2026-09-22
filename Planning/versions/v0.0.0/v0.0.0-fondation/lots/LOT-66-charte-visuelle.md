+++
id = "LOT-66"
titre = "Charte visuelle : sortir de l'identité pixel art"
version = "0.0.0"
filiere = "interface"
statut = "livre"
taille = "L"
resume = "L'interface quitte l'identité du jeu de plateforme pour celle du parchemin de Tanares, dont chaque teinte est relevée sur le corpus."
prerequis = []
livrables = [
  "La **palette d'identité** relevée sur `Character_Sheets_Tanares.pdf`, écrite dans `Source/HMI/Interface/DesignTokens.cpp` et `.design-mockups/_page_head.html`.",
  "Cinq modules neufs dans `Source/HMI/Interface/` : `IdentityScale`, `ParchmentFrame`, `ParchmentPanel`, `FocusMarker`, `MenuEntryButton`.",
  "La suppression des modules pixel art (**641 lignes**) : `PixelFocusCaret`, `PixelFrameGeometry`, `PixelFrameWidget`, `PixelMenuButton` ; `PixelArtScale` renommé.",
  "La refonte des onze exigences qui imposaient le pixel art, à commencer par `EX-ARCH-022`.",
  "`hmi::paintFocusFleuron`, tracé unique de la marque de focus.",
]
criteres = [
  "**Un écran du jeu et un écran de l'éditeur placés côte à côte se distinguent immédiatement** : parchemin et encre sépia d'un côté, gris neutres et accent ambre de l'autre.",
  "**`scripts/check_design_tokens.py` passe sur les nouveaux rôles** — 9 couleurs vérifiées entre la planche et le code.",
  "**Le focus reste signalé par une marque, pas par une teinte** : `paintFocusFleuron`, appelé par les deux peintres.",
  "Les 641 lignes de widgets pixel art ont quitté `Source/HMI/Interface/`.",
  "Les onze exigences citent leur refonte et le lot qui la porte.",
  "`ctest` : **1009/1009**.",
]
+++

## Pourquoi

L'interface était celle du jeu de plateforme dont ce dépôt est issu. Ce lot lui substitue
l'identité du **parchemin de Tanares** — parchemin, encre sépia, filets et cabochons dorés,
titrage à empattements — et retire du code les cinq widgets qui n'existaient que pour le pixel art.

## Périmètre

**La palette d'identité**, relevée sur le corpus (`Character_Sheets_Tanares.pdf`) et écrite aux deux
seuls endroits qui la portent : `Source/HMI/Interface/DesignTokens.cpp` et
`.design-mockups/_page_head.html`.

**Cinq modules neufs** dans `Source/HMI/Interface/` — `IdentityScale`, `ParchmentFrame`,
`ParchmentPanel`, `FocusMarker`, `MenuEntryButton` — en remplacement des cinq modules pixel art
(**641 lignes** supprimées).

**La refonte des onze exigences** qui imposaient le pixel art, à commencer par la racine
`EX-ARCH-022`.

## Conception

### La palette est relevée, pas choisie

Chaque teinte vient de l'histogramme quantifié des pages rendues des feuilles de personnage. La
raison est celle du §4 du corpus (`EX-CNT-020`), transposée à la couleur : **une couleur inventée
ressemble à la source sans en venir, et rien ne le dit jamais.** Six mois plus tard, personne ne
sait plus laquelle des deux on regarde.

| Rôle | Valeur | Ce que c'est dans la feuille |
|---|---|---|
| `background` | `#d0c0a0` | parchemin vieilli, le bord de la feuille |
| `surface` | `#e0d0b0` | le champ, là où l'on écrit |
| `surfaceAlt` | `#f0e0d0` | l'encadré clair |
| `text` | `#302000` | l'encre sépia |
| `textMuted` | `#705020` | l'encre délavée |
| `accent` | `#c0a060` | l'or des filets |
| `frameEdge` | `#302000` | le trait extérieur |
| `frameOrnament` | `#907030` | le filet ornemental |
| `frameShadow` | `#705020` | l'ombre du filet sur le champ |

**Un seul rôle n'est pas attesté : `error`.** Une feuille de personnage n'a pas d'état d'erreur à
montrer. Le rouge de garance assombri qui l'occupe est choisi pour tenir le contraste sur le
parchemin — et **signalé comme non attesté dans le code**, plutôt que glissé dans la liste comme
s'il en venait.

### Les rôles de cadre changent de nom, et c'est le cœur du lot

`outline`, `bevelLight`, `bevelDark` nommaient un **biseau** : une lumière venue d'en haut à gauche,
simulée par un clair et un sombre. Le parchemin n'a pas de relief à simuler. Garder ces noms en
peignant un encadrement plat aurait produit exactement la sorte de mensonge que ce dépôt traque :
du code juste, dont les noms décrivent autre chose. Ils deviennent `frameEdge`, `frameOrnament`,
`frameShadow`, dans les **deux** portées — la structure `ColorTokens` est commune par construction,
et c'est ce qui garantit qu'un rôle ajouté à l'une existe dans l'autre.

### Ce qui fait un encadrement, c'est la réserve

Un trait d'encre, **une réserve de parchemin**, un filet doré. C'est la réserve du milieu, et rien
d'autre, qui fait lire les deux traits comme un cadre : sans elle ils se touchent, et l'ensemble
devient une bordure épaisse de deux tons. Le défaut ne lèverait aucune erreur — toutes les bandes
seraient toujours là, au bon endroit, de la bonne couleur — d'où un test qui relève le rôle
**visible** à mi-hauteur, une unité à l'intérieur du trait, et exige d'y trouver du parchemin.

Les angles portent un **cabochon** doré posé par-dessus le trait : exactement l'inverse de
l'entaille du pixel art, qui *retirait* de la matière aux coins.

### Le facteur d'agrandissement reste entier, pour une autre raison

C'est le point où il aurait été facile de se tromper. `PIXEL_ART_MAX_SCALE` existait parce que le
filtrage au plus proche voisin ne sait pas rendre une bordure d'un pixel et demi ; la charte
parchemin peint anticrénelé, cette raison-là disparaît.

Le facteur reste pourtant entier, pour une **seconde** raison intacte : les longueurs de la feuille
de style sont des **entiers de pixels**. À 1,5×, le trait d'une unité et le filet d'une unité
s'arrondissent tous deux à 2 px — la réserve disparaît, et l'encadrement se lit comme une bordure
épaisse. Une échelle fractionnaire ne serait pas *floue*, elle serait **fausse**, et silencieuse.

### Une déviation assumée sur la liste de suppressions

La feuille de route demandait la suppression des cinq modules pixel art. **Quatre le sont**
(`PixelFocusCaret`, `PixelFrameGeometry`, `PixelFrameWidget`, `PixelMenuButton`). Le cinquième,
`PixelArtScale`, est **renommé** `IdentityScale` : `EX-IHM-081` — le garde-fou anti-cliquet, que ce
lot ne touche pas — est écrite *en fonction de ce facteur*, et le supprimer laisserait une exigence
sans mise en œuvre. Ce qui devait disparaître de ce module a disparu : son nom, et la justification
« pixel art » qui n'était plus vraie.

### Le focus n'est jamais perdu de vue

`EX-IHM-071` (marque explicite) et `EX-IHM-072` (aucun réglage inopérant) sont **tenues sans
changement**, comme la feuille de route l'exigeait. Le curseur triangulaire en escalier devient un
**fleuron** anticrénelé à quatre sommets — échancré à l'arrière, pour qu'il ne se lise pas comme le
curseur de saisie d'un champ de texte.

Il est tracé **une seule fois** (`hmi::paintFocusFleuron`) et appelé des deux côtés — par
`MenuEntryButton` dans sa gouttière, par `FocusMarker` à côté des contrôles ordinaires. Deux tracés
séparés dériveraient l'un de l'autre à la première retouche : le joueur verrait une marque dans le
menu et une autre dans les options, et croirait à deux états différents.

Le fleuron est aussi **borné à la gouttière** du bouton. À la taille d'un intertitre il débordait
sur le texte, qui se serait décalé d'une entrée à l'autre selon qu'elle porte ou non la marque — un
menu dont les lignes bougent quand on les parcourt.

### Le renversement est assumé, pas subi

Le [LOT-01](LOT-01-fork-purge.md) a **délibérément conservé** l'atelier pixel art lors de la purge, et le
[LOT-11](LOT-11-editeur-multicouches.md) en fait un acquis à ne pas régresser. Ce lot revient sur cette décision. Il ne
la contourne pas : `EX-EDIT-045` porte désormais, écrite noir sur blanc, la raison pour laquelle
l'atelier **reste** — c'est un outil de travail, pas une esthétique — et le fait que sa suppression
éventuelle était le `LOT-69` — absorbé par le [LOT-88](LOT-88-retrait-heritage.md) —, pas celui-ci.

## Ce que le lot ne fait pas

**Il ne redessine pas les planches de `.design-mockups/`.** Elles reçoivent la palette, le nommage
des rôles et la composition de l'encadrement — le lint `check_design_tokens.py` l'exige, et une
maquette qui ne décrit plus le jeu ne sert plus à décider quoi que ce soit. Mais les **ornements,
l'illustration peinte et la mise en page à empattements** relèvent du [LOT-68](LOT-68-chassis-ecrans-rpg.md), qui
porte les planches.

**Il ne bascule pas le filtrage de la scène.** `EX-ARCH-022` n'impose plus le plus proche voisin,
mais le rendu le pratique encore : c'est correct pour les tuiles héritées, ses seuls assets
aujourd'hui. Le basculement suit l'arrivée des plans peints ([LOT-76](LOT-76-habillage-interface.md)) — et c'est écrit
dans l'exigence, pas laissé à deviner.

**Il ne touche pas à la portée éditeur.** Un outil de travail garde son apparence d'outil de
travail ; aucun parchemin ne se répand dans ses tables denses. Seuls les **noms** des trois rôles de
cadre y changent, la structure étant commune — leurs valeurs sont intactes.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1009/1009, `clang-format`, les six lints, cahier de test et Doxygen verts ; tous les critères d'acceptation sont cochés dans l'epic d'origine. Alimente [LOT-67](LOT-67-menus-vocabulaire-rpg.md), [LOT-68](LOT-68-chassis-ecrans-rpg.md), `LOT-69` (absorbé par le [LOT-88](LOT-88-retrait-heritage.md)), [LOT-76](LOT-76-habillage-interface.md).

Exigences refondues : [`EX-ARCH-022`](../../../../../Documentation/Specification/architecture.md#EX-ARCH-022) (racine), [`EX-DEC-003`](../../../../../Documentation/Specification/exigences-retirees.md#EX-DEC-003), [`EX-DEC-032`](../../../../../Documentation/Specification/exigences-retirees.md#EX-DEC-032), [`EX-DEC-043`](../../../../../Documentation/Specification/exigences-retirees.md#EX-DEC-043), [`EX-REN-032`](../../../../../Documentation/Specification/rendu-technique.md#EX-REN-032), [`EX-REN-041`](../../../../../Documentation/Specification/rendu-technique.md#EX-REN-041), [`EX-IHM-053`](../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-053), [`EX-IHM-070`](../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-070), [`EX-IHM-073`](../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-073), [`EX-EDIT-041`](../../../../../Documentation/Specification/editeur-niveaux.md#EX-EDIT-041), [`EX-EDIT-045`](../../../../../Documentation/Specification/editeur-niveaux.md#EX-EDIT-045). Exigences **tenues sans changement** : [`EX-IHM-071`](../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-071), [`EX-IHM-072`](../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-072).
