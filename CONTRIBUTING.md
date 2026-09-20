# Contribuer à JustAnotherRpgGame

## Conventions de code
Voir [`Documentation/Specification/conventions.md`](Documentation/Specification/conventions.md). Le code doit être formaté (`clang-format`) et compiler sans avertissement avant tout commit.

## Poste de développement
Le poste exécute les mêmes outils que la CI, aux mêmes versions, lues dans `env:` de
`.github/workflows/ci.yml` — aucune version n'est écrite ailleurs.
- **Vérifier** : `powershell -ExecutionPolicy Bypass -File scripts/setup_dev.ps1` affiche chaque
  outil avec la version attendue et la version trouvée. **Installer** ce qui diverge :
  `… setup_dev.ps1 -Install` (`-WhatIf` pour voir sans rien faire) — LLVM, Doxygen et
  OpenCppCoverage par winget, sccache par son archive officielle, pre-commit, clang-format et uv
  par pip, PSScriptAnalyzer depuis la PowerShell Gallery ; puis l'environnement Python et les hooks
  du clone. Visual Studio et Qt ne sont que vérifiés.
- **Python des scripts** (`pyproject.toml`, `uv.lock`) : `uv sync --locked` crée `.venv/` avec
  exactement les dépendances du runner (jsonschema, pytest). Tests des scripts : `uv run pytest`.
- **Hooks** (`.pre-commit-config.yaml`) : avant chaque commit, clang-format, ruff, actionlint,
  zizmor, gitleaks, conflits de fusion et de casse, YAML, JSON (`scripts/check_json_files.py`) et
  garde-fou binaires (`scripts/check_binary_files.py`) ; à la rédaction du message, son format. À
  installer **dans chaque worktree** : `pre-commit install`. Tout rejouer :
  `pre-commit run --all-files`. Le job `pre-commit` de la CI les rejoue sur tout le dépôt.
- **Tous les contrôles du référentiel en une commande** : `uv run scripts/check.py`. Il lit les
  étapes du job `lint-exigences` dans `ci.yml` et les exécute (pytest et PSScriptAnalyzer compris),
  puis lance les hooks — un contrôle ajouté à la CI y est rejoué sans qu'on y pense.
- **Cache de compilation** : dès que `sccache` est dans le PATH, les presets Ninja compilent à
  travers lui (`ENABLE_COMPILER_CACHE`, `CMakeLists.txt`) ; le preset `vs` n'est pas concerné.
  Avec un MSVC en français, CMake ne l'active pas : sccache réécrit les lignes `/showIncludes` et
  Ninja perdrait des dépendances d'en-têtes. Module linguistique anglais de Visual Studio et
  `VSLANG=1033` pour en profiter.
- **Tests d'un seul étage** : `scripts/build.ps1 -Label unitaire` (ou `integration`, `systeme`).
- **Tests Qt Quick** (`Source/Test/Qml`, cible `QmlTests`, étage `unitaire`) : chaque `.ui.qml` de
  `Jadg.Ui` se construit sans avertissement, les briques se comportent comme la galerie le suppose,
  et chaque écran ressemble à sa **capture de référence** (`Source/Test/Qml/References`, rendu
  logiciel à 960 × 540). Un écran modifié **volontairement** : régénérer ses références avec
  `JADG_UPDATE_REFERENCES=1` puis `ctest --preset ninja -R QmlTests`, et relire les images dans le
  diff. En cas d'écart, la capture et l'image des différences sont dans `build/<preset>/qml-captures`.
- **Traductions** : une chaîne ajoutée à un écran se traduit dans la même PR.
  `cmake --build --preset ninja --target update_translations` met `jadg_en.ts` à jour du code (les
  chaînes retirées en sortent), Qt Linguist le traduit, `scripts/check_translations.py` le vérifie —
  le job `build-ninja` rejoue les deux premiers.
- **Plantages** : le jeu et l'éditeur écrivent un minidump sous `Crashes/`, à côté de `Logs/`. Le
  lire : ouvrir le `.dmp` dans Visual Studio avec le zip de **symboles** de la même version (son nom
  porte la version). `JustAnotherRpgGame.exe --crash-test` provoque un plantage pour l'éprouver.
- **Couverture sur le poste** : `powershell -File scripts/coverage.ps1 -BinDir build/vs/bin/Debug`
  (même script que la CI), rapport dans `coverage-html/`.
- **Éditeur** : `.clangd` branche clangd sur `build/ninja/compile_commands.json` et les checks de
  `.clang-tidy`.
- **Binaires** : aucun fichier au-delà de 5 Mio, et un fichier binaire doit avoir une extension
  déclarée `binary` dans `.gitattributes` — l'y déclarer est la décision d'admettre une nouvelle
  famille d'assets.

## Messages de commit — Conventional Commits
Format : `<type>(<portée facultative>): <description à l'impératif>`, ou, sur une branche de lot,
`LOT-NN — <description>` (tiret cadratin). Vérifié par le hook `commit-msg`
(`scripts/check_commit_message.py`) ; les messages de fusion, `Revert` et `fixup!` sont admis.

Types :
| Type | Usage |
|------|-------|
| `feat` | Nouvelle fonctionnalité |
| `fix` | Correction de bug |
| `docs` | Documentation seule |
| `refactor` | Refonte sans changement de comportement |
| `test` | Ajout/modification de tests |
| `build` | Build, CMake, dépendances |
| `ci` | Intégration continue |
| `chore` | Tâche diverse (config, outillage) |

Exemples :
```
feat(core): ajouter la détection de collision AABB
fix(hmi): corriger le ratio d'aspect au redimensionnement
docs(conventions): préciser la politique de gestion d'erreurs
```
La portée correspond en général au module (`core`, `hmi`, `elements`, `test`, `build`…).

## Stratégie de branches
- `main` est **protégée** : **aucun push direct**. Toute évolution passe par une **Pull Request**.
- **Une branche par lot** : `lot/LOT-XX-nom-du-lot` (ex. `lot/LOT-03-fondation-ecs`).
  - Pour un correctif isolé hors lot : `fix/...` ; pour de la doc seule : `docs/...`.
- Ouvrir une **Pull Request** vers `main`. Le merge exige une **CI verte** (build + tests + couverture, zéro avertissement).
- `main` reste **toujours compilable et testée**.

## Automatisations sur `main` (après merge d'une PR)
- **CI** (`.github/workflows/ci.yml`) : s'exécute sur chaque PR ; contrôle requis pour merger.
- **Release** (`release.yml`) : à chaque push sur `main`, republie l'exécutable Debug autonome
  dans la **Release roulante `debug-latest`** (préversion, toujours à jour). À chaque tag
  `vX.Y.Z` poussé, publie une **Release versionnée** (non préversion) avec les exécutables
  **Debug et Release**, chacun autonome — destinés aux non-développeurs (télécharger,
  décompresser, lancer). La release versionnée n'est publiée que si le tag est **sur `main`** et
  que les tests passent **sur ce commit** en Debug et en Release (job `test-tag`). Chaque release
  porte aussi un zip de **symboles** (`.pdb`) par configuration, sans lequel un plantage de la
  version livrée est illisible, et un fichier **`SHA256SUMS`** (`sha256sum -c SHA256SUMS`).
  Avant toute publication, chaque archive jouable est **décompressée et lancée**
  (`scripts/smoke_test_release.ps1` : le jeu doit rendre une image et quitter seul), et chaque
  fichier reçoit une **attestation de provenance** :
  `gh attestation verify <archive>.zip --repo azertval/JustAnotherRpgGame` prouve qu'il sort de ce
  workflow et de ce commit.
- **Nuit** (`nightly.yml`, 02 h 17 UTC, non bloquant) : clang-tidy sur tout `Source/`, tests en
  ordre aléatoire répété (la graine est dans le résumé), MSVC `/analyze` et cppcheck, fuzzing des
  lecteurs de données (`Source/Fuzz`, libFuzzer de MSVC), mesures de performance avec historique
  (`Source/Benchmark`, branche `benchmarks`), lancement de l'archive Release, build contre la
  version de Qt suivante, liens de la documentation (`lychee.toml`). Une PR qui modifie ce workflow
  ou ses sources l'exécute en version courte. Les analyses vont dans *Security > Code scanning*,
  chacune dans sa catégorie ; **CodeQL** (`codeql.yml`) y ajoute les siennes sur chaque PR.
- Tous les workflows se relancent à la main depuis l'onglet **Actions** (`workflow_dispatch`),
  sans commit vide. Un nouveau push sur une PR **annule** le run précédent.
- Les actions GitHub sont **épinglées par SHA** de commit, le tag en commentaire ; **Dependabot**
  (`.github/dependabot.yml`) propose leur mise à jour chaque semaine, en une PR, ainsi que celle des
  dépendances Python (`uv.lock`). **Renovate** (`renovate.json`) ne suit que ce que Dependabot ne
  lit pas : les `GIT_TAG` de FetchContent (`External/CMakeLists.txt`). L'installation de Qt n'est
  écrite qu'une fois : `.github/actions/setup-qt/action.yml`.
- **Lire une PR sans ouvrir de log** : les tests des trois builds sont publiés en commentaire et en
  check run (`test-report`) ; la couverture des lignes ajoutées est commentée par **Codecov**
  (informatif, le seul seuil bloquant reste celui de `ci.yml`) ; les avertissements MSVC, les
  assertions GoogleTest, les écarts `clang-format` et les diagnostics `clang-tidy` (en SARIF, onglet
  *Security > Code scanning*) s'affichent en **annotation sur la ligne** ; chaque job écrit un
  **résumé** en tête du run.
- **`CHANGELOG`** (`changelog.yml`) : une PR doit ajouter au moins une ligne à `## [Non publié]`,
  ou porter le label **`no-changelog`** si elle n'apporte rien de notable (CI interne, coquille,
  PR de publication d'une version). Dependabot en est exempté.

## Publier une version
1. Bumper `VERSION` dans le `project()` du `CMakeLists.txt` racine — **seul** endroit où le numéro
   est écrit : il alimente `core::Engine::version()` à la compilation, et `scripts/build_docs.py`
   l'injecte dans la documentation générée. Rien d'autre à aligner à la main.
2. Dans `CHANGELOG.md`, transformer `## [Non publié]` en `## [X.Y.Z] - AAAA-MM-JJ`, lui ajouter un
   chapeau de jalon, et rouvrir un `## [Non publié]` vide au-dessus. Cette PR n'ajoute rien à la
   section : lui poser le label `no-changelog`.
3. Vérifier les notes que produira la release :
   `python scripts/extract_release_notes.py vX.Y.Z` — le workflow lit **cette** section du
   CHANGELOG (`--notes-file`) et **échoue** si elle est absente.
4. Merger, puis poser le tag sur le commit de merge : `git tag vX.Y.Z && git push origin vX.Y.Z`.
- **Documentation et site qualité** (`docs.yml`) : à chaque merge, publie sur **`gh-pages`** la
  Doxygen (racine du site) et la page **qualité** (`/qualite/`) : couverture de `main` par domaine
  et son rapport détaillé, dernières mesures de performance de la nuit et leurs courbes. Republiée
  chaque matin pour y faire entrer les mesures de la nuit. Le **site de planification**
  (`/planning/`) est engendré au même moment depuis `Planning/`
  (`python Planning/outils/build_planning_site.py --out build/planning-site` pour le voir en local).

## Avant d'ouvrir une PR
0. `uv run scripts/check.py` est vert (contrôles du référentiel, tests des scripts et hooks).
1. `cmake --build --preset vs` compile sans avertissement.
2. `ctest --preset vs` passe à 100 %.
3. `cmake --preset vs && cmake --build --preset vs-release && ctest --preset vs-release` compile et
   teste en configuration **Release** (LOT-58) : certaines casses (variable lue uniquement par une
   assertion, code conditionné à `core::kDeveloperBuild`) ne se voient qu'ici.
4. Le code est formaté (`clang-format`) et les nouveaux comportements sont couverts par des tests.
   Vérifié en CI (LOT-58) avec une version **épinglée** (`LLVM_VERSION` dans `ci.yml`) : deux
   versions majeures ne formatent pas identiquement. Reproduire localement (même version,
   installée en isolation via le paquet PyPI qui redistribue les binaires officiels LLVM, sans
   dépendre de celle fournie par l'IDE) :
   `pip install "clang-format==$LLVM_VERSION" && git ls-files 'Source/*.cpp' 'Source/*.h' | xargs clang-format --dry-run --Werror --style=file`
5. `clang-tidy` sur les fichiers `Source/*.cpp` modifiés (LOT-58) :
   `cmake -S . -B build/ninja-tidy -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_PCH=OFF`
   puis `clang-tidy -p build/ninja-tidy <fichier.cpp>` (version LLVM épinglée : `LLVM_VERSION` dans
   `ci.yml`). Seules les violations `bugprone-*` font échouer la CI ; les autres familles
   (`cppcoreguidelines-*`, `modernize-*`, `performance-*`, `readability-*`) restent visibles mais
   non bloquantes (triage complet hors périmètre du `LOT-58`, voir
   `Documentation/Lot/LOT-58-verification-release-analyse/tache-03-clang-tidy.md`).
6. Le `CHANGELOG.md` (section `## [Non publié]`) consigne l'apport de la PR — vérifié en CI
   (`changelog.yml`) ; sinon, label `no-changelog`.
7. Si `QT_VERSION_MINIMUM` (`Source/HMI/CMakeLists.txt`) a changé, `env.QT_VERSION` de `ci.yml` et
   `release.yml` doit être bumpé à l'identique — vérifié automatiquement par
   `python scripts/check_qt_version_pin.py` (job `lint-exigences`), pas seulement par relecture.
   Depuis le `LOT-69`, la CI installe Qt avec un `aqtinstall` pris **depuis git à un commit
   épinglé** (`env.AQT_SOURCE`), la version PyPI ne sachant pas installer Qt ≥ 6.11 : dès
   qu'`aqtinstall 3.3.1` paraît, remplacer `aqtsource` par `aqtversion: '==3.3.1'` et supprimer
   `AQT_SOURCE`. Le motif complet est dans [`External/README.md`](External/README.md).
