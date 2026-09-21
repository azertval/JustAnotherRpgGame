# Exigences non fonctionnelles

> Statut : **livré**. Outillage qualité effectivement exécuté à chaque PR (CI Debug **et** Release,
> clang-tidy, clang-format, ASan, couverture agrégée, contrôles de reproductibilité). Transverse à
> toutes les specs.

## 1. Performance
- **EX-NFR-001** — Le jeu doit maintenir **60 images/seconde** sur une
  configuration de bureau récente. Rendue **observable** par l'incrustation de diagnostic des
  options : la cadence dépend de la machine, elle **reste hors de portée d'un contrôle
  automatique** (une machine virtuelle partagée ne la mesure pas de façon reproductible).
- **EX-NFR-002** — La simulation doit fonctionner à **pas de temps fixe** et rester déterministe (mêmes entrées → même résultat).
- **EX-NFR-003** — L'empreinte mémoire doit rester stable dans le temps (aucune
  fuite ; vérifiable via AddressSanitizer). Vérifiée par le job `sanitize` de `ci.yml` : les
  exécutables de test s'exécutent sous AddressSanitizer à chaque PR.
- **EX-NFR-005** — Le rendu ne doit soumettre que les primitives **effectivement
  visibles** : le contenu hors du cadrage de la caméra est écarté avant soumission, marge comprise.
  Le nombre de primitives émises par image doit rester **borné et observable**. **Vérifiée** sans
  GPU par les tests du culling (`Source/Test/Unit/HMI/Graphics/test_render_culling.cpp`,
  `EX-NFR-004`).

## 2. Architecture & maintenabilité
- **EX-NFR-010** — La logique (`Core`) doit être **indépendante** de la présentation (`HMI`) et testable sans fenêtre ni GPU.
- **EX-NFR-011** — Aucune dépendance cyclique entre modules (`HMI → Core`, jamais l'inverse).
- **EX-NFR-012** — Le code doit respecter le [guide de conventions](conventions.md) (nommage, RAII, documentation, gestion d'erreurs).
- **EX-NFR-013** — Le code livré doit compiler **sans avertissement** (`/W4 /WX`).

## 3. Qualité & vérification
- **EX-NFR-020** — Toute logique de gameplay livrée dans `Core` doit être couverte par des **tests unitaires** (GoogleTest).
- **EX-NFR-022** — La **CI** doit exécuter build, tests et couverture à chaque
  push/PR et rester verte pour merger. La couverture agrège `UnitTests`, `IntegrationTests` et
  `SystemTests` (job `build-test-coverage` de `ci.yml`) ; une chute sous le seuil consigné
  (`COVERAGE_THRESHOLD_PERCENT`) fait échouer la CI.
- **EX-NFR-023** — La configuration **Release** doit être construite et testée en
  CI, sur **chaque PR** — avant tout tag, jamais découverte après. Vérifiée par le job
  `build-test-release` de `ci.yml`, contrôle requis pour merger au même titre que
  `build-test-coverage`.
- **EX-NFR-024** — L'analyse statique et le formatage doivent être **vérifiés
  automatiquement**, pas seulement configurés. Vérifiés par les jobs `clang-tidy` (`bugprone-*`
  bloquant, le reste consigné) et `format` (`clang-format --dry-run --Werror`, version épinglée) de
  `ci.yml`.
- **EX-NFR-004** — La chaîne de rendu doit être **vérifiable sans GPU** : les
  primitives de dessin produites pour une scène donnée doivent pouvoir être **capturées et
  inspectées** par un test (ordre des calques, tri par profondeur, choix des pièces, effets de bord
  du culling). Un critère d'acceptation du type « rendu identique » ou « ordre de calque correct »
  ne doit pas reposer sur une vérification à l'œil quand il peut être asserté.

## 4. Portabilité & reproductibilité
- **EX-NFR-030** — Le projet doit se construire **exclusivement via CMake**, reproductible sur plusieurs postes (cf. `README`).
- **EX-NFR-031** — Les dépendances tierces doivent être **épinglées** à une version (ex. GoogleTest).
- **EX-NFR-032** — Cible actuelle : **Windows/Direct3D**. Le `Core` doit toutefois éviter toute dépendance système inutile pour préserver sa testabilité (et une portabilité future).

## 5. Robustesse
- **EX-NFR-040** — Une erreur récupérable (fichier de carte invalide, ressource manquante) ne doit pas faire planter le jeu : elle est signalée et gérée (cf. politique d'erreurs des conventions).
- **EX-NFR-041** — Les ressources (mémoire, ressources GPU) doivent être gérées en **RAII** (libération garantie).
- **EX-NFR-042** — Une **version publiée** doit produire une **trace exploitable**
  d'exécution : journal écrit dans un fichier à côté de l'exécutable, contenant au minimum la
  version, la configuration de build et le contexte matériel (adaptateur graphique, manette). Le
  volume est **borné** (taille maximale et rotation), la trace reste **locale** — aucun envoi réseau,
  aucune donnée personnelle — et un dossier inaccessible dégrade la journalisation, jamais le jeu
  (`EX-NFR-040`). Sans cela, un défaut signalé par un joueur n'est accompagné d'aucun élément.

## 6. Build & dépendances
- **EX-BUILD-010** — Une dépendance tierce **non gérable par `FetchContent`**
  (SDK volumineux tel que **Qt**) doit être **provisionnée et documentée de façon reproductible** sur
  les trois environnements : poste local (installeur officiel ou `aqtinstall`), **CI** (étape
  d'installation dans le workflow, sur le runner épinglé) et **release** (déploiement des bibliothèques
  dynamiques requises à côté de l'exécutable, ex. `windeployqt`). La version est **épinglée**
  (`EX-NFR-031`) et la licence documentée. Le poste local déclare une
  version minimale (`QT_VERSION_MINIMUM`, `Source/HMI/CMakeLists.txt`) alignée sur celle de la CI
  et vérifiée automatiquement contre elle (`scripts/check_qt_version_pin.py`) ; un écart local
  produit un avertissement explicite plutôt qu'une divergence silencieuse. L'exigence de
  reproductibilité porte aussi sur l'**outil de provisionnement lui-même** : lorsqu'il doit être
  pris ailleurs que sur son dépôt de paquets habituel — cas d'`aqtinstall`, dont la version publiée
  ne sait pas installer Qt ≥ 6.11 — la source est épinglée à une **révision
  précise** (jamais une branche mobile), et le motif du détour ainsi que sa condition de sortie
  sont écrits à l'endroit où il est déclaré.

## Exigences retirées {#nfr-retirees}

> Ancres conservées, jamais renumérotées : les lots livrés s'y réfèrent.

- **EX-NFR-021** *(retirée en `LOT-67`)* — test système de franchissabilité des
  niveaux : un bac à sable n'a pas de sortie qui le termine.
- **EX-NFR-043** *(retirée au `LOT-88`)* — plafond de mémoire de texture des
  plans picturaux.

## Traçabilité
Ces exigences transverses conditionnent l'acceptation de chaque lot. Elles s'appuient sur de l'outillage **effectivement exécuté** à chaque PR (CMake, CI Debug **et**
Release, clang-tidy, clang-format, ASan, couverture agrégée) et non plus seulement configuré :
voir le tableau « Outillage qualité » de [`conventions.md`](conventions.md) pour le job qui vérifie
chaque outil.

`EX-NFR-032` (cible Windows/Direct3D) est, comme les exigences ci-dessus, un **invariant
transverse** : aucun lot n'a besoin de le citer pour le respecter, ce silence n'est pas une
exigence orpheline.
