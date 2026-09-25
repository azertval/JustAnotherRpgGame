# Tests d'intégration

Tests d'intégration — **5 cas** (3 critiques, 2 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_exploration_carte.cpp`](#test-exploration-cartecpp) | 4 | - | 2 | 2 | - |
| [`test_quete_trois_etapes.cpp`](#test-quete-trois-etapescpp) | 1 | - | 1 | - | - |

## test_exploration_carte.cpp

### ExplorationCarteIntegration.UneCarteSeChargeEtSeCompose

*Critique · Integration · Exploration* — `Source/Test/Integration/test_exploration_carte.cpp:48`

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

*Majeur · Integration · Exploration* — `Source/Test/Integration/test_exploration_carte.cpp:72`

Marcher sur une carte deplace le heros et change sa bande.

**Étapes**

1. Entrer sur `bourg/place`.
2. Avancer d'une seconde, par pas de 1/60 s, dans une direction libre.

**Résultat attendu**

- Vérifie que `play.enter("bourg/place", {})` est vrai.
- Vérifie que `moved` est vrai.
- Vérifie que `play.figures().back().clip` vaut `"walk"`.

### ExplorationCarteIntegration.UnPasNeRefaitPasLaCarte

*Majeur · Integration · Exploration · Rendu* — `Source/Test/Integration/test_exploration_carte.cpp:104`

Marcher ne recompose pas la carte.

**Étapes**

1. Entrer sur `bourg/place` et prendre la carte en valeurs.
2. Marcher une seconde, par pas de 1/60 s.
3. Poser un drapeau, puis faire un pas.

**Résultat attendu**

- Vérifie que `play.enter("bourg/place", {})` est vrai.
- Vérifie que `before` diffère de `nullptr`.
- Vérifie que `result.sceneChanged` est faux.
- Vérifie que `figuresChanged` est vrai.
- Vérifie que `play.scene().get()` vaut `before.get()`.
- Vérifie que `result.sceneChanged` est vrai.
- Vérifie que `play.scene().get()` diffère de `before.get()`.

### ExplorationCarteIntegration.UneCarteQuiPuiseDansQuatreNiveauxSeJoue

*Critique · Integration · Exploration · Arborescence* — `Source/Test/Integration/test_exploration_carte.cpp:139`

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

## test_quete_trois_etapes.cpp

### QueteIntegration.UneQueteDeTroisEtapesSeJoueSansFenetre

*Critique · Integration · Quetes* — `Source/Test/Integration/test_quete_trois_etapes.cpp:168`

Une quete de trois etapes se joue sans fenetre.

**Étapes**

1. Monter la partie : quetes et dialogues de la racine d'essai, un parvis en memoire ou la mere attend et ou le garde ne parait que sous `quete.essai == acceptee`.
2. Parler a la mere, accepter.
3. Parler au garde, lui faire relacher l'enfant.
4. Revenir a la mere, lui rendre l'enfant.
5. Lire le journal.

**Résultat attendu**

- Vérifie que `partie.erreurs.empty()` est vrai.
- Vérifie que `partie.play().session().quests().find(QUETE)` diffère de `nullptr`.
- Vérifie que `partie.play().enter("parvis", {})` est vrai.
- Vérifie que `partie.figuresDePnj()` vaut `1U`.
- Vérifie que `partie.parlerDepuis({6, 6})` vaut `std::nullopt`.
- Vérifie que `partie.parlerDepuis({4, 4})` vaut `"essai-mere"`.
- Vérifie que `partie.etapesAtteintes(&sceneChangee)` vaut `(std::vector<std::string>{"essai-trois-etapes/acceptee"})`.
- Vérifie que `sceneChangee` est vrai.
- Vérifie que `partie.figuresDePnj()` vaut `2U`.
- Vérifie que `partie.parlerDepuis({6, 6})` vaut `"essai-garde"`.
- Vérifie que `partie.etapesAtteintes()` vaut `(std::vector<std::string>{"essai-trois-etapes/garde-vu"})`.
- Vérifie que `partie.figuresDePnj()` vaut `1U`.
- Vérifie que `partie.parlerDepuis({6, 6})` vaut `std::nullopt`.
- Vérifie que `partie.parlerDepuis({4, 4})` vaut `"essai-mere"`.
- Vérifie que `partie.etapesAtteintes()` vaut `(std::vector<std::string>{"essai-trois-etapes/rendue"})`.
- Vérifie que `drapeaux.isSet("essai/recompense-donnee")` est vrai.
- Vérifie que `core::questProgress(quete, drapeaux).status` vaut `core::QuestStatus::Succeeded`.
- Vérifie que `journal.quests.size()` vaut `1U`.
- Vérifie que `journal.quests.front().value` vaut `"journal.status.succeeded"`.
- Vérifie que `journal.objectives.size()` vaut `3U`.
- Vérifie que `journal.detail` vaut `"quest.essai-trois-etapes.rendue"`.
