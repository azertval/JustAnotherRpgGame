# HMI · Presentation

Tests unitaires — **19 cas** (5 critiques, 11 majeurs, 3 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_credits_catalog.cpp`](#test-credits-catalogcpp) | 7 | - | 2 | 4 | 1 |
| [`test_inventory_screen.cpp`](#test-inventory-screencpp) | 6 | - | 1 | 3 | 2 |
| [`test_quest_journal_screen.cpp`](#test-quest-journal-screencpp) | 1 | - | 1 | - | - |
| [`test_world_maps.cpp`](#test-world-mapscpp) | 5 | - | 1 | 4 | - |

## test_credits_catalog.cpp

### CreditsCatalogTest.CreditsLusDansLaLangueDemandee

*Majeur · Unitaire · Crédits* — `Source/Test/Unit/HMI/Presentation/test_credits_catalog.cpp:44`

Les crédits se lisent dans la langue demandée.

**Étapes**

1. Lire un JSON de deux sections en anglais.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `result.sections.size()` vaut `2U`.
- Vérifie que `result.sections[0].title` vaut `"Development"`.
- Vérifie que `result.sections[0].column` vaut `0`.
- Vérifie que `result.sections[0].lines.size()` vaut `1U`.
- Vérifie que `result.sections[0].lines[0].role` vaut `"Programming"`.
- Vérifie que `result.sections[0].lines[0].names` vaut `(std::vector<std::string>{"A", "B"})`.
- Vérifie que `result.sections[1].column` vaut `1`.

### CreditsCatalogTest.LibelleNonTraduitRetombeSurLeFrancais

*Mineur · Unitaire · Crédits* — `Source/Test/Unit/HMI/Presentation/test_credits_catalog.cpp:65`

Un libellé non traduit des crédits s'affiche en français.

**Étapes**

1. Lire en anglais une section dont le titre et le rôle n'ont que le français.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `result.sections[1].title` vaut `"Audio"`.
- Vérifie que `result.sections[1].lines[0].role` vaut `"Bruitages"`.

### CreditsCatalogTest.NomTraduitSuitLaLangue

*Majeur · Unitaire · Crédits* — `Source/Test/Unit/HMI/Presentation/test_credits_catalog.cpp:81`

Une mention des crédits se traduit comme un rôle.

**Étapes**

1. Lire en anglais une ligne dont les noms mêlent une chaîne, un libellé traduit et un libellé qui n'a que le français.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `result.sections[0].lines[0].names` vaut `(std::vector<std::string>{"Dragori Games, Inc.", "Unofficial fan game", "Non commercial"})`.

### CreditsCatalogTest.NomIllisibleRefuse

*Majeur · Unitaire · Crédits* — `Source/Test/Unit/HMI/Presentation/test_credits_catalog.cpp:103`

Un nom de crédits illisible fait échouer la lecture.

**Étapes**

1. Lire une ligne dont un nom est un nombre, puis une autre dont un nom est un libellé vide.

**Résultat attendu**

- Vérifie que `hmi::readCredits( R"({"sections":[{"id":"x","title":{"fr":"X"},"lines":[{"role":{"fr":"R"},"names":[3]}]}]})", "fr") .ok()` est faux.
- Vérifie que `hmi::readCredits( R"({"sections":[{"id":"x","title":{"fr":"X"},"lines":[{"role":{"fr":"R"},"names":[{"en":""}]}]}]})", "fr") .ok()` est faux.

### CreditsCatalogTest.LigneSansNomFaitEchouerLaLecture

*Critique · Unitaire · Crédits* — `Source/Test/Unit/HMI/Presentation/test_credits_catalog.cpp:126`

Une ligne de crédits sans nom fait échouer la lecture.

**Étapes**

1. Lire un JSON dont la seule ligne a une liste de noms vide.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.sections.empty()` est vrai.
- Vérifie que `result.error.find("x")` diffère de `std::string::npos`.

### CreditsCatalogTest.ColonneHorsBornesRefusee

*Majeur · Unitaire · Crédits* — `Source/Test/Unit/HMI/Presentation/test_credits_catalog.cpp:144`

Une section de crédits hors des deux colonnes est refusée.

**Étapes**

1. Lire une section de colonne 2.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.

### CreditsCatalogTest.FichierLivreSeLitDansLesDeuxLangues

*Critique · Unitaire · Crédits* — `Source/Test/Unit/HMI/Presentation/test_credits_catalog.cpp:160`

Le fichier de crédits livré se lit en français et en anglais.

**Étapes**

1. Lire `Source/Elements/Credits/credits.json` en français, puis en anglais.

**Résultat attendu**

- Vérifie que `file` est vrai.
- Vérifie que `result.ok()` est vrai.
- Vérifie que `left && right` est vrai.

## test_inventory_screen.cpp

### InventoryScreenTest.OngletsFiltrentParFamille

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/HMI/Presentation/test_inventory_screen.cpp:70`

Les onglets de l'inventaire filtrent le sac par famille.

**Étapes**

1. Remplir un sac d'une épée, de torches et d'outils.
2. Demander les cases de chaque filtre.

**Résultat attendu**

- Vérifie que `hmi::backpackCells(inventory, catalogs.lookup(), hmi::ItemFamily::All).size()` vaut `3U`.
- Vérifie que `equipment.size()` vaut `1U`.
- Vérifie que `equipment[0].name` vaut `"Épée longue"`.
- Vérifie que `gear.size()` vaut `1U`.
- Vérifie que `gear[0].quantity` vaut `5`.
- Vérifie que `hmi::backpackCells(inventory, catalogs.lookup(), hmi::ItemFamily::Tools).size()` vaut `1U`.

### InventoryScreenTest.EquiperEchangeAvecLEmplacement

*Critique · Unitaire · Inventaire* — `Source/Test/Unit/HMI/Presentation/test_inventory_screen.cpp:99`

Équiper depuis l'inventaire rend l'objet remplacé au sac.

**Étapes**

1. Porter une dague en main directrice, avoir une épée longue dans le sac.
2. Équiper l'épée depuis le sac.

**Résultat attendu**

- Vérifie que `hmi::equipFromBackpack(inventory, "epee-longue", catalogs.lookup())` est vrai.
- Vérifie que `inventory.at(core::EquipmentSlot::MainHand)` vaut `"epee-longue"`.
- Vérifie que `quantityOf(inventory, "dague")` vaut `1`.
- Vérifie que `quantityOf(inventory, "epee-longue")` vaut `0`.

### InventoryScreenTest.EmplacementNaturel

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/HMI/Presentation/test_inventory_screen.cpp:121`

Un objet s'équipe à son emplacement naturel, ou pas du tout.

**Étapes**

1. Demander l'emplacement naturel d'un arc, d'une épée, d'une armure, d'un bouclier, d'une torche.
2. Tenter d'équiper la torche.

**Résultat attendu**

- Vérifie que `hmi::naturalSlot("arc-long", catalogs.lookup())` vaut `core::EquipmentSlot::Ranged`.
- Vérifie que `hmi::naturalSlot("epee-longue", catalogs.lookup())` vaut `core::EquipmentSlot::MainHand`.
- Vérifie que `hmi::naturalSlot("cuir", catalogs.lookup())` vaut `core::EquipmentSlot::Torso`.
- Vérifie que `hmi::naturalSlot("bouclier", catalogs.lookup())` vaut `core::EquipmentSlot::OffHand`.
- Vérifie que `hmi::naturalSlot("torche", catalogs.lookup()).has_value()` est faux.
- Vérifie que `hmi::equipFromBackpack(inventory, "torche", catalogs.lookup())` est faux.
- Vérifie que `quantityOf(inventory, "torche")` vaut `1`.

### InventoryScreenTest.RetirerEtJeter

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/HMI/Presentation/test_inventory_screen.cpp:147`

Retirer un objet le range au sac, jeter en retire un exemplaire.

**Étapes**

1. Retirer le bouclier porté.
2. Jeter une torche d'une pile de trois.
3. Retirer un emplacement vide.

**Résultat attendu**

- Vérifie que `hmi::unequipToBackpack(inventory, core::EquipmentSlot::OffHand)` est vrai.
- Vérifie que `inventory.isEquipped(core::EquipmentSlot::OffHand)` est faux.
- Vérifie que `quantityOf(inventory, "bouclier")` vaut `1`.
- Vérifie que `hmi::dropFromBackpack(inventory, "torche")` est vrai.
- Vérifie que `quantityOf(inventory, "torche")` vaut `2`.
- Vérifie que `hmi::unequipToBackpack(inventory, core::EquipmentSlot::Head)` est faux.

### InventoryScreenTest.TrierParNom

*Mineur · Unitaire · Inventaire* — `Source/Test/Unit/HMI/Presentation/test_inventory_screen.cpp:173`

Trier l'inventaire range le sac par nom.

**Étapes**

1. Remplir un sac de torches, d'une épée et d'une dague, dans cet ordre.
2. Trier.

**Résultat attendu**

- Vérifie que `inventory.backpack.size()` vaut `3U`.
- Vérifie que `inventory.backpack[0].itemId` vaut `"dague"`.
- Vérifie que `inventory.backpack[1].itemId` vaut `"torche"`.
- Vérifie que `inventory.backpack[2].itemId` vaut `"epee-longue"`.

### InventoryScreenTest.FicheDObjet

*Mineur · Unitaire · Inventaire* — `Source/Test/Unit/HMI/Presentation/test_inventory_screen.cpp:197`

La fiche d'un objet de l'inventaire décrit sa nature et son poids.

**Étapes**

1. Demander la fiche de l'armure de cuir, du bouclier et de la torche.

**Résultat attendu**

- Vérifie que `armor.kind` vaut `"Armure légère"`.
- Vérifie que `armor.armor` vaut `"CA 11 + Dex"`.
- Vérifie que `armor.equippable` est vrai.
- Vérifie que `hmi::itemSheet("bouclier", catalogs.lookup()).armor` vaut `"+2"`.
- Vérifie que `torch.kind` vaut `"Matériel"`.
- Vérifie que `torch.weight` vaut `"0,5 kg"`.
- Vérifie que `torch.equippable` est faux.

## test_quest_journal_screen.cpp

### QuestJournalScreenTest.LeJournalSeLitDansLesDrapeaux

*Critique · Unitaire · Journal de quetes* — `Source/Test/Unit/HMI/Presentation/test_quest_journal_screen.cpp:45`

Le journal se lit dans les drapeaux.

**Étapes**

1. Deux quetes ; aucune commencee.
2. Faire atteindre `un` puis `deux` a la premiere, `un` a la seconde.
3. Choisir la seconde, puis la voisine suivante et precedente.
4. Clore la premiere en echec.

**Résultat attendu**

- Vérifie que `vide.quests.empty()` est vrai.
- Vérifie que `vide.selected.empty()` est vrai.
- Vérifie que `vide.detail` vaut `"journal.empty"`.
- Vérifie que `ouvert.quests.size()` vaut `2U`.
- Vérifie que `ouvert.selected` vaut `"pommes"`.
- Vérifie que `ouvert.quests[0]` vaut `(hmi::QuestJournalRow{"pommes", "› quest.pommes.title", "journal.status.active"})`.
- Vérifie que `ouvert.quests[1].label` vaut `"quest.caves.title"`.
- Vérifie que `ouvert.detail` vaut `"quest.pommes.deux"`.
- Vérifie que `ouvert.objectives.size()` vaut `2U`.
- Vérifie que `ouvert.objectives[0]` vaut `(hmi::QuestJournalRow{"un", "quest.pommes.un", "✓"})`.
- Vérifie que `ouvert.objectives[1]` vaut `(hmi::QuestJournalRow{"deux", "quest.pommes.deux", ""})`.
- Vérifie que `seconde.selected` vaut `"caves"`.
- Vérifie que `seconde.quests[1].label` vaut `"› quest.caves.title"`.
- Vérifie que `seconde.quests[0].label` vaut `"quest.pommes.title"`.
- Vérifie que `hmi::neighbourQuest(seconde, 1)` vaut `"caves"`.
- Vérifie que `hmi::neighbourQuest(seconde, -1)` vaut `"pommes"`.
- Vérifie que `hmi::neighbourQuest(ouvert, -1)` vaut `"pommes"`.
- Vérifie que `close.quests[0].value` vaut `"journal.status.failed"`.
- Vérifie que `close.objectives.size()` vaut `3U`.
- Vérifie que `close.objectives[2].value` vaut `"journal.status.failed"`.

## test_world_maps.cpp

### WorldMapsTest.FichierLuEtBorne

*Majeur · Unitaire · Carte* — `Source/Test/Unit/HMI/Presentation/test_world_maps.cpp:69`

Les cartes se lisent, et une position hors de la carte est refusée.

**Étapes**

1. Lire un fichier à une région et une ville.
2. Lire une position d'abscisse 1,2, puis un cadre qui déborde.

**Résultat attendu**

- Vérifie que `maps.ok()` est vrai.
- Vérifie que `maps.worldImage` vaut `"world.jpg"`.
- Vérifie que `maps.regions.size()` vaut `1U`.
- Vérifie que `region.anchor.x` vaut `0.25` (comparaison flottante).
- Vérifie que `region.frame.width` vaut `0.3` (comparaison flottante).
- Vérifie que `region.places.at("a-town").y` vaut `0.6` (comparaison flottante).
- Vérifie que `region.labels.size()` vaut `1U`.
- Vérifie que `region.labels.front().kind` vaut `"lake"`.
- Vérifie que `maps.cities.at("a-town").sites.size()` vaut `1U`.
- Vérifie que `maps.cities.at("a-town").sites.front().number` vaut `3`.
- Vérifie que `outside.ok()` est faux.
- Vérifie que `outside.regions.empty()` est vrai.
- Vérifie que `outside.error.find("c")` diffère de `std::string::npos`.
- Vérifie que `overflowing.ok()` est faux.
- Vérifie que `overflowing.error.find("cadre")` diffère de `std::string::npos`.

### WorldMapsTest.UnQuartierRenduPorteSaGrilleEtSesSousZones

*Majeur · Unitaire · Carte* — `Source/Test/Unit/HMI/Presentation/test_world_maps.cpp:108`

La carte rendue d'un quartier se lit avec sa grille et ses sous-zones.

**Étapes**

1. Lire un plan dont le quartier nomme une carte rendue, sa grille et une sous-zone.
2. Lire le même quartier sans grille, puis une sous-zone sans entrée.

**Résultat attendu**

- Vérifie que `maps.ok()` est vrai.
- Vérifie que `district.image` vaut `"Regions/r/t/a/Map/a.jpg"`.
- Vérifie que `district.grid.has_value()` est vrai.
- Vérifie que `point.x` vaut `0.5 + (2 * 0.02) - 0.02` (comparaison flottante).
- Vérifie que `point.y` vaut `0.1 + (2 * 0.025) + 0.025` (comparaison flottante).
- Vérifie que `district.zones.size()` vaut `1U`.
- Vérifie que `zone.name` vaut `"La crypte"`.
- Vérifie que `zone.entrance.x` vaut `23.0` (comparaison flottante).
- Vérifie que `zone.entrance.y` vaut `6.0` (comparaison flottante).
- Vérifie que `zone.grid.has_value()` est vrai.
- Vérifie que `gridless.ok()` est faux.
- Vérifie que `gridless.error.find("grid")` diffère de `std::string::npos`.
- Vérifie que `doorless.ok()` est faux.
- Vérifie que `doorless.error.find("entrance")` diffère de `std::string::npos`.

### WorldMapsTest.JointureRangeLesLieux

*Majeur · Unitaire · Carte* — `Source/Test/Unit/HMI/Presentation/test_world_maps.cpp:154`

Une région montre ses lieux posés puis les autres, sans les entrées écartées ni les quartiers d'une ville.

**Étapes**

1. Joindre un atlas de quatre lieux (une règle, un lieu sans position, une ville à plan, son quartier) au fichier minimal.

**Résultat attendu**

- Vérifie que `maps.ok()` est vrai.
- Vérifie que `mismatches.empty()` est vrai.
- Vérifie que `views.regions.size()` vaut `1U`.
- Vérifie que `places.size()` vaut `2U`.
- Vérifie que `places[0].id` vaut `"a-town"`.
- Vérifie que `places[0].placed` est vrai.
- Vérifie que `places[0].hasCityMap` est vrai.
- Vérifie que `places[1].id` vaut `"a-hidden"`.
- Vérifie que `places[1].placed` est faux.
- Vérifie que `views.cities.size()` vaut `1U`.
- Vérifie que `points.size()` vaut `2U`.
- Vérifie que `points[0].id` vaut `"a-town-market"`.
- Vérifie que `points[0].number` vaut `1`.
- Vérifie que `points[1].name` vaut `"Harbour"`.
- Vérifie que `points[1].number` vaut `3`.

### WorldMapsTest.EcartsNommes

*Majeur · Unitaire · Carte* — `Source/Test/Unit/HMI/Presentation/test_world_maps.cpp:191`

Une carte sans région, une région sans carte et une position étrangère sont signalées.

**Étapes**

1. Joindre au fichier minimal, qui ne connaît que la région `a`, un atlas dont la seule région est `b`.

**Résultat attendu**

- Vérifie que `views.regions.empty()` est vrai.
- Vérifie que `views.cities.empty()` est vrai.
- Vérifie que `mismatches.size()` est supérieur ou égal à `3U`.

### WorldMapsTest.AtlasLivreEntierementCartographie

*Critique · Unitaire · Carte* — `Source/Test/Unit/HMI/Presentation/test_world_maps.cpp:217`

Les treize régions de l'atlas ont leur carte, et chaque position désigne un lieu de sa région.

**Étapes**

1. Charger l'atlas livré et `Source/Elements/Maps/world-maps.json`.
2. Joindre les deux.
3. Chercher l'image de chaque carte sous `Source/Elements/Assets/Maps/`.

**Résultat attendu**

- Vérifie que `atlas.errors.empty()` est vrai.
- Vérifie que `maps.ok()` est vrai.
- Vérifie que `views.regions.size()` vaut `atlas.regions.size()`.
- Vérifie que `std::filesystem::is_regular_file(images / views.worldImage)` est vrai.
- Vérifie que `std::filesystem::is_regular_file(images / region.image)` est vrai.
- Vérifie que `views.cities.size()` vaut `2U`.
- Vérifie que `std::filesystem::is_regular_file(images / city.image)` est vrai.
- Vérifie que `city.points.size()` vaut `12U`.
- Vérifie que `region` diffère de `views.regions.end()`.
- Vérifie que `place` diffère de `region->places.end()`.
- Vérifie que `place->placed` est vrai.
- Vérifie que `place->hasCityMap` est vrai.
- Vérifie que `std::filesystem::is_regular_file(imageOf(point.district->image))` est vrai.
- Vérifie que `zone.image.empty() || std::filesystem::is_regular_file(imageOf(zone.image))` est vrai.
