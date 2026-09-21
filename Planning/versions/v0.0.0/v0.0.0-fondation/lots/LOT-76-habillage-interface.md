+++
id = "LOT-76"
titre = "Habillage d'interface extrait des livres"
version = "0.0.0"
filiere = "interface"
statut = "livre"
taille = "M"
resume = "Les écrans du jeu portent la pierre sertie à l'angle et le bandeau de titre à ailes des pages de Tanares, tracés et non découpés, donc dynamiques et tenus par les jetons."
prerequis = [
  "LOT-30",
  "LOT-66",
]
livrables = [
  "`hmi::cabochonShapes` : le cabochon tracé, posé sur les quatre angles de `ParchmentPanel` (mode normal).",
  "`hmi::titleBannerShapes` et `hmi::TitleBanner` : le bandeau à ailes, auquel les six `QLabel` de titre sont promus.",
  "Deux jetons neufs relevés sur le corpus : `identityTokens().color.gem` (`#701010`) et `gemShadow` (`#400000`).",
  "Les constantes `WING_SPAN_RATIO`, `PARCHMENT_CABOCHON_FACTOR` et `ORNAMENT_MINIMUM_SIZE`.",
  "L'exigence `EX-IHM-075` : l'habillage se trace et ne se livre pas en image.",
]
criteres = [
  "**Les éléments s'intègrent aux jetons de la charte du `LOT-66`** : chaque forme porte un rôle, et aucune couleur n'est écrite hors de `DesignTokens.cpp`.",
  "**`scripts/check_design_tokens.py` reste vert** — onze couleurs vérifiées entre la planche et le code, deux de plus qu'avant.",
  "**Aucune image n'est tirée par extraction de flux brut** — aucune image n'est tirée du tout.",
  "**L'ornement est dynamique** : les ailes d'un bandeau gardent leur envergure de 600 à 1 400 px de large, et le cabochon se redessine à toute taille au-dessus de huit pixels.",
  "`ctest` : **1032/1032** (1021 avant, plus 11).",
]
+++

## Pourquoi

Sortir du vocabulaire d'aplats. Le [LOT-66](@ref lot-66) avait donné aux écrans du jeu la palette
du parchemin et un encadrement à trois bandes ; il leur manquait ce qui fait qu'une page de Tanares
se reconnaît en une seconde : **la pierre sertie à l'angle et le bandeau de titre à ailes**. Ce lot
les livre, et il les livre **tracés**.

## Conception

### Le renversement du milieu du lot, et pourquoi il valait mieux

Le titre de ce lot dit « extrait des livres », et la feuille de route le décrivait comme un
découpage : *« panneaux de parchemin, cadres, bordures, pictogrammes… sortent parfaitement par
rendu clippé »*. Cette voie a été construite en entier — vingt et une planches PNG à 300 ppp, un
manifeste, un lint d'intégrité — puis **abandonnée**, sur la seule preuve qui compte : l'écran.

Un cabochon de 108 pixels posé sur le panneau des crédits, haut de 340, mangeait le tiers de sa
hauteur et recouvrait la première ligne. Le corriger demandait de découpler la taille de découpe de
la taille de pose ; à ce point-là, on n'étirait plus une image, on la **redessinait mal**. Et deux
défauts de plus attendaient derrière :

- **le bandeau ne peut pas s'étirer honnêtement.** Sa plaque doit s'allonger avec le titre, ses
  ailes doivent rester à la taille de leur hauteur. Aucun découpage en tranches ne fait les deux :
  neuf tranches étirent tout ce qui n'est pas un coin ;
- **une image fige ses couleurs hors des jetons** (`EX-IHM-051`). C'était déjà l'argument du
  `LOT-66`, et il n'avait pas cessé d'être vrai — il avait seulement été contourné.

Le lot livre donc ce que la feuille de route voulait — *l'habillage du livre dans le jeu* — par le
moyen que la feuille de route n'avait pas envisagé. `EX-IHM-075` écrit noir sur blanc la conclusion,
pour que la voie du découpage ne soit pas réessayée dans trois lots.

### Ce que le corpus donne encore : la mesure, pas la matière

Rien de l'analyse n'est perdu, et c'est ce qui rend le tracé légitime plutôt qu'inventé
(`EX-IHM-070`) :

| Relevé | Valeur | Où il vit |
|---|---|---|
| Grenat du cabochon | `#701010` | `identityTokens().color.gem` |
| Grenat profond de la plaque | `#400000` | `identityTokens().color.gemShadow` |
| Or du filet | `#b09040` | déjà couvert par `frameOrnament` / `accent` |
| Envergure d'une aile | 1,6 × la hauteur de la plaque | `WING_SPAN_RATIO` |
| Débord de la pierre sur l'angle | 2 × l'épaisseur du filet | `PARCHMENT_CABOCHON_FACTOR` |

Le grenat mérite un mot. Le `LOT-66` signalait `error` comme le **seul** rôle non attesté par le
corpus — *« une feuille de personnage n'a pas d'état d'erreur à montrer »*. C'était vrai d'un état
d'erreur, et faux du rouge : il est là, dans les cabochons et les plaques de bandeau. Sa valeur est
la dominante quantifiée des pixels franchement rouges d'un cabochon, mesurée **séparément sur deux
angles opposés** de la planche, qui donnent la même valeur — la même méthode que le `LOT-66`, avec
la même vérification croisée.

### Deux ornements, deux invariants

**Le cabochon** (`hmi::cabochonShapes`) — un octogone d'or, un losange de grenat à deux facettes,
un éclat. L'octogone plutôt qu'un carré : ce sont les angles coupés qui se lisent comme les griffes
tenant la pierre, et ils survivent à la petite taille quand les facettes internes, elles, ne se
lisent plus. Il **déborde** du carré d'angle du `LOT-66` — d'un facteur deux — parce qu'à la taille
exacte du filet il s'y confond : posé sur l'angle, pas encastré dedans.

**Le bandeau** (`hmi::titleBannerShapes`) — une plaque de grenat à chevrons entre deux ailes d'or.
Son invariant est le cœur du lot : **l'envergure des ailes suit la hauteur, jamais la largeur**. Un
titre long allonge la plaque et rien d'autre. C'est exactement ce qu'une image étirée ne sait pas
faire, et un test le vérifie sur trois largeurs.

Quand la largeur ne suffit plus, ce sont les **ailes** qui cèdent — d'abord en se réduisant, puis
en disparaissant sous `ORNAMENT_MINIMUM_SIZE`. Jamais la plaque : c'est elle qui porte le titre, et
un moignon de six pixels ne se lit pas comme une aile mais comme une bavure.

### L'accentué reste tracé à l'ancienne, et ce n'est pas un oubli

`ParchmentPanel` a deux modes. Le normal porte désormais ses quatre cabochons. L'**accentué** —
pause, écrans superposés — garde ses carrés d'or nus : son filet passe à la couleur d'accent pour
signaler que l'écran se superpose au jeu, et une pierre dessinée par-dessus rendrait ce signal
illisible. Un ornement qui masque une information n'est pas un ornement, c'est un défaut.

### Le titre devient son bandeau, sur les six écrans

Les six `QLabel` de titre sont promus `hmi::TitleBanner`. Trois conséquences valaient d'être
tranchées plutôt que découvertes :

- **le sélecteur de style ne bouge pas.** La feuille écrit `QLabel#menuTitle` ; un sélecteur de
  type Qt s'applique aux sous-classes. Le réécrire aurait décrit la **classe** au lieu du rôle ;
- **le texte tient entre les ailes**, par `contentsMargins` et non par un décalage au moment de
  peindre — sinon l'élision et le retour à la ligne décident sur la mauvaise largeur, et le mot
  coupé n'apparaît que sur le titre le plus long, c'est-à-dire jamais pendant qu'on le règle ;
- **la couleur du titre change**, de l'or des filets à l'or **pâle** des reflets : l'ancien tenait
  sur du parchemin et disparaît sur le grenat. C'est la même valeur qui éclaire la facette du
  cabochon — pour que le doré du bandeau soit *un* doré, et non deux.

## Ce que le lot ne fait pas

**Il ne livre pas de pictogrammes.** La feuille de route en annonçait : les seize médaillons
d'emplacement d'équipement des feuilles de personnage. Ce sont des **illustrations** — une armure
de plates peinte, une bourse de cuir — pas de l'habillage, et le tracé n'est pas le bon outil pour
elles. Elles relèvent de la filière d'assets du `LOT-39`, avec les autres illustrations du corpus,
et non d'un module de géométrie.

**Il ne dessine pas de cadre de champ ni de médaillon rond.** Les deux seraient traçables et
utiles ; aucun n'a de consommateur aujourd'hui. Le châssis d'écrans du `LOT-68` et l'inventaire du
[LOT-14](@ref lot-14) les demanderont, avec la taille et le comportement qu'ils leur faudront —
livrés d'avance, ils seraient devinés.

**Il ne redessine pas les planches de `.design-mockups/`.** Elles reçoivent les deux jetons neufs,
que `check_design_tokens.py` exige ; les ornements eux-mêmes relèvent du `LOT-68`, qui porte les
planches.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1032/1032, `clang-format`, les six lints, cahier de test et Doxygen verts ; tous les critères d'acceptation sont cochés dans l'epic d'origine. Alimente `LOT-38`, [LOT-15](@ref lot-15), [LOT-24](@ref lot-24).

Exigence ajoutée : [`EX-IHM-075`](@ref EX-IHM-075). Exigences tenues sans changement : [`EX-IHM-051`](@ref EX-IHM-051) (aucune couleur hors des jetons), [`EX-IHM-070`](@ref EX-IHM-070) (identité du parchemin, teintes relevées), [`EX-IHM-081`](@ref EX-IHM-081) (facteur d'agrandissement entier).
