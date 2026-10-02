# Entrées et actions logiques

Le jeu se pilote au clavier et à la souris. Les écrans Qt Quick lisent directement leurs événements
de touches, de pointeur et de molette. L'éditeur garde une table de traduction distincte pour ses
raccourcis reconfigurables.

## De la touche à l'intention

Le gameplay ne dépend pas d'une touche physique (`EX-CTRL-010`). Dans l'exploration, les flèches,
ZQSD et WASD sont traduits en une direction normalisée. `E` et `Espace` produisent la même demande
d'interaction (`EX-CTRL-022`). `core::ExplorationSession` ne reçoit que cette intention :

- `move` : un `core::Vector2` de longueur au plus 1 ; deux directions opposées se neutralisent ;
- `interact` : vrai pendant le pas où le joueur demande l'interaction, puis remis à zéro.

`Source/App/Game/Qml/Screens/GameView.qml` retient les touches enfoncées et transmet la direction
à `hmi::WorldModel::setMove`. La vue d'essai de l'éditeur construit la même intention. La demande
d'interaction est consommée au pas suivant (`EX-CTRL-020`).

La perte de focus libère les directions maintenues : un retour depuis une autre fenêtre ne doit
pas faire avancer le personnage tout seul.

## Clavier de l'éditeur

`HMI/Input/Key.h` nomme les touches utiles par leur code virtuel Win32. Le jeu Qt Quick ne passe
pas par cette énumération. `HMI/Input/QtKeyMap.h` traduit entre `Qt::Key` et `hmi::Key` pour les
raccourcis de l'éditeur. Les lettres et chiffres partagent leur code ; les touches spéciales
(flèches, Échap, Tab, Maj, Ctrl, F1, F2, F10) ont une correspondance explicite.

`hmi::EditorKeyBindings` charge `Settings/keybindings.json`. Un fichier absent ou incomplet retombe
sur les raccourcis par défaut (`EX-CTRL-012`). Les remappages restent hors de `Core`.

## Parcours des écrans

Les contrôles QML exposent leurs actions par des signaux que les jumeaux de `Jadg.App` relient au
routeur ou aux vues modèles. Les marques de focus restent visibles au clavier (`EX-IHM-071`). La
carte se parcourt aussi à la souris : clic sur un repère, glisser pour déplacer, molette pour
agrandir. Les touches et gestes de chaque écran sont détaillés dans le [manuel de jeu](Manuel/jouer.md).

Le trajet reste : **événement Qt → intention ou commande nommée → modèle de jeu → Core**. Cette
frontière permet de tester la logique sans fenêtre ni entrée physique (`EX-NFR-010`).
