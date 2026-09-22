# Core · Math

Tests unitaires — **26 cas** (4 bloquants, 18 majeurs, 4 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_deterministic_random.cpp`](#test-deterministic-randomcpp) | 6 | 4 | - | 2 | - |
| [`test_math_utils.cpp`](#test-math-utilscpp) | 5 | - | - | 2 | 3 |
| [`test_rect.cpp`](#test-rectcpp) | 7 | - | - | 7 | - |
| [`test_vector2.cpp`](#test-vector2cpp) | 8 | - | - | 7 | 1 |

## test_deterministic_random.cpp

### DeterministicRandomTest.SplitMix64VecteursDeReference

*Bloquant · Unitaire · Core Math* — `Source/Test/Unit/Core/Math/test_deterministic_random.cpp:25`

splitMix64 : vecteurs de référence.

**Étapes**

1. `splitMix64` sur `0`, `1` et `0x0123456789ABCDEF`.

**Résultat attendu**

- Vérifie que `core::splitMix64(0x0000000000000000ULL)` vaut `0x0000000000000000ULL`.
- Vérifie que `core::splitMix64(0x0000000000000001ULL)` vaut `0x5692161D100B05E5ULL`.
- Vérifie que `core::splitMix64(0x0123456789ABCDEFULL)` vaut `0xB2C058E4EBB5112CULL`.

### DeterministicRandomTest.DeriveSeedVecteursDeReference

*Bloquant · Unitaire · Core Math* — `Source/Test/Unit/Core/Math/test_deterministic_random.cpp:44`

deriveSeed : vecteurs de référence et séparation du triplet.

**Étapes**

1. `deriveSeed` sur `(1,0,0)`, `(1,0,1)` et `(1,1,0)`.
2. Comparer les trois entre eux.

**Résultat attendu**

- Vérifie que `core::deriveSeed(1, 0, 0)` vaut `0x71CAC37448049CE4ULL`.
- Vérifie que `core::deriveSeed(1, 0, 1)` vaut `0xC14BF009DE212E89ULL`.
- Vérifie que `core::deriveSeed(1, 1, 0)` vaut `0x98DCCAA31A8BAF69ULL`.
- Vérifie que `core::deriveSeed(1, 0, 1)` diffère de `core::deriveSeed(1, 1, 0)`.

### DeterministicRandomTest.SuiteDeReferenceDepuisUneGraineFixee

*Bloquant · Unitaire · Core Math* — `Source/Test/Unit/Core/Math/test_deterministic_random.cpp:64`

DeterministicRandom : suite de référence depuis une graine fixée.

**Étapes**

1. `DeterministicRandom(42)`.
2. Cinq appels à `nextUInt32()`.

**Résultat attendu**

- Vérifie que `random.nextUInt32()` vaut `0xBDD73226U`.
- Vérifie que `random.nextUInt32()` vaut `0x28EFE333U`.
- Vérifie que `random.nextUInt32()` vaut `0x47526757U`.
- Vérifie que `random.nextUInt32()` vaut `0x581CE1FFU`.
- Vérifie que `random.nextUInt32()` vaut `0x09BC585AU`.

### DeterministicRandomTest.MemeGraineMemeSuite

*Bloquant · Unitaire · Core Math* — `Source/Test/Unit/Core/Math/test_deterministic_random.cpp:82`

DeterministicRandom : même graine, même suite ; graines différentes, suites différentes.

**Étapes**

1. Deux instances de graine `7`, 16 tirages chacune.
2. Une instance de graine `8`, premier tirage.

**Résultat attendu**

- Vérifie que `first.nextUInt32()` vaut `second.nextUInt32()`.
- Vérifie que `other.nextUInt32()` diffère de `reference.nextUInt32()`.

### DeterministicRandomTest.NextFloat01ExclutLaBorneHaute

*Majeur · Unitaire · Core Math* — `Source/Test/Unit/Core/Math/test_deterministic_random.cpp:105`

nextFloat01 : borne haute exclue.

**Étapes**

1. 10 000 tirages depuis une graine fixée.

**Résultat attendu**

- Vérifie que `value` est supérieur ou égal à `0.0f`.
- Vérifie que `value` est strictement inférieur à `1.0f`.

### DeterministicRandomTest.NextRangeRespecteLesBornes

*Majeur · Unitaire · Core Math* — `Source/Test/Unit/Core/Math/test_deterministic_random.cpp:123`

nextRange : bornes respectées, intervalle dégénéré rendu exactement.

**Étapes**

1. 1 000 tirages dans `[-2, 5]`.
2. Un tirage dans `[3, 3]`.

**Résultat attendu**

- Vérifie que `value` est supérieur ou égal à `-2.0f`.
- Vérifie que `value` est inférieur ou égal à `5.0f`.
- Vérifie que `random.nextRange(3.0f, 3.0f)` vaut `3.0f` (comparaison flottante).

## test_math_utils.cpp

### MathUtilsTest.EgaliteExacte

*Mineur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_math_utils.cpp:15`

approximatelyEqual : égalité exacte

**Étapes**

1. Comparer des valeurs identiques (1, 0, -2,5).

**Résultat attendu**

- Vérifie que `core::approximatelyEqual(1.0f, 1.0f)` est vrai.
- Vérifie que `core::approximatelyEqual(0.0f, 0.0f)` est vrai.
- Vérifie que `core::approximatelyEqual(-2.5f, -2.5f)` est vrai.

### MathUtilsTest.ToleranceParDefaut

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_math_utils.cpp:29`

approximatelyEqual : tolérance par défaut

**Étapes**

1. Comparer 1 et 1+1e-6.
2. Comparer 1 et 1,1.

**Résultat attendu**

- Vérifie que `core::approximatelyEqual(1.0f, 1.0f + 1e-6f)` est vrai.
- Vérifie que `core::approximatelyEqual(1.0f, 1.1f)` est faux.

### MathUtilsTest.MiseALEchelleGrandesMagnitudes

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_math_utils.cpp:42`

approximatelyEqual : mise à l'échelle des grandes magnitudes

**Étapes**

1. Comparer 1e6 et 1e6+1.
2. Comparer 1e6 et 1e6+1000.

**Résultat attendu**

- Vérifie que `core::approximatelyEqual(1.0e6f, 1.0e6f + 1.0f)` est vrai.
- Vérifie que `core::approximatelyEqual(1.0e6f, 1.0e6f + 1000.0f)` est faux.

### MathUtilsTest.SignesOpposes

*Mineur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_math_utils.cpp:55`

approximatelyEqual : signes opposés

**Étapes**

1. Comparer 2 et -2.

**Résultat attendu**

- Vérifie que `core::approximatelyEqual(2.0f, -2.0f)` est faux.

### MathUtilsTest.ToleranceExplicite

*Mineur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_math_utils.cpp:67`

approximatelyEqual : tolérance explicite

**Étapes**

1. Comparer 1 et 1,5 sans tolérance.
2. Recomparer avec une tolérance de 1.

**Résultat attendu**

- Vérifie que `core::approximatelyEqual(1.0f, 1.5f)` est faux.
- Vérifie que `core::approximatelyEqual(1.0f, 1.5f, 1.0f)` est vrai.

## test_rect.cpp

### RectTest.Bords

*Majeur · Unitaire · Rect* — `Source/Test/Unit/Core/Math/test_rect.cpp:20`

Les bords exposés découlent de la position et de la taille.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `rect.left()` vaut `10.0f`, à `TOLERANCE` près.
- Vérifie que `rect.top()` vaut `20.0f`, à `TOLERANCE` près.
- Vérifie que `rect.right()` vaut `40.0f`, à `TOLERANCE` près.
- Vérifie que `rect.bottom()` vaut `60.0f`, à `TOLERANCE` près.

### RectTest.ContientPointInterieur

*Majeur · Unitaire · Rect* — `Source/Test/Unit/Core/Math/test_rect.cpp:38`

Un point strictement intérieur est contenu.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `rect.contains(core::Vector2{5.0f, 5.0f})` est vrai.

### RectTest.ContientBords

*Majeur · Unitaire · Rect* — `Source/Test/Unit/Core/Math/test_rect.cpp:53`

Contenance inclusive haut/gauche, exclusive bas/droit.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `rect.contains(core::Vector2{0.0f, 0.0f})` est vrai.
- Vérifie que `rect.contains(core::Vector2{10.0f, 5.0f})` est faux.
- Vérifie que `rect.contains(core::Vector2{5.0f, 10.0f})` est faux.

### RectTest.NeContientPasExterieur

*Majeur · Unitaire · Rect* — `Source/Test/Unit/Core/Math/test_rect.cpp:70`

Un point extérieur n'est pas contenu.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `rect.contains(core::Vector2{-1.0f, 5.0f})` est faux.
- Vérifie que `rect.contains(core::Vector2{5.0f, 100.0f})` est faux.

### RectTest.IntersectionRecouvrement

*Majeur · Unitaire · Rect* — `Source/Test/Unit/Core/Math/test_rect.cpp:86`

Deux rectangles qui se recouvrent s'intersectent (relation symétrique).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `a.intersects(b)` est vrai.
- Vérifie que `b.intersects(a)` est vrai.

### RectTest.ContactBordSansIntersection

*Majeur · Unitaire · Rect* — `Source/Test/Unit/Core/Math/test_rect.cpp:103`

Un simple contact par un bord ne compte pas comme intersection.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `a.intersects(b)` est faux.

### RectTest.Disjonction

*Majeur · Unitaire · Rect* — `Source/Test/Unit/Core/Math/test_rect.cpp:119`

Deux rectangles disjoints ne s'intersectent pas.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `a.intersects(b)` est faux.

## test_vector2.cpp

### Vector2Test.AdditionSoustraction

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_vector2.cpp:19`

Vector2 : addition et soustraction

**Étapes**

1. Poser deux vecteurs.
2. Calculer a+b et a-b.

**Résultat attendu**

- Vérifie que `a + b` vaut `core::Vector2(4.0f, -2.0f)`.
- Vérifie que `a - b` vaut `core::Vector2(-2.0f, 6.0f)`.

### Vector2Test.EchelleScalaire

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_vector2.cpp:34`

Vector2 : échelle par un scalaire

**Étapes**

1. Poser un vecteur.
2. Multiplier/diviser par un scalaire, prendre l'opposé.

**Résultat attendu**

- Vérifie que `v * 2.0f` vaut `core::Vector2(4.0f, -6.0f)`.
- Vérifie que `2.0f * v` vaut `core::Vector2(4.0f, -6.0f)`.
- Vérifie que `v / 2.0f` vaut `core::Vector2(1.0f, -1.5f)`.
- Vérifie que `-v` vaut `core::Vector2(-2.0f, 3.0f)`.

### Vector2Test.OperateursComposes

*Mineur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_vector2.cpp:50`

Vector2 : opérateurs composés

**Étapes**

1. Appliquer +=, -=, *=, /= à un vecteur.

**Résultat attendu**

- Vérifie que `v` vaut `core::Vector2(3.0f, 4.0f)`.
- Vérifie que `v` vaut `core::Vector2(2.0f, 3.0f)`.
- Vérifie que `v` vaut `core::Vector2(4.0f, 6.0f)`.
- Vérifie que `v` vaut `core::Vector2(2.0f, 3.0f)`.

### Vector2Test.ProduitScalaire

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_vector2.cpp:70`

Vector2 : produit scalaire

**Étapes**

1. Calculer a·b.
2. Calculer le produit de deux vecteurs orthogonaux.

**Résultat attendu**

- Vérifie que `a.dot(b)` vaut `11.0f`, à `TOLERANCE` près.
- Vérifie que `core::Vector2(1.0f, 0.0f).dot(core::Vector2(0.0f, 1.0f))` vaut `0.0f`, à `TOLERANCE` près.

### Vector2Test.Longueur

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_vector2.cpp:86`

Vector2 : longueur

**Étapes**

1. Calculer `lengthSquared` et `length` de (3, 4).

**Résultat attendu**

- Vérifie que `v.lengthSquared()` vaut `25.0f`, à `TOLERANCE` près.
- Vérifie que `v.length()` vaut `5.0f`, à `TOLERANCE` près.

### Vector2Test.NormalisationNonNulle

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_vector2.cpp:100`

Vector2 : normalisation (non nul)

**Étapes**

1. Normaliser (3, 4).

**Résultat attendu**

- Vérifie que `normalized.length()` vaut `1.0f`, à `TOLERANCE` près.
- Vérifie que `normalized` vaut `core::Vector2(0.6f, 0.8f)`.

### Vector2Test.NormalisationVecteurNul

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_vector2.cpp:114`

Vector2 : normalisation (vecteur nul)

**Étapes**

1. Normaliser le vecteur nul.

**Résultat attendu**

- Vérifie que `core::Vector2{}.normalized()` vaut `core::Vector2(0.0f, 0.0f)`.

### Vector2Test.EgaliteApprochee

*Majeur · Unitaire · Mathématiques* — `Source/Test/Unit/Core/Math/test_vector2.cpp:126`

Vector2 : égalité approchée

**Étapes**

1. Comparer (0,1+0,2 ; 1) à (0,3 ; 1).
2. Comparer deux vecteurs distincts.

**Résultat attendu**

- Vérifie que `a` vaut `core::Vector2(0.3f, 1.0f)`.
- Vérifie que `core::Vector2(0.0f, 0.0f)` diffère de `core::Vector2(1.0f, 0.0f)`.
