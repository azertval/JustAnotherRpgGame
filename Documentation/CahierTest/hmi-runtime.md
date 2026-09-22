# HMI · Runtime

Tests unitaires — **3 cas** (1 critique, 2 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_arena_model.cpp`](#test-arena-modelcpp) | 3 | - | 1 | 2 | - |

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
