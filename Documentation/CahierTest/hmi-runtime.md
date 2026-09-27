# HMI · Runtime

Tests unitaires — **6 cas** (2 bloquants, 2 critiques, 2 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_encounter_model.cpp`](#test-encounter-modelcpp) | 3 | 1 | - | 2 | - |
| [`test_party_model.cpp`](#test-party-modelcpp) | 3 | 1 | 2 | - | - |

## Exigences vérifiées par cette page

Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la [matrice de traçabilité](couverture-exigences.md) les rassemble toutes.

| Exigence | Cas |
|---|---|
| `EX-EXP-013` | [`PartyModelTest.LEcranDeGroupeCompose`](#partymodeltestlecrandegroupecompose) |
| `EX-EXP-014` | [`PartyModelTest.ChangerDeMeneurChangeLaFigurineEtLePortrait`](#partymodeltestchangerdemeneurchangelafigurineetleportrait), [`PartyModelTest.LeMeneurEstCeluiQuiCombat`](#partymodeltestlemeneurestceluiquicombat) |
| `EX-IHM-091` | [`EncounterModelTest.DuDeclenchementAuRetourALExploration`](#encountermodeltestdudeclenchementauretouralexploration) |

## test_encounter_model.cpp

### EncounterModelTest.DuDeclenchementAuRetourALExploration

*Bloquant · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:67`

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
- Vérifie que `rencontre.fighters().size()` vaut `4`.
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

*Majeur · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:160`

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

*Majeur · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Runtime/test_encounter_model.cpp:188`

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

## test_party_model.cpp

### PartyModelTest.ChangerDeMeneurChangeLaFigurineEtLePortrait

*Bloquant · Unitaire · Groupe* — `Source/Test/Unit/HMI/Runtime/test_party_model.cpp:80`

Exigences : `EX-EXP-014`

Le groupe de depart compte les quatre fiches pre-tirees ; passer la main change la figurine menee, le portrait du meneur et la voix du dialogue.

**Étapes**

1. Nouvelle partie dans le donjon d'essai.
2. Passer la main au suivant (la touche `Tab`).
3. Ouvrir un dialogue.

**Résultat attendu**

- Vérifie que `identifiants(monde.partyMembers())` vaut `(QStringList{"heros-brawler", "heros-mage", "heros-priest", "heros-scoundrel"})`.
- Vérifie que `monde.leaderId()` vaut `QStringLiteral("heros-brawler")`.
- Vérifie que `monde.heroFigure()` vaut `QStringLiteral("Common/Characters/Heroes/brawler")`.
- Vérifie que `monde.play().followerFigures()` vaut `(std::vector<std::string>{"Common/Characters/Heroes/mage", "Common/Characters/Heroes/priest", "Common/Characters/Heroes/scoundrel"})`.
- Vérifie que `avant` vaut `QUrl::fromLocalFile(QString::fromStdString(portraitDuBrawler.string()))`.
- Vérifie que `monde.rotateLeader()` est vrai.
- Vérifie que `annonces` vaut `1`.
- Vérifie que `monde.leaderId()` vaut `QStringLiteral("heros-mage")`.
- Vérifie que `monde.leaderName()` vaut `QStringLiteral("Faelar Trace-Carte")`.
- Vérifie que `monde.heroFigure()` vaut `QStringLiteral("Common/Characters/Heroes/mage")`.
- Vérifie que `monde.play().followerFigures().back()` vaut `"Common/Characters/Heroes/brawler"`.
- Vérifie que `monde.leaderPortrait()` diffère de `avant`.
- Vérifie que `figures.size()` est supérieur ou égal à `4U`.
- Vérifie que `figures.back().hero` est vrai.
- Vérifie que `suiveurs` vaut `3U`.
- Vérifie que `dialogue.partyVoice()` vaut `QStringLiteral("Faelar Trace-Carte")`.

### PartyModelTest.LeMeneurEstCeluiQuiCombat

*Critique · Unitaire · Groupe* — `Source/Test/Unit/HMI/Runtime/test_party_model.cpp:137`

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

*Critique · Unitaire · Groupe* — `Source/Test/Unit/HMI/Runtime/test_party_model.cpp:170`

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
- Vérifie que `identifiants(monde.partyMembers())` vaut `(QStringList{"heros-brawler", "heros-mage", "heros-priest", "heros-scoundrel"})`.
