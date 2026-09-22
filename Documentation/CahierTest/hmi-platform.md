# HMI · Platform

Tests unitaires — **5 cas** (2 critiques, 3 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_crash_dump.cpp`](#test-crash-dumpcpp) | 5 | - | 2 | 3 | - |

## test_crash_dump.cpp

### CrashDumpFileName.PorteApplicationVersionEtHorodatage

*Majeur · Unitaire · Crash Dump* — `Source/Test/Unit/HMI/Platform/test_crash_dump.cpp:122`

Le nom d'un minidump porte l'application, la version et l'heure du plantage.

**Étapes**

1. Composer le nom pour JustAnotherRpgGame 0.0.4 au 15/09/2026 21:04:07.

**Résultat attendu**

- Vérifie que `hmi::crashDumpFileName("JustAnotherRpgGame", "0.0.4", fixedTime())` vaut `"JustAnotherRpgGame_0.0.4_20260915_210407.dmp"`.

### CrashDumpFileName.RemplaceLesCaracteresHorsNomDeFichier

*Majeur · Unitaire · Crash Dump* — `Source/Test/Unit/HMI/Platform/test_crash_dump.cpp:137`

Les caractères hors nom de fichier sont remplacés dans le nom du minidump.

**Étapes**

1. Composer le nom pour « Level Editor » en version « 0.1.0+dev/x ».

**Résultat attendu**

- Vérifie que `hmi::crashDumpFileName("Level Editor", "0.1.0+dev/x", fixedTime())` vaut `"Level_Editor_0.1.0_dev_x_20260915_210407.dmp"`.

### CrashDumpTest.EcritUnMinidumpSansExceptionEtCreeLeDossier

*Critique · Unitaire · Crash Dump* — `Source/Test/Unit/HMI/Platform/test_crash_dump.cpp:151`

Un minidump de l'état courant s'écrit, dossier parent créé au besoin.

**Étapes**

1. Écrire un dump sans contexte d'exception dans un sous-dossier inexistant.
2. Lire les quatre premiers octets.

**Résultat attendu**

- Vérifie que `hmi::writeMiniDump(path, nullptr)` est vrai.
- Vérifie que `std::filesystem::exists(path)` est vrai.
- Vérifie que `signature(path)` vaut `"MDMP"`.
- Vérifie que `hasExceptionStream(path)` est faux.

### CrashDumpTest.EcritUnMinidumpAvecLeContexteDUneException

*Critique · Unitaire · Crash Dump* — `Source/Test/Unit/HMI/Platform/test_crash_dump.cpp:170`

Un minidump s'écrit avec le contexte d'une exception structurée.

**Étapes**

1. Lever une exception structurée, l'attraper dans un filtre SEH qui écrit le dump.
2. Lire la signature et la taille.

**Résultat attendu**

- Vérifie que `dumpFromStructuredException(&path)` est vrai.
- Vérifie que `signature(path)` vaut `"MDMP"`.
- Vérifie que `std::filesystem::file_size(path)` est strictement supérieur à `1024U`.
- Vérifie que `hasExceptionStream(path)` est vrai.

### CrashDumpTest.EchoueSansLeverSurUnCheminImpossible

*Majeur · Unitaire · Crash Dump* — `Source/Test/Unit/HMI/Platform/test_crash_dump.cpp:189`

Un chemin de minidump impossible échoue sans exception ni plantage.

**Étapes**

1. Écrire un dump sous un nom interdit par Windows (« a?b.dmp »).

**Résultat attendu**

- Vérifie que `hmi::writeMiniDump(directory_ / "a?b.dmp", nullptr)` est faux.
