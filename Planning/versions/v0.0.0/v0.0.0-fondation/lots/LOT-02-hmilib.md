+++
id = "LOT-02"
titre = "Bibliothèque `HmiLib`"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "S"
resume = "La couche de présentation sans Qt Widgets devient une bibliothèque liée par l'application comme par les tests, et ses sources ne se relistent plus à deux endroits."
prerequis = ["LOT-01"]
livrables = [
  "`add_library(HmiLib STATIC …)` dans `Source/HMI/CMakeLists.txt`, avec les 80 sources sans Qt, déclarée **avant** `option(BUILD_EDITOR_QT)`.",
  "L'exécutable perd ces 80 sources de sa liste et lie `HmiLib`.",
  "`Source/Test/CMakeLists.txt` perd les 79 chemins `../HMI/…` relistés et lie `HmiLib` ; seules les quatre sources dépendantes de Qt restent conditionnelles.",
  "Exigences `EX-ARCH-*` et `EX-BUILD-*` nouvelles sur la bibliothèque de présentation sans Qt Widgets.",
]
criteres = [
  "`Source/Test/CMakeLists.txt` ne contient plus que les **quatre** chemins `../HMI/…` conditionnels à Qt.",
  "Build complet sans avertissement en `/W4 /WX` ; tests intégralement verts, **sans modification du corps d'un seul test** (refactoring à comportement constant).",
  "`cmake --preset vs -DBUILD_EDITOR_QT=OFF` configure et construit les tests seuls.",
]
+++

## Pourquoi

Faire de la couche de présentation **sans Qt Widgets** une bibliothèque, liée par l'application
comme par les tests, pour cesser de relister ses sources à deux endroits.

### Le problème

`Source/HMI` était un `add_executable`. Conséquence : `Source/Test/CMakeLists.txt` relistait
**79 chemins `../HMI/…`** pour recompiler ces sources dans `UnitTests`. Chaque fichier ajouté à
l'IHM devait l'être **deux fois**, et un oubli ne se voyait qu'à l'édition de liens. Le RPG va
ajouter des dizaines de fichiers (règles, dialogues, inventaire, IHM de combat) : la dette doublait
à chaque lot.

## Périmètre

- `add_library(HmiLib STATIC …)` avec les 80 sources sans Qt (`Graphics/` sans GPU, `Input/` sans
  le pont Qt, `Localization/`, `Diagnostics/`, `Audio/` hors moteur, `Game/` hors viewport,
  `Editor/` logique pure, `Interface/` hors widgets, `Platform/`), déclarée **avant**
  `option(BUILD_EDITOR_QT)`.
- Racine d'inclusion `Source/`, `Core` et `nlohmann_json` propagés en `PUBLIC` : les consommateurs
  n'ont plus à les redemander. `project_warnings`/`project_options` restent `PRIVATE`.
- L'exécutable perd ces 80 sources de sa liste et lie `HmiLib`.
- `UnitTests` perd les 79 chemins relistés et lie `HmiLib`. Les **quatre** sources qui dépendent de
  Qt (`QtKeyMap`, `TextureLoader`, `SpriteBatch`, `AudioEngine`) restent ajoutées
  conditionnellement, comme aujourd'hui.

## Conception

### Le découpage retenu — et pourquoi il n'est pas « une seule `HmiLib` »

Le réflexe est de mettre **tout** `HMI` dans une bibliothèque. C'est faux ici, pour deux raisons
que le projet maintenait déjà à la main dans des commentaires :

1. **`UnitTests` ne lie pas `Qt6::Widgets`.** Il compile délibérément un sous-ensemble de logique
   pure (géométrie de caméra, composition de scène sans GPU, catalogues, gestes d'éditeur) et
   n'instancie jamais de `QApplication`. Une bibliothèque monolithique le forcerait à lier toute
   l'IHM.
2. **`Source/HMI/CMakeLists.txt` sort tôt (`return()`) sans Qt**, et c'est délibéré :
   `-DBUILD_EDITOR_QT=OFF` doit continuer de construire les tests seuls. Une bibliothèque déclarée
   après cette garde n'existerait pas dans ce cas.

D'où : **`HmiLib` porte le sous-ensemble sans Qt**, et est déclarée **avant** la garde. Ce n'est
pas qu'une déduplication — cela **nomme** une frontière que le projet documentait jusqu'ici en
prose, fichier par fichier.

## Risques et questions ouvertes

Points de vigilance :

- **AUTOMOC** : `set(CMAKE_AUTOMOC ON)` intervient plus bas dans le fichier ; `HmiLib` est donc
  compilée sans moc. C'est correct — aucune de ces sources n'a de `Q_OBJECT` (c'est précisément ce
  qui les rendait compilables dans les tests). Toute source ajoutée à `HmiLib` doit respecter cette
  règle.
- **Ressources Qt** (`qt6_add_shaders`, `.ui`) : elles restent sur l'exécutable. Les embarquer dans
  une bibliothèque **statique** exposerait au rejet des objets par l'éditeur de liens (une
  ressource que rien ne référence est écartée) — problème classique, inutile de l'affronter ici.
- Ne pas transformer `HmiLib` en fourre-tout : un fichier qui a besoin de `QWidget` appartient à
  l'exécutable, pas à la bibliothèque.

## Exigences couvertes

`EX-ARCH-*` et `EX-BUILD-*` nouvelles : « la couche de présentation sans Qt Widgets est une
bibliothèque, liée par l'application et par les tests ».

## Bilan

Statut écrit dans l'epic d'origine : **en cours** — l'epic n'a jamais été mis à jour. La
bibliothèque `HmiLib` existe bien dans `Source/HMI/CMakeLists.txt` et tous les lots suivants la
lient : le lot est tenu pour livré.
