# Core · World

Tests unitaires — **56 cas** (1 bloquant, 28 critiques, 24 majeurs, 3 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_atlas.cpp`](#test-atlascpp) | 13 | - | 8 | 4 | 1 |
| [`test_city_block.cpp`](#test-city-blockcpp) | 1 | - | - | 1 | - |
| [`test_city_plan.cpp`](#test-city-plancpp) | 3 | - | 1 | 2 | - |
| [`test_combat_zone.cpp`](#test-combat-zonecpp) | 3 | - | 3 | - | - |
| [`test_entity_kinds.cpp`](#test-entity-kindscpp) | 9 | 1 | - | 7 | 1 |
| [`test_entity_presence.cpp`](#test-entity-presencecpp) | 3 | - | 2 | 1 | - |
| [`test_exploration_reach.cpp`](#test-exploration-reachcpp) | 3 | - | 2 | 1 | - |
| [`test_exploration_session.cpp`](#test-exploration-sessioncpp) | 4 | - | 3 | 1 | - |
| [`test_world_graph.cpp`](#test-world-graphcpp) | 10 | - | 5 | 4 | 1 |
| [`test_world_travel.cpp`](#test-world-travelcpp) | 7 | - | 4 | 3 | - |

## test_atlas.cpp

### AtlasTest.LesTreizeRegionsEtLeursLieuxChargentSansErreur

*Critique · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:57`

Les treize regions et leurs cent sept lieux se chargent tous.

**Étapes**

1. Charger Source/Elements/World.
2. Compter les regions, les lieux et les erreurs rapportees.

**Résultat attendu**

- Vérifie que `atlas().errors.empty()` est vrai.
- Vérifie que `atlas().regions.size()` vaut `REGIONS_DU_LIVRE`.
- Vérifie que `atlas().locations.size()` vaut `LIEUX_DU_LIVRE`.

### AtlasTest.ChaqueRegionPorteSesSeptAxes

*Critique · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:74`

Chaque region porte les sept statistiques regionales.

**Étapes**

1. Pour chaque region, lire les sept axes de l'encart.

**Résultat attendu**

- Vérifie que `axe.appraisals.empty()` est faux.

### AtlasTest.LEncartDeLEmpireCentralEstRejoueDepuisLeLivre

*Critique · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:96`

L'encart de l'Empire central concorde avec le Sourcebook.

**Étapes**

1. Comparer les sept notes de l'Empire central aux valeurs lues sur la page imprimee 90.
2. Comparer l'effectif et la part de la premiere espece au bloc d'introduction.

**Résultat attendu**

- Vérifie que `empire` diffère de `nullptr`.
- Vérifie que `empire->statistic(note.axis).grade()` vaut `note.grade`.
- Vérifie que `empire->statistic(note.axis).varies()` est faux.
- Vérifie que `empire->population.total.has_value()` est vrai.
- Vérifie que `*empire->population.total` vaut `2300000`.
- Vérifie que `empire->population.species.empty()` est faux.
- Vérifie que `empire->population.species.front().species` vaut `"human"`.
- Vérifie que `empire->population.species.front().percent` vaut `84`.

### AtlasTest.UneRegionNonUniformePorteSesDeuxAppreciations

*Critique · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:136`

Une region non uniforme porte ses deux appreciations avec leur portee.

**Étapes**

1. Lire la liberte civique du Benenet imperial et la criminalite des Domaines straviens.
2. Verifier les deux notes et les deux portees de chacune.

**Résultat attendu**

- Vérifie que `benenet` diffère de `nullptr`.
- Vérifie que `liberte.appraisals.size()` vaut `2U`.
- Vérifie que `liberte.varies()` est vrai.
- Vérifie que `liberte.appraisals[0].grade` vaut `core::RegionGrade::Low`.
- Vérifie que `liberte.appraisals[0].scope` vaut `"south"`.
- Vérifie que `liberte.appraisals[1].grade` vaut `core::RegionGrade::High`.
- Vérifie que `liberte.appraisals[1].scope` vaut `"north"`.
- Vérifie que `stravian` diffère de `nullptr`.
- Vérifie que `crime.appraisals.size()` vaut `2U`.
- Vérifie que `crime.appraisals[0].grade` vaut `core::RegionGrade::Low`.
- Vérifie que `crime.appraisals[0].scope` vaut `"underground"`.
- Vérifie que `crime.appraisals[1].grade` vaut `core::RegionGrade::High`.
- Vérifie que `crime.appraisals[1].scope` vaut `"surface"`.

### AtlasTest.UneRegionUniformeNePorteAucunePortee

*Mineur · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:173`

Une region uniforme ne nomme aucune portee.

**Étapes**

1. Pour chaque axe ne portant qu'une appreciation, lire sa portee.

**Résultat attendu**

- Vérifie que `axe.appraisals.front().scope.empty()` est vrai.

### AtlasTest.LaCapitaleEtSesDouzeQuartiersSontRejouesDepuisLeLivre

*Majeur · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:195`

La Capitale et ses douze quartiers sont des lieux de l'Empire central.

**Étapes**

1. Resoudre la Capitale et chacun des douze quartiers, noms recopies du PDF.
2. Verifier leur region et une phrase de Martpart et d'Arenarea lue sur la page.

**Résultat attendu**

- Vérifie que `ville` diffère de `nullptr`.
- Vérifie que `ville->region` vaut `"central-empire"`.
- Vérifie que `ville->description.find("Approximately 680,000")` diffère de `std::string::npos`.
- Vérifie que `lieu` diffère de `nullptr`.
- Vérifie que `lieu->region` vaut `"central-empire"`.
- Vérifie que `lieu->description.empty()` est faux.
- Vérifie que `atlas().findLocation(capitale + "-martpart")->description.find( "Lantern-lit stalls and culturally blended architecture adorn cobblestone")` diffère de `std::string::npos`.
- Vérifie que `atlas().findLocation(capitale + "-arenarea")->description.find( "Famed for the Arena of Fate")` diffère de `std::string::npos`.

### AtlasTest.AucunLieuSansRegion

*Critique · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:233`

Aucun lieu n'est orphelin de region.

**Étapes**

1. Pour chaque lieu, resoudre la region qu'il declare.
2. Verifier que cette region le cite en retour.

**Résultat attendu**

- Vérifie que `region` diffère de `nullptr`.
- Vérifie que `std::ranges::find(region->locations, lieu.id)` diffère de `region->locations.end()`.

### AtlasTest.ToutLieuCiteParUneRegionExiste

*Majeur · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:252`

Toute region ne cite que des lieux existants.

**Étapes**

1. Pour chaque region, resoudre chacun des lieux qu'elle cite.

**Résultat attendu**

- Vérifie que `atlas().findLocation(identifiant)` diffère de `nullptr`.

### AtlasTest.LeVoisinageEstSymetriqueEtLeGrapheConnexe

*Critique · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:270`

Le voisinage des regions est symetrique et le graphe connexe.

**Étapes**

1. Pour chaque arete, verifier que la region voisine declare la reciproque.
2. Parcourir le graphe depuis la premiere region.

**Résultat attendu**

- Vérifie que `autre` diffère de `nullptr`.
- Vérifie que `std::ranges::find(autre->neighbors, region.id)` diffère de `autre->neighbors.end()`.
- Vérifie que `atlas().regions.empty()` est faux.
- Vérifie que `atlas().unreachableFrom(atlas().regions.front().id).empty()` est vrai.

### AtlasTest.UneRegionCoupeeDuGrapheEstSignalee

*Critique · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:295`

Le controle de connexite sait echouer.

**Étapes**

1. Retirer les aretes de Yama, dans une copie de l'atlas.
2. Relancer le parcours depuis une autre region.

**Résultat attendu**

- Vérifie que `ampute.regions.size()` est supérieur ou égal à `2U`.
- Vérifie que `cible` diffère de `nullptr`.
- Vérifie que `depart` diffère de `isolee`.
- Vérifie que `std::ranges::find(perdues, isolee)` diffère de `perdues.end()`.

### AtlasTest.LesNotesDuMoteurCoincidentAvecCellesDuSchema

*Critique · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:334`

Les notes et les axes du moteur coincident avec region.schema.json.

**Étapes**

1. Lire le schema LIVRE.
2. Y chercher les cinq notes et les sept axes que le moteur nomme.
3. Verifier qu'une note hors des cinq est refusee.

**Résultat attendu**

- Vérifie que `flux.is_open()` est vrai.
- Vérifie que `schema.find('"' + std::string{nom} + '"')` diffère de `std::string::npos`.
- Vérifie que `core::regionGradeFromName(nom)` vaut `std::optional<core::RegionGrade>{static_cast<core::RegionGrade>(rang)}`.
- Vérifie que `core::regionGradeFromName("average").has_value()` est faux.
- Vérifie que `schema.find('"' + std::string{nom} + '"')` diffère de `std::string::npos`.

### AtlasTest.UnDossierAbsentEstUneErreurPasUnMondeVide

*Majeur · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:372`

Un dossier d'atlas absent produit une erreur, pas un monde vide.

**Étapes**

1. Charger un dossier qui n'existe pas.

**Résultat attendu**

- Vérifie que `absent.regions.empty()` est vrai.
- Vérifie que `absent.errors.empty()` est faux.

### AtlasTest.LesPartsDePopulationRestentPlausibles

*Majeur · Unitaire · Atlas* — `Source/Test/Unit/Core/World/test_atlas.cpp:388`

Les parts de population de chaque region somment a 100 % a l'arrondi pres.

**Étapes**

1. Pour chaque region, sommer les parts d'espece et la part << others >>.

**Résultat attendu**

- Vérifie que `part.percent` est strictement supérieur à `0`.
- Vérifie que `part.species.empty()` est faux.
- Vérifie que `somme` est supérieur ou égal à `97`.
- Vérifie que `somme` est inférieur ou égal à `103`.

## test_city_block.cpp

### CityBlockTest.LesIlotsSeLisentEtLesSaisiesFautivesSontEcartees

*Majeur · Unitaire · Plan de la ville* — `Source/Test/Unit/Core/World/test_city_block.cpp:38`

Un ilot est un rectangle nomme de la carte ; un ilot sans nom ou vide est ecarte.

**Étapes**

1. Poser sur une carte deux ilots valides, un sans nom, un de largeur nulle, et une entite d'un autre type.
2. Lire les ilots ; demander a chacun s'il contient quelques cases.

**Résultat attendu**

- Vérifie que `ilots.size()` vaut `2U`.
- Vérifie que `ilots[0].name` vaut `"place"`.
- Vérifie que `ilots[1].name` vaut `"ruelles"`.
- Vérifie que `ilots[0].columns` vaut `5`.
- Vérifie que `ilots[0].rows` vaut `4`.
- Vérifie que `ilots[0].contains({2, 3})` est vrai.
- Vérifie que `ilots[0].contains({6, 6})` est vrai.
- Vérifie que `ilots[0].contains({7, 3})` est faux.
- Vérifie que `ilots[0].contains({2, 7})` est faux.

## test_city_plan.cpp

### CityPlanTest.UneVilleSeLit

*Critique · Unitaire · Graphe de la ville* — `Source/Test/Unit/Core/World/test_city_plan.cpp:51`

Une ville se lit, et sa porte de depart s'ouvre.

**Étapes**

1. Lire la ville de la racine d'essai.
2. Entrer a sa porte de depart par le chargeur du jeu.

**Résultat attendu**

- Vérifie que `lue.ok()` est vrai.
- Vérifie que `capitale.location` vaut `"test-city"`.
- Vérifie que `capitale.districts.size()` vaut `6U`.
- Vérifie que `cartes` vaut `2`.
- Vérifie que `gardees` vaut `4`.
- Vérifie que `capitale.startMap()` vaut `"bourg/place"`.
- Vérifie que `capitale.startArrival` vaut `"porte-est"`.
- Vérifie que `cave` diffère de `nullptr`.
- Vérifie que `cave->id` vaut `"test-city-cave"`.
- Vérifie que `capitale.districtOfMap("donjon")` vaut `nullptr`.
- Vérifie que `voyage.enter(capitale.startMap(), capitale.startArrival)` vaut `core::TravelResult::Moved`.

### CityPlanFileTest.UnQuartierDoitAvoirSoitUneCarteSoitUnePorteGardee

*Majeur · Unitaire · Graphe de la ville* — `Source/Test/Unit/Core/World/test_city_plan.cpp:88`

Un quartier doit avoir soit une carte, soit une porte gardee.

**Étapes**

1. Lire une ville dont un quartier porte `map` et `guard`, puis une autre dont un quartier ne porte ni l'un ni l'autre.

**Résultat attendu**

- Vérifie que `lesDeux.ok()` est faux.
- Vérifie que `lesDeux.error.find("« a »")` diffère de `std::string::npos`.
- Vérifie que `aucun.ok()` est faux.
- Vérifie que `aucun.error.find("« b »")` diffère de `std::string::npos`.

### CityPlanFileTest.UnDepartDansUnQuartierSansCarteEstRefuse

*Majeur · Unitaire · Graphe de la ville* — `Source/Test/Unit/Core/World/test_city_plan.cpp:114`

« Nouvelle partie » doit pouvoir poser le heros : le depart a une carte.

**Étapes**

1. Lire une ville dont le quartier de depart est ferme par une porte gardee.
2. Lire un fichier absent.

**Résultat attendu**

- Vérifie que `ferme.ok()` est faux.
- Vérifie que `ferme.plan.startMap().empty()` est vrai.
- `EXPECT_NO_THROW(absent = core::loadCityPlan(dir / "inexistant.json"))`
- Vérifie que `absent.ok()` est faux.

## test_combat_zone.cpp

### CombatZoneTest.LaCarteReduiteNEstQueLaZone

*Critique · Unitaire · Zone de combat* — `Source/Test/Unit/Core/World/test_combat_zone.cpp:66`

La carte reduite a la zone ne porte que la zone, entites translatees.

**Étapes**

1. Declarer une zone « sable » de 4 x 3 en (4, 4) sur une carte de 12 x 10.
2. Reduire la carte a la zone.

**Résultat attendu**

- Vérifie que `zones.size()` vaut `1U`.
- Vérifie que `zones.front()` vaut `(core::CombatZone{.name = "sable", .origin = {4, 4}, .columns = 4, .rows = 3})`.
- Vérifie que `zones.front().contains({4, 4})` est vrai.
- Vérifie que `zones.front().contains({7, 6})` est vrai.
- Vérifie que `zones.front().contains({8, 6})` est faux.
- Vérifie que `core::findCombatZone(zones, "sable")` vaut `&zones.front()`.
- Vérifie que `core::findCombatZone(zones, "")` vaut `&zones.front()`.
- Vérifie que `core::findCombatZone(zones, "tribunes")` vaut `nullptr`.
- Vérifie que `reduite.tileMap().width()` vaut `4`.
- Vérifie que `reduite.tileMap().height()` vaut `3`.
- Vérifie que `reduite.entities().size()` vaut `3U`.
- Vérifie que `entite.position` vaut `(core::GridPosition{1, 1})`.
- Vérifie que `entite.position` vaut `(core::GridPosition{0, 2})`.
- Vérifie que `pnj` vaut `1`.
- Vérifie que `reduite.layers().size()` vaut `1U`.
- Vérifie que `reduite.layers().front().tiles.width()` vaut `4`.
- Vérifie que `reduite.layers().front().tiles.tile(0, 0)` vaut `core::TileType::Wall`.
- Vérifie que `reduite.layers().front().pieceAt(1, 1)` vaut `"torch-left"`.
- Vérifie que `reduite.layers().front().pieces.size()` vaut `12U`.
- Vérifie que `reduite.forcedCollision()` vaut `(std::vector<core::GridPosition>{{2, 1}})`.

### CombatZoneTest.LaSessionIgnoreLesCasesHorsZone

*Critique · Unitaire · Zone de combat* — `Source/Test/Unit/Core/World/test_combat_zone.cpp:127`

La grille de combat de la carte reduite ne connait que la zone.

**Étapes**

1. Monter une `core::ArenaSession` sur la carte reduite a la zone.

**Résultat attendu**

- Vérifie que `zones.size()` vaut `1U`.
- Vérifie que `session.combat().grid().width()` vaut `4`.
- Vérifie que `session.combat().grid().height()` vaut `3`.
- Vérifie que `session.combat().grid().inBounds({4, 0})` est faux.

### CombatZoneTest.ChaqueDefautDeZoneEstReleveAuChargement

*Critique · Unitaire · Zone de combat* — `Source/Test/Unit/Core/World/test_combat_zone.cpp:149`

Une zone degeneree, debordante ou entierement pleine est refusee, avec son code.

**Étapes**

1. Declarer une zone de largeur nulle, une zone qui sort de la carte, une zone entierement muree.
2. Valider la carte.

**Résultat attendu**

- Vérifie que `core::validateCombatZones("colisee", degeneree).size()` vaut `1U`.
- Vérifie que `core::validateCombatZones("colisee", degeneree).front().code` vaut `WorldIssueCode::CombatZoneDegenerate`.
- Vérifie que `core::validateCombatZones("colisee", debordante).size()` vaut `1U`.
- Vérifie que `core::validateCombatZones("colisee", debordante).front().code` vaut `WorldIssueCode::CombatZoneOutOfBounds`.
- Vérifie que `core::validateCombatZones("colisee", muree).size()` vaut `1U`.
- Vérifie que `defaut.code` vaut `WorldIssueCode::CombatZoneBlocked`.
- Vérifie que `defaut.value` vaut `"mur"`.
- Vérifie que `defaut.position` vaut `(core::GridPosition{2, 0})`.
- Vérifie que `core::validateWorldMap("colisee", muree).empty()` est faux.
- Vérifie que `core::validateWorldMap("colisee", carte({zone("sable", {4, 4}, 4, 3)})).empty()` est vrai.

## test_entity_kinds.cpp

### FamillesDEntitesTest.LaTableReprendLesTypesDuGameplay

*Majeur · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:62`

La table des familles reprend les types du gameplay.

**Étapes**

1. Chercher chaque type lu par le gameplay dans la table.

**Résultat attendu**

- Vérifie que `core::findEntityKind(type)` diffère de `nullptr`.
- Vérifie que `core::findEntityKind("dragon")` vaut `nullptr`.
- Vérifie que `core::findEntityKind(core::NPC_ENTITY_TYPE)` diffère de `nullptr`.
- Vérifie que `core::findEntityKind(core::NPC_ENTITY_TYPE)->find(core::NPC_DIALOGUE_PROPERTY)` diffère de `nullptr`.

### FamillesDEntitesTest.EntiteNeuvePorteSesDefauts

*Mineur · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:85`

Une entite neuve porte ses proprietes par defaut.

**Étapes**

1. Fabriquer une entree d'arene en (3, 4).

**Résultat attendu**

- Vérifie que `kind` diffère de `nullptr`.
- Vérifie que `made.type` vaut `core::ARENA_ENTRY_ENTITY_TYPE`.
- Vérifie que `made.position` vaut `(core::GridPosition{.column = 3, .row = 4})`.
- Vérifie que `made.properties.at(std::string{core::ARENA_SIDE_PROPERTY})` vaut `core::PropertyValue{std::string{"allies"}}`.
- Vérifie que `made.properties.at(std::string{core::ARENA_RANK_PROPERTY})` vaut `core::PropertyValue{std::int64_t{1}}`.

### FamillesDEntitesTest.CarteBienRenseigneeEstMuette

*Majeur · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:106`

Une carte bien renseignee est muette.

**Étapes**

1. Valider un PNJ, une rencontre, un portail vers un point connu, un point d'arrivee et un PNJ figurant.

**Résultat attendu**

- Vérifie que `core::validateMapEntities(entities, context()).empty()` est vrai.

### FamillesDEntitesTest.ReferencesCasseesNommees

*Majeur · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:128`

Les references cassees sont nommees.

**Étapes**

1. Valider un PNJ au dialogue inconnu, une rencontre inconnue, un portail vers une carte inconnue et un portail vers un point absent d'une carte connue.

**Résultat attendu**

- Vérifie que `issues.size()` vaut `4U`.
- Vérifie que `issues[0]` vaut `(core::EntityIssue{.entityIndex = 0, .code = core::EntityIssueCode::UnknownDialogue, .key = "dialogue", .value = "inconnu"})`.
- Vérifie que `issues[1].code` vaut `core::EntityIssueCode::UnknownEncounter`.
- Vérifie que `issues[2]` vaut `(core::EntityIssue{.entityIndex = 2, .code = core::EntityIssueCode::UnknownTargetMap, .key = "targetMap", .value = "nulle-part"})`.
- Vérifie que `issues[3]` vaut `(core::EntityIssue{.entityIndex = 3, .code = core::EntityIssueCode::UnknownArrivalPoint, .key = "arrival", .value = "lisiere"})`.

### FamillesDEntitesTest.ProprietesMalRenseigneesSignalees

*Majeur · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:164`

Les proprietes mal renseignees sont signalees.

**Étapes**

1. Valider une rencontre sans rencontre, un portail a cible vide, une entree d'arene au rang textuel et au camp inconnu, et un type inconnu.

**Résultat attendu**

- Vérifie que `codes(core::validateMapEntities(entities, context()))` vaut `(std::vector<core::EntityIssueCode>{ core::EntityIssueCode::MissingProperty, core::EntityIssueCode::MissingProperty, core::EntityIssueCode::MissingProperty, core::EntityIssueCode::InvalidChoice, core::EntityIssueCode::WrongValueType, core::EntityIssueCode::UnknownType})`.

### FamillesDEntitesTest.NomDePointDArriveeUnique

*Majeur · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:191`

Un nom de point d'arrivee est unique dans la carte.

**Étapes**

1. Poser deux points « puits » et un point « gue ».
2. Valider et relever les noms.

**Résultat attendu**

- Vérifie que `issues.size()` vaut `1U`.
- Vérifie que `issues[0].entityIndex` vaut `2U`.
- Vérifie que `issues[0].code` vaut `core::EntityIssueCode::DuplicateArrivalPoint`.
- Vérifie que `core::arrivalPointNames(entities)` vaut `(std::set<std::string, std::less<>>{"gue", "puits"})`.

### FamillesDEntitesTest.ToutFamilleLueParLeJeuEstDansLaTable

*Bloquant · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:217`

Aucune famille lue par le jeu n'echappe a l'editeur.

**Étapes**

1. Relever dans les sources du jeu (Core, HMI, App) chaque constante <code>*_ENTITY_TYPE</code>.
2. Y ajouter les familles interactives (<code>core::knownInteractableKinds</code>) et chaque type des cartes livrees.
3. Chercher chacune dans la table.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `read.size()` est supérieur ou égal à `11U`.
- Vérifie que `core::findEntityKind(type)` diffère de `nullptr`.

### FamillesDEntitesTest.LaTableEstCoherenteAvecSesFormes

*Majeur · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:274`

La table des familles est coherente avec ses formes.

**Étapes**

1. Parcourir la table.

**Résultat attendu**

- Vérifie que `spec` diffère de `nullptr`.
- Vérifie que `spec->kind` vaut `core::EntityPropertyKind::Integer`.
- Vérifie que `spec->minimum` vaut `1`.
- Vérifie que `kind.find(kind.labelProperty)` diffère de `nullptr`.
- Vérifie que `kind.find(kind.figureProperty)` diffère de `nullptr`.
- Vérifie que `kind.find(kind.figureProperty)->source` vaut `core::EntityChoiceSource::Figures`.

### FamillesDEntitesTest.BornesEtCataloguesControles

*Majeur · Unitaire · Familles d'entites* — `Source/Test/Unit/Core/World/test_entity_kinds.cpp:305`

Le schema type controle bornes et catalogues.

**Étapes**

1. Une zone de combat de largeur 0 ; une entree d'arene de rang 0.
2. Un PNJ a figurine inconnue, gardant un lieu inconnu ; un portail exigeant un drapeau que rien ne pose.
3. Les memes, references connues.

**Résultat attendu**

- Vérifie que `codes(core::validateMapEntities(bounded, references))` vaut `(std::vector<core::EntityIssueCode>{core::EntityIssueCode::OutOfRange, core::EntityIssueCode::OutOfRange})`.
- Vérifie que `codes(core::validateMapEntities(referenced, references))` vaut `(std::vector<core::EntityIssueCode>{core::EntityIssueCode::UnknownFigure, core::EntityIssueCode::UnknownLocation, core::EntityIssueCode::UnsetFlag})`.
- Vérifie que `core::validateMapEntities(referenced, references).empty()` est vrai.

## test_entity_presence.cpp

### EntityPresenceTest.LaConditionDePresenceSeLitSurTroisProprietes

*Critique · Unitaire · Quetes* — `Source/Test/Unit/Core/World/test_entity_presence.cpp:114`

La condition de presence se lit sur trois proprietes.

**Étapes**

1. Lire une entite sans condition, puis avec drapeau seul, drapeau et valeurs, test explicite.
2. Lire des formes fautives : test inconnu, `equals` sans valeur, valeurs sans drapeau, drapeau non textuel.

**Résultat attendu**

- Vérifie que `core::presenceConditionOf(libre).condition.has_value()` est faux.
- Vérifie que `core::isEntityPresent(libre, drapeaux)` est vrai.
- Vérifie que `seul.condition.has_value()` est vrai.
- Vérifie que `seul.condition->test` vaut `core::FlagTest::IsSet`.
- Vérifie que `parmi.condition.has_value()` est vrai.
- Vérifie que `parmi.condition->test` vaut `core::FlagTest::Equals`.
- Vérifie que `parmi.condition->values` vaut `(std::vector<std::string>{"acceptee", "persuasion-echouee"})`.
- Vérifie que `sauf.condition.has_value()` est vrai.
- Vérifie que `sauf.condition->test` vaut `core::FlagTest::NotEquals`.
- Vérifie que `core::presenceConditionOf(garde({1, 1}, presence("f", "parmi", "a"))).issue` vaut `core::PresenceIssue::UnknownTest`.
- Vérifie que `core::presenceConditionOf(garde({1, 1}, presence("f", "equals", ""))).issue` vaut `core::PresenceIssue::MissingValue`.
- Vérifie que `core::presenceConditionOf(garde({1, 1}, sansDrapeau)).issue` vaut `core::PresenceIssue::MissingFlag`.
- Vérifie que `core::presenceConditionOf(fautive).issue` vaut `core::PresenceIssue::WrongValueType`.
- Vérifie que `core::isEntityPresent(fautive, drapeaux)` est vrai.

### EntityPresenceTest.UnPnjConditionneParaitEtDisparaitSansRechargerLaCarte

*Critique · Unitaire · Quetes* — `Source/Test/Unit/Core/World/test_entity_presence.cpp:164`

Un PNJ conditionne parait et disparait sans recharger la carte.

**Étapes**

1. Poser sur une carte un garde present sous `quete.pommes == acceptee`, en (5, 4).
2. Donner la quete a la session ; y entrer, heros en (4, 4) ; lui parler.
3. Poser `acceptee`, faire un pas, lui parler.
4. Poser `enfant-libere`, faire un pas, lui parler.

**Résultat attendu**

- Vérifie que `session.start("parvis", "")` est vrai.
- Vérifie que `session.interactables().empty()` est vrai.
- Vérifie que `parlerADroite(session).empty()` est vrai.
- Vérifie que `session.flags().setValue("quete.pommes", "acceptee")` est vrai.
- Vérifie que `pas.size()` vaut `1U`.
- Vérifie que `pas.front().kind` vaut `ExplorationEventKind::QuestAdvanced`.
- Vérifie que `pas.front().value` vaut `"pommes/acceptee"`.
- Vérifie que `session.interactables().size()` vaut `1U`.
- Vérifie que `parole.size()` vaut `1U`.
- Vérifie que `parole.front().kind` vaut `ExplorationEventKind::Dialogue`.
- Vérifie que `parole.front().value` vaut `"garde"`.
- Vérifie que `session.flags().setValue("quete.pommes", "enfant-libere")` est vrai.
- Vérifie que `session.update(core::ExplorationIntent{}, 1.0F / 60.0F).empty()` est vrai.
- Vérifie que `session.interactables().empty()` est vrai.
- Vérifie que `parlerADroite(session).empty()` est vrai.
- Vérifie que `disque.lectures` vaut `1`.

### EntityPresenceTest.LeControleReleveUnePresenceSurUnDrapeauJamaisPose

*Majeur · Unitaire · Quetes* — `Source/Test/Unit/Core/World/test_entity_presence.cpp:208`

Le controle releve une presence sur un drapeau jamais pose.

**Étapes**

1. Valider trois PNJ : l'un present sous un drapeau pose par un dialogue, l'autre sous un drapeau que rien ne pose, le dernier au test inconnu.

**Résultat attendu**

- Vérifie que `problemes.size()` vaut `2U`.
- Vérifie que `problemes[0]` vaut `(core::EntityIssue{.entityIndex = 1, .code = core::EntityIssueCode::UnsetFlag, .key = std::string{core::PRESENCE_FLAG_PROPERTY}, .value = "jamais-pose"})`.
- Vérifie que `problemes[1].entityIndex` vaut `2U`.
- Vérifie que `problemes[1].code` vaut `core::EntityIssueCode::InvalidPresence`.
- Vérifie que `problemes[1].key` vaut `core::PRESENCE_TEST_PROPERTY`.

## test_exploration_reach.cpp

### ExplorationReachTest.UnCouloirSePasseUnMurLeFerme

*Critique · Unitaire · Atteignabilité* — `Source/Test/Unit/Core/World/test_exploration_reach.cpp:40`

Le héros suit un couloir d'une case, jusqu'au mur.

**Étapes**

1. Une salle, un couloir d'une case, une salle derrière un mur.
2. Parcourir depuis la première salle.

**Résultat attendu**

- Vérifie que `atteinte.reaches({.column = 5, .row = 3})` est vrai.
- Vérifie que `atteinte.reaches({.column = 3, .row = 2})` est vrai.
- Vérifie que `atteinte.reaches({.column = 0, .row = 4})` est faux.
- Vérifie que `atteinte.reaches({.column = 2, .row = 0})` est faux.
- Vérifie que `atteinte.reaches({.column = 9, .row = 9})` est faux.

### ExplorationReachTest.DeuxMursEnDiagonaleFermentLePassage

*Critique · Unitaire · Atteignabilité* — `Source/Test/Unit/Core/World/test_exploration_reach.cpp:67`

On ne passe pas entre deux murs en diagonale.

**Étapes**

1. Deux cases libres qui ne se touchent que par un coin, entre deux murs.

**Résultat attendu**

- Vérifie que `atteinte.reaches({.column = 0, .row = 0})` est vrai.
- Vérifie que `atteinte.reaches({.column = 1, .row = 1})` est faux.
- Vérifie que `atteinte.count()` vaut `1U`.

### ExplorationReachTest.UnDepartMureNAtteintRien

*Majeur · Unitaire · Atteignabilité* — `Source/Test/Unit/Core/World/test_exploration_reach.cpp:88`

Un départ muré n'atteint rien.

**Étapes**

1. Partir d'un mur, puis de hors de la carte, puis des deux côtés d'un mur.

**Résultat attendu**

- Vérifie que `core::ExplorationReach(carte, {{.column = 1, .row = 0}}).count()` vaut `0U`.
- Vérifie que `core::ExplorationReach(carte, {{.column = -1, .row = 0}}).count()` vaut `0U`.
- Vérifie que `core::ExplorationReach(carte, {{.column = 0, .row = 0}, {.column = 2, .row = 0}}).count()` vaut `2U`.

## test_exploration_session.cpp

### ExplorationSessionTest.LeHerosMarcheEtLeMurLArrete

*Critique · Unitaire · Exploration* — `Source/Test/Unit/Core/World/test_exploration_session.cpp:108`

Le heros marche sur la carte et bute sur le mur, en glissant le long.

**Étapes**

1. Entrer sur une carte muree, marcher vers la droite jusqu'au mur.
2. Marcher en diagonale contre ce mur.

**Résultat attendu**

- Vérifie que `session.start("place", "")` est vrai.
- Vérifie que `session.heroPoint()` vaut `(core::CellPoint{1.5F, 1.5F})`.
- Vérifie que `session.heroPoint().column` vaut `8.7F`, à `0.05F` près.
- Vérifie que `session.heroPoint().row` vaut `1.5F`, à `0.001F` près.
- Vérifie que `session.heroPoint().row` est strictement supérieur à `avant`.
- Vérifie que `session.heroPoint().column` vaut `8.7F`, à `0.05F` près.

### ExplorationSessionTest.UnPortailDeposeAuPointDArriveeNomme

*Critique · Unitaire · Exploration* — `Source/Test/Unit/Core/World/test_exploration_session.cpp:138`

Marcher sur un portail depose le heros au point d'arrivee nomme de la cible.

**Étapes**

1. Marcher jusqu'a la case du portail.
2. Rester dessus quelques pas.

**Résultat attendu**

- Vérifie que `session.start("place", "")` est vrai.
- Vérifie que `vus.size()` vaut `1U`.
- Vérifie que `vus.front().kind` vaut `ExplorationEventKind::MapEntered`.
- Vérifie que `vus.front().value` vaut `"cave"`.
- Vérifie que `session.mapId()` vaut `"cave"`.
- Vérifie que `session.heroPoint()` vaut `(core::CellPoint{6.5F, 7.5F})`.
- Vérifie que `session.update(core::ExplorationIntent{.move = {}, .interact = false}, 1.0F / 60.0F) .empty()` est vrai.

### ExplorationSessionTest.OnParleAuPnjQueLOnRegarde

*Critique · Unitaire · Exploration* — `Source/Test/Unit/Core/World/test_exploration_session.cpp:180`

L'interaction ouvre le dialogue du PNJ vise, pas celui d'un autre.

**Étapes**

1. Poser deux PNJ de part et d'autre du heros.
2. Regarder l'un, interagir ; puis regarder l'autre, interagir.

**Résultat attendu**

- Vérifie que `session.start("place", "")` est vrai.
- Vérifie que `vus.size()` vaut `1U`.
- Vérifie que `vus.front().kind` vaut `ExplorationEventKind::Dialogue`.
- Vérifie que `vus.front().value` vaut `"heraut-colisee"`.
- Vérifie que `vus.size()` vaut `1U`.
- Vérifie que `vus.front().value` vaut `"myr-marche"`.

### ExplorationSessionTest.UneCarteGeleeNeBougePlus

*Majeur · Unitaire · Exploration* — `Source/Test/Unit/Core/World/test_exploration_session.cpp:213`

Gelee, la session ne deplace plus le heros et n'ouvre plus rien.

**Étapes**

1. Geler la session, marcher et interagir.
2. Degeler, refaire les memes gestes.

**Résultat attendu**

- Vérifie que `session.start("place", "")` est vrai.
- Vérifie que `session.heroPoint()` vaut `core::cellCenter({4, 4})`.
- Vérifie que `session.update(core::ExplorationIntent{.move = {}, .interact = true}, 1.0F / 60.0F) .empty()` est vrai.
- Vérifie que `session.heroPoint().column` est strictement supérieur à `4.5F`.

## test_world_graph.cpp

### WorldGraphTest.UnAllerRetourSeResoutDansLesDeuxSens

*Critique · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:91`

Un aller-retour A -> B -> A se resout dans les deux sens.

**Étapes**

1. Construire deux cartes, chacune avec un point d'arrivee et un portail vers l'autre.
2. Lire les portails sortants et entrants de chaque carte.

**Résultat attendu**

- Vérifie que `graphe.portals.size()` vaut `2U`.
- Vérifie que `lien.status` vaut `PortalLinkStatus::Resolved`.
- Vérifie que `graphe.portalsFrom("village").size()` vaut `1U`.
- Vérifie que `*graphe.portalsFrom("village").front()` vaut `(core::WorldPortalLink{.fromMap = "village", .position = {3, 1}, .toMap = "foret", .arrival = "lisiere", .status = PortalLinkStatus::Resolved})`.
- Vérifie que `graphe.portalsTo("village").size()` vaut `1U`.
- Vérifie que `graphe.portalsTo("village").front()->fromMap` vaut `"foret"`.
- Vérifie que `graphe.unreachableFrom("village").empty()` est vrai.
- Vérifie que `graphe.unreachableFrom("foret").empty()` est vrai.

### WorldGraphTest.ChaqueDefautDePortailRecoitSonStatut

*Critique · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:125`

Chaque defaut de portail recoit son statut.

**Étapes**

1. Construire un portail par defaut : cible vide, carte inconnue, carte illisible, arrivee vide, arrivee inconnue.
2. Combiner deux defauts (carte illisible et arrivee vide).

**Résultat attendu**

- Vérifie que `statutUnique(portail("", "quai"), cibleSaine)` vaut `PortalLinkStatus::MissingTarget`.
- Vérifie que `statutUnique(portail("", ""), cibleSaine)` vaut `PortalLinkStatus::MissingTarget`.
- Vérifie que `statutUnique(portail("ailleurs", "quai"), cibleSaine)` vaut `PortalLinkStatus::UnknownMap`.
- Vérifie que `statutUnique(portail("cible", "quai"), cibleIllisible)` vaut `PortalLinkStatus::TargetUnreadable`.
- Vérifie que `statutUnique(portail("cible", ""), cibleIllisible)` vaut `PortalLinkStatus::TargetUnreadable`.
- Vérifie que `statutUnique(portail("cible", ""), cibleSaine)` vaut `PortalLinkStatus::MissingArrival`.
- Vérifie que `statutUnique(portail("cible", "gare"), cibleSaine)` vaut `PortalLinkStatus::UnknownArrival`.
- Vérifie que `statutUnique(portail("cible", "quai"), cibleSaine)` vaut `PortalLinkStatus::Resolved`.

### WorldGraphTest.UneProprieteNonTextuelleVautVide

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:153`

Une propriete non textuelle vaut une propriete vide.

**Étapes**

1. Poser un portail dont la cible est un entier, puis un dont l'arrivee est un booleen.
2. Poser un point d'arrivee dont le nom est un entier.

**Résultat attendu**

- Vérifie que `statutUnique(cibleEntiere, cible)` vaut `PortalLinkStatus::MissingTarget`.
- Vérifie que `statutUnique(arriveeBooleenne, cible)` vaut `PortalLinkStatus::MissingArrival`.
- Vérifie que `graphe.find("cible")` diffère de `nullptr`.
- Vérifie que `graphe.find("cible")->arrivalPoints` vaut `(std::vector<std::string>{"quai"})`.

### WorldGraphTest.LesPointsDArriveeSontTriesSansDoublon

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:181`

Les points d'arrivee sont tries, sans doublon ni nom vide.

**Étapes**

1. Poser quatre points d'arrivee dans le desordre, dont un doublon et un nom vide.

**Résultat attendu**

- Vérifie que `graphe.find("port")` diffère de `nullptr`.
- Vérifie que `graphe.find("port")->arrivalPoints` vaut `(std::vector<std::string>{"digue", "quai"})`.

### WorldGraphTest.UnPortailVersSaPropreCarteEstLegal

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:198`

Un portail vers sa propre carte est legal.

**Étapes**

1. Construire une carte dont le portail ramene a un de ses propres points d'arrivee.

**Résultat attendu**

- Vérifie que `graphe.portals.size()` vaut `1U`.
- Vérifie que `graphe.portals.front().status` vaut `PortalLinkStatus::Resolved`.
- Vérifie que `graphe.portalsFrom("donjon").size()` vaut `1U`.
- Vérifie que `graphe.portalsTo("donjon").size()` vaut `1U`.
- Vérifie que `graphe.unreachableFrom("donjon").empty()` est vrai.

### WorldGraphTest.LesCartesInjoignablesSuiventLesPortailsResolus

*Critique · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:218`

Les cartes injoignables suivent le sens des portails resolus.

**Étapes**

1. Relier A -> B par un portail resolu, B -> C par un portail casse ; D ne mene qu'a A.
2. Demander les cartes injoignables depuis A, puis depuis une carte inexistante.

**Résultat attendu**

- Vérifie que `graphe.unreachableFrom("a")` vaut `(std::vector<std::string>{"c", "d"})`.
- Vérifie que `graphe.unreachableFrom("d")` vaut `(std::vector<std::string>{"c"})`.
- Vérifie que `graphe.unreachableFrom("nulle-part")` vaut `(std::vector<std::string>{"a", "b", "c", "d"})`.

### WorldGraphTest.LOrdreDuGrapheEstDeterministe

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:242`

Le graphe ne depend pas de l'ordre des cartes recues.

**Étapes**

1. Construire le meme monde avec les cartes dans deux ordres opposes.

**Résultat attendu**

- Vérifie que `endroit.maps.size()` vaut `3U`.
- Vérifie que `endroit.maps[0].mapId` vaut `"alpha"`.
- Vérifie que `endroit.maps[1].mapId` vaut `"mu"`.
- Vérifie que `endroit.maps[2].mapId` vaut `"zeta"`.
- Vérifie que `endroit.portals.size()` vaut `3U`.
- Vérifie que `endroit.portals[0].fromMap` vaut `"alpha"`.
- Vérifie que `endroit.portals[1].position` vaut `(core::GridPosition{1, 0})`.
- Vérifie que `endroit.portals[2].position` vaut `(core::GridPosition{2, 0})`.
- Vérifie que `endroit.portals` vaut `envers.portals`.
- Vérifie que `envers.maps.size()` vaut `endroit.maps.size()`.
- Vérifie que `envers.maps[rang].mapId` vaut `endroit.maps[rang].mapId`.

### WorldGraphFileTest.UnDossierDeNiveauxSeLitEnGraphe

*Critique · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:277`

Un dossier de niveaux se lit en graphe, carte illisible comprise.

**Étapes**

1. Ecrire deux cartes valides reliees par des portails, un fichier JSON malforme, un fichier `sequence-*.json` et un fichier non JSON.
2. Charger le graphe du dossier.

**Résultat attendu**

- Vérifie que `graphe.maps.size()` vaut `3U`.
- Vérifie que `graphe.maps[0].mapId` vaut `"foret"`.
- Vérifie que `graphe.maps[1].mapId` vaut `"grotte"`.
- Vérifie que `graphe.maps[2].mapId` vaut `"village"`.
- Vérifie que `graphe.maps[2].name` vaut `"Village"`.
- Vérifie que `graphe.maps[2].loadError.empty()` est vrai.
- Vérifie que `graphe.maps[1].loadError.empty()` est faux.
- Vérifie que `graphe.maps[0].arrivalPoints` vaut `(std::vector<std::string>{"lisiere"})`.
- Vérifie que `graphe.portals.size()` vaut `3U`.
- Vérifie que `graphe.portals[0].status` vaut `PortalLinkStatus::Resolved`.
- Vérifie que `graphe.portals[1].status` vaut `PortalLinkStatus::Resolved`.
- Vérifie que `graphe.portals[2].toMap` vaut `"grotte"`.
- Vérifie que `graphe.portals[2].status` vaut `PortalLinkStatus::TargetUnreadable`.
- Vérifie que `graphe.unreachableFrom("village")` vaut `(std::vector<std::string>{"grotte"})`.

### WorldGraphFileTest.UneCarteDUnSousDossierAPourIdentifiantSonCheminRelatif

*Critique · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:320`

Les cartes d'un sous-dossier entrent au graphe sous leur chemin relatif.

**Étapes**

1. Ecrire `capital/martpart.json` et `capital/arenarea.json`, reliees par portails, et `coliseum.json` a la racine.
2. Charger le graphe du dossier.

**Résultat attendu**

- Vérifie que `graphe.maps.size()` vaut `3U`.
- Vérifie que `graphe.maps[0].mapId` vaut `"capital/arenarea"`.
- Vérifie que `graphe.maps[1].mapId` vaut `"capital/martpart"`.
- Vérifie que `graphe.maps[2].mapId` vaut `"coliseum"`.
- Vérifie que `graphe.portals.size()` vaut `2U`.
- Vérifie que `graphe.portals[0].status` vaut `PortalLinkStatus::Resolved`.
- Vérifie que `graphe.portals[1].status` vaut `PortalLinkStatus::Resolved`.
- Vérifie que `core::mapIdOf(dir, dir / "capital" / "martpart.json")` vaut `"capital/martpart"`.

### WorldGraphFileTest.UnDossierAbsentDonneUnGrapheVide

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Core/World/test_world_graph.cpp:354`

Un dossier absent donne un graphe vide.

**Étapes**

1. Charger le graphe d'un dossier qui n'existe pas.

**Résultat attendu**

- `EXPECT_NO_THROW(graphe = core::loadWorldGraph(dir / "inexistant"))`
- Vérifie que `graphe.maps.empty()` est vrai.
- Vérifie que `graphe.portals.empty()` est vrai.

## test_world_travel.cpp

### WorldTravelTest.CinqCartesSeParcourentDansLesDeuxSens

*Critique · Unitaire · Monde parcouru* — `Source/Test/Unit/Core/World/test_world_travel.cpp:140`

Un parcours de cinq cartes, aller et retour, par points d'arrivee nommes.

**Étapes**

1. Entrer sur la premiere carte, puis franchir quatre portails a la file.
2. Revenir par les portails de retour jusqu'a la premiere carte.

**Résultat attendu**

- Vérifie que `voyage.enter("un", "")` vaut `TravelResult::Moved`.
- Vérifie que `voyage.position()` vaut `(core::GridPosition{1, 1})`.
- Vérifie que `voyage.cross({6, 2}, drapeaux)` vaut `TravelResult::Moved`.
- Vérifie que `voyage.currentMapId()` vaut `attendue`.
- Vérifie que `voyage.position()` vaut `(core::GridPosition{3, 3})`.
- Vérifie que `voyage.cross({0, 3}, drapeaux)` vaut `TravelResult::Moved`.
- Vérifie que `voyage.currentMapId()` vaut `attendue`.
- Vérifie que `voyage.position()` vaut `(core::GridPosition{2, 2})`.
- Vérifie que `voyage.loadedMapCount()` vaut `5U`.
- Vérifie que `dossier.lectures(nom)` vaut `1`.

### WorldTravelTest.UneCaseSansPortailNEstPasUneTraversee

*Majeur · Unitaire · Monde parcouru* — `Source/Test/Unit/Core/World/test_world_travel.cpp:180`

Une case ordinaire ne declenche aucune traversee.

**Étapes**

1. Entrer sur une carte, puis tenter de franchir une case vide.

**Résultat attendu**

- Vérifie que `voyage.enter("un", "")` vaut `TravelResult::Moved`.
- Vérifie que `voyage.cross({4, 4}, drapeaux)` vaut `TravelResult::NoPortal`.
- Vérifie que `voyage.currentMapId()` vaut `"un"`.
- Vérifie que `voyage.position()` vaut `(core::GridPosition{1, 1})`.

### WorldTravelTest.UnPortailADrapeauResteFermeSansLeDrapeau

*Critique · Unitaire · Monde parcouru* — `Source/Test/Unit/Core/World/test_world_travel.cpp:200`

Un portail exigeant un drapeau ne s'ouvre qu'une fois le drapeau pose.

**Étapes**

1. Franchir un portail qui exige un drapeau absent.
2. Poser le drapeau, puis franchir le meme portail.

**Résultat attendu**

- Vérifie que `voyage.enter("place", "")` vaut `TravelResult::Moved`.
- Vérifie que `voyage.cross({5, 5}, drapeaux)` vaut `TravelResult::Locked`.
- Vérifie que `voyage.currentMapId()` vaut `"place"`.
- Vérifie que `voyage.cross({5, 5}, drapeaux)` vaut `TravelResult::Moved`.
- Vérifie que `voyage.currentMapId()` vaut `"repaire"`.
- Vérifie que `voyage.position()` vaut `(core::GridPosition{1, 4})`.

### WorldTravelTest.UnPortailOrphelinEstRefuseAuChargement

*Critique · Unitaire · Monde parcouru* — `Source/Test/Unit/Core/World/test_world_travel.cpp:230`

Chaque defaut de portail devient un code exploitable au chargement.

**Étapes**

1. Batir un dossier ou un portail vise une carte absente et un autre un point d'arrivee absent.
2. Valider le graphe.

**Résultat attendu**

- Vérifie que `defauts.size()` vaut `3U`.
- Vérifie que `defauts[0]` vaut `(core::WorldIssue{.mapId = "place", .position = {5, 5}, .code = WorldIssueCode::UnknownTargetMap, .value = "nulle-part"})`.
- Vérifie que `defauts[1]` vaut `(core::WorldIssue{.mapId = "place", .position = {6, 5}, .code = WorldIssueCode::UnknownArrivalPoint, .value = "cave"})`.
- Vérifie que `defauts[2].code` vaut `WorldIssueCode::MissingTargetMap`.
- Vérifie que `voyage.enter("place", "")` vaut `TravelResult::Moved`.
- Vérifie que `voyage.cross({5, 5}, core::WorldFlags{})` vaut `TravelResult::UnreadableMap`.
- Vérifie que `voyage.lastIssue().has_value()` est vrai.
- Vérifie que `voyage.lastIssue()->code` vaut `WorldIssueCode::UnreadableMap`.

### WorldTravelTest.UnPointDArriveeEnDoubleEstReleveSurLaCarte

*Majeur · Unitaire · Monde parcouru* — `Source/Test/Unit/Core/World/test_world_travel.cpp:268`

Deux points d'arrivee du meme nom sont un defaut de la carte.

**Étapes**

1. Poser deux points d'arrivee « seuil » sur la meme carte.
2. Valider la carte lue, puis le graphe.

**Résultat attendu**

- Vérifie que `defauts.size()` vaut `1U`.
- Vérifie que `defauts.front()` vaut `(core::WorldIssue{.mapId = "place", .position = {6, 6}, .code = WorldIssueCode::DuplicateArrivalPoint, .value = "seuil"})`.
- Vérifie que `premier.has_value()` est vrai.
- Vérifie que `*premier` vaut `(core::GridPosition{1, 4})`.

### WorldTravelTest.UneArriveeInconnueRefuseLEntree

*Majeur · Unitaire · Monde parcouru* — `Source/Test/Unit/Core/World/test_world_travel.cpp:294`

Entrer par un point d'arrivee absent est refuse, le nom cite.

**Étapes**

1. Entrer sur une carte par un point d'arrivee qu'elle n'offre pas.

**Résultat attendu**

- Vérifie que `voyage.enter("un", "")` vaut `TravelResult::Moved`.
- Vérifie que `voyage.enter("deux", "cave")` vaut `TravelResult::UnknownArrival`.
- Vérifie que `voyage.lastIssue().has_value()` est vrai.
- Vérifie que `voyage.lastIssue()->code` vaut `WorldIssueCode::UnknownArrivalPoint`.
- Vérifie que `voyage.lastIssue()->value` vaut `"cave"`.
- Vérifie que `voyage.currentMapId()` vaut `"un"`.

### WorldTravelTest.LesCartesDuPremierDossierPassentDevant

*Critique · Unitaire · Monde parcouru* — `Source/Test/Unit/Core/World/test_world_travel.cpp:315`

Les cartes du brouillon passent devant celles du jeu.

**Étapes**

1. Poser deux dossiers, la meme carte dans les deux sous un nom different, et une seconde carte dans le dernier seulement.

**Résultat attendu**

- Vérifie que `std::filesystem::create_directories(racine / "brouillons")` est vrai.
- Vérifie que `std::filesystem::create_directories(racine / "livrees")` est vrai.
- Vérifie que `core::LevelWriter::saveToFile(brouillon, racine / "brouillons" / "martpart.json")` est vrai.
- Vérifie que `core::LevelWriter::saveToFile(livree, racine / "livrees" / "martpart.json")` est vrai.
- Vérifie que `core::LevelWriter::saveToFile(voisine, racine / "livrees" / "arenarea.json")` est vrai.
- Vérifie que `retouchee.ok()` est vrai.
- Vérifie que `retouchee.level->name()` vaut `"Martpart retouche"`.
- Vérifie que `autour.ok()` est vrai.
- Vérifie que `autour.level->name()` vaut `"Arenarea"`.
- Vérifie que `absente.ok()` est faux.
- Vérifie que `absente.errorCode` vaut `core::LevelValidationError::FileNotFound`.
