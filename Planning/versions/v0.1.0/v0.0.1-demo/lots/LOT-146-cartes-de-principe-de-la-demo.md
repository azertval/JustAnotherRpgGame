+++
id = "LOT-146"
titre = "Les trois cartes de principe de la démo"
version = "0.0.1"
filiere = "cartes"
statut = "livre"
taille = "M"
resume = "La démo se traverse de bout en bout — Martpart, Arenarea, l'Arena of Fate — sur trois cartes de principe enchaînées, jouées en maquette, sans qu'une seule pièce soit produite."
prerequis = ["LOT-103", "LOT-118", "LOT-124", "LOT-125", "LOT-126", "LOT-127", "LOT-128"]
livrables = [
  "`Levels/central-empire/capital/martpart.json` : le plan de principe de Martpart (24 × 11 cases), dessiné **dans l'éditeur** — Market Gate, la place des étals, Stravian Avenue jusqu'au portail d'Arenarea.",
  "`Levels/central-empire/capital/arenarea.json` : le plan de principe d'Arenarea (24 × 13 cases) — Herofate Avenue du portail de Martpart au parvis, l'escalier de l'arène, la façade du casino. Il prend l'identifiant de la carte du `LOT-109`, qui n'est pas la carte de la démo (décision D-25).",
  "`Levels/central-empire/capital/arenarea/arena-of-fate.json` et `arena-of-fate/undercroft.json` : le sable et sa zone de combat, et le niveau −1 réduit à ce que la quête traverse — le vestiaire A et l'escalier de la porte du triomphe (un niveau est une carte, D-21).",
  "Sur chaque carte : portails et points d'arrivée nommés (`from-martpart`, `from-arenarea`, `from-arena-of-fate`), la zone de combat du sable, la zone du parvis et son déclencheur, les portes de l'arène closes sous `condamne` (`prop`, LOT-126).",
  "Les cinq PNJ de la quête à leur place, avec leur condition de présence : la mère, le garde, l'enfant, le maître d'arène, le combattant — des **jetons** (LOT-128) que les **mannequins** (LOT-145) remplacent dès qu'ils sont installés, sans toucher à la carte.",
  "Le rendu `--render --plan` de chaque carte, joint à la PR, et l'entrée de chaque carte dans le catalogue des lieux.",
]
criteres = [
  "On traverse la démo en chaîne, dans le vrai jeu : Market Gate → Stravian Avenue → parvis d'Arenarea → vestiaire A → sable, et retour jusqu'à l'étal de la mère, par les seuls portails.",
  "`LevelEditor --check` passe sur les quatre cartes : aucune case inatteignable, aucun portail sans arrivée, aucune référence morte ; le vestiaire A est atteint par le transfert du parvis.",
  "Le `--render --plan` de chaque carte **se lit comme** son plan de principe : mêmes lieux, mêmes seuils, mêmes pastilles.",
  "Une rencontre se joue sur le sable et rend à l'exploration (LOT-118) ; le garde et l'enfant paraissent sur le parvis sous `acceptee` et le quittent sous `enfant-libere`.",
  "Aucune pièce d'asset n'est produite ni retouchée par le lot ; une carte habillée ensuite par `Change sheet…` ne change pas d'un octet en collision, entités, portails et zones.",
]
maquettes = ["../maquettes/plan-martpart.svg", "../maquettes/plan-arenarea.svg", "../maquettes/plan-arena-of-fate.svg"]
+++

## Pourquoi

La démo tenait trois lieux **habillés** : assets, cartes et PNJ de chaque zone, en six lots. Ces
lots sont du *world building* — un quartier entier, des centaines de pièces, une foule aux
proportions du livre — et l'expérience d'Arenarea (`LOT-108`, `LOT-109`) a montré qu'une zone au
standard du jeu final n'est pas encore à portée d'un lot de démo. La décision
[D-25](../../../../vision/decisions.md) les reporte à la `0.0.3`, où ils s'inscrivent avec les six
autres quartiers.

La démo garde ses **trois cartes enchaînées**, fortement réduites : ce que les plans de principe
montrent, rien de plus. Le [LOT-128](LOT-128-cartes-maquettes.md) a été fait pour cela — une carte se
dessine par sa physique et se **joue** sans texture. Ce lot en est l'application à la démo : un
seul lot pour les trois cartes, qui donne au moteur de la quête (`LOT-116`, `LOT-117`, `LOT-118`,
`LOT-119`) et à la quête elle-même (`LOT-120`) le terrain dont ils ont besoin.

## Périmètre

**Dedans** : la physique des trois lieux — sols, murs, eau, portails, zones, points d'apparition —,
les entités de la quête avec leurs conditions, la zone de combat du sable, le déclencheur du parvis.
Les cartes se dessinent dans l'éditeur et se contrôlent par `--check`.

**Pas dedans** : la moindre pièce d'asset — ni produite, ni retouchée. Les figurines des PNJ
(`LOT-113`, `LOT-114`, `LOT-115`), les pièces propres (`LOT-106`, `LOT-110`) et les cartes
définitives (`LOT-107`, `LOT-111`, `LOT-147`) sont à la `0.0.3`. Les dialogues et l'équilibrage
sont au `LOT-120` ; les images de l'onglet « Carte », au `LOT-121`.

**Facultatif, sans être un critère** : habiller une carte de principe avec ce qui existe déjà —
le kit commun (`LOT-105`), les dix-huit pièces du Colisée installées par le `LOT-104`, les pièces
d'Arenarea (`LOT-108`) — par `Change sheet…`. Rien n'oblige à le faire, et rien ne se produit pour
le faire.

## Conception

### Les trois cartes, à l'échelle des plans

| Carte | Cases | Ce qu'elle contient | Plan |
|---|---|---|---|
| Martpart | 24 × 11 | Market Gate (apparition), la place des étals et la mère, Stravian Avenue → Arenarea | [plan](../maquettes/plan-martpart.svg) |
| Arenarea | 24 × 13 | Herofate Avenue ← Martpart, le parvis (garde, enfant, déclencheur), l'escalier → Arena of Fate, la façade du casino | [plan](../maquettes/plan-arenarea.svg) |
| Arena of Fate, sable | 34 × 24 | l'ovale de sable (zone de combat, 22 × 14), les trois anneaux en décor non parcouru | [plan](../maquettes/plan-arena-of-fate.svg) |
| Arena of Fate, niveau −1 | au tracé | le vestiaire A (maître d'arène, arrivée du condamné), le couloir, l'escalier de la porte du triomphe | même plan |

Les plans sont des **schémas de principe** : ils fixent ce que la carte contient et comment on y
circule, pas son dessin — et c'est exactement ce que la carte de principe reproduit. Le niveau −1
se réduit à ce que la quête traverse : ni vestiaire B, ni prison, ni escalier des catacombes ; ils
viennent avec la carte définitive (`LOT-107`).

### Les PNJ : jetons, puis mannequins

Un PNJ sans figurine est un **jeton** (`LOT-128`) : jaune s'il parle, gris sinon, rouge s'il est
hostile. Dès que le mannequin de sa silhouette est installé (`LOT-145`), il le remplace, et se
dessine, marche, attaque et tombe ; la carte ne change pas. Le jour où sa figurine arrive
(`0.0.3`), seul le manifeste `Characters/` change. Les cinq PNJ de la quête sont donc posés ici, une
fois pour toutes, avec leurs conditions de présence ; leurs répliques sont au `LOT-120`.

### La carte d'Arenarea du `LOT-109`

La carte livrée par le `LOT-109` couvre le quartier entier — 128 × 88 cases — et son habillage,
comme les pièces du `LOT-108`, est loin du standard voulu pour le jeu final (D-25). Elle n'est pas
la carte de la démo. La carte de principe prend son identifiant, `central-empire/capital/arenarea`,
pour que le plan de la Capitale et les portails la trouvent sans changement. La carte du `LOT-109`
reste dans l'historique et dans l'atelier (`Carte109/`) ; elle revient, reprise, avec le
[LOT-147](../../v0.0.3-capitale-intra-muros/lots/LOT-147-zone-arenarea-reprise.md). Le lot décide
sur pièce s'il tire la carte de principe d'une découpe de cette carte ou la redessine sur le plan :
le plan fait foi.

### Ce que la quête demande

Tout ce que « Des pommes pour l'arène » pose sur une carte est déjà dans l'éditeur
([LOT-126](LOT-126-ce-que-la-quete-demande-aux-cartes.md)) : la condition de présence du garde et de
l'enfant, les portes de l'arène closes sous `condamne`, le transfert du parvis vers le vestiaire A,
la zone de combat. Ce lot les **pose** ; le `LOT-120` les fait jouer.

## Risques et questions ouvertes

- Le `LOT-127` a redessiné Martpart en maquette pour la recette de l'éditeur : si cette carte
  existe encore, elle est le point de départ, pas une carte à refaire.
- Quatre cartes en maquette, c'est peu de dessin et beaucoup de branchements (portails, arrivées,
  déclencheurs, conditions) : la chaîne se vérifie **dans le jeu**, pas seulement par `--check`.
- L'ovale de sable en 34 × 24 avec ses anneaux en décor est la seule carte « grande » ; si elle
  coûte, le sable seul suffit à la démo.

## Décisions de réalisation

Livré le 25 septembre 2026, avec le [LOT-120](LOT-120-quete-des-pommes-pour-l-arene.md) dans la
même branche, sur décision de l'auteur.

1. **Quatre cartes, dessinées par `--apply`, aucune pièce.** La carte de Martpart du `LOT-127`
   n'existait plus (table rase) : les quatre cartes sont dessinées depuis les plans, en types de
   tuile seulement (`pavement`, `alley`, `grass`, `water`, `stall`, `wall` ; `sand`, `tiers`,
   `solid`, `column`, `stairs`, `door` ; `flagstone`), par des fichiers de gestes que l'éditeur
   rejoue — l'annexe `annexes/LOT-146-cartes-de-principe/gestes/`. Un squelette de carte neuve
   (celui de « New map » sans lieu) reçoit les gestes ; le fichier livré est celui que l'éditeur
   écrit. L'ovale entier est dessiné (34 × 24), ses anneaux en décor.
2. **Le maître d'arène se tient sur le sable, pas au vestiaire.** Un dialogue engage la rencontre
   sur la zone de combat où se tient le PNJ (`LOT-118`, D6), et une entité `encounter` posée sur
   la carte ne se déclenche pas en marchant : le vestiaire A n'a pas de zone de combat. Le
   vestiaire garde l'arrivée du condamné et la porte close ; le maître attend sur le sable, le
   combattant (marqueur `encounter`) deux cases à sa droite, là où la rencontre le fait paraître.
3. **Pas de transfert de zone vers le vestiaire.** La zone du parvis déclenche le dialogue du
   garde à l'entrée ; un transfert de zone se joue aussi à l'entrée, avant que le dialogue ait
   décidé, et emmènerait au vestiaire celui qui vient de convaincre le garde. Le condamné marche
   jusqu'à l'escalier de l'arène — « suivez-moi » —, arrive au vestiaire derrière la porte close
   (`prop` sous `condamne`), et ne peut plus revenir. Le critère « le vestiaire A est atteint par
   le transfert du parvis » est tenu par le portail de l'escalier, que `--check` suit de même.
4. **Le plan de principe couche aussi les blocs de la couche de décor.** `--render --plan`
   n'aplatissait que les blocs du sol ; une carte neuve met ses murs sur `relief`, et le plan les
   taisait. Corrigé dans le composeur (`composeRelief`), avec son test.
5. **Market Gate est un point d'arrivée nommé** (`market-gate`) : le plan de ville l'ouvre pour
   « Nouvelle partie » (`LOT-120`).
6. **Le test des cartes livrées admet une carte sans pièce** : la retouche qu'il rejoue gomme le
   premier bloc typé du décor quand aucune pièce n'est nommée.

## Livraison

Les quatre cartes, leurs gestes, les clés de nom dans les deux catalogues, le README de la
Capitale. Critères : la chaîne se traverse dans le vrai jeu (`SystemGameTests`, `LOT-120`) et
sans fenêtre (`IntegrationTests`, `QueteDesPommes`) ; `--check` vert, 0 avertissement ; les rendus
`--render --plan` joints à la PR ; aucune pièce produite.
