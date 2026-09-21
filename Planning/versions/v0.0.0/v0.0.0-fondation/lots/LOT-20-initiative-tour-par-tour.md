+++
id = "LOT-20"
titre = "Initiative et tour par tour"
version = "0.0.0"
filiere = "regles"
statut = "livre"
taille = "L"
resume = "Un combat s'ordonne par initiative, se joue tour par tour avec son économie d'action, et se termine par l'une de ses trois fins."
prerequis = ["LOT-12", "LOT-19"]
livrables = [
  "`core::TurnOrder` (`TurnOrder.{h,cpp}`) : l'ordre d'initiative, `core::actsBefore`, les repères d'initiative fixe.",
  "`core::ActionEconomy` (`ActionEconomy.{h,cpp}`) : les ressources d'un tour, comme une liste.",
  "`core::ScopedCounters` et `core::ImmunityLedger` (`CombatCounters.{h,cpp}`).",
  "`core::CombatState` (`CombatState.{h,cpp}`) : montage, rounds, tours, crochets, points de vie, les trois fins.",
  "`core::mountEncounter`, qui pose sur la grille la rencontre engagée par `core::beginEncounter`.",
  "`core::CombatHook` : neuf points d'insertion nommés ; `core::InitiativeMarker` et l'acteur flottant.",
  "`StepRules::cost` : la case d'une autre créature coûte double.",
]
criteres = [
  "Un combat à cinq combattants se déroule en headless du premier round à une condition de fin.",
  "Les trois conditions de fin sont couvertes par un test chacune.",
  "Un test monte quatre alliés.",
  "Rejeu à graine fixe strictement reproductible, ordre d'initiative compris.",
  "`ctest` : 1084/1084 (1065 avant, plus les dix-neuf cas de ce lot).",
]
sources = [
  "Manuel des Joueurs, « Initiative » (PDF p. 191)",
  "Manuel des Joueurs, « Se déplacer au milieu d'autres créatures » (PDF p. 192-193)",
]
+++

## Pourquoi

Ordonnancer les combattants par initiative et structurer le tour : déplacement, action, action
bonus, réaction — et terminer un combat par l'une de ses trois fins.

## Périmètre

Quatre briques de `Core`, pures et vérifiables sans fenêtre (`Source/Core/Combat/`) :

- `core::TurnOrder` (`TurnOrder.{h,cpp}`) — l'ordre d'initiative, sa règle de départage, et les
  **repères** d'initiative fixe ;
- `core::ActionEconomy` (`ActionEconomy.{h,cpp}`) — les ressources d'un tour, comme une **liste** ;
- `core::ScopedCounters` et `core::ImmunityLedger` (`CombatCounters.{h,cpp}`) — « une fois par
  tour, par round, par rencontre, par jour », et « immunisé 24 heures contre cette source » ;
- `core::CombatState` (`CombatState.{h,cpp}`) — la machine à états : montage, rounds, tours,
  crochets, entrées et sorties, points de vie, les trois fins ; et `core::mountEncounter`, qui pose
  sur la grille la rencontre que `core::beginEncounter` ([LOT-18](@ref lot-18)) a engagée.

Il ne livre ni attaque, ni IA, ni affichage. Le jet d'attaque et le pipeline de dégâts sont au
[LOT-21](@ref lot-21), qui aboutira dans `CombatState::applyDamage` ; la tactique ennemie au
[LOT-23](@ref lot-23) ; le bandeau d'initiative et les cases surlignées au [LOT-24](@ref lot-24).
`hmi::CombatMode` n'est pas branché sur la machine : il reste le mode qui gèle le monde, et le
combat en cours vit hors de lui, pour la raison qui y gardait déjà `core::EncounterRun` — se
vérifier sans fenêtre. Le brancher sur une session de jeu est l'affaire du `LOT-24` et de l'arène
(`LOT-50`), qui sera le premier lieu où un combat s'ouvre pour de vrai.

### Ce qui reste hors du lot, nommément

- **La surprise** (Manuel, « Surprise ») : un combattant surpris ne peut ni se déplacer ni agir à son
  premier tour, ni réagir avant la fin de ce tour. Elle suppose la discrétion et la perception
  passive ; le crochet `TurnStart` et `ActionEconomy::spend` suffisent à l'écrire, avec les
  conditions du [LOT-72](@ref lot-72).
- **Les jets contre la mort** : un allié à terre voit ses tours passés ici ; le `LOT-72` lui en
  rendra un.
- **Les attaques d'opportunité** : la réaction est dépensable hors du tour, mais le déclencheur
  « sortir d'une allonge » est au [LOT-21](@ref lot-21).
- **Les actions nommées** (se précipiter, se désengager, esquiver, se tenir prêt) : ce sont des
  usages de l'action, au `LOT-21`.
- **La taille d'un personnage** : la fiche n'en porte pas, et `profileFor(CharacterSheet)` le fait
  de taille M. L'espèce la déclarera.
- **Le choix du vol** pour une créature qui vole et marche : `profileFor(Creature)` vole si la
  vitesse de vol dépasse la vitesse de marche ; choisir autrement, tour par tour, est l'IA du
  `LOT-23`.

## Conception

### Les trois points que la grille laissait ouverts

Le [LOT-19](@ref lot-19) attendait ce lot sur trois points, faute de tour :

- **les identifiants et les placements** — `CombatState::enlist` attribue les `core::CombatantId`
  dans l'ordre des enrôlements, et `core::mountEncounter` enrôle le groupe puis les créatures de la
  rencontre, en **refusant avec leur raison** celles dont la case tombe dans un mur ou sur une case
  prise, et en nommant celles que le bestiaire ne connaît pas. Un combattant refusé n'est pas
  enrôlé : sans case, il ne peut pas combattre sur la grille ;
- **qui l'on traverse** — `CombatState::moverFor` remplit `core::Mover::canPassThrough` selon le
  Manuel des Joueurs, « Se déplacer au milieu d'autres créatures » (PDF p. 193) : un allié se
  traverse, un ennemi seulement s'il a **deux catégories de taille** d'écart ;
- **le budget d'un tour** — `CombatState::reachableArea` construit la `core::ReachableArea` sur ce
  qui **reste** du déplacement, et `CombatState::move` paie le chemin. Le déplacement se fractionne
  donc : trois cases, une attaque, trois cases.

### L'initiative : jetée une fois, départagée par une règle écrite

L'initiative est un test de Dextérité (Manuel des Joueurs, « Initiative », PDF p. 191) : elle passe
par `core::rollCheck` comme tout autre jet, et le résultat reste **restituable**
(`Combatant::initiativeRoll`, `EX-REG-003`). Elle est jetée **une fois**, à `start`, par identifiant
croissant — à graine égale, les mêmes dés tombent sur les mêmes combattants —, et l'ordre ne bouge
plus (`EX-CBT-010`).

Le livre laisse les égalités au MD : entre ses créatures, il choisit ; entre personnages, les
joueurs ; entre un monstre et un personnage, le MD encore — ou il fait relancer un d20. Le jeu n'a
pas de MD, et `EX-CBT-010` interdit de s'en remettre à l'ordre en mémoire. La règle
(`core::actsBefore`) :

1. le total le plus haut ;
2. le modificateur d'initiative le plus haut ;
3. la Dextérité la plus haute ;
4. les **alliés avant les ennemis** — la part du MD, tranchée en faveur du joueur ;
5. l'identifiant le plus petit, qui est l'ordre de la **donnée**, pas de la mémoire.

**La relance d'un d20 est écartée**, et ce n'est pas une simplification : un tirage de départage
consommerait la suite aléatoire, et l'ordre du combat dépendrait alors du *nombre* d'égalités
survenues avant — deux combats identiques au premier jet divergeraient au second. Un test range cinq
places, qui ne se distinguent chacune de la suivante que par un critère, dans les cent vingt ordres
possibles, et obtient cent vingt fois la même suite.

### Le tour : une place, pas un indice

Le curseur du round est une **place** (`core::TurnSlot`), porteuse de tout ce qui la range, et non
un indice dans un tableau. Un renfort qui entre allonge l'ordre, un fuyard le raccourcit : un indice
sauterait un tour ou en rejouerait un. La place suivante se **calcule** par la règle d'ordre — même
après le départ du combattant qui l'occupait —, et un renfort rangé après la place en cours joue ce
round-ci, rangé avant, au round suivant. C'est ce que dit le livre, et il n'a fallu aucun cas
particulier pour l'obtenir.

La fin d'un tour est **explicite** (`EX-CBT-012`). Épuiser action, action bonus, réaction et
déplacement ne termine rien, et un test le vérifie ; seul `endTurn` passe la main. Deux exceptions,
qui ne sont pas des décisions du joueur mais des faits : un combattant qui **tombe** à 0 point de vie
ou qui **sort** pendant son propre tour ne peut plus le jouer, et son tour se termine — le crochet
de fin de tour est annoncé quand même, parce que les actions légendaires ne distinguent pas un tour
fini d'un tour interrompu.

Un combattant à terre garde sa place et ses tours sont **passés** ; relevé, il rejoue à sa place.
Les jets contre la mort, qui lui rendront un tour, sont au [LOT-72](@ref lot-72).

### L'économie d'action est une liste

`EX-CBT-011` : une action, une action bonus, une réaction, un déplacement. Le §4bis ajoutait qu'une
**troisième économie d'action** existe — la *Heroic Action* des Marques Héroïques — et qu'une
capacité octroie une réaction à un allié. `core::ActionEconomy` est donc une liste de ressources
nommées, chacune avec ce que le tour en restaure : la *Heroic Action* du `LOT-50` sera un appel à
`declare`, et un octroi (`grant`) dure jusqu'au prochain début de tour de son porteur.

Chaque ressource revient **au début du tour de son porteur**, jamais à la fin du tour courant. Un
test le vérifie avec trois combattants : la réaction dépensée par le troisième pendant le tour du
premier est encore à 0 pendant le tour du deuxième, et revient au début du sien. Le défaut inverse —
restaurer en fin de tour — a été réintroduit à la main : ce test échoue, et lui seul.

`EX-CBT-011` dit chaque ressource « consommable une seule fois ». Pour le déplacement, c'est le
**budget** qui ne se consomme qu'une fois — il ne revient pas dans le tour —, pas le geste de se
déplacer : le Manuel laisse « décider si vous voulez d'abord vous déplacer ou agir », et fractionner
sa vitesse autour de l'action est la moitié de la tactique sur une grille.

### Les trois fins

Le critère le plus coûteux à oublier : une machine qui n'en couvre que deux laisse un combat qui ne
finit jamais. Les trois sont évaluées après **chaque** changement, pas en fin de tour — un combat
gagné par une réaction pendant le tour d'un ennemi est gagné tout de suite :

| Fin | Condition |
|---|---|
| **Victoire** | plus aucun ennemi n'est debout : tous à terre, ou partis |
| **Défaite** | plus aucun allié n'est debout, et aucun n'est parti |
| **Fuite** | plus aucun allié n'est debout, et au moins un est **parti** |

Trois arbitrages que la feuille de route ne tranchait pas :

- **Un ennemi qui part compte pour la victoire.** `EX-CBT-001` fait sortir du combat « quand plus
  aucun camp hostile n'est engagé » ; un ennemi en fuite ne l'est plus. La feuille de route écrivait
  « tous les ennemis à 0 PV » : c'était le cas courant, pas la règle.
- **La fuite n'exige pas que tout le groupe parte.** Un allié à terre laissé derrière ne change pas
  l'issue en défaite pour ceux qui sont partis ; ce qui arrive à celui qui reste est l'affaire de
  l'agonie (`LOT-72`).
- **Si un même changement abat les deux camps, c'est une défaite.** `applyDamage` accepte une salve
  entière (`std::span<const HitPointChange>`) et n'évalue l'issue qu'une fois : une boule de feu
  qui abat le dernier allié et le dernier ennemi n'est pas une victoire ou une défaite selon l'ordre
  des cibles. Le défaut inverse a été réintroduit : le test de défaite échoue.

Une rencontre dont on ne fuit pas (`core::Encounter::escapable`, [LOT-18](@ref lot-18)) refuse la
sortie d'un allié (`WithdrawResult::NotEscapable`) ; `mountEncounter` recopie le drapeau. L'issue
produite est le `core::CombatOutcome` que `core::endEncounter` consommait déjà : un test monte une
rencontre, la gagne, et vérifie que le drapeau de monde est posé.

### Les crochets, et les conséquences en cascade

Le §4bis demandait des **points d'insertion nommés** ; aucun n'a de consommateur dans ce lot, et
tous en auront un. `core::CombatHook` en nomme neuf :

| Crochet | Ce qui s'y greffera |
|---|---|
| `BeforeFirstTurn` | la réaction du *Natural Strategist*, avant le premier tour |
| `RoundStart` | les effets « au début de chaque round » |
| `InitiativeCount` | un repère fixe : actions de repaire à 20, renforts à 0 |
| `TurnStart`, `TurnEnd` | les effets de début de tour ; les actions légendaires, en fin de tour d'autrui |
| `AttackDeclared` | les postures du moine, qui répondent à l'**intention** avant le jet |
| `CombatantJoined`, `CombatantLeft` | un renfort, une fuite |
| `CombatEnded` | l'expérience (`LOT-74`), le retour à l'exploration |

Un **repère d'initiative fixe** (`core::InitiativeMarker`) est une place de l'ordre qui n'est pas
un combattant, et qui **perd les égalités** : c'est la formule du livre pour les actions de repaire.
Un **acteur flottant** (*Law of Time*, `CombatantProfile::floating`) n'a pas de place : il demande
à jouer avant n'importe quel tour (`interject`), une fois par round, et joue en fin de round s'il n'a
rien demandé — un acteur indécis ne perd pas son tour.

Un abonné reçoit l'état **mutable** : il peut infliger des dégâts, faire entrer un renfort, octroyer
une réaction. Ce qui en découle ne se règle **jamais** au milieu d'une annonce : la machine tient la
profondeur d'appel, et règle les conséquences quand l'appel extérieur se termine, une par une et
dans un ordre fixe — la fin du combat d'abord, puis le tour d'un combattant qui ne peut plus le
jouer, puis la place suivante. Sans cette discipline, un renfort appelé au repère « renforts »
aurait ouvert son tour **pendant** l'annonce du repère, et deux abonnés du même crochet auraient vu
deux états différents. Un abonné ne peut pas terminer un tour : la fin d'un tour est la décision de
celui qui joue.

### Les mémoires à portée

« Une fois par tour » se compte pendant **n'importe quel** tour, pas seulement celui du porteur : une
attaque sournoise portée par une réaction pendant le tour d'un ennemi en consomme l'usage. La portée
`Turn` se vide donc à chaque fin de tour, `Round` à chaque début de round, `Encounter` à la fin du
combat ; `Day` n'est jamais touchée par le combat, et attend l'horloge du `LOT-70`.

L'immunité « 24 heures contre la Présence terrifiante » tient à un **couple** (créature, source) —
la même créature reste sensible à un autre dragon — et à une **échéance** en secondes de jeu, que
le registre reçoit sans lire d'horloge. Les deux mémoires sont indexées par chaîne, pas par
`CombatantId` : elles survivent au combat, l'identifiant non.

### Sans héros unique

Rien dans l'ordre ni dans la machine ne distingue « le » joueur : les alliés sont un camp, et
`core::CombatantProfile` se construit indifféremment d'une fiche (`profileFor(CharacterSheet)`) ou
d'un bloc du bestiaire (`profileFor(Creature)`). Un test monte **quatre alliés** et deux ennemis :
chacun des six joue exactement trois tours en trois rounds, trois alliés à terre ne terminent pas le
combat, et le quatrième le gagne. C'est la précaution que le [LOT-29](@ref lot-29) demande pour rester
un lot d'ajout.

## Relevé en chemin : la case d'une créature coûte double

En relisant la page du Manuel qui fonde `canPassThrough`, une phrase que le [LOT-19](@ref lot-19)
n'appliquait pas : « N'oubliez pas que l'emplacement occupé par une autre créature est considéré
comme un terrain difficile » (PDF p. 192 et 193). Traverser un allié coûtait 1. Le défaut était
**latent** — `canPassThrough` était vide, et personne ne traversait personne — et ce lot le rendait
réel. `StepRules::cost` compte désormais double toute case tenue par une autre créature, en vol
comme au sol : c'est la créature qui gêne, pas le sol. Le test du `LOT-19` qui traversait un allié
attendait un coût de 1 ; il attend 2.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔ Deux alliés contre trois gobelins, sur une carte à pilier : chacun marche vers l'ennemi le plus proche par `reachableArea`/`move`, frappe au contact, termine son tour ; le combat se termine en quatre rounds.
2. ✔ Victoire, défaite (et la salve qui abat les deux camps), fuite (et le refus d'une rencontre dont on ne fuit pas).
3. ✔
4. ✔ L'escarmouche rejouée à la même graine produit le même journal — crochets, pas, jets d'attaque détaillés — et une autre graine un autre journal ; trois rounds d'initiative rejoués donnent les mêmes dés et le même ordre.
5. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1084/1084 en Debug et en Release, `clang-format`, les lints et le cahier de test verts.

Exigences couvertes : `EX-CBT-010`, `EX-CBT-011`, `EX-CBT-012`, et la part « tour par tour » d'`EX-VIS-004`. Aucune exigence ajoutée.
