# JustAnotherRpgGame

[![CI](https://github.com/azertval/JustAnotherRpgGame/actions/workflows/ci.yml/badge.svg)](https://github.com/azertval/JustAnotherRpgGame/actions/workflows/ci.yml)
[![Documentation](https://github.com/azertval/JustAnotherRpgGame/actions/workflows/docs.yml/badge.svg)](https://github.com/azertval/JustAnotherRpgGame/actions/workflows/docs.yml)
[![Release](https://github.com/azertval/JustAnotherRpgGame/actions/workflows/release.yml/badge.svg)](https://github.com/azertval/JustAnotherRpgGame/releases/latest)
![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C)
![Qt QRhi](https://img.shields.io/badge/Qt%20QRhi-Direct3D%2011-8A2BE2)

RPG 2D **en vue de dessus** développé **from scratch** en **C++20** (Windows), sans moteur
tiers : exploration en temps réel, et rencontres en **combat tactique au tour par tour** régi par
un système **d20** maison. Le rendu passe par **QRhi**, la couche d'accès au GPU
de Qt, qui retient **Direct3D 11** par défaut sous Windows.

> **Fan game non commercial.** Ce jeu gratuit s'inspire des univers de *Dungeons & Dragons* et de
> *Tanares*, sans affiliation ni approbation de leurs ayants droit (Wizards of the Coast, Dragori
> Games). Voir [Licence](#licence).

- 📖 **Documentation en ligne** : <https://azertval.github.io/JustAnotherRpgGame/>
- 📊 **Qualité** (couverture, performances) : <https://azertval.github.io/JustAnotherRpgGame/qualite/>
- 🗺️ **Planification** (versions, lots, avancement) : <https://azertval.github.io/JustAnotherRpgGame/planning/> — source : [`Planning/`](Planning/README.md)
- ⬇️ **Télécharger la dernière version** : <https://github.com/azertval/JustAnotherRpgGame/releases/latest>
  (préversion roulante du dernier `main` : <https://github.com/azertval/JustAnotherRpgGame/releases/tag/debug-latest>)

## Description

JustAnotherRpgGame est un RPG en vue de dessus, dessiné en isométrique, bâti sur un moteur 2D maison.
Ses partis pris :

- **Séparation stricte** entre la logique (`Core`) et la présentation (`HMI`) : `Core` est
  indépendant du GPU et de la fenêtre, donc **testable sans GPU**.
- **Règles chiffrées en données** : espèces, classes, historiques, créatures, équipement et
  dialogues se définissent en JSON, jamais en dur dans le C++ — l'équilibrage ne demande pas de
  recompiler.
- **Deux exécutables, deux technologies d'interface** : le jeu en **Qt Quick** (`JustAnotherRpgGame`),
  l'éditeur de cartes en **Qt Widgets** (`LevelEditor`). Ils partagent le moteur, le rendu et le
  modèle de carte, jamais leurs écrans.
- **Hasard déterministe** (graine explicite) : un combat se rejoue à l'identique.

Le *quoi* et le *pourquoi* sont décrits dans les
[spécifications](https://azertval.github.io/JustAnotherRpgGame/) ; le *comment* dans le
**Guide du développeur** et la référence de code Doxygen.

## Fonctionnalités (état actuel)

> Le jeu quitte le pixel art pour la **2D HD** et repart d'une démo basique (`0.0.1`) : la
> [planification](Planning/README.md) dit ce qui reste. L'ancienne
> [feuille de route](Planning/vision/archives/feuille-de-route-jeu.md) est figée.

Le **jeu** :

- **Exploration** en temps réel du Colisée et de deux quartiers de la Capitale (Martpart,
  Arenarea) : déplacement au clavier, PNJ, portails d'une carte à l'autre, points d'arrivée nommés.
- **Combat tactique au tour par tour** au Colisée : initiative, économie d'actions, attaques et
  dégâts, portée et ligne de vue, zones d'effet, IA ennemie décrite par profils.
- **Écrans du RPG** : fiche de personnage, compétences, inventaire, dialogues, journal, compagnie,
  marchand, carte du monde à trois niveaux (monde, région, ville) et plan de ville par quartier.
- **Menu, options** (plein écran, volume, langue), **pause** et **crédits**, en français et en
  anglais, navigables au clavier et à la souris ; l'arène et la carte du monde se pilotent aussi à
  la manette (XInput).
- **Diagnostics** : journal de session sur disque, minidump en cas de plantage.

L'**éditeur de cartes** :

- Peinture des **trois couches** d'une carte (sol, décor, collision), rectangle, sélection,
  copier/coller, annuler/refaire.
- **Entités** : PNJ, coffres, panneaux, portails, points d'arrivée, rencontres ; propriétés
  éditées dans un panneau, références validées contre les catalogues.
- **Graphe du monde** des cartes et de leurs portails, avertissements de terrain tactique.
- **Essai immédiat** : la carte en cours se joue avec le moteur du jeu, sans l'enregistrer.

Toute la simulation vit dans `Core` et est **couverte par des tests** (unitaires, intégration,
système) — voir le **Cahier de test**.

## Organisation du dépôt

| Dossier | Rôle |
|---------|------|
| `Documentation/` | Les pages du site, en **Markdown nu** : `Guide/` (guide du développeur, manuel utilisateur en tête), `Specification/` (exigences et conventions), `CahierTest/` (engendré depuis les tests), `SourceBook/` (corpus d'entrée, non versionné) ; `outils/` les rend et les contrôle. Doxygen ne fait plus que la référence du code, annexe du guide. |
| `Site/` | La **charte du site publié** : palette, fontes et barre d'en-tête, partagées par la Doxygen, la planification et la page qualité. Aucune couleur ne s'écrit ailleurs — voir [`Site/README.md`](Site/README.md). |
| `Source/` | Code source, réparti par fonction. |
| `scripts/` | Build, contrôles de CI, ateliers d'extraction et de découpe d'assets. |
| `.github/workflows/` | Intégration continue (voir plus bas). |

### Découpage de `Source/`

| Sous-dossier | Contenu |
|--------------|---------|
| `Core/` | Logique et moteur : règles d20, combat, monde et exploration, modèle de carte, ECS, mathématiques, diagnostics — **sans dépendance au GPU ni à la fenêtre**. |
| `HMI/` | Présentation partagée : rendu 2D sur **QRhi**, entrées, localisation, vues-modèles du jeu (`Runtime/`), et l'éditeur de cartes (`Editor/`, `Interface/`). Dépend de `Core`, jamais l'inverse. |
| `Ui/` | Les formulaires QML du jeu (module `Jadg.Ui`), ouvrables dans Qt Design Studio. |
| `App/` | Les points d'entrée du jeu (et son câblage QML) et de l'éditeur. |
| `Elements/` | Données et assets : cartes, catalogues RPG, monde, planches de lieux, figurines, interface, polices, traductions. |
| `Test/` | Tests **unitaires** (`Unit/`), **d'intégration** (`Integration/`) et **système** (`Systeme/`) — GoogleTest ; tests QML. |
| `Benchmark/`, `Fuzz/` | Mesures de performance et harnais libFuzzer des lecteurs de données. |

## Build

Le projet se construit **exclusivement via CMake**. Visual Studio est utilisé comme IDE
grâce à son intégration CMake native (aucun `.vcxproj`/`.sln` versionné : ils sont
générés dans `build/`).

### Prérequis
- Visual Studio 2022+ avec la charge de travail **« Développement Desktop en C++ »**
  (inclut CMake, Ninja et le compilateur MSVC).
- **Qt6** (`Widgets`, `Gui`, `Quick`, `Multimedia`), version **6.11.2 ou
  supérieure** (celle validée par la CI — un écart produit un avertissement à la configuration,
  pas un échec), détecté automatiquement (`CMAKE_PREFIX_PATH`, cf.
  `Source/HMI/CMakeLists.txt`) s'il est installé à l'emplacement conventionnel de
  l'[installateur officiel](https://www.qt.io/download-qt-installer) ou via
  [`aqtinstall`](https://github.com/miurahr/aqtinstall)
  (`-m qtmultimedia qtshadertools qtcanvaspainter`). Installer Qt ≥ 6.11 avec
  `aqtinstall` demande une version de l'outil plus récente que celle publiée sur PyPI — voir
  [`External/README.md`](External/README.md).
  Sans Qt, les cibles `JustAnotherRpgGame` et `LevelEditor` sont **ignorées** (avertissement
  explicite) : seuls les tests se construisent.

### Depuis Visual Studio (recommandé)
1. `Fichier > Ouvrir > Dossier…` puis sélectionner la racine du dépôt.
2. VS détecte `CMakeLists.txt` et `CMakePresets.json`.
3. Choisir le preset `vs` (ou `ninja`) dans la barre d'outils, puis générer.

### En ligne de commande
```sh
cmake --preset vs        # configure (ou : ninja)
cmake --build --preset vs
ctest --preset vs        # lance les tests
```

Presets **Release** (`vs-release`, `ninja-release`) : mêmes commandes de build/test avec
`--preset vs-release` ou `--preset ninja-release` (configurer avec `ninja-release` pour ce dernier).
Vérifiés en CI (LOT-58) : certaines casses (variable lue uniquement par une assertion, `NDEBUG`)
n'apparaissent qu'en Release.

> Reproductible sur plusieurs postes : tout est versionné sauf `build/` (local).
> GoogleTest est récupéré automatiquement par CMake (FetchContent).

## Process d'implémentation

Le travail avance par **lots** (un incrément livrable par lot), décrits dans
`Planning/versions/<version>/lots/` (une fiche par lot, en-tête TOML puis récit).

- **Branches** : `main` est **protégée** (aucun push direct). Une **branche par lot**
  (`lot/LOT-XX-nom`) ; correctifs isolés en `fix/…`, documentation seule en `docs/…`.
- **Pull Requests obligatoires** vers `main`, avec **CI verte** requise pour merger.
  `main` reste toujours compilable et testée.
- **Commits** : [Conventional Commits](https://www.conventionalcommits.org/) en français
  (`feat`, `fix`, `docs`, `refactor`, `test`, `build`, `ci`, `chore`).
- **Conventions de code** : nommage, RAII, documentation Doxygen (`.h` **et** `.cpp`),
  gestion d'erreurs — voir
  [`Documentation/Specification/conventions.md`](Documentation/Specification/conventions.md).
  Le code compile **sans avertissement** (`/W4 /WX`).
- **Tests** : toute logique de `Core` est couverte par des tests (unitaires et, au besoin,
  d'intégration). `CHANGELOG.md` est tenu à jour.
- **Traçabilité** : les exigences `EX-…` (spécifications) sont des identifiants stables,
  vérifiés en CI (`scripts/lint_exigences.py`).

Détails dans [`CONTRIBUTING.md`](CONTRIBUTING.md).

### Vérifications locales

Les mêmes contrôles qu'en intégration continue, tous lançables **depuis la racine du dépôt** :

```sh
python scripts/lint_exigences.py           # identifiants EX-… : ni doublon, ni orphelin
python scripts/lint_exigences.py --next    # prochain numéro libre, par catégorie
python scripts/generate_cahier_test.py --check   # cahier de test à jour
python scripts/build_docs.py               # référence du code, Doxygen (WARN_AS_ERROR)
python Documentation/outils/build_docs_site.py --out build/site   # les pages du site
```

> `build_docs.py` existe parce que Doxygen résout les chemins de son fichier de configuration
> relativement au **répertoire courant**, et non à l'emplacement du `Doxyfile` : lancer
> `doxygen Documentation/Doxyfile` depuis la racine échoue (`source '…' is not a readable file`).
> Le script se place dans `Documentation/` pour vous, quel que soit le répertoire d'appel.

**Version de Doxygen : `1.16.1`**, épinglée en CI (`EX-NFR-031`) et à installer à l'identique en
local. Les versions plus anciennes appliquent des règles de résolution de liens différentes : une
documentation générée sans avertissement avec une autre version peut échouer en CI, et la
vérification locale ne prédirait plus rien. Binaires officiels :
<https://github.com/doxygen/doxygen/releases/tag/Release_1_16_1>.

## Intégration continue

Chaque job ci-dessous est un **contrôle requis pour merger** (protection de branche), à l'exception
de `docs` (`docs.yml`, informatif).

| Workflow | Job | Déclencheur | Rôle |
|----------|-----|-------------|------|
| **CI** (`ci.yml`) | `build-test-coverage` | PR vers `main` | Build + tests (CTest) **Debug** sur `windows-2022`, **couverture** agrégée `UnitTests`+`IntegrationTests`+`SystemTests` avec seuil (LOT-58). |
| **CI** (`ci.yml`) | `build-test-release` | PR vers `main` | Build + tests **Release** (LOT-58) : une casse Release-only (variable inutilisée sous `NDEBUG`…) est refusée avant le tag, pas après. |
| **CI** (`ci.yml`) | `build-ninja` | PR vers `main` | Build + tests via le générateur Ninja (détection Qt automatique). |
| **CI** (`ci.yml`) | `sanitize` | PR vers `main` | Les trois exécutables de test sous **AddressSanitizer** (LOT-58, `EX-NFR-003`). |
| **CI** (`ci.yml`) | `clang-tidy` | PR vers `main` | Analyse statique sur le diff de la PR ; `bugprone-*` bloquant, le reste consigné (LOT-58). |
| **CI** (`ci.yml`) | `format` | PR vers `main` | `clang-format --dry-run --Werror`, version épinglée (LOT-58). |
| **CI** (`ci.yml`) | `lint-exigences` | PR vers `main` | Identifiants `EX-…`, graphe des lots, cahier de test, catalogues et assets. |
| **Documentation** (`docs.yml`) | `docs` | Push sur `main` | Assemble le site — pages, référence Doxygen (`WARN_AS_ERROR`), planification, qualité — et le publie sur `gh-pages`. |
| **Release** (`release.yml`) | `rolling-debug` | Push sur `main` | Compile un exécutable **Debug autonome** et publie la préversion roulante **`debug-latest`** pour les non-développeurs. |
| **Release** (`release.yml`) | `versioned-release` | Tag `vX.Y.Z` | Publie une **release versionnée** (non préversion) avec les exécutables **Debug et Release**, chacun autonome. |

> « Autonome » signifie qu'aucune installation n'est requise côté utilisateur : `windeployqt` dépose
> les DLL Qt, le plugin de plateforme et le runtime du compilateur à côté de l'exécutable. Le
> runtime MSVC est **dynamique** (`/MD`) et non statique — les DLL Qt officielles sont construites
> ainsi, et un CRT statique provoquerait des incohérences d'allocation entre l'application et Qt.

## Licence

**JustAnotherRpgGame est un fan game gratuit et sans but commercial.** Il ne se vend pas, ne se
monnaye pas, et ses licences interdisent à quiconque d'en faire un usage commercial.

| Ce qui est couvert | Licence | Texte |
|---|---|---|
| Le **code** du projet (`Source/**/*.h`, `*.cpp`, `*.qml`, `scripts/`, `CMakeLists.txt`…) | **PolyForm Noncommercial 1.0.0** (`PolyForm-Noncommercial-1.0.0`) | [`LICENSE`](LICENSE) |
| Les **contenus originaux** du projet (textes, données, images et sons créés pour lui) | **Creative Commons BY-NC-SA 4.0** (`CC-BY-NC-SA-4.0`) | [`LICENSE-CONTENT`](LICENSE-CONTENT) |
| Les **ressources et bibliothèques tierces** (Kenney, polices, Qt…) | leur licence propre (CC0, SIL OFL, LGPLv3, MIT…) | [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md) |
| Les **univers, règles et marques** dont le jeu s'inspire (Dungeons & Dragons, Tanares) | **aucune** : ils restent la propriété de leurs ayants droit | ci-dessous |

Ce que cela implique concrètement :

- **Lire, étudier, modifier et partager le code gratuitement : oui**, pour tout usage non commercial
  (personnel, amateur, éducatif), à condition de transmettre la licence et la ligne
  `Required Notice` en tête de [`LICENSE`](LICENSE).
- **Vendre le jeu, un dérivé ou un service fondé dessus : non.** Ce n'est donc **pas** une licence
  *open source* au sens de l'OSI, qui exige d'autoriser le commerce : le code est **ouvert en
  lecture**, pas libre de tout usage.
- **Qt reste sous LGPLv3**, en lien dynamique : la LGPLv3 n'impose rien à la licence de
  l'application qui l'utilise, tant que ses DLL restent remplaçables. Les obligations propres à Qt
  sont détaillées dans [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md).
- Les en-têtes `SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0` marquent le **code**, et lui
  seul. Un `.png` ou un `.wav` du dépôt n'est pas sous PolyForm pour autant.

### Fan game, univers et marques

JustAnotherRpgGame est une **œuvre de fan, non officielle**. Il s'inspire de deux univers qui ne lui
appartiennent pas, et **aucune licence du projet ne s'étend à eux** :

- **Dungeons & Dragons** — *Dungeons & Dragons*, *D&D* et les noms de produits de Wizards of the
  Coast sont des **marques de Wizards of the Coast LLC**. Les manuels de la 5ᵉ édition en français
  (*Manuel des Joueurs*, *Guide du Maître*, *Manuel des Monstres*) sont © Wizards of the Coast LLC,
  version française **Black Book Éditions** ; les *Basic Rules* en français sont une traduction
  communautaire d'[AideDD](https://www.aidedd.org/).
- **Tanares** — l'univers de *Tanares*, le *Player's Guide to Tanares* et le *Tanares Sourcebook*
  sont © **Dragori Games, Inc.** « Tanares », « Penumbral Plane » et les autres noms réservés par
  l'éditeur sont son *Product Identity*.

Le projet n'est **ni affilié, ni approuvé, ni soutenu** par Wizards of the Coast, Black Book Éditions
ou Dragori Games. Les livres eux-mêmes ne sont pas dans le dépôt. Tout ayant droit qui souhaite le
retrait d'un élément peut le demander : il sera retiré.

Une partie des règles provient du **System Reference Document 5.1**, seule partie de D&D ouverte à
tous :

> This work includes material taken from the System Reference Document 5.1 (“SRD 5.1”) by Wizards of
> the Coast LLC and available at <https://dnd.wizards.com/resources/systems-reference-document>. The
> SRD 5.1 is licensed under the Creative Commons Attribution 4.0 International License available at
> <https://creativecommons.org/licenses/by/4.0/legalcode>.

Ces mentions sont aussi affichées **dans le jeu**, à l'écran *Crédits* — la LGPLv3 de Qt et la SIL
OFL des polices l'exigent, et un joueur qui n'ouvrira jamais ce dépôt doit pouvoir les lire.
