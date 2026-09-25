# Source/HMI/

Couche de **présentation** : ce que le jeu (`JustAnotherRpgGame`, Qt Quick, point d'entrée dans
`../App/Game/`) et l'éditeur de cartes partagent — rendu via **QRhi**, entrées, traduction. La
partie sans Qt Widgets est la bibliothèque statique `HmiLib`, que consomment aussi les tests (voir
`CMakeLists.txt`). L'éditeur lui-même est un module à part, [`../Editor/`](../Editor/README.md)
(`LOT-EDITOR-01`) : il dépend de ce dossier, et rien ici ne dépend de lui.

Ce dossier dépend de `../Core/` pour l'état à afficher, mais ne contient pas la logique de jeu
elle-même. Les assets vivent dans [`../Elements/`](../Elements/README.md) ; les écrans du jeu (QML)
dans `../Ui/`.

## Découpage par domaine

| Dossier | Rôle |
|---|---|
| [`Platform/`](Platform/README.md)   | Provisionnement bas niveau (répertoire de l'exécutable, minidump). |
| [`Input/`](Input/README.md)         | Entrées : état, manette, pont Qt→`Key`. |
| [`Graphics/`](Graphics/README.md)   | Rendu via **QRhi** (pipeline 2D, caméra, lieu, combat sur la carte). |
| [`Game/`](Game/README.md)           | La carte qu'on parcourt (`WorldPlay`), partagée par le jeu et l'essai de l'éditeur. |
| `Presentation/` | Logique de présentation pure (enchaînement des écrans, échelle, valeurs de fiche, crédits). |
| `Runtime/`   | Les types C++ que les écrans du jeu voient : module QML `Jadg.Runtime` (vues-modèles, surfaces de rendu). |
| [`Localization/`](Localization/README.md) | Catalogue de traduction. |
| [`Audio/`](Audio/README.md)         | Moteur de lecture et volume ; aucun son livré. |

## Build & déploiement

Cible Qt **optionnelle** : sans Qt, la configuration CMake n'échoue pas (les tests unitaires
compilent les sources pures directement). `windeployqt` copie les DLL Qt, le plugin de plateforme et
le runtime du compilateur à côté de l'exécutable — **aucune bibliothèque à installer** côté
utilisateur. Provisionnement de Qt : [`../../External/README.md`](../../External/README.md).

Guide détaillé : [`guide-ihm-qt`](../../Documentation/Guide/guide-ihm-qt.md).
