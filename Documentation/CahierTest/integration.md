# Tests d'intégration

Tests d'intégration — **11 cas** (6 critiques, 5 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_exploration_carte.cpp`](#test-exploration-cartecpp) | 4 | - | 2 | 2 | - |
| [`test_quete_des_pommes.cpp`](#test-quete-des-pommescpp) | 6 | - | 3 | 3 | - |
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
- Vérifie que `snapshot.figures.back().hero` est vrai.
- Vérifie que `snapshot.figures.back().figure` vaut `play.heroResolved().directory`.
- Vérifie que `play.heroResolved().directory` vaut `hmi::placeholderFigureDirectory("humanoid")`.
- Vérifie que `play.heroResolved().placeholder` est vrai.

### ExplorationCarteIntegration.LeHerosMarcheSurUneCarte

*Majeur · Integration · Exploration* — `Source/Test/Integration/test_exploration_carte.cpp:76`

Marcher sur une carte deplace le heros et change sa bande.

**Étapes**

1. Entrer sur `bourg/place`.
2. Avancer d'une seconde, par pas de 1/60 s, dans une direction libre.

**Résultat attendu**

- Vérifie que `play.enter("bourg/place", {})` est vrai.
- Vérifie que `moved` est vrai.
- Vérifie que `play.figures().back().clip` vaut `"walk"`.

### ExplorationCarteIntegration.UnPasNeRefaitPasLaCarte

*Majeur · Integration · Exploration · Rendu* — `Source/Test/Integration/test_exploration_carte.cpp:108`

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

*Critique · Integration · Exploration · Arborescence* — `Source/Test/Integration/test_exploration_carte.cpp:143`

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
- Vérifie que `play.resolveFigure("anariel", {}).directory` vaut `anariel`.
- Vérifie que `play.resolveFigure("Peoples/human/guard", {}).directory` vaut `garde`.
- Vérifie que `play.resolveFigure("anariel", {}).placeholder` est faux.
- Vérifie que `std::ranges::any_of(snapshot.figures, [&dossier](const auto& figure) { return figure.figure == dossier; })` est vrai.
- Vérifie que `snapshot.figureDirectories.at(dossier)` vaut `dossier`.
- Vérifie que `snapshot.figures.empty()` est faux.
- Vérifie que `snapshot.figures.back().hero` est vrai.
- Vérifie que `snapshot.figures.back().figure` vaut `play.heroResolved().directory`.

## test_quete_des_pommes.cpp

### QueteDesPommes.LaVoieDeLaParole

*Critique · Integration · Quete de la demo* — `Source/Test/Integration/test_quete_des_pommes.cpp:391`

La demo se finit par la parole quand la Persuasion reussit.

**Étapes**

1. Market Gate, la mere, accepter.
2. Stravian Avenue par le portail ; le parvis declenche le garde.
3. Convaincre, avec un jet qui reussit.
4. Revenir a l'etal par les portails, parler a la mere.

**Résultat attendu**

- `ASSERT_NO_FATAL_FAILURE(jusquAuGarde(partie))`
- Vérifie que `partie.valeur()` vaut `"enfant-libere"`.
- Vérifie que `partie.etapesAtteintes()` vaut `(std::vector<std::string>{"pommes/enfant-libere"})`.
- Vérifie que `partie.parlerDepuis({GARDE.column - 1, GARDE.row})` vaut `std::nullopt`.
- Vérifie que `partie.parlerDepuis({ENFANT_AU_PARVIS.column - 1, ENFANT_AU_PARVIS.row})` vaut `std::nullopt`.
- `ASSERT_NO_FATAL_FAILURE(retourChezLaMere(partie, "parole"))`

### QueteDesPommes.LaVoieDeLArene

*Critique · Integration · Quete de la demo* — `Source/Test/Integration/test_quete_des_pommes.cpp:420`

La demo se finit par l'arene quand le joueur endosse le crime et gagne.

**Étapes**

1. Jusqu'au garde ; convaincre avec un jet qui echoue, puis endosser.
2. L'escalier de l'arene : on arrive au vestiaire A, et la porte du couloir arrete le pas.
3. La porte du triomphe : le sable ; le maitre d'arene engage la rencontre ; la jouer a la premiere graine qui la gagne.
4. Redescendre, passer la porte ouverte, revenir a l'etal.

**Résultat attendu**

- `ASSERT_NO_FATAL_FAILURE(jusquAuGarde(partie))`
- Vérifie que `partie.valeur()` vaut `"condamne"`.
- Vérifie que `partie.etapesAtteintes()` vaut `(std::vector<std::string>{"pommes/persuasion-echouee", "pommes/condamne"})`.
- Vérifie que `partie.parlerDepuis({GARDE.column - 1, GARDE.row})` vaut `std::nullopt`.
- Vérifie que `partie.marcherJusquA(DEVANT_L_ESCALIER, {1.0F, 0.0F}, core::ExplorationEventKind::MapEntered)` vaut `VESTIAIRES`.
- Vérifie que `partie.session().heroCell()` vaut `ARRIVEE_AUX_VESTIAIRES`.
- Vérifie que `partie.marcherJusquA(ARRIVEE_AUX_VESTIAIRES, {1.0F, 0.0F}, core::ExplorationEventKind::MapEntered)` vaut `std::nullopt`.
- Vérifie que `partie.session().heroCell().column` est strictement inférieur à `PORTE_DE_L_ARENE.column`.
- Vérifie que `partie.marcherJusquA(PIED_DE_L_ESCALIER, {0.0F, -1.0F}, core::ExplorationEventKind::MapEntered)` vaut `SABLE`.
- Vérifie que `partie.parlerDepuis(DEVANT_LE_MAITRE)` vaut `"maitre-arene"`.
- Vérifie que `defi.rencontres` vaut `(std::vector<std::string>{std::string{RENCONTRE}})`.
- Vérifie que `heros.loaded.errors.empty()` est vrai.
- Vérifie que `arene.bestiary.find("combattant-de-l-arene") != nullptr` est vrai.
- Vérifie que `sable` diffère de `nullptr`.
- Vérifie que `gagnante.has_value()` est vrai.
- Vérifie que `partie.drapeaux().isSet(core::encounterWonFlag(RENCONTRE))` est vrai.
- Vérifie que `partie.etapesAtteintes()` vaut `(std::vector<std::string>{"pommes/victoire", "pommes/enfant-libere"})`.
- Vérifie que `partie.valeur()` vaut `"enfant-libere"`.
- Vérifie que `partie.parlerDepuis(DEVANT_LE_MAITRE)` vaut `std::nullopt`.
- Vérifie que `partie.marcherJusquA(PORTE_DU_TRIOMPHE, {0.0F, -1.0F}, core::ExplorationEventKind::MapEntered)` vaut `VESTIAIRES`.
- Vérifie que `partie.marcherJusquA(ARRIVEE_AUX_VESTIAIRES, {1.0F, 0.0F}, core::ExplorationEventKind::MapEntered)` vaut `ARENAREA`.
- Vérifie que `partie.session().heroCell()` vaut `DEVANT_L_ESCALIER`.
- `ASSERT_NO_FATAL_FAILURE(retourChezLaMere(partie, "arene"))`

### QueteDesPommes.LaDefaiteSurLeSable

*Majeur · Integration · Quete de la demo* — `Source/Test/Integration/test_quete_des_pommes.cpp:501`

Une defaite sur le sable ne pose rien : la demo s'y termine.

**Étapes**

1. Condamne, sur le sable, la rencontre engagee.
2. La jouer a la premiere graine qui la perd.

**Résultat attendu**

- Vérifie que `partie.erreurs.empty()` est vrai.
- Vérifie que `partie.session().flags().setValue(DRAPEAU, "condamne")` est vrai.
- Vérifie que `partie.play().enter(SABLE, "from-undercroft")` est vrai.
- Vérifie que `partie.parlerDepuis(DEVANT_LE_MAITRE)` vaut `"maitre-arene"`.
- Vérifie que `sable` diffère de `nullptr`.
- Vérifie que `perdante.has_value()` est vrai.
- Vérifie que `partie.drapeaux().isSet(core::encounterWonFlag(RENCONTRE))` est faux.
- Vérifie que `partie.etapesAtteintes().empty()` est vrai.
- Vérifie que `partie.valeur()` vaut `"condamne"`.

### QueteDesPommes.LeCombatSeGagneDeuxFoisSurTrois

*Critique · Integration · Quete de la demo · Equilibrage* — `Source/Test/Integration/test_quete_des_pommes.cpp:539`

Le heros gagne le combat de l'arene entre 60 et 70 fois sur cent.

**Étapes**

1. Le sable, la rencontre de l'arene, le heros de la demo joue par l'IA.
2. Cent combats, aux graines 1 a 100.

**Résultat attendu**

- Vérifie que `sable.ok()` est vrai.
- Vérifie que `heros.loaded.errors.empty()` est vrai.
- Vérifie que `issue.has_value()` est vrai.
- Vérifie que `victoires` est supérieur ou égal à `60`.
- Vérifie que `victoires` est inférieur ou égal à `70`.

### QueteDesPommes.LaProbabiliteDeVictoireTientSurMilleGraines

*Majeur · Integration · Quete de la demo · Equilibrage* — `Source/Test/Integration/test_quete_des_pommes.cpp:573`

Sur mille combats a graines tirees, le heros gagne deux fois sur trois.

**Étapes**

1. Mille graines tirees de la graine maitresse 120.
2. Un combat par graine, les deux camps par l'IA.

**Résultat attendu**

- Chaque combat se termine ; entre 600 et 700 victoires.

### QueteDesPommes.LaPersuasionReussitUneFoisSurQuatre

*Majeur · Integration · Quete de la demo · Equilibrage* — `Source/Test/Integration/test_quete_des_pommes.cpp:610`

Sur deux mille jets a graines tirees, la Persuasion reussit une fois sur quatre.

**Étapes**

1. Le modificateur de Persuasion du heros de la demo, par sa fiche.
2. Deux mille jets contre le DD du degre « moyenne », a graines tirees d'une graine maitresse.

**Résultat attendu**

- Entre 20 % et 30 % de reussites.

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
