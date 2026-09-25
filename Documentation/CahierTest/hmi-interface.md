# HMI · Interface

Tests unitaires — **31 cas** (2 bloquants, 11 critiques, 17 majeurs, 1 mineur). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_character_sheet_values.cpp`](#test-character-sheet-valuescpp) | 6 | - | 2 | 3 | 1 |
| [`test_dialogue_screen.cpp`](#test-dialogue-screencpp) | 4 | - | - | 4 | - |
| [`test_identity_scale.cpp`](#test-identity-scalecpp) | 8 | 2 | 2 | 4 | - |
| [`test_rpg_screens.cpp`](#test-rpg-screenscpp) | 7 | - | 1 | 6 | - |
| [`test_screen_flow.cpp`](#test-screen-flowcpp) | 6 | - | 6 | - | - |

## Exigences vérifiées par cette page

Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la [matrice de traçabilité](couverture-exigences.md) les rassemble toutes.

| Exigence | Cas |
|---|---|
| `EX-IHM-090` | [`ScreenFlowTest.EcranDuRpgRevientVersSonEcranDOrigine`](#screenflowtestecrandurpgrevientverssonecrandorigine) |
| `EX-IHM-091` | [`RpgScreensTest.LaRegleDeSuperpositionEstCelleAttendue`](#rpgscreenstestlaregledesuperpositionestcelleattendue) |
| `EX-REN-033` | [`RpgScreensTest.ChaqueCleDeLOssatureExisteDansLesDeuxLangues`](#rpgscreenstestchaquecledelossatureexistedanslesdeuxlangues) |

## test_character_sheet_values.cpp

### CharacterSheetValuesTest.LesModificateursSAffichentAvecLeurSigne

*Majeur · Unitaire · Fiche de personnage* — `Source/Test/Unit/HMI/Interface/test_character_sheet_values.cpp:104`

Les modificateurs s'affichent signes, les caracteristiques avec leur modificateur.

**Étapes**

1. Produire les valeurs d'une fiche de Force 16 et de Charisme 8.
2. Lire les deux lignes de caracteristique.

**Résultat attendu**

- Vérifie que `valeurs.at("sheet.ability.strength")` vaut `"16 (+3)"`.
- Vérifie que `valeurs.at("sheet.ability.charisma")` vaut `"8 (-1)"`.
- Vérifie que `valeurs.at("sheet.initiative")` vaut `"+1"`.

### CharacterSheetValuesTest.LeJetDeSauvegardeMaitriseComptteLaMaitrise

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/HMI/Interface/test_character_sheet_values.cpp:127`

Un jet de sauvegarde maitrise porte le bonus de maitrise.

**Étapes**

1. Produire les valeurs d'une fiche maitrisant la sauvegarde de Force.
2. Comparer les lignes de Force et de Dexterite.

**Résultat attendu**

- Vérifie que `valeurs.at("sheet.proficiency_bonus")` vaut `"+2"`.
- Vérifie que `valeurs.at("sheet.save.strength")` vaut `"+5"`.
- Vérifie que `valeurs.at("sheet.save.dexterity")` vaut `"+1"`.

### CharacterSheetValuesTest.LaCompetenceMaitriseeSeSignaleEtLaPerceptionPassiveEnDerive

*Majeur · Unitaire · Fiche de personnage* — `Source/Test/Unit/HMI/Interface/test_character_sheet_values.cpp:149`

Une competence maitrisee se signale, et la Perception passive en derive.

**Étapes**

1. Produire les valeurs d'une fiche maitrisant l'Athletisme.
2. Lire Athletisme, Discretion et la Perception passive.

**Résultat attendu**

- Vérifie que `valeurs.at("sheet.skill.athletics")` vaut `"+5 •"`.
- Vérifie que `valeurs.at("sheet.skill.stealth")` vaut `"+1"`.
- Vérifie que `valeurs.at("sheet.passive_perception")` vaut `"11"`.

### CharacterSheetValuesTest.LesPointsDeVieSeLisentContreLeurMaximum

*Mineur · Unitaire · Fiche de personnage* — `Source/Test/Unit/HMI/Interface/test_character_sheet_values.cpp:172`

Les points de vie se lisent contre leur maximum.

**Étapes**

1. Produire les valeurs d'une fiche a 25 points de vie sur 30.

**Résultat attendu**

- Vérifie que `valeurs.at("sheet.hit_points")` vaut `"25 / 30"`.
- Vérifie que `valeurs.at("sheet.hit_points_max")` vaut `"30"`.
- Vérifie que `valeurs.at("sheet.speed")` vaut `"9 m"`.

### CharacterSheetValuesTest.SansFicheAucuneValeurNEstProduite

*Majeur · Unitaire · Fiche de personnage* — `Source/Test/Unit/HMI/Interface/test_character_sheet_values.cpp:194`

Sans fiche, aucune valeur n'est produite.

**Étapes**

1. Produire les valeurs d'un contexte sans fiche.

**Résultat attendu**

- Vérifie que `hmi::characterSheetValues({}).empty()` est vrai.

### CharacterSheetValuesTest.ChaqueChampDeLaFicheEstAlimente

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/HMI/Interface/test_character_sheet_values.cpp:213`

Chaque champ de la fiche declarant une source est rempli.

**Étapes**

1. Lire les identifiants de valeur declares par l'ossature de la fiche.
2. Produire les valeurs d'une fiche complete.
3. Verifier que chaque identifiant declare est produit.

**Résultat attendu**

- Vérifie que `valeurs.count(champ.valueId) > 0` est vrai.
- Vérifie que `declares` est strictement supérieur à `10`.

## test_dialogue_screen.cpp

### DialogueScreenTest.UneRepliqueAnnonceLeJetDeSaReponse

*Majeur · Unitaire · Interface* — `Source/Test/Unit/HMI/Interface/test_dialogue_screen.cpp:81`

L'ecran de dialogue annonce le jet avant le choix.

**Étapes**

1. Ouvrir un dialogue de garde hostile.
2. Lire les valeurs de l'ecran.

**Résultat attendu**

- Vérifie que `runner.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `v.speakerName` vaut `"<dialogue.garde.speaker>"`.
- Vérifie que `v.attitude` vaut `"<dialogue.attitude.hostile>"`.
- Vérifie que `v.line` vaut `"<dialogue.garde.halte>"`.
- Vérifie que `v.replies.size()` vaut `2U`.
- Vérifie que `v.replies[0].id` vaut `"negocier"`.
- Vérifie que `v.replies[0].label` vaut `"<dialogue.garde.halte.negocier>"`.
- Vérifie que `v.replies[0].value` vaut `"[<rpg.skill.animal_handling> DD 10]"`.
- Vérifie que `v.replies[1].value` vaut `""`.
- Vérifie que `v.checkOutcome.empty()` est vrai.
- Vérifie que `v.checkTitle.empty()` est vrai.
- Vérifie que `v.finished` est faux.

### DialogueScreenTest.LeJetSeRestitueSurLaRepliqueQuiSuit

*Majeur · Unitaire · Interface* — `Source/Test/Unit/HMI/Interface/test_dialogue_screen.cpp:116`

Le jet se restitue sur la replique qui suit, puis s'efface.

**Étapes**

1. Negocier (bonus +30 contre 10).
2. Lire les valeurs.
3. Continuer jusqu'a la fin.

**Résultat attendu**

- Vérifie que `runner.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `runner.choose("negocier")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `apres.attitude` vaut `"<dialogue.attitude.friendly>"`.
- Vérifie que `apres.checkOutcome` vaut `"<rpg.skill.animal_handling> DD 10 -- d20 " + de + ", total " + total + " -- <dialogue.check.success>"`.
- Vérifie que `apres.checkTitle` vaut `"<rpg.skill.animal_handling> DD 10"`.
- Vérifie que `apres.checkDie` vaut `de`.
- Vérifie que `apres.checkDetail` vaut `de + " + 30 = " + total`.
- Vérifie que `apres.checkVerdict` vaut `"<dialogue.check.success>"`.
- Vérifie que `apres.checkSucceeded` est vrai.
- Vérifie que `apres.replies.size()` vaut `1U`.
- Vérifie que `apres.replies[0].id` vaut `"continue"`.
- Vérifie que `apres.replies[0].label` vaut `"<dialogue.continue>"`.
- Vérifie que `runner.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `fin.finished` est vrai.
- Vérifie que `fin.checkOutcome.empty()` est vrai.
- Vérifie que `fin.checkTitle.empty()` est vrai.
- Vérifie que `fin.replies.size()` vaut `1U`.
- Vérifie que `fin.replies[0].id` vaut `std::string(hmi::DIALOGUE_LEAVE_REPLY)`.

### DialogueScreenTest.UnRefusMontreLeRefusEtQuitter

*Majeur · Unitaire · Interface* — `Source/Test/Unit/HMI/Interface/test_dialogue_screen.cpp:163`

L'ecran montre le refus faute de langue commune.

**Étapes**

1. Ouvrir le dialogue du garde avec un interlocuteur qui ne parle que l'elfique.
2. Lire les valeurs.

**Résultat attendu**

- Vérifie que `runner.start()` vaut `core::DialogueState::Refused`.
- Vérifie que `v.line` vaut `"<dialogue.refused>"`.
- Vérifie que `v.finished` est faux.
- Vérifie que `v.replies.size()` vaut `1U`.
- Vérifie que `v.replies[0].id` vaut `std::string(hmi::DIALOGUE_LEAVE_REPLY)`.
- Vérifie que `v.replies[0].label` vaut `"<dialogue.leave>"`.

### DialogueScreenTest.UnEchecSeMontreEtNeSeRetentePas

*Majeur · Unitaire · Interface* — `Source/Test/Unit/HMI/Interface/test_dialogue_screen.cpp:192`

L'ecran montre un echec, puis un jet deja tente sans de.

**Étapes**

1. Negocier avec -30 contre 10.
2. Lire les valeurs.
3. Continuer jusqu'a la halte.
4. Rouvrir le dialogue sur les memes drapeaux, sur un graphe ou la halte mene au jet sans choix.

**Résultat attendu**

- Vérifie que `runner.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `runner.choose("negocier")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `rate.checkSucceeded` est faux.
- Vérifie que `rate.checkDie` vaut `de`.
- Vérifie que `rate.checkDetail` vaut `de + " - 30 = " + total`.
- Vérifie que `rate.checkVerdict` vaut `"<dialogue.check.failure>"`.
- Vérifie que `runner.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `halte.replies.size()` vaut `1U`.
- Vérifie que `halte.replies[0].id` vaut `"partir"`.
- Vérifie que `lu.graph.has_value()` est vrai.
- Vérifie que `seconde.start()` vaut `core::DialogueState::AwaitingChoice`.
- Vérifie que `seconde.choose("continue")` vaut `core::ChoiceResult::Advanced`.
- Vérifie que `deja.checkTitle` vaut `"<rpg.skill.animal_handling> DD 10"`.
- Vérifie que `deja.checkDie.empty()` est vrai.
- Vérifie que `deja.checkDetail` vaut `"<dialogue.check.already-failed>"`.
- Vérifie que `deja.checkOutcome` vaut `"<rpg.skill.animal_handling> DD 10 -- deja -- <dialogue.check.failure>"`.
- Vérifie que `deja.checkSucceeded` est faux.

## test_identity_scale.cpp

### IdentityScaleTest.FacteurToujoursBorneEntreUnEtLePlafond

*Critique · Unitaire · Echelle de l identite* — `Source/Test/Unit/HMI/Interface/test_identity_scale.cpp:18`

Le facteur reste entier, superieur a 1 et borne par le plafond.

**Étapes**

1. Demander le facteur pour des hauteurs allant du negatif au tres grand.

**Résultat attendu**

- Vérifie que `scale` est supérieur ou égal à `1`.
- Vérifie que `scale` est inférieur ou égal à `hmi::IDENTITY_MAX_SCALE`.

### IdentityScaleTest.FacteurMonotoneEnHauteur

*Majeur · Unitaire · Echelle de l identite* — `Source/Test/Unit/HMI/Interface/test_identity_scale.cpp:37`

Le facteur croit de facon monotone avec la hauteur de fenetre.

**Étapes**

1. Parcourir les hauteurs de 0 a 2400 par pas de 1.
2. Comparer chaque facteur au precedent.

**Résultat attendu**

- Vérifie que `scale` est supérieur ou égal à `previous`.

### IdentityScaleTest.LeSeuilEstAtteintParDefautJamaisParArrondi

*Critique · Unitaire · Echelle de l identite* — `Source/Test/Unit/HMI/Interface/test_identity_scale.cpp:59`

Une hauteur juste insuffisante ne declenche pas le facteur superieur.

**Étapes**

1. Demander le facteur juste avant et juste apres chaque multiple de la hauteur de base.

**Résultat attendu**

- Vérifie que `hmi::identityScaleFor(threshold)` vaut `factor`.
- Vérifie que `hmi::identityScaleFor(threshold - 1)` vaut `factor - 1 < 1 ? 1 : factor - 1`.

### IdentityScaleTest.HauteursCourantesDonnentLesFacteursAnnonces

*Majeur · Unitaire · Echelle de l identite* — `Source/Test/Unit/HMI/Interface/test_identity_scale.cpp:79`

Les hauteurs d'ecran courantes donnent les facteurs annonces.

**Étapes**

1. Demander le facteur pour 360, 720, 1080 et 1440 pixels logiques.

**Résultat attendu**

- Vérifie que `hmi::identityScaleFor(360)` vaut `1`.
- Vérifie que `hmi::identityScaleFor(720)` vaut `2`.
- Vérifie que `hmi::identityScaleFor(1080)` vaut `3`.
- Vérifie que `hmi::identityScaleFor(1440)` vaut `3`.

### IdentityScaleTest.FacteurBorneParLaZoneDisponible

*Bloquant · Unitaire · Echelle de l identite* — `Source/Test/Unit/HMI/Interface/test_identity_scale.cpp:98`

Le facteur borne n'excede jamais celui de la zone disponible.

**Étapes**

1. Demander le facteur borne pour des couples (hauteur de fenetre, zone disponible).
2. Comparer au facteur de la seule zone disponible.

**Résultat attendu**

- Vérifie que `bounded` est inférieur ou égal à `hmi::identityScaleFor(available)`.
- Vérifie que `bounded` est supérieur ou égal à `1`.

### IdentityScaleTest.UneFenetreDebordanteNeGagnePasDeFacteur

*Bloquant · Unitaire · Echelle de l identite* — `Source/Test/Unit/HMI/Interface/test_identity_scale.cpp:124`

Une fenetre plus haute que l'ecran ne gagne pas de facteur.

**Étapes**

1. Zone disponible de 1009 pixels (cas rapporte).
2. Demander le facteur pour des hauteurs de fenetre croissantes, jusqu'au-dela de l'ecran.

**Résultat attendu**

- Vérifie que `ceiling` vaut `2`.
- Vérifie que `hmi::identityScaleForDisplay(window, AVAILABLE)` vaut `ceiling`.

### IdentityScaleTest.ZoneDisponibleInconnueLaisseLaFenetreDecider

*Majeur · Unitaire · Echelle de l identite* — `Source/Test/Unit/HMI/Interface/test_identity_scale.cpp:146`

Une zone disponible inconnue laisse la fenetre decider.

**Étapes**

1. Demander le facteur borne avec une zone disponible nulle puis negative.

**Résultat attendu**

- Vérifie que `hmi::identityScaleForDisplay(window, 0)` vaut `hmi::identityScaleFor(window)`.
- Vérifie que `hmi::identityScaleForDisplay(window, -1)` vaut `hmi::identityScaleFor(window)`.

### IdentityScaleTest.UnePetiteFenetreGardeSonFacteur

*Majeur · Unitaire · Echelle de l identite* — `Source/Test/Unit/HMI/Interface/test_identity_scale.cpp:164`

Une petite fenetre garde son facteur sur un grand ecran.

**Étapes**

1. Zone disponible de 1440 pixels.
2. Demander le facteur pour des fenetres de 360 et 720 pixels.

**Résultat attendu**

- Vérifie que `hmi::identityScaleForDisplay(360, 1440)` vaut `1`.
- Vérifie que `hmi::identityScaleForDisplay(720, 1440)` vaut `2`.
- Vérifie que `hmi::identityScaleForDisplay(1080, 1440)` vaut `3`.

## test_rpg_screens.cpp

### RpgScreensTest.LeCycleTraverseLesHuitEcrans

*Critique · Unitaire · Ecrans du RPG* — `Source/Test/Unit/HMI/Interface/test_rpg_screens.cpp:43`

Le cycle de navigation traverse les huit ecrans et revient au premier.

**Étapes**

1. Partir du premier ecran.
2. Appeler nextRpgScreen huit fois en notant chaque ecran atteint.

**Résultat attendu**

- Vérifie que `rpgScreens().size()` vaut `hmi::RPG_SCREEN_COUNT`.
- Vérifie que `visited.size()` vaut `hmi::RPG_SCREEN_COUNT`.
- Vérifie que `screen` vaut `first`.

### RpgScreensTest.LePasArriereEstLInverseDuPasAvant

*Majeur · Unitaire · Ecrans du RPG* — `Source/Test/Unit/HMI/Interface/test_rpg_screens.cpp:68`

Le pas arriere est l'inverse du pas avant.

**Étapes**

1. Pour chaque ecran, avancer puis reculer.
2. Reculer depuis le premier ecran.

**Résultat attendu**

- Vérifie que `previousRpgScreen(nextRpgScreen(descriptor.id))` vaut `descriptor.id`.
- Vérifie que `nextRpgScreen(previousRpgScreen(descriptor.id))` vaut `descriptor.id`.
- Vérifie que `previousRpgScreen(rpgScreens().front().id)` vaut `rpgScreens().back().id`.

### RpgScreensTest.LaRegleDeSuperpositionEstCelleAttendue

*Majeur · Unitaire · Ecrans du RPG* — `Source/Test/Unit/HMI/Interface/test_rpg_screens.cpp:87`

Exigences : `EX-IHM-091`

La regle de superposition est celle attendue pour chacun des huit ecrans.

**Étapes**

1. Interroger pausesGame pour chacun des huit ecrans.

**Résultat attendu**

- Vérifie que `pausesGame(RpgScreenId::CharacterSheet)` est vrai.
- Vérifie que `pausesGame(RpgScreenId::Skills)` est vrai.
- Vérifie que `pausesGame(RpgScreenId::Inventory)` est vrai.
- Vérifie que `pausesGame(RpgScreenId::QuestJournal)` est vrai.
- Vérifie que `pausesGame(RpgScreenId::Dialogue)` est vrai.
- Vérifie que `pausesGame(RpgScreenId::Merchant)` est vrai.
- Vérifie que `pausesGame(RpgScreenId::Company)` est vrai.
- Vérifie que `pausesGame(RpgScreenId::WorldMap)` est faux.
- Vérifie que `pausesGame(RpgScreenId::CombatHud)` est faux.

### RpgScreensTest.ChaqueEcranEstIdentifiableEtNonVide

*Majeur · Unitaire · Ecrans du RPG* — `Source/Test/Unit/HMI/Interface/test_rpg_screens.cpp:115`

Chaque ecran a un nom d'objet unique, un titre unique et une ossature non vide.

**Étapes**

1. Collecter noms d'objets et cles de titre.
2. Verifier leur unicite et la presence d'au moins un bloc par ecran.

**Résultat attendu**

- Vérifie que `objectNames.insert(descriptor.objectName).second` est vrai.
- Vérifie que `titleKeys.insert(descriptor.titleKey).second` est vrai.
- Vérifie que `descriptor.layout.leftColumn.empty()` est faux.
- Vérifie que `hmi::rpgScreenDescriptor(descriptor.id).id` vaut `descriptor.id`.

### RpgScreensTest.ChaqueBlocEstCoherentAvecSonGenre

*Majeur · Unitaire · Ecrans du RPG* — `Source/Test/Unit/HMI/Interface/test_rpg_screens.cpp:145`

Chaque bloc de l'ossature est coherent avec son genre.

**Étapes**

1. Parcourir les blocs des deux colonnes de chaque ecran.
2. Verifier les dimensions et libelles exiges par chaque genre.

**Résultat attendu**

- Vérifie que `block.fields.empty()` est faux.
- Vérifie que `field.labelKey[0]` diffère de `'\0'`.
- Vérifie que `block.columns` est strictement supérieur à `0`.
- Vérifie que `block.rows` est strictement supérieur à `0`.
- Vérifie que `block.rows > 0 || !block.valueIds.empty()` est vrai.
- Vérifie que `block.columns` est strictement supérieur à `0`.

### RpgScreensTest.ChaqueCleDeLOssatureExisteDansLesDeuxLangues

*Majeur · Unitaire · Ecrans du RPG* — `Source/Test/Unit/HMI/Interface/test_rpg_screens.cpp:199`

Exigences : `EX-REN-033`

Chaque cle de l'ossature des ecrans du RPG existe en francais et en anglais.

**Étapes**

1. Lire fr.lang et en.lang.
2. Verifier chaque cle de titre, de bloc et de champ, plus les cles du pied d'actions.

**Résultat attendu**

- Vérifie que `fr.empty()` est faux.
- Vérifie que `en.empty()` est faux.
- Vérifie que `fr.count(key) > 0` est vrai.
- Vérifie que `en.count(key) > 0` est vrai.

### RpgScreensTest.LesIdentifiantsDeValeurSontUniquesParEcran

*Majeur · Unitaire · Ecrans du RPG* — `Source/Test/Unit/HMI/Interface/test_rpg_screens.cpp:254`

Les identifiants de valeur sont uniques a l'interieur d'un ecran.

**Étapes**

1. Collecter les identifiants de valeur de chaque ecran.
2. Verifier qu'aucun ne se repete dans un meme ecran.

**Résultat attendu**

- Vérifie que `vus.insert(field.valueId).second` est vrai.
- Vérifie que `vus.insert(id).second` est vrai.

## test_screen_flow.cpp

### ScreenFlowTest.TransitionsAutoriseesMenentALEcranAttendu

*Critique · Unitaire · Machine à états des écrans* — `Source/Test/Unit/HMI/Interface/test_screen_flow.cpp:25`

Chaque transition autorisée mène à l'écran attendu.

**Étapes**

1. Résoudre chacune des transitions autorisées listées.
2. Vérifier l'écran résultant.

**Résultat attendu**

- Vérifie que `resolveTransition(menu, ScreenEvent::OpenGame)->screen` vaut `ScreenId::Game`.
- Vérifie que `resolveTransition(menu, ScreenEvent::OpenOptions)->screen` vaut `ScreenId::Options`.
- Vérifie que `resolveTransition(game, ScreenEvent::OpenMenu)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(game, ScreenEvent::OpenPause)->screen` vaut `ScreenId::Pause`.
- Vérifie que `resolveTransition(pause, ScreenEvent::ResumePause)->screen` vaut `ScreenId::Game`.
- Vérifie que `resolveTransition(pause, ScreenEvent::QuitPauseToMenu)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(pause, ScreenEvent::OpenOptions)->screen` vaut `ScreenId::Options`.
- Vérifie que `resolveTransition(optionsFromMenu, ScreenEvent::CloseOptions)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(optionsFromPause, ScreenEvent::CloseOptions)->screen` vaut `ScreenId::Pause`.
- Vérifie que `resolveTransition(pause, ScreenEvent::QuitPauseToMenu)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(menu, ScreenEvent::OpenCredits)->screen` vaut `ScreenId::Credits`.
- Vérifie que `resolveTransition(credits, ScreenEvent::CloseCredits)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(menu, ScreenEvent::OpenRpgScreen)->screen` vaut `ScreenId::RpgScreen`.
- Vérifie que `resolveTransition(game, ScreenEvent::OpenRpgScreen)->screen` vaut `ScreenId::RpgScreen`.
- Vérifie que `resolveTransition(pause, ScreenEvent::OpenRpgScreen)->screen` vaut `ScreenId::RpgScreen`.
- Vérifie que `resolveTransition(menu, ScreenEvent::OpenArena)->screen` vaut `ScreenId::Arena`.
- Vérifie que `resolveTransition(arena, ScreenEvent::CloseArena)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(arena, ScreenEvent::OpenMenu)->screen` vaut `ScreenId::Menu`.

### ScreenFlowTest.OptionsRevientVersSonEcranDOrigine

*Critique · Unitaire · Machine à états des écrans* — `Source/Test/Unit/HMI/Interface/test_screen_flow.cpp:72`

Options revient vers son écran d'origine (Menu ou Pause).

**Étapes**

1. Ouvrir Options depuis Menu, puis le fermer : vérifier le retour au Menu.
2. Ouvrir Options depuis Pause, puis le fermer : vérifier le retour à Pause.

**Résultat attendu**

- Vérifie que `openedFromMenu.has_value()` est vrai.
- Vérifie que `resolveTransition(*openedFromMenu, ScreenEvent::CloseOptions)->screen` vaut `ScreenId::Menu`.
- Vérifie que `openedFromPause.has_value()` est vrai.
- Vérifie que `resolveTransition(*openedFromPause, ScreenEvent::CloseOptions)->screen` vaut `ScreenId::Pause`.

### ScreenFlowTest.TransitionInterditeEstRefusee

*Critique · Unitaire · Machine à états des écrans* — `Source/Test/Unit/HMI/Interface/test_screen_flow.cpp:100`

Une transition interdite est refusée.

**Étapes**

1. Tenter Menu -> Pause directement.
2. Tenter Game -> Crédits.
3. Tenter Pause -> Colisée.

**Résultat attendu**

- Vérifie que `resolveTransition(menu, ScreenEvent::OpenPause)` vaut `std::nullopt`.
- Vérifie que `resolveTransition(menu, ScreenEvent::ResumePause)` vaut `std::nullopt`.
- Vérifie que `resolveTransition(game, ScreenEvent::OpenCredits)` vaut `std::nullopt`.
- Vérifie que `resolveTransition(menu, ScreenEvent::CloseRpgScreen)` vaut `std::nullopt`.
- Vérifie que `resolveTransition(pause, ScreenEvent::OpenArena)` vaut `std::nullopt`.
- Vérifie que `resolveTransition(menu, ScreenEvent::CloseArena)` vaut `std::nullopt`.

### ScreenFlowTest.LeColiseeRevientSurLaCarteQuandLeHerautYEnvoie

*Critique · Unitaire · Machine à états des écrans* — `Source/Test/Unit/HMI/Interface/test_screen_flow.cpp:128`

Le Colisee revient sur la carte quand le heraut y envoie.

**Étapes**

1. Ouvrir le Colisee depuis le menu, le refermer.
2. L'ouvrir depuis la carte, le refermer.

**Résultat attendu**

- Vérifie que `depuisLeMenu.has_value()` est vrai.
- Vérifie que `depuisLeMenu->screen` vaut `ScreenId::Arena`.
- Vérifie que `depuisLeMenu->arenaReturnTo` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(*depuisLeMenu, ScreenEvent::CloseArena)->screen` vaut `ScreenId::Menu`.
- Vérifie que `depuisLaCarte.has_value()` est vrai.
- Vérifie que `depuisLaCarte->screen` vaut `ScreenId::Arena`.
- Vérifie que `depuisLaCarte->arenaReturnTo` vaut `ScreenId::Game`.
- Vérifie que `resolveTransition(*depuisLaCarte, ScreenEvent::CloseArena)->screen` vaut `ScreenId::Game`.

### ScreenFlowTest.EcranDuRpgRevientVersSonEcranDOrigine

*Critique · Unitaire · Machine à états des écrans* — `Source/Test/Unit/HMI/Interface/test_screen_flow.cpp:158`

Exigences : `EX-IHM-090`

Un ecran du RPG revient vers son ecran d'origine (Menu, Game ou Pause).

**Étapes**

1. Ouvrir un ecran du RPG depuis le menu, le jeu puis la pause.
2. Le fermer a chaque fois et verifier l'ecran atteint.

**Résultat attendu**

- Vérifie que `opened.has_value()` est vrai.
- Vérifie que `opened->rpgReturnTo` vaut `expected`.
- Vérifie que `resolveTransition(*opened, ScreenEvent::CloseRpgScreen)->screen` vaut `expected`.

### ScreenFlowTest.LesEcransDeFinFermentLaPartie

*Critique · Unitaire · Machine à états des écrans* — `Source/Test/Unit/HMI/Interface/test_screen_flow.cpp:186`

Les ecrans de mort et de fin de la demo ferment la partie.

**Étapes**

1. Ouvrir la mort depuis un ecran du RPG (le HUD de combat) et depuis le jeu.
2. En sortir par OpenGame, OpenMenu, puis tenter la pause, les options, la fermeture d'un ecran du RPG.
3. Ouvrir la fin de la demo depuis un ecran du RPG (le dialogue) et depuis le jeu.
4. En sortir par les credits et le menu, puis tenter OpenGame.
5. Tenter les deux depuis le menu.

**Résultat attendu**

- Vérifie que `mort.has_value()` est vrai.
- Vérifie que `mort->screen` vaut `ScreenId::Death`.
- Vérifie que `resolveTransition(game, ScreenEvent::OpenDeath)->screen` vaut `ScreenId::Death`.
- Vérifie que `resolveTransition(*mort, ScreenEvent::OpenGame)->screen` vaut `ScreenId::Game`.
- Vérifie que `resolveTransition(*mort, ScreenEvent::OpenMenu)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(*mort, ScreenEvent::OpenPause).has_value()` est faux.
- Vérifie que `resolveTransition(*mort, ScreenEvent::OpenOptions).has_value()` est faux.
- Vérifie que `resolveTransition(*mort, ScreenEvent::CloseRpgScreen).has_value()` est faux.
- Vérifie que `fin.has_value()` est vrai.
- Vérifie que `fin->screen` vaut `ScreenId::DemoEnd`.
- Vérifie que `resolveTransition(game, ScreenEvent::OpenDemoEnd)->screen` vaut `ScreenId::DemoEnd`.
- Vérifie que `credits.has_value()` est vrai.
- Vérifie que `credits->screen` vaut `ScreenId::Credits`.
- Vérifie que `resolveTransition(*credits, ScreenEvent::CloseCredits)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(*fin, ScreenEvent::OpenMenu)->screen` vaut `ScreenId::Menu`.
- Vérifie que `resolveTransition(*fin, ScreenEvent::OpenGame).has_value()` est faux.
- Vérifie que `resolveTransition(menu, ScreenEvent::OpenDeath).has_value()` est faux.
- Vérifie que `resolveTransition(menu, ScreenEvent::OpenDemoEnd).has_value()` est faux.
