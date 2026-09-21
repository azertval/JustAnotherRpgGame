+++
id = "LOT-128"
titre = "Les cartes maquettes : dessiner et jouer sans texture"
version = "0.0.1"
filiere = "editeur"
statut = "livre"
taille = "L"
resume = "Une carte se dessine d'abord par sa physique — sols, murs, eau, portails, jetons — et se **joue** telle quelle, dans l'éditeur comme dans le jeu ; les textures viennent après, sans rien refaire."
prerequis = ["LOT-100"]
livrables = [
  "Une **primitive de couleur** dans la composition : un quad à quatre sommets libres (`hmi::PolyQuad`), que le GPU (`SpriteBatch`) et `QPainter` (`ScenePainter`) dessinent tous deux. C'est elle qui rend les losanges et les faces de bloc, sans texture ni fichier.",
  "Un **rendu de maquette** dans `SceneComposition`, partagé par le jeu et l'éditeur : une case sans pièce se dessine à la couleur de son type, et un type qui bloque (`wall`, `solid`, `cliff`) en **bloc extrudé** qui se lit comme un obstacle et masque ce qui est derrière. Il sert la carte sans lieu, et toute case qu'un lieu n'habille pas.",
  "La **palette de maquette** : les teintes des plans de principe du planning (pavé, ruelle, bâti, jardin, eau, sable, gradins), en une table nommée, à la place des couleurs saturées de l'atlas procédural.",
  "Les **jetons** : une entité sans figurine est un rond posé sur sa case, avec sa lettre — **vert** le joueur (entrée, points d'apparition), **jaune** le PNJ qui parle, **rouge** l'hostile (rencontre, entrée d'arène adverse), gris le PNJ muet ; portails en flèche, zones en contour. La couleur se déduit de l'entité, sans propriété nouvelle dans le format.",
  "Le jeu, l'essai immédiat (`P`) et l'essai complet (`F5`) **montrent** ce rendu : une carte sans aucune image se parcourt en voyant où l'on marche, ce qui bloque, qui est où et par où l'on sort.",
  "`--render` dessine les jetons, les portails et les zones ; `--render --plan` rend la carte **au vocabulaire des maquettes du planning** (losanges plats, pastilles à lettre, légende).",
  "« New map » sans lieu crée toujours ses couches (sol, relief), pour que la carte reçoive un lieu plus tard ; un modèle **Blockout** s'ajoute aux trois existants, et l'annexe connaît l'état `blockout`.",
  "Le guide d'usage : *maquetter, jouer, puis habiller* — de la carte sans lieu à `Change sheet…`.",
]
criteres = [
  "Le plan de principe de Martpart (24 × 11) est redessiné dans l'éditeur **sans une seule pièce**, en moins d'une séance, par l'auteur ; `--check` est vert ; son `--render --plan` **se lit comme** la maquette du planning : même projection, mêmes familles de teintes, mêmes pastilles, même légende.",
  "Cette carte se joue dans le vrai jeu (`F5`) : on distingue sol, murs et eau, on voit le jeton jaune de la mère, le jeton vert de l'apparition, la flèche du portail vers Arenarea, et on le franchit.",
  "La même carte reçoit ensuite un lieu par `Change sheet…` : collision, entités, portails et zones ne changent **pas d'un octet** ; seules les couches gagnent leur `scene`.",
  "Sur une carte **avec** lieu, un type que `appearance.json` ne couvre pas (`water`, `deepwater` aujourd'hui) n'est plus invisible : il prend le rendu de maquette, et `--check` le signale.",
  "Le jeu et l'éditeur produisent, pour la même carte, **la même liste de primitives** : un test compare les deux compositions, jetons compris.",
  "`EX-EXP-005` (« une carte lisible sans qu'aucun fichier d'image ne soit présent ») est tenue par un test de rendu hors écran, dans le jeu et dans l'éditeur.",
]
maquettes = ["../maquettes/plan-martpart.svg", "../maquettes/plan-arenarea.svg", "../maquettes/plan-arena-of-fate.svg"]
+++

## Pourquoi

Les plans de principe du planning disent *ce que la carte contient et comment on y circule, pas son
dessin*. C'est exactement ce qu'on veut pouvoir **jouer** avant de commander un seul asset : une
carte dont la physique est juste — où l'on marche, ce qui bloque, qui attend où, par où l'on sort —,
que les textures viennent habiller ensuite. Trois bénéfices :

- le **moteur de la quête** (LOT-116, LOT-117, LOT-118, LOT-119) n'attend plus les assets : la démo
  se joue de bout en bout en maquette pendant que les planches se produisent ;
- une **erreur de tracé** se paie en minutes, pas en pièces redessinées : on corrige la largeur
  d'une rue ou la place d'un escalier avant que l'art existe ;
- la **commande d'assets** d'une zone se déduit d'une maquette jouée, pas d'un schéma.

## Ce qui existe, ce qui manque

Relevé dans le code le 21 septembre 2026.

**Existe.** « New map » propose le lieu « (none: colored tile types) » ; la palette *Types* peint
les douze types de tuile ; la collision **se déduit du type** (`deriveCollision` : `wall` et `solid`
bloquent le pas et la vue, `cliff` et `deepwater` le pas) et suit chaque geste, sans case forcée ;
`--check` est vert sans lieu ; entités, zones et portails se posent normalement ; `Change sheet…`
donne un lieu à la carte sans refaire la physique.

**Manque.**

| # | Constat | Preuve |
|---|---|---|
| M1 | Dans le jeu, une carte sans lieu est un **écran uniforme** : ni sol ni mur, on bute sans savoir pourquoi. `EX-EXP-005` n'est pas tenue | `WorldPlay.cpp:70-77` charge une apparence vide, à dessein ; `composeFloor` / `composeRelief` n'ont aucun repli par type |
| M2 | L'essai immédiat (`P`) ne peint que la scène du jeu : même écran vide | `EditorViewport.cpp:503-508` |
| M3 | Un mur sans pièce est un **losange plat** : il ne masque rien et ne se lit pas comme un obstacle | `paintIsoTypeColors`, `EditorViewport.cpp:565-597` |
| M4 | Un PNJ sans `figure` n'est pas dessiné ; portails et zones ne le sont jamais dans le jeu | `WorldSceneComposer.cpp:205-207` |
| M5 | `--render` ne dessine aucune entité | `MapRender.cpp` |
| M6 | Avec un lieu, un type absent de `appearance.json` est invisible, sans damier ni alerte | `WorldSceneComposer.cpp:82-84` |
| M7 | « New map » sans lieu ni modèle donne une grille sans couche, que `changeScene` refuse ensuite | `LevelFileOperations.cpp:149-162`, `LevelDraft.cpp:628` |
| M8 | Aucun test ne couvre le cas sans lieu : rendu, essai, contrôle | — |

## Conception

**Un seul rendu, au bon endroit.** Le repli vit dans `SceneComposition`, que le jeu et l'éditeur
partagent : le composeur émet, pour une case sans pièce, une primitive **de couleur** (losange de
sol, ou bloc à trois faces pour un type qui bloque), triée en profondeur comme une pièce. Pas de
texture engendrée, pas d'asset : la maquette ne dépend d'aucun fichier. `paintIsoTypeColors`, propre
à l'éditeur, disparaît au profit de ce chemin.

**Une case sans pièce, quelle qu'en soit la raison.** Le repli ne se déclenche pas sur « la carte
n'a pas de lieu » mais sur « cette case n'a pas nommé de pièce ». Les deux manques se referment
alors du même geste : la carte sans lieu (M1) et le type qu'`appearance.json` ne couvre pas (M6)
sont le même cas, et il n'existe qu'un chemin à écrire et à tester.

**Maquetter, puis habiller.** La carte sans lieu est l'état de départ normal d'une carte, pas un
mode dégradé : l'annexe `.editor.json` la dit `blockout`, avant `retouched` et `finished`, et le
navigateur de cartes le montre. La définition de « livré » d'une carte ne change pas : une carte
livrée a son lieu et ses pièces.

## Décisions de planification

Prises le 21 septembre 2026, après relevé dans le code.

### D1 — La primitive est un quad à quatre sommets libres

`ComposedScene` ne connaissait que `SpriteQuad` (rectangle aligné sur les axes) et `LineQuad`
(segment orienté). Un losange isométrique n'est ni l'un ni l'autre : au rapport 0,62, ce n'est pas
un carré tourné. On ajoute donc `hmi::PolyQuad` — quatre sommets libres, une teinte —, et
`QuadKind::Poly` à côté de `Sprite` et `Line`.

Le pipeline ne change pas : un quad, ce sont déjà **quatre sommets** au format
`(x, y, u, v, r, g, b, a)`, et `SpriteBatch::draw(const LineQuad&)` en produit déjà quatre à des
positions libres. La texture liée est le **blanc 1 × 1** (`createTexture(context, 1, 1,
{0xFFFFFFFFU})`), l'idiome qu'`AssetGalleryRenderer` emploie déjà pour ses cadres : le culling, le
regroupement par texture et le tri restent ceux de tout le monde, et aucun lot n'invente un chemin
de dessin concurrent. Côté éditeur, `ScenePainter` traduit la primitive en `QPainter::drawPolygon`.

### D2 — Le jeton est une image engendrée, avec sa lettre

Il n'existe **aucun** rendu de texte en scène côté jeu : `RenderLayer::UI` ne porte que des
rectangles, et le LOT-88 a supprimé les polices pixel. Plutôt que d'en introduire un, le jeton
entier — disque coloré et lettre — est **peint en code pur** dans `SceneComposition`, exactement
comme `core::assetMarker` peint le marqueur d'un asset manquant : une image RVBA déterministe, que
le jeu téléverse en texture (`createTexture`) et que l'éditeur dessine en `QImage`.

Une table de glyphes 5 × 7 pour `A`–`Z` et `0`–`9` suffit, écrite dans le fichier. Le bénéfice est
la **parité par construction** : jeu et éditeur ne montrent pas deux dessins qui se ressemblent, ils
montrent la même image, et un seul chemin est à tester.

### D3 — La couleur du jeton se déduit de ce que le format dit déjà

La fiche annonçait « rouge : rencontre, **PNJ hostile**, entrée d'arène adverse ». Or
`core::MapEntity` n'a aucune notion d'hostilité : la famille `npc` ne déclare que `dialogue`,
`figure` et `guardedDistrict`. Et la « condition de quête » du jeton jaune n'existera qu'au
LOT-116. Aucune propriété n'est ajoutée au format pour combler l'écart — la table se règle sur ce
qui existe :

| Jeton | Couleur | Ce qui le décide, aujourd'hui |
|---|---|---|
| Joueur | **vert** | l'entrée de la carte, `spawnPoint`, `arenaEntry` de côté `allies` |
| PNJ qui parle | **jaune** | un `npc` qui porte une propriété `dialogue` |
| PNJ muet | gris clair | tout autre `npc` |
| Hostile | **rouge** | `encounter`, `arenaEntry` de côté `enemies` |
| Objet | gris-bleu | `chest`, `sign` |
| Portail | flèche, or | `portal` ; barrée s'il est condamné (LOT-126) |
| Zone | contour en pointillé | `combatZone` en rouge, `zone` en blanc, `cityBlock` en or |
| Trajet | ligne brisée, blanc | `route`, par ses points de passage |

La lettre du rond est tirée du nom de l'entité, comme les pastilles des plans du planning. Le
LOT-116 rebranchera le jaune sur la quête quand les drapeaux existeront ; c'est une ligne à
changer, pas une décision à reprendre.

Dans le jeu, le jeton ne paraît que si la figurine manque : une carte habillée ne montre aucun
jeton. Portails, zones et trajets ne se dessinent dans le jeu qu'**en maquette** (carte sans lieu) —
une carte finie ne montre pas ses déclencheurs.

### D4 — `--render --plan` se *lit comme* la maquette, il ne s'y superpose pas

Les trois plans du planning parlent un vocabulaire **propre à chaque lieu** : pavé, ruelle, bâti et
étals à Martpart ; sable, podium, gradins, coursive et tribunes à l'Arena of Fate. `core::TileType`
n'a que douze valeurs génériques : aucune palette générique ne distinguera jamais un gradin d'un
podium. Le critère est donc révisé — même projection (isométrie au rapport 0,62), mêmes familles de
teintes, mêmes pastilles, même légende, et non superposition au pixel. Martpart tombe juste
(pavé → `Dirt`, bâti → `Wall`, jardin → `Grass`, eau → `Water`) ; l'Arena of Fate approxime ses
gradins et son podium par `Solid`, et c'est assumé.

Une palette **par lieu** a été écartée : une carte sans lieu n'en aurait par définition aucune —
et c'est précisément le cas que le lot sert —, il en faudrait donc deux à tenir pour un gain qui ne
sert que le rendu de planning.

### D5 — La palette vit dans le code, pas dans un fichier

« En données » se lit ici : *une table nommée, au lieu des couleurs de l'atlas procédural*. Elle ne
devient pas un fichier : la maquette doit se dessiner quand **aucun** fichier d'asset n'est présent
(`EX-EXP-005`), et une palette chargée depuis le disque introduirait exactement la dépendance que
le lot supprime. C'est un **standard**, comme l'échelle ou le rapport du losange, pas du contenu.

Teintes de départ, tirées des plans du planning, à ajuster sur pièce :

| Type | Teinte | Type | Teinte |
|---|---|---|---|
| `Empty` | rien (case non dessinée) | `Water` | `#2f7f86` |
| `Grass` | `#3f6b34` | `DeepWater` | `#1d4f57` |
| `Dirt` | `#d9c7a3` | `Wall` | `#6b5a43`, bloc |
| `Sand` | `#c9a86a` | `Solid` | `#8d8272`, bloc |
| `Bridge` | `#8e6f45` | `Cliff` | `#5a5346`, bloc |
| `Stairs` | `#c9b48a` | `Entry` | `#d9c7a3` + jeton vert |

### D6 — Un bloc fait une case de haut, quel que soit le type

La hauteur réelle du relief reste hors périmètre. Tout type qui bloque s'extrude d'**une** hauteur
de losange, en trois faces (dessus, gauche, droite) éclairées différemment, triées au pied de la
case comme une pièce de relief. Un `cliff` à deux cases viendra avec la hauteur du format v4, qui
lui garde déjà sa place.

## Décisions de réalisation

### D7 — Le jeton est une marque, pas un objet du monde

Posé d'abord dans la bande de profondeur, comme la figurine qu'il remplace, le jeton se faisait
couper en deux par le premier mur d'en face — et un point d'apparition contre le bord de la carte,
ce qu'est justement la Market Gate de Martpart, devenait illisible. Il vit donc sur le calque de
l'**interface en scène**, avec les tracés : c'est une pastille sur un plan, comme celles des plans
du planning, et une pastille à demi cachée ne fait plus son seul travail — dire où est le PNJ.

### D8 — La lettre vient du nom, à défaut du **type**, jamais de l'identifiant

Les identifiants d'entité s'écrivent tous `e<numéro>` : les tirer de là donnait un `E` à tous les
jetons. À défaut d'étiquette, c'est le type qui parle — `N` pour un PNJ, `C` pour un coffre.

### D9 — Un jeton est peint ou absent, jamais un damier

`ScenePieceTextures::resolve` retombe sur le damier pour toute pièce manquante ; un damier à la
place d'un jeton ne dirait rien et se ferait passer pour une planche oubliée. Les jetons passent
donc par `find`, sans repli — ce qui fait aussi qu'un rendu qui les ignore, comme l'arrière-plan de
combat de l'arène, n'en hérite pas.

## Plan de réalisation

Six phases ; chacune se construit et se teste seule.

| # | Phase | Ce qui change | Ce qu'on voit |
|---|---|---|---|
| 1 | **La primitive** | `PolyQuad` dans `Quad.h`, `QuadKind::Poly` et `ComposedScene::addPoly`, `SpriteBatch::draw`, `ScenePainter`, `QuadRecorder` ; tests de culling et de tri | rien — aucun appelant |
| 2 | **La palette et le sol** | `MaquettePalette.h` ; `WorldSceneSnapshot` porte le **type** de chaque case ; une case sans pièce prend son losange ; `paintIsoTypeColors` supprimé | M1, M6 : le canevas et le jeu montrent les sols |
| 3 | **Les blocs** | `wall`, `solid`, `cliff` en bloc à trois faces, triés au pied | M3 : un mur masque ce qui est derrière |
| 4 | **Les jetons** | `maquetteToken()` pur (disque, glyphe 5 × 7), la table D3, flèches de portail, contours de zone ; le snapshot les porte | M4 : qui est où, et par où l'on sort |
| 5 | **Le jeu et les essais** | `WorldPlay` sans lieu ne se tait plus ; `P` et `F5` montrent la maquette ; `--check` signale un type non couvert ; test de rendu hors écran et test de parité jeu / éditeur | M2, M8, `EX-EXP-005` |
| 6 | **L'atelier** | « New map » sans lieu crée ses couches, modèle *Blockout*, `MapState::Blockout` dans l'annexe et le navigateur ; `--render` dessine les entités, `--plan` ajoute la légende ; le guide *maquetter, jouer, habiller* | M5, M7 |

La recette est le critère : l'auteur redessine Martpart en maquette, en moins d'une séance, et la
joue.

## Ce qui reste à l'auteur

Tout est écrit et vérifié par les tests, **sauf le premier critère**, qui ne peut l'être que par
l'auteur : redessiner le plan de Martpart en maquette, en moins d'une séance, le jouer par `F5`,
puis lui donner son lieu par `Change sheet…`. C'est la recette du lot, et elle vaut aussi pour la
vérification à la souris due depuis les lots de l'éditeur. Le reste — rendu, jetons, `--check`,
`--plan`, parité jeu/éditeur, `EX-EXP-005` — tient par les tests.

## Périmètre

**Pas dedans** : le combat sur une carte en maquette — il vient avec le LOT-118, qui héritera du
rendu ; la hauteur réelle des murs (D6) ; toute texture engendrée ; le rendu de texte en scène,
que D2 contourne.

## Risques

- La parité éditeur / jeu se joue sur une primitive **nouvelle** : `SpriteBatch` côté GPU et
  `ScenePainter` côté `QPainter` doivent tous deux l'apprendre. C'est la phase 1, isolée exprès, et
  le test de parité de la phase 5 la garde.
- Le LOT-103 touche le même composeur : l'ordre entre les deux est libre, mais le second se
  rebase sur le premier.
- `WorldSceneSnapshot` gagne deux champs (types, jetons) : c'est une structure de **valeurs**
  comparée par `operator==` et prise dans `synchronize()`, donc sans risque de fil, mais les
  instantanés de test existants sont à relire.
