# Core · Gameplay

Tests unitaires — **17 cas** (10 critiques, 6 majeurs, 1 mineur). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_interaction.cpp`](#test-interactioncpp) | 9 | - | 5 | 3 | 1 |
| [`test_quest.cpp`](#test-questcpp) | 8 | - | 5 | 3 | - |

## test_interaction.cpp

### InteractionTest.LaCaseViseeSuitLaDirectionDominante

*Majeur · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:69`

La case visee suit la direction dominante de l'orientation.

**Étapes**

1. Viser avec quatre orientations cardinales, puis une orientation a 30 degres.
2. Viser avec une orientation nulle.

**Résultat attendu**

- Vérifie que `core::aimedCell(depart, {1.0F, 0.0F})` vaut `(core::GridPosition{3, 2})`.
- Vérifie que `core::aimedCell(depart, {-1.0F, 0.0F})` vaut `(core::GridPosition{1, 2})`.
- Vérifie que `core::aimedCell(depart, {0.0F, 1.0F})` vaut `(core::GridPosition{2, 3})`.
- Vérifie que `core::aimedCell(depart, {0.0F, -1.0F})` vaut `(core::GridPosition{2, 1})`.
- Vérifie que `core::aimedCell(depart, {0.866F, 0.5F})` vaut `(core::GridPosition{3, 2})`.
- Vérifie que `core::aimedCell(depart, {0.5F, 0.866F})` vaut `(core::GridPosition{2, 3})`.
- Vérifie que `core::aimedCell(depart, {0.0F, 0.0F})` vaut `depart`.

### InteractionTest.UnCoffreNeDonneSonButinQuUneFois

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:97`

Un coffre ne donne son butin qu'a la premiere ouverture.

**Étapes**

1. Interagir deux fois avec le meme coffre.

**Résultat attendu**

- Vérifie que `premiere.found()` est vrai.
- Vérifie que `butin.happened` est vrai.
- Vérifie que `butin.consumed` est vrai.
- Vérifie que `butin.type` vaut `"chest"`.
- Vérifie que `seconde.found()` est faux.
- Vérifie que `rien.happened` est faux.

### InteractionTest.LEtatDUnCoffreSurvitAUnAllerRetourDeCarte

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:128`

Un coffre ouvert le reste apres avoir quitte la carte et y etre revenu.

**Étapes**

1. Ouvrir un coffre, puis DETRUIRE les entites de la carte et les recreer depuis la couche objects, comme le fait un changement de carte.

**Résultat attendu**

- Vérifie que `core::spawnMapEntities(monde, niveau, niveau.name())` vaut `1U`.
- Vérifie que `objets.size()` vaut `1U`.
- Vérifie que `cible.found()` est vrai.
- Vérifie que `core::interact(cible, drapeaux).consumed` est vrai.
- Vérifie que `core::spawnMapEntities(monde, niveau, niveau.name())` vaut `1U`.
- Vérifie que `objets.size()` vaut `1U`.
- Vérifie que `cible.found()` est faux.

### InteractionTest.LInteractionNeTraversePasUnMur

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:184`

Un objet place sur une case pleine n'est pas atteignable.

**Étapes**

1. Poser un coffre sur une case, puis rendre cette case pleine.

**Résultat attendu**

- Vérifie que `core::findInteractionTarget({2, 2}, {1.0F, 0.0F}, carte, candidats(objets), drapeaux) .found()` est vrai.
- Vérifie que `derriereLeMur.found()` est faux.
- Vérifie que `derriereLeMur.aimedCell` vaut `(core::GridPosition{3, 2})`.

### InteractionTest.LaCibleEstDeterministeAEgalite

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:211`

Deux objets sur la meme case designent toujours le meme.

**Étapes**

1. Poser deux objets sur la case visee et designer la cible dix fois.

**Résultat attendu**

- Vérifie que `cible.found()` est vrai.
- Vérifie que `cible.index` vaut `0U`.
- Vérifie que `cible.interactable->type` vaut `"chest"`.

### InteractionTest.UnPanneauSeRelitIndefiniment

*Majeur · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:236`

Une entite non consommable reste une cible apres interaction.

**Étapes**

1. Interagir trois fois avec un panneau.

**Résultat attendu**

- Vérifie que `cible.found()` est vrai.
- Vérifie que `lecture.happened` est vrai.
- Vérifie que `lecture.consumed` est faux.
- Vérifie que `lecture.promptKey` vaut `"interaction.sign"`.
- Vérifie que `drapeaux.size()` vaut `0U`.

### InteractionTest.DeuxCartesNeSeMarchentPasDessus

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:262`

Deux coffres de cartes differentes a la meme case ont des drapeaux distincts.

**Étapes**

1. Fabriquer la cle de deux coffres a la case (3, 2), sur deux cartes.
2. Ouvrir le premier.

**Résultat attendu**

- Vérifie que `cleVillage` diffère de `cleDonjon`.
- Vérifie que `cible.found()` est vrai.
- Vérifie que `core::interact(cible, drapeaux).consumed` est vrai.
- Vérifie que `core::findInteractionTarget({2, 2}, {1.0F, 0.0F}, carte, candidats(auDonjon), drapeaux) .found()` est vrai.

### InteractionTest.UnTypeInconnuProduitUneEntiteNonInteractive

*Majeur · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:294`

Un objet de type inconnu apparait sur la carte sans etre interactif.

**Étapes**

1. Peupler un monde depuis une couche objects portant un type inconnu.

**Résultat attendu**

- Vérifie que `core::spawnMapEntities(monde, niveau, niveau.name())` vaut `2U`.
- Vérifie que `interactif.type` vaut `"chest"`.
- Vérifie que `interactifs` vaut `1U`.

### InteractionTest.LesDrapeauxSeRelisentTries

*Mineur · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:322`

Les drapeaux acquis se relisent dans un ordre stable.

**Étapes**

1. Lever trois drapeaux dans le desordre, puis les relire.
2. Lever deux fois le meme.

**Résultat attendu**

- Vérifie que `drapeaux.set("village/chest@3,2")` est vrai.
- Vérifie que `drapeaux.set("donjon/chest@1,1")` est vrai.
- Vérifie que `drapeaux.set("village/chest@0,0")` est vrai.
- Vérifie que `drapeaux.set("village/chest@3,2")` est faux.
- Vérifie que `drapeaux.size()` vaut `3U`.
- Vérifie que `tous.size()` vaut `3U`.
- Vérifie que `std::is_sorted(tous.begin(), tous.end())` est vrai.
- Vérifie que `drapeaux.isSet("donjon/chest@1,1")` est faux.
- Vérifie que `drapeaux.size()` vaut `2U`.

## test_quest.cpp

### QuestFlagsTest.UnDrapeauAValeursEstType

*Critique · Unitaire · Quetes* — `Source/Test/Unit/Core/Gameplay/test_quest.cpp:66`

Un drapeau a valeurs est type.

**Étapes**

1. Declarer `quete.pommes` a quatre valeurs, initiale `inconnue`.
2. Lire, poser `acceptee`, puis une valeur absente, puis le poser sans valeur.
3. L'effacer.

**Résultat attendu**

- Vérifie que `drapeaux.declare("quete.pommes", VALEURS, "inconnue")` est vrai.
- Vérifie que `drapeaux.value("quete.pommes")` vaut `"inconnue"`.
- Vérifie que `drapeaux.isSet("quete.pommes")` est faux.
- Vérifie que `drapeaux.setValue("quete.pommes", "acceptee")` est vrai.
- Vérifie que `drapeaux.value("quete.pommes")` vaut `"acceptee"`.
- Vérifie que `drapeaux.revision()` est strictement supérieur à `avant`.
- Vérifie que `drapeaux.setValue("quete.pommes", "accepte")` est faux.
- Vérifie que `drapeaux.set("quete.pommes")` est faux.
- Vérifie que `drapeaux.value("quete.pommes")` vaut `"acceptee"`.
- Vérifie que `drapeaux.revision()` vaut `apres`.
- Vérifie que `drapeaux.value("quete.pommes")` vaut `"inconnue"`.
- Vérifie que `drapeaux.setValue("non-declare", "x")` est faux.
- Vérifie que `drapeaux.declare("vide", {}, "x")` est faux.
- Vérifie que `drapeaux.declare("hors", VALEURS, "absente")` est faux.

### QuestFlagsTest.UneConditionSurDrapeauSeLitEtSEvalue

*Critique · Unitaire · Quetes* — `Source/Test/Unit/Core/Gameplay/test_quest.cpp:102`

Une condition sur drapeau se lit et s'evalue.

**Étapes**

1. Lire `isSet`, `equals` (une valeur, une liste) et `notEquals`.
2. Les evaluer sur un drapeau declare a son initiale, puis a `condamne`.
3. Lire des formes fautives.

**Résultat attendu**

- Vérifie que `drapeaux.declare("quete.pommes", VALEURS, "inconnue")` est vrai.
- Vérifie que `initiale.test` vaut `core::FlagTest::Equals`.
- Vérifie que `absent.test` vaut `core::FlagTest::IsUnset`.
- Vérifie que `core::describeFlagCondition(parmi)` vaut `"quete.pommes == condamne|enfant-libere"`.
- Vérifie que `initiale.holds(drapeaux)` est vrai.
- Vérifie que `parmi.holds(drapeaux)` est faux.
- Vérifie que `sauf.holds(drapeaux)` est vrai.
- Vérifie que `absent.holds(drapeaux)` est vrai.
- Vérifie que `drapeaux.setValue("quete.pommes", "condamne")` est vrai.
- Vérifie que `initiale.holds(drapeaux)` est faux.
- Vérifie que `parmi.holds(drapeaux)` est vrai.
- Vérifie que `sauf.holds(drapeaux)` est faux.
- Vérifie que `absent.holds(drapeaux)` est faux.
- Vérifie que `lue.condition.has_value()` est faux.
- Vérifie que `lue.error.empty()` est faux.
- Vérifie que `core::splitFlagValues(" acceptee | condamne||acceptee ")` vaut `(std::vector<std::string>{"acceptee", "condamne"})`.

### QuestLoadTest.UnPointeurJsonDonneLaLigneDeSaValeur

*Majeur · Unitaire · Donnees* — `Source/Test/Unit/Core/Gameplay/test_quest.cpp:151`

Un pointeur JSON donne la ligne de sa valeur.

**Étapes**

1. Chercher `/steps/2/when`, `/steps/1` et `/id` dans la quete d'essai.
2. Chercher un chemin absent.

**Résultat attendu**

- Vérifie que `core::positionOfPointer(QUETE_VALIDE, Pointeur("/id")).line` vaut `2`.
- Vérifie que `core::positionOfPointer(QUETE_VALIDE, Pointeur("/steps/1")).line` vaut `8`.
- Vérifie que `core::positionOfPointer(QUETE_VALIDE, Pointeur("/steps/2/when")).line` vaut `11`.
- Vérifie que `core::positionOfPointer(QUETE_VALIDE, Pointeur("/steps/9")).line` vaut `0`.

### QuestLoadTest.UneQueteBienFormeeSeLit

*Critique · Unitaire · Quetes* — `Source/Test/Unit/Core/Gameplay/test_quest.cpp:169`

Une quete bien formee se lit.

**Étapes**

1. Lire la quete d'essai a trois etapes.

**Résultat attendu**

- Vérifie que `quete.id` vaut `"essai"`.
- Vérifie que `quete.flags.size()` vaut `1U`.
- Vérifie que `quete.flags.front().initial` vaut `"inconnue"`.
- Vérifie que `quete.steps.size()` vaut `3U`.
- Vérifie que `quete.steps[2].outcome` vaut `core::QuestOutcome::Success`.
- Vérifie que `quete.steps[2].effects.size()` vaut `1U`.
- Vérifie que `quete.steps[2].effects.front().flag` vaut `"essai/recompense"`.
- Vérifie que `core::questTextKeys(quete)` vaut `(std::vector<std::string>{"quest.essai.title", "quest.essai.acceptee", "quest.essai.garde-vu", "quest.essai.rendue"})`.

### QuestLoadTest.UneQueteMalFormeeEstRefuseeEnNommantLaLigne

*Critique · Unitaire · Quetes* — `Source/Test/Unit/Core/Gameplay/test_quest.cpp:194`

Une quete mal formee est refusee en nommant la ligne.

**Étapes**

1. Lire un JSON a la virgule manquante.
2. Lire une quete dont la deuxieme etape compare une valeur non declaree et dont la troisieme n'a pas de condition.

**Résultat attendu**

- Vérifie que `syntaxe.quest.has_value()` est faux.
- Vérifie que `syntaxe.errors.size()` vaut `1U`.
- Vérifie que `syntaxe.errors.front().find("quetes/x.json:4")` diffère de `std::string::npos`.
- Vérifie que `sens.quest.has_value()` est faux.
- Vérifie que `sens.errors.size()` vaut `2U`.
- Vérifie que `sens.errors[0].find("quetes/fautive.json:8")` diffère de `std::string::npos`.
- Vérifie que `sens.errors[0].find("'c'")` diffère de `std::string::npos`.
- Vérifie que `sens.errors[1].find("quetes/fautive.json:9")` diffère de `std::string::npos`.
- Vérifie que `sens.errors[1].find("'when'")` diffère de `std::string::npos`.

### QuestLoadTest.LeCatalogueDesQuetesRefuseLesDoublons

*Majeur · Unitaire · Quetes* — `Source/Test/Unit/Core/Gameplay/test_quest.cpp:233`

Le catalogue des quetes refuse les doublons.

**Étapes**

1. Ecrire trois quetes dans un dossier temporaire : une valide, une dont le nom de fichier differe de l'identifiant, une qui redeclare le drapeau de la premiere.
2. Charger le dossier, puis un dossier absent.

**Résultat attendu**

- Vérifie que `catalogue.quests.size()` vaut `1U`.
- Vérifie que `catalogue.quests.front().id` vaut `"essai"`.
- Vérifie que `catalogue.errors.size()` vaut `2U`.
- Vérifie que `catalogue.errors[0].find("mal-nommee.json")` diffère de `std::string::npos`.
- Vérifie que `catalogue.errors[1].find("quete.essai")` diffère de `std::string::npos`.
- Vérifie que `catalogue.findFlag("quete.essai")` diffère de `nullptr`.
- Vérifie que `vide.quests.empty()` est vrai.
- Vérifie que `vide.errors.empty()` est vrai.

### QuestAdvanceTest.UneQueteDeTroisEtapesAvanceParLesDrapeaux

*Critique · Unitaire · Quetes* — `Source/Test/Unit/Core/Gameplay/test_quest.cpp:275`

Une quete de trois etapes avance par les drapeaux.

**Étapes**

1. Declarer les drapeaux de la quete d'essai.
2. Poser tour a tour `acceptee`, `garde-vu`, `rendue`, en faisant avancer les quetes a chaque fois.
3. Faire avancer une fois de plus.

**Résultat attendu**

- Vérifie que `core::advanceQuests(catalogue, drapeaux).empty()` est vrai.
- Vérifie que `core::questProgress(catalogue.quests.front(), drapeaux).status` vaut `core::QuestStatus::NotStarted`.
- Vérifie que `drapeaux.setValue("quete.essai", valeur)` est vrai.
- Vérifie que `evenements.size()` vaut `1U`.
- Vérifie que `evenements.front()` vaut `(core::QuestEvent{"essai", valeur, issue})`.
- Vérifie que `drapeaux.isSet("essai/recompense")` est vrai.
- Vérifie que `fin.status` vaut `core::QuestStatus::Succeeded`.
- Vérifie que `fin.reachedSteps` vaut `(std::vector<std::string>{"acceptee", "garde-vu", "rendue"})`.
- Vérifie que `core::advanceQuests(catalogue, drapeaux).empty()` est vrai.

### QuestAdvanceTest.LesUsagesDeDrapeauxSontConfrontesAuxDeclarations

*Majeur · Unitaire · Quetes* — `Source/Test/Unit/Core/Gameplay/test_quest.cpp:312`

Les usages de drapeaux sont confrontes aux declarations.

**Étapes**

1. Lire un dialogue qui pose `quete.essai = acceptee`, compare a `accepte` (faute) et pose `quete.essai` sans valeur.
2. Le confronter a la quete d'essai.
3. Relever ce qu'il pose et ce qu'il lit.

**Résultat attendu**

- Vérifie que `lu.graph.has_value()` est vrai.
- Vérifie que `erreurs.size()` vaut `2U`.
- Vérifie que `erreurs[0].find("'accepte'")` diffère de `std::string::npos`.
- Vérifie que `erreurs[1].find("'value'")` diffère de `std::string::npos`.
- Vérifie que `erreurs[1].find("dialogue 'mere'")` diffère de `std::string::npos`.
- Vérifie que `poses.contains("quete.essai")` est vrai.
- Vérifie que `poses.contains(core::questStepFlag("essai", "rendue"))` est vrai.
- Vérifie que `poses.contains("essai/recompense")` est vrai.
- Vérifie que `std::ranges::all_of( lus, [](const core::FlagRead& lu) { return lu.flag == "quete.essai"; })` est vrai.
- Vérifie que `lus.size()` vaut `4U`.
