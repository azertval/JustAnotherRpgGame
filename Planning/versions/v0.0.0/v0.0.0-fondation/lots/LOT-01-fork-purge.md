+++
id = "LOT-01"
titre = "Fork, purge et remise à nu"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Le dépôt d'origine est ramené à un moteur 2D générique, sans gameplay de plateforme ni solveur d'IA, sur lequel les lots suivants construisent le RPG en vue de dessus."
prerequis = []
livrables = [
  "Suppression de `Source/AiSolver/` (12 k lignes), de `Source/HMI/Ai/`, d'`AiModeScreen` et de l'entrée « Mode IA » du menu.",
  "Suppression de `Core/Ecs/Systems/CharacterPhysicsSystem`, de `Core/Gameplay/` hors `MechanismController`, de `SlopeGeometry`, `DangerGeometry` et `SlopeMask`.",
  "25 valeurs de `TileType` sur 36 retirées, avec les 26 niveaux de démonstration et leurs tests.",
  "`HMI/Graphics/TileSilhouette`, reste vidé de `SlopeMask` (`SILHOUETTE_TILE_TYPES` vide).",
  "`Core/World/GridDistanceField`, sauvé d'`aisolver` et rebasculé dans le namespace `core`, avec son test.",
  "Renommage du dépôt en `JustAnotherRpgGame` (cibles CMake, binaire, documentation, workflows), préfixe de macro `JADG_`, version remise à `0.1.0`.",
]
criteres = [
  "Tag d'archive posé **avant** toute suppression (supprimé depuis par le [LOT-88](LOT-88-retrait-heritage.md)).",
  'Un `grep -rn "gravity" Source/` ne renvoie plus rien.',
  "Configuration CMake réussie, `Core` et `JustAnotherRpgGame` compilés sans avertissement en `/W4 /WX`, exécutable produit.",
  "Le nombre de tests survivants est mesuré et publié : il devient l'oracle de référence des lots `LOT-02` à `LOT-05`, tous des refactorings à comportement constant.",
  "Plus aucune référence au nom d'origine hors de l'archive et de l'historique git.",
]
+++

## Pourquoi

Ramener le dépôt d'origine à un **moteur 2D générique**, sans gameplay de
plateforme ni solveur d'IA, pour que les lots suivants construisent le RPG en vue de dessus sur
une base propre plutôt qu'à côté d'un jeu qu'on ne finira pas.

### Pourquoi purger en premier

Le dépôt d'origine est un jeu de plateforme/puzzle en vue de côté livré en `0.1.3` après 74 lots.
Son architecture est remarquablement réutilisable — la frontière `HMI → Core` (`EX-ARCH-001`), un
ECS à composants de données pures (`EX-ARCH-010`/`011`/`012`), une simulation à pas fixe découplée
du rendu (`EX-ARCH-030`/`031`) — mais son **contenu** ne l'est pas :

- 131 fichiers référençaient `core::TileType`, dont la moitié des valeurs décrivaient des pentes,
  arrondis, blocs réduits, plateformes mobiles et dangers directionnels ;
- `hmi::GameSession::update` codait en dur un ordre de passes propre à la plateforme, réécrit à
  l'identique dans quatre orchestrations indépendantes ;
- 12 000 lignes de solveur RL (`AiSolver`) encodaient leur observation d'après `TILE_TYPE_COUNT`.

Une stratégie de coexistence aurait traîné tout cela pendant les vingt-cinq lots du programme, en
payant deux fois chaque évolution de format. Le fork permet l'inverse : purger d'abord, construire
ensuite sur une base trois fois plus petite.

Le prix est assumé et connu : purger tôt supprime une partie de l'oracle de test. La mitigation
tient en un point — **la purge est une suppression mécanique, pas une transformation**. Aucun
comportement conservé n'est réécrit ; on retire des fichiers entiers et les tests qui les
visaient. Ce qui reste garde ses propres tests, et c'est cet ensemble résiduel qui sert d'oracle
aux refactorings des lots `LOT-02` à `LOT-05`.

## Périmètre

### Conservé tel quel

`Core/Ecs` (World, EntityManager, ComponentPool, View, ISystem ; composants `Transform`,
`Velocity`, `Collider`, `Sprite`, `Animation`, `Particle`), `Core/Time/FixedTimestep`, `Core/Math`
(Vector2, Rect, MathUtils, DeterministicRandom), `Core/Physics` (`sweepAabb`, `sweepAabbVsAabb`,
`Aabb`), `Core/Diagnostics`, `Core/Levels` (TileMap, chargeur/écrivain/brouillon, LevelScene),
tout `HMI/Graphics`, `HMI/Input`, `HMI/Interface`, `HMI/Audio`, `HMI/Localization`, `HMI/Editor`
(undo/redo, bibliothèque d'assets, atelier pixel art) et l'infrastructure CI/doc/test.

### Supprimé

- `Source/AiSolver/` (12 k lignes), `Source/HMI/Ai/`, `AiModeScreen`, `TrainingChartWidget`,
  `ReplayPlayback`, les rejeux livrés, l'entrée « Mode IA » du menu et son état de `ScreenFlow`.
- `Core/Ecs/Systems/CharacterPhysicsSystem` — **seul** consommateur de la gravité dans tout le
  dépôt (`grep -rn "gravity" Source/Core --include=*.cpp` ne renvoyait que lui).
- `Core/Gameplay/` sauf `MechanismController` (voir ci-dessous) : blocs poussables, plateformes
  mobiles, dangers à état, blocs descendants/fragiles/éphémères.
- `Core/Physics/SlopeGeometry`, `Core/Levels/DangerGeometry`, `HMI/Graphics/SlopeMask`.
- L'outil « Parcours » de l'éditeur (`PathGeometry`, `PathGesture`) et le panneau « Propriétés »,
  qui ne pilotaient que des réglages de plateforme.
- 25 valeurs de `TileType` sur 36, les 26 niveaux de démonstration et leurs plans picturaux, et
  les tests correspondants.

### Conséquence assumée

`hmi::GameSession::update` ne déplace plus le personnage : la physique de plateforme est partie et
son remplaçant top-down (`core::TopDownMovementSystem`) arrive au `LOT-06`. **Le jeu se lance,
charge et affiche une carte, mais n'est pas jouable** dans cet intervalle. L'intention d'entrée
est lue sans être consommée, pour que le système de déplacement se branche sans changer ni la
signature ni l'ordre des passes.

### Identité du dépôt

Renommage en `JustAnotherRpgGame` (cibles CMake, binaire, documentation, workflows) et préfixe de
macro `JADG_`. Version remise à `0.1.0`. Le programme de lots d'origine est mis en archive (le
`LOT-88` l'a depuis retiré du dépôt).

## Conception

### Conservé contre l'attente — deux décisions

**`MechanismController` est resté.** Interrupteur↔porte, plaque de pression, clé↔porte verrouillée
ne sont pas du gameplay de plateforme : c'est le vocabulaire de puzzle qu'un Zelda-like utilise
tel quel. Il a été retiré puis restauré, débarrassé de sa seule partie caduque (les dangers
commutés, dont le type de tuile disparaît). Les cinq types de tuile associés restent donc dans
`TileType`, et le mécanisme reste testé.

**Le détourage par silhouette est resté, vidé.** `SlopeMask` portait à la fois la géométrie des
pentes (caduque) et le mécanisme de découpe partagé par l'atlas procédural, le détourage des skins
(`hmi::TextureCache`) et les vignettes de la palette. Il devient `HMI/Graphics/TileSilhouette`,
dont la liste `SILHOUETTE_TILE_TYPES` est **vide** : trois chaînes d'appel restent fonctionnelles
pour un fichier de quarante lignes, et le vocabulaire de tuiles du `LOT-08` (falaises, bords
d'eau, ponts) la repeuplera.

**`aisolver::GridDistanceField` a été sauvé.** BFS 4-voisins multi-source sur `core::TileMap`, à
lectures `O(1)`, ne dépendant que de `Core` : c'est exactement le calcul de portée de déplacement
du combat tactique. Déplacé en `Core/World/GridDistanceField`, rebasculé dans le namespace `core`,
avec son test — consommé par le `LOT-19`.

## Exigences couvertes

Aucune exigence nouvelle : ce lot **retire**. Les catégories `EX-GP-*` (gameplay de plateforme) et
`EX-IA-*` (solveur autonome) passent en **héritage, hors périmètre** — jamais renumérotées, la
règle du dépôt l'interdit. Les catégories du RPG (`EX-EXP-*`, `EX-REG-*`, `EX-CBT-*`, `EX-RPG-*`)
sont créées par les lots qui les concrétisent.

## Bilan

Statut : **fait**. Vérification automatisée : configuration CMake, build `Core` et
`JustAnotherRpgGame` sans avertissement en `/W4 /WX`, exécutable produit. La vérification IHM
manuelle — menu, éditeur, chargement d'une carte — reste à faire par l'utilisateur, comme pour
tout lot touchant moteur et rendu. C'est le premier lot du dépôt : il n'a aucun prérequis.
