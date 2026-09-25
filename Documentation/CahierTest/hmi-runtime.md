# HMI · Runtime

Tests unitaires — **7 cas** (1 bloquant, 2 critiques, 4 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_arena_model.cpp`](#test-arena-modelcpp) | 4 | - | 2 | 2 | - |
| [`test_encounter_model.cpp`](#test-encounter-modelcpp) | 3 | 1 | - | 2 | - |

## test_arena_model.cpp

### ArenaModelTest.LaReactionSeBasculeDepuisLaBarre

*Critique · Unitaire · IHM* — `Source/Test/Unit/HMI/Runtime/test_arena_model.cpp:55`

La derniere action du tour est la reaction : la confirmer fait laisser passer les attaques d'opportunite, la confirmer encore les fait saisir ; les actions du Manuel se choisissent au numero et en boucle.

**Étapes**

1. Lancer ; lire les actions.
2. Choisir la derniere, confirmer, relire ; confirmer encore.
3. Passer a l'action suivante depuis la derniere.

**Résultat attendu**

- Vérifie que `actions.size()` est supérieur ou égal à `5`.
- Vérifie que `actions[derniere - 3].toMap().value("kind").toString()` vaut `"dodge"`.
- Vérifie que `actions[derniere - 2].toMap().value("kind").toString()` vaut `"disengage"`.
- Vérifie que `actions[derniere - 1].toMap().value("kind").toString()` vaut `"dash"`.
- Vérifie que `actions[derniere].toMap().value("kind").toString()` vaut `"reaction"`.
- Vérifie que `arena.turnActions()[derniere].toMap().value("selected").toBool()` est vrai.
- Vérifie que `arena.preview().isEmpty()` est faux.
- Vérifie que `apres` diffère de `avant`.
- Vérifie que `arena.turnActions()[derniere].toMap().value("label").toString()` vaut `avant`.
- Vérifie que `arena.turnActions()[0].toMap().value("selected").toBool()` est vrai.

### ArenaModelTest.LeSurvolPoseLeCurseur

*Majeur · Unitaire · IHM* — `Source/Test/Unit/HMI/Runtime/test_arena_model.cpp:93`

pointCursor pose le curseur sur la case survolee, et ignore ce qui n'en est pas une.

**Étapes**

1. Avant le combat, pointer une case.
2. Lancer ; pointer l'ennemi le plus proche depuis une autre case.
3. Pointer hors de la grille.

**Résultat attendu**

- Vérifie que `arena.cursorColumn()` vaut `colonneInitiale`.
- Vérifie que `arena.cursorRow()` vaut `ligneInitiale`.
- Vérifie que `arena.cursorColumn() == colonne && arena.cursorRow() == ligne` est faux.
- Vérifie que `arena.cursorColumn()` vaut `colonne`.
- Vérifie que `arena.cursorRow()` vaut `ligne`.
- Vérifie que `arena.cursorColumn()` vaut `colonne`.
- Vérifie que `arena.cursorRow()` vaut `ligne`.

### ArenaModelTest.LeCalqueDeLaGrilleDecritCombattantsEtCasesAtteignables

*Majeur · Unitaire · IHM* — `Source/Test/Unit/HMI/Runtime/test_arena_model.cpp:128`

fighters decrit chaque combattant sur la grille et garde secrets les points de vie ennemis ; reachableCells ne liste que des cases libres.

**Étapes**

1. Avant le combat, lire les deux listes.
2. Lancer ; lire les combattants et les cases atteignables.

**Résultat attendu**

- Vérifie que `arena.fighters().isEmpty()` est vrai.
- Vérifie que `arena.reachableCells().isEmpty()` est vrai.
- Vérifie que `fighters.size()` vaut `2`.
- Vérifie que `fighter.value("footprint").toInt()` est supérieur ou égal à `1`.
- Vérifie que `hitPoints.contains('/')` est vrai.
- Vérifie que `fighter.value("side").toString()` vaut `"enemies"`.
- Vérifie que `hitPoints.contains('/')` est faux.
- Vérifie que `actifs` vaut `1`.
- Vérifie que `reachable.isEmpty()` est faux.
- Vérifie que `cell.value("column") == fighter.value("column") && cell.value("row") == fighter.value("row")` est faux.

### ArenaModelTest.LEnnemiDeLIaMarcheVersLeJoueur

*Critique · Unitaire · IHM · IA* — `Source/Test/Unit/HMI/Runtime/test_arena_model.cpp:176`

L'ennemi de l'IA marche vers le joueur sur la carte de l'arene.

**Étapes**

1. Lancer le personnage contre un sanglier, IA en marche, sur la zone « salle » du donjon d'essai (les deux camps entrent a dix-sept cases l'un de l'autre).
2. Finir le tour du joueur sans bouger, trois fois.

**Résultat attendu**

- Vérifie que `allie` vaut `1`.
- Vérifie que `ennemi` vaut `1`.
- Vérifie que `avant` est strictement supérieur à `2`.
- Vérifie que `apres < avant || attaque` est vrai.
- Vérifie que `std::ranges::any_of(journal, [](const QString& ligne) { return ligne.contains(QStringLiteral("deplacement refuse")); })` est faux.
- Vérifie que `std::ranges::any_of(arena.journal(), [](const QString& ligne) { return ligne.startsWith(QStringLiteral("pas ")); })` est vrai.

## test_encounter_model.cpp

### EncounterModelTest.DuDeclenchementAuRetourALExploration

*Bloquant · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:65`

Du declenchement sur la carte au retour a l'exploration, sans fenetre.

**Étapes**

1. Ouvrir le donjon d'essai, le heros devant le maitre d'arene, dans la zone « salle ».
2. Engager « rats-du-donjon » a la graine 2026.
3. Jouer : attaquer le rat le plus proche, finir le tour, jusqu'a l'issue ou dix rounds.
4. Quitter.

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `rencontre.active()` est vrai.
- Vérifie que `monde.frozen()` est vrai.
- Vérifie que `monde.showsCombat()` est vrai.
- Vérifie que `rencontre.zoneColumn()` vaut `10`.
- Vérifie que `rencontre.zoneRow()` vaut `10`.
- Vérifie que `rencontre.setup()` diffère de `nullptr`.
- Vérifie que `rencontre.setup()->heroCell` vaut `(core::GridPosition{.column = 14, .row = 9})`.
- Vérifie que `rencontre.fighters().size()` vaut `4`.
- Vérifie que `rencontre.encounterName()` vaut `QStringLiteral("Les rats du donjon")`.
- Vérifie que `figure.combatant` est vrai.
- Vérifie que `figure.point.x` est supérieur ou égal à `10.0F`.
- Vérifie que `figure.point.y` est supérieur ou égal à `10.0F`.
- Vérifie que `heros` est vrai.
- Vérifie que `bandes.contains(std::string{hmi::figure_clips::ATTACK})` est vrai.
- Vérifie que `bandes.contains(std::string{hmi::figure_clips::DEATH})` est vrai.
- Vérifie que `rencontre.ended()` est vrai.
- Vérifie que `rencontre.outcome().isEmpty()` est faux.
- Vérifie que `rencontre.active()` est faux.
- Vérifie que `monde.frozen()` est faux.
- Vérifie que `monde.showsCombat()` est faux.
- Vérifie que `fini.size()` vaut `1`.
- Vérifie que `fini.front()` vaut `issue`.
- Vérifie que `rencontre.setup() == nullptr` est vrai.
- Vérifie que `arrivee.column` est supérieur ou égal à `10`.
- Vérifie que `arrivee.row` est supérieur ou égal à `10`.
- Vérifie que `figure.combatant` est faux.
- Vérifie que `mannequin` est vrai.

### EncounterModelTest.UnRefusLaisseLExplorationIntacte

*Majeur · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:156`

Un refus de montage laisse l'exploration intacte.

**Étapes**

1. Ouvrir le donjon a la porte (19, 32), hors de la zone.
2. Engager une rencontre inconnue, puis « rats-du-donjon ».

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("dragons"))` est faux.
- Vérifie que `rencontre.active()` est faux.
- Vérifie que `rencontre.status().isEmpty()` est faux.
- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est faux.
- Vérifie que `rencontre.active()` est faux.
- Vérifie que `monde.frozen()` est faux.
- Vérifie que `monde.showsCombat()` est faux.
- Vérifie que `rencontre.status().indexOf(QStringLiteral("zone"))` diffère de `-1`.

### EncounterModelTest.LesGestesAttendentLaFinDUnMouvement

*Majeur · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:184`

Les gestes attendent la fin d'un mouvement.

**Étapes**

1. Engager les rats, avancer d'un pas : l'IA a pu jouer, la file est occupee.
2. Tant que la file joue, finir le tour ; puis sauter l'animation.

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `rencontre.busy()` est vrai.
- Vérifie que `rencontre.journal().size()` vaut `lignes`.
- Vérifie que `rencontre.busy()` est faux.
- Vérifie que `rencontre.active()` est vrai.
- Vérifie que `rencontre.ended()` est vrai.
- Vérifie que `rencontre.active()` est faux.
