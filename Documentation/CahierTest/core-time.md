# Core · Time

Tests unitaires — **7 cas** (1 critique, 6 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_fixed_timestep.cpp`](#test-fixed-timestepcpp) | 7 | - | 1 | 6 | - |

## Exigences vérifiées par cette page

Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la [matrice de traçabilité](couverture-exigences.md) les rassemble toutes.

| Exigence | Cas |
|---|---|
| `EX-GP-041` | [`FixedTimestepTest.PauseSansAppelNAccumuleAucunPas`](#fixedtimesteptestpausesansappelnaccumuleaucunpas) |

## test_fixed_timestep.cpp

### FixedTimestepTest.UnPasExact

*Majeur · Unitaire · Fixed Timestep* — `Source/Test/Unit/Core/Time/test_fixed_timestep.cpp:19`

Un temps écoulé égal au pas fixe produit exactement un pas.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `timestep.advance(STEP)` vaut `1`.

### FixedTimestepTest.TempsInsuffisant

*Majeur · Unitaire · Fixed Timestep* — `Source/Test/Unit/Core/Time/test_fixed_timestep.cpp:34`

Un temps écoulé inférieur au pas ne produit aucun pas.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `timestep.advance(STEP * 0.5f)` vaut `0`.

### FixedTimestepTest.TempsNulOuNegatif

*Majeur · Unitaire · Fixed Timestep* — `Source/Test/Unit/Core/Time/test_fixed_timestep.cpp:49`

Un temps écoulé nul ou négatif ne produit aucun pas.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `timestep.advance(0.0f)` vaut `0`.
- Vérifie que `timestep.advance(-1.0f)` vaut `0`.

### FixedTimestepTest.ResteConserve

*Majeur · Unitaire · Fixed Timestep* — `Source/Test/Unit/Core/Time/test_fixed_timestep.cpp:65`

2,5 pas donnent 2 pas, et le reste (0,5 pas) est conservé puis complété.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `timestep.advance(STEP * 2.5f)` vaut `2`.
- Vérifie que `timestep.interpolationAlpha()` vaut `0.5f`, à `1e-4f` près.
- Vérifie que `timestep.advance(STEP * 0.6f)` vaut `1`.

### FixedTimestepTest.PlafondAntiSpirale

*Majeur · Unitaire · Fixed Timestep* — `Source/Test/Unit/Core/Time/test_fixed_timestep.cpp:85`

Un temps écoulé énorme est plafonné (anti-spirale de la mort).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `timestep.advance(STEP * 1000.0f)` vaut `maximum`.
- Vérifie que `timestep.advance(0.0f)` vaut `0`.

### FixedTimestepTest.PasFixeExpose

*Majeur · Unitaire · Fixed Timestep* — `Source/Test/Unit/Core/Time/test_fixed_timestep.cpp:103`

Le pas fixe exposé correspond à la configuration.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `timestep.fixedDeltaSeconds()` vaut `STEP`, à `1e-6f` près.

### FixedTimestepTest.PauseSansAppelNAccumuleAucunPas

*Critique · Unitaire · Fixed Timestep* — `Source/Test/Unit/Core/Time/test_fixed_timestep.cpp:122`

Exigences : `EX-GP-041`

Une pause simulée (aucun appel à advance()) n'accumule aucun pas.

**Étapes**

1. Avancer normalement de quelques pas.
2. Simuler une pause de longue durée en n'appelant PAS advance() (aucun appel, pas un appel à zéro).
3. Reprendre avec un petit delta, comme après réarmement de l'horloge de référence.

**Résultat attendu**

- Vérifie que `timestep.advance(STEP * 3.0f)` vaut `3`.
- Vérifie que `timestep.advance(STEP)` vaut `1`.
