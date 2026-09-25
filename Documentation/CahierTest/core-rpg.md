# Core · Rpg

Tests unitaires — **85 cas** (3 bloquants, 48 critiques, 33 majeurs, 1 mineur). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_bestiary.cpp`](#test-bestiarycpp) | 6 | - | 3 | 3 | - |
| [`test_character_options.cpp`](#test-character-optionscpp) | 9 | - | 5 | 4 | - |
| [`test_character_sheet.cpp`](#test-character-sheetcpp) | 10 | - | 6 | 4 | - |
| [`test_check.cpp`](#test-checkcpp) | 5 | - | 4 | 1 | - |
| [`test_dialogue.cpp`](#test-dialoguecpp) | 15 | 3 | 7 | 5 | - |
| [`test_dice.cpp`](#test-dicecpp) | 9 | - | 5 | 4 | - |
| [`test_equipment.cpp`](#test-equipmentcpp) | 7 | - | 6 | 1 | - |
| [`test_inventory.cpp`](#test-inventorycpp) | 13 | - | 5 | 7 | 1 |
| [`test_multiclassing.cpp`](#test-multiclassingcpp) | 6 | - | 4 | 2 | - |
| [`test_rpg_enums.cpp`](#test-rpg-enumscpp) | 5 | - | 3 | 2 | - |

## Exigences vérifiées par cette page

Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la [matrice de traçabilité](couverture-exigences.md) les rassemble toutes.

| Exigence | Cas |
|---|---|
| `EX-CNT-011` | [`DiceTest.AllerRetourDesCaracteristiques`](#dicetestallerretourdescaracteristiques), [`RpgEnumsTest.UnNomInconnuEstRefuse`](#rpgenumstestunnominconnuestrefuse), [`RpgEnumsTest.LesEnumerationsCoincidentAvecLesSchemas`](#rpgenumstestlesenumerationscoincidentaveclesschemas) |
| `EX-CNT-031` | [`BestiaryTest.LesMecanismesExigesSontAnnonces`](#bestiarytestlesmecanismesexigessontannonces), [`CharacterOptionsTest.LesMecanismesExigesSontAnnonces`](#characteroptionstestlesmecanismesexigessontannonces) |
| `EX-CNT-032` | [`CharacterOptionsTest.LesClassesProvisoiresNeSontReferenceesParRien`](#characteroptionstestlesclassesprovisoiresnesontreferenceesparrien) |
| `EX-NFR-002` | [`DiceTest.RejouabiliteStricte`](#dicetestrejouabilitestricte) |
| `EX-REG-003` | [`CheckTest.ModificateursEtRestitution`](#checktestmodificateursetrestitution) |
| `EX-RPG-042` | [`DialogueTest.UnDialogueEstRefuseFauteDeLangueCommune`](#dialoguetestundialogueestrefusefautedelanguecommune) |

## test_bestiary.cpp

### BestiaryTest.LesQuatreVingtQuatorzeProfilsChargent

*Critique · Unitaire · Bestiaire* — `Source/Test/Unit/Core/Rpg/test_bestiary.cpp:96`

Les 94 creatures du bestiaire de base se chargent toutes.

**Étapes**

1. Charger Source/Elements/Rpg/creatures.
2. Compter les creatures chargees et les erreurs rapportees.

**Résultat attendu**

- Vérifie que `catalogue.creatures.size()` vaut `BETES_DU_SRD`.
- Vérifie que `identifiants.insert(creature.id).second` est vrai.
- Vérifie que `creature.name.empty()` est faux.
- Vérifie que `creature.source` vaut `"srd"`.

### BestiaryTest.DixProfilsConcordentAvecLeLivre

*Critique · Unitaire · Bestiaire* — `Source/Test/Unit/Core/Rpg/test_bestiary.cpp:123`

Dix blocs de statistiques du bestiaire concordent avec le livre.

**Étapes**

1. Pour douze creatures, comparer classe d'armure, points de vie, vitesse de marche, facteur de puissance et degats de l'attaque principale aux valeurs lues sur Animaux.pdf.

**Résultat attendu**

- Vérifie que `creature` diffère de `nullptr`.
- Vérifie que `creature->armorClass` vaut `attendu.armorClass`.
- Vérifie que `creature->hitPoints` vaut `attendu.hitPoints`.
- Vérifie que `creature->speed.walk` vaut `attendu.walk` (comparaison flottante).
- Vérifie que `creature->challengeRating` vaut `attendu.challengeRating` (comparaison flottante).
- Vérifie que `action` diffère de `nullptr`.
- Vérifie que `action->damage.has_value()` est vrai.
- Vérifie que `des.has_value()` est vrai.
- Vérifie que `*action->damage` vaut `*des`.
- Vérifie que `action->damageType.has_value()` est vrai.
- Vérifie que `*action->damageType` vaut `attendu.damageType`.

### BestiaryTest.ChaqueProfilEstComplet

*Critique · Unitaire · Bestiaire* — `Source/Test/Unit/Core/Rpg/test_bestiary.cpp:163`

Aucun profil du bestiaire n'a de caracteristique ni de vitesse manquante.

**Étapes**

1. Pour chaque creature, verifier que les six caracteristiques sont dans [1, 30].
2. Verifier que la vitesse de marche est renseignee, meme nulle.

**Résultat attendu**

- Vérifie que `valeur` est supérieur ou égal à `1`.
- Vérifie que `valeur` est inférieur ou égal à `30`.
- Vérifie que `creature.speed.walk` est supérieur ou égal à `0.0F`.
- Vérifie que `creature.hitPoints` est strictement supérieur à `0`.
- Vérifie que `creature.armorClass` est strictement supérieur à `0`.

### BestiaryTest.UneVitesseNulleSignifieUnAutreDeplacement

*Majeur · Unitaire · Bestiaire* — `Source/Test/Unit/Core/Rpg/test_bestiary.cpp:187`

Une creature sans vitesse de marche possede un autre mode de deplacement.

**Étapes**

1. Relever les creatures dont `walk` vaut 0.

**Résultat attendu**

- Vérifie que `autrement` est vrai.
- Vérifie que `immobiles` est strictement supérieur à `0U`.

### BestiaryTest.LesMecanismesExigesSontAnnonces

*Majeur · Unitaire · Bestiaire* — `Source/Test/Unit/Core/Rpg/test_bestiary.cpp:215`

Exigences : `EX-CNT-031`

Le bestiaire annonce les mecanismes que les creatures exigent du moteur.

**Étapes**

1. Collecter les `mecanismesRequis` de toutes les creatures chargees.

**Résultat attendu**

- Vérifie que `std::ranges::find(mecanismes, "resistance-conditionnelle")` diffère de `mecanismes.end()`.
- Vérifie que `std::ranges::find(mecanismes, "langue-comprise-non-parlee")` diffère de `mecanismes.end()`.

### BestiaryTest.UnDossierAbsentEstSignale

*Majeur · Unitaire · Bestiaire* — `Source/Test/Unit/Core/Rpg/test_bestiary.cpp:236`

Charger un dossier de creatures inexistant produit une erreur nommee.

**Étapes**

1. Charger un chemin qui n'existe pas.

**Résultat attendu**

- Vérifie que `vide.creatures.empty()` est vrai.
- Vérifie que `vide.errors.size()` vaut `1U`.
- Vérifie que `vide.errors.front().find("dossier-inexistant")` diffère de `std::string::npos`.

## test_character_options.cpp

### CharacterOptionsTest.LesTroisCataloguesChargent

*Critique · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:89`

Les especes, historiques et classes livres se chargent tous.

**Étapes**

1. Charger species/, backgrounds/ et classes/.

**Résultat attendu**

- Vérifie que `lues.species.empty()` est faux.
- Vérifie que `lues.backgrounds.empty()` est faux.
- Vérifie que `lues.classes.size()` vaut `CLASSES.size()`.
- Vérifie que `identifiants.insert(espece.id).second` est vrai.
- Vérifie que `espece.name.empty()` est faux.
- Vérifie que `espece.speed` est strictement supérieur à `0.0F`.

### CharacterOptionsTest.LesEspecesConcordentAvecLesLivres

*Critique · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:115`

Les especes livrees portent les valeurs des livres dont elles sont tirees.

**Étapes**

1. Pour sept especes des trois livres, comparer provenance, taille, vitesse et augmentation de caracteristique aux valeurs lues sur les documents.

**Résultat attendu**

- Vérifie que `espece` diffère de `nullptr`.
- Vérifie que `espece->source` vaut `attendue.source`.
- Vérifie que `espece->size` vaut `attendue.size`.
- Vérifie que `espece->speed` vaut `attendue.speed` (comparaison flottante).
- Vérifie que `espece->increase(attendue.augmentee)` vaut `attendue.augmentation`.

### CharacterOptionsTest.LAugmentationSAppliqueEtResteBornee

*Majeur · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:138`

L'augmentation d'une espece s'applique et reste bornee a 20.

**Étapes**

1. Appliquer l'augmentation du nain a une Constitution de 14, puis de 19.

**Résultat attendu**

- Vérifie que `nain` diffère de `nullptr`.
- Vérifie que `regles.ok()` est vrai.
- Vérifie que `core::abilityScoreWith(*nain, core::Ability::Constitution, 14, plafond)` vaut `16`.
- Vérifie que `core::abilityScoreWith(*nain, core::Ability::Constitution, plafond - 1, plafond)` vaut `plafond`.
- Vérifie que `core::abilityScoreWith(*nain, core::Ability::Charisma, 14, plafond)` vaut `14`.

### CharacterOptionsTest.LesClassesConcordentAvecLeLivre

*Critique · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:164`

Les classes simplifiees portent le de de vie et les sauvegardes du livre.

**Étapes**

1. Pour les quatre classes, comparer de de vie, caracteristique principale et jets de sauvegarde a la table << New Simplified Classes >> de la page 57.

**Résultat attendu**

- Vérifie que `classe` diffère de `nullptr`.
- Vérifie que `classe->hitDie` vaut `attendue.hitDie`.
- Vérifie que `classe->primaryAbility.empty()` est faux.
- Vérifie que `classe->primaryAbility.front()` vaut `attendue.primaire`.
- Vérifie que `classe->savingThrowProficiencies.size()` vaut `2U`.
- Vérifie que `classe->savingThrowProficiencies[0]` vaut `attendue.sauvegarde1`.
- Vérifie que `classe->savingThrowProficiencies[1]` vaut `attendue.sauvegarde2`.

### CharacterOptionsTest.LaProgressionVientDeLaDonnee

*Critique · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:189`

Le bonus de maitrise des niveaux 1 a 5 est lu dans la table livree.

**Étapes**

1. Lire le bonus de maitrise des niveaux 1 a 5 de chaque classe dans sa table.
2. Le comparer aux valeurs de la table du livre.

**Résultat attendu**

- Vérifie que `classe` diffère de `nullptr`.
- Vérifie que `classe->progression.size()` vaut `20U`.
- Vérifie que `ligne` diffère de `nullptr`.
- Vérifie que `ligne->proficiencyBonus` vaut `BONUS_ATTENDUS[static_cast<std::size_t>(niveau - 1)]`.
- Vérifie que `ligne->features.empty()` est faux.
- Vérifie que `classe->progression[i].level` vaut `static_cast<int>(i) + 1`.

### CharacterOptionsTest.LesMecanismesExigesSontAnnonces

*Majeur · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:225`

Exigences : `EX-CNT-031`

Une espece qui exige un mecanisme absent le declare au chargement.

**Étapes**

1. Collecter les `mecanismesRequis` des especes chargees.

**Résultat attendu**

- Vérifie que `std::ranges::find(mecanismes, "augmentation-de-caracteristique-au-choix")` diffère de `mecanismes.end()`.

### CharacterOptionsTest.LesClassesProvisoiresNeSontReferenceesParRien

*Critique · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:243`

Exigences : `EX-CNT-032`

Les classes provisoires portent leur marque et ne sont referencees par aucune donnee definitive.

**Étapes**

1. Verifier que les quatre classes portent `status.provisoire` et un critere de retrait.
2. Balayer tous les fichiers de `Source/Elements/Rpg/` et chercher leur identifiant.

**Résultat attendu**

- Vérifie que `provisoires.size()` vaut `CLASSES.size()`.
- Vérifie que `classe.status.provisional` est vrai.
- Vérifie que `classe.status.removalCriterion.empty()` est faux.
- Vérifie que `contenu.find('"' + identifiant + '"')` vaut `std::string::npos`.

### CharacterOptionsTest.UnDossierAbsentEstSignale

*Majeur · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:299`

Charger un dossier d'options inexistant produit une erreur nommee.

**Étapes**

1. Charger trois chemins qui n'existent pas.

**Résultat attendu**

- Vérifie que `vides.species.empty()` est vrai.
- Vérifie que `vides.backgrounds.empty()` est vrai.
- Vérifie que `vides.classes.empty()` est vrai.
- Vérifie que `vides.errors.size()` vaut `3U`.

### CharacterOptionsTest.LesHistoriquesCitentDesCompetencesQuiExistent

*Majeur · Unitaire · Options de personnage* — `Source/Test/Unit/Core/Rpg/test_character_options.cpp:317`

Les competences citees par un historique existent au catalogue.

**Étapes**

1. Pour chaque historique, verifier que le fichier de chaque competence citee existe dans `Source/Elements/Rpg/skills/`.

**Résultat attendu**

- Vérifie que `historique.skillProficiencies.empty()` est faux.
- Vérifie que `std::filesystem::exists(chemin)` est vrai.
- Vérifie que `historique.languageCount` est supérieur ou égal à `0`.

## test_character_sheet.cpp

### CharacterSheetTest.LesTablesDeReglesSeChargent

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:69`

La table d'experience et les constantes de creation se chargent depuis la donnee.

**Étapes**

1. Charger rules/experience.json et rules/character-creation.json.

**Résultat attendu**

- Vérifie que `lus.experience.levels.size()` vaut `20U`.
- Vérifie que `lus.experience.maximumLevel()` vaut `20`.
- Vérifie que `lus.experience.levels[i].experience` est strictement supérieur à `lus.experience.levels[i - 1].experience`.
- Vérifie que `lus.experience.levels[i].proficiencyBonus` est supérieur ou égal à `lus.experience.levels[i - 1].proficiencyBonus`.
- Vérifie que `lus.rules.ok()` est vrai.
- Vérifie que `lus.skills.skills.empty()` est faux.

### CharacterSheetTest.QuatreFichesIndependantes

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:100`

Quatre fiches de personnage coexistent, chacune avec ses propres valeurs.

**Étapes**

1. Construire quatre fiches d'especes et de classes differentes.
2. Modifier les points de vie de la premiere et faire monter la deuxieme de niveau.

**Résultat attendu**

- Vérifie que `groupe.size()` vaut `4U`.
- Vérifie que `personnage.speciesId.empty()` est faux.
- Vérifie que `personnage.classId.empty()` est faux.
- Vérifie que `personnage.backgroundId.empty()` est faux.
- Vérifie que `personnage.maximumHitPoints` est strictement supérieur à `0`.
- Vérifie que `personnage.speedMeters` est strictement supérieur à `0.0F`.
- Vérifie que `groupe[0].maximumHitPoints` vaut `pvAvant`.
- Vérifie que `groupe[0].level` vaut `niveauAvant`.
- Vérifie que `groupe[1].level` vaut `3`.
- Vérifie que `groupe[2].level` vaut `1`.
- Vérifie que `groupe[3].level` vaut `1`.
- Vérifie que `groupe[0].currentHitPoints` diffère de `groupe[0].maximumHitPoints`.
- Vérifie que `groupe[2].currentHitPoints` vaut `groupe[2].maximumHitPoints`.

### CharacterSheetTest.TroisClassesDonnentLesBonsModificateurs

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:145`

Trois classes chargees depuis JSON produisent les bons points de vie et jets de sauvegarde.

**Étapes**

1. Construire une fiche pour le brawler (d12), le mage (d6) et le priest (d8).
2. Comparer points de vie et modificateurs de sauvegarde au calcul du livre.

**Résultat attendu**

- Vérifie que `classe` diffère de `nullptr`.
- Vérifie que `personnage.modifier(core::Ability::Constitution)` vaut `modificateurAttendu`.
- Vérifie que `personnage.maximumHitPoints` vaut `classe->hitDie + modificateurAttendu`.
- Vérifie que `personnage.currentHitPoints` vaut `personnage.maximumHitPoints`.
- Vérifie que `maitrise` vaut `lus.experience.proficiencyBonusAt(1)`.
- Vérifie que `core::savingThrowModifier(personnage, lus.experience, caracteristique)` vaut `personnage.modifier(caracteristique) + maitrise`.
- Vérifie que `core::savingThrowModifier(personnage, lus.experience, caracteristique)` vaut `personnage.modifier(caracteristique)`.

### CharacterSheetTest.LaMonteeDeNiveauEstTesteeAuxBornes

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:196`

La montee de niveau franchit le seuil exact, le depassement et plusieurs niveaux d'un coup.

**Étapes**

1. Donner exactement le seuil du niveau 2, puis un point de moins, puis le seuil du niveau 5 d'un coup.

**Résultat attendu**

- Vérifie que `classe` diffère de `nullptr`.
- Vérifie que `seuil2` est strictement supérieur à `0`.
- Vérifie que `seuil5` est strictement supérieur à `seuil2`.
- Vérifie que `montee.gainedLevel()` est vrai.
- Vérifie que `montee.previousLevel` vaut `1`.
- Vérifie que `montee.newLevel` vaut `2`.
- Vérifie que `montee.hitPointsGained` est strictement supérieur à `0`.
- Vérifie que `sansMontee.gainedLevel()` est faux.
- Vérifie que `presque.level` vaut `1`.
- Vérifie que `sansMontee.hitPointsGained` vaut `0`.
- Vérifie que `saut.newLevel` vaut `5`.
- Vérifie que `bond.level` vaut `5`.
- Vérifie que `saut.newProficiencyBonus` est strictement supérieur à `saut.previousProficiencyBonus`.
- Vérifie que `bond.maximumHitPoints` vaut `core::maximumHitPointsFor(classe->hitDie, 5, bond.modifier(core::Ability::Constitution))`.
- Vérifie que `rien.gainedLevel()` est faux.
- Vérifie que `stable.experiencePoints` vaut `seuil5`.

### CharacterSheetTest.LaMonteeDeNiveauEstReproductible

*Majeur · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:255`

La montee de niveau est reproductible : le chemin ne change pas le resultat.

**Étapes**

1. Donner le seuil du niveau 5 en une fois a une fiche, en deux fois a une autre.

**Résultat attendu**

- Vérifie que `classe` diffère de `nullptr`.
- Vérifie que `enUneFois.level` vaut `enDeuxFois.level`.
- Vérifie que `enUneFois.maximumHitPoints` vaut `enDeuxFois.maximumHitPoints`.
- Vérifie que `enUneFois.experiencePoints` vaut `enDeuxFois.experiencePoints`.

### CharacterSheetTest.LeModificateurDeCompetenceVientDuCatalogue

*Majeur · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:282`

Un jet de competence emploie la caracteristique que le catalogue lui donne.

**Étapes**

1. Calculer le modificateur d'une competence maitrisee et d'une autre non maitrisee.
2. Demander une competence inconnue du catalogue.

**Résultat attendu**

- Vérifie que `moine.skillProficiencies.empty()` est faux.
- Vérifie que `competence` diffère de `nullptr`.
- Vérifie que `calcul.found` est vrai.
- Vérifie que `calcul.proficient` est vrai.
- Vérifie que `calcul.value` vaut `moine.modifier(competence->ability) + core::proficiencyBonus(moine, lus.experience)`.
- Vérifie que `inconnue.found` est faux.

### CharacterSheetTest.LeHerosDeLaDemoEstLaFicheDuLivre

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:317`

La fiche du heros redonne les nombres du livre, et sa Persuasion a DD 18 reussit une fois sur dix.

**Étapes**

1. Charger Rpg/characters/heros-brawler.json.
2. Comparer caracteristiques, points de vie, sauvegardes et competences a la fiche du Player's Guide to Tanares, p. 195.
3. Compter les faces du d20 qui font reussir la Persuasion a DD 18.

**Résultat attendu**

- Vérifie que `charge.errors.empty()` est vrai.
- Vérifie que `heros.level` vaut `1`.
- Vérifie que `heros.abilities` vaut `(std::array<int, 6>{16, 13, 16, 10, 12, 8})`.
- Vérifie que `heros.maximumHitPoints` vaut `15`.
- Vérifie que `core::savingThrowModifier(heros, lus.experience, core::Ability::Strength)` vaut `5`.
- Vérifie que `core::savingThrowModifier(heros, lus.experience, core::Ability::Constitution)` vaut `5`.
- Vérifie que `calcul.proficient` est vrai.
- Vérifie que `calcul.value` vaut `valeur`.
- Vérifie que `persuasion.proficient` est faux.
- Vérifie que `persuasion.value` vaut `-1`.
- Vérifie que `faces` vaut `2`.

### CharacterSheetTest.LaVitesseSeLitEnCases

*Majeur · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:370`

La vitesse d'un personnage se lit en cases via l'echelle unique du projet.

**Étapes**

1. Convertir la vitesse d'un nain (7,50 m) et d'un humain (9 m) en cases.

**Résultat attendu**

- Vérifie que `nain.speedInTiles()` vaut `core::tilesFromMeters(nain.speedMeters)` (comparaison flottante).
- Vérifie que `humain.speedInTiles()` vaut `core::tilesFromMeters(humain.speedMeters)` (comparaison flottante).
- Vérifie que `nain.speedInTiles()` vaut `5.0F` (comparaison flottante).
- Vérifie que `humain.speedInTiles()` vaut `6.0F` (comparaison flottante).

### CharacterSheetTest.LeComposantDistingueLAbsenceDeFiche

*Majeur · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:389`

Une entite sans fiche ne se confond pas avec une entite liee a la fiche zero.

**Étapes**

1. Construire un RpgActor par defaut, puis un lie a l'indice 0.

**Résultat attendu**

- Vérifie que `sansFiche.hasSheet()` est faux.
- Vérifie que `premiere.hasSheet()` est vrai.
- Vérifie que `premiere.sheetIndex` vaut `0U`.

### CharacterSheetTest.AucuneValeurDeRegleNEstEcriteDansLeCpp

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/Core/Rpg/test_character_sheet.cpp:409`

La classe d'armure sans armure et le plafond de caracteristique viennent de la donnee.

**Étapes**

1. Lire les deux constantes dans rules/character-creation.json.
2. Verifier que la CA d'une fiche sans armure vaut la constante lue plus le modificateur de Dexterite.

**Résultat attendu**

- Vérifie que `lus.rules.ok()` est vrai.
- Vérifie que `personnage.armorClass` vaut `lus.rules.unarmoredArmorClass + personnage.modifier(core::Ability::Dexterity)`.
- Vérifie que `personnage.ability(caracteristique)` est inférieur ou égal à `lus.rules.maximumAbilityScore`.

## test_check.cpp

### CheckTest.AvantageEtDesavantageGardentLeBonDe

*Critique · Unitaire · Jet de d20* — `Source/Test/Unit/Core/Rpg/test_check.cpp:47`

L'avantage garde le meilleur de deux des, le desavantage le pire.

**Étapes**

1. Lancer mille jets avec avantage, puis mille avec desavantage.

**Résultat attendu**

- Vérifie que `avantage.dice.size()` vaut `2U`.
- Vérifie que `avantage.keptDie` vaut `std::max(avantage.dice[0], avantage.dice[1])`.
- Vérifie que `desavantage.dice.size()` vaut `2U`.
- Vérifie que `desavantage.keptDie` vaut `std::min(desavantage.dice[0], desavantage.dice[1])`.

### CheckTest.AvantageEtDesavantageSAnnulent

*Critique · Unitaire · Jet de d20* — `Source/Test/Unit/Core/Rpg/test_check.cpp:72`

Avantage et desavantage s'annulent entierement, et ne se cumulent jamais.

**Étapes**

1. Determiner la posture pour 0/0, 1/0, 0/1, 1/1 et 2/1 sources.

**Résultat attendu**

- Vérifie que `core::rollStance(0, 0)` vaut `core::RollStance::Normal`.
- Vérifie que `core::rollStance(1, 0)` vaut `core::RollStance::Advantage`.
- Vérifie que `core::rollStance(0, 1)` vaut `core::RollStance::Disadvantage`.
- Vérifie que `core::rollStance(1, 1)` vaut `core::RollStance::Normal`.
- Vérifie que `core::rollStance(2, 1)` vaut `core::RollStance::Normal`.
- Vérifie que `core::rollStance(1, 3)` vaut `core::RollStance::Normal`.
- Vérifie que `core::rollStance(3, 0)` vaut `core::RollStance::Advantage`.
- Vérifie que `resultat.dice.size()` vaut `1U`.

### CheckTest.NaturelsDistinguesDuTotal

*Critique · Unitaire · Jet de d20* — `Source/Test/Unit/Core/Rpg/test_check.cpp:99`

Un 1 ou 20 naturel se distingue d'un total de 1 ou 20.

**Étapes**

1. Lancer jusqu'a obtenir un 20 naturel et un 1 naturel.
2. Construire un resultat de total 20 obtenu avec un de de 8.

**Résultat attendu**

- Vérifie que `resultat.isNaturalTwenty()` est vrai.
- Vérifie que `resultat.isNaturalOne()` est faux.
- Vérifie que `resultat.isNaturalOne()` est vrai.
- Vérifie que `resultat.isNaturalTwenty()` est faux.
- Vérifie que `vingtVu` est vrai.
- Vérifie que `unVu` est vrai.
- Vérifie que `total20.isNaturalTwenty()` est faux.

### CheckTest.ModificateursEtRestitution

*Critique · Unitaire · Jet de d20* — `Source/Test/Unit/Core/Rpg/test_check.cpp:139`

Exigences : `EX-REG-003`

Les modificateurs d'un jet s'appliquent et conservent leur origine.

**Étapes**

1. Lancer un jet avec +3 de Dexterite et +2 de maitrise.
2. Lire la restitution.

**Résultat attendu**

- Vérifie que `cible` vaut `15`.
- Vérifie que `resultat.total` vaut `resultat.keptDie + 5`.
- Vérifie que `resultat.target` vaut `cible`.
- Vérifie que `resultat.succeeded()` vaut `resultat.total >= cible`.
- Vérifie que `restitution.find("Dexterite")` diffère de `std::string::npos`.
- Vérifie que `restitution.find("maitrise")` diffère de `std::string::npos`.
- Vérifie que `restitution.find(std::to_string(cible))` diffère de `std::string::npos`.
- Vérifie que `restitution.find(std::to_string(resultat.total))` diffère de `std::string::npos`.

### CheckTest.EchelleDeDifficulteEnDonnee

*Majeur · Unitaire · Jet de d20* — `Source/Test/Unit/Core/Rpg/test_check.cpp:173`

L'echelle de difficulte est une donnee complete et croissante.

**Étapes**

1. Lire les paliers de difficulty.json.

**Résultat attendu**

- Vérifie que `document.ok()` est vrai.
- Vérifie que `paliers.size()` vaut `6U`.
- Vérifie que `dc` est strictement supérieur à `precedent`.
- Vérifie que `seuil("tres-facile")` vaut `5`.
- Vérifie que `seuil("quasi-impossible")` vaut `30`.

## test_dialogue.cpp

### DialogueTest.LeDialogueDeDemonstrationSeChargeEtSesReferencesExistent

*Bloquant · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:133`

Le dialogue du heraut se charge et ses references existent.

**Étapes**

1. Charger Source/Elements/World/dialogues.
2. Charger competences, degres de difficulte, objets et langues.
3. Valider les references du heraut.

**Résultat attendu**

- Vérifie que `dialogues().errors.empty()` est vrai.
- Vérifie que `echelle().errors.empty()` est vrai.
- Vérifie que `heraut` diffère de `nullptr`.
- Vérifie que `heraut->nodes.size()` est supérieur ou égal à `10U`.
- Vérifie que `std::ranges::count(heraut->nodes, core::DialogueNodeKind::Condition, &core::DialogueNode::kind)` est supérieur ou égal à `2`.
- Vérifie que `std::ranges::any_of(heraut->nodes, [](const core::DialogueNode& n) { return n.kind == core::DialogueNodeKind::Check && n.skill == "persuasion"; })` est vrai.
- Vérifie que `erreurs.empty()` est vrai.

### DialogueTest.LeDialogueDuHerautSeParcourtEnHeadless

*Bloquant · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:176`

Le dialogue du heraut se parcourt sans fenetre.

**Étapes**

1. Ouvrir la conversation avec un interlocuteur parlant le commun, +20 aux jets.
2. Demander la Marque, continuer, demander l'inscription, convaincre.
3. Continuer jusqu'a la fin.
4. Rouvrir une seconde conversation sur les memes drapeaux, demander l'inscription.

**Résultat attendu**

- Vérifie que `heraut` diffère de `nullptr`.
- Vérifie que `premiere.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `premiere.lineKey()` vaut `"dialogue.heraut-colisee.presentation"`.
- Vérifie que `drapeaux.isSet(RENCONTRE)` est vrai.
- Vérifie que `identifiants(premiere.choices())` vaut `(std::vector<std::string>{"marque", "inscription", "partir"})`.
- Vérifie que `premiere.attitude()` vaut `core::DialogueAttitude::Indifferent`.
- Vérifie que `premiere.choose("marque")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `premiere.lineKey()` vaut `"dialogue.heraut-colisee.marque"`.
- Vérifie que `identifiants(premiere.choices())` vaut `(std::vector<std::string>{"continue"})`.
- Vérifie que `premiere.choices().front().textKey` vaut `"dialogue.continue"`.
- Vérifie que `premiere.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `premiere.lineKey()` vaut `"dialogue.heraut-colisee.retour"`.
- Vérifie que `premiere.choose("inscription")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `premiere.lineKey()` vaut `"dialogue.heraut-colisee.demande"`.
- Vérifie que `identifiants(demande)` vaut `(std::vector<std::string>{"convaincre", "renoncer"})`.
- Vérifie que `demande.front().checkSkill` vaut `"persuasion"`.
- Vérifie que `demande.front().checkDc` vaut `15`.
- Vérifie que `demande.back().checkDc` vaut `0`.
- Vérifie que `premiere.choose("convaincre")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `premiere.lastCheck().has_value()` est vrai.
- Vérifie que `premiere.lastCheck()->skill` vaut `"persuasion"`.
- Vérifie que `premiere.lastCheck()->result.target` vaut `15`.
- Vérifie que `premiere.lastCheck()->result.succeeded()` est vrai.
- Vérifie que `drapeaux.isSet(core::questStartedFlag("champion-du-colisee"))` est vrai.
- Vérifie que `premiere.lineKey()` vaut `"dialogue.heraut-colisee.accepte-replique"`.
- Vérifie que `premiere.attitude()` vaut `core::DialogueAttitude::Friendly`.
- Vérifie que `premiere.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `premiere.state()` vaut `core::DialogueState::Ended`.
- Vérifie que `premiere.choices().empty()` est vrai.
- Vérifie que `premiere.choose("continue")` vaut `core::ChoiceResult::NotAwaiting`.
- Vérifie que `seconde.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `seconde.lineKey()` vaut `"dialogue.heraut-colisee.retour"`.
- Vérifie que `seconde.choose("inscription")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `seconde.lineKey()` vaut `"dialogue.heraut-colisee.deja-inscrit"`.
- Vérifie que `seconde.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `seconde.state()` vaut `core::DialogueState::Ended`.
- Vérifie que `traverses.size()` est supérieur ou égal à `10U`.
- Vérifie que `contient(premiere.journal(), "jet : jet-persuasion (persuasion)")` est vrai.
- Vérifie que `contient(premiere.journal(), "quete demarree : champion-du-colisee")` est vrai.

### DialogueTest.UnEchecMeneALAutreSuiteEtFermeLaReponseConditionnelle

*Critique · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:249`

Un echec de Persuasion ferme la reponse de retentative.

**Étapes**

1. Parcourir jusqu'a la demande avec -20 aux jets.
2. Convaincre.
3. Continuer, redemander l'inscription.
4. Choisir « convaincre » malgre tout.

**Résultat attendu**

- Vérifie que `heraut` diffère de `nullptr`.
- Vérifie que `runner.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `runner.choose("inscription")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `runner.choose("convaincre")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `runner.lastCheck().has_value()` est vrai.
- Vérifie que `runner.lastCheck()->result.succeeded()` est faux.
- Vérifie que `runner.lineKey()` vaut `"dialogue.heraut-colisee.refuse-replique"`.
- Vérifie que `runner.attitude()` vaut `core::DialogueAttitude::Hostile`.
- Vérifie que `drapeaux.isSet(ECHEC)` est vrai.
- Vérifie que `drapeaux.isSet(core::questStartedFlag("champion-du-colisee"))` est faux.
- Vérifie que `runner.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `runner.choose("inscription")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `identifiants(runner.choices())` vaut `(std::vector<std::string>{"renoncer"})`.
- Vérifie que `runner.choose("convaincre")` vaut `core::ChoiceResult::Unavailable`.
- Vérifie que `runner.choose("inexistante")` vaut `core::ChoiceResult::Unavailable`.
- Vérifie que `runner.lineKey()` vaut `"dialogue.heraut-colisee.demande"`.

### DialogueTest.UnDialogueEstRefuseFauteDeLangueCommune

*Critique · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:286`

Exigences : `EX-RPG-042`

Un dialogue est refuse faute de langue commune.

**Étapes**

1. Ouvrir le heraut (commun) avec un interlocuteur qui ne parle que le nain.
2. Tenter une reponse.

**Résultat attendu**

- Vérifie que `heraut` diffère de `nullptr`.
- Vérifie que `runner.start()` vaut `core::DialogueState::Refused`.
- Vérifie que `drapeaux.size()` vaut `0U`.
- Vérifie que `runner.currentLine()` vaut `nullptr`.
- Vérifie que `runner.choices().empty()` est vrai.
- Vérifie que `runner.choose("marque")` vaut `core::ChoiceResult::NotAwaiting`.
- Vérifie que `contient(runner.journal(), "aucune langue commune (common)")` est vrai.
- Vérifie que `runner.start()` vaut `core::DialogueState::Refused`.

### DialogueTest.UnGrapheMalFormeEstRejeteAuChargement

*Bloquant · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:315`

Les graphes mal formes sont refuses au chargement.

**Étapes**

1. Lire une serie de graphes fautifs : cible inconnue, entree inconnue, choix vide, reponse sans identifiant, reponses toutes conditionnelles, cycle sans reponse, monologue en boucle, orphelin, impasse, difficulte chiffree, noeud en double, type inconnu, sans fin, sans langue.

**Résultat attendu**

- Vérifie que `lu.graph.has_value()` est faux.
- Vérifie que `contient(lu.errors, un.motif)` est vrai.
- Vérifie que `contient(lu.errors, "essai.json")` est vrai.

### DialogueTest.UneReponseAJetRateeNeSeProposePlus

*Critique · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:404`

Une reponse a jet ratee ne se propose plus.

**Étapes**

1. Lire un graphe : un menu propose « convaincre » (jet de Persuasion, moyenne), « detour » (une replique qui mene au meme jet) et « partir ».
2. Convaincre avec -20 aux jets.
3. Revenir au menu ; ouvrir une seconde conversation sur les memes drapeaux.
4. Prendre le detour.

**Résultat attendu**

- Vérifie que `lu.graph.has_value()` est vrai.
- Vérifie que `rate` vaut `"dialogue/essai/j/failed"`.
- Vérifie que `runner.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `identifiants(avant)` vaut `(std::vector<std::string>{"convaincre", "detour", "partir"})`.
- Vérifie que `avant.front().checkSkill` vaut `"persuasion"`.
- Vérifie que `avant.front().checkDc` vaut `15`.
- Vérifie que `runner.choose("convaincre")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `runner.lastCheck().has_value()` est vrai.
- Vérifie que `runner.lastCheck()->result.succeeded()` est faux.
- Vérifie que `runner.lastCheck()->alreadyFailed` est faux.
- Vérifie que `drapeaux.isSet(rate)` est vrai.
- Vérifie que `runner.lineKey()` vaut `"dialogue.essai.non"`.
- Vérifie que `runner.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `identifiants(runner.choices())` vaut `(std::vector<std::string>{"detour", "partir"})`.
- Vérifie que `runner.choose("convaincre")` vaut `core::ChoiceResult::Unavailable`.
- Vérifie que `seconde.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `identifiants(seconde.choices())` vaut `(std::vector<std::string>{"detour", "partir"})`.
- Vérifie que `seconde.choose("detour")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `seconde.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `seconde.lastCheck().has_value()` est vrai.
- Vérifie que `seconde.lastCheck()->alreadyFailed` est vrai.
- Vérifie que `seconde.lastCheck()->result.target` vaut `15`.
- Vérifie que `seconde.lineKey()` vaut `"dialogue.essai.non"`.
- Vérifie que `contient(seconde.journal(), "jet : j (persuasion) deja rate, echec")` est vrai.
- Vérifie que `hasard.nextUInt32()` vaut `copie.nextUInt32()`.

### DialogueTest.UneBouclePasseeParUnChoixEstUnHubVoulu

*Majeur · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:470`

Une boucle par une replique a reponses est acceptee.

**Étapes**

1. Lire un graphe ou une question revient au menu par une replique et une action.
2. Le jouer trois tours de boucle, puis partir.

**Résultat attendu**

- Vérifie que `lu.graph.has_value()` est vrai.
- Vérifie que `runner.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `runner.choose("encore")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `runner.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `runner.lineKey()` vaut `"dialogue.essai.menu"`.
- Vérifie que `runner.choose("partir")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `runner.state()` vaut `core::DialogueState::Ended`.
- Vérifie que `drapeaux.isSet("vu")` est vrai.

### DialogueTest.LesActionsTouchentLeMonde

*Majeur · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:507`

Les quatre actions d'un dialogue s'appliquent.

**Étapes**

1. Jouer un noeud qui pose puis retire un drapeau, donne trois torches et demarre une quete.

**Résultat attendu**

- Vérifie que `lu.graph.has_value()` est vrai.
- Vérifie que `runner.start()` vaut `core::DialogueState::Ended`.
- Vérifie que `drapeaux.isSet("porte")` est faux.
- Vérifie que `drapeaux.isSet("quest/le-puits/started")` est vrai.
- Vérifie que `receveur.recus.size()` vaut `1U`.
- Vérifie que `receveur.recus.front()` vaut `(std::pair<std::string, int>{"torche", 3})`.
- Vérifie que `pose` diffère de `journal.end()`.
- Vérifie que `retire` diffère de `journal.end()`.
- Vérifie que `pose` est strictement inférieur à `retire`.

### DialogueTest.UnDialogueSeRejoueAGraineFixee

*Majeur · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:545`

Un dialogue se rejoue a l'identique a graine fixee.

**Étapes**

1. Jouer le heraut jusqu'au jet, sans bonus, a la graine 42, deux fois.
2. Le jouer sur une plage de graines.

**Résultat attendu**

- Vérifie que `heraut` diffère de `nullptr`.
- Vérifie que `jouer(42).first` vaut `jouer(42).first`.
- Vérifie que `reussi` est vrai.
- Vérifie que `rate` est vrai.

### DialogueTest.LesDialoguesSontTraduitsEnFrancaisEtEnAnglais

*Critique · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:582`

Les dialogues sont traduits en francais et en anglais.

**Étapes**

1. Lire fr.lang et en.lang.
2. Pour chaque dialogue livre, fabriquer ses cles.
3. Ajouter les cles de l'ecran (quitter, refus, jet).

**Résultat attendu**

- Vérifie que `dialogues().dialogues.empty()` est faux.
- Vérifie que `trouve != langue->end() && !trouve->second.empty()` est vrai.

### DialogueTest.LesDegresDeDifficulteSeChargent

*Majeur · Unitaire · Jet de d20* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:618`

Les degres de difficulte se chargent.

**Étapes**

1. Charger rules/difficulty.json.
2. Charger un chemin inexistant.

**Résultat attendu**

- Vérifie que `echelle().errors.empty()` est vrai.
- Vérifie que `echelle().tiers.size()` vaut `6U`.
- Vérifie que `echelle().find("moyenne")` diffère de `nullptr`.
- Vérifie que `echelle().find("moyenne")->dc` vaut `15`.
- Vérifie que `echelle().find("quasi-impossible")` diffère de `nullptr`.
- Vérifie que `echelle().find("quasi-impossible")->dc` vaut `30`.
- Vérifie que `echelle().find("inventee")` vaut `nullptr`.
- Vérifie que `absente.tiers.empty()` est vrai.
- Vérifie que `absente.errors.empty()` est faux.

### DialogueTest.LaFicheEcouteUnPnjAvecSesLanguesEtSesModificateurs

*Critique · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:643`

La fiche ecoute un PNJ : langues et modificateurs.

**Étapes**

1. Charger la fiche de demonstration.
2. L'envelopper dans CharacterListener.
3. Interroger ses langues, ses modificateurs de Persuasion et d'Athletisme.
4. Lui donner un objet.

**Résultat attendu**

- Vérifie que `heros.errors.empty()` est vrai.
- Vérifie que `auditeur.speaks("common")` est vrai.
- Vérifie que `auditeur.speaks("orc")` est vrai.
- Vérifie que `auditeur.speaks("draconic")` est vrai.
- Vérifie que `auditeur.speaks("elvish")` est faux.
- Vérifie que `persuasion.size()` vaut `1U`.
- Vérifie que `persuasion.front().source` vaut `"charisma"`.
- Vérifie que `persuasion.front().value` vaut `heros.sheet.modifier(core::Ability::Charisma)`.
- Vérifie que `athletisme.size()` vaut `2U`.
- Vérifie que `athletisme[1].source` vaut `"maitrise"`.
- Vérifie que `athletisme[0].value + athletisme[1].value` vaut `core::skillModifier(heros.sheet, experience, competences, "athletics").value`.
- Vérifie que `auditeur.skillModifiers("inexistante").empty()` est vrai.
- Vérifie que `std::ranges::any_of(heros.inventory.backpack, [](const core::InventoryStack& s) { return s.itemId == "corde-en-soie-15-m"; })` est vrai.

### DialogueTest.UnPnjDeCarteOuvreSonDialogue

*Majeur · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:692`

Un PNJ de carte ouvre son dialogue.

**Étapes**

1. Chercher la famille « npc » dans la table des interactifs.
2. Lire trois entites : un PNJ avec dialogue, un PNJ muet, un coffre.

**Résultat attendu**

- Vérifie que `famille` diffère de `core::knownInteractableKinds().end()`.
- Vérifie que `famille->consumable` est faux.
- Vérifie que `declencheur.has_value()` est vrai.
- Vérifie que `declencheur->dialogueId` vaut `HERAUT`.
- Vérifie que `declencheur->position` vaut `(core::GridPosition{4, 2})`.
- Vérifie que `core::dialogueTriggerFor(muet).has_value()` est faux.
- Vérifie que `core::dialogueTriggerFor(coffre).has_value()` est faux.

### DialogueTest.UnDialoguePeutEngagerUneRencontreSurLaCarte

*Critique · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:729`

Un dialogue peut engager une rencontre sur la carte.

**Étapes**

1. Lire un graphe dont le noeud d'action porte `startEncounter` vers « rats-du-donjon ».
2. Le jouer avec un auditeur d'essai.
3. Lire un graphe dont l'action `startEncounter` n'a pas de champ `encounter`.

**Résultat attendu**

- Vérifie que `lu.graph.has_value()` est vrai.
- Vérifie que `runner.start()` vaut `core::DialogueState::Ended`.
- Vérifie que `receveur.rencontres.size()` vaut `1U`.
- Vérifie que `receveur.rencontres.front()` vaut `"rats-du-donjon"`.
- Vérifie que `std::ranges::find(runner.journal(), "rencontre demandee : rats-du-donjon")` diffère de `runner.journal().end()`.
- Vérifie que `refuse.graph.has_value()` est faux.

### DialogueTest.UnDialoguePeutTerminerLaDemo

*Critique · Unitaire · Dialogue* — `Source/Test/Unit/Core/Rpg/test_dialogue.cpp:766`

Un dialogue peut terminer la demo.

**Étapes**

1. Lire un graphe dont le noeud d'action porte `endDemo` vers « arene ».
2. Le jouer avec un auditeur d'essai.
3. Lire les cles de traduction du graphe.
4. Lire un graphe dont l'action `endDemo` n'a pas de champ `ending`.

**Résultat attendu**

- Vérifie que `lu.graph.has_value()` est vrai.
- Vérifie que `runner.start()` vaut `core::DialogueState::Ended`.
- Vérifie que `receveur.fins` vaut `(std::vector<std::string>{"arene"})`.
- Vérifie que `contient(runner.journal(), "fin de la demo : arene")` est vrai.
- Vérifie que `std::ranges::find(cles, core::demoEndingKey("arene"))` diffère de `cles.end()`.
- Vérifie que `core::demoEndingKey("arene")` vaut `"ending.arene"`.
- Vérifie que `refuse.graph.has_value()` est faux.

## test_dice.cpp

### DiceTest.AnalyseDesTroisFormes

*Critique · Unitaire · Des* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:30`

La notation de des est analysee sous ses trois formes.

**Étapes**

1. Analyser << 2d6+3 >>, << 1d8 >>, << 4 >> et << 1d4-1 >>.

**Résultat attendu**

- Vérifie que `avecModificateur.has_value()` est vrai.
- Vérifie que `avecModificateur->count` vaut `2`.
- Vérifie que `avecModificateur->faces` vaut `6`.
- Vérifie que `avecModificateur->modifier` vaut `3`.
- Vérifie que `simple.has_value()` est vrai.
- Vérifie que `*simple` vaut `(core::Dice{1, 8, 0})`.
- Vérifie que `fixe.has_value()` est vrai.
- Vérifie que `*fixe` vaut `(core::Dice{0, 0, 4})`.
- Vérifie que `negatif.has_value()` est vrai.
- Vérifie que `negatif->modifier` vaut `-1`.
- Vérifie que `core::formatDice(*avecModificateur)` vaut `"2d6+3"`.
- Vérifie que `core::formatDice(*negatif)` vaut `"1d4-1"`.
- Vérifie que `core::formatDice(*fixe)` vaut `"4"`.

### DiceTest.UneNotationMalFormeeEstRefusee

*Critique · Unitaire · Des* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:63`

Une notation de des mal formee est refusee au lieu d'etre devinee.

**Étapes**

1. Analyser << ld8 >> (faute d'OCR), une chaine vide, << 1d >>, << d6 >> et << 1d6+2x >>.

**Résultat attendu**

- Vérifie que `core::parseDice("ld8").has_value()` est faux.
- Vérifie que `core::parseDice("").has_value()` est faux.
- Vérifie que `core::parseDice("1d").has_value()` est faux.
- Vérifie que `core::parseDice("d6").has_value()` est faux.
- Vérifie que `core::parseDice("1d6+2x").has_value()` est faux.
- Vérifie que `core::parseDice("0d6").has_value()` est faux.
- Vérifie que `core::parseDice("1d0").has_value()` est faux.

### DiceTest.UnLancerResteDansSesBornes

*Critique · Unitaire · Des* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:85`

Un lancer de des reste dans ses bornes et conserve le detail de chaque de.

**Étapes**

1. Lancer 2d6+3 mille fois a graine fixe.

**Résultat attendu**

- Vérifie que `roll.faces.size()` vaut `2U`.
- Vérifie que `face` est supérieur ou égal à `1`.
- Vérifie que `face` est inférieur ou égal à `6`.
- Vérifie que `roll.total` est supérieur ou égal à `dice.minimum()`.
- Vérifie que `roll.total` est inférieur ou égal à `dice.maximum()`.

### DiceTest.RejouabiliteStricte

*Critique · Unitaire · Des* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:109`

Exigences : `EX-NFR-002`

Deux generateurs de meme graine produisent exactement la meme suite de des.

**Étapes**

1. Lancer cent fois 3d8 avec deux generateurs de meme graine.
2. Recommencer avec une graine differente.

**Résultat attendu**

- Vérifie que `suiteA` vaut `suiteB`.
- Vérifie que `suiteA` diffère de `suiteAutre`.

### DiceTest.DistributionUniformeSurCentMilleTirages

*Majeur · Unitaire · Des* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:139`

La distribution d'un d20 est uniforme sur cent mille tirages a graine fixe.

**Étapes**

1. Tirer cent mille d20 a graine fixe et compter chaque face.

**Résultat attendu**

- Vérifie que `effectifs.size()` vaut `static_cast<std::size_t>(FACES)`.
- Vérifie que `face` est supérieur ou égal à `1`.
- Vérifie que `face` est inférieur ou égal à `FACES`.
- Vérifie que `static_cast<double>(compte)` vaut `attendu`, à `attendu * 0.10` près.

### DiceTest.BornesDuTirageEntier

*Majeur · Unitaire · Des* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:171`

Le tirage entier respecte ses bornes, y compris quand elles sont egales.

**Étapes**

1. Tirer dix mille fois dans [3, 3], puis dans [-5, 5].

**Résultat attendu**

- Vérifie que `random.nextInt(3, 3)` vaut `3`.
- Vérifie que `valeur` est supérieur ou égal à `-5`.
- Vérifie que `valeur` est inférieur ou égal à `5`.
- Vérifie que `minAtteint` est vrai.
- Vérifie que `maxAtteint` est vrai.

### DiceTest.ModificateurArrondiVersLeBas

*Critique · Unitaire · Caracteristiques* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:200`

Le modificateur de caracteristique s'arrondit vers le bas, y compris en negatif.

**Étapes**

1. Calculer le modificateur pour 1, 7, 8, 9, 10, 11, 15 et 20.

**Résultat attendu**

- Vérifie que `core::abilityModifier(1)` vaut `-5`.
- Vérifie que `core::abilityModifier(3)` vaut `-4`.
- Vérifie que `core::abilityModifier(7)` vaut `-2`.
- Vérifie que `core::abilityModifier(8)` vaut `-1`.
- Vérifie que `core::abilityModifier(9)` vaut `-1`.
- Vérifie que `core::abilityModifier(10)` vaut `0`.
- Vérifie que `core::abilityModifier(11)` vaut `0`.
- Vérifie que `core::abilityModifier(15)` vaut `2`.
- Vérifie que `core::abilityModifier(20)` vaut `5`.
- Vérifie que `core::abilityModifier(30)` vaut `10`.

### DiceTest.AllerRetourDesCaracteristiques

*Majeur · Unitaire · Caracteristiques* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:225`

Exigences : `EX-CNT-011`

Les six caracteristiques font l'aller-retour par leur nom sans perte.

**Étapes**

1. Convertir chaque caracteristique en nom, puis le nom en caracteristique.
2. Analyser un nom francais.

**Résultat attendu**

- Vérifie que `core::allAbilities().size()` vaut `6U`.
- Vérifie que `nom.empty()` est faux.
- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `*relu` vaut `ability`.
- Vérifie que `core::parseAbility("force").has_value()` est faux.

### DiceTest.EchelleDuMonde

*Majeur · Unitaire · Echelle* — `Source/Test/Unit/Core/Rpg/test_dice.cpp:248`

L'echelle du monde convertit metres et cases dans les deux sens.

**Étapes**

1. Convertir 9 metres en cases, puis 6 cases en metres.

**Résultat attendu**

- Vérifie que `core::METERS_PER_TILE` vaut `1.5f` (comparaison flottante).
- Vérifie que `core::tilesFromMeters(9.0f)` vaut `6.0f` (comparaison flottante).
- Vérifie que `core::metersFromTiles(6.0f)` vaut `9.0f` (comparaison flottante).
- Vérifie que `core::tilesFromMeters(core::metersFromTiles(4.0f))` vaut `4.0f` (comparaison flottante).

## test_equipment.cpp

### EquipmentTest.LesDeuxCataloguesSeChargent

*Critique · Unitaire · Equipement* — `Source/Test/Unit/Core/Rpg/test_equipment.cpp:87`

Les armes et les armures livrees se chargent toutes.

**Étapes**

1. Charger weapons/ et armors/.

**Résultat attendu**

- Vérifie que `lus.weapons.size()` vaut `37U`.
- Vérifie que `lus.armors.size()` vaut `ARMURES.size()`.
- Vérifie que `regles().ok()` est vrai.

### EquipmentTest.LesTreizeArmuresPortentLesValeursDuLivre

*Critique · Unitaire · Equipement* — `Source/Test/Unit/Core/Rpg/test_equipment.cpp:107`

Chaque armure du catalogue porte la CA, le poids et le prix du livre.

**Étapes**

1. Pour les 13 rangees de la table p. 50, comparer categorie, CA de base, plafond de Dexterite, Force exigee, discretion, poids et prix aux valeurs recopiees a la main.

**Résultat attendu**

- Vérifie que `armure` diffère de `nullptr`.
- Vérifie que `core::armorCategoryName(armure->category)` vaut `attendue.categorie`.
- Vérifie que `armure->baseArmorClass` vaut `attendue.base`.
- Vérifie que `armure->weightGrams` vaut `attendue.poidsGrammes`.
- Vérifie que `armure->price` vaut `attendue.prixCuivre`.
- Vérifie que `armure->stealthDisadvantage` vaut `attendue.discretion`.
- Vérifie que `armure->dexterityBonus` est faux.
- Vérifie que `armure->dexterityBonus` est vrai.
- Vérifie que `armure->dexterityBonusMax.has_value()` est faux.
- Vérifie que `armure->dexterityBonusMax.has_value()` est vrai.
- Vérifie que `*armure->dexterityBonusMax` vaut `attendue.dexMax`.
- Vérifie que `armure->strengthRequired.has_value()` est faux.
- Vérifie que `armure->strengthRequired.has_value()` est vrai.
- Vérifie que `*armure->strengthRequired` vaut `attendue.forceExigee`.

### EquipmentTest.LaCaCalculeeEgaleLaColonneDuLivre

*Critique · Unitaire · Equipement* — `Source/Test/Unit/Core/Rpg/test_equipment.cpp:149`

La classe d'armure calculee pour chaque armure egale la colonne CA du livre.

**Étapes**

1. Avec une Dexterite de 18 (modificateur +4), calculer la CA de chaque armure.
2. La comparer a la formule de la colonne CA, appliquee a la main.

**Résultat attendu**

- Vérifie que `dexterite` vaut `4`.
- Vérifie que `armure` diffère de `nullptr`.
- Vérifie que `core::armorClassFor(fiche, regles(), armure, nullptr)` vaut `attendu`.

### EquipmentTest.LeBouclierAjouteIlNeRemplacePas

*Critique · Unitaire · Equipement* — `Source/Test/Unit/Core/Rpg/test_equipment.cpp:188`

Un bouclier ajoute son bonus a la classe d'armure, seul ou avec une armure.

**Étapes**

1. Calculer la CA d'un personnage en bouclier seul, puis en harnois et bouclier.

**Résultat attendu**

- Vérifie que `bouclier` diffère de `nullptr`.
- Vérifie que `harnois` diffère de `nullptr`.
- Vérifie que `core::armorClassFor(fiche, regles(), bouclier, nullptr)` vaut `regles().unarmoredArmorClass + fiche.modifier(core::Ability::Dexterity)`.
- Vérifie que `core::armorClassFor(fiche, regles(), nullptr, bouclier)` vaut `regles().unarmoredArmorClass + fiche.modifier(core::Ability::Dexterity) + bouclier->baseArmorClass`.
- Vérifie que `core::armorClassFor(fiche, regles(), harnois, bouclier)` vaut `harnois->baseArmorClass + bouclier->baseArmorClass`.

### EquipmentTest.LePoidsDUnInventaireDeDepartCorrespondAuCalculManuel

*Critique · Unitaire · Equipement* — `Source/Test/Unit/Core/Rpg/test_equipment.cpp:216`

Le poids total d'un inventaire de depart egale la somme calculee a la main.

**Étapes**

1. Composer l'inventaire d'un guerrier : cotte de mailles, epee longue, bouclier et deux javelines.
2. Comparer le total au calcul fait a la main depuis les poids du livre.

**Résultat attendu**

- Vérifie que `cotte` diffère de `nullptr`.
- Vérifie que `bouclier` diffère de `nullptr`.
- Vérifie que `epee` diffère de `nullptr`.
- Vérifie que `javeline` diffère de `nullptr`.
- Vérifie que `core::totalWeightGrams(sac)` vaut `TOTAL_ATTENDU_GRAMMES`.
- Vérifie que `core::totalWeightGrams(avecVides)` vaut `cotte->weightGrams`.
- Vérifie que `core::totalWeightGrams({})` vaut `0`.

### EquipmentTest.LeFiletNInfligeAucunDegat

*Majeur · Unitaire · Equipement* — `Source/Test/Unit/Core/Rpg/test_equipment.cpp:255`

Une arme sans degats est chargee sans que des lui soient inventes.

**Étapes**

1. Charger le filet et une arme ordinaire.

**Résultat attendu**

- Vérifie que `filet` diffère de `nullptr`.
- Vérifie que `filet->damage.has_value()` est faux.
- Vérifie que `filet->damageType.has_value()` est faux.
- Vérifie que `filet->text.empty()` est faux.
- Vérifie que `dague` diffère de `nullptr`.
- Vérifie que `dague->damage.has_value()` est vrai.
- Vérifie que `dague->damage->count` vaut `1`.
- Vérifie que `dague->damage->faces` vaut `4`.
- Vérifie que `dague->damageType.has_value()` est vrai.
- Vérifie que `*dague->damageType` vaut `core::DamageType::Piercing`.

### EquipmentTest.LesArmesPortentLeurCategorieEtLeurPortee

*Critique · Unitaire · Equipement* — `Source/Test/Unit/Core/Rpg/test_equipment.cpp:282`

Chaque arme porte la categorie et la portee de son intertitre.

**Étapes**

1. Verifier la categorie et la portee de quatre armes, une par groupe du livre.

**Résultat attendu**

- Vérifie que `arme` diffère de `nullptr`.
- Vérifie que `arme->category` vaut `cas.categorie`.
- Vérifie que `arme->ranged` vaut `cas.distance`.

## test_inventory.cpp

### InventoryTest.LaClasseDArmureNeDerivePasAvecLOrdre

*Critique · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:119`

La classe d'armure ne derive pas avec l'ordre des equipements.

**Étapes**

1. Relever la CA sans equipement.
2. Jouer six ordres d'equipement et de retrait d'une armure et d'un bouclier.
3. Relever la CA apres chaque sequence complete.

**Résultat attendu**

- Vérifie que `depart` vaut `12`.
- Vérifie que `core::derivedStatsFor(personnage, sac, catalogues, regles(), charge()).armorClass` vaut `depart`.

### InventoryTest.ChaqueCategorieDArmureAppliqueSaRegle

*Critique · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:185`

Chaque categorie d'armure applique sa propre regle de Dexterite.

**Étapes**

1. Equiper une armure legere, puis une intermediaire, avec une Dexterite de 18.
2. Ajouter un bouclier.

**Résultat attendu**

- Vérifie que `core::derivedStatsFor(personnage, sac, catalogues, regles(), charge()).armorClass` vaut `15`.
- Vérifie que `core::derivedStatsFor(personnage, sac, catalogues, regles(), charge()).armorClass` vaut `17`.
- Vérifie que `core::derivedStatsFor(personnage, sac, catalogues, regles(), charge()).armorClass` vaut `19`.

### InventoryTest.UnEmplacementRendCeQuIlPortait

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:214`

Un emplacement ne porte jamais deux pieces, et ce qui en sort est rendu.

**Étapes**

1. Equiper une armure, puis une autre au meme emplacement.

**Résultat attendu**

- Vérifie que `core::equip(sac, EquipmentSlot::Torso, "cuir")` vaut `""`.
- Vérifie que `core::equip(sac, EquipmentSlot::Torso, "demi-plate")` vaut `"cuir"`.
- Vérifie que `sac.at(EquipmentSlot::Torso)` vaut `"demi-plate"`.
- Vérifie que `core::unequip(sac, EquipmentSlot::Torso)` vaut `"demi-plate"`.
- Vérifie que `sac.isEquipped(EquipmentSlot::Torso)` est faux.

### InventoryTest.LeSacEmpileEtSeVide

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:232`

Le sac empile, et une ligne videe disparait.

**Étapes**

1. Ajouter deux fois le meme objet.
2. En retirer plus qu'il n'y en a.

**Résultat attendu**

- Vérifie que `sac.backpack.size()` vaut `1U`.
- Vérifie que `sac.backpack.front().quantity` vaut `5`.
- Vérifie que `core::removeFromBackpack(sac, "torche", 10)` vaut `5`.
- Vérifie que `sac.backpack.empty()` est vrai.
- Vérifie que `core::removeFromBackpack(sac, "torche", 1)` vaut `0`.

### InventoryTest.LePoidsPorteCompteLeSacEtLEquipement

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:254`

Le poids porte compte le sac et l'equipement.

**Étapes**

1. Equiper une armure et une arme.
2. Ajouter des objets au sac.

**Résultat attendu**

- Vérifie que `core::carriedWeightGrams(sac, catalogues)` vaut `5000 + 1500 + 2000 + 5000`.

### InventoryTest.UnIdentifiantInconnuEstSignale

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:278`

Un identifiant inconnu des catalogues est signale.

**Étapes**

1. Mettre au sac un identifiant absent des catalogues.

**Résultat attendu**

- Vérifie que `inconnus` vaut `(std::vector<std::string>{"bidule-inexistant", "heaume-fantome"})`.
- Vérifie que `core::carriedWeightGrams(sac, catalogues)` vaut `0`.

### InventoryTest.LaChargeFranchitSesSeuils

*Critique · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:301`

La charge franchit ses trois seuils et definit le depassement.

**Étapes**

1. Charger un personnage de Force 10 en dessous du premier seuil, puis entre les deux, puis au-dela du second, puis au-dela de la capacite.

**Résultat attendu**

- Vérifie que `leger.encumbrance` vaut `core::EncumbranceLevel::Unencumbered`.
- Vérifie que `leger.speedMeters` vaut `9.0F` (comparaison flottante).
- Vérifie que `leger.carryingCapacityGrams` vaut `75000`.
- Vérifie que `encombre.encumbrance` vaut `core::EncumbranceLevel::Encumbered`.
- Vérifie que `encombre.speedMeters` vaut `6.0F` (comparaison flottante).
- Vérifie que `lourd.encumbrance` vaut `core::EncumbranceLevel::HeavilyEncumbered`.
- Vérifie que `lourd.speedMeters` vaut `3.0F` (comparaison flottante).
- Vérifie que `trop.encumbrance` vaut `core::EncumbranceLevel::OverCapacity`.
- Vérifie que `trop.speedMeters` vaut `0.0F` (comparaison flottante).

### InventoryTest.SansRegleDeChargeAucunEncombrementNEstAffirme

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:342`

Sans regle de charge, aucun encombrement n'est affirme.

**Étapes**

1. Deriver les statistiques avec des bornes de charge non chargees.

**Résultat attendu**

- Vérifie que `derivees.encumbrance` vaut `core::EncumbranceLevel::Unencumbered`.
- Vérifie que `derivees.carryingCapacityGrams` vaut `0`.
- Vérifie que `derivees.speedMeters` vaut `9.0F` (comparaison flottante).

### InventoryTest.LaCaracteristiqueDAttaqueSuitLArme

*Critique · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:366`

La caracteristique d'attaque suit l'arme portee.

**Étapes**

1. Equiper une arme de melee, puis une arme a distance, puis une arme de finesse, avec une Dexterite superieure a la Force.

**Résultat attendu**

- Vérifie que `derivees.attackAbility` vaut `Ability::Strength`.
- Vérifie que `derivees.damage.has_value()` est vrai.
- Vérifie que `derivees.damage->faces` vaut `8`.
- Vérifie que `derivees.attackAbility` vaut `Ability::Dexterity`.
- Vérifie que `derivees.attackAbility` vaut `Ability::Dexterity`.
- Vérifie que `derivees.damage.has_value()` est faux.

### InventoryTest.LaCategorieDecideAvantLEmplacement

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:402`

C'est la categorie qui dit comment une piece compte, pas l'emplacement.

**Étapes**

1. Ranger un bouclier a l'emplacement du torse.

**Résultat attendu**

- Vérifie que `core::derivedStatsFor(personnage, sac, catalogues, regles(), charge()).armorClass` vaut `14`.

### InventoryTest.LesEmplacementsSeNommentEtSeRelisent

*Mineur · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:422`

Les seize emplacements se nomment et se relisent.

**Étapes**

1. Convertir chaque emplacement en nom, puis le nom en emplacement.

**Résultat attendu**

- Vérifie que `nom.empty()` est faux.
- Vérifie que `core::parseEquipmentSlot(nom)` vaut `emplacement`.
- Vérifie que `core::parseEquipmentSlot("chapeau-de-paille").has_value()` est faux.

### InventoryTest.LeCatalogueLivreSeCharge

*Majeur · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:441`

Le catalogue d'objets livre se charge.

**Étapes**

1. Charger Source/Elements/Rpg/items.

**Résultat attendu**

- Vérifie que `catalogue.errors.empty()` est vrai.
- Vérifie que `catalogue.items.size()` est strictement supérieur à `100U`.
- Vérifie que `objet.id.empty()` est faux.
- Vérifie que `objet.name.empty()` est faux.
- Vérifie que `objet.weightGrams` est supérieur ou égal à `0`.

### InventoryTest.LesBornesDeChargeLivreesSontCellesDuLivre

*Critique · Unitaire · Inventaire* — `Source/Test/Unit/Core/Rpg/test_inventory.cpp:463`

Les bornes de charge livrees portent les valeurs du livre.

**Étapes**

1. Charger Source/Elements/Rpg/rules/encumbrance.json.

**Résultat attendu**

- Vérifie que `regles.isLoaded()` est vrai.
- Vérifie que `regles.carryingCapacityGramsPerStrength` vaut `7500`.
- Vérifie que `regles.encumberedGramsPerStrength` vaut `2500`.
- Vérifie que `regles.heavilyEncumberedGramsPerStrength` vaut `5000`.
- Vérifie que `regles.encumberedSpeedPenaltyMeters` vaut `3.0F` (comparaison flottante).
- Vérifie que `regles.heavilyEncumberedSpeedPenaltyMeters` vaut `6.0F` (comparaison flottante).

## test_multiclassing.cpp

### MulticlassingTest.ExempleTravailleDuManuelDesJoueurs

*Critique · Unitaire · Multiclassage* — `Source/Test/Unit/Core/Rpg/test_multiclassing.cpp:59`

L'exemple de multiclassage du Manuel des Joueurs est reproduit a l'identique.

**Étapes**

1. Calculer le niveau de lanceur d'un rodeur 4 / magicien 3.
2. Lire les emplacements correspondants dans la table livree.

**Résultat attendu**

- Vérifie que `niveau` vaut `5`.
- Vérifie que `table.size()` est strictement supérieur à `static_cast<std::size_t>(niveau)`.
- Vérifie que `table[static_cast<std::size_t>(niveau)][0]` vaut `4`.
- Vérifie que `table[static_cast<std::size_t>(niveau)][1]` vaut `3`.
- Vérifie que `table[static_cast<std::size_t>(niveau)][2]` vaut `2`.

### MulticlassingTest.LArrondiSeFaitParClasse

*Critique · Unitaire · Multiclassage* — `Source/Test/Unit/Core/Rpg/test_multiclassing.cpp:89`

L'arrondi du niveau de lanceur se fait par classe, jamais sur le total.

**Étapes**

1. Calculer le niveau de lanceur d'un paladin 3 / rodeur 3.
2. Calculer celui d'un guerrier 5 / roublard 5 sous archetype.

**Résultat attendu**

- Vérifie que `core::multiclassCasterLevel(paladinRodeur)` vaut `2`.
- Vérifie que `core::multiclassCasterLevel(guerrierRoublard)` vaut `2`.

### MulticlassingTest.LaMagieDePacteEstCompteeAPart

*Critique · Unitaire · Multiclassage* — `Source/Test/Unit/Core/Rpg/test_multiclassing.cpp:110`

Les emplacements de magie de pacte n'entrent pas dans le cumul multiclasse.

**Étapes**

1. Calculer le niveau de lanceur d'un sorcier 5 / magicien 3.

**Résultat attendu**

- Vérifie que `core::multiclassCasterLevel(sorcierMagicien)` vaut `3`.

### MulticlassingTest.ClassesSansIncantationEtBorneSuperieure

*Majeur · Unitaire · Multiclassage* — `Source/Test/Unit/Core/Rpg/test_multiclassing.cpp:128`

Une classe sans incantation n'apporte rien au niveau de lanceur, borne a 20.

**Étapes**

1. Calculer le niveau d'un barbare 10 seul, puis d'un magicien 20 / clerc 20.

**Résultat attendu**

- Vérifie que `core::multiclassCasterLevel(barbare)` vaut `0`.
- Vérifie que `core::multiclassCasterLevel(impossible)` vaut `20`.

### MulticlassingTest.AllerRetourDesNomsDeProgression

*Majeur · Unitaire · Multiclassage* — `Source/Test/Unit/Core/Rpg/test_multiclassing.cpp:145`

Les noms de progression de lanceur font l'aller-retour sans perte.

**Étapes**

1. Convertir chaque progression en nom, puis le nom en progression.
2. Analyser un nom inconnu.

**Résultat attendu**

- Vérifie que `nom.empty()` est faux.
- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `*relu` vaut `progression`.
- Vérifie que `core::parseCasterProgression("plein").has_value()` est faux.
- Vérifie que `core::parseCasterProgression("").has_value()` est faux.

### MulticlassingTest.LesProgressionsDeLaDonneeSontConnuesDuMoteur

*Critique · Unitaire · Multiclassage* — `Source/Test/Unit/Core/Rpg/test_multiclassing.cpp:171`

Les progressions de lanceur de la donnee sont toutes connues du moteur.

**Étapes**

1. Lire `casterProgression` de multiclassing.json.
2. Analyser chaque valeur avec `parseCasterProgression`.

**Résultat attendu**

- Vérifie que `document.ok()` est vrai.
- Vérifie que `progressions.size()` vaut `12U`.
- Vérifie que `core::parseCasterProgression(nom).has_value()` est vrai.

## test_rpg_enums.cpp

### RpgEnumsTest.AllerRetourSurToutesLesValeurs

*Critique · Unitaire · Enumerations RPG* — `Source/Test/Unit/Core/Rpg/test_rpg_enums.cpp:67`

Chaque valeur des enumerations RPG fait l'aller-retour par son nom sans perte.

**Étapes**

1. Pour chaque type de degats, condition et ecole de magie, convertir en nom puis reconvertir en valeur.

**Résultat attendu**

- Vérifie que `nom.empty()` est faux.
- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `*relu` vaut `type`.
- Vérifie que `nom.empty()` est faux.
- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `*relu` vaut `condition`.
- Vérifie que `nom.empty()` est faux.
- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `*relu` vaut `school`.
- Vérifie que `nom.empty()` est faux.
- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `*relu` vaut `size`.

### RpgEnumsTest.LesNomsSontUniques

*Critique · Unitaire · Enumerations RPG* — `Source/Test/Unit/Core/Rpg/test_rpg_enums.cpp:109`

Deux valeurs distinctes d'une enumeration RPG ne portent jamais le meme nom.

**Étapes**

1. Collecter les noms de chaque enumeration dans un ensemble.

**Résultat attendu**

- Vérifie que `nomsDuMoteur(core::allDamageTypes(), core::damageTypeName).size()` vaut `core::allDamageTypes().size()`.
- Vérifie que `nomsDuMoteur(core::allConditions(), core::conditionName).size()` vaut `core::allConditions().size()`.
- Vérifie que `nomsDuMoteur(core::allMagicSchools(), core::magicSchoolName).size()` vaut `core::allMagicSchools().size()`.
- Vérifie que `nomsDuMoteur(core::allCreatureSizes(), core::creatureSizeName).size()` vaut `core::allCreatureSizes().size()`.

### RpgEnumsTest.UnNomInconnuEstRefuse

*Majeur · Unitaire · Enumerations RPG* — `Source/Test/Unit/Core/Rpg/test_rpg_enums.cpp:129`

Exigences : `EX-CNT-011`

Un nom d'enumeration RPG inconnu est refuse au lieu d'etre devine.

**Étapes**

1. Analyser une chaine vide, un terme francais, et un nom de casse differente.

**Résultat attendu**

- Vérifie que `core::parseDamageType("").has_value()` est faux.
- Vérifie que `core::parseDamageType("psychique").has_value()` est faux.
- Vérifie que `core::parseDamageType("Psychic").has_value()` est faux.
- Vérifie que `core::parseCondition("empoisonne").has_value()` est faux.
- Vérifie que `core::parseMagicSchool("invocation").has_value()` est faux.
- Vérifie que `core::parseCreatureSize("P").has_value()` est faux.
- Vérifie que `core::parseCreatureSize("moyenne").has_value()` est faux.

### RpgEnumsTest.LesEnumerationsCoincidentAvecLesSchemas

*Critique · Unitaire · Enumerations RPG* — `Source/Test/Unit/Core/Rpg/test_rpg_enums.cpp:152`

Exigences : `EX-CNT-011`

Les enumerations partagees entre le C++ et les schemas JSON sont identiques.

**Étapes**

1. Lire les enumerations `damageType`, `conditionRef` et `magicSchool` de `common.schema.json`.
2. Les comparer aux noms produits par `core::DamageType`, `core::Condition` et `core::MagicSchool`.

**Résultat attendu**

- Vérifie que `nomsDuMoteur(core::allDamageTypes(), core::damageTypeName)` vaut `enumDuSchema("damageType")`.
- Vérifie que `nomsDuMoteur(core::allConditions(), core::conditionName)` vaut `enumDuSchema("conditionRef")`.
- Vérifie que `nomsDuMoteur(core::allMagicSchools(), core::magicSchoolName)` vaut `enumDuSchema("magicSchool")`.
- Vérifie que `nomsDuMoteur(core::allCreatureSizes(), core::creatureSizeName)` vaut `enumDuSchema("size")`.

### RpgEnumsTest.LesEnsemblesFermesOntLeurCardinal

*Majeur · Unitaire · Enumerations RPG* — `Source/Test/Unit/Core/Rpg/test_rpg_enums.cpp:185`

Les enumerations fermees du RPG ont le nombre de valeurs fixe par les regles.

**Étapes**

1. Compter les types de degats, les conditions et les ecoles de magie.

**Résultat attendu**

- Vérifie que `core::allDamageTypes().size()` vaut `13U`.
- Vérifie que `core::allConditions().size()` vaut `15U`.
- Vérifie que `core::allMagicSchools().size()` vaut `8U`.
- Vérifie que `core::allCreatureSizes().size()` vaut `6U`.
