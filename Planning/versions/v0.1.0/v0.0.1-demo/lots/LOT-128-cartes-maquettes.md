+++
id = "LOT-128"
titre = "Les cartes maquettes : dessiner et jouer sans texture"
version = "0.0.1"
filiere = "editeur"
statut = "a-faire"
taille = "M"
resume = "Une carte se dessine d'abord par sa physique — sols, murs, eau, portails, jetons — et se **joue** telle quelle, dans l'éditeur comme dans le jeu ; les textures viennent après, sans rien refaire."
prerequis = ["LOT-100"]
livrables = [
  "Un **rendu de maquette** dans `SceneComposition`, partagé par le jeu et l'éditeur : une case sans pièce se dessine à la couleur de son type, et un type qui bloque (`wall`, `solid`, `cliff`) en **bloc extrudé** qui se lit comme un obstacle et masque ce qui est derrière. Il sert la carte sans lieu, et toute case qu'un lieu n'habille pas.",
  "La **palette de maquette** : les teintes des plans de principe du planning (pavé, ruelle, bâti, jardin, eau, sable, gradins), en données, à la place des couleurs saturées de l'atlas procédural.",
  "Les **jetons** : une entité sans figurine est un rond posé sur sa case, avec sa lettre — **vert** le joueur (entrée, points d'apparition), **jaune** le PNJ de quête, **rouge** l'hostile (PNJ hostile, rencontre), gris le PNJ neutre ; portails en flèche, zones en contour. La couleur se déduit de l'entité, sans propriété nouvelle dans le format.",
  "Le jeu, l'essai immédiat (`P`) et l'essai complet (`F5`) **montrent** ce rendu : une carte sans aucune image se parcourt en voyant où l'on marche, ce qui bloque, qui est où et par où l'on sort.",
  "`--render` dessine les jetons, les portails et les zones ; `--render --plan` rend la carte **au vocabulaire des maquettes du planning** (losanges plats, pastilles à lettre, légende).",
  "« New map » sans lieu crée toujours ses couches (sol, relief), pour que la carte reçoive un lieu plus tard ; un modèle **Blockout** s'ajoute aux trois existants.",
  "Le guide d'usage : *maquetter, jouer, puis habiller* — de la carte sans lieu à `Change sheet…`.",
]
criteres = [
  "Le plan de principe de Martpart (24 × 11) est redessiné dans l'éditeur **sans une seule pièce**, en moins d'une séance, par l'auteur ; `--check` est vert ; son `--render --plan` se superpose à la maquette du planning.",
  "Cette carte se joue dans le vrai jeu (`F5`) : on distingue sol, murs et eau, on voit le jeton jaune de la mère, le jeton vert de l'apparition, la flèche du portail vers Arenarea, et on le franchit.",
  "La même carte reçoit ensuite un lieu par `Change sheet…` : collision, entités, portails et zones ne changent **pas d'un octet** ; seules les couches gagnent leur `scene`.",
  "Sur une carte **avec** lieu, un type que `appearance.json` ne couvre pas (`water`, `deepwater` aujourd'hui) n'est plus invisible : il prend le rendu de maquette, et `--check` le signale.",
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

**Les jetons** (décision de l'auteur, 21 sept. 2026) :

| Jeton | Couleur | Entités |
|---|---|---|
| Joueur | **vert** | l'entrée de la carte, les `spawnPoint`, les `arenaEntry` du camp allié |
| PNJ de quête | **jaune** | un `npc` qui porte un dialogue ou une condition de quête |
| Hostile | **rouge** | un `encounter`, un `npc` hostile, les `arenaEntry` du camp adverse |
| PNJ neutre | gris clair *(proposé)* | tout autre `npc` |
| Portail | flèche, or *(proposé)* | `portal` ; barrée s'il est condamné (LOT-126) |
| Zone | contour en pointillé *(proposé)* | `combatZone` en rouge, `zone` en blanc |

Le rond porte une **lettre** tirée du nom de l'entité, comme les pastilles des plans du planning.
Dans le jeu, le jeton ne paraît que si la figurine manque : une carte habillée ne montre aucun jeton.
Portails et zones ne se dessinent dans le jeu qu'**en maquette** (carte sans lieu) — une carte
finie ne montre pas ses déclencheurs.

**Maquetter, puis habiller.** La carte sans lieu est l'état de départ normal d'une carte, pas un
mode dégradé : l'annexe `.editor.json` la dit `blockout`, avant `retouched` et `finished`, et le
navigateur de cartes le montre. La définition de « livré » d'une carte ne change pas : une carte
livrée a son lieu et ses pièces.

## Périmètre

**Pas dedans** : le combat sur une carte en maquette — il vient avec le LOT-118, qui héritera du
rendu ; la hauteur réelle des murs (un bloc fait une case de haut, deux pour `cliff`) ; toute
texture engendrée.

## Risques

- La parité éditeur / jeu se joue sur une primitive **nouvelle** (un polygone de couleur, pas un
  sprite) : `SpriteBatch` côté GPU et `ScenePainter` côté `QPainter` doivent tous deux l'apprendre.
- Le LOT-103 touche le même composeur : l'ordre entre les deux est libre, mais le second se
  rebase sur le premier.
