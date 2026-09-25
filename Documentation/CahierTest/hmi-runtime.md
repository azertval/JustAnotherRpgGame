# HMI · Runtime

Tests unitaires — **3 cas** (1 bloquant, 2 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_encounter_model.cpp`](#test-encounter-modelcpp) | 3 | 1 | - | 2 | - |

## Exigences vérifiées par cette page

Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la [matrice de traçabilité](couverture-exigences.md) les rassemble toutes.

| Exigence | Cas |
|---|---|
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
