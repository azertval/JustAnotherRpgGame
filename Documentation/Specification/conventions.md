# Conventions de code

Règles à respecter dès la première ligne, pour garder un code cohérent et une documentation Doxygen exploitable. Le squelette existant (`Source/Core`, `Source/HMI`) suit déjà ces règles.

---

## 1. Langage & standard
- **C++20** (fixé dans `CMakeLists.txt`, extensions compilateur désactivées).
- Préférer la bibliothèque standard aux API brutes quand c'est possible.
- Pas de `using namespace` en portée de fichier d'en-tête.

## 2. Nommage

| Élément | Convention | Exemple |
|---------|-----------|---------|
| Fichier | PascalCase, `.h` / `.cpp` | `Core.h`, `PlayerController.cpp` |
| Espace de noms | minuscules | `core`, `hmi` |
| Type (class, struct, enum) | PascalCase | `GameState`, `TileMap` |
| Fonction / méthode | camelCase | `version()`, `updatePhysics()` |
| Variable / paramètre | camelCase | `deltaTime`, `tileIndex` |
| Membre de classe | prefixe `_` + camelCase  | `_position`, `_isGrounded` |
| Constante (`constexpr`) | UPPER_SNAKE_CASE | `MAXIMUM_ENTITIES` |
| Macro (à éviter) | UPPER_SNAKE_CASE | `JADG_ASSERT` |

- Un fichier `.h`/`.cpp` porte le nom du type ou module principal qu'il contient.
- Noms en **anglais** pour le code (identifiants), commentaires et documentation **en français**.
- **Aucune abréviation** dans les noms de variables et de fonctions : le nom est écrit en toutes lettres (`deltaTime` et non `dt`, `position` et non `pos`, `updatePhysics` et non `updPhys`). Seuls les prénoms d'itérateurs conventionnels (`i`, `j`) et les termes standard du domaine restent tolérés.

## 3. Mise en forme
- Indentation : **4 espaces**, jamais de tabulation.
- Accolades style K&R (ouvrante en fin de ligne) :
  ```cpp
  if (_isGrounded) {
      jump();
  }
  ```
- Une déclaration par ligne. Toujours des accolades, même pour un bloc d'une ligne.
- Longueur de ligne indicative : **100 colonnes**.
- Include guard : **`#pragma once`** (pas de macros de garde manuelles).

## 4. Inclusions (\#include)

### Chemins complets depuis Source/
Les en-têtes du projet s'incluent par leur **chemin complet depuis la racine `Source/`**, jamais par nom seul :

```cpp
#include "Core/Core.h"                    // ✅ chemin complet
#include "HMI/Game/GameViewport.h"        // ✅
#include "Core.h"                         // ❌ nom seul
```

- Séparateur : **slash `/`** (portable), y compris sous Windows — jamais d'antislash.
- La racine d'inclusion `Source/` est fournie par CMake (`target_include_directories(... ${PROJECT_SOURCE_DIR}/Source)`).
- Bénéfice : les dépendances sont explicites et un déplacement de fichier lors d'une refonte d'architecture se repère et se corrige par simple recherche du chemin.

### Ordre des groupes
Dans un `.cpp`, du plus proche au plus général, chaque groupe trié et séparé par une ligne vide :
1. L'en-tête correspondant au fichier (`#include "Core/Core.h"` en premier dans `Core.cpp`).
2. En-têtes standard (`<string>`, `<vector>`, …).
3. En-têtes tiers (`<gtest/gtest.h>`, DirectX…).
4. En-têtes du projet, chemin complet (`"Core/..."`, `"HMI/..."`).

## 5. Architecture (dépendances entre modules)
- **`Core`** : logique/moteur, **indépendant** de la présentation. Ne connaît ni DirectX ni la fenêtre.
- **`HMI`** : dépend de `Core`, jamais l'inverse. L'unique application **Qt** (`JustAnotherRpgGame`) : rendu Direct3D 11 du jeu, entrées, et widgets Qt de l'IHM hors-jeu.
- **`Elements`** : données/assets statiques, aucun code exécutable — dont les **assets Qt déclaratifs** (`.ui`, `.qrc`, thèmes `.qss`).
- Aucune dépendance cyclique. `Core` reste testable sans fenêtre ni GPU.

### IHM Qt : la mise en page hors code pour le jeu, en code pour l'éditeur
- **Les écrans du jeu** sont des formulaires `.ui.qml` (`Source/Ui`), dessinés dans Qt Design Studio ; leurs textes passent par `qsTr` (`EX-REN-033`, `EX-IHM-103`).
- **L'éditeur** est un outil interne (`LOT-EDITOR-01`) : ses widgets sont **construits en code**, chaque panneau gardant les siens dans une `struct Widgets` privée ; ses textes sont en **anglais, écrits dans le code**, sans clé de traduction ; son style est Fusion, sans feuille de style.

### Classes plutôt que fonctions libres
- **Privilégier une classe** (avec état encapsulé et membres privés) à un ensemble de fonctions libres dans un espace de noms. Une classe se dérive et se spécialise : elle permet d'étendre les comportements sans réécrire les appelants.
- Réserver les fonctions libres dans un `namespace` aux utilitaires purs, sans état.
- Exposer le minimum : membres et méthodes internes en `private` par défaut, `public` seulement pour l'interface réellement nécessaire.

### RAII obligatoire
- **Toute classe respecte le RAII** : une ressource (mémoire, handle DirectX, fichier…) est acquise dans le constructeur et libérée dans le destructeur. Aucune méthode `init()` / `cleanup()` manuelle à appeler séparément.
- Un objet construit est immédiatement dans un état valide et utilisable ; sa destruction libère tout.
- Bénéfice recherché : **traçage des bugs et tests facilités** — la durée de vie d'une ressource est celle de l'objet, sans état intermédiaire à moitié initialisé.
- Gérer la copie/déplacement explicitement (règle des 0/3/5) ; préférer `= default` ou `= delete` à une implémentation manuelle quand c'est possible.

## 6. Documentation Doxygen
Tout élément public (fichier, type, fonction) est documenté **en français**. `JAVADOC_AUTOBRIEF` est actif : la première phrase sert de description courte.

### Doxygen dans le header, commentaires simples // dans le .cpp
La documentation Doxygen (exportée vers le site) vit **uniquement dans les en-têtes** (`.h`) :
- Dans le **`.h`** : le bloc Doxygen `/** … */` complet — contrat de la fonction (rôle,
  `@param`, `@return`), lu par les appelants et exporté sur le site. C'est la **source unique**.
- Dans le **`.cpp`** : **pas de bloc Doxygen** (`/** */`, `///`, `//!`), uniquement des
  **commentaires simples `//`** (non exportés) documentant le **corps** de la fonction.

Ne **jamais** redupliquer les `@param`/`@return` dans le `.cpp` : cela disperse la source de
vérité et fait échouer la génération (Doxygen ≥ 1.16 rejette les `@param` documentés à la fois
dans la déclaration et la définition — « multiple @param documentation sections »).

**Déclaration (`Core.h`) — doc Doxygen exportée**
```cpp
/**
 * @brief Met à jour la physique d'une entité pour une frame.
 * @param entity    Entité à mettre à jour.
 * @param deltaTime Temps écoulé depuis la dernière frame, en secondes.
 * @return true si l'entité a changé de position.
 */
bool updatePhysics(Entity& entity, float deltaTime);
```

**Définition (`Core.cpp`) — commentaires simples `//`, non exportés**
```cpp
// Met à jour la physique d'une entité pour une frame.
bool updatePhysics(Entity& entity, float deltaTime) {
    const Vector2 ancientPosition = entity.position();

    // Intégration semi-implicite : la vitesse est mise à jour avant la position
    // pour un comportement stable sous gravité constante.
    entity.applyGravity(deltaTime);
    entity.move(entity.velocity() * deltaTime);

    // L'entité est considérée « au sol » uniquement si un contact est détecté
    // sous ses pieds : sinon la gravité continue de s'appliquer à la frame suivante.
    if (entity.detectGroundContact()) {
        entity.resetVerticalVelocity();
    }

    return entity.position() != ancientPosition;
}
```

> Une méthode qui **redéfinit** (`override`) une fonction déjà documentée dans un header (par
> ex. une interface) n'a pas besoin d'être redocumentée : Doxygen hérite de la doc de base.
> Les fonctions **internes** au `.cpp` (espace de noms anonyme) se contentent aussi de `//`.

### Documentation du corps (.cpp)
Dans les définitions, commenter en français avec des `//` :
- Le rôle de chaque **branche `if`/`else`** non triviale (pourquoi cette condition).
- Le principe de tout **algorithme complexe** (intention, invariants, cas limites), pas la paraphrase ligne à ligne.
- Les choix non évidents (pourquoi telle formule, telle borne, telle optimisation).

Balises Doxygen (dans les **en-têtes** uniquement) : `@file`, `@brief`, `@param`, `@return`,
`@note`, `@warning`, `@see`.

Commentaire de **membre bref** (dans le `.h`) — deux formes acceptées, à choisir selon la
lisibilité, une seule par déclaration :
- `/// description` **au-dessus** de la déclaration : pour une description autonome, souvent plus
  longue (membres de `struct`) ;
- `///< description` **après** la déclaration (même ligne) : pour une annotation compacte, idéale
  sur des listes serrées (énumérateurs, champs privés courts).

```cpp
struct Transform {
    /// Position du repère de l'entité, en unités monde (origine haut-gauche, Y-bas).
    Vector2 position{};
};

enum class LogLevel {
    Trace,  ///< Détails fins de débogage.
    Info,   ///< Information de fonctionnement normal.
};
```

Ne **jamais** utiliser un `///` de fin de ligne (sans `<`) : il se rattacherait à la déclaration
*suivante*, pas à celle qu'on croit documenter.

## 7. Bonnes pratiques
- `const` par défaut ; `constexpr` quand c'est possible.
- Passage par référence (`const T&`) pour les objets non triviaux.
- RAII et pointeurs intelligents (`std::unique_ptr`) ; éviter `new`/`delete` bruts.
- Initialiser toutes les variables. Préférer l'initialisation par accolades `{}`.
- Pas de nombres magiques : constantes nommées.
- **`[[nodiscard]]`** sur toute fonction dont ignorer le résultat serait une erreur (getters, valeurs de statut).
- **`noexcept`** sur les fonctions qui ne lèvent pas d'exception (obligatoire sur les destructeurs et les opérations de déplacement).
- **`enum class`** plutôt qu'`enum` nu (typage fort, pas de conversion implicite).
- **`override`** / **`final`** explicites sur les méthodes virtuelles redéfinies.

## 8. Tests
- Un test = un comportement, nommé `TEST(SuiteName, ComportementTeste)`.
- Fichiers de test : `test_<cible>.cpp` dans `Source/Test/{Unit,Integration,Systeme}/`.
- Toute logique de `Core` livrée s'accompagne de tests unitaires.

## 9. Gestion des erreurs
Politique par catégorie d'erreur (à respecter dans tout le code) :

| Nature | Traitement |
|--------|-----------|
| **Erreur de programmation** (précondition violée, invariant rompu, index hors bornes) | `assert` (voir §10). Non récupérable : le code appelant est fautif et doit être corrigé. |
| **Erreur récupérable attendue** (fichier absent, ressource introuvable, entrée invalide) | Valeur de retour explicite : `std::optional<T>` pour une simple absence, ou un type `Result<T>` / code d'erreur pour distinguer les causes. |
| **Erreur d'initialisation irrécupérable** (démarrage : device DirectX, fenêtre) | Exception, capturée uniquement à la frontière de démarrage. |

Règles :
- **Pas d'exception dans la boucle de jeu** ni dans les chemins critiques de performance.
- Jamais d'exception qui traverse un destructeur (destructeurs `noexcept`).
- Ne jamais ignorer silencieusement un `Result`/`optional` d'erreur (cf. `[[nodiscard]]`).

> Note : `std::expected` (C++23) remplacera avantageusement `Result<T>` lorsque le projet passera à C++23.

## 10. Assertions & journalisation
- **Assertions** : vérifier les préconditions et invariants avec une macro projet `JADG_ASSERT(condition, message)` (basée sur `assert`, active en Debug, retirée en Release). Une assertion signale un **bug**, pas une erreur d'exécution normale.
- **Journalisation** : passer par le module de log du projet (`Core/Diagnostics`, dont le journal de fichier tient `EX-NFR-042`), **jamais** `std::cout`/`printf` directement dans le code de production. Niveaux : `trace`, `info`, `warning`, `error`.

## 11. Outillage qualité (automatisé)
Ces règles — le guide lui-même est la référence d'`EX-NFR-012` — sont appliquées par des outils,
pas seulement par relecture ; la construction passe exclusivement par CMake (`EX-NFR-030`) :

| Outil | Fichier | Rôle | Où il s'exécute |
|-------|---------|------|------------------|
| **clang-format** | `.clang-format` | Formatage automatique (indentation, accolades, ordre des `#include`, `EX-NFR-024`). À exécuter avant chaque commit ; VS l'applique nativement. | Job `format` de `ci.yml` (`--dry-run --Werror`, version LLVM épinglée), sur chaque PR — **bloquant**. |
| **clang-tidy** | `.clang-tidy` | Analyse statique + vérification des règles de nommage (§2, `EX-NFR-024`). | Job `clang-tidy` de `ci.yml`, sur le diff de chaque PR — `bugprone-*` **bloquant** (ramené à zéro) ; `cppcoreguidelines-*`/`modernize-*`/`performance-*`/`readability-*` consignés, non bloquants (triage complet hors périmètre du lot). |
| **EditorConfig** | `.editorconfig` | Cohérence d'édition (encodage, fins de ligne, indentation) entre postes et éditeurs. | Appliqué par l'éditeur, non vérifié en CI. |
| **Avertissements compilateur** | `CMakeLists.txt` | `/W4 /WX` (MSVC) : avertissements au niveau élevé, **traités comme des erreurs** (`EX-NFR-013`). Bloquant en CI. | Jobs `build-test-coverage`, `build-test-release` et `build-ninja` de `ci.yml`, sur chaque PR — Debug **et** Release (`build-test-release`, `EX-NFR-023`). |
| **AddressSanitizer** | option `ENABLE_ASAN` | Détection à l'exécution des débordements et usages après libération (`EX-NFR-003`). Activable en Debug : `cmake --preset ninja -DENABLE_ASAN=ON`. | Job `sanitize` de `ci.yml`, sur les trois exécutables de test, à chaque PR — bloquant. |

Le code livré compile **sans aucun avertissement**. Un avertissement légitime et inévitable est neutralisé localement et commenté (jamais désactivé globalement).

## 12. Identifiants d'exigences (EX-…)
Les exigences sont identifiées par un code `EX-<CAT>-<NNN>` (ex. `EX-ARCH-011`),
déclaré dans les [spécifications](README.md) et référencé depuis les
lots **et** le code. Ces identifiants sont **stables** : ils servent de handles
permanents.

Règles :
- **Immuable** : un identifiant est alloué une seule fois, **jamais renuméroté**,
  **jamais réutilisé** (le renuméroter casserait toutes les références).
- **Allocation au prochain numéro libre** de la catégorie. Le numéro **n'encode
  pas l'ordre** : l'ordre de lecture est celui du document, pas celui des numéros.
- **Insérer une exigence** = écrire son paragraphe là où il doit être + lui donner
  un numéro libre. Aucune renumérotation.

Mise en œuvre :
- Chaque exigence est déclarée par une **puce qui s'ouvre sur son identifiant en gras** :
  `- **EX-CAT-NNN** — …`. Le site en fait une ancre : elle se cite par un lien
  (`[EX-CAT-NNN](combat.md#EX-CAT-NNN)`) ou, plus simplement, en code (`` `EX-CAT-NNN` ``), que le
  site relie de lui-même à la déclaration. Un lien cassé fait échouer `lint_docs.py`.
- Le script `scripts/checks/lint_exigences.py` vérifie en CI l'**unicité** des
  déclarations et l'absence de **référence orpheline** ; `python
  scripts/checks/lint_exigences.py --next` affiche le prochain numéro libre par catégorie.
