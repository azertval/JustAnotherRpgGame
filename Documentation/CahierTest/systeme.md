# Tests système

Tests système — **4 cas** (4 critiques). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_demo_de_bout_en_bout.cpp`](#test-demo-de-bout-en-boutcpp) | 3 | - | 3 | - | - |
| [`test_parcours_edition_rpg.cpp`](#test-parcours-edition-rpgcpp) | 1 | - | 1 | - | - |

## test_demo_de_bout_en_bout.cpp

### DemoDeBoutEnBout.LaFinParLaParole

*Critique · Systeme · Demo* — `Source/Test/Systeme/test_demo_de_bout_en_bout.cpp:363`

Nouvelle partie, puis la demo jusqu'a sa fin par la parole.

**Étapes**

1. Nouvelle partie : Market Gate.
2. La mere, accepter.
3. Stravian Avenue, le parvis ; le garde, convaincre, a une graine dont le d20 reussit le DD.
4. Retour a l'etal par les portails ; la mere.

**Résultat attendu**

- `ASSERT_NO_FATAL_FAILURE(jusquAuParvis(jeu))`
- `ASSERT_NO_FATAL_FAILURE(jeu.repondre({"partir"}))`
- Vérifie que `jeu.graineDuDialogue` diffère de `0`.
- Vérifie que `jeu.parler(DEVANT_LE_GARDE)` vaut `"garde"`.
- `ASSERT_NO_FATAL_FAILURE(jeu.repondre({"convaincre", "continue"}))`
- Vérifie que `jeu.valeur()` vaut `"enfant-libere"`.
- Vérifie que `pomper([&jeu] { return jeu.etapes.size() >= 2; }, 1000)` est vrai.
- Vérifie que `jeu.etapes.back()` vaut `"pommes/enfant-libere"`.
- Vérifie que `jeu.parler(DEVANT_LE_GARDE)` vaut `std::nullopt`.
- `ASSERT_NO_FATAL_FAILURE(finirChezLaMere(jeu, "parole"))`

### DemoDeBoutEnBout.LaFinParLArene

*Critique · Systeme · Demo* — `Source/Test/Systeme/test_demo_de_bout_en_bout.cpp:394`

Nouvelle partie, puis la demo jusqu'a sa fin par l'arene.

**Étapes**

1. Jusqu'au parvis ; le garde a une graine dont le d20 echoue : convaincre, puis endosser.
2. L'escalier de l'arene : le vestiaire, la porte close ; la porte du triomphe : le sable.
3. Le maitre d'arene engage la rencontre ; la jouer a la premiere graine qui la gagne.
4. Redescendre, passer la porte ouverte, revenir a l'etal.

**Résultat attendu**

- `ASSERT_NO_FATAL_FAILURE(jusquAuParvis(jeu))`
- `ASSERT_NO_FATAL_FAILURE(jeu.repondre({"partir"}))`
- Vérifie que `jeu.graineDuDialogue` diffère de `0`.
- Vérifie que `jeu.parler(DEVANT_LE_GARDE)` vaut `"garde"`.
- `ASSERT_NO_FATAL_FAILURE(jeu.repondre({"convaincre", "endosser", "continue"}))`
- Vérifie que `jeu.valeur()` vaut `"condamne"`.
- Vérifie que `pomper([&jeu] { return jeu.etapes.size() >= 3; }, 1000)` est vrai.
- Vérifie que `jeu.etapes` vaut `(std::vector<std::string>{"pommes/acceptee", "pommes/persuasion-echouee", "pommes/condamne"})`.
- Vérifie que `jeu.passerLePortail(DEVANT_L_ESCALIER, {1.0F, 0.0F}, VESTIAIRES)` est vrai.
- Vérifie que `jeu.heros()` vaut `ARRIVEE_AUX_VESTIAIRES`.
- Vérifie que `jeu.passerLePortail(ARRIVEE_AUX_VESTIAIRES, {1.0F, 0.0F}, ARENAREA)` est faux.
- Vérifie que `jeu.carte()` vaut `VESTIAIRES`.
- Vérifie que `jeu.passerLePortail(PIED_DE_L_ESCALIER, {0.0F, -1.0F}, SABLE)` est vrai.
- Vérifie que `jeu.parler(DEVANT_LE_MAITRE)` vaut `"maitre-arene"`.
- `ASSERT_NO_FATAL_FAILURE(jeu.repondre({"combattre"}))`
- Vérifie que `jeu.dialogueEngage.has_value()` est vrai.
- Vérifie que `issue.empty()` est faux.
- Vérifie que `jeu.router.currentScreen()` diffère de `Screen::Death`.
- Vérifie que `gagnante.has_value()` est vrai.
- Vérifie que `jeu.monde.flags().isSet(core::encounterWonFlag(RENCONTRE))` est vrai.
- Vérifie que `pomper([&jeu] { return jeu.etapes.size() >= 5; }, 1000)` est vrai.
- Vérifie que `jeu.etapes.back()` vaut `"pommes/enfant-libere"`.
- Vérifie que `jeu.valeur()` vaut `"enfant-libere"`.
- Vérifie que `jeu.parler(DEVANT_LE_MAITRE)` vaut `std::nullopt`.
- Vérifie que `jeu.passerLePortail(PORTE_DU_TRIOMPHE, {0.0F, -1.0F}, VESTIAIRES)` est vrai.
- Vérifie que `jeu.passerLePortail(ARRIVEE_AUX_VESTIAIRES, {1.0F, 0.0F}, ARENAREA)` est vrai.
- `ASSERT_NO_FATAL_FAILURE(finirChezLaMere(jeu, "arene"))`

### DemoDeBoutEnBout.LaMortSurLeSable

*Critique · Systeme · Demo* — `Source/Test/Systeme/test_demo_de_bout_en_bout.cpp:459`

Nouvelle partie, puis la demo jusqu'a la mort sur le sable.

**Étapes**

1. Jusqu'au sable, condamne.
2. Le combat a la premiere graine qui le perd.
3. Recommencer.

**Résultat attendu**

- `ASSERT_NO_FATAL_FAILURE(jusquAuParvis(jeu))`
- `ASSERT_NO_FATAL_FAILURE(jeu.repondre({"endosser", "continue"}))`
- Vérifie que `jeu.valeur()` vaut `"condamne"`.
- Vérifie que `jeu.passerLePortail(DEVANT_L_ESCALIER, {1.0F, 0.0F}, VESTIAIRES)` est vrai.
- Vérifie que `jeu.passerLePortail(PIED_DE_L_ESCALIER, {0.0F, -1.0F}, SABLE)` est vrai.
- Vérifie que `jeu.parler(DEVANT_LE_MAITRE)` vaut `"maitre-arene"`.
- `ASSERT_NO_FATAL_FAILURE(jeu.repondre({"combattre"}))`
- Vérifie que `issue.empty()` est faux.
- Vérifie que `perdante.has_value()` est vrai.
- Vérifie que `jeu.router.currentScreen()` vaut `Screen::Death`.
- Vérifie que `jeu.rencontre.active()` est vrai.
- Vérifie que `jeu.monde.flags().isSet(core::encounterWonFlag(RENCONTRE))` est faux.
- Vérifie que `jeu.monde.loaded()` est faux.
- Vérifie que `jeu.monde.startNewGame()` est vrai.
- Vérifie que `jeu.carte()` vaut `MARTPART`.
- Vérifie que `jeu.heros()` vaut `MARKET_GATE`.
- Vérifie que `jeu.valeur()` vaut `"inconnue"`.

## test_parcours_edition_rpg.cpp

### ParcoursEditionSysteme.ProduitUneCarteDuRpgSansEcrireDeJson

*Critique · Système · Éditeur de niveaux* — `Source/Test/Systeme/test_parcours_edition_rpg.cpp:63`

Produire une carte du RPG dans l'editeur, sans JSON ecrit a la main.

**Étapes**

1. Creer une carte, poser l'entree et un couloir de murs dans la collision.
2. Ajouter un sol (qui reprend l'image) puis un decor, peindre l'un et l'autre.
3. Poser un PNJ au dialogue du garde, un coffre, un point d'arrivee et un portail qui y mene, une rencontre en terrain ouvert et une dans le couloir.
4. Valider contre les catalogues d essai et analyser le terrain.
5. Enregistrer, recharger, construire le graphe du monde et peupler le monde ECS.
6. Tout annuler.

**Résultat attendu**

- Vérifie que `ground && decor` est vrai.
- Vérifie que `draft.layers()[*ground].tiles.tile(12, 5)` vaut `core::TileType::Wall`.
- Vérifie que `draft.paintLayerRegion(*ground, 0, 0, std::vector<std::vector<core::TileType>>( 4, std::vector<core::TileType>(6, core::TileType::Grass)))` est vrai.
- Vérifie que `draft.paintLayerTile(*decor, 3, 3, core::TileType::Water)` est vrai.
- Vérifie que `draft.paintLayerTile(*decor, 3, 4, core::TileType::Entry)` est faux.
- Vérifie que `core::validateMapEntities(draft.entities(), context).empty()` est vrai.
- Vérifie que `terrains.size()` vaut `2U`.
- Vérifie que `terrains[0].entityIndex` vaut `open`.
- Vérifie que `terrains[0].valid()` est vrai.
- Vérifie que `terrains[1].entityIndex` vaut `corridor`.
- Vérifie que `terrains[1].valid()` est faux.
- Vérifie que `validated.ok()` est vrai.
- Vérifie que `core::LevelWriter::saveToFile(*validated.level, directory / "parcours-lot-11.json")` est vrai.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `reloaded.level->layers().size()` vaut `3U`.
- Vérifie que `reloaded.level->layers()[*decor].tiles.tile(3, 3)` vaut `core::TileType::Water`.
- Vérifie que `reloaded.level->entities().size()` vaut `6U`.
- Vérifie que `world.portals.size()` vaut `1U`.
- Vérifie que `world.portals.front().status` vaut `core::PortalLinkStatus::Resolved`.
- Vérifie que `core::spawnMapEntities(ecs, *reloaded.level, reloaded.level->name())` vaut `6U`.
- Vérifie que `interactables` vaut `(std::vector<std::string>{"chest", "npc"})`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.entities().empty()` est vrai.
- Vérifie que `draft.layers().empty()` est vrai.
