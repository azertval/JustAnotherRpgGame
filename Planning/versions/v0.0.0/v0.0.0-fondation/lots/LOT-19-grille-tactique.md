+++
id = "LOT-19"
titre = "Grille tactique et déplacement"
version = "0.0.0"
filiere = "regles"
statut = "livre"
taille = "L"
resume = "Le combat a sa grille : obstacles, terrain difficile, occupation des cases, et un déplacement par budget au chemin déterministe."
prerequis = ["LOT-18"]
livrables = [
  "`core::BattleGrid` (`Source/Core/Combat/BattleGrid.{h,cpp}`) : obstacles, terrain difficile, zones, objets posés, occupation par emprise.",
  "`core::ReachableArea` et `core::findPath` (`Source/Core/Combat/Pathfinding.{h,cpp}`) : budget, destinations et chemin.",
  "`core::Locomotion` : le vol comme manière de traverser la grille.",
  "`footprintSide`, `PlacementResult`, `Mover::canPassThrough`, `movementBudget`.",
  "`GridObject` et `damageObject`, `setDifficult`, `zonesAt`.",
  "Dix-neuf cas de test, dont l'accord de `findPath` et `pathTo` sur deux cents cartes à graine fixe.",
]
criteres = [
  "Même entrée → même chemin, systématiquement.",
  "L'ensemble des cases atteignables correspond exactement au budget, ni une de plus ni une de moins.",
  "Deux créatures ne partagent jamais une case.",
  "Le terrain difficile double le coût, et la portée s'en trouve réduite en conséquence.",
  "`ctest` : 1065/1065 (1046 avant, plus les dix-neuf cas de ce lot).",
]
sources = [
  "Manuel des Joueurs, « Variante : jouer sur un quadrillage » (PDF p. 194)",
  "Manuel des Joueurs, table « Catégories de tailles » (PDF p. 193)",
]
+++

## Pourquoi

Poser la grille de combat, l'occupation des cases, et le calcul du déplacement par budget.

## Périmètre

Il livre deux briques de `Core`, pures et vérifiables sans fenêtre :

- `core::BattleGrid` (`Source/Core/Combat/BattleGrid.{h,cpp}`) — ce qui fait obstacle, ce qui
  ralentit, les zones que la carte déclare, les objets posés sur les cases, et **qui se tient où** ;
- `core::ReachableArea` et `core::findPath` (`Source/Core/Combat/Pathfinding.{h,cpp}`) — le budget
  de déplacement, les cases où finir son mouvement, et le chemin pour s'y rendre.

Il ne livre ni tour, ni camp, ni affichage. C'est le [LOT-20](@ref lot-20) qui attribuera les
identifiants de combattants, posera sur la grille les placements que `core::beginEncounter` calcule
déjà, et demandera au joueur où aller ; c'est le [LOT-24](@ref lot-24) qui surlignera
`ReachableArea::destinations()`. La moitié « montrées avant que le joueur ne s'engage »
d'`EX-CBT-020` a donc ici sa **donnée**, et son dessin au `LOT-24`. `hmi::CombatMode` n'est pas
touché, sinon dans deux commentaires qui renvoyaient au `LOT-19` ce que le `LOT-20` fera.

### Ce qui reste hors du lot, nommément

- **Se faufiler** dans un espace d'une taille plus petite (le Manuel le permet à coût double) : aucun
  contenu du *vertical slice* ne le demande. Une créature trop grande pour un passage ne le franchit
  pas.
- Les **attaques d'opportunité** en sortant d'une allonge : [LOT-21](@ref lot-21).
- La **ligne de vue**, les portées et la couverture : [LOT-22](@ref lot-22), sur cette grille.
- La traversée d'un **ennemi** plus grand ou plus petit de deux tailles : elle suppose les camps du
  [LOT-20](@ref lot-20), qui la déclarera par `canPassThrough`.

## Conception

### Ce que la feuille de route supposait, et qui n'a pas tenu

La section du lot disait de ne pas réécrire `core::GridDistanceField`, sauvé de la purge par le
[LOT-01](@ref lot-01) « pour ce lot précis » : ce serait « exactement le calcul » des cases
atteignables. **Il ne l'est pas**, et le corpus le dit en une demi-page.

Le Manuel des Joueurs, « Variante : jouer sur un quadrillage » (PDF p. 194) :

- entrer dans une case coûte **une case de déplacement, même en diagonale** ;
- entrer dans une case de terrain difficile en coûte **deux**, et il faut qu'il en reste deux ;
- on ne passe pas en diagonale par le **coin** d'une case que remplit un élément du terrain.

`GridDistanceField` est un parcours en largeur à **quatre voisins** et à coût **uniforme**. Il ne
sait ni les diagonales, ni le double coût, ni les coins. Le détourner aurait donné un combat où
l'on ne se déplace qu'en croix et où la boue ne ralentit personne — ou bien un champ réécrit qui
aurait cessé de servir la récompense de progression (`EX-IA-023`) pour laquelle il existe. Il reste
donc intact, à sa place, et le déplacement est un Dijkstra sur huit voisins aux coûts 1 et 2.

### Le départage est une règle, et elle ne dépend pas de l'algorithme

Deux chemins de même coût existent presque toujours. Le critère du lot demandait que le départage
soit une règle explicite ; il en fallait une qui tienne **deux** algorithmes d'accord :
`ReachableArea` (Dijkstra borné par le budget, pour le tour du joueur) et `findPath` (A*, sans
budget, pour l'IA du [LOT-23](@ref lot-23) qui marche vers une cible lointaine).

La règle : **parmi les prédécesseurs qui atteignent une case à son meilleur coût, on retient le
plus proche de la droite qui joint le départ à l'arrivée**, et à égalité celui d'indice de case le
plus petit (ligne, puis colonne). Elle se définit sur le graphe, pas sur l'ordre d'exploration :
l'ordre des voisins, celui de la file, l'algorithme lui-même n'y changent rien. L'exploration
retient donc **tous** les prédécesseurs au meilleur coût (un masque de huit bits par case), et
c'est la remontée, qui connaît l'arrivée, qui choisit.

*Corrigé après livraison* : la première règle ne retenait que le plus petit indice, indépendamment
de l'arrivée. Vers une case en haut à droite, le chemin montait d'abord (la rangée la plus haute
gagne toujours) puis redescendait — la prévisualisation du déplacement (`LOT-24`) dessinait un
triangle là où le joueur attendait une ligne. La droite départ→arrivée est le seul critère qui ne
dépende ni de l'orientation ni de la position sur la carte.

Encore faut-il que l'A* voie tous ces prédécesseurs. Arrêté à la première sortie de la destination,
il rend un chemin **juste**, mais pas toujours **le** chemin : un prédécesseur de même coût et
d'indice plus petit peut rester en file, à estimation égale. `findPath` vide donc la file tant
qu'elle porte une estimation qui ne dépasse pas le coût trouvé. Ce n'est pas une précaution
théorique : le test qui compare les deux algorithmes sur deux cents cartes tirées d'une graine fixe
**échoue** quand on rétablit l'arrêt à la première sortie — vérifié en réintroduisant le défaut.

### L'altitude est un attribut, jamais une géométrie

C'était la décision que le §4bis demandait d'écrire ici « avant la première créature volante ». La
grille n'a pas de hauteur ; le vol est une **manière de la traverser** (`core::Locomotion`) :

| Au sol | En vol |
|---|---|
| l'eau profonde et la falaise arrêtent | survolées — un volant s'y tient en vol stationnaire |
| le terrain difficile coûte double | ignoré : c'est une gêne de sol |
| le mur, la matière pleine, la porte fermée arrêtent | arrêtent aussi |
| une case, un combattant | une case, un combattant |

Le §4bis écrivait qu'un volant « ignore la couche collision au sol ». La formule a été **resserrée** :
la grille de collision ne distingue pas un muret d'un rempart, et un volant qui traverserait tous
les pleins traverserait les murs d'un donjon jusqu'à la voûte. Ce qui se survole se nomme donc dans
`BattleGrid`, type de tuile par type de tuile (`DeepWater`, `Cliff`), et nulle part ailleurs. Qu'un
volant reste ciblable à portée découle de la même décision, et c'est le [LOT-22](@ref lot-22) qui
la lira.

Le coin d'un mur se juge, lui, **comme en vol** : seule la matière qui « remplit l'espace »
l'interdit. Longer une mare en diagonale est permis ; couper l'angle d'un mur ne l'est pas.

### Deux créatures ne partagent jamais une case

L'occupation se tient par **emprise** : `footprintSide` traduit la table « Catégories de tailles »
du Manuel (PDF p. 193) — M et moins, une case ; G, 2 × 2 ; TG, 3 × 3 ; Gig, 4 × 4. Le bestiaire du
[LOT-33](@ref lot-33) compte des bêtes de grande taille, et une grille qui n'aurait su que des pions
d'une case aurait menti au premier ours.

Un écart assumé : une créature **très petite** occupe une case entière, là où le livre en tolère
quatre dans la même. C'est le critère du lot, et un contenu qui voudrait la règle du livre la
rouvrira par une exception nommée plutôt que par un partage silencieux.

`place` et `moveTo` **refusent** plutôt que de corriger, et disent pourquoi (`PlacementResult`) : une
case voulue qui tombe dans un mur est une information pour le montage de la rencontre, et la
déplacer d'office cacherait une formation mal écrite. `moveTo` ne vérifie ni chemin ni budget — la
grille tient l'occupation, `ReachableArea` a validé le trajet —, mais une créature de 2 × 2 qui
avance d'une case recouvre la moitié de son emprise, et ne s'y gêne pas elle-même.

#### Traverser n'est pas s'arrêter

Le Manuel permet de traverser l'espace d'une créature non hostile, jamais d'y finir. La grille ne
connaît pas les camps : la requête le lui dit (`Mover::canPassThrough`), et le tour par tour du
`LOT-20` le remplira. Vide, **personne** ne se traverse — le parti prudent, qui ne fait jamais
passer à travers un ennemi faute d'avoir renseigné un camp. Une case traversée a un coût
(`costTo`) et n'est pas une destination (`canEndAt`).

### Le budget s'arrondit à la case inférieure

`movementBudget` divise la vitesse par 1,5 (`core::METERS_PER_TILE`, figée au
[LOT-12](@ref lot-12)) et **tronque** : le livre dépense la vitesse « par segments de 1,50 mètre »,
et un segment entamé n'en est pas un. Arrondir au plus proche ferait gagner une case à qui porte
trop — 10 m après un malus d'encombrement valent six cases, pas sept. Une tolérance d'un millième
empêche l'inverse : qu'une soustraction de flottants fasse perdre une case à 5,9999.

### Les propriétés de zone : un crochet, et une seule lecture

Une couche de la carte qui porte des propriétés libres (`EX-LVL-018`) déclare une **zone** : ses
cases non vides. La grille les relève sans les interpréter (`zonesAt`), à une exception près —
`difficultTerrain: true`. Combat interdit, aucun soin, type de dégâts aléatoire : les règles de zone
du Sourcebook sont lues par le [LOT-50](@ref lot-50) et le [LOT-81](@ref lot-81), et ce lot n'en
invente aucune.

`difficultTerrain` n'est vrai que pour le **booléen** `true`. Un `1` ou un `"oui"` est une faute de
saisie, et le lire comme vrai ferait d'une coquille une règle — un test le vérifie.

Le terrain difficile peut aussi **naître en combat** (`setDifficult` : un séisme) et un objet posé
sur une case a des **points de vie** (`GridObject`, `damageObject` : les toiles du Sourcebook) ; il
bloque tant qu'il tient, ou ne bloque pas du tout, comme une toile qu'on traverse et dont l'état
qu'elle inflige est l'affaire du [LOT-21](@ref lot-21).

### Une seule source de vérité pour « peut-on se tenir ici »

Les obstacles viennent de la grille de collision de la carte, jamais d'un masque de combat
(`EX-CBT-001`) : `Level::tileMap()` pour la solidité statique, ou
`MechanismController::collisionMap()` pour que les portes fermées en soient. La grille la
**recopie** : le combat gèle les mécanismes ([LOT-18](@ref lot-18)), et une copie ne change pas
sous un tour en cours.

## Relevé en chemin

Huit cas de test de `test_encounter.cpp` ([LOT-18](@ref lot-18)), `test_game_mode.cpp`,
`test_rpg_screens.cpp`, `test_screen_flow.cpp` et `test_character_sheet_values.cpp` portaient leurs
balises `\tcat`, `\tcrit`, `\tetapes` et `\tattendu` écrites avec une **tabulation** à la place du
`\t` — un échappement interprété au passage par un script d'écriture. Le générateur les lisait
comme du texte : le cahier de test les affichait sans criticité (« (?) ») et sans étapes. Corrigé ;
ce sont les trente-deux lignes du même défaut.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔ Cent requêtes identiques rendent le même chemin ; la règle de départage est vérifiée sur deux cas écrits à la main ; `findPath` et `pathTo` s'accordent sur toute destination de deux cents cartes aléatoires à graine fixe.
2. ✔ Budgets 0 à 5 sur une grille ouverte, chaque case comparée à sa distance.
3. ✔ Y compris par l'emprise d'une grande créature.
4. ✔ La boue partout ramène 80 destinations à 24, et une case difficile à un pas du bout du budget reste hors de portée.
5. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1065/1065 en Debug et en Release, `clang-format`, `clang-tidy` `bugprone-*` sur les deux fichiers ajoutés, les lints, cahier de test et Doxygen verts.

En amont du prérequis direct : [LOT-04](@ref lot-04) (la grille de collision), [LOT-12](@ref lot-12) (l'échelle de 1,5 m), [LOT-13](@ref lot-13) (la vitesse de la fiche).

Exigences couvertes : `EX-CBT-020`, `EX-REG-051`. Aucune exigence ajoutée.
