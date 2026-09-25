# HMI · Game

Tests unitaires — **19 cas** (7 critiques, 9 majeurs, 3 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_combat_cues.cpp`](#test-combat-cuescpp) | 3 | - | 2 | 1 | - |
| [`test_debug_commands.cpp`](#test-debug-commandscpp) | 4 | - | 1 | 2 | 1 |
| [`test_figure_resolver.cpp`](#test-figure-resolvercpp) | 2 | - | 1 | 1 | - |
| [`test_launch_options.cpp`](#test-launch-optionscpp) | 6 | - | 1 | 3 | 2 |
| [`test_level_scan.cpp`](#test-level-scancpp) | 4 | - | 2 | 2 | - |

## Exigences vérifiées par cette page

Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la [matrice de traçabilité](couverture-exigences.md) les rassemble toutes.

| Exigence | Cas |
|---|---|
| `EX-NFR-040` | [`LevelScan.DossierAbsentNeContientRien`](#levelscandossierabsentnecontientrien) |

## test_combat_cues.cpp

### CombatCuesTest.UneMarcheSeRejoueCaseParCase

*Critique · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Game/test_combat_cues.cpp:24`

Une marche se rejoue a deux cases par seconde, puis revient au repos.

**Étapes**

1. Poser le heros en (0, 0) ; pousser une marche par (1, 0), (2, 0), (2, 1).
2. Avancer de 0,25 s, puis de 0,5 s, puis jusqu'au bout.

**Résultat attendu**

- Vérifie que `file.busy()` est faux.
- Vérifie que `file.busy()` est vrai.
- Vérifie que `heros` diffère de `nullptr`.
- Vérifie que `heros->clip` vaut `hmi::figure_clips::WALK`.
- Vérifie que `heros->point.x` vaut `1.0F`, à `1e-4F` près.
- Vérifie que `heros->point.y` vaut `0.5F`, à `1e-4F` près.
- Vérifie que `heros->facing` vaut `hmi::FigureFacing::SouthEast`.
- Vérifie que `heros->point.x` vaut `2.0F`, à `1e-4F` près.
- Vérifie que `heros->point.y` vaut `0.5F`, à `1e-4F` près.
- Vérifie que `file.busy()` est faux.
- Vérifie que `heros->clip` vaut `hmi::figure_clips::IDLE`.
- Vérifie que `heros->point.x` vaut `2.5F`, à `1e-4F` près.
- Vérifie que `heros->point.y` vaut `1.5F`, à `1e-4F` près.
- Vérifie que `heros->facing` vaut `hmi::FigureFacing::SouthWest`.

### CombatCuesTest.LeCoupPorteAMiGesteEtUnMortResteATerre

*Critique · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Game/test_combat_cues.cpp:66`

Attaque, touche et mort s'enchainent a l'instant de l'impact.

**Étapes**

1. Poser le heros en (0, 0) et le rat en (1, 0) ; pousser une attaque du heros sur le rat, un touche du rat, une mort du rat.
2. Avancer d'un quart de geste, puis jusqu'a la moitie et au-dela, puis d'une seconde de plus.

**Résultat attendu**

- Vérifie que `heros` diffère de `nullptr`.
- Vérifie que `rat` diffère de `nullptr`.
- Vérifie que `heros->clip` vaut `hmi::figure_clips::ATTACK`.
- Vérifie que `heros->facing` vaut `hmi::FigureFacing::SouthEast`.
- Vérifie que `rat->clip` vaut `hmi::figure_clips::IDLE`.
- Vérifie que `rat->clip` vaut `hmi::figure_clips::DEATH`.
- Vérifie que `rat->dead` est vrai.
- Vérifie que `file.busy()` est faux.
- Vérifie que `heros->clip` vaut `hmi::figure_clips::IDLE`.
- Vérifie que `rat->clip` vaut `hmi::figure_clips::DEATH`.
- Vérifie que `rat->clipSeconds` est strictement supérieur à `CombatCueTrack::ACTION_SECONDS`.
- Vérifie que `rat->clip` vaut `hmi::figure_clips::DEATH`.
- Vérifie que `file.busy()` est faux.

### CombatCuesTest.LInconnuEstIgnoreEtToutPeutFinirDUnCoup

*Majeur · Unitaire · Combat sur la carte* — `Source/Test/Unit/HMI/Game/test_combat_cues.cpp:118`

La file ignore l'inconnu et sait tout finir d'un coup.

**Étapes**

1. Pousser une marche pour un combattant jamais pose, et une marche vide pour le heros.
2. Pousser une vraie marche et une attaque, puis tout finir.

**Résultat attendu**

- Vérifie que `file.busy()` est faux.
- Vérifie que `file.pending()` vaut `0U`.
- Vérifie que `file.pending()` vaut `2U`.
- Vérifie que `file.busy()` est faux.
- Vérifie que `heros` diffère de `nullptr`.
- Vérifie que `heros->clip` vaut `hmi::figure_clips::IDLE`.
- Vérifie que `heros->point.y` vaut `2.5F`, à `1e-4F` près.
- Vérifie que `file.motionOf(HEROS)` vaut `nullptr`.

## test_debug_commands.cpp

### DebugCommands.LeCatalogueSuitLesSourcesDuJeu

*Critique · Unitaire · Console de debug* — `Source/Test/Unit/HMI/Game/test_debug_commands.cpp:25`

Le catalogue et les sources du jeu lisent les memes options.

**Étapes**

1. Relever les noms d'options que Main.cpp, Bootstrap.cpp et WorldMap.qml lisent.
2. Comparer a ceux du catalogue.

**Résultat attendu**

- Vérifie que `flux.is_open()` est vrai.
- Vérifie que `sources.find(cite)` diffère de `std::string::npos`.
- Vérifie que `fin` diffère de `std::string::npos`.
- Vérifie que `hmi::findDebugOption(nom)` diffère de `nullptr`.

### DebugCommands.SepareNomEtValeurAuPremierEgal

*Majeur · Unitaire · Console de debug* — `Source/Test/Unit/HMI/Game/test_debug_commands.cpp:70`

Un mot se separe au premier = ; une option sans valeur garde son nom entier.

**Étapes**

1. Separer `--map=capital/arenarea@martpart`, `--flags=a=1,b`, `--crash-test`, `aide`.

**Résultat attendu**

- Vérifie que `hmi::splitDebugArgument("--map=capital/arenarea@martpart")` vaut `(hmi::DebugArgument{.name = "--map=", .value = "capital/arenarea@martpart"})`.
- Vérifie que `hmi::splitDebugArgument("--flags=a=1,b")` vaut `(hmi::DebugArgument{.name = "--flags=", .value = "a=1,b"})`.
- Vérifie que `hmi::splitDebugArgument("--crash-test")` vaut `(hmi::DebugArgument{.name = "--crash-test", .value = ""})`.
- Vérifie que `hmi::splitDebugArgument("aide")` vaut `(hmi::DebugArgument{.name = "aide", .value = ""})`.
- Vérifie que `hmi::findDebugOption("--map=")->takesValue()` est vrai.
- Vérifie que `hmi::findDebugOption("--crash-test")->takesValue()` est faux.
- Vérifie que `hmi::findDebugOption("--inconnue=")` vaut `nullptr`.

### DebugCommands.DecoupeLaLigneEnRespectantLesGuillemets

*Majeur · Unitaire · Console de debug* — `Source/Test/Unit/HMI/Game/test_debug_commands.cpp:94`

Les guillemets gardent un chemin avec espaces en un seul mot.

**Étapes**

1. Decouper une ligne a blancs multiples et un chemin entre guillemets.

**Résultat attendu**

- Vérifie que `hmi::splitCommandLine(" --map=donjon --screenshot=\"C:\\Mes captures\\a.png\" " "--at=1,2\t")` vaut `(std::vector<std::string>{"--map=donjon", "--screenshot=C:\\Mes captures\\a.png", "--at=1,2"})`.
- Vérifie que `hmi::splitCommandLine(" ").empty()` est vrai.

### DebugCommands.LitLaTailleDeFenetre

*Mineur · Unitaire · Console de debug* — `Source/Test/Unit/HMI/Game/test_debug_commands.cpp:111`

La taille de fenetre se lit en LxH, et rien d'autre.

**Étapes**

1. Lire `1920x1080`, puis `1920`, `0x10`, `axb`.

**Résultat attendu**

- Vérifie que `hmi::parseWindowSize("1920x1080").has_value()` est vrai.
- Vérifie que `*hmi::parseWindowSize("1920x1080")` vaut `(std::pair{1920, 1080})`.
- Vérifie que `hmi::parseWindowSize("1920").has_value()` est faux.
- Vérifie que `hmi::parseWindowSize("0x10").has_value()` est faux.
- Vérifie que `hmi::parseWindowSize("axb").has_value()` est faux.

## test_figure_resolver.cpp

### FigureResolverTest.LaRegleDeRepliEnTroisTemps

*Critique · Unitaire · Mannequins* — `Source/Test/Unit/HMI/Game/test_figure_resolver.cpp:44`

Le resolveur applique la regle de repli en trois temps.

**Étapes**

1. Resoudre le heros (installe, oriente).
2. Resoudre un loup absent, silhouette quadrupede.
3. Resoudre un garde absent, sans silhouette.
4. Resoudre un oiseau absent, silhouette volante (pas de mannequin volant).
5. Retirer l'humanoide et resoudre le garde a nouveau, resolveur vide.

**Résultat attendu**

- Vérifie que `heros.directory` vaut `"Common/Characters/Heroes/brawler"`.
- Vérifie que `heros.oriented` est vrai.
- Vérifie que `heros.placeholder` est faux.
- Vérifie que `loup.directory` vaut `hmi::placeholderFigureDirectory("quadruped")`.
- Vérifie que `loup.oriented` est faux.
- Vérifie que `loup.placeholder` est vrai.
- Vérifie que `garde.directory` vaut `hmi::placeholderFigureDirectory(hmi::DEFAULT_SILHOUETTE)`.
- Vérifie que `garde.placeholder` est vrai.
- Vérifie que `oiseau.directory` vaut `hmi::placeholderFigureDirectory(hmi::DEFAULT_SILHOUETTE)`.
- Vérifie que `sansRien.directory` vaut `table.figureDirectory("guard")`.
- Vérifie que `sansRien.placeholder` est faux.
- Vérifie que `sansRien.oriented` est faux.

### FigureResolverTest.LaReponseSeRetientJusquAClear

*Majeur · Unitaire · Mannequins* — `Source/Test/Unit/HMI/Game/test_figure_resolver.cpp:91`

Le resolveur retient ce qu'il a trouve jusqu'a ce qu'on l'oublie.

**Étapes**

1. Resoudre un garde absent (humanoide).
2. Installer sa bande de repos sur le disque, resoudre a nouveau.
3. Oublier, resoudre a nouveau.

**Résultat attendu**

- Vérifie que `resolveur.resolve("Npc/guard", {}, table).placeholder` est vrai.
- Vérifie que `resolveur.resolve("Npc/guard", {}, table).placeholder` est vrai.
- Vérifie que `propre.placeholder` est faux.
- Vérifie que `propre.directory` vaut `"Npc/guard"`.

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

## test_level_scan.cpp

### LevelScan.IdentifiantEstLeCheminSousLevels

*Critique · Unitaire · Lanceur de cartes* — `Source/Test/Unit/HMI/Game/test_level_scan.cpp:62`

L'identifiant d'une carte est son chemin sous Levels/, sans .json.

**Étapes**

1. Poser trois cartes, dont deux dans des sous-dossiers.
2. Lister le dossier.

**Résultat attendu**

- Vérifie que `identifiants(cartes)` vaut `(std::vector<std::string>{"bourg/place", "central-empire/capital/arenarea", "donjon"})`.
- Vérifie que `cartes.size()` vaut `3U`.
- Vérifie que `cartes[2].file` vaut `levels.chemin("donjon.json")`.
- Vérifie que `cartes[2].directory` vaut `levels.chemin()`.

### LevelScan.EcarteLesNotesDeLEditeurEtLesAutresFichiers

*Majeur · Unitaire · Lanceur de cartes* — `Source/Test/Unit/HMI/Game/test_level_scan.cpp:89`

Les notes de l'editeur et les fichiers etrangers ne sont pas des cartes.

**Étapes**

1. Poser une carte, ses notes `.editor.json`, un README et un PNG.
2. Lister.

**Résultat attendu**

- Vérifie que `identifiants(hmi::scanLevelDirectories({levels.chemin()}))` vaut `(std::vector<std::string>{"cave"})`.

### LevelScan.LePremierDossierLEmporteSurUnDoublon

*Critique · Unitaire · Lanceur de cartes* — `Source/Test/Unit/HMI/Game/test_level_scan.cpp:110`

Le premier dossier l'emporte sur une carte en double.

**Étapes**

1. Poser `donjon.json` dans un dossier de brouillons et dans celui du binaire, et une carte propre a chacun.
2. Lister les deux, brouillons d'abord.

**Résultat attendu**

- Vérifie que `identifiants(cartes)` vaut `(std::vector<std::string>{"cave", "donjon", "essai"})`.
- Vérifie que `cartes[1].directory` vaut `brouillons.chemin()`.
- Vérifie que `cartes[0].directory` vaut `binaire.chemin()`.

### LevelScan.DossierAbsentNeContientRien

*Majeur · Unitaire · Lanceur de cartes* — `Source/Test/Unit/HMI/Game/test_level_scan.cpp:137`

Exigences : `EX-NFR-040`

Un dossier absent ne contient rien et ne fait pas echouer le parcours.

**Étapes**

1. Lister un dossier inexistant puis un dossier d'une carte.

**Résultat attendu**

- Vérifie que `identifiants(hmi::scanLevelDirectories( {levels.chemin("nulle-part"), levels.chemin()}))` vaut `(std::vector<std::string>{"cave"})`.
- Vérifie que `hmi::scanLevelDirectories({}).empty()` est vrai.
