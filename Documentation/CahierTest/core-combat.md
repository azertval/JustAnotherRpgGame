# Core · Combat

Tests unitaires — **116 cas** (33 bloquants, 51 critiques, 31 majeurs, 1 mineur). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_action_economy.cpp`](#test-action-economycpp) | 3 | - | 1 | 2 | - |
| [`test_area_of_effect.cpp`](#test-area-of-effectcpp) | 3 | 1 | 1 | 1 | - |
| [`test_arena.cpp`](#test-arenacpp) | 7 | 2 | 5 | - | - |
| [`test_attack.cpp`](#test-attackcpp) | 9 | 3 | 4 | 2 | - |
| [`test_battle_grid.cpp`](#test-battle-gridcpp) | 7 | 1 | 2 | 4 | - |
| [`test_combat_preview.cpp`](#test-combat-previewcpp) | 2 | 1 | 1 | - | - |
| [`test_combat_state.cpp`](#test-combat-statecpp) | 13 | 6 | 6 | 1 | - |
| [`test_damage.cpp`](#test-damagecpp) | 7 | 3 | 3 | 1 | - |
| [`test_encounter.cpp`](#test-encountercpp) | 12 | - | 7 | 5 | - |
| [`test_enemy_ai.cpp`](#test-enemy-aicpp) | 12 | 5 | 7 | - | - |
| [`test_iso_projection.cpp`](#test-iso-projectioncpp) | 9 | 5 | - | 3 | 1 |
| [`test_line_of_sight.cpp`](#test-line-of-sightcpp) | 3 | 1 | 1 | 1 | - |
| [`test_map_encounter.cpp`](#test-map-encountercpp) | 3 | - | 2 | 1 | - |
| [`test_pathfinding.cpp`](#test-pathfindingcpp) | 13 | 4 | 6 | 3 | - |
| [`test_tactical_terrain.cpp`](#test-tactical-terraincpp) | 10 | - | 4 | 6 | - |
| [`test_turn_order.cpp`](#test-turn-ordercpp) | 3 | 1 | 1 | 1 | - |

## Exigences vérifiées par cette page

Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la [matrice de traçabilité](couverture-exigences.md) les rassemble toutes.

| Exigence | Cas |
|---|---|
| `EX-CBT-031` | [`DamageTest.LeCritiqueDoubleLesDesPasLeModificateur`](#damagetestlecritiquedoublelesdespaslemodificateur) |
| `EX-CBT-050` | [`EnemyAiTest.LIaNeLitQueLEtatEnsanglante`](#enemyaitestlianelitqueletatensanglante) |
| `EX-REG-003` | [`AttackTest.ChaqueJetProduitUneEntreeDeJournalComplete`](#attacktestchaquejetproduituneentreedejournalcomplete) |

## test_action_economy.cpp

### ActionEconomyTest.ChaqueRessourceUneFoisParTour

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_action_economy.cpp:22`

Action, action bonus et reaction se depensent une fois ; le deplacement se depense en plusieurs fois sans jamais revenir dans le tour.

**Étapes**

1. Economie standard a 6 cases.
2. Depenser deux fois l'action, l'action bonus et la reaction.
3. Depenser 2 puis 4 cases, puis 1.
4. Depenser 0, une ressource inconnue, puis restaurer.

**Résultat attendu**

- Vérifie que `economie.spend(ressource)` est vrai.
- Vérifie que `economie.spend(ressource)` est faux.
- Vérifie que `economie.remaining(ressource)` vaut `0`.
- Vérifie que `economie.spend(core::MOVEMENT_RESOURCE, 2)` est vrai.
- Vérifie que `economie.spend(core::MOVEMENT_RESOURCE, 5)` est faux.
- Vérifie que `economie.remaining(core::MOVEMENT_RESOURCE)` vaut `4`.
- Vérifie que `economie.spend(core::MOVEMENT_RESOURCE, 4)` est vrai.
- Vérifie que `economie.spend(core::MOVEMENT_RESOURCE, 1)` est faux.
- Vérifie que `economie.spend(core::MOVEMENT_RESOURCE, 0)` est faux.
- Vérifie que `economie.spend("inconnue")` est faux.
- Vérifie que `economie.has("inconnue")` est faux.
- Vérifie que `economie.remaining(core::ACTION_RESOURCE)` vaut `1`.
- Vérifie que `economie.remaining(core::BONUS_ACTION_RESOURCE)` vaut `1`.
- Vérifie que `economie.remaining(core::REACTION_RESOURCE)` vaut `1`.
- Vérifie que `economie.remaining(core::MOVEMENT_RESOURCE)` vaut `6`.

### ActionEconomyTest.UneRessourceSeDeclareEtSOctroie

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_action_economy.cpp:61`

Une ressource nouvelle se declare sans rien casser, et un octroi ne dure que jusqu'au prochain debut de tour.

**Étapes**

1. Declarer « heroicAction » a 1 par tour.
2. Octroyer une reaction, puis une ressource inconnue.
3. Restaurer.

**Résultat attendu**

- Vérifie que `noms` vaut `(std::vector<std::string>{"action", "bonusAction", "reaction", "movement", "heroicAction"})`.
- Vérifie que `economie.spend("heroicAction")` est vrai.
- Vérifie que `economie.remaining(core::REACTION_RESOURCE)` vaut `2`.
- Vérifie que `economie.remaining("sursis")` vaut `2`.
- Vérifie que `economie.remaining(core::REACTION_RESOURCE)` vaut `1`.
- Vérifie que `economie.remaining("heroicAction")` vaut `1`.
- Vérifie que `economie.remaining("sursis")` vaut `0`.

### CombatCountersTest.LesPorteesSontSeparees

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_action_economy.cpp:95`

« Une fois par tour », « une fois par rencontre » et « immunise 24 heures » se comptent separement.

**Étapes**

1. Incrementer un meme compteur aux portees tour, rencontre et jour.
2. Vider la portee tour.
3. Immuniser un gobelin contre la presence d'un dragon pendant 24 h.

**Résultat attendu**

- Vérifie que `compteurs.increment(core::CounterScope::Turn, "moine", "posture")` vaut `1`.
- Vérifie que `compteurs.increment(core::CounterScope::Turn, "moine", "posture")` vaut `2`.
- Vérifie que `compteurs.value(core::CounterScope::Turn, "moine", "posture")` vaut `0`.
- Vérifie que `compteurs.value(core::CounterScope::Encounter, "moine", "posture")` vaut `1`.
- Vérifie que `compteurs.value(core::CounterScope::Day, "moine", "posture")` vaut `3`.
- Vérifie que `compteurs.value(core::CounterScope::Day, "gobelin", "posture")` vaut `0`.
- Vérifie que `immunites.isImmune("gobelin", "dragon-rouge", maintenant)` est vrai.
- Vérifie que `immunites.isImmune("gobelin", "dragon-bleu", maintenant)` est faux.
- Vérifie que `immunites.isImmune("ogre", "dragon-rouge", maintenant)` est faux.
- Vérifie que `immunites.isImmune("gobelin", "dragon-rouge", maintenant + core::IMMUNITY_DAY_SECONDS - 1)` est vrai.
- Vérifie que `immunites.isImmune("gobelin", "dragon-rouge", maintenant + core::IMMUNITY_DAY_SECONDS - 1)` est faux.

## test_area_of_effect.cpp

### AreaOfEffectTest.LesGabaritsCouvrentLesCasesDeReference

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_area_of_effect.cpp:69`

Sphère, cylindre, cône, ligne et cube couvrent chacun les cases de leur dessin de référence : une case est dans la zone si la forme en couvre au moins la moitié.

**Étapes**

1. Sphère de rayon 2 sur une intersection ; cylindre identique.
2. Cône de 3 vers l'est depuis le milieu du bord d'une case.
3. Ligne de 4 sur 1 vers l'est depuis le milieu d'un bord ; la même en diagonale depuis un coin.
4. Cube d'arête 2 vers le nord depuis une intersection.

**Résultat attendu**

- Vérifie que `gabarit(sphere, 8)` vaut `boule`.
- Vérifie que `gabarit(cylindre, 8)` vaut `boule`.
- Vérifie que `gabarit(cone, 7)` vaut `souffle`.
- Vérifie que `gabarit(ligne, 5)` vaut `(std::vector<std::string>{".....", "XXXX.", ".....", ".....", "....."})`.
- Vérifie que `gabarit(diagonale, 5)` vaut `(std::vector<std::string>{"X....", ".X...", "..X..", ".....", "....."})`.
- Vérifie que `gabarit(cube, 5)` vaut `(std::vector<std::string>{".....", ".....", ".XX..", ".XX..", "....."})`.

### AreaOfEffectTest.LOrigineEtLesTailles

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_area_of_effect.cpp:139`

Une sphère posée au centre d'une case la contient ; un cône qui part du même centre n'en couvre qu'un huitième et ne la contient pas ; un cône sans direction ne couvre rien.

**Étapes**

1. Sphère de rayon 1 au centre de la case (2, 2).
2. Cône de 2 vers l'est depuis le même centre.
3. Un cône dont la direction est son origine.
4. Convertir 6 m, 4,50 m, 7,50 m et 1 m en cases.

**Résultat attendu**

- Vérifie que `contient(core::areaTemplate(sphere, 5, 5), GridPosition{2, 2})` est vrai.
- Vérifie que `contient(souffle, GridPosition{2, 2})` est faux.
- Vérifie que `contient(souffle, GridPosition{3, 2})` est vrai.
- Vérifie que `core::areaTemplate(sansDirection, 5, 5).empty()` est vrai.
- Vérifie que `core::areaTilesFromMeters(6.0F)` vaut `4`.
- Vérifie que `core::areaTilesFromMeters(4.5F)` vaut `3`.
- Vérifie que `core::areaTilesFromMeters(7.5F)` vaut `5`.
- Vérifie que `core::areaTilesFromMeters(1.0F).has_value()` est faux.

### AreaOfEffectTest.UnMurArreteLEffet

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_area_of_effect.cpp:179`

Une sphère posée derrière un mur n'atteint pas les cases qu'aucune ligne droite ne relie à son origine, ni le mur lui-même ; une créature de grande taille à moitié dans la zone y est, une fois.

**Étapes**

1. Une grille 7 × 7 barrée d'un mur vertical sur cinq cases, en colonne 4.
2. Une sphère de rayon 3 au centre de la case (3, 3), dont le gabarit déborde derrière le mur.
3. Un gobelin derrière le mur, un ogre de taille G à cheval sur le bord de la zone, un allié dans la zone.

**Résultat attendu**

- Vérifie que `dessin(gabarit, 7, 7)` vaut `(std::vector<std::string>{".......", ".XXXXX.", ".XXXXX.", ".XXXXX.", ".XXXXX.", ".XXXXX.", "......."})`.
- Vérifie que `dessin(atteintes, 7, 7, &grille)` vaut `(std::vector<std::string>{".......", ".XXX#..", ".XXX#..", ".XXX#..", ".XXX#..", ".XXX#..", "......."})`.
- Vérifie que `core::combatantsInArea(combat, boule)` vaut `(std::vector<core::CombatantId>{core::CombatantId{1}, core::CombatantId{3}})`.

## test_arena.cpp

### ArenaTest.LesPointsDEntreeSeLisentDeLaCarte

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_arena.cpp:164`

Les points d'entree de l'arene se lisent de la carte, ranges par camp puis par rang.

**Étapes**

1. Une piste avec trois entrees par camp, declarees dans le desordre, et une entree sans camp.
2. Lire les points d'entree.

**Résultat attendu**

- Vérifie que `entrees.size()` vaut `6U`.
- Vérifie que `entrees` vaut `attendu`.

### ArenaTest.LaPremiereCarteSeChargeEtAccueilleLesDeuxCamps

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_arena.cpp:185`

La carte que nomme la premiere arene jouable se charge et accueille les deux camps ; les arenes et les huit Marques Heroiques se chargent.

**Étapes**

1. Charger le catalogue des arenes d'essai et les Marques Heroiques.
2. Charger la carte que la premiere arene nomme.
3. Lire ses points d'entree et verifier qu'aucun n'est dans un mur.

**Résultat attendu**

- Vérifie que `arenes.errors.empty()` est vrai.
- Vérifie que `marques.errors.empty()` est vrai.
- Vérifie que `marques.marks.size()` vaut `8U`.
- Vérifie que `marques.find("tank")` diffère de `nullptr`.
- Vérifie que `marques.find("paladin")` vaut `nullptr`.
- Vérifie que `jouable` diffère de `nullptr`.
- Vérifie que `jouable->lethal` est faux.
- Vérifie que `jouable->heroicMark` est vrai.
- Vérifie que `arenes.find(jouable->id)` vaut `jouable`.
- Vérifie que `carte.ok()` est vrai.
- Vérifie que `grille.isObstructed(entree.position, core::Locomotion::Walk)` est faux.
- Vérifie que `allies` est supérieur ou égal à `4`.
- Vérifie que `ennemis` est supérieur ou égal à `4`.

### ArenaTest.LeMontagePlaceAuxEntreesEtRefuseEnLeDisant

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_arena.cpp:235`

Le montage place chaque combattant au prochain point d'entree libre de son camp, ou a la case demandee, et nomme chaque refus.

**Étapes**

1. Quatre allies sur trois entrees, un ennemi a une case demandee dans le pilier, un ennemi libre.
2. Monter avec la Marque Heroique, puis sans.

**Résultat attendu**

- Vérifie que `montage.allies` vaut `(std::vector<CombatantId>{CombatantId{1}, CombatantId{2}, CombatantId{3}})`.
- Vérifie que `montage.enemies` vaut `(std::vector<CombatantId>{CombatantId{4}})`.
- Vérifie que `montage.refusals.size()` vaut `2U`.
- Vérifie que `montage.refusals[0].who` vaut `"Allie3"`.
- Vérifie que `montage.refusals[0].placement` vaut `core::PlacementResult::OutOfBounds`.
- Vérifie que `montage.refusals[1].who` vaut `"Golem"`.
- Vérifie que `montage.refusals[1].placement` vaut `core::PlacementResult::Obstructed`.
- Vérifie que `session.combat().grid().positionOf(CombatantId{1})` vaut `(core::GridPosition{2, 3})`.
- Vérifie que `session.combat().grid().positionOf(CombatantId{2})` vaut `(core::GridPosition{2, 2})`.
- Vérifie que `session.combat().grid().positionOf(CombatantId{4})` vaut `(core::GridPosition{9, 3})`.
- Vérifie que `session.combat().economy(CombatantId{1})->has(core::HEROIC_ACTION_RESOURCE)` est vrai.
- Vérifie que `session.attacks(CombatantId{4})` diffère de `nullptr`.
- Vérifie que `session.attacks(CombatantId{9})` vaut `nullptr`.
- Vérifie que `session.combat().find(CombatantId{4})->profile.armorClass` vaut `13`.
- Vérifie que `session.combat().economy(CombatantId{1})->has(core::HEROIC_ACTION_RESOURCE)` est faux.

### ArenaTest.LAttaqueSeRefuseEtSeResout

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_arena.cpp:282`

L'action attaquer de l'arene refuse hors tour actif, contre un allie ou un inconnu, hors allonge, sans attaque ou sans action ; a portee elle jette le d20 contre la classe d'armure du profil et l'ecrit au journal.

**Étapes**

1. Heroine en (2,3), compagnon en (2,2), gobelin en (3,3) a la CA 30, un second gobelin en (8,3), graine 3.
2. Attaquer avant le debut ; le compagnon ; un inconnu ; le gobelin lointain ; avec une attaque inexistante.
3. Attaquer le gobelin voisin, puis encore.

**Résultat attendu**

- Vérifie que `session.attack(CombatantId{3}).result` vaut `core::ArenaActionResult::NoActiveTurn`.
- Vérifie que `session.start()` est vrai.
- Vérifie que `session.attack(CombatantId{2}).result` vaut `core::ArenaActionResult::InvalidTarget`.
- Vérifie que `session.attack(CombatantId{9}).result` vaut `core::ArenaActionResult::InvalidTarget`.
- Vérifie que `session.attack(CombatantId{4}).result` vaut `core::ArenaActionResult::OutOfReach`.
- Vérifie que `session.attack(CombatantId{3}, 5).result` vaut `core::ArenaActionResult::NoAttack`.
- Vérifie que `attaque.result` vaut `core::ArenaActionResult::Done`.
- Vérifie que `attaque.outcome.has_value()` est vrai.
- Vérifie que `attaque.outcome->roll.check.target` vaut `30`.
- Vérifie que `attaque.outcome->roll.check.modifiers.size()` vaut `1U`.
- Vérifie que `attaque.outcome->roll.check.modifiers[0].value` vaut `5`.
- Vérifie que `session.journal().back()` vaut `attaque.outcome->describe()`.
- Vérifie que `session.journal().back().starts_with("attaque Heroine -> Gobelin")` est vrai.
- Vérifie que `session.attack(CombatantId{3}).result` vaut `core::ArenaActionResult::NoAction`.
- Vérifie que `std::ranges::find_if( session.journal(), [](const std::string& l) { return l.starts_with("attaque declaree"); })` diffère de `session.journal().end()`.

### ArenaTest.LePilierCacheEtAbrite

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_arena.cpp:336`

Dans l'arene, un tir vers une cible cachee par le pilier est refuse ; vers une cible que le pilier abrite partiellement, il est jete contre sa CA + 2, et le journal le dit.

**Étapes**

1. Une archere (portee 16/64) en (5,3), un gobelin a la CA 12 en (7,3) derriere le pilier (6,3), un second en (7,5).
2. L'archere tire sur le premier.
3. Elle se place en (5,2), d'ou le pilier ne cache plus que le bas de la case du gobelin, et tire encore.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{1}`.
- Vérifie que `session.attack(CombatantId{2}).result` vaut `core::ArenaActionResult::TotalCover`.
- Vérifie que `session.combat().find(CombatantId{1})->economy.remaining(core::ACTION_RESOURCE)` vaut `1`.
- Vérifie que `session.move(core::GridPosition{5, 2}).result` vaut `core::MoveResult::Moved`.
- Vérifie que `tir.result` vaut `core::ArenaActionResult::Done`.
- Vérifie que `tir.outcome.has_value()` est vrai.
- Vérifie que `tir.outcome->roll.armorClass` vaut `14`.
- Vérifie que `session.journal().back().find("abri partiel : CA 12 -> 14")` diffère de `std::string::npos`.

### ArenaTest.LOpportuniteLeDesengagementEtLEsquive

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_arena.cpp:379`

Quitter l'allonge d'un ennemi provoque son attaque d'opportunite, qui depense sa reaction ; se desengager l'evite ; esquiver impose le desavantage a qui attaque.

**Étapes**

1. Heroine en (2,3) au contact d'un ogre en (3,3) (CA 1, bonus 0, 1 degat).
2. L'heroine s'eloigne en (2,5).
3. Remonter, se desengager, puis s'eloigner.
4. Remonter, esquiver, finir le tour ; l'ogre attaque l'heroine.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{1}`.
- Vérifie que `fuite.result` vaut `core::MoveResult::Moved`.
- Vérifie que `session.combat().grid().positionOf(CombatantId{1})` vaut `(core::GridPosition{2, 5})`.
- Vérifie que `opportunites(session)` vaut `1`.
- Vérifie que `session.combat().find(CombatantId{2})->economy.remaining(core::REACTION_RESOURCE)` vaut `0`.
- Vérifie que `prudente.start()` est vrai.
- Vérifie que `prudente.disengage()` est vrai.
- Vérifie que `prudente.dodge()` est faux.
- Vérifie que `prudente.move(core::GridPosition{2, 5}).result` vaut `core::MoveResult::Moved`.
- Vérifie que `opportunites(prudente)` vaut `0`.
- Vérifie que `prudente.combat().find(CombatantId{2})->economy.remaining(core::REACTION_RESOURCE)` vaut `1`.
- Vérifie que `esquive.start()` est vrai.
- Vérifie que `esquive.dodge()` est vrai.
- Vérifie que `esquive.endTurn()` est vrai.
- Vérifie que `esquive.combat().activeCombatant()` vaut `CombatantId{2}`.
- Vérifie que `riposte.outcome.has_value()` est vrai.
- Vérifie que `riposte.outcome->roll.check.stance` vaut `core::RollStance::Disadvantage`.
- Vérifie que `riposte.outcome->describe().find("esquive de la cible")` diffère de `std::string::npos`.

### ArenaTest.UnAffrontementSeJoueSeRejoueEtPersonneNYMeurt

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_arena.cpp:444`

Un affrontement se joue jusqu'a son issue ; le rejeu a la meme graine donne le meme journal ; a la fin, la Marque Heroique releve tout le monde, sauf dans une arene letale.

**Étapes**

1. Deux allies contre trois gobelins, graine 2026, jouer jusqu'a l'issue.
2. Rejouer par `replay`, puis a une autre graine.
3. Meme affrontement dans une arene letale.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.outcome().has_value()` est vrai.
- Vérifie que `session.combat().round()` est supérieur ou égal à `2`.
- Vérifie que `std::count_if(journal.begin(), journal.end(), [](const std::string& l) { return l.starts_with("issue"); })` vaut `1`.
- Vérifie que `journal.back()` vaut `"marque heroique : tous releves"`.
- Vérifie que `c->status` vaut `core::CombatantStatus::Standing`.
- Vérifie que `c->profile.currentHitPoints` vaut `c->profile.maximumHitPoints`.
- Vérifie que `remontage.allies.size()` vaut `2U`.
- Vérifie que `remontage.enemies.size()` vaut `3U`.
- Vérifie que `session.outcome().has_value()` est faux.
- Vérifie que `jouer(session)` vaut `journal`.
- Vérifie que `session.outcome()` vaut `issue`.
- Vérifie que `autre.start()` est vrai.
- Vérifie que `jouer(autre)` diffère de `journal`.
- Vérifie que `letale.start()` est vrai.
- Vérifie que `letale.outcome().has_value()` est vrai.
- Vérifie que `journalLetal.back()` diffère de `"marque heroique : tous releves"`.
- Vérifie que `std::ranges::any_of(letale.combat().combatants(), [&](CombatantId id) { return letale.combat().find(id)->status == core::CombatantStatus::Down; })` est vrai.

## test_attack.cpp

### AttackTest.UnVingtNaturelToucheEtDoubleLesDes

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:88`

Un 20 naturel touche une CA hors d'atteinte, est un critique, et double les des de degats sans doubler le modificateur.

**Étapes**

1. L'heroine (+5, 1d8+3) attaque un gobelin a la CA 40.
2. Le d20 est force a 20.

**Résultat attendu**

- Vérifie que `issue.has_value()` est vrai.
- Vérifie que `issue->roll.hit` est vrai.
- Vérifie que `issue->roll.critical` est vrai.
- Vérifie que `issue->roll.check.total` vaut `25`.
- Vérifie que `issue->damage.size()` vaut `1U`.
- Vérifie que `des.faces.size()` vaut `2U`.
- Vérifie que `des.dice.modifier` vaut `3`.
- Vérifie que `duel.combat.find(CombatantId{2})->profile.currentHitPoints` vaut `30 - perte`.
- Vérifie que `ligne.find("critique (20 naturel)")` diffère de `std::string::npos`.
- Vérifie que `ligne.find("(des doubles)")` diffère de `std::string::npos`.

### AttackTest.UnUnNaturelRateMemeAuDessusDeLaCA

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:120`

Un 1 naturel rate toujours, meme quand le total depasse la classe d'armure.

**Étapes**

1. L'heroine attaque avec +30 un gobelin a la CA 5.
2. Le d20 est force a 1.

**Résultat attendu**

- Vérifie que `issue.has_value()` est vrai.
- Vérifie que `issue->roll.check.total` vaut `31`.
- Vérifie que `issue->roll.hit` est faux.
- Vérifie que `issue->damage.empty()` est vrai.
- Vérifie que `duel.combat.find(CombatantId{2})->profile.currentHitPoints` vaut `30`.
- Vérifie que `issue->describe()` vaut `"attaque Heroine -> Gobelin (Epee longue) : d20 = 1 + 30 (benediction) = 31 contre " "CA 5 : rate (1 naturel)"`.

### AttackTest.ChaqueJetProduitUneEntreeDeJournalComplete

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:147`

Exigences : `EX-REG-003`

Une attaque touchee et une attaque ratee s'ecrivent au journal avec le de, chaque modificateur et son origine, la CA, l'issue, les des de degats, leur type et les PV.

**Étapes**

1. Le d20 force a 12, contre CA 15 : touche.
2. Le d20 force a 4 : rate.

**Résultat attendu**

- Vérifie que `touche.has_value() && touche->report.has_value()` est vrai.
- Vérifie que `touche->describe()` vaut `attendu`.
- Vérifie que `rate.has_value()` est vrai.
- Vérifie que `rate->describe()` vaut `"attaque Heroine -> Gobelin (Epee longue) : d20 = 4 + 3 (Force) + " "2 (maitrise) = 9 contre CA 15 : rate"`.

### AttackTest.LeJetSAmendeAvantQueLIssueNeSoitFigee

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:180`

Un greffon ajoute une source de desavantage avant le jet, relance un de apres le jet, ajoute un modificateur apres avoir vu le total ; l'issue tient compte des trois.

**Étapes**

1. Avant le jet : un desavantage et un seuil critique a 19.
2. Apres les des : substituer 19 au premier de et 19 au second.
3. Avant l'issue : +5 si le total rate.
4. Un attaquant avantage et desavantage a la fois.
5. L'annonce de l'attaque precede le jet.

**Résultat attendu**

- Vérifie que `jet.check.dice.size()` vaut `2U`.
- Vérifie que `issue.has_value()` est vrai.
- Vérifie que `ordre` vaut `(std::vector<std::string>{"declaree", "jet"})`.
- Vérifie que `issue->roll.check.stance` vaut `core::RollStance::Disadvantage`.
- Vérifie que `issue->roll.amendments.size()` vaut `2U`.
- Vérifie que `issue->roll.check.total` vaut `27`.
- Vérifie que `issue->roll.hit` est vrai.
- Vérifie que `issue->roll.critical` est vrai.
- Vérifie que `jet.check.stance` vaut `core::RollStance::Normal`.
- Vérifie que `jet.check.dice.size()` vaut `1U`.
- Vérifie que `jet.check.target` vaut `10`.

### AttackTest.LaGrilleDitLaPorteeEtLesCirconstances

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:242`

L'allonge se mesure entre emprises, et un tir est desavantage au contact d'un ennemi ou au-dela de sa portee normale.

**Étapes**

1. Un ogre de taille G ancre en (5,5), un archer en (7,5), un guerrier en (2,2).
2. Mesurer les distances ; l'allonge d'une attaque de 1 et de 2 cases.
3. L'archer tire au contact de l'ogre ; puis a 5 cases avec une portee 4/12.
4. Une cible a terre.

**Résultat attendu**

- Vérifie que `combat.start(hasard)` est vrai.
- Vérifie que `core::gridDistance(combat, CombatantId{1}, CombatantId{2})` vaut `1`.
- Vérifie que `core::gridDistance(combat, CombatantId{3}, CombatantId{2})` vaut `3`.
- Vérifie que `core::inReach(combat, CombatantId{2}, CombatantId{1}, massue)` est vrai.
- Vérifie que `core::inReach(combat, CombatantId{3}, CombatantId{2}, massue)` est faux.
- Vérifie que `core::inReach(combat, CombatantId{3}, CombatantId{2}, massue)` est vrai.
- Vérifie que `core::attackCircumstances(combat, CombatantId{1}, CombatantId{2}, arc).disadvantages` vaut `(std::vector<std::string>{"tir au contact d'un ennemi"})`.
- Vérifie que `core::inReach(combat, CombatantId{1}, CombatantId{4}, arc)` est faux.
- Vérifie que `core::inReach(combat, CombatantId{1}, CombatantId{4}, arc)` est vrai.
- Vérifie que `core::attackCircumstances(combat, CombatantId{1}, CombatantId{4}, arc).disadvantages` vaut `(std::vector<std::string>{"tir au contact d'un ennemi", "longue portee"})`.
- Vérifie que `core::resolveAttack(combat, CombatantId{1}, CombatantId{4}, arc, hasard).has_value()` est faux.

### AttackTest.LesProfilsSeTirentDuBestiaireEtDeLaFiche

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:289`

Les attaques d'une creature se lisent de son bloc ; celles d'un personnage de son arme, de sa Force ou de sa Dexterite et de sa maitrise ; le coup a mains nues vaut 1 + Force ; aucune creature livree n'a de degats sans type.

**Étapes**

1. Une creature a quatre actions : allonge 3 m, sans allonge, sans type, sans degats.
2. Une fiche de Force 16 et Dexterite 18, maitrise +2 : epee longue, rapiere de finesse, filet, mains nues, arme non maitrisee.
3. Charger le bestiaire livre.

**Résultat attendu**

- Vérifie que `attaques.attacks.size()` vaut `2U`.
- Vérifie que `attaques.attacks[0].reach` vaut `2`.
- Vérifie que `attaques.attacks[0].kind` vaut `core::AttackKind::Melee`.
- Vérifie que `attaques.attacks[0].modifiers[0].value` vaut `6`.
- Vérifie que `attaques.attacks[1].kind` vaut `core::AttackKind::Ranged`.
- Vérifie que `attaques.refused` vaut `(std::vector<std::string>{"Etrange : degats sans type"})`.
- Vérifie que `epeeLongue.modifiers[0].source` vaut `"Force"`.
- Vérifie que `epeeLongue.modifiers[0].value` vaut `3`.
- Vérifie que `epeeLongue.modifiers[1].value` vaut `2`.
- Vérifie que `epeeLongue.damage[0].dice` vaut `*core::parseDice("1d8+3")`.
- Vérifie que `finesse.modifiers[0].source` vaut `"Dexterite"`.
- Vérifie que `finesse.damage[0].dice` vaut `*core::parseDice("1d8+4")`.
- Vérifie que `core::weaponAttackFor(fiche, &filet, 2).damage.empty()` est vrai.
- Vérifie que `mains.damage[0].type` vaut `DamageType::Bludgeoning`.
- Vérifie que `mains.damage[0].dice.minimum()` vaut `4`.
- Vérifie que `mains.damage[0].dice.maximum()` vaut `4`.
- Vérifie que `core::weaponAttackFor(fiche, &longue, 2, false).modifiers.size()` vaut `1U`.
- Vérifie que `bestiaire.creatures.empty()` est faux.
- Vérifie que `lues.refused.empty()` est vrai.
- Vérifie que `total` est strictement supérieur à `bestiaire.creatures.size() / 2`.

### AttackTest.LAbriChangeLaCAUneFois

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:374`

Une cible derriere un muret gagne +2 a sa CA ; un greffon qui pose le meme abri ne l'ajoute pas une seconde fois ; un abri important par-dessus porte le bonus a +5, pas a +7 ; un abri total n'est pas un bonus.

**Étapes**

1. Un archer en (0,1), un gobelin a la CA 15 en (3,1), un muret en (2,1).
2. Tirer, d20 force a 12.
3. Tirer avec un greffon qui pose l'abri partiel, puis l'abri total.
4. Tirer avec un greffon qui pose l'abri important.

**Résultat attendu**

- Vérifie que `combat.grid().placeObject({2, 1}, {.kind = "muret", .hitPoints = 10, .blocksMovement = true, .cover = core::Cover::Half, .damageTraits = {}})` vaut `core::PlacementResult::Placed`.
- Vérifie que `combat.start(hasard)` est vrai.
- Vérifie que `core::coverBetween(combat, CombatantId{1}, CombatantId{2})` vaut `core::Cover::Half`.
- Vérifie que `simple.has_value()` est vrai.
- Vérifie que `simple->roll.armorClass` vaut `17`.
- Vérifie que `simple->roll.cover` vaut `core::Cover::Half`.
- Vérifie que `simple->describe().find("[abri partiel : CA 15 -> 17] = 12")` diffère de `std::string::npos`.
- Vérifie que `simple->describe().find("= 17 contre CA 17 : touche")` diffère de `std::string::npos`.
- Vérifie que `repose.has_value()` est vrai.
- Vérifie que `repose->roll.armorClass` vaut `17`.
- Vérifie que `repose->roll.amendments.size()` vaut `1U`.
- Vérifie que `important.has_value()` est vrai.
- Vérifie que `important->roll.armorClass` vaut `20`.
- Vérifie que `important->roll.cover` vaut `core::Cover::ThreeQuarters`.
- Vérifie que `important->roll.amendments.size()` vaut `2U`.
- Vérifie que `important->roll.hit` est faux.

### AttackTest.ViserDemandeLaPorteeEtLaVue

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:442`

Une cible derriere un mur ne se vise pas, ni a distance ni au contact par le coin de deux murs ; au-dela de la longue portee non plus ; un ennemi adjacent qui ne voit pas le tireur ne lui impose pas le desavantage du tir au contact.

**Étapes**

1. Un archer en (1,1), un gobelin en (2,2) derriere deux murs en (2,1) et (1,2), un loup en (0,3), un rat en (5,5).
2. Verifier chaque cible, a distance (portee 2/3) et au contact.
3. Les circonstances du tir vers le loup.

**Résultat attendu**

- Vérifie que `combat.start(hasard)` est vrai.
- Vérifie que `core::gridDistance(combat, CombatantId{1}, CombatantId{2})` vaut `1`.
- Vérifie que `core::checkTarget(combat, CombatantId{1}, CombatantId{2}, arc)` vaut `core::TargetCheck::TotalCover`.
- Vérifie que `core::checkTarget(combat, CombatantId{1}, CombatantId{2}, epee())` vaut `core::TargetCheck::TotalCover`.
- Vérifie que `core::checkTarget(combat, CombatantId{1}, CombatantId{3}, arc)` vaut `core::TargetCheck::Valid`.
- Vérifie que `core::checkTarget(combat, CombatantId{1}, CombatantId{4}, arc)` vaut `core::TargetCheck::OutOfReach`.
- Vérifie que `core::checkTarget(combat, CombatantId{1}, CombatantId{1}, arc)` vaut `core::TargetCheck::NotOnGrid`.
- Vérifie que `core::attackCircumstances(combat, CombatantId{1}, CombatantId{3}, arc) .disadvantages.empty()` est vrai.

### AttackTest.LesPorteesSeLisentDansLaDonnee

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_attack.cpp:486`

Chaque arme qui se tire ou se lance porte ses portees ; l'arc long tire a 30/120 cases, la hallebarde frappe a 2, la dague se lance a 4/12 ; le squelette tire a 16/64.

**Étapes**

1. Charger le catalogue d'armes et le bestiaire livres.
2. Pour chaque arme : portees presentes si et seulement si elle a les munitions ou le lancer.
3. Tirer les profils de l'arc long, de la hallebarde, de la dague (lancee), de l'epee longue (qui ne se lance pas) et du squelette.

**Résultat attendu**

- Vérifie que `catalogue.errors.empty()` est vrai.
- Vérifie que `catalogue.weapons.size()` vaut `37U`.
- Vérifie que `arme.rangeNormal.has_value()` vaut `tiree`.
- Vérifie que `arme.rangeLong.has_value()` vaut `tiree`.
- Vérifie que `tiree` est vrai.
- Vérifie que `arcLong.range.has_value()` est vrai.
- Vérifie que `arcLong.range->normal` vaut `30`.
- Vérifie que `arcLong.range->maximum` vaut `120`.
- Vérifie que `core::weaponAttackFor(fiche, catalogue.findWeapon("hallebarde"), 2).reach` vaut `2`.
- Vérifie que `core::weaponAttackFor(fiche, catalogue.findWeapon("epee-longue"), 2).reach` vaut `1`.
- Vérifie que `dague.has_value() && dague->range.has_value()` est vrai.
- Vérifie que `dague->label` vaut `"Dague (lancer)"`.
- Vérifie que `dague->kind` vaut `core::AttackKind::Ranged`.
- Vérifie que `dague->range->normal` vaut `4`.
- Vérifie que `dague->range->maximum` vaut `12`.
- Vérifie que `dague->modifiers[0].source` vaut `"Dexterite"`.
- Vérifie que `core::thrownAttackFor(fiche, *catalogue.findWeapon("epee-longue"), 2).has_value()` est faux.
- Vérifie que `squelette` diffère de `nullptr`.
- Vérifie que `attaques.attacks.size()` vaut `2U`.
- Vérifie que `attaques.attacks[1].kind` vaut `core::AttackKind::Ranged`.
- Vérifie que `attaques.attacks[1].range.has_value()` est vrai.
- Vérifie que `attaques.attacks[1].range->normal` vaut `16`.
- Vérifie que `attaques.attacks[1].range->maximum` vaut `64`.

## test_battle_grid.cpp

### BattleGridTest.LesObstaclesSontCeuxDeLaCollision

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_battle_grid.cpp:74`

Les obstacles de la grille de combat sont ceux de la couche collision.

**Étapes**

1. Construire une grille sur une collision portant un mur, de l'eau peu profonde et de l'eau profonde.
2. Interroger chaque case, au sol puis en vol.

**Résultat attendu**

- Vérifie que `grille.isObstructed({0, 0})` est vrai.
- Vérifie que `grille.isObstructed({0, 0}, core::Locomotion::Fly)` est vrai.
- Vérifie que `grille.isObstructed({1, 0})` est faux.
- Vérifie que `grille.isObstructed({2, 0})` est vrai.
- Vérifie que `grille.isObstructed({2, 0}, core::Locomotion::Fly)` est faux.
- Vérifie que `grille.isObstructed({3, 0})` est faux.
- Vérifie que `grille.isObstructed({4, 0})` est vrai.
- Vérifie que `grille.isObstructed({-1, 0})` est vrai.

### BattleGridTest.DeuxCreaturesNePartagentJamaisUneCase

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_battle_grid.cpp:104`

Deux creatures ne partagent jamais une case.

**Étapes**

1. Placer un combattant.
2. Tenter d'en placer un second sur sa case, puis d'y deplacer un troisieme.
3. Tenter de poser une creature 2x2 dont l'emprise mord sur la case.

**Résultat attendu**

- Vérifie que `grille.place(HEROS, {3, 3})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.place(RAT, {3, 3})` vaut `core::PlacementResult::Occupied`.
- Vérifie que `grille.place(RAT, {5, 5})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.moveTo(RAT, {3, 3})` vaut `core::PlacementResult::Occupied`.
- Vérifie que `grille.place(OURS, {2, 2}, 2)` vaut `core::PlacementResult::Occupied`.
- Vérifie que `grille.occupantAt({3, 3})` vaut `HEROS`.
- Vérifie que `grille.positionOf(RAT)` vaut `(core::GridPosition{5, 5})`.
- Vérifie que `grille.positionOf(OURS).has_value()` est faux.

### BattleGridTest.UneGrandeCreatureOccupeToutSonEmprise

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_battle_grid.cpp:131`

Une creature 2x2 occupe quatre cases et avance sur sa propre emprise.

**Étapes**

1. Placer une creature de taille G (2x2).
2. La deplacer d'une case vers la droite.

**Résultat attendu**

- Vérifie que `core::footprintSide(core::CreatureSize::Tiny)` vaut `1`.
- Vérifie que `core::footprintSide(core::CreatureSize::Large)` vaut `2`.
- Vérifie que `core::footprintSide(core::CreatureSize::Gargantuan)` vaut `4`.
- Vérifie que `grille.place(OURS, {1, 1}, core::footprintSide(core::CreatureSize::Large))` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.occupantAt(case_)` vaut `OURS`.
- Vérifie que `grille.sideOf(OURS)` vaut `2`.
- Vérifie que `grille.moveTo(OURS, {2, 1})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.occupantAt({1, 1}).has_value()` est faux.
- Vérifie que `grille.occupantAt({3, 2})` vaut `OURS`.

### BattleGridTest.UnPlacementImpossibleEstRefuseAvecSaRaison

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_battle_grid.cpp:163`

Un placement impossible est refuse avec sa raison.

**Étapes**

1. Placer hors de la carte, dans un mur, a cheval sur le bord.
2. Placer deux fois le meme combattant, deplacer un combattant absent.
3. Retirer un combattant et reprendre sa case.

**Résultat attendu**

- Vérifie que `grille.place(HEROS, {4, 0})` vaut `core::PlacementResult::OutOfBounds`.
- Vérifie que `grille.place(HEROS, {1, 1})` vaut `core::PlacementResult::Obstructed`.
- Vérifie que `grille.place(OURS, {3, 3}, 2)` vaut `core::PlacementResult::OutOfBounds`.
- Vérifie que `grille.place(HEROS, {0, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.place(HEROS, {2, 2})` vaut `core::PlacementResult::InvalidCombatant`.
- Vérifie que `grille.moveTo(RAT, {2, 2})` vaut `core::PlacementResult::InvalidCombatant`.
- Vérifie que `grille.place(RAT, {2, 2}, 0)` vaut `core::PlacementResult::InvalidCombatant`.
- Vérifie que `grille.remove(HEROS)` est vrai.
- Vérifie que `grille.remove(HEROS)` est faux.
- Vérifie que `grille.place(RAT, {0, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.combatants()` vaut `(std::vector<core::CombatantId>{RAT})`.

### BattleGridTest.LesProprietesDeZoneSontRelevees

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_battle_grid.cpp:194`

Les proprietes de zone de la carte sont relevees par la grille.

**Étapes**

1. Construire la grille d'une carte dont une couche porte difficultTerrain, une autre une regle inconnue, une troisieme une coquille (difficultTerrain: 1).
2. Interroger les cases.

**Résultat attendu**

- Vérifie que `grille.isObstructed({5, 0})` est vrai.
- Vérifie que `grille.isDifficult({1, 1})` est vrai.
- Vérifie que `grille.isDifficult({2, 1})` est vrai.
- Vérifie que `grille.isDifficult({3, 1})` est faux.
- Vérifie que `grille.isDifficult({0, 3})` est faux.
- Vérifie que `zones.size()` vaut `2U`.
- Vérifie que `zones[0]->contains("difficultTerrain")` est vrai.
- Vérifie que `std::get<bool>(zones[1]->at("noHealing"))` est vrai.
- Vérifie que `grille.zonesAt({4, 3}).empty()` est vrai.
- Vérifie que `grille.zonesAt({-1, 0}).empty()` est vrai.

### BattleGridTest.LeTerrainDifficileSeCreeEnCombat

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_battle_grid.cpp:224`

Le terrain difficile se cree et s'efface en combat.

**Étapes**

1. Rendre une case difficile (un seisme).
2. L'effacer.
3. Viser une case hors de la carte.

**Résultat attendu**

- Vérifie que `grille.isDifficult({1, 1})` est vrai.
- Vérifie que `grille.isDifficult({1, 1})` est faux.
- Vérifie que `grille.isDifficult({9, 9})` est faux.

### BattleGridTest.UnObjetDeGrilleBloqueEtSeDetruit

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_battle_grid.cpp:246`

Un objet de grille bloque tant qu'il tient, puis se detruit.

**Étapes**

1. Poser une barricade de 10 PV et une toile non bloquante.
2. Infliger 4 puis 6 degats a la barricade.
3. Placer un combattant sur la toile, puis tenter une barricade sur lui.

**Résultat attendu**

- Vérifie que `grille.placeObject({1, 1}, {.kind = "barricade", .hitPoints = 10})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.placeObject({2, 2}, {.kind = "web", .hitPoints = 10, .blocksMovement = false})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.placeObject({1, 1}, {.kind = "tonneau"})` vaut `core::PlacementResult::Occupied`.
- Vérifie que `grille.isObstructed({1, 1})` est vrai.
- Vérifie que `grille.place(HEROS, {1, 1})` vaut `core::PlacementResult::Obstructed`.
- Vérifie que `grille.damageObject({1, 1}, 4)` est faux.
- Vérifie que `grille.objectAt({1, 1})` diffère de `nullptr`.
- Vérifie que `grille.objectAt({1, 1})->hitPoints` vaut `6`.
- Vérifie que `grille.damageObject({1, 1}, 6)` est vrai.
- Vérifie que `grille.objectAt({1, 1})` vaut `nullptr`.
- Vérifie que `grille.isObstructed({1, 1})` est faux.
- Vérifie que `grille.isObstructed({2, 2})` est faux.
- Vérifie que `grille.place(HEROS, {3, 3})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.placeObject({3, 3}, {.kind = "barricade"})` vaut `core::PlacementResult::Occupied`.

## test_combat_preview.cpp

### CombatPreviewTest.LaPrevisualisationEstLeJet

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_preview.cpp:84`

Ce que l'ecran montre avant l'attaque -- CA abri compris, posture, sources d'avantage et de desavantage, chance de toucher -- est exactement ce que le jet jette.

**Étapes**

1. Une attaque au contact d'un ennemi pris en tenaille, dans une arene a tenaille.
2. Un tir a longue portee, a travers un allie, sur une cible qui esquive.
3. Une cible hors de portee.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{1}`.
- Vérifie que `apercu.has_value()` est vrai.
- Vérifie que `apercu->stance` vaut `core::RollStance::Advantage`.
- Vérifie que `apercu->advantages` vaut `(std::vector<std::string>{"prise en tenaille"})`.
- Vérifie que `apercu->requiredRoll` vaut `8`.
- Vérifie que `apercu->hitPercent()` vaut `88`.
- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{3}`.
- Vérifie que `session.dodge()` est vrai.
- Vérifie que `session.endTurn()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{1}`.
- Vérifie que `apercu.has_value()` est vrai.
- Vérifie que `apercu->cover` vaut `core::Cover::Half`.
- Vérifie que `apercu->armorClass` vaut `14`.
- Vérifie que `apercu->stance` vaut `core::RollStance::Disadvantage`.
- Vérifie que `apercu->disadvantages.size()` vaut `2U`.
- Vérifie que `session.start()` est vrai.
- Vérifie que `apercu.has_value()` est vrai.
- Vérifie que `apercu->check` vaut `core::TargetCheck::OutOfReach`.
- Vérifie que `apercu->hitChance` vaut `0`.
- Vérifie que `core::firstValidAttack(session, CombatantId{2}).has_value()` est faux.
- Vérifie que `core::previewAttack(session, CombatantId{2}, 3).has_value()` est faux.

### CombatPreviewTest.LeDeplacementSePrevisualiseEtLOpportuniteSeDecline

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_preview.cpp:156`

La previsualisation d'un deplacement donne le chemin, le deplacement restant et qui frappera en chemin ; un combattant dont le joueur laisse passer les opportunites ne frappe pas.

**Étapes**

1. Une heroine au contact d'un ogre ; previsualiser un pas qui sort de son allonge.
2. Le joueur dit que l'ogre laisse passer ; previsualiser, puis jouer le pas.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{1}`.
- Vérifie que `pas.path.has_value()` est vrai.
- Vérifie que `pas.path->cost` vaut `2`.
- Vérifie que `pas.movementLeft` vaut `4`.
- Vérifie que `pas.opportunities` vaut `(std::vector<CombatantId>{CombatantId{2}})`.
- Vérifie que `core::previewMove(session, {9, 9}).path.has_value()` est faux.
- Vérifie que `session.takesOpportunities(CombatantId{2})` est vrai.
- Vérifie que `core::previewMove(session, {2, 5}).opportunities.empty()` est vrai.
- Vérifie que `session.move({2, 5}).result` vaut `core::MoveResult::Moved`.
- Vérifie que `std::ranges::none_of( session.journal(), [](const std::string& l) { return l.starts_with("opportunite : "); })` est vrai.
- Vérifie que `session.combat().find(CombatantId{2})->economy.remaining(core::REACTION_RESOURCE)` vaut `1`.
- Vérifie que `session.takesOpportunities(CombatantId{2})` est faux.

## test_combat_state.cpp

### CombatStateTest.LInitiativeEstJeteeUneFoisEtLOrdreTient

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:225`

L'ordre d'initiative est jete au debut et reste stable de round en round.

**Étapes**

1. Engager trois combattants, graine 7.
2. Jouer trois rounds en terminant chaque tour.
3. Rejouer le meme combat a la meme graine.

**Résultat attendu**

- Vérifie que `combat.start(hasard)` est vrai.
- Vérifie que `c->initiativeRoll.has_value()` est vrai.
- Vérifie que `c->initiativeRoll->total` vaut `c->initiativeRoll->keptDie + place.modifier`.
- Vérifie que `place.total` vaut `c->initiativeRoll->total`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.round()` vaut `4`.
- Vérifie que `combat.turnOrder().entries().size()` vaut `initial.size()`.
- Vérifie que `combat.turnOrder().entries()[rang].combatant` vaut `initial[rang].combatant`.
- Vérifie que `combat.turnOrder().entries()[rang].total` vaut `initial[rang].total`.
- Vérifie que `tours[rang]` vaut `initial[rang].combatant`.
- Vérifie que `tours[rang + 3]` vaut `initial[rang].combatant`.
- Vérifie que `tours[rang + 6]` vaut `initial[rang].combatant`.
- Vérifie que `des` vaut `desRejeu`.
- Vérifie que `tours` vaut `toursRejeu`.

### CombatStateTest.LeTourNeFinitQueSurDemande

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:276`

La fin d'un tour est explicite : epuiser ses ressources ne la declenche pas.

**Étapes**

1. Heroine puis loup.
2. Depenser action, action bonus, reaction et les six cases de l'heroine.
3. Terminer le tour, puis celui du loup.
4. Tenter de terminer un tour depuis un abonne au debut de tour.

**Résultat attendu**

- Vérifie que `combat.endTurn()` est faux.
- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.start(des)` est faux.
- Vérifie que `combat.activeCombatant()` vaut `heroine`.
- Vérifie que `combat.spend(core::ACTION_RESOURCE)` est vrai.
- Vérifie que `combat.spend(core::BONUS_ACTION_RESOURCE)` est vrai.
- Vérifie que `combat.spend(core::REACTION_RESOURCE)` est vrai.
- Vérifie que `combat.spend(core::MOVEMENT_RESOURCE, 6)` est vrai.
- Vérifie que `combat.spend(core::ACTION_RESOURCE)` est faux.
- Vérifie que `combat.phase()` vaut `core::CombatPhase::TurnActive`.
- Vérifie que `combat.activeCombatant()` vaut `heroine`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `loup`.
- Vérifie que `combat.round()` vaut `1`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `heroine`.
- Vérifie que `combat.round()` vaut `2`.
- Vérifie que `combat.economy(heroine)->remaining(core::ACTION_RESOURCE)` vaut `1`.
- Vérifie que `combat.economy(heroine)->remaining(core::MOVEMENT_RESOURCE)` vaut `6`.
- Vérifie que `finsDepuisUnAbonne` vaut `(std::vector<bool>{false, false, false})`.

### CombatStateTest.LaReactionRevientAuDebutDuTourDeSonPorteur

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:322`

Une reaction depensee hors de son tour ne revient qu'au debut du tour de son porteur.

**Étapes**

1. Ordre : heroine, loup, compagnon.
2. Pendant le tour de l'heroine, le compagnon depense sa reaction.
3. Terminer le tour de l'heroine, puis celui du loup.

**Résultat attendu**

- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.economy(compagnon)->spend(core::REACTION_RESOURCE)` est vrai.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `loup`.
- Vérifie que `combat.economy(compagnon)->remaining(core::REACTION_RESOURCE)` vaut `0`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `compagnon`.
- Vérifie que `combat.economy(compagnon)->remaining(core::REACTION_RESOURCE)` vaut `1`.

### CombatStateTest.UneVictoireQuandPlusAucunEnnemiNEstDebout

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:352`

Un combat se termine par une victoire quand plus aucun ennemi n'est debout.

**Étapes**

1. Deux allies contre deux gobelins.
2. Abattre un gobelin, puis faire fuir l'autre... non : abattre le second.
3. Tenter de jouer apres la fin.

**Résultat attendu**

- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.find(premier)->status` vaut `core::CombatantStatus::Down`.
- Vérifie que `combat.phase()` diffère de `core::CombatPhase::Ended`.
- Vérifie que `combat.find(second)->profile.currentHitPoints` vaut `0`.
- Vérifie que `combat.phase()` vaut `core::CombatPhase::Ended`.
- Vérifie que `combat.outcome()` vaut `core::CombatOutcome::Victory`.
- Vérifie que `combat.activeCombatant().has_value()` est faux.
- Vérifie que `combat.endTurn()` est faux.
- Vérifie que `std::count(journal.begin(), journal.end(), "issue")` vaut `1`.

### CombatStateTest.UneDefaiteQuandTousLesAlliesSontATerre

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:388`

Un combat se termine par une defaite quand tous les allies sont a terre ; si les deux camps tombent ensemble, c'est une defaite.

**Étapes**

1. Deux allies contre un ogre ; abattre un allie, puis l'autre.
2. Second combat : une meme salve abat le premier allie, l'ogre et le second allie.

**Résultat attendu**

- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.outcome().has_value()` est faux.
- Vérifie que `combat.outcome()` vaut `core::CombatOutcome::Defeat`.
- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.outcome()` vaut `core::CombatOutcome::Defeat`.

### CombatStateTest.UneFuiteQuandLesAlliesQuittentLaZone

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:430`

Un combat se termine par une fuite quand plus aucun allie n'est debout et que l'un d'eux est parti.

**Étapes**

1. Rencontre dont on ne fuit pas : l'heroine tente de sortir.
2. La rendre fuyable ; l'heroine sort pendant son tour ; un gobelin sort aussi.
3. Le compagnon tombe.

**Résultat attendu**

- Vérifie que `combat.withdraw(heroine)` vaut `core::WithdrawResult::NotInCombat`.
- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `heroine`.
- Vérifie que `combat.withdraw(heroine)` vaut `core::WithdrawResult::NotEscapable`.
- Vérifie que `combat.withdraw(heroine)` vaut `core::WithdrawResult::Withdrawn`.
- Vérifie que `journal` vaut `(std::vector<std::string>{"sortie 1", "fin 1", "debut 2"})`.
- Vérifie que `combat.grid().positionOf(heroine).has_value()` est faux.
- Vérifie que `combat.turnOrder().contains(heroine)` est faux.
- Vérifie que `combat.withdraw(heroine)` vaut `core::WithdrawResult::NotInCombat`.
- Vérifie que `combat.withdraw(gobelin)` vaut `core::WithdrawResult::Withdrawn`.
- Vérifie que `combat.outcome().has_value()` est faux.
- Vérifie que `combat.activeCombatant()` vaut `compagnon`.
- Vérifie que `combat.outcome()` vaut `core::CombatOutcome::Flight`.

### CombatStateTest.QuatreAlliesSansHerosUnique

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:479`

Un combat a quatre allies se deroule sans qu'aucun ne soit traite a part.

**Étapes**

1. Quatre allies contre deux ennemis, graine 11.
2. Jouer trois rounds.
3. Abattre trois allies, jouer un round, puis abattre les ennemis.

**Résultat attendu**

- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.turnOrder().entries().size()` vaut `6U`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `tours.size()` vaut `6U`.
- Vérifie que `nombre` vaut `3`.
- Vérifie que `combat.outcome().has_value()` est faux.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `joueurs` vaut `(std::vector<CombatantId>{allies[3], loup, ours})`.
- Vérifie que `combat.outcome()` vaut `core::CombatOutcome::Victory`.

### CombatStateTest.UnCombatACinqSeDerouleSansFenetreEtSeRejoue

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:530`

Une escarmouche a cinq combattants va du premier round a une fin, et le rejeu a graine fixe est exact.

**Étapes**

1. Deux allies contre trois gobelins, sur une carte a pilier.
2. Chaque combattant marche vers l'ennemi le plus proche, frappe au contact, termine son tour.
3. Rejouer a la meme graine, puis a une autre.

**Résultat attendu**

- Vérifie que `issue.has_value()` est vrai.
- Vérifie que `rounds` est supérieur ou égal à `2`.
- Vérifie que `std::find(journal.begin(), journal.end(), debut)` diffère de `journal.end()`.
- Vérifie que `std::count(journal.begin(), journal.end(), "issue")` vaut `1`.
- Vérifie que `escarmouche(2026, issueRejeu, roundsRejeu)` vaut `journal`.
- Vérifie que `issueRejeu` vaut `issue`.
- Vérifie que `roundsRejeu` vaut `rounds`.
- Vérifie que `escarmouche(7, autreIssue, autresRounds)` diffère de `journal`.

### CombatStateTest.UnRenfortEntreEnCoursDeCombat

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:565`

Un renfort appele au rang 0 joue au round suivant ; un combattant qui entre apres la place en cours joue ce round-ci.

**Étapes**

1. Heroine (tres haute initiative), repere « renforts » au rang 0, chef (tres basse).
2. Au repere du round 1, faire entrer un gobelin a l'initiative 0.
3. Au round 2, pendant le tour de l'heroine, faire entrer un loup d'initiative plus basse que tout, puis un combattant sur une case prise.

**Résultat attendu**

- Vérifie que `combat.addInitiativeMarker({.count = 0, .name = "renforts"})` est vrai.
- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.round()` vaut `2`.
- Vérifie que `loup.combatant` vaut `CombatantId{4}`.
- Vérifie que `refuse.combatant.has_value()` est faux.
- Vérifie que `refuse.placement` vaut `core::PlacementResult::Occupied`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `journal` vaut `attendu`.
- Vérifie que `combat.combatants().size()` vaut `4U`.

### CombatStateTest.LesCrochetsSAnnoncentDansLOrdre

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:621`

Les crochets du combat s'annoncent dans un ordre fixe, repere fixe et acteur flottant compris.

**Étapes**

1. Heroine (haute initiative), acteur flottant, gobelin (basse), repere « repaire » au rang 20.
2. Round 1 : l'acteur flottant demande a jouer pendant le tour de l'heroine ; une attaque est declaree.
3. Round 2 : il ne demande rien.

**Résultat attendu**

- Vérifie que `combat.addInitiativeMarker({.count = 20, .name = "repaire"})` est vrai.
- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `demandeAvantLePremierTour` vaut `false`.
- Vérifie que `combat.turnOrder().contains(flottant)` est faux.
- Vérifie que `combat.find(flottant)->initiativeRoll.has_value()` est faux.
- Vérifie que `combat.interject(heroine)` est faux.
- Vérifie que `combat.interject(flottant)` est vrai.
- Vérifie que `combat.interject(flottant)` est faux.
- Vérifie que `combat.declareAttack(heroine, gobelin)` est vrai.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `journal` vaut `attendu`.

### CombatStateTest.UnCombattantATerrePasseSonTour

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:671`

Les tours d'un combattant a terre sont passes ; tomber pendant son tour le termine.

**Étapes**

1. Ordre : heroine, loup, compagnon, ours.
2. Abattre le compagnon pendant le tour de l'heroine, puis terminer les tours.
3. Le relever ; au round 2, abattre le loup pendant son propre tour.

**Résultat attendu**

- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `loup`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `ours`.
- Vérifie que `combat.find(compagnon)->status` vaut `core::CombatantStatus::Standing`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `heroine`.
- Vérifie que `combat.endTurn()` est vrai.
- Vérifie que `combat.activeCombatant()` vaut `loup`.
- Vérifie que `journal` vaut `(std::vector<std::string>{"fin 2", "debut 3"})`.
- Vérifie que `combat.activeCombatant()` vaut `compagnon`.

### CombatStateTest.LeDeplacementPaieLeCheminEtLesCampsDisentQuiSeTraverse

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:714`

Le deplacement se paie sur le budget restant ; on traverse un allie, un ennemi tres petit, jamais un ennemi de taille voisine.

**Étapes**

1. Couloir 7x1 : heroine (6 cases) en 0, compagnon en 1, rat tres petit ennemi en 3.
2. Aller en 2, puis en 4.
3. Meme couloir avec un gobelin de taille M a la place du rat.

**Résultat attendu**

- Vérifie que `combat->move({2, 0}).result` vaut `core::MoveResult::NoActiveTurn`.
- Vérifie que `combat->start(des)` est vrai.
- Vérifie que `combat->reachableArea()->destinations()` vaut `(std::vector<core::GridPosition>{{2, 0}, {4, 0}})`.
- Vérifie que `combat->move({5, 0}).result` vaut `core::MoveResult::Unreachable`.
- Vérifie que `premier.result` vaut `core::MoveResult::Moved`.
- Vérifie que `premier.path.cost` vaut `3`.
- Vérifie que `premier.path.steps` vaut `(std::vector<core::GridPosition>{{1, 0}, {2, 0}})`.
- Vérifie que `combat->economy(CombatantId{1})->remaining(core::MOVEMENT_RESOURCE)` vaut `3`.
- Vérifie que `second.result` vaut `core::MoveResult::Moved`.
- Vérifie que `second.path.cost` vaut `3`.
- Vérifie que `combat->grid().positionOf(CombatantId{1})` vaut `(core::GridPosition{4, 0})`.
- Vérifie que `combat->economy(CombatantId{1})->remaining(core::MOVEMENT_RESOURCE)` vaut `0`.
- Vérifie que `combat->reachableArea()->destinations().empty()` est vrai.
- Vérifie que `combat->start(des)` est vrai.
- Vérifie que `combat->reachableArea()->destinations()` vaut `(std::vector<core::GridPosition>{{2, 0}})`.

### CombatStateTest.LeMontageDUneRencontrePlaceEtRefuseEnLeDisant

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_combat_state.cpp:771`

Le montage pose le groupe puis les creatures de la rencontre, et nomme chaque refus avec sa raison.

**Étapes**

1. Carte a un mur ; rencontre de cinq entrees : un gobelin libre, un dans le mur, une creature inconnue, une chauve-souris, un gobelin sur la case de l'heroine.
2. Monter la rencontre avec l'heroine au declencheur.
3. Gagner le combat et le terminer.

**Résultat attendu**

- Vérifie que `montage.allies` vaut `(std::vector<CombatantId>{CombatantId{1}})`.
- Vérifie que `montage.enemies` vaut `(std::vector<CombatantId>{CombatantId{2}, CombatantId{3}})`.
- Vérifie que `montage.refusals.size()` vaut `3U`.
- Vérifie que `montage.refusals[0].who` vaut `"gobelin"`.
- Vérifie que `montage.refusals[0].position` vaut `(core::GridPosition{6, 3})`.
- Vérifie que `montage.refusals[0].placement` vaut `core::PlacementResult::Obstructed`.
- Vérifie que `montage.refusals[1].who` vaut `"fantome"`.
- Vérifie que `montage.refusals[1].placement.has_value()` est faux.
- Vérifie que `montage.refusals[2].placement` vaut `core::PlacementResult::Occupied`.
- Vérifie que `combat.find(CombatantId{1})->profile.initiativeModifier` vaut `3`.
- Vérifie que `combat.find(CombatantId{1})->profile.movement` vaut `6`.
- Vérifie que `combat.grid().positionOf(CombatantId{2})` vaut `(core::GridPosition{5, 3})`.
- Vérifie que `combat.find(CombatantId{3})->profile.locomotion` vaut `core::Locomotion::Fly`.
- Vérifie que `combat.find(CombatantId{3})->profile.movement` vaut `6`.
- Vérifie que `combat.escapable()` est faux.
- Vérifie que `combat.start(des)` est vrai.
- Vérifie que `combat.withdraw(CombatantId{1})` vaut `core::WithdrawResult::NotEscapable`.
- Vérifie que `combat.outcome()` vaut `core::CombatOutcome::Victory`.
- Vérifie que `drapeaux.isSet("carte/embuscade/3/3")` est vrai.

## test_damage.cpp

### DamageTest.LeCritiqueDoubleLesDesPasLeModificateur

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_damage.cpp:72`

Exigences : `EX-CBT-031`

Un coup critique lance deux fois les des de degats et ajoute le modificateur une seule fois.

**Étapes**

1. Lancer 1d6+3 en critique, a deux cents graines.
2. Lancer 1d4-5 sans critique.

**Résultat attendu**

- Vérifie que `lance.size()` vaut `1U`.
- Vérifie que `lance[0].roll.faces.size()` vaut `2U`.
- Vérifie que `lance[0].roll.dice.count` vaut `2`.
- Vérifie que `lance[0].roll.dice.modifier` vaut `3`.
- Vérifie que `lance[0].amount` vaut `lance[0].roll.faces[0] + lance[0].roll.faces[1] + 3`.
- Vérifie que `lance[0].critical` est vrai.
- Vérifie que `minimum` vaut `5`.
- Vérifie que `maximum` vaut `15`.
- Vérifie que `core::rollDamage(faible, false, hasard)[0].amount` vaut `0`.

### DamageTest.LesResistancesSAppliquentDansLOrdreDuManuel

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_damage.cpp:110`

Immunite, resistance et vulnerabilite s'appliquent dans l'ordre du Manuel, apres les autres modificateurs, et une seule fois chacune.

**Étapes**

1. 25 degats contondants sur une cible resistante, sous une aura qui retire 5.
2. 25 sur une cible resistante et vulnerable.
3. 25 sur une cible deux fois resistante ; sur une cible immunisee et vulnerable ; une resistance contournee par une source magique ; une source qui ignore les resistances.

**Résultat attendu**

- Vérifie que `exemple.work.hitPointLoss` vaut `10`.
- Vérifie que `exemple.work.trace.size()` vaut `2U`.
- Vérifie que `exemple.work.trace[0].source` vaut `"aura"`.
- Vérifie que `exemple.work.trace[1].source` vaut `"resistance (contondant)"`.
- Vérifie que `exemple.work.trace[1].before` vaut `20`.
- Vérifie que `exemple.work.trace[1].after` vaut `10`.
- Vérifie que `encaisser({{resiste, vulnerable}}, 0, false).work.hitPointLoss` vaut `24`.
- Vérifie que `encaisser({{resiste, resiste}}, 0, false).work.hitPointLoss` vaut `12`.
- Vérifie que `encaisser({{immunise, vulnerable}}, 0, false).work.hitPointLoss` vaut `0`.
- Vérifie que `encaisser({{nonMagique}}, 0, false).work.hitPointLoss` vaut `12`.
- Vérifie que `encaisser({{nonMagique}}, core::flagsOf(DamageFlag::Magical), false).work.hitPointLoss` vaut `25`.
- Vérifie que `encaisser({{resiste}}, core::flagsOf(DamageFlag::IgnoresResistance), false) .work.hitPointLoss` vaut `25`.

### DamageTest.LesEtapesSEnchainentEtLaConversionPrecedeLaResistance

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_damage.cpp:167`

Les cinq etapes du pipeline s'executent dans l'ordre nomme, et un type converti est resiste selon son nouveau type.

**Étapes**

1. Un greffon par etape, qui note son passage.
2. Un greffon de conversion qui change le feu en froid.
3. 20 degats de feu sur une cible resistante au froid.

**Résultat attendu**

- Vérifie que `etat` diffère de `nullptr`.
- Vérifie que `passages` vaut `(std::vector<std::string>{"source", "conversion", "resistances", "reserves", "points de vie"})`.
- Vérifie que `rapport.work.hitPointLoss` vaut `10`.
- Vérifie que `combat.find(CombatantId{1})->profile.currentHitPoints` vaut `40`.

### DamageTest.LesPointsDeVieTemporairesAbsorbentDAbordEtNeSeCumulentPas

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_damage.cpp:205`

Les points de vie temporaires se perdent avant les points de vie, ne se cumulent pas, ne se soignent pas, et absorbent encore a 0 PV sans relever.

**Étapes**

1. Donner 10 PV temporaires, puis 12, puis 5 ; une reserve cumulable de 3.
2. Infliger 7, puis 12.
3. Soigner ; infliger des degats qui ignorent les reserves.
4. A 0 PV, donner des PV temporaires et infliger 4.

**Résultat attendu**

- Vérifie que `combat.grantReserve(id, {.source = temporaires, .amount = 10, .stacks = false})` est vrai.
- Vérifie que `combat.grantReserve(id, {.source = temporaires, .amount = 12, .stacks = false})` est vrai.
- Vérifie que `combat.grantReserve(id, {.source = temporaires, .amount = 5, .stacks = false})` est faux.
- Vérifie que `combat.grantReserve(id, {.source = "Exosquelette", .amount = 3, .stacks = true})` est vrai.
- Vérifie que `combat.reserves(id)->size()` vaut `2U`.
- Vérifie que `combat.reserves(id)->front().amount` vaut `12`.
- Vérifie que `rapport.work.absorbed` vaut `7`.
- Vérifie que `rapport.work.hitPointLoss` vaut `0`.
- Vérifie que `combat.find(id)->profile.currentHitPoints` vaut `20`.
- Vérifie que `combat.reserves(id)->size()` vaut `1U`.
- Vérifie que `combat.reserves(id)->front().amount` vaut `8`.
- Vérifie que `rapport.work.absorbed` vaut `8`.
- Vérifie que `rapport.work.hitPointLoss` vaut `4`.
- Vérifie que `combat.reserves(id)->empty()` est vrai.
- Vérifie que `combat.find(id)->profile.currentHitPoints` vaut `16`.
- Vérifie que `combat.reserves(id)->empty()` est vrai.
- Vérifie que `combat.find(id)->profile.currentHitPoints` vaut `20`.
- Vérifie que `rapport.work.absorbed` vaut `0`.
- Vérifie que `combat.find(id)->status` vaut `core::CombatantStatus::Down`.
- Vérifie que `combat.reserves(id)->front().amount` vaut `50`.
- Vérifie que `rapport.work.absorbed` vaut `4`.
- Vérifie que `combat.find(id)->status` vaut `core::CombatantStatus::Down`.

### DamageTest.LesPointsDeVieSontBornesAZeroEtLExcedentEstRapporte

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_damage.cpp:264`

Les points de vie sont bornes a 0 aux trois bornes, la chute s'annonce une seule fois, et les degats restants au-dela de 0 sont rapportes pour l'agonie.

**Étapes**

1. Une cible a 10 PV : 9 degats, puis 1, puis 5.
2. Le clerc du Manuel, 6 PV sur 12, subit 18 degats d'un coup critique.
3. Un seuil « sous la moitie ».

**Résultat attendu**

- Vérifie que `combat.find(cible)->profile.currentHitPoints` vaut `1`.
- Vérifie que `combat.find(cible)->status` vaut `core::CombatantStatus::Standing`.
- Vérifie que `chutes.empty()` est vrai.
- Vérifie que `moitie` vaut `1`.
- Vérifie que `juste.hitPointsAfter` vaut `0`.
- Vérifie que `juste.overflow` vaut `0`.
- Vérifie que `combat.find(cible)->status` vaut `core::CombatantStatus::Down`.
- Vérifie que `chutes.size()` vaut `1U`.
- Vérifie que `combat.find(cible)->profile.currentHitPoints` vaut `0`.
- Vérifie que `chutes.size()` vaut `1U`.
- Vérifie que `degats.size()` vaut `3U`.
- Vérifie que `degats.back().hitPointsBefore` vaut `0`.
- Vérifie que `moitie` vaut `1`.
- Vérifie que `excedent.hitPointsBefore` vaut `6`.
- Vérifie que `excedent.hitPointsAfter` vaut `0`.
- Vérifie que `excedent.overflow` vaut `12`.
- Vérifie que `chutes.size()` vaut `2U`.
- Vérifie que `chutes.back().overflow` vaut `12`.
- Vérifie que `chutes.back().maximumHitPoints` vaut `12`.
- Vérifie que `chutes.back().critical` est vrai.

### DamageTest.UneSalveSeLanceUneFoisEtSAppliqueDUnCoup

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_damage.cpp:327`

Des degats qui touchent plusieurs cibles sont lances une fois pour toutes, et une salve qui abat les deux camps est une defaite quel que soit l'ordre des cibles.

**Étapes**

1. Un allie et un ennemi a 5 PV, combat commence.
2. Un seul jet de 3d6+10 de feu applique aux deux, l'ennemi en premier.

**Résultat attendu**

- Vérifie que `combat.start(hasard)` est vrai.
- Vérifie que `rapports.size()` vaut `2U`.
- Vérifie que `rapports[0].work.hitPointLoss` vaut `rapports[1].work.hitPointLoss`.
- Vérifie que `combat.outcome()` vaut `core::CombatOutcome::Defeat`.

### DamageTest.LesStructuresOntDesPointsDeVieEtLeBestiaireSesAffinites

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_damage.cpp:358`

Une structure de la grille traverse le pipeline avec ses resistances et quitte la grille detruite ; les affinites d'une creature se lisent de son bloc.

**Étapes**

1. Une porte de 10 PV immunisee au poison et resistante au perforant.
2. 30 degats de poison, puis 14 perforants, puis 6 perforants.
3. Une creature resistante au froid, immunisee au poison, vulnerable au feu.

**Résultat attendu**

- Vérifie que `grille.placeObject(ou, porte)` vaut `core::PlacementResult::Placed`.
- Vérifie que `pipeline.applyToStructure(grille, ou, poison).hitPointsAfter` vaut `10`.
- Vérifie que `pipeline.applyToStructure(grille, ou, fleches).hitPointsAfter` vaut `3`.
- Vérifie que `grille.objectAt(ou)` diffère de `nullptr`.
- Vérifie que `pipeline.applyToStructure(grille, ou, fin).overflow` vaut `0`.
- Vérifie que `grille.objectAt(ou)` vaut `nullptr`.
- Vérifie que `traits.affinities.size()` vaut `3U`.
- Vérifie que `traits.affinities[0].kind` vaut `DamageAffinityKind::Immunity`.
- Vérifie que `traits.applies(DamageAffinityKind::Vulnerability, DamageType::Fire, 0)` est vrai.
- Vérifie que `traits.applies(DamageAffinityKind::Resistance, DamageType::Fire, 0)` est faux.

## test_encounter.cpp

### EncounterTest.UnAllerRetourRestitueLEtatDExploration

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:52`

Un aller-retour exploration -> combat -> exploration restitue l'etat.

**Étapes**

1. Relever un etat d'exploration.
2. Engager une rencontre.
3. La terminer par une victoire.

**Résultat attendu**

- Vérifie que `apres.captured` est vrai.
- Vérifie que `apres.playerPosition.x` vaut `avant.playerPosition.x` (comparaison flottante).
- Vérifie que `apres.playerPosition.y` vaut `avant.playerPosition.y` (comparaison flottante).
- Vérifie que `apres.playerFacing.x` vaut `avant.playerFacing.x` (comparaison flottante).
- Vérifie que `apres.playerFacing.y` vaut `avant.playerFacing.y` (comparaison flottante).
- Vérifie que `apres.cameraPosition.x` vaut `avant.cameraPosition.x` (comparaison flottante).
- Vérifie que `apres.cameraPosition.y` vaut `avant.cameraPosition.y` (comparaison flottante).

### EncounterTest.UnEnnemiVaincuNeReapparaitPas

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:80`

Un ennemi vaincu ne reapparait pas.

**Étapes**

1. Gagner une rencontre portant une cle de drapeau.
2. Interroger les drapeaux.

**Résultat attendu**

- Vérifie que `core::encounterAlreadyCleared(drapeaux, cle)` est faux.
- Vérifie que `drapeaux.isSet(cle)` est vrai.
- Vérifie que `core::encounterAlreadyCleared(drapeaux, cle)` est vrai.

### EncounterTest.UneFuiteNeMarquePasLEnnemiVaincu

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:104`

Une fuite ne marque pas l'ennemi vaincu.

**Étapes**

1. Engager une rencontre portant une cle de drapeau.
2. La terminer par une fuite, puis par une defaite.

**Résultat attendu**

- Vérifie que `drapeaux.isSet(cle)` est faux.
- Vérifie que `apresFuite.captured` est vrai.
- Vérifie que `drapeaux.isSet(cle)` est faux.

### EncounterTest.UneRencontreSansCleSeRedeclenche

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:129`

Une rencontre sans cle de drapeau se redeclenche.

**Étapes**

1. Gagner une rencontre sans cle de drapeau.

**Résultat attendu**

- Vérifie que `core::encounterAlreadyCleared(drapeaux, "")` est faux.

### EncounterTest.UneVictoirePoseLeFaitDeLaRencontreGagnee

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:150`

Une victoire pose le fait encounter/&lt;id&gt;/won, une fuite non.

**Étapes**

1. Gagner une rencontre sans cle d'entite.
2. En fuir une autre.

**Résultat attendu**

- Vérifie que `fait` vaut `"encounter/" + modele.id + "/won"`.
- Vérifie que `drapeaux.isSet(fait)` est faux.
- Vérifie que `drapeaux.isSet(fait)` est vrai.

### EncounterTest.LesCombattantsSePlacentRelativementAuDeclencheur

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:176`

Les combattants se placent relativement au declencheur.

**Étapes**

1. Placer la meme rencontre a deux endroits differents.

**Résultat attendu**

- Vérifie que `ici.size()` vaut `2U`.
- Vérifie que `ici[0].position.column` vaut `10`.
- Vérifie que `ici[0].position.row` vaut `9`.
- Vérifie que `ici[1].position.column` vaut `12`.
- Vérifie que `ici[1].position.row` vaut `11`.
- Vérifie que `ailleurs.size()` vaut `2U`.
- Vérifie que `ailleurs[rang].creatureId` vaut `ici[rang].creatureId`.
- Vérifie que `ailleurs[rang].position.column - ici[rang].position.column` vaut `30`.
- Vérifie que `ailleurs[rang].position.row - ici[rang].position.row` vaut `-8`.

### EncounterTest.UneRencontreNonFuyableLeDeclare

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:206`

Une rencontre non fuyable le declare.

**Étapes**

1. Engager une rencontre declaree non fuyable.

**Résultat attendu**

- Vérifie que `core::beginEncounter(rencontre(true), exploration(), {}, "").escapable` est vrai.
- Vérifie que `core::beginEncounter(rencontre(false), exploration(), {}, "").escapable` est faux.

### EncounterTest.LeCatalogueLivreSeCharge

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:222`

Le catalogue de rencontres livre se charge.

**Étapes**

1. Charger Source/Elements/Rpg/encounters.

**Résultat attendu**

- Vérifie que `catalogue.errors.empty()` est vrai.
- Vérifie que `catalogue.encounters.empty()` est faux.
- Vérifie que `combat.id.empty()` est faux.
- Vérifie que `combat.name.empty()` est faux.
- Vérifie que `combat.combatants.empty()` est faux.
- Vérifie que `combattant.creatureId.empty()` est faux.
- Vérifie que `catalogue.find("colisee-fauves")` diffère de `nullptr`.
- Vérifie que `catalogue.find("rencontre-qui-n-existe-pas")` vaut `nullptr`.

### EncounterTest.UnInstantaneNonReleveSeDistingueDeLOrigine

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:250`

Un instantane non releve se distingue de l'origine.

**Étapes**

1. Construire un instantane par defaut.

**Résultat attendu**

- Vérifie que `vide.captured` est faux.

### EncounterTest.UnEnnemiPosePorteUneCleUneZoneNonN

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:266`

Un ennemi pose porte une cle, une zone n'en porte pas.

**Étapes**

1. Lire une entite de rencontre posee.
2. Lire la meme avec respawns.

**Résultat attendu**

- Vérifie que `declencheur.has_value()` est vrai.
- Vérifie que `declencheur->encounterId` vaut `"colisee-fauves"`.
- Vérifie que `declencheur->position.column` vaut `5`.
- Vérifie que `declencheur->defeatFlagKey.empty()` est faux.
- Vérifie que `declencheurZone.has_value()` est vrai.
- Vérifie que `declencheurZone->defeatFlagKey.empty()` est vrai.

### EncounterTest.DeuxDeclencheursNePartagentJamaisLeurCle

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:297`

Deux declencheurs ne partagent jamais leur cle.

**Étapes**

1. Lire deux entites de rencontre a des cases differentes de la meme carte.
2. Lire la meme case sur deux cartes differentes.

**Résultat attendu**

- Vérifie que `a && b && ailleurs` est vrai.
- Vérifie que `a->defeatFlagKey` diffère de `b->defeatFlagKey`.
- Vérifie que `a->defeatFlagKey` diffère de `ailleurs->defeatFlagKey`.

### EncounterTest.CeQuiNEstPasUnDeclencheurNEnDevientPasUn

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_encounter.cpp:323`

Ce qui n'est pas un declencheur n'en devient pas un.

**Étapes**

1. Lire une entite d'un autre type.
2. Lire une entite de rencontre sans encounterId.

**Résultat attendu**

- Vérifie que `core::encounterTriggerFor(coffre, "grotte").has_value()` est faux.
- Vérifie que `core::encounterTriggerFor(sansRencontre, "grotte").has_value()` est faux.

## test_enemy_ai.cpp

### FlankingTest.LaLigneDesCentresTranche

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:107`

Deux allies prennent un ennemi en tenaille si la ligne entre leurs centres passe par deux cotes ou deux angles opposes de son emplacement, s'ils lui sont adjacents, debout, et le voient.

**Étapes**

1. Un ennemi en (5,5) ; tester la geometrie de cotes opposes, d'angles opposes, d'une position en L et de deux cases du meme cote.
2. Un ennemi de taille G en (5,5)-(6,6).
3. Dans un combat : l'allie a terre, puis un mur entre l'allie et l'ennemi.

**Résultat attendu**

- Vérifie que `core::crossesOppositeSides({4, 5}, {6, 5}, moyen)` est vrai.
- Vérifie que `core::crossesOppositeSides({5, 4}, {5, 6}, moyen)` est vrai.
- Vérifie que `core::crossesOppositeSides({4, 4}, {6, 6}, moyen)` est vrai.
- Vérifie que `core::crossesOppositeSides({6, 4}, {4, 6}, moyen)` est vrai.
- Vérifie que `core::crossesOppositeSides({4, 5}, {6, 6}, moyen)` est faux.
- Vérifie que `core::crossesOppositeSides({4, 4}, {6, 5}, moyen)` est faux.
- Vérifie que `core::crossesOppositeSides({4, 4}, {4, 6}, moyen)` est faux.
- Vérifie que `core::crossesOppositeSides({4, 5}, {5, 4}, moyen)` est faux.
- Vérifie que `core::crossesOppositeSides({4, 5}, {7, 6}, grand)` est vrai.
- Vérifie que `core::crossesOppositeSides({4, 4}, {7, 7}, grand)` est vrai.
- Vérifie que `core::crossesOppositeSides({4, 5}, {5, 7}, grand)` est faux.
- Vérifie que `core::isFlanked(session.combat(), CombatantId{1}, CombatantId{3})` est vrai.
- Vérifie que `core::isFlanked(session.combat(), CombatantId{2}, CombatantId{3})` est vrai.
- Vérifie que `core::isFlankedFrom(session.combat(), CombatantId{1}, {4, 2}, CombatantId{3})` est faux.
- Vérifie que `core::isFlanked(session.combat(), CombatantId{3}, CombatantId{1})` est faux.
- Vérifie que `core::isFlanked(session.combat(), CombatantId{1}, CombatantId{3})` est faux.
- Vérifie que `core::crossesOppositeSides({4, 4}, {6, 2}, {.anchor = {5, 3}, .side = 1})` est vrai.
- Vérifie que `core::isFlanked(murs.combat(), CombatantId{1}, CombatantId{3})` est faux.

### FlankingTest.LaTenailleDonneLAvantageDansLArene

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:161`

Dans une arene a prise en tenaille, une attaque au corps a corps contre un ennemi pris en tenaille est jetee avec avantage, et le journal le dit ; ailleurs, non.

**Étapes**

1. Deux allies de part et d'autre d'un ennemi, le premier joue.
2. Il attaque, dans une arene avec tenaille puis sans.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{1}`.
- Vérifie que `attaque.outcome.has_value()` est vrai.
- Vérifie que `attaque.outcome->roll.check.stance` vaut `tenaille ? core::RollStance::Advantage : core::RollStance::Normal`.
- Vérifie que `attaque.outcome->describe().find("prise en tenaille") != std::string::npos` vaut `tenaille`.

### EnemyAiTest.LeJetRequisEtLEsperanceSuiventLeGuide

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:192`

Le jet requis est la CA moins le bonus d'attaque ; la chance de toucher et l'esperance de degats en decoulent, en entiers.

**Étapes**

1. L'exemple du Guide : des orcs a +5 contre un guerrier a CA 19.
2. La chance d'un jet requis de 14, normal, avec avantage, avec desavantage ; d'un jet requis de 1 et de 25.
3. L'esperance d'un gobelin (+4, 1d6+2) contre CA 15, et contre CA 15 avec un seuil critique a 19.

**Résultat attendu**

- Vérifie que `core::requiredRoll(19, 5)` vaut `14`.
- Vérifie que `core::hitChance(14, core::RollStance::Normal)` vaut `140`.
- Vérifie que `core::hitChance(14, core::RollStance::Advantage)` vaut `231`.
- Vérifie que `core::hitChance(14, core::RollStance::Disadvantage)` vaut `49`.
- Vérifie que `core::hitChance(1, core::RollStance::Normal)` vaut `380`.
- Vérifie que `core::hitChance(25, core::RollStance::Normal)` vaut `20`.
- Vérifie que `core::criticalChance(20, core::RollStance::Normal)` vaut `20`.
- Vérifie que `core::criticalChance(19, core::RollStance::Advantage)` vaut `400 - 18 * 18`.
- Vérifie que `core::attackBonusOf(gobelin)` vaut `4`.
- Vérifie que `core::expectedDamage(gobelin, 15, core::RollStance::Normal)` vaut `2340`.
- Vérifie que `core::expectedDamage(gobelin, 15, core::RollStance::Normal)` vaut `11 * 200 + 7 * 40`.

### EnemyAiTest.LesProfilsSontDesDonnees

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:226`

Les profils de comportement livres se chargent sans erreur, ne tolerent jamais trois menaces, et s'attribuent aux creatures par leurs regles.

**Étapes**

1. Charger behaviors.json.
2. Charger le bestiaire, lire le profil du loup (Tactique de groupe), du sanglier et du singe.
3. Un sanglier synthetique qui tire plus fort qu'il ne mord.

**Résultat attendu**

- Vérifie que `profils.errors.empty()` est vrai.
- Vérifie que `profil` diffère de `nullptr`.
- Vérifie que `profil->toleratedThreats` est inférieur ou égal à `2`.
- Vérifie que `profil->approachPerTile` est strictement supérieur à `0`.
- Vérifie que `profils.defaultBehavior` vaut `"aggressive"`.
- Vérifie que `trouver("wolf")` diffère de `bestiaire.creatures.end()`.
- Vérifie que `trouver("boar")` diffère de `bestiaire.creatures.end()`.
- Vérifie que `trouver("ape")` diffère de `bestiaire.creatures.end()`.
- Vérifie que `core::behaviorFor(*trouver("wolf"), profils)` vaut `"pack"`.
- Vérifie que `core::behaviorFor(*trouver("boar"), profils)` vaut `"aggressive"`.
- Vérifie que `core::behaviorFor(*trouver("ape"), profils)` vaut `"archer"`.
- Vérifie que `core::behaviorFor(tireur, profils)` vaut `"archer"`.

### EnemyAiTest.LIaNeLitQueLEtatEnsanglante

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:283`

Exigences : `EX-CBT-050`

Les points de vie d'un adversaire restent secrets : deux cibles qui ne different que par des points de vie au-dessus de la moitie sont indiscernables ; une cible ensanglantee est preferee.

**Étapes**

1. Un gobelin agressif au contact de deux heros identiques, a 20/20 et 11/20.
2. Planifier son tour.
3. Le second passe a 9/20, planifier encore.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{3}`.
- Vérifie que `plein.action` vaut `core::TurnAction::Attack`.
- Vérifie que `plein.target` vaut `CombatantId{1}`.
- Vérifie que `entame.target` vaut `CombatantId{1}`.
- Vérifie que `entame.score` vaut `plein.score`.
- Vérifie que `ensanglante.target` vaut `CombatantId{2}`.
- Vérifie que `ensanglante.score` est strictement supérieur à `plein.score`.

### EnemyAiTest.PasDeSuicideQuandUneCaseSureExiste

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:323`

Aucun profil ne finit son tour a portee immediate de trois ennemis quand une case moins exposee etait atteignable.

**Étapes**

1. Trois heros inoffensifs en (6,2), (6,4) et (7,3), le dernier ensanglante ; la case (6,3) les touche tous, et c'est la seule d'ou le gobelin prend le heros ensanglante en tenaille avec son complice en (8,3) : la plus rentable, et la seule dangereuse. D'autres cases frappent au plus deux heros.
2. Pour chaque profil livre, jouer le tour du gobelin.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{4}`.
- Vérifie que `core::playTurn(session, profils)` est vrai.
- Vérifie que `aPortee` est inférieur ou égal à `2`.
- Vérifie que `fin` diffère de `(GridPosition{6, 3})`.

### EnemyAiTest.UnTireurNeComptePasDansLAntiSuicide

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:369`

Deux archers allies qui couvrent toute la salle n'empechent pas un prudent d'aller frapper le heros au contact.

**Étapes**

1. Salle 12x8 : un heros de contact en (7,3), deux archers allies en (2,2) et (2,5) dont la portee couvre la salle, un gobelin prudent (tolere une menace) en (5,3).
2. Planifier le tour du gobelin.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{4}`.
- Vérifie que `plan.action` vaut `core::TurnAction::Attack`.
- Vérifie que `plan.target` vaut `CombatantId{1}`.
- Vérifie que `plan.immediateThreats` vaut `1`.

### EnemyAiTest.SansAttaquePossibleChaqueProfilAvance

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:401`

Dans une salle aux dimensions de l'arene, un ennemi de chaque profil qui ne peut pas encore frapper se rapproche a chaque tour, jusqu'a attaquer.

**Étapes**

1. Salle 20x14, heros en (3,6) (+4, 1d8+2), ennemi de contact en (16,6) a treize cases, pour chacun des profils livres.
2. Le heros passe son tour ; jouer l'ennemi par l'IA, jusqu'a six tours.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `distance` vaut `13`.
- Vérifie que `session.endTurn()` est vrai.
- Vérifie que `core::playTurn(session, profils)` est vrai.
- Vérifie que `apres` est strictement inférieur à `distance`.
- Vérifie que `aAttaque()` est vrai.
- Vérifie que `toursEnnemi` est inférieur ou égal à `3`.

### EnemyAiTest.LeRepliVaALaCaseSureLaPlusProche

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:458`

Une archere prudente tire puis recule juste hors de portee du heros, vers la case la plus proche de lui.

**Étapes**

1. Salle 20x8, heros de contact en (16,3), archere prudente en (10,3) a sept cases de portee : le heros atteint en un tour toute case de la colonne 9 et au-dela, et elle ne peut tirer que de la.
2. Jouer le tour de l'archere.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{2}`.
- Vérifie que `core::playTurn(session, profils)` est vrai.
- Vérifie que `std::ranges::any_of( journal, [](const std::string& l) { return l.starts_with("attaque Archere -> Heros"); })` est vrai.
- Vérifie que `std::ranges::any_of( journal, [](const std::string& l) { return l.find(": recule en") != std::string::npos; })` est vrai.
- Vérifie que `fin.column` vaut `8`.
- Vérifie que `fin.row` vaut `1`.

### EnemyAiTest.LArchereChercheLaVue

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:499`

Une IA archere cachee de sa cible par un pilier se deplace jusqu'a une case qui la voit et tire ; elle n'essaie jamais un tir que la ligne de vue refuse.

**Étapes**

1. Une archere en (5,3), un heros en (7,3) derriere un pilier en (6,3).
2. Jouer son tour par l'IA.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{2}`.
- Vérifie que `plan.action` vaut `core::TurnAction::Attack`.
- Vérifie que `plan.moveTo.has_value()` est vrai.
- Vérifie que `core::hasLineOfSight(session.combat().grid(), {.anchor = *plan.moveTo, .side = 1}, {.anchor = {7, 3}, .side = 1})` est vrai.
- Vérifie que `core::playTurn(session, profils)` est vrai.
- Vérifie que `std::ranges::any_of( journal, [](const std::string& l) { return l.starts_with("attaque Archere -> Heros"); })` est vrai.

### EnemyAiTest.SePrecipiterEtChoisirSesOpportunites

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:533`

Une IA qui ne peut attaquer se precipite vers l'ennemi le plus proche ; une IA prudente laisse passer une attaque d'opportunite dont le jet requis depasse son seuil, une agressive la prend.

**Étapes**

1. Un gobelin agressif a quinze cases d'un heros ; jouer son tour.
2. Un ogre au contact d'un heros a CA 20 (+4 : jet requis 16) ; le heros s'eloigne, l'ogre prudent puis agressif.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `core::playTurn(session, profils)` est vrai.
- Vérifie que `std::ranges::any_of(session.journal(), [](const std::string& l) { return l.starts_with("precipitation Gobelin"); })` est vrai.
- Vérifie que `session.combat().grid().positionOf(CombatantId{2})->column` est strictement supérieur à `7`.
- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().activeCombatant()` vaut `CombatantId{1}`.
- Vérifie que `session.move({2, 5}).result` vaut `core::MoveResult::Moved`.
- Vérifie que `prises` vaut `std::string(profil) == "aggressive" ? 1 : 0`.

### EnemyAiTest.UnCombatIaContreIaSeTermineToujoursEtSeRejoue

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_enemy_ai.cpp:582`

Sur des configurations generees -- salles, piliers, compositions, profils, tireurs, tenaille ou non --, un combat joue par l'IA des deux cotes atteint son issue ; a graine fixee, deux parties donnent le meme journal.

**Étapes**

1. Pour trente graines, generer une salle de 10 a 16 cases sur 8 a 10 avec des piliers, deux a quatre combattants par camp, chacun avec un profil livre, un tiers de tireurs, la tenaille une fois sur deux.
2. Jouer par l'IA jusqu'a l'issue, avec une garde de 600 tours.
3. Une graine sur trois, rejouer par `replay`.

**Résultat attendu**

- Vérifie que `profils.errors.empty()` est vrai.
- Vérifie que `montage.refusals.empty()` est vrai.
- Vérifie que `session.start()` est vrai.
- Vérifie que `tours` est supérieur ou égal à `0`.
- Vérifie que `session.outcome().has_value()` est vrai.
- Vérifie que `jouerParLIa(session, profils)` vaut `tours`.
- Vérifie que `session.journal()` vaut `journal`.
- Vérifie que `longest` est strictement supérieur à `0`.

## test_iso_projection.cpp

### IsoProjectionTest.DimensionsDeLaScene

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:85`

Une grille 4 × 3 à losange de 10 unités : H = 6,2, murs 8,5, scène 35 × 30,2, origine (10 ; 8,5).

**Étapes**

1. Construire la projection d'une grille 4 × 3, losange de 10 unités.
2. Lire les dimensions.

**Résultat attendu**

- Vérifie que `projection.tileWidth()` vaut `10.0f`, à `TOLERANCE` près.
- Vérifie que `projection.tileHeight()` vaut `6.2f`, à `TOLERANCE` près.
- Vérifie que `projection.wallHeight()` vaut `8.5f`, à `TOLERANCE` près.
- Vérifie que `projection.diagonals()` vaut `7`.

### IsoProjectionTest.LesCasesDAngleTouchentLesBords

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:108`

Sur une grille 4 × 3, la case (0, 2) touche le bord gauche, (3, 0) le bord droit, (3, 2) le bord bas, et (0, 0) est posée sous la bande des murs.

**Étapes**

1. Lire la boîte du losange des quatre cases d'angle.

**Résultat attendu**

- Vérifie que `gauche.left()` vaut `0.0f`, à `TOLERANCE` près.
- Vérifie que `gauche.top()` vaut `14.7f`, à `TOLERANCE` près.
- Vérifie que `droite.left()` vaut `25.0f`, à `TOLERANCE` près.
- Vérifie que `droite.top()` vaut `17.8f`, à `TOLERANCE` près.
- Vérifie que `droite.right()` vaut `projection.sceneSize().x`, à `TOLERANCE` près.
- Vérifie que `bas.left()` vaut `15.0f`, à `TOLERANCE` près.
- Vérifie que `bas.top()` vaut `24.0f`, à `TOLERANCE` près.
- Vérifie que `bas.bottom()` vaut `projection.sceneSize().y`, à `TOLERANCE` près.

### IsoProjectionTest.LaCaseCentraleEstAuCentre

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:144`

Sur une grille 5 × 5, le centre de la case (2, 2) est au milieu horizontal de la scène et au milieu vertical du plateau (sous la bande des murs).

**Étapes**

1. Construire la projection 5 × 5, losange de 10.
2. Lire le centre de (2, 2).

**Résultat attendu**

- (25 ; 8,5 + 31 / 2 = 24).

### IsoProjectionTest.LeSensDesAxes

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:161`

Garde contre l'erreur de signe : un pas de colonne déplace le centre de (+L/2 ; +H/2), un pas de ligne de (−L/2 ; +H/2).

**Étapes**

1. Comparer les centres de (1, 1), (2, 1) et (1, 2) sur une grille 4 × 3.
2. Lire les quatre sommets du losange de (1, 1) en coordonnées de grille continues.

**Résultat attendu**

- Colonne : (+5 ; +3,1) ; ligne : (−5 ; +3,1) ; sommets haut, droit, bas, gauche aux milieux et coins de la boîte du losange.

### IsoProjectionTest.AllerRetourCaseMondeCase

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:191`

Chaque case d'une grille 12 × 9, projetée à son centre puis relue, redonne la même case ; de même pour quatre points pris juste à l'intérieur de ses sommets.

**Étapes**

1. Pour chaque case : centre → `worldToTile`.
2. Pour chaque case : les points de grille (c + 0,02 ; r + 0,02), (c + 0,98 ; r + 0,02), (c + 0,02 ; r + 0,98), (c + 0,98 ; r + 0,98) → monde → `worldToTile`.

**Résultat attendu**

- Toujours la case de départ.

### IsoProjectionTest.AllerRetourContinu

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:220`

`worldToGrid` est l'inverse de `gridToWorld`, y compris hors de la grille.

**Étapes**

1. Parcourir un quadrillage de points continus de −3 à 15 par pas de 0,37 sur une grille 12 × 9, losange de 7,3.
2. Projeter, puis relire.

**Résultat attendu**

- Vérifie que `relu.x` vaut `gc`, à `1e-3f` près.
- Vérifie que `relu.y` vaut `gr`, à `1e-3f` près.

### IsoProjectionTest.LaRelectureSuitLeLosange

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:245`

Un point dans un coin de la boîte d'une case, hors de son losange, appartient à la voisine ; les arêtes appartiennent à la case de plus grand indice ; hors grille, rien.

**Étapes**

1. Grille 5 × 5, losange 4, rapport 0,5, sans murs (valeurs exactes en binaire).
2. Lire les quatre coins de la boîte de (2, 2), rentrés d'un centième.
3. Lire les sommets exacts de (2, 2).
4. Lire le centre projeté de (−1, 0), (5, 2), (2, 5) et un point au-dessus de la scène.

**Résultat attendu**

- Vérifie que `projection.worldToTile(projection.tileToWorld({-1, 0})).has_value()` est faux.
- Vérifie que `projection.worldToTile(projection.tileToWorld({5, 2})).has_value()` est faux.
- Vérifie que `projection.worldToTile(projection.tileToWorld({2, 5})).has_value()` est faux.
- Vérifie que `projection.worldToTile({10.0f, -1.0f}).has_value()` est faux.
- Vérifie que `projection.contains({0, 5})` est faux.
- Vérifie que `projection.contains({4, 4})` est vrai.

### IsoProjectionTest.ConformeALaSceneQml

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:283`

Pour plusieurs tailles d'élément et de grille, la position QML de chaque brique `ArenaTile` vaut décalage + échelle × position monde, où l'échelle est le rapport des largeurs de losange et le décalage celui qui centre la scène.

**Étapes**

1. Transcrire littéralement les formules de l'écran QML du Colisée (LOT-50).
2. Pour des éléments 1280 × 720, 800 × 900, 640 × 360 et des grilles 16 × 12, 9 × 14, 1 × 1 : comparer la position de chaque case.

**Résultat attendu**

- Vérifie que `scene.x` vaut `qml.sceneWidth()`, à `1e-2f` près.
- Vérifie que `scene.y` vaut `qml.sceneHeight()`, à `1e-2f` près.
- Vérifie que `pixel.x` vaut `qml.x(c, r)`, à `1e-2f` près.
- Vérifie que `pixel.y` vaut `qml.y(c, r)`, à `1e-2f` près.

### IsoProjectionTest.ValeursParDefautEtEntreesDegenerees

*Mineur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_iso_projection.cpp:325`

Le losange par défaut fait 86 / 16 unités (la tuile de la planche à sa taille native au zoom 1) ; la profondeur est colonne + ligne ; une taille négative devient nulle, une largeur nulle devient celle par défaut, une grille vide garde une diagonale.

**Étapes**

1. Construire une projection par défaut, puis des projections dégénérées.

**Résultat attendu**

- Vérifie que `defaut.tileWidth()` vaut `5.375f`, à `TOLERANCE` près.
- Vérifie que `defaut.tileHeight()` vaut `3.3325f`, à `TOLERANCE` près.
- Vérifie que `core::IsoProjection::depth({3, 4})` vaut `7`.
- Vérifie que `vide.columns()` vaut `0`.
- Vérifie que `vide.rows()` vaut `0`.
- Vérifie que `vide.diagonals()` vaut `1`.
- Vérifie que `vide.tileWidth()` vaut `core::ARENA_TILE_WIDTH_UNITS`, à `TOLERANCE` près.
- Vérifie que `vide.worldToTile(vide.tileToWorld({0, 0})).has_value()` est faux.

## test_line_of_sight.cpp

### LineOfSightTest.LaVueEstSymetriqueSurDesGrillesGenerees

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_line_of_sight.cpp:115`

A voit B si et seulement si B voit A — vérifié exhaustivement sur des grilles générées, pour chaque paire de points de grille et chaque paire d'emprises, 1 × 1 et 2 × 2.

**Étapes**

1. Générer 20 grilles 5 × 5 à des densités de 10 à 55 % de murs, d'eau profonde, de portes et de herses.
2. Pour chaque paire ordonnée de points de grille : comparer les deux sens du segment.
3. Pour chaque paire ordonnée d'emprises 1 × 1, et d'emprises 2 × 2 contre 1 × 1 : comparer les deux sens de la ligne de vue.
4. Pour chacune : l'abri total équivaut à l'absence de ligne de vue.

**Résultat attendu**

- Vérifie que `core::isSightClear(grille, a, b)` vaut `core::isSightClear(grille, b, a)`.
- Vérifie que `ab` vaut `core::hasLineOfSight(grille, b, a)`.
- Vérifie que `core::coverFrom(grille, a, b) == Cover::Total` vaut `!ab`.
- Vérifie que `core::coverFrom(grille, b, a) == Cover::Total` vaut `!ab`.
- Vérifie que `segments` est strictement supérieur à `25000U`.
- Vérifie que `vues` est strictement supérieur à `5000U`.
- Vérifie que `cachees` est strictement supérieur à `1000U`.

### LineOfSightTest.CeQuiArreteLaVue

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_line_of_sight.cpp:181`

Un mur caché, un gouffre non ; le coin commun de deux murs ne laisse pas passer le regard ; une porte fermée cache comme un mur.

**Étapes**

1. Deux combattants alignés, un mur d'une case entre eux.
2. Les mêmes, séparés par une rivière d'eau profonde.
3. Deux combattants en diagonale, deux murs sur les deux autres cases du carré ; puis un seul mur.
4. Une porte fermée entre deux combattants.

**Résultat attendu**

- Vérifie que `core::hasLineOfSight(mur, une(0, 1), une(2, 1))` est faux.
- Vérifie que `core::coverFrom(mur, une(0, 1), une(2, 1))` vaut `Cover::Total`.
- Vérifie que `core::hasLineOfSight(mur, une(0, 0), une(2, 0))` est vrai.
- Vérifie que `core::hasLineOfSight(riviere, une(0, 1), une(4, 1))` est vrai.
- Vérifie que `core::coverFrom(riviere, une(0, 1), une(4, 1))` vaut `Cover::None`.
- Vérifie que `core::hasLineOfSight(coin, une(0, 0), une(1, 1))` est faux.
- Vérifie que `core::hasLineOfSight(demiCoin, une(0, 0), une(1, 1))` est vrai.
- Vérifie que `core::hasLineOfSight(porte, une(1, 0), une(1, 2))` est faux.

### LineOfSightTest.LesAbrisEtCeQuiLesDonne

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_line_of_sight.cpp:213`

Un muret et une créature interposée abritent partiellement, une herse de façon importante ; un angle de mur donne l'abri selon les lignes qu'il coupe ; les abris ne s'additionnent pas.

**Étapes**

1. Un tireur, une cible à trois cases, rien entre eux.
2. Un muret, puis une herse, sur la case devant la cible.
3. Une créature sur cette case.
4. La cible derrière un angle de mur.
5. La herse et une créature ensemble.

**Résultat attendu**

- Vérifie que `core::coverFrom(libre, une(0, 1), une(3, 1))` vaut `Cover::None`.
- Vérifie que `core::coverFrom(muret, une(0, 1), une(3, 1))` vaut `Cover::Half`.
- Vérifie que `core::coverFrom(herse, une(0, 1), une(3, 1))` vaut `Cover::ThreeQuarters`.
- Vérifie que `core::coverFrom(libre, une(0, 1), une(3, 1), garde)` vaut `Cover::Half`.
- Vérifie que `core::coverFrom(herse, une(0, 1), une(3, 1), garde)` vaut `Cover::ThreeQuarters`.
- Vérifie que `core::coverFrom(angle, une(0, 0), une(3, 1))` vaut `Cover::Half`.
- Vérifie que `core::coverFromPoint(angle, coin, une(3, 1))` vaut `Cover::Half`.
- Vérifie que `core::coverFromPoint(libre, coin, une(3, 1), voisin)` vaut `Cover::Half`.
- Vérifie que `core::coverFromPoint(angle, coin, une(3, 1), voisin)` vaut `Cover::Half`.
- Vérifie que `core::coverFrom(angle, une(0, 0), une(3, 1), voisin)` vaut `Cover::Half`.
- Vérifie que `core::coverBonus(Cover::None)` vaut `0`.
- Vérifie que `core::coverBonus(Cover::Half)` vaut `2`.
- Vérifie que `core::coverBonus(Cover::ThreeQuarters)` vaut `5`.
- Vérifie que `core::coverBonus(Cover::Total)` vaut `0`.

## test_map_encounter.cpp

### MapEncounterTest.LaZoneDuDeclencheurEstChoisieEtLesCasesTranslatees

*Critique · Unitaire · Combat sur la carte* — `Source/Test/Unit/Core/Combat/test_map_encounter.cpp:63`

Une rencontre se pose sur la zone de combat du declencheur, cases translatees.

**Étapes**

1. Preparer une rencontre de deux loups, declenchee en (14, 8), heros en (15, 10), sur une carte dont la zone « cour » couvre (10, 5) a (19, 12).

**Résultat attendu**

- Vérifie que `resultat.ok()` est vrai.
- Vérifie que `montage.zone.name` vaut `"cour"`.
- Vérifie que `montage.battlefield.tileMap().width()` vaut `10`.
- Vérifie que `montage.battlefield.tileMap().height()` vaut `8`.
- Vérifie que `montage.heroCell` vaut `(core::GridPosition{.column = 5, .row = 5})`.
- Vérifie que `montage.run.placements.size()` vaut `2U`.
- Vérifie que `montage.run.placements[0].position` vaut `(core::GridPosition{.column = 4, .row = 2})`.
- Vérifie que `montage.run.placements[1].position` vaut `(core::GridPosition{.column = 5, .row = 3})`.
- Vérifie que `montage.run.escapable` est faux.
- Vérifie que `montage.run.defeatFlagKey` vaut `"essai/loups"`.
- Vérifie que `montage.notes.empty()` est vrai.
- Vérifie que `core::zoneToMap(montage.zone, montage.heroCell)` vaut `(core::GridPosition{.column = 15, .row = 10})`.
- Vérifie que `core::mapToZone(montage.zone, {.column = 15, .row = 10})` vaut `montage.heroCell`.

### MapEncounterTest.UnePlaceImpossibleSeRapprocheEtSeNote

*Critique · Unitaire · Combat sur la carte* — `Source/Test/Unit/Core/Combat/test_map_encounter.cpp:98`

Une place impossible est rapprochee de la case voulue, et notee.

**Étapes**

1. Declencher en (12, 7) : le loup « un pas devant » vise (12, 6), un mur.
2. Declencher en (10, 5) avec le heros hors de la zone, en (2, 2) : le second loup vise (11, 5), la place du heros vise une case hors zone.

**Résultat attendu**

- Vérifie que `mur.ok()` est vrai.
- Vérifie que `posee` diffère de `(core::GridPosition{.column = 12, .row = 6})`.
- Vérifie que `std::max(std::abs(posee.column - 12), std::abs(posee.row - 6))` est inférieur ou égal à `1`.
- Vérifie que `mur.setup->notes.size()` vaut `1U`.
- Vérifie que `mur.setup->notes.front().find("wolf")` diffère de `std::string::npos`.
- Vérifie que `dehors.ok()` est vrai.
- Vérifie que `core::zoneToMap(dehors.setup->zone, dehors.setup->heroCell)` vaut `(core::GridPosition{.column = 10, .row = 5})`.
- Vérifie que `std::ranges::find(cases, place.position)` vaut `cases.end()`.
- Vérifie que `dehors.setup->zone.contains(core::zoneToMap(dehors.setup->zone, place.position))` est vrai.

### MapEncounterTest.SansZoneLaRencontreEstRefusee

*Majeur · Unitaire · Combat sur la carte* — `Source/Test/Unit/Core/Combat/test_map_encounter.cpp:139`

Une rencontre hors de toute zone de combat est refusee.

**Étapes**

1. Declencher en (2, 2), heros en (3, 3), hors de la zone « cour ».

**Résultat attendu**

- Vérifie que `resultat.ok()` est faux.
- Vérifie que `resultat.issue.find("essai")` diffère de `std::string::npos`.
- Vérifie que `resultat.issue.find("2,2")` diffère de `std::string::npos`.
- Vérifie que `resultat.issue.find("3,3")` diffère de `std::string::npos`.

## test_pathfinding.cpp

### PathfindingTest.LeBudgetSeDeduitDeLaVitesse

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:69`

Le budget de deplacement vaut la vitesse divisee par 1,5, arrondie en dessous.

**Étapes**

1. Convertir 9 m, 7,5 m, 10 m, 0 m et -3 m.
2. Lire le budget d'une fiche a 9 m.
3. Lire le budget de marche et de vol d'une creature qui ne vole pas, puis d'une qui vole.

**Résultat attendu**

- Vérifie que `core::movementBudget(9.0F)` vaut `6`.
- Vérifie que `core::movementBudget(7.5F)` vaut `5`.
- Vérifie que `core::movementBudget(10.0F)` vaut `6`.
- Vérifie que `core::movementBudget(13.5F - 4.5F)` vaut `6`.
- Vérifie que `core::movementBudget(0.0F)` vaut `0`.
- Vérifie que `core::movementBudget(-3.0F)` vaut `0`.
- Vérifie que `core::movementBudget(fiche)` vaut `6`.
- Vérifie que `core::movementBudget(loup, core::Locomotion::Walk)` vaut `8`.
- Vérifie que `core::movementBudget(loup, core::Locomotion::Fly)` vaut `0`.
- Vérifie que `core::movementBudget(chouette, core::Locomotion::Fly)` vaut `12`.

### PathfindingTest.LesCasesAtteignablesCorrespondentExactementAuBudget

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:103`

Les cases atteignables correspondent exactement au budget.

**Étapes**

1. Grille ouverte 11x11, heros au centre.
2. Calculer l'aire atteignable pour les budgets 0 a 5.
3. Comparer chaque case a sa distance de Tchebychev.

**Résultat attendu**

- Vérifie que `aire.origin()` vaut `centre`.
- Vérifie que `aire.destinations().size()` vaut `attendu`.
- Vérifie que `aire.canEndAt(case_)` vaut `distance >= 1 && distance <= budget`.
- Vérifie que `aire.costTo(case_)` vaut `distance`.
- Vérifie que `aire.costTo(case_).has_value()` est faux.

### PathfindingTest.UnMurCouteLeDetour

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:140`

Un mur entre deux cases voisines coute le detour.

**Étapes**

1. Grille 7x5, mur vertical en colonne 3 des lignes 0 a 2.
2. Heros en (2,1), cible en (4,1), a deux cases de l'autre cote du mur.

**Résultat attendu**

- Vérifie que `grille.place(HEROS, {2, 1})` vaut `core::PlacementResult::Placed`.
- Vérifie que `courte.canEndAt({4, 1})` est faux.
- Vérifie que `longue.canEndAt({4, 1})` est vrai.
- Vérifie que `longue.costTo({4, 1})` vaut `6`.
- Vérifie que `chemin.has_value()` est vrai.
- Vérifie que `chemin->cost` vaut `6`.
- Vérifie que `chemin->steps.size()` vaut `6U`.
- Vérifie que `chemin->steps.back()` vaut `(core::GridPosition{4, 1})`.

### PathfindingTest.UneDiagonaleNeCoupePasLeCoinDUnMur

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:172`

Une diagonale ne coupe pas le coin d'un mur.

**Étapes**

1. Grille 3x3, mur en (1,0).
2. Heros en (0,0), cible (1,1) en diagonale.
3. Meme chose avec de l'eau profonde a la place du mur.

**Résultat attendu**

- Vérifie que `grilleMur.place(HEROS, {0, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `core::ReachableArea(grilleMur, {.combatant = HEROS}, 3).costTo({1, 1})` vaut `2`.
- Vérifie que `grilleMare.place(HEROS, {0, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `core::ReachableArea(grilleMare, {.combatant = HEROS}, 3).costTo({1, 1})` vaut `1`.

### PathfindingTest.LeTerrainDifficileDoubleLeCoutEtReduitLaPortee

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:197`

Le terrain difficile double le cout et reduit la portee.

**Étapes**

1. Grille ouverte 11x11, heros au centre, budget 4.
2. Rendre difficile une case voisine, puis toute la grille.
3. Recalculer l'aire avec un budget de 1 contre une case difficile.

**Résultat attendu**

- Vérifie que `uneCase.costTo(voisine)` vaut `2`.
- Vérifie que `uneCase.costTo({7, 5})` vaut `2`.
- Vérifie que `core::ReachableArea(grille, {.combatant = HEROS}, 1).canEndAt(voisine)` est faux.
- Vérifie que `core::ReachableArea(grilleOuverte(11), {.combatant = HEROS}, 4).destinations().size()` vaut `80U`.
- Vérifie que `boue.destinations().size()` vaut `24U`.
- Vérifie que `boue.costTo({7, 7})` vaut `4`.
- Vérifie que `boue.costTo({8, 5}).has_value()` est faux.

### PathfindingTest.UnAllieSeTraverseUnEnnemiNon

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:234`

Un allie se traverse, un ennemi non, et aucun ne sert de destination.

**Étapes**

1. Couloir 5x1 : heros en (0,0), allie en (1,0), ennemi en (3,0).
2. Calculer l'aire sans droit de passage, puis en autorisant l'allie.

**Résultat attendu**

- Vérifie que `grille.place(HEROS, {0, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.place(ALLIE, {1, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.place(ENNEMI, {3, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `bloque.destinations().empty()` est vrai.
- Vérifie que `passage.destinations()` vaut `(std::vector<core::GridPosition>{{2, 0}})`.
- Vérifie que `passage.costTo({1, 0})` vaut `2`.
- Vérifie que `passage.costTo({2, 0})` vaut `3`.
- Vérifie que `passage.canEndAt({1, 0})` est faux.
- Vérifie que `passage.costTo({4, 0}).has_value()` est faux.
- Vérifie que `core::findPath(grille, heros, {4, 0}).has_value()` est faux.
- Vérifie que `core::findPath(grille, heros, {1, 0}).has_value()` est faux.

### PathfindingTest.MemeEntreeMemeChemin

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:268`

Meme entree, meme chemin, et le departage suit la regle du plus petit indice.

**Étapes**

1. Grille ouverte 3x3, heros en (0,0).
2. Demander le chemin vers (2,0) et vers (0,2), chacun a deux chemins de cout 2.
3. Repeter cent fois la requete sur une grille accidentee.

**Résultat attendu**

- Vérifie que `petite.place(HEROS, {0, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `versLaDroite.has_value()` est vrai.
- Vérifie que `versLaDroite->steps` vaut `(std::vector<core::GridPosition>{{1, 0}, {2, 0}})`.
- Vérifie que `versLeBas.has_value()` est vrai.
- Vérifie que `versLeBas->steps` vaut `(std::vector<core::GridPosition>{{0, 1}, {0, 2}})`.
- Vérifie que `reference.has_value()` est vrai.
- Vérifie que `chemin.has_value()` est vrai.
- Vérifie que `chemin->steps` vaut `reference->steps`.
- Vérifie que `chemin->cost` vaut `reference->cost`.

### PathfindingTest.LeCheminSuitLaDroite

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:308`

Sur une grille ouverte, le chemin retenu parmi ceux de meme cout est le plus proche de la droite depart-arrivee, et reste dans la boite qui les englobe.

**Étapes**

1. Grille ouverte 11x11, heros au centre (5,5).
2. Demander le chemin vers (9,3), en haut a droite, par l'aire et par A*.
3. Verifier toute destination de l'aire.

**Résultat attendu**

- Vérifie que `hautDroite.has_value()` est vrai.
- Vérifie que `hautDroite->steps` vaut `(std::vector<core::GridPosition>{{6, 4}, {7, 4}, {8, 3}, {9, 3}})`.
- Vérifie que `core::findPath(grille, {.combatant = HEROS}, {9, 3})->steps` vaut `hautDroite->steps`.
- Vérifie que `chemin.has_value()` est vrai.
- Vérifie que `pas.column` est supérieur ou égal à `colonneMin`.
- Vérifie que `pas.column` est inférieur ou égal à `colonneMax`.
- Vérifie que `pas.row` est supérieur ou égal à `ligneMin`.
- Vérifie que `pas.row` est inférieur ou égal à `ligneMax`.

### PathfindingTest.LAireEtAStarRendentLeMemeChemin

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:348`

findPath et ReachableArea::pathTo rendent le meme chemin.

**Étapes**

1. Grille accidentee (mur en L, boue).
2. Calculer l'aire pour un budget de 12.
3. Pour chaque destination, comparer pathTo a findPath.

**Résultat attendu**

- Vérifie que `aire.destinations().empty()` est faux.
- Vérifie que `parAire.has_value()` est vrai.
- Vérifie que `parAStar.has_value()` est vrai.
- Vérifie que `parAire->steps` vaut `parAStar->steps`.
- Vérifie que `parAire->cost` vaut `parAStar->cost`.
- Vérifie que `aire.costTo(destination)` vaut `parAire->cost`.
- Vérifie que `tchebychev(pas, precedente)` vaut `1`.

### PathfindingTest.UnCheminImpossibleEstRefuse

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:382`

Un chemin impossible est refuse.

**Étapes**

1. Grille coupee en deux par un mur plein.
2. Demander un chemin de l'autre cote, un chemin vers un mur, un chemin pour un combattant absent.

**Résultat attendu**

- Vérifie que `grille.place(HEROS, {0, 2})` vaut `core::PlacementResult::Placed`.
- Vérifie que `core::findPath(grille, {.combatant = HEROS}, {4, 2}).has_value()` est faux.
- Vérifie que `core::findPath(grille, {.combatant = HEROS}, {2, 2}).has_value()` est faux.
- Vérifie que `core::findPath(grille, {.combatant = ENNEMI}, {1, 2}).has_value()` est faux.
- Vérifie que `aire.pathTo({0, 2}).has_value()` est faux.
- Vérifie que `aire.pathTo({4, 2}).has_value()` est faux.
- Vérifie que `aire.destinations().size()` vaut `9U`.
- Vérifie que `core::ReachableArea(grille, {.combatant = ENNEMI}, 20).destinations().empty()` est vrai.

### PathfindingTest.UnVolantSurvoleLesObstaclesDeSol

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:411`

Un volant survole les obstacles de sol, pas les murs.

**Étapes**

1. Couloir 5x3 : colonne 2 en eau profonde, case (1,1) en boue, ligne 0 murée.
2. Calculer l'aire du meme combattant au sol, puis en vol.

**Résultat attendu**

- Vérifie que `grille.place(HEROS, {0, 1})` vaut `core::PlacementResult::Placed`.
- Vérifie que `marche.canEndAt({3, 1})` est faux.
- Vérifie que `marche.costTo({1, 1})` vaut `2`.
- Vérifie que `vol.canEndAt({3, 1})` est vrai.
- Vérifie que `vol.costTo({1, 1})` vaut `1`.
- Vérifie que `vol.canEndAt({2, 1})` est vrai.
- Vérifie que `vol.costTo({1, 0}).has_value()` est faux.

### PathfindingTest.UneGrandeCreatureNePassePasParUnCouloirEtroit

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:446`

Une creature 2x2 ne passe pas par un couloir d'une case.

**Étapes**

1. Salle 6x6 coupee par un mur en colonne 3, percee d'une seule case en (3,2).
2. Calculer l'aire d'un combattant 1x1, puis d'un 2x2, au budget 10.

**Résultat attendu**

- Vérifie que `grille.place(HEROS, {0, 0})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.place(ENNEMI, {0, 3}, 2)` vaut `core::PlacementResult::Placed`.
- Vérifie que `core::ReachableArea(grille, {.combatant = HEROS}, 10).canEndAt({5, 2})` est vrai.
- Vérifie que `ours.destinations().empty()` est faux.
- Vérifie que `place.column` est strictement inférieur à `3`.
- Vérifie que `grille.canStand(place, 2, ENNEMI)` est vrai.

### PathfindingTest.SurDesCartesAleatoiresAStarEtLAireSAccordent

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_pathfinding.cpp:479`

Sur deux cents cartes aleatoires a graine fixe, A* et l'aire s'accordent.

**Étapes**

1. Tirer deux cents grilles 12x9 (graine fixe) : murs, eau profonde, boue, un allie a traverser.
2. Pour chaque destination de l'aire au budget 14, comparer pathTo et findPath.

**Résultat attendu**

- Vérifie que `grille.place(HEROS, {1, 1})` vaut `core::PlacementResult::Placed`.
- Vérifie que `grille.place(ALLIE, {2, 1})` vaut `core::PlacementResult::Placed`.
- Vérifie que `parAire.has_value()` est vrai.
- Vérifie que `parAStar.has_value()` est vrai.
- Vérifie que `parAire->steps` vaut `parAStar->steps`.
- Vérifie que `parAire->cost` vaut `parAStar->cost`.
- Vérifie que `comparaisons` est strictement supérieur à `2000`.

## test_tactical_terrain.cpp

### TacticalTerrainTest.UneRencontreEnChampOuvertEstValide

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:61`

Une rencontre en champ ouvert est un terrain tactique valide.

**Étapes**

1. Poser une rencontre de deux rats au centre d'une carte vide de 20 x 20.
2. Analyser le terrain.

**Résultat attendu**

- Vérifie que `verdicts.size()` vaut `1U`.
- Vérifie que `verdict.valid()` est vrai.
- Vérifie que `verdict.entityIndex` vaut `0U`.
- Vérifie que `verdict.encounterId` vaut `"rats"`.
- Vérifie que `verdict.placements.size()` vaut `2U`.
- Vérifie que `verdict.requiredCells` vaut `(2 + core::TACTICAL_PARTY_SIZE) * 4`.
- Vérifie que `verdict.area.size()` vaut `13U * 13U`.
- Vérifie que `verdict.area.front()` vaut `(core::GridPosition{.column = 4, .row = 4})`.
- Vérifie que `verdict.area.back()` vaut `(core::GridPosition{.column = 16, .row = 16})`.

### TacticalTerrainTest.UnCombattantDansUnMurEstSignale

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:90`

Un combattant pose dans un mur est signale.

**Étapes**

1. Murer la case devant le declencheur.
2. Analyser le terrain.

**Résultat attendu**

- Vérifie que `verdicts.size()` vaut `1U`.
- Vérifie que `verdicts.front().issues` vaut `attendus`.

### TacticalTerrainTest.UnCombattantHorsDeLaCarteEstSignale

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:114`

Un combattant hors de la carte est signale.

**Étapes**

1. Poser le declencheur sur la premiere ligne : le premier rat tombe en ligne -1.
2. Analyser le terrain.

**Résultat attendu**

- Vérifie que `verdicts.size()` vaut `1U`.
- Vérifie que `verdicts.front().issues` vaut `attendus`.

### TacticalTerrainTest.DeuxCombattantsSuperposesSontSignales

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:137`

Deux combattants superposes sont signales.

**Étapes**

1. Ecrire une rencontre dont deux combattants partagent un decalage.
2. Analyser le terrain.

**Résultat attendu**

- Vérifie que `verdicts.size()` vaut `1U`.
- Vérifie que `verdicts.front().issues` vaut `attendus`.

### TacticalTerrainTest.UnCouloirTropEtroitEstSignale

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:165`

Un couloir trop etroit est signale.

**Étapes**

1. Murer une carte de 30 x 3 sauf sa ligne du milieu.
2. Y poser une rencontre de deux combattants alignes.
3. Analyser le terrain.

**Résultat attendu**

- Vérifie que `verdicts.size()` vaut `1U`.
- Vérifie que `verdict.area.size()` vaut `13U`.
- Vérifie que `verdict.requiredCells` vaut `24`.
- Vérifie que `verdict.issues` vaut `attendus`.

### TacticalTerrainTest.UnDeclencheurDansUnMurNAAucuneZone

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:200`

Un declencheur dans un mur n'a aucune zone.

**Étapes**

1. Murer la case du declencheur d'un champ ouvert.
2. Analyser le terrain.

**Résultat attendu**

- Vérifie que `verdicts.size()` vaut `1U`.
- Vérifie que `verdicts.front().area.empty()` est vrai.
- Vérifie que `verdicts.front().issues.empty()` est faux.
- Vérifie que `verdicts.front().issues.back().code` vaut `core::TacticalIssueCode::AreaTooNarrow`.

### TacticalTerrainTest.UneRencontreInconnueEstIgnoree

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:222`

Une rencontre inconnue est ignoree.

**Étapes**

1. Poser une rencontre inconnue, puis une connue.
2. Analyser le terrain.

**Résultat attendu**

- Vérifie que `verdicts.size()` vaut `1U`.
- Vérifie que `verdicts.front().entityIndex` vaut `1U`.
- Vérifie que `verdicts.front().encounterId` vaut `"rats"`.

### TacticalTerrainTest.LesAutresEntitesSontIgnorees

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:244`

Les entites qui ne sont pas des rencontres sont ignorees.

**Étapes**

1. Poser un coffre portant un encounterId, et une rencontre sans encounterId, tous deux dans un mur.
2. Analyser le terrain.

**Résultat attendu**

- Vérifie que `core::analyzeEncounterTerrain(collision, entites, catalogue()).empty()` est vrai.

### TacticalTerrainTest.LEmpriseDUneGrandeCreatureVientDuBestiaire

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:268`

L'emprise d'une grande creature vient du bestiaire.

**Étapes**

1. Poser un ogre (taille G) dont seule la case diagonale de son emprise est muree.
2. Analyser sans bestiaire, puis avec.

**Résultat attendu**

- Vérifie que `sansBestiaire.size()` vaut `1U`.
- Vérifie que `sansBestiaire.front().valid()` est vrai.
- Vérifie que `avecBestiaire.size()` vaut `1U`.
- Vérifie que `avecBestiaire.front().issues` vaut `attendus`.

### TacticalTerrainTest.LAnalyseEstDeterministe

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_tactical_terrain.cpp:306`

L'analyse est deterministe.

**Étapes**

1. Poser trois rencontres, dont une mal posee.
2. Analyser deux fois.

**Résultat attendu**

- Vérifie que `premier.size()` vaut `3U`.
- Vérifie que `second.size()` vaut `3U`.
- Vérifie que `premier[i].entityIndex` vaut `i`.
- Vérifie que `premier[i].area` vaut `second[i].area`.
- Vérifie que `premier[i].issues` vaut `second[i].issues`.
- Vérifie que `premier[0].valid()` est vrai.
- Vérifie que `premier[1].valid()` est faux.
- Vérifie que `premier[2].valid()` est faux.

## test_turn_order.cpp

### TurnOrderTest.LOrdreNeDependPasDeLInsertion

*Bloquant · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_turn_order.cpp:69`

L'ordre d'initiative ne depend pas de l'ordre dans lequel on range les combattants.

**Étapes**

1. Cinq places qui ne se distinguent chacune de la suivante que par un critere : total, modificateur, Dexterite, camp, identifiant.
2. Les ranger dans les 120 ordres possibles.

**Résultat attendu**

- Vérifie que `ordre.add(place)` est vrai.
- Vérifie que `ordreDe(ordre)` vaut `attendu`.
- Vérifie que `permutations` vaut `120`.
- Vérifie que `ordre.add(places[0])` est vrai.
- Vérifie que `ordre.add(places[0])` est faux.
- Vérifie que `ordre.entries().size()` vaut `1U`.

### TurnOrderTest.UnRepereFixePerdLesEgalites

*Majeur · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_turn_order.cpp:107`

Les actions de repaire au rang 20 jouent apres un combattant a 20, les renforts au rang 0 en dernier.

**Étapes**

1. Ranger un combattant a 20, un a 12 et un a 0, un repere « repaire » a 20 et un « renforts » a 0.
2. Parcourir le round de place en place.
3. Ranger deux fois le meme repere.

**Résultat attendu**

- Vérifie que `ordre.addMarker({.count = 0, .name = "renforts"})` est vrai.
- Vérifie que `ordre.addMarker({.count = 20, .name = "repaire"})` est vrai.
- Vérifie que `ordre.add(combattant(1, 0))` est vrai.
- Vérifie que `ordre.add(combattant(2, 12))` est vrai.
- Vérifie que `ordre.add(combattant(3, 20))` est vrai.
- Vérifie que `ordre.addMarker({.count = 20, .name = "repaire"})` est faux.
- Vérifie que `parcours` vaut `(std::vector<std::string>{"C3", "repaire", "C2", "C1", "renforts"})`.

### TurnOrderTest.UnePlaceSeCalculeApresUnDepart

*Critique · Unitaire · Combat* — `Source/Test/Unit/Core/Combat/test_turn_order.cpp:139`

Un combattant qui sort ne fait sauter aucun tour ; un renfort joue ce round-ci s'il est range apres la place en cours.

**Étapes**

1. Ordre 18, 12, 6 ; la place en cours est celle de 12.
2. Retirer le combattant a 12.
3. Ranger un renfort a 9, puis un a 15.

**Résultat attendu**

- Vérifie que `ordre.add(combattant(1, 18))` est vrai.
- Vérifie que `ordre.add(combattant(2, 12))` est vrai.
- Vérifie que `ordre.add(combattant(3, 6))` est vrai.
- Vérifie que `enCours.has_value()` est vrai.
- Vérifie que `enCours->combatant()` vaut `core::CombatantId{2}`.
- Vérifie que `ordre.remove(core::CombatantId{2})` est vrai.
- Vérifie que `ordre.remove(core::CombatantId{2})` est faux.
- Vérifie que `ordre.contains(core::CombatantId{2})` est faux.
- Vérifie que `ordre.add(combattant(4, 9))` est vrai.
- Vérifie que `ordre.add(combattant(5, 15))` est vrai.
- Vérifie que `suivante.has_value()` est vrai.
- Vérifie que `suivante->combatant()` vaut `core::CombatantId{4}`.
- Vérifie que `suivante.has_value()` est vrai.
- Vérifie que `suivante->combatant()` vaut `core::CombatantId{3}`.
- Vérifie que `ordre.slotAfter(*suivante).has_value()` est faux.
- Vérifie que `ordre.firstSlot()->combatant()` vaut `core::CombatantId{1}`.
