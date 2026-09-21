# Core

Tests unitaires — **1 cas** (1 majeur). [Retour à la synthèse](README.md).

## test_core.cpp

### EngineTest.VersionNonVide

*Majeur · Unitaire · Engine* — `Source/Test/Unit/Core/test_core.cpp:15`

Vérifie que la version du moteur n'est pas vide.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `engine.version().empty()` est faux.
