# Core · Diagnostics

Tests unitaires — **22 cas** (19 majeurs, 3 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_assert.cpp`](#test-assertcpp) | 2 | - | - | 2 | - |
| [`test_file_log_sink.cpp`](#test-file-log-sinkcpp) | 5 | - | - | 2 | 3 |
| [`test_log_format.cpp`](#test-log-formatcpp) | 4 | - | - | 4 | - |
| [`test_log_level_parse.cpp`](#test-log-level-parsecpp) | 3 | - | - | 3 | - |
| [`test_logger.cpp`](#test-loggercpp) | 3 | - | - | 3 | - |
| [`test_scoped_log_level.cpp`](#test-scoped-log-levelcpp) | 3 | - | - | 3 | - |
| [`test_sinks.cpp`](#test-sinkscpp) | 2 | - | - | 2 | - |

## test_assert.cpp

### AssertTest.ConditionVraieNInvoquePasLeHandler

*Majeur · Unitaire · Assert* — `Source/Test/Unit/Core/Diagnostics/test_assert.cpp:17`

Une condition vraie n'invoque pas le gestionnaire d'assertion.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `invoked` est faux.

### AssertTest.ConditionFausseInvoqueLeHandler

*Majeur · Unitaire · Assert* — `Source/Test/Unit/Core/Diagnostics/test_assert.cpp:40`

Une condition fausse invoque le gestionnaire une fois, avec le message (Debug uniquement).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `count` vaut `1`.
- Vérifie que `capturedMessage` vaut `"echec attendu"`.

## test_file_log_sink.cpp

### FileLogSinkTest.EcritureEstPersisteeImmediatement

*Majeur · Unitaire · Diagnostics* — `Source/Test/Unit/Core/Diagnostics/test_file_log_sink.cpp:49`

FileLogSink : écriture persistée immédiatement (flush).

**Étapes**

1. Construire un `FileLogSink` sur un chemin dans un dossier temporaire.
2. Écrire un message.
3. Relire le fichier sans détruire le sink.

**Résultat attendu**

- Vérifie que `sink.isOpen()` est vrai.
- Vérifie que `content.find("message de test")` diffère de `std::string::npos`.

### FileLogSinkTest.MessagesSuccessifsUneLigneChacunOrdrePreserve

*Majeur · Unitaire · Diagnostics* — `Source/Test/Unit/Core/Diagnostics/test_file_log_sink.cpp:70`

FileLogSink : plusieurs messages, une ligne chacun, ordre préservé.

**Étapes**

1. Construire un `FileLogSink`.
2. Écrire trois messages successifs.
3. Relire le fichier.

**Résultat attendu**

- Vérifie que `std::getline(stream, line)` est vrai.
- Vérifie que `line` vaut `"premier"`.
- Vérifie que `std::getline(stream, line)` est vrai.
- Vérifie que `line` vaut `"deuxieme"`.
- Vérifie que `std::getline(stream, line)` est vrai.
- Vérifie que `line` vaut `"troisieme"`.

### FileLogSinkTest.CreeLesDossiersParentsManquants

*Mineur · Unitaire · Diagnostics* — `Source/Test/Unit/Core/Diagnostics/test_file_log_sink.cpp:98`

FileLogSink : crée les dossiers parents manquants.

**Étapes**

1. Construire un `FileLogSink` sur `dir/sous/dossier/session.log` (dossiers inexistants).
2. Écrire un message.

**Résultat attendu**

- Vérifie que `sink.isOpen()` est vrai.
- Vérifie que `std::filesystem::exists(path)` est vrai.
- Vérifie que `readFile(path).find("message")` diffère de `std::string::npos`.

### FileLogSinkTest.EcraseParDefautCompleteAvecAppend

*Mineur · Unitaire · Diagnostics* — `Source/Test/Unit/Core/Diagnostics/test_file_log_sink.cpp:119`

FileLogSink : écrase par défaut, complète avec `append = true`.

**Étapes**

1. Écrire un fichier avec un premier sink.
2. Rouvrir sur le même chemin sans `append`, puis avec `append = true`.

**Résultat attendu**

- Vérifie que `afterTruncate.find("premiere ouverture")` vaut `std::string::npos`.
- Vérifie que `afterTruncate.find("seconde ouverture")` diffère de `std::string::npos`.
- Vérifie que `afterAppend.find("seconde ouverture")` diffère de `std::string::npos`.
- Vérifie que `afterAppend.find("troisieme ouverture")` diffère de `std::string::npos`.

### FileLogSinkTest.CheminInvalideSinkFermeEtWriteSilencieux

*Mineur · Unitaire · Diagnostics* — `Source/Test/Unit/Core/Diagnostics/test_file_log_sink.cpp:155`

FileLogSink : chemin invalide, sink fermé, write() silencieux.

**Étapes**

1. Construire un `FileLogSink` dont un segment du chemin est en réalité un fichier existant (impossible à traverser comme dossier).
2. Appeler `write()`.

**Résultat attendu**

- Vérifie que `sink.isOpen()` est faux.
- `EXPECT_NO_THROW(sink.write(core::LogLevel::Error, "ne doit rien faire"))`

## test_log_format.cpp

### LogFormatTest.LigneContientTousLesChamps

*Majeur · Unitaire · Log Format* — `Source/Test/Unit/Core/Diagnostics/test_log_format.cpp:17`

La ligne formatée contient horodatage, niveau, catégorie, position source et message.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `line.find("12:34:56")` diffère de `std::string::npos`.
- Vérifie que `line.find("WARNING")` diffère de `std::string::npos`.
- Vérifie que `line.find("HMI")` diffère de `std::string::npos`.
- Vérifie que `line.find("Foo.cpp:42")` diffère de `std::string::npos`.
- Vérifie que `line.find("message test")` diffère de `std::string::npos`.

### LogFormatTest.CheminReduitAuNomDeFichier

*Majeur · Unitaire · Log Format* — `Source/Test/Unit/Core/Diagnostics/test_log_format.cpp:40`

Le chemin source est réduit à son nom de fichier dans la ligne.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `line.find("Foo.cpp:7")` diffère de `std::string::npos`.
- Vérifie que `line.find("JustAnotherRpgGame")` vaut `std::string::npos`.
- Vérifie que `line.find("Source")` vaut `std::string::npos`.

### LogFormatTest.FileNameIsoleLeNom

*Majeur · Unitaire · Log Format* — `Source/Test/Unit/Core/Diagnostics/test_log_format.cpp:60`

fileName isole le nom de fichier des chemins Windows et POSIX, ou renvoie l'entrée.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `core::fileName("a/b/c/File.cpp")` vaut `"File.cpp"`.
- Vérifie que `core::fileName("a\\b\\File.cpp")` vaut `"File.cpp"`.
- Vérifie que `core::fileName("File.cpp")` vaut `"File.cpp"`.

### LogFormatTest.HorodatageFormatHeure

*Majeur · Unitaire · Log Format* — `Source/Test/Unit/Core/Diagnostics/test_log_format.cpp:77`

L'horodatage courant respecte le format HH:MM:SS (longueur 8).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `timestamp.size()` vaut `8u`.
- Vérifie que `timestamp[2]` vaut `':'`.
- Vérifie que `timestamp[5]` vaut `':'`.

## test_log_level_parse.cpp

### LogLevelParseTest.NiveauxReconnus

*Majeur · Unitaire · Log Level Parse* — `Source/Test/Unit/Core/Diagnostics/test_log_level_parse.cpp:15`

Les noms de niveaux reconnus sont convertis (y compris l'alias « warn »).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `core::parseLogLevel("trace")` vaut `core::LogLevel::Trace`.
- Vérifie que `core::parseLogLevel("info")` vaut `core::LogLevel::Info`.
- Vérifie que `core::parseLogLevel("warning")` vaut `core::LogLevel::Warning`.
- Vérifie que `core::parseLogLevel("warn")` vaut `core::LogLevel::Warning`.
- Vérifie que `core::parseLogLevel("error")` vaut `core::LogLevel::Error`.

### LogLevelParseTest.InsensibleALaCasse

*Majeur · Unitaire · Log Level Parse* — `Source/Test/Unit/Core/Diagnostics/test_log_level_parse.cpp:33`

L'analyse est insensible à la casse.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `core::parseLogLevel("INFO")` vaut `core::LogLevel::Info`.
- Vérifie que `core::parseLogLevel("Warning")` vaut `core::LogLevel::Warning`.
- Vérifie que `core::parseLogLevel("Error")` vaut `core::LogLevel::Error`.

### LogLevelParseTest.ValeurInconnue

*Majeur · Unitaire · Log Level Parse* — `Source/Test/Unit/Core/Diagnostics/test_log_level_parse.cpp:49`

Une valeur inconnue ou vide n'est pas convertie.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `core::parseLogLevel("verbose").has_value()` est faux.
- Vérifie que `core::parseLogLevel("").has_value()` est faux.

## test_logger.cpp

### LoggerTest.FiltreParNiveauMinimal

*Majeur · Unitaire · Logger* — `Source/Test/Unit/Core/Diagnostics/test_logger.cpp:18`

Un message au-dessus du niveau minimal est diffusé ; en dessous, il est filtré.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `observed->entries().size()` vaut `1u`.
- Vérifie que `observed->entries()[0].level` vaut `core::LogLevel::Error`.
- Vérifie que `observed->entries()[0].message` vaut `"garde"`.

### LoggerTest.DiffuseAPlusieursSinks

*Majeur · Unitaire · Logger* — `Source/Test/Unit/Core/Diagnostics/test_logger.cpp:44`

Le même message atteint tous les sinks enregistrés.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `firstObserved->entries().size()` vaut `1u`.
- Vérifie que `secondObserved->entries().size()` vaut `1u`.

### LoggerTest.ClearSinksArreteLaDiffusion

*Majeur · Unitaire · Logger* — `Source/Test/Unit/Core/Diagnostics/test_logger.cpp:69`

clearSinks retire les destinations : plus rien n'est diffusé ensuite.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `observedAfterClear->entries().size()` vaut `1u`.
- Vérifie que `observedAfterClear->entries()[0].message` vaut `"personne"`.

## test_scoped_log_level.cpp

### ScopedLogLevelTest.ReleveUnNiveauPermissifPuisLeRestaure

*Majeur · Unitaire · ScopedLogLevel* — `Source/Test/Unit/Core/Diagnostics/test_scoped_log_level.cpp:17`

ScopedLogLevel relève un niveau permissif, puis le restaure.

**Étapes**

1. Logger au niveau Trace.
2. Ouvrir une portée `ScopedLogLevel(logger, Warning)`.
3. Fermer la portée.

**Résultat attendu**

- Vérifie que `logger.minimumLevel()` vaut `core::LogLevel::Warning`.
- Vérifie que `logger.isEnabled(core::LogLevel::Info)` est faux.
- Vérifie que `logger.minimumLevel()` vaut `core::LogLevel::Trace`.
- Vérifie que `logger.isEnabled(core::LogLevel::Trace)` est vrai.

### ScopedLogLevelTest.NAssouplitJamaisUnNiveauDejaPlusStrict

*Majeur · Unitaire · ScopedLogLevel* — `Source/Test/Unit/Core/Diagnostics/test_scoped_log_level.cpp:40`

ScopedLogLevel n'assouplit jamais un niveau déjà plus strict.

**Étapes**

1. Logger au niveau Error.
2. Ouvrir une portée `ScopedLogLevel(logger, Warning)`.

**Résultat attendu**

- Vérifie que `logger.minimumLevel()` vaut `core::LogLevel::Error`.
- Vérifie que `logger.minimumLevel()` vaut `core::LogLevel::Error`.

### ScopedLogLevelTest.ElevationsImbriqueesNeRestaurentQuALaDerniereSortie

*Majeur · Unitaire · Core Diagnostics* — `Source/Test/Unit/Core/Diagnostics/test_scoped_log_level.cpp:67`

Élévations imbriquées : restauration à la sortie de la dernière seulement.

**Étapes**

1. Journaliseur à `Info`.
2. Ouvrir une première élévation à `Warning`, puis une seconde.
3. Fermer la première, vérifier le niveau ; fermer la seconde, vérifier à nouveau.

**Résultat attendu**

- Vérifie que `logger.minimumLevel()` vaut `core::LogLevel::Warning`.
- Vérifie que `logger.minimumLevel()` vaut `core::LogLevel::Warning`.
- Vérifie que `logger.minimumLevel()` vaut `core::LogLevel::Warning`.
- Vérifie que `logger.minimumLevel()` vaut `core::LogLevel::Info`.

## test_sinks.cpp

### MemoryLogSinkTest.ConserveNiveauEtTexteDansLOrdre

*Majeur · Unitaire · Memory Log Sink* — `Source/Test/Unit/Core/Diagnostics/test_sinks.cpp:15`

Le sink mémoire conserve fidèlement niveau et texte, dans l'ordre.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `sink.entries().size()` vaut `2u`.
- Vérifie que `sink.entries()[0].level` vaut `core::LogLevel::Info`.
- Vérifie que `sink.entries()[0].message` vaut `"premier"`.
- Vérifie que `sink.entries()[1].level` vaut `core::LogLevel::Error`.
- Vérifie que `sink.entries()[1].message` vaut `"second"`.

### MemoryLogSinkTest.ClearVideLesMessages

*Majeur · Unitaire · Memory Log Sink* — `Source/Test/Unit/Core/Diagnostics/test_sinks.cpp:37`

clear vide les messages mémorisés.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `sink.entries().empty()` est vrai.
