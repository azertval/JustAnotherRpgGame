+++
id = "LOT-22"
titre = "Portée, ligne de vue et zones d'effet"
version = "0.0.0"
filiere = "regles"
statut = "livre"
taille = "L"
resume = "La géométrie tactique compte : portée d'arme, ligne de vue symétrique, abri et gabarits de zone font jouer le combat avec le terrain."
prerequis = ["LOT-19", "LOT-21"]
livrables = [
  "`Source/Core/Combat/LineOfSight.{h,cpp}` : `core::GridPoint`, `core::isSightClear`, `core::hasLineOfSight`, `core::coverFrom`, `core::coverFromPoint`, `core::coverBetween`.",
  "`Source/Core/Combat/AreaOfEffect.{h,cpp}` : `core::AreaShape`, `core::areaTemplate`, `core::affectedCells`, `core::combatantsInArea`, `core::areaTilesFromMeters`.",
  "`core::BattleGrid::blocksSight` et `core::GridObject::cover`.",
  "`core::checkTarget`, `core::AttackRoll::applyCover`, `core::thrownAttackFor`.",
  "Les propriétés structurées des 37 armes et les portées du bestiaire, tirées à l'extraction (`scripts/sourcebook/equipement.py`, `bestiaire.py`).",
  "Dans l'arène : `core::ArenaActionResult::TotalCover`, l'esquive et l'opportunité soumises à la vue, l'arme lancée proposée.",
]
criteres = [
  "Symétrie de la ligne de vue vérifiée exhaustivement sur des grilles générées.",
  "Les gabarits couvrent exactement les cases attendues, figées par des cas de référence.",
  "La couverture modifie la CA du montant prévu, et ne s'applique jamais deux fois.",
  "Testable headless.",
  "`ctest` : 1130/1130 (1120 avant ; trois tests de ligne de vue, trois de zones, trois d'attaque, un d'arène).",
]
+++

## Pourquoi

Rendre la **géométrie** tactique signifiante : portée d'arme, ligne de vue bloquée, abri, gabarits
d'effet de zone. C'est ce lot qui fait qu'un combat se joue avec le terrain plutôt que sur une
grille vide.

Le **Manuel des Joueurs** fait foi pour chaque règle qu'il écrit : chapitre 9 (« Attaques à
distance », « Portée », « Abri », « Attaques d'opportunité », « Esquiver »), chapitre 10 (« Un
chemin dégagé jusqu'à la cible », « Zones d'effet »), chapitre 5 (« Allonge », « Lancer »). Il ne
dit pas comment les mesurer sur un quadrillage : ce que le lot a tranché là est nommé ci-dessous.

## Périmètre

- **La ligne de vue et l'abri** (`Source/Core/Combat/LineOfSight.{h,cpp}`) : des points de grille en
  demi-cases (`core::GridPoint`), le segment dégagé (`core::isSightClear`), la vue entre deux
  emprises (`core::hasLineOfSight`), l'abri contre une emprise ou contre un point d'origine
  (`core::coverFrom`, `core::coverFromPoint`), et entre deux combattants (`core::coverBetween`).
- **Les zones d'effet** (`Source/Core/Combat/AreaOfEffect.{h,cpp}`) : les cinq formes du Manuel
  (`core::AreaShape`), le gabarit seul (`core::areaTemplate`), les cases atteintes une fois les murs
  comptés (`core::affectedCells`), les combattants touchés (`core::combatantsInArea`), et la
  conversion des tailles du corpus (`core::areaTilesFromMeters`).
- **Sur la grille** : ce qui arrête la vue (`core::BattleGrid::blocksSight`) et l'abri qu'un objet
  déclare (`core::GridObject::cover`, `core::Cover`).
- **Dans l'attaque** : le ciblage (`core::checkTarget` : la distance, puis la vue), l'abri posé sur
  le jet une fois et une seule (`core::AttackRoll::applyCover`), le désavantage du tir au contact
  réservé à un ennemi « qui vous voit », l'allonge des armes `reach`, les portées des armes et des
  actions du bestiaire, et l'attaque d'une arme **lancée** (`core::thrownAttackFor`).
- **Dans la donnée** : les **propriétés structurées** des 37 armes (`properties`, `versatileDamage`,
  `rangeNormal`, `rangeLong`) et les portées des actions à distance du bestiaire (`rangeNormal`,
  `rangeLong`, ajoutés au schéma des créatures), tirées de la prose **à l'extraction**
  (`scripts/sourcebook/equipement.py`, `bestiaire.py`) — le moteur ne lit jamais « portée
  24/96 m ».
- **Dans l'arène** : l'attaque refusée sous abri total (`core::ArenaActionResult::TotalCover`),
  l'esquive qui ne désavantage qu'un attaquant vu, l'attaque d'opportunité réservée à un fuyard
  « situé dans votre champ de vision ». L'écran choisit la première attaque qui peut viser la cible
  — l'arc du squelette tire enfin — et propose l'arme lancée d'un personnage qui en porte une.

Il ne livre **pas** les sorts qui emploient ces zones, ni les jets de sauvegarde de Dextérité
auxquels l'abri s'ajoute (`core::coverBonus` en donne le montant) : ils arrivent avec les classes.
Ni la lumière, les sens et les ténèbres — « voir » est ici la ligne de vue —, ni le déplacement de
l'origine d'une zone visée derrière un obstacle.

### Ce qui reste hors du lot, nommément

- **Les sorts** et leurs jets de sauvegarde, auxquels l'abri s'ajoute : `LOT-25`, `LOT-35`, avec
  les classes. Ils trouveront `core::AreaOfEffect`, `core::affectedCells` et `core::coverFromPoint`.
- **La lumière, les sens, se cacher**, voir sans être vu : aucun modèle de vision encore.
- **L'origine d'une zone visée derrière un obstacle** (« le point d'origine se retrouve du côté de
  l'obstacle le plus proche ») : avec les sorts qui la visent.
- **L'arme improvisée lancée** (une épée longue jetée) et les **munitions** consommées : avec
  l'inventaire en combat.
- **Montrer** portées, lignes de vue et gabarits au joueur : l'IHM de combat, `LOT-24`.

## Conception

### Les règles du Manuel, et où chacune vit

| Règle | Où | Vérifiée par |
|---|---|---|
| Au-delà de la portée normale : désavantage ; au-delà de la longue portée : impossible | `core::inReach`, `core::attackCircumstances` | `AttackTest.LaGrilleDitLaPorteeEtLesCirconstances`, `ViserDemandeLaPorteeEtLaVue` |
| Tir désavantagé à 1,50 m d'une créature hostile « qui vous voit » | `core::attackCircumstances` | `AttackTest.ViserDemandeLaPorteeEtLaVue` |
| Abri partiel +2, important +5 à la CA ; « les types d'abri ne s'additionnent pas » | `core::AttackRoll::applyCover`, `core::coverFrom` | `AttackTest.LAbriChangeLaCAUneFois`, `LineOfSightTest.LesAbrisEtCeQuiLesDonne` |
| Une créature, amie ou ennemie, abrite partiellement ; une herse, une meurtrière de façon importante | `core::coverFrom` (familles), `core::GridObject::cover` | `LineOfSightTest.LesAbrisEtCeQuiLesDonne` |
| Une cible sous abri total ne peut pas être ciblée directement | `core::checkTarget` | `AttackTest.ViserDemandeLaPorteeEtLaVue`, `ArenaTest.LePilierCacheEtAbrite` |
| Une zone s'étend en lignes droites ; seul un abri total les bloque | `core::affectedCells` | `AreaOfEffectTest.UnMurArreteLEffet` |
| Cône : largeur = distance, origine exclue ; sphère et cylindre : origine incluse ; ligne : longueur et largeur ; cube : origine sur une face | `core::areaTemplate` | `AreaOfEffectTest.LesGabaritsCouvrentLesCasesDeReference`, `LOrigineEtLesTailles` |
| Allonge : l'arme ajoute 1,50 m ; Lancer : même caractéristique qu'au corps à corps | `core::weaponAttackFor`, `core::thrownAttackFor` | `AttackTest.LesPorteesSeLisentDansLaDonnee` |
| Attaque d'opportunité contre une créature « située dans votre champ de vision » | `core::ArenaSession::move` | `ArenaTest.LOpportuniteLeDesengagementEtLEsquive` (non-régression) |
| Esquiver : désavantage « si vous pouvez voir l'attaquant » | `core::ArenaSession` (`contextAgainst`) | idem |

### La symétrie, par construction

La feuille de route demandait que la ligne de vue soit **symétrique** et que ce soit vérifié
exhaustivement. Le défaut classique vient d'un tracé qui **avance** — case par case, depuis A,
jusqu'au premier obstacle — et qui, parti de B, ne passe pas par les mêmes cases.

Ici, rien n'avance. La question posée est « ce segment touche-t-il cette case ? », en arithmétique
**entière exacte** : les points sont en demi-cases (coin de case pair, centre impair), chaque axe
donne l'intervalle des paramètres où le segment est dans la case, et l'intersection se compare en
fractions. Un segment n'a pas de sens de parcours, et l'ensemble des segments examinés — tous les
points de grille d'une emprise vers tous ceux de l'autre — est le même dans les deux sens.

- **Un mur coupe un segment qui le touche**, bord et coin compris, extrémités exceptées : raser la
  face d'un mur ne permet pas de voir au travers, et un tireur debout contre un mur n'est pas aveugle
  pour autant, puisque le coin de sa case qui touche ce mur n'est qu'une extrémité.
- **Le coin commun de deux murs en diagonale arrête le regard** comme il arrête le pas (« Coins »,
  chapitre 9). Le test des boîtes ignorant les extrémités, un segment qui *part* de ce coin était
  passé : les tests de référence l'ont trouvé, et une règle d'extrémité le rattrape — elle ne
  dépend que de l'extrémité et de la direction qui s'en éloigne, et reste donc symétrique.
- **Tous les points de grille d'une emprise**, pas seulement ses quatre coins : c'est ce qui garde la
  relation symétrique pour une créature de grande taille.

`LineOfSightTest.LaVueEstSymetriqueSurDesGrillesGenerees` génère vingt grilles 5 × 5 de murs, d'eau
profonde, de portes et de herses à des densités de 10 à 55 %, et compare les deux sens pour chaque
paire de points de grille (plus de 25 000 segments), puis pour chaque paire d'emprises 1 × 1 et
2 × 2 contre 1 × 1, en vérifiant au passage que l'abri total équivaut exactement à l'absence de vue.
**Réintroduit à la main**, un segment qui compte son point de départ et pas son point d'arrivée —
exactement le tracé qui avance — fait échouer ce test à la première paire asymétrique.

### L'abri : ce que la grille compte

Le Manuel définit l'abri par la **fraction du corps** qu'un obstacle protège, et ne dit pas comment
la mesurer sur un quadrillage. Le lot prend la méthode du *Guide du Maître* — hors du corpus, mais la
seule que les tables connaissent — : l'attaquant choisit un point de son emprise, trace des lignes
vers les quatre coins d'une case de la cible, et compte celles qu'un obstacle coupe. Il choisit le
point et la case qui l'arrangent.

Trois **familles** d'obstacles se comptent séparément, parce que les abris ne s'additionnent pas :

- **ce qui arrête la vue** (murs, portes fermées, objets à abri total) abrite selon les lignes
  coupées : une ou deux, partiel ; trois, important ; quatre, total ;
- **un objet à abri important** (la herse, la meurtrière) abrite de façon importante **dès qu'il
  coupe une ligne** ;
- **une créature interposée** ou **un objet à abri partiel** (le muret) abrite partiellement dès qu'il
  coupe une ligne.

Pour chaque choix de l'attaquant, le meilleur abri des trois familles compte ; puis l'attaquant
garde le choix le moins abrité. Un mur qui coupe deux lignes et une créature qui coupe les deux
autres font donc un abri **partiel**, pas un abri total — le test le rejoue depuis un point fixe.

L'abri se pose sur le jet **avant le premier greffon** `BeforeRoll`, par `AttackRoll::applyCover`,
qui ne remplace un abri que par un meilleur : poser deux fois l'abri partiel laisse la CA à +2,
l'abri important par-dessus la porte à +5, pas à +7, et le journal l'écrit — « [abri partiel : CA
15 -> 17] ». **Réintroduit à la main**, un bonus qui s'ajoute à chaque appel fait lire 19 et 22 au
test là où il attend 17 et 20.

### Les zones d'effet : la moitié d'une case

Une case est **dans la zone si la forme en couvre au moins la moitié** — la règle du *Guide du
Maître* pour les zones circulaires, étendue aux quatre autres formes plutôt que d'en inventer une
par forme. La surface se calcule exactement : le cône (un triangle, sa largeur égale à sa
distance), la ligne et le cube (des rectangles) sont découpés par la case ; le disque d'une sphère
ou d'un cylindre s'intègre analytiquement. Une case couverte à 50 % pile est dedans.

Le point d'origine est un `core::GridPoint` : une intersection pour une boule de feu, le milieu du
bord d'une créature pour un souffle, son centre pour une aura. « L'origine n'est pas incluse » dans
un cône tombe alors de la géométrie : parti du centre d'une case, il n'en couvre qu'un huitième.
Les gabarits sont figés par des **dessins** de référence (`X` pour une case de la zone), relus
contre la règle avant d'être écrits : la sphère de rayon 2 sur une intersection est un carré 4 × 4
sans ses coins, le cône de 3 depuis le bord d'une case couvre 1, 1 puis 3 cases, la ligne
diagonale de 4 cases en couvre 3.

Une case de la zone qu'aucun segment dégagé ne relie à l'origine — par l'un de ses quatre coins —
en sort, et une case qui arrête elle-même la vue n'est pas un emplacement : l'effet s'y heurte.

### La donnée : des propriétés, pas de la prose

Le `LOT-21` avait laissé les portées « en prose » et une règle n'avait pas le droit de les y lire.
Le schéma des armes prévoyait déjà `properties`, `rangeNormal` et `rangeLong` : ils sont remplis, à
l'**extraction**, par les fonctions `proprietes_d_arme` et `portees` des scripts du corpus, qui
lisent la colonne des propriétés des *Basic Rules* et les blocs de créatures, et **refusent** une
propriété inconnue plutôt que de la deviner. Les fichiers livrés ont été réécrits par ces mêmes
fonctions ; `check_rpg_data.py` les valide.

Deux conséquences qu'il faut savoir : les armes de **finesse** du catalogue (dague, rapière,
cimeterre, épée courte, fouet, fléchette) le sont désormais pour de vrai, et
`core::weaponAttackAbility` leur donne la meilleure de la Force et de la Dextérité ; la **hallebarde**,
la coutille, la pique, le fouet et la lance d'arçon frappent à deux cases. Un test charge le
catalogue et vérifie qu'une arme a des portées **si et seulement si** elle a les munitions ou le
lancer.

## Décisions de réalisation

- **Les portées sont arrondies vers le bas** en cases : une portée ne dépasse jamais ce que le texte
  promet. Toutes celles du corpus sont des multiples de 1,50 m ; l'arrondi ne mord pas encore.
- **Une action du bestiaire qui porte une allonge et une portée** donne deux attaques, au contact
  puis à distance. Aucune n'en porte encore les deux.
- **Voir, c'est la ligne de vue.** La lumière, la vision dans le noir et les ténèbres ne sont pas
  modélisées : là où le Manuel dit « qui vous voit », le moteur lit `hasLineOfSight`.
- **L'eau profonde et la falaise n'arrêtent pas la vue** : elles arrêtent la marche. L'altitude reste
  un attribut (`core::Locomotion`) : un volant se voit et se vise sans hauteur à ajouter.
- **Frapper au contact demande aussi la vue** : deux combattants en diagonale de part et d'autre du
  coin de deux murs ne se touchent pas plus qu'ils ne s'y faufilent.
- **Un combattant à terre abrite encore** : il reste sur la grille, et un corps est un obstacle.
- **Le cylindre est son disque.** Sans hauteur, cylindre et sphère ont le même gabarit ; les deux
  formes restent, parce que le Manuel les distingue et qu'un sort les nomme.
- **Un cube suit la direction donnée**, qui peut être diagonale : le Manuel ne l'oriente pas, et
  l'interdire serait une règle de plus sans texte pour la porter.
- **L'abri d'un objet est déclaré, jamais déduit** de son `kind`, que la grille n'interprète pas.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔ Vingt grilles, chaque paire de points de grille et d'emprises, dans les deux sens ; un tracé asymétrique réintroduit fait échouer le test.
2. ✔ Sphère, cylindre, cône, ligne cardinale et diagonale, cube, comparés à leur dessin case pour case ; l'origine incluse ou non ; une zone coupée par un mur.
3. ✔ +2, +5, un abri reposé sans effet, un meilleur qui remplace ; un bonus cumulé réintroduit fait échouer le test.
4. ✔ Tout est dans `Core`, et se teste sans fenêtre.
5. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1130/1130 en Debug et en Release, `clang-format`, les lints, le contrôle des données RPG et le cahier de test verts ; deux défauts réintroduits à la main font chacun échouer leur test.

Alimente [LOT-23](LOT-23-ia-tactique.md), [LOT-24](LOT-24-ihm-combat.md), [LOT-25](../../../../vision/archives/feuille-de-route-jeu.md#lot-25), `LOT-35` et `LOT-72`.

Exigences couvertes : `EX-CBT-021` (la ligne de vue et la couverture), `EX-CBT-022` (l'arme déclare son allonge ou sa portée). Aucune exigence ajoutée.
