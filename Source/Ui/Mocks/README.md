# Source/Ui/Mocks/

Les **doublures** des types C++ de `Jadg.Runtime`, pour Qt Design Studio (LOT-87).

L'atelier dessine avec son propre Qt et un marionnettiste (`qmlpuppet`) qui ne charge aucun plugin
C++ du projet : `OptionsModel`, `ScreenRouter`, `PendingData`, `CharacterSheetModel`,
`InventoryModel` et `GameViewport` n'y existent pas. Ce dossier en fournit une version QML aux
**mêmes noms et mêmes propriétés**, que `JadgUi.qmlproject` place dans ses `importPaths`. C'est ce
qui permet d'ouvrir les **jumeaux** de câblage (`Source/App/Game/Qml/Screens/*.qml`) dans l'atelier,
et pas seulement les formulaires.

Le jeu ne les voit jamais : CMake ne connaît pas ce dossier, et `Jadg.Runtime` y est le vrai module.

Règle de maintenance : une propriété ajoutée à un type de `Source/HMI/Runtime/` s'ajoute à sa
doublure. `scripts/checks/check_qml_designer_compat.py` vérifie que chaque type nommé par un jumeau existe
ici, et que chaque propriété `Q_PROPERTY` et chaque `Q_INVOKABLE` du C++ a son pendant.
