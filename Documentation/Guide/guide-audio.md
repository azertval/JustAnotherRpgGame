# Audio

Cette page explique comment le jeu produit du son : le socle de lecture (`hmi::AudioEngine`), le
réglage de volume qui le pilote depuis l'écran des options, et le provisionnement de Qt
Multimedia. Le jeu ne livre aujourd'hui **aucun bruitage** : le socle existe, le volume l'atteint,
mais aucun son n'est encore préchargé ni joué.

## La règle d'or, une fois de plus

`Core` **expose des transitions d'état** ; c'est `HMI` qui décide qu'une transition fait du bruit.
Exactement la même séparation que pour le rendu ([Rendu 2D : de la scène à l'écran](guide-rendu.md)) — et pour la même raison : la
simulation reste pure, déterministe et testable **sans périphérique audio** (`EX-NFR-010`,
`EX-ARCH-012`, `EX-REN-047`). Aucun fichier de `Core/` n'inclut Qt, ni ne sait qu'un son existe.

## Le socle : `hmi::AudioEngine`

`Source/HMI/Audio/AudioEngine.h` enveloppe `QSoundEffect` (Qt Multimedia), le composant Qt conçu
précisément pour des échantillons courts à faible latence — le cas d'usage exact d'un bruitage,
par opposition à `QMediaPlayer` (pensé pour la musique, latence et coût plus élevés).

- **Détection du périphérique** à la construction (`QMediaDevices::defaultAudioOutput()`) : absent
  → le moteur passe en état **muet**, journalise un avertissement une seule fois, et **toute**
  demande de lecture ultérieure ne fait rien. Un constructeur dédié (`ForceMuted::Yes`) force cet
  état sans dépendre du matériel réel de la machine qui exécute les tests.
- **Préchargement obligatoire.** `QSoundEffect` charge son fichier de façon **asynchrone** : jouer
  immédiatement après construction ne produit rien. `AudioEngine::preload(id, fichier)` doit donc
  être appelé au démarrage pour chaque son — jamais au premier déclenchement, sous peine d'une
  première occurrence silencieuse (défaut difficile à attribuer, puisque les suivantes
  fonctionnent).
- **Tourniquet d'instances.** `preload` prépare en réalité `MAX_INSTANCES_PER_EVENT` (3)
  `QSoundEffect` identiques par identifiant, et `play` les consomme en rotation : un même son
  déclenché en rafale se recouvre sans s'interrompre lui-même de façon audible, sans empiler
  indéfiniment des lectures superposées.
- **Volume borné** à `[0, 1]`, appliqué à tous les échantillons préchargés.

## Le volume : de l'écran des options au moteur

Le moteur vit dans l'application du jeu (`Source/App/Game/Main.cpp`), pas dans la vue-modèle des
options : ce n'est pas à un écran de réglages de posséder le son du jeu. `connectOptions` lit le
volume de `hmi::OptionsModel` au démarrage, puis le réapplique à chaque `volumeChanged` — le
curseur de l'écran des options atteint donc réellement le moteur (`EX-REN-048`, `EX-IHM-083`),
même tant qu'aucun son n'est joué.

## Provisionnement : Qt Multimedia

`Qt6::Multimedia` est un **composant additionnel** de Qt, pas une bibliothèque tierce : il figure
dans `find_package(Qt6 ... COMPONENTS ... Multimedia ...)` de `Source/CMakeLists.txt`, avec la
même garde que `Widgets`/`Gui` (absent → cibles Qt ignorées, configuration jamais cassée,
`EX-BUILD-010`). Provisionné en CI par `modules: qtmultimedia` dans l'action
`.github/actions/setup-qt` ; en local, via le composant `Multimedia` de l'installateur Qt officiel
ou `aqtinstall -m qtmultimedia`.

`windeployqt` (`POST_BUILD`) déploie `Qt6Multimedia.dll` et ses greffons (`multimedia/`) à côté de
l'exécutable — vérifié explicitement en CI (`ci.yml`), puisqu'un build local réussi ne prouve rien
sur le zip publié.

## Voir aussi
- `hmi::AudioEngine`, `hmi::OptionsModel`.
- [Rendu 2D : de la scène à l'écran](guide-rendu.md) — la même séparation `Core`/`HMI` appliquée à l'image.
- [Écrans, navigation et boucle de jeu](guide-ecrans.md) — l'écran des options, d'où vient le volume.
