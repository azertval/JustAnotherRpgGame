# Tests d'intégration

Tests d'intégration — **3 cas** (2 critiques, 1 majeur). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_exploration_carte.cpp`](#test-exploration-cartecpp) | 3 | - | 2 | 1 | - |

## test_exploration_carte.cpp

### ExplorationCarteIntegration.UneCarteSeChargeEtSeCompose

*Critique · Integration · Exploration* — `Source/Test/Integration/test_exploration_carte.cpp:47`

Une carte du disque se charge et se compose.

**Étapes**

1. Entrer sur la carte `donjon` a son point d'arrivee.
2. Prendre l'instantane de la scene.

**Résultat attendu**

- Vérifie que `play.enter("donjon", {})` est vrai.
- Vérifie que `snapshot.place.empty()` est faux.
- Vérifie que `snapshot.columns` vaut `play.session().map()->tileMap().width()`.
- Vérifie que `snapshot.rows` vaut `play.session().map()->tileMap().height()`.
- Vérifie que `hasDrawnFloor(snapshot)` est vrai.
- Vérifie que `snapshot.figures.empty()` est faux.
- Vérifie que `snapshot.figures.back().figure` vaut `hmi::WorldPlay::DEFAULT_HERO_FIGURE`.

### ExplorationCarteIntegration.LeHerosMarcheSurUneCarte

*Majeur · Integration · Exploration* — `Source/Test/Integration/test_exploration_carte.cpp:71`

Marcher sur une carte deplace le heros et change sa bande.

**Étapes**

1. Entrer sur `bourg/place`.
2. Avancer d'une seconde, par pas de 1/60 s, dans une direction libre.

**Résultat attendu**

- Vérifie que `play.enter("bourg/place", {})` est vrai.
- Vérifie que `moved` est vrai.
- Vérifie que `play.figures().back().clip` vaut `"walk"`.

### ExplorationCarteIntegration.UneCarteQuiPuiseDansQuatreNiveauxSeJoue

*Critique · Integration · Exploration · Arborescence* — `Source/Test/Integration/test_exploration_carte.cpp:103`

Une carte qui puise dans quatre niveaux se joue.

**Étapes**

1. Entrer sur `central-empire/capital/arenarea` de la racine LevelTree.
2. Prendre l'instantane de la scene.

**Résultat attendu**

- Vérifie que `play.enter("central-empire/capital/arenarea", {})` est vrai.
- Vérifie que `snapshot.place` vaut `"central-empire/capital/arenarea"`.
- Vérifie que `hasDrawnFloor(snapshot)` est vrai.
- Vérifie que `std::ranges::any_of(snapshot.pieceFiles, [level](const auto& entry) { return entry.second.starts_with(level); })` est vrai.
- Vérifie que `std::filesystem::is_regular_file(tree / "Assets" / file)` est vrai.
- Vérifie que `snapshot.figureDirectories.at("anariel")` vaut `"Regions/central-empire/capital/arenarea/Characters/anariel"`.
- Vérifie que `snapshot.figureDirectories.at("Peoples/human/guard")` vaut `"Common/Characters/Peoples/human/guard"`.
- Vérifie que `snapshot.figures.empty()` est faux.
- Vérifie que `snapshot.figures.back().figure` vaut `hmi::WorldPlay::DEFAULT_HERO_FIGURE`.
