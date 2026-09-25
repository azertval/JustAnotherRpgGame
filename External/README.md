# External/

Dépendances **tierces** du projet.

## Stratégies
- **FetchContent** : la dépendance est récupérée automatiquement à la configuration CMake (cas de **GoogleTest**, voir `CMakeLists.txt`). Rien n'est versionné dans le dépôt.
- **Vendored** : une bibliothèque copiée ici, dans son propre sous-dossier, puis intégrée via `add_subdirectory()` dans `CMakeLists.txt`.

- **Provisionné (hors dépôt)** : SDK trop volumineux pour FetchContent (ex. **Qt**), installé
  séparément (poste local, runner CI) et localisé par CMake via `find_package` / `CMAKE_PREFIX_PATH`.

## Actuel
| Dépendance | Version | Mode |
|------------|---------|------|
| GoogleTest | v1.15.2 | FetchContent |
| nlohmann/json | v3.11.3 | FetchContent |
| **Qt** | **6.11.2** (`win64_msvc2022_64`) | **Provisionné** (LOT-34, relevé en LOT-69) |

> DirectX ne se place pas ici : il provient du **Windows SDK** et est lié directement (`d3d11.lib`, `dxgi.lib`, `d3dcompiler.lib`, …) au moment du lot de rendu.

## Qt (LOT-34, refonte IHM)

Qt porte les deux exécutables du projet : le jeu `JustAnotherRpgGame` (Qt Quick, `Source/App/Game`
et `Source/HMI`) et l'éditeur `LevelEditor` (Qt Widgets, `Source/Editor`), sur les données
déclaratives de `Source/Elements`. **Qt 6.11+** est requis depuis le `LOT-69` : c'est la première
version à fournir **Qt Canvas Painter** (peinture 2D accélérée sur cible QRhi). L'archi
`win64_msvc2022_64` reste celle retenue (Qt 6.7 n'exposait que `win64_msvc2019_64`). Licence
**LGPLv3, lien dynamique** — aucune obligation de publication du source du jeu ; les DLL Qt sont
redistribuées à côté de l'exécutable (`windeployqt`).

> **Qt ≥ 6.11 et aqtinstall.** Qt a changé la disposition de son dépôt à partir de 6.11 :
> `qt6_6112/qt6_6112_msvc2022_64/` (un sous-dossier par architecture) au lieu de
> `qt6_6103/qt6_6103/`. **aqtinstall 3.3.0, dernière version publiée sur PyPI, code en dur
> l'ancienne** et échoue sur `Failed to download checksum for the file 'Updates.xml'`. Le correctif
> amont est mergé (`miurahr/aqtinstall` PR #1000, issues #959/#1007) mais pas encore publié : il
> faut donc installer aqt **depuis git**, à un commit épinglé. À remplacer par la version PyPI dès
> qu'`aqtinstall 3.3.1` paraît — ce détour n'a pas vocation à rester.

- **Poste local** (recommandé, scriptable) :
  ```
  pip install "git+https://github.com/miurahr/aqtinstall.git@16db45a70b5905ad596941b223469bc86a56901e"
  python -m aqt install-qt windows desktop 6.11.2 win64_msvc2022_64 ^
      -m qtmultimedia qtshadertools qtcanvaspainter --outputdir C:/Qt
  ```
  puis configurer en pointant CMake sur l'installation :
  ```
  cmake --preset vs -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64
  ```
  (ou installeur officiel Qt). Les exécutables sont **optionnels** à la configuration : sans Qt,
  elle n'échoue pas, mais seuls les **tests** se construisent (ni jeu, ni éditeur) ; on l'obtient
  explicitement avec `-DBUILD_EDITOR_QT=OFF`.
- **CI** (`.github/workflows/ci.yml` et les autres workflows) : l'installation passe par l'action
  composite du dépôt, `.github/actions/setup-qt`, qui enveloppe `jurplel/install-qt-action` (avec
  cache) avant la configuration ; `CMAKE_PREFIX_PATH` est renseigné depuis `QT_ROOT_DIR`. L'action
  est un wrapper d'aqtinstall : elle hérite donc de la limite ci-dessus, contournée par son entrée
  **`aqtsource`** (prioritaire sur `aqtversion`), pointée sur le même commit épinglé.
- **Release** (`.github/workflows/release.yml`) : Qt y est installé par la même action, et
  **`windeployqt`**, lancé à la construction (`POST_BUILD`), dépose les DLL Qt et leurs greffons
  à côté des deux exécutables ; l'archive publiée emporte tout le dossier.
