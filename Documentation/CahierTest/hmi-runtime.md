# HMI · Runtime

Tests unitaires — **12 cas** (3 bloquants, 7 critiques, 2 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_character_sheet_model.cpp`](#test-character-sheet-modelcpp) | 2 | 1 | 1 | - | - |
| [`test_encounter_model.cpp`](#test-encounter-modelcpp) | 6 | 1 | 3 | 2 | - |
| [`test_party_model.cpp`](#test-party-modelcpp) | 4 | 1 | 3 | - | - |

## Exigences vérifiées par cette page

Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la [matrice de traçabilité](couverture-exigences.md) les rassemble toutes.

| Exigence | Cas |
|---|---|
| `EX-CBT-061` | [`EncounterModelTest.LeRejeuAGraineFixeeDonneLeMemeCombat`](#encountermodeltestlerejeuagrainefixeedonnelememecombat) |
| `EX-EXP-013` | [`PartyModelTest.LEcranDeGroupeCompose`](#partymodeltestlecrandegroupecompose) |
| `EX-EXP-014` | [`PartyModelTest.ChangerDeMeneurChangeLaFigurineEtLePortrait`](#partymodeltestchangerdemeneurchangelafigurineetleportrait), [`PartyModelTest.LeMeneurEstCeluiQuiCombat`](#partymodeltestlemeneurestceluiquicombat) |
| `EX-IHM-091` | [`EncounterModelTest.DuDeclenchementAuRetourALExploration`](#encountermodeltestdudeclenchementauretouralexploration) |
| `EX-IHM-108` | [`EncounterModelTest.LInterfaceDeGroupeLitLaVueModele`](#encountermodeltestlinterfacedegroupelitlavuemodele) |
| `EX-IHM-109` | [`CharacterSheetModelTest.LesQuatreFichesSontLeursPagesDuLivre`](#charactersheetmodeltestlesquatrefichessontleurspagesdulivre) |

## test_character_sheet_model.cpp

### CharacterSheetModelTest.LesQuatreFichesSontLeursPagesDuLivre

*Bloquant · Unitaire · Fiche de personnage* — `Source/Test/Unit/HMI/Runtime/test_character_sheet_model.cpp:65`

Exigences : `EX-IHM-109`

Les quatre pages du Player's Guide, ouvertes par leur identifiant.

**Étapes**

1. Sans partie, charger chaque fiche par `loadCharacter(id)`.
2. Lire nom, classe, points de vie, CA, capacites acquises, sorts connus, capacites a venir.

**Résultat attendu**

- Vérifie que `fiche.characterId()` vaut `QString::fromUtf8(page.id)`.
- Vérifie que `fiche.name()` vaut `page.name`.
- Vérifie que `fiche.className()` vaut `page.className`.
- Vérifie que `fiche.level()` vaut `QStringLiteral("1")`.
- Vérifie que `fiche.hitPoints()` vaut `page.hitPoints`.
- Vérifie que `fiche.armorClass()` vaut `page.armorClass`.
- Vérifie que `names(fiche.capacities())` vaut `page.capacities`.
- Vérifie que `names(fiche.spells())` vaut `page.spells`.
- Vérifie que `row.toMap().value("level").toInt()` vaut `1`.
- Vérifie que `row.toMap().value("iconKey").toString().startsWith( QStringLiteral("ui/icon/capacity/"))` est vrai.
- Vérifie que `fiche.upcomingCapacities().isEmpty()` est faux.
- Vérifie que `fiche.attacks().size()` vaut `1`.
- Vérifie que `fiche.attacks().front().toMap().value("value").toString().startsWith('+')` est vrai.
- Vérifie que `texte.contains(QStringLiteral(" p. "))` est faux.
- Vérifie que `texte.isEmpty()` est faux.
- Vérifie que `sort.value("usesText").toString()` vaut `QStringLiteral("à volonté")`.
- Vérifie que `sort.value("usesText").toString()` vaut `QStringLiteral("2 / 2")`.
- Vérifie que `sort.value("iconKey").toString().startsWith(QStringLiteral("ui/icon/spell/"))` est vrai.
- Vérifie que `sort.value("details").toString().isEmpty()` est faux.
- Vérifie que `sort.value("text").toString().contains(QStringLiteral(" p. "))` est faux.
- Vérifie que `sort.value("text").toString().isEmpty()` est faux.
- Vérifie que `sort.value("components").toString().isEmpty()` est faux.
- Vérifie que `sort.value("range").toString().isEmpty()` est faux.
- Vérifie que `auNiveau3` vaut `(std::set<QString>{"Sneak Attack Simplified", "Adventurer's Aptitude"})`.
- Vérifie que `fiche.name().isEmpty()` est faux.

### CharacterSheetModelTest.LaMonteeDeNiveauDonneeSeVoitPartout

*Critique · Unitaire · Fiche de personnage* — `Source/Test/Unit/HMI/Runtime/test_character_sheet_model.cpp:168`

Un niveau donne se voit partout, et n'est pas un soin.

**Étapes**

1. Ouvrir le donjon ; noter 5 PV au Priest.
2. `levelUp("heros-priest")` deux fois, puis `levelUp("party")`.
3. Lire la fiche du Priest, l'ecran de groupe, le panneau de l'actif d'une rencontre ; monter cinq fois de plus.

**Résultat attendu**

- Vérifie que `monde.levelUp(QStringLiteral("heros-priest"))` est vrai.
- Vérifie que `monde.levelUp(QStringLiteral("heros-priest"))` est vrai.
- Vérifie que `monde.levelUp(QStringLiteral("inconnu"))` est faux.
- Vérifie que `fiche.level()` vaut `QStringLiteral("3")`.
- Vérifie que `fiche.hitPoints()` vaut `QStringLiteral("23 / 30")`.
- Vérifie que `names(fiche.spells()).contains(QStringLiteral("Epargner les mourants"))` est vrai.
- Vérifie que `names(fiche.spells()).contains(QStringLiteral("Arme spirituelle"))` est vrai.
- Vérifie que `monde.levelUp(QStringLiteral("party"))` est vrai.
- Vérifie que `membre.value("level").toString()` vaut `membre.value("id").toString() == QStringLiteral("heros-priest") ? QStringLiteral("4") : QStringLiteral("2")`.
- Vérifie que `monde.shownCharacterId()` vaut `QStringLiteral("heros-brawler")`.
- Vérifie que `monde.shownCharacterId()` vaut `QStringLiteral("heros-mage")`.
- Vérifie que `fiche.characterId()` vaut `QStringLiteral("heros-mage")`.
- Vérifie que `fiche.level()` vaut `QStringLiteral("2")`.
- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `actif.value("level").toInt()` est supérieur ou égal à `2`.
- Vérifie que `monde.levelUp(QStringLiteral("heros-priest"))` est faux.

## test_encounter_model.cpp

### EncounterModelTest.DuDeclenchementAuRetourALExploration

*Bloquant · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:84`

Exigences : `EX-IHM-091`

Du declenchement sur la carte au retour a l'exploration, sans fenetre.

**Étapes**

1. Ouvrir le donjon d'essai, le heros devant le maitre d'arene, dans la zone « salle ».
2. Engager « rats-du-donjon » a la graine 2026.
3. Jouer : attaquer le rat le plus proche, finir le tour, jusqu'a l'issue ou dix rounds.
4. Quitter.

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `rencontre.active()` est vrai.
- Vérifie que `monde.frozen()` est vrai.
- Vérifie que `monde.showsCombat()` est vrai.
- Vérifie que `rencontre.zoneColumn()` vaut `10`.
- Vérifie que `rencontre.zoneRow()` vaut `10`.
- Vérifie que `rencontre.setup()` diffère de `nullptr`.
- Vérifie que `rencontre.setup()->heroCell` vaut `(core::GridPosition{.column = 14, .row = 9})`.
- Vérifie que `rencontre.setup()->partyCells.size()` vaut `4U`.
- Vérifie que `rencontre.fighters().size()` vaut `7`.
- Vérifie que `rencontre.partyMembers().size()` vaut `4`.
- Vérifie que `rencontre.partyMembers().front().toMap().value("label").toString()` vaut `QStringLiteral("Grom Tranche-Écaille")`.
- Vérifie que `rencontre.encounterName()` vaut `QStringLiteral("Les rats du donjon")`.
- Vérifie que `figure.combatant` est vrai.
- Vérifie que `figure.point.x` est supérieur ou égal à `10.0F`.
- Vérifie que `figure.point.y` est supérieur ou égal à `10.0F`.
- Vérifie que `heros` est vrai.
- Vérifie que `bandes.contains(std::string{hmi::figure_clips::ATTACK})` est vrai.
- Vérifie que `bandes.contains(std::string{hmi::figure_clips::DEATH})` est vrai.
- Vérifie que `rencontre.ended()` est vrai.
- Vérifie que `rencontre.outcome().isEmpty()` est faux.
- Vérifie que `rencontre.active()` est faux.
- Vérifie que `monde.frozen()` est faux.
- Vérifie que `monde.showsCombat()` est faux.
- Vérifie que `fini.size()` vaut `1`.
- Vérifie que `fini.front()` vaut `issue`.
- Vérifie que `rencontre.setup() == nullptr` est vrai.
- Vérifie que `arrivee.column` est supérieur ou égal à `10`.
- Vérifie que `arrivee.row` est supérieur ou égal à `10`.
- Vérifie que `figure.combatant` est faux.
- Vérifie que `mannequin` est vrai.

### EncounterModelTest.UnRefusLaisseLExplorationIntacte

*Majeur · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:181`

Un refus de montage laisse l'exploration intacte.

**Étapes**

1. Ouvrir le donjon a la porte (19, 32), hors de la zone.
2. Engager une rencontre inconnue, puis « rats-du-donjon ».

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("dragons"))` est faux.
- Vérifie que `rencontre.active()` est faux.
- Vérifie que `rencontre.status().isEmpty()` est faux.
- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est faux.
- Vérifie que `rencontre.active()` est faux.
- Vérifie que `monde.frozen()` est faux.
- Vérifie que `monde.showsCombat()` est faux.
- Vérifie que `rencontre.status().indexOf(QStringLiteral("zone"))` diffère de `-1`.

### EncounterModelTest.LesGestesAttendentLaFinDUnMouvement

*Majeur · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:209`

Les gestes attendent la fin d'un mouvement.

**Étapes**

1. Engager les rats, avancer d'un pas : l'IA a pu jouer, la file est occupee.
2. Tant que la file joue, finir le tour ; puis sauter l'animation.

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `rencontre.busy()` est vrai.
- Vérifie que `rencontre.journal().size()` vaut `lignes`.
- Vérifie que `rencontre.busy()` est faux.
- Vérifie que `rencontre.active()` est vrai.
- Vérifie que `rencontre.ended()` est vrai.
- Vérifie que `rencontre.active()` est faux.

### EncounterModelTest.LeRejeuAGraineFixeeDonneLeMemeCombat

*Critique · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:253`

Exigences : `EX-CBT-061`

Deux combats de groupe a la meme graine sont identiques.

**Étapes**

1. Engager les rats a la graine 41, jouer trois rounds (attaquer, finir le tour), relever le journal, fuir.
2. Reposer le groupe, recommencer a la meme graine.

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `premier` vaut `second`.
- Vérifie que `membres.size()` est strictement supérieur à `1U`.
- Vérifie que `membres.contains(-1)` est faux.

### EncounterModelTest.LeCombatLaisseAuxFichesCeQuIlEnReste

*Critique · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:302`

Le registre du groupe : points de vie relus, mort qui ne suit plus.

**Étapes**

1. Noter au registre 5 PV pour le Brawler ; engager les rats.
2. Fuir ; lire le registre.
3. Enterrer le Brawler, puis tenter d'enterrer tout le monde.

**Résultat attendu**

- Vérifie que `groupe.leaderHitPoints()` vaut `QStringLiteral("5 / 15")`.
- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `rencontre.heroHitPoints()` vaut `QStringLiteral("5 / 15")`.
- Vérifie que `rencontre.partyMembers().front().toMap().value("value").toString()` vaut `QStringLiteral("5 / 15")`.
- Vérifie que `rencontre.ended()` est vrai.
- Vérifie que `record` diffère de `nullptr`.
- Vérifie que `record->hitPoints.has_value()` est vrai.
- Vérifie que `*record->hitPoints` est supérieur ou égal à `1`.
- Vérifie que `monde.buryMember("heros-brawler")` est vrai.
- Vérifie que `monde.leaderId()` vaut `QStringLiteral("heros-priest")`.
- Vérifie que `monde.party().size()` vaut `3U`.
- Vérifie que `monde.play().session().followers()` vaut `2U`.
- Vérifie que `monde.ledger().record("heros-brawler")` vaut `nullptr`.
- Vérifie que `monde.buryMember("heros-priest")` est vrai.
- Vérifie que `monde.buryMember("heros-scoundrel")` est vrai.
- Vérifie que `monde.buryMember("heros-mage")` est faux.
- Vérifie que `monde.party().size()` vaut `1U`.

### EncounterModelTest.LInterfaceDeGroupeLitLaVueModele

*Critique · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:355`

Exigences : `EX-IHM-108`

Round, jetons, panneau de l'actif, actions detaillees, previsualisation.

**Étapes**

1. Engager les rats a la graine 2026, jusqu'au tour du joueur.
2. Lire `round`, `turnOrder`, `activeProfile`, `turnActions`.
3. Viser le rat le plus proche avec la premiere attaque ; puis ramener le curseur sur soi ; puis viser une case libre atteignable.

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `rencontre.ended()` est faux.
- Vérifie que `rencontre.round()` vaut `1`.
- Vérifie que `ordre.size()` vaut `7`.
- Vérifie que `rang.value("initials").toString().isEmpty()` est faux.
- Vérifie que `jeton.toString().endsWith(QStringLiteral("token.png"))` est vrai.
- Vérifie que `jeton.isEmpty()` est vrai.
- Vérifie que `actif.value("name").toString()` vaut `rencontre.activeName()`.
- Vérifie que `actif.value("side").toString()` vaut `QStringLiteral("allies")`.
- Vérifie que `classes.contains(actif.value("classId").toString())` est vrai.
- Vérifie que `actif.value("level").toInt()` vaut `1`.
- Vérifie que `actif.value("action").toInt()` vaut `1`.
- Vérifie que `actif.value("actionMax").toInt()` vaut `1`.
- Vérifie que `actif.value("movementMax").toInt()` est strictement supérieur à `0`.
- Vérifie que `actif.value("armorClass").toInt()` est strictement supérieur à `9`.
- Vérifie que `actif.value("capacities").toList().isEmpty()` est faux.
- Vérifie que `capacite.toMap().value("iconKey").toString().startsWith( QStringLiteral("ui/icon/capacity/"))` est vrai.
- Vérifie que `actions.isEmpty()` est faux.
- Vérifie que `actions.front().toMap().value("kind").toString()` vaut `QStringLiteral("attack")`.
- Vérifie que `actions.front().toMap().value("detail").toString().startsWith('+')` est vrai.
- Vérifie que `actions.front().toMap().value("uses").toInt()` vaut `-1`.
- Vérifie que `actions.front().toMap().value("iconKey").toString().startsWith( QStringLiteral("ui/icon/action/"))` est vrai.
- Vérifie que `apercu.value("kind").toString()` vaut `QStringLiteral("attack")`.
- Vérifie que `apercu.value("title").toString().contains(QStringLiteral("›"))` est vrai.
- Vérifie que `lignes.isEmpty()` est faux.
- Vérifie que `lignes.front().toMap().value("label").toString()` vaut `QStringLiteral("Toucher")`.
- Vérifie que `apercu.value("expected").toString().isEmpty()` est faux.
- Vérifie que `lignes.front().toMap().value("label").toString()` vaut `QStringLiteral("Cible")`.
- Vérifie que `apercu.value("kind").toString()` vaut `QStringLiteral("attack")`.
- Vérifie que `apercu.value("valid").toBool()` est faux.
- Vérifie que `atteignables.isEmpty()` est faux.
- Vérifie que `apercu.value("kind").toString()` vaut `QStringLiteral("move")`.
- Vérifie que `apercu.value("valid").toBool()` est vrai.
- Vérifie que `apercu.value("lines").toList().front().toMap().value("label").toString()` vaut `QStringLiteral("Chemin")`.

## test_party_model.cpp

### PartyModelTest.ChangerDeMeneurChangeLaFigurineEtLePortrait

*Bloquant · Unitaire · Groupe* — `Source/Test/Unit/HMI/Runtime/test_party_model.cpp:83`

Exigences : `EX-EXP-014`

Le groupe de depart compte les quatre fiches pre-tirees ; passer la main change la figurine menee, le portrait du meneur et la voix du dialogue.

**Étapes**

1. Nouvelle partie dans le donjon d'essai.
2. Passer la main au suivant (la touche `Tab`).
3. Ouvrir un dialogue.

**Résultat attendu**

- Vérifie que `identifiants(monde.partyMembers())` vaut `(QStringList{"heros-brawler", "heros-priest", "heros-scoundrel", "heros-mage"})`.
- Vérifie que `monde.leaderId()` vaut `QStringLiteral("heros-brawler")`.
- Vérifie que `monde.heroFigure()` vaut `QStringLiteral("Common/Characters/Heroes/brawler")`.
- Vérifie que `monde.play().followerFigures()` vaut `(std::vector<std::string>{"Common/Characters/Heroes/priest", "Common/Characters/Heroes/scoundrel", "Common/Characters/Heroes/mage"})`.
- Vérifie que `avant` vaut `QUrl::fromLocalFile(QString::fromStdString(portraitDuBrawler.string()))`.
- Vérifie que `monde.rotateLeader()` est vrai.
- Vérifie que `annonces` vaut `1`.
- Vérifie que `monde.leaderId()` vaut `QStringLiteral("heros-priest")`.
- Vérifie que `monde.leaderName()` vaut `QStringLiteral("Helga Pierre-Sûre")`.
- Vérifie que `monde.heroFigure()` vaut `QStringLiteral("Common/Characters/Heroes/priest")`.
- Vérifie que `monde.play().followerFigures().back()` vaut `"Common/Characters/Heroes/brawler"`.
- Vérifie que `monde.leaderPortrait()` diffère de `avant`.
- Vérifie que `figures.size()` est supérieur ou égal à `4U`.
- Vérifie que `figures.back().hero` est vrai.
- Vérifie que `suiveurs` vaut `3U`.
- Vérifie que `dialogue.partyVoice()` vaut `QStringLiteral("Helga Pierre-Sûre")`.

### PartyModelTest.LeJoueurChoisitQuiParle

*Critique · Unitaire · Groupe* — `Source/Test/Unit/HMI/Runtime/test_party_model.cpp:140`

Dans le dialogue, le menu du bas donne la parole a un membre du groupe : le jet de Persuasion se fait avec ses modificateurs.

**Étapes**

1. Ouvrir le dialogue du garde a la graine 7 : le meneur (Grom, Charisme 8) parle ; tenter de le convaincre.
2. Rouvrir a la meme graine, donner la parole a Nessa (Charisme 13), tenter de nouveau.

**Résultat attendu**

- Vérifie que `dialogue.selectVoice(voix)` est vrai.
- Vérifie que `dialogue.selectVoice(QStringLiteral("heros-inconnu"))` est faux.
- Vérifie que `voix.size()` vaut `4`.
- Vérifie que `voix.front().toMap().value(QStringLiteral("current")).toBool()` est vrai.
- Vérifie que `dialogue.voiceId()` vaut `QStringLiteral("heros-brawler")`.
- Vérifie que `dialogue.voiceId()` vaut `QStringLiteral("heros-mage")`.
- Vérifie que `deDeGrom.isEmpty()` est faux.
- Vérifie que `deDeGrom` vaut `deDeNessa`.
- Vérifie que `totalDeNessa` est strictement supérieur à `totalDeGrom`.

### PartyModelTest.LeMeneurEstCeluiQuiCombat

*Critique · Unitaire · Groupe* — `Source/Test/Unit/HMI/Runtime/test_party_model.cpp:193`

Exigences : `EX-EXP-014`

Une rencontre engagee apres un changement de meneur met le nouveau meneur en jeu.

**Étapes**

1. Engager « rats-du-donjon », puis fuir.
2. Faire mener la Scoundrel, engager de nouveau.

**Résultat attendu**

- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `rencontre.heroName()` vaut `QStringLiteral("Grom Tranche-Écaille")`.
- Vérifie que `rencontre.active()` est faux.
- Vérifie que `monde.setLeader(QStringLiteral("heros-scoundrel"))` est vrai.
- Vérifie que `rencontre.begin(QStringLiteral("rats-du-donjon"))` est vrai.
- Vérifie que `rencontre.heroName()` vaut `QStringLiteral("Nessa Double-Vie")`.

### PartyModelTest.LEcranDeGroupeCompose

*Critique · Unitaire · Groupe* — `Source/Test/Unit/HMI/Runtime/test_party_model.cpp:226`

Exigences : `EX-EXP-013`

L'ecran de groupe lit la fiche des quatre et compose le groupe de la partie.

**Étapes**

1. Ouvrir le modele de l'ecran de groupe.
2. Laisser la Scoundrel, puis tenter de laisser tous les autres.
3. Reprendre la Scoundrel, la faire mener, reculer le Mage.

**Résultat attendu**

- Vérifie que `groupe.candidates().size()` vaut `4`.
- Vérifie que `brawler.value(QStringLiteral("value")).toString()` vaut `QStringLiteral("15 / 15")`.
- Vérifie que `brawler.value(QStringLiteral("armorClass")).toString()` vaut `QStringLiteral("14")`.
- Vérifie que `brawler.value(QStringLiteral("rank")).toInt()` vaut `0`.
- Vérifie que `brawler.value(QStringLiteral("leader")).toBool()` est vrai.
- Vérifie que `groupe.members().front().toMap().value(QStringLiteral("ratio")).toDouble()` vaut `1.0` (comparaison flottante).
- Vérifie que `groupe.leaderHitPoints()` vaut `QStringLiteral("15 / 15")`.
- Vérifie que `groupe.toggleMember(QStringLiteral("heros-scoundrel"))` est vrai.
- Vérifie que `groupe.size()` vaut `3`.
- Vérifie que `monde.play().session().followers()` vaut `2U`.
- Vérifie que `groupe.toggleMember(QStringLiteral("heros-mage"))` est vrai.
- Vérifie que `groupe.toggleMember(QStringLiteral("heros-priest"))` est vrai.
- Vérifie que `groupe.toggleMember(QStringLiteral("heros-brawler"))` est faux.
- Vérifie que `monde.play().session().followers()` vaut `0U`.
- Vérifie que `groupe.toggleMember(QStringLiteral("heros-mage"))` est vrai.
- Vérifie que `groupe.toggleMember(QStringLiteral("heros-scoundrel"))` est vrai.
- Vérifie que `groupe.setLeader(QStringLiteral("heros-scoundrel"))` est vrai.
- Vérifie que `identifiants(groupe.members())` vaut `(QStringList{"heros-scoundrel", "heros-brawler", "heros-mage"})`.
- Vérifie que `groupe.moveMember(QStringLiteral("heros-brawler"), 1)` est vrai.
- Vérifie que `identifiants(groupe.members())` vaut `(QStringList{"heros-scoundrel", "heros-mage", "heros-brawler"})`.
- Vérifie que `groupe.moveMember(QStringLiteral("heros-brawler"), 1)` est faux.
- Vérifie que `groupe.leaderName()` vaut `QStringLiteral("Nessa Double-Vie")`.
- Vérifie que `identifiants(monde.partyMembers())` vaut `(QStringList{"heros-brawler", "heros-priest", "heros-scoundrel", "heros-mage"})`.
