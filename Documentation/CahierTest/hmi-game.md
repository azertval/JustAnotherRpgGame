# HMI · Game

Tests unitaires — **6 cas** (1 critique, 3 majeurs, 2 mineurs). [Retour à la synthèse](README.md).

## test_launch_options.cpp

### LaunchOptions.CarteSeuleNePosePasDOptionVide

*Majeur · Unitaire · Essai complet* — `Source/Test/Unit/HMI/Game/test_launch_options.cpp:21`

Une carte seule ne pose aucune option vide.

**Étapes**

1. Construire la ligne de commande d'un essai qui ne nomme qu'une carte.

**Résultat attendu**

- Vérifie que `hmi::gameLaunchArguments(options)` vaut `(std::vector<std::string>{"--map=capital/martpart"})`.

### LaunchOptions.PointDArriveeSurLOptionDeCarte

*Mineur · Unitaire · Essai complet* — `Source/Test/Unit/HMI/Game/test_launch_options.cpp:36`

Le point d'arrivee s'ecrit sur l'option de carte.

**Étapes**

1. Construire la ligne de commande d'une carte et d'un point d'arrivee.

**Résultat attendu**

- Vérifie que `hmi::gameLaunchArguments(options).front()` vaut `"--map=capital/arenarea@martpart"`.

### LaunchOptions.AllerRetourCompletParLAnalyse

*Critique · Unitaire · Essai complet* — `Source/Test/Unit/HMI/Game/test_launch_options.cpp:50`

Ce que l'editeur ecrit, le jeu le relit a l'identique.

**Étapes**

1. Construire la ligne de commande d'un essai complet (carte, case, drapeaux, dossiers).
2. Relire chaque option par les fonctions d'analyse du jeu.

**Résultat attendu**

- Vérifie que `arguments.size()` vaut `4U`.
- Vérifie que `arguments[1]` vaut `"--at=12,39"`.
- Vérifie que `arguments[2]` vaut `"--flags=quete-du-heraut,porte-est-ouverte"`.
- Vérifie que `relue.has_value()` est vrai.
- Vérifie que `relue->column` vaut `12`.
- Vérifie que `relue->row` vaut `39`.
- Vérifie que `hmi::parseWorldFlags("quete-du-heraut,porte-est-ouverte")` vaut `options.flags`.
- Vérifie que `hmi::parseLevelDirectories(arguments[3].substr(std::string_view{"--levels="}.size()))` vaut `options.levelDirectories`.

### LaunchOptions.LesDossiersSeSeparentAuPointVirgule

*Majeur · Unitaire · Essai complet* — `Source/Test/Unit/HMI/Game/test_launch_options.cpp:81`

Les dossiers de cartes se separent au point-virgule.

**Étapes**

1. Analyser `--levels=` sur deux chemins Windows portant chacun un deux-points.

**Résultat attendu**

- Vérifie que `dossiers.size()` vaut `2U`.
- Vérifie que `dossiers[0]` vaut `std::filesystem::path{"C:/tmp/essai"}`.
- Vérifie que `dossiers[1]` vaut `std::filesystem::path{"D:/depot/Levels"}`.

### LaunchOptions.CaseIllisibleRefusee

*Majeur · Unitaire · Essai complet* — `Source/Test/Unit/HMI/Game/test_launch_options.cpp:98`

Une case illisible est refusee, jamais ramenee a zero.

**Étapes**

1. Analyser des valeurs de `--at=` incompletes, non numeriques ou negatives.

**Résultat attendu**

- Vérifie que `hmi::parseStartCell("").has_value()` est faux.
- Vérifie que `hmi::parseStartCell("12").has_value()` est faux.
- Vérifie que `hmi::parseStartCell("12,").has_value()` est faux.
- Vérifie que `hmi::parseStartCell("12,39,4").has_value()` est faux.
- Vérifie que `hmi::parseStartCell("douze,39").has_value()` est faux.
- Vérifie que `hmi::parseStartCell("12,-3").has_value()` est faux.
- Vérifie que `hmi::parseStartCell("12,39x").has_value()` est faux.

### LaunchOptions.DrapeauxSansDoublonNiVide

*Mineur · Unitaire · Essai complet* — `Source/Test/Unit/HMI/Game/test_launch_options.cpp:117`

Les drapeaux se lisent sans doublon ni valeur vide.

**Étapes**

1. Analyser `--flags=` avec un separateur en trop et un drapeau repete.

**Résultat attendu**

- Vérifie que `hmi::parseWorldFlags("a,,b,a")` vaut `(std::vector<std::string>{"a", "b"})`.
- Vérifie que `hmi::parseWorldFlags("").empty()` est vrai.
