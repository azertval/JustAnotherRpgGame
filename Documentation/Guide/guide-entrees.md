# Entrées et actions logiques

Cette page explique comment une touche physique devient un déplacement du personnage, et pourquoi
ce trajet passe par une étape intermédiaire qui, au premier abord, peut sembler superflue.

## Le principe : ne jamais coder « en dur » une touche dans le gameplay

Le gameplay ne dépend **jamais** directement d'une touche physique (`EX-CTRL-010`). Autrement dit,
`core::ExplorationSession` ne contient **aucun** test du genre « si la touche flèche gauche est
enfoncée ». À la place, les entrées brutes (clavier, manette) sont d'abord traduites en une
**intention** neutre — « aller vers le nord-ouest », « interagir » — que `Core` consomme sans jamais
savoir **quelle touche** a produit cette intention.

**Pourquoi cette indirection.** Sans elle, chaque logique qui a besoin d'une entrée devrait
connaître les touches physiques, ce qui pose plusieurs problèmes concrets :

- **plusieurs touches, une commande** — les flèches, `ZQSD` et `WASD` déplacent tous le héros, `E`
  et `Espace` interagissent tous deux (`EX-CTRL-022`) ; un seul point de traduction évite de répéter
  cette correspondance dans chaque logique ;
- **tests plus difficiles** — tester l'exploration obligerait à simuler de vrais événements clavier
  plutôt qu'à construire directement une intention `{ .move = {1, 0}, .interact = true }` ;
- **couplage entre `Core` et `HMI`** — `Core` n'a **aucune** dépendance à la fenêtre ni à Qt
  (`EX-NFR-010`) ; lui faire connaître des codes de touche briserait cette frontière
  architecturale.

Le trajet complet est donc : **touche physique → événement Qt → traduction (`HMI` ou jumeau QML) →
`core::ExplorationIntent` (intention) → `Core` (logique de jeu)**. Les premières étapes vivent du
côté de la présentation, la dernière dans `Core`, indépendante.

## L'intention : `core::ExplorationIntent`

`core::ExplorationIntent` est le **contrat** de données entre la présentation et l'exploration :

- `move` : la direction voulue, un `core::Vector2` de longueur **au plus 1** — déjà normalisée, pour
  qu'une diagonale n'aille pas plus vite qu'une ligne droite ([Mathématiques du moteur](guide-maths.md)). Deux touches
  opposées enfoncées ensemble **se neutralisent** plutôt que de privilégier arbitrairement l'une ;
- `interact` : vrai **le pas** où le joueur demande l'interaction — un appui ponctuel, pas un état
  maintenu.

`Core` ne reçoit **que** cette structure : il ignore totalement l'existence des touches et boutons
physiques. Deux traducteurs la construisent, un par application :

- dans le **jeu**, l'écran d'exploration (`Source/App/Game/Qml/Screens/GameView.qml`) tient les
  directions enfoncées (flèches, `Z`/`W`, `Q`/`A`, `S`, `D`), en compose la direction normalisée et
  la pousse à `hmi::WorldModel::setMove` à chaque appui ou relâchement ; `E`/`Espace` appellent
  `WorldModel::interact`, `Échap` ouvre la pause. `WorldModel` range l'intention et la remet à la
  session au pas suivant ([Boucle de jeu et pas de temps fixe](guide-boucle.md)) ;
- dans l'**essai immédiat** de l'éditeur, `hmi::EditorViewport` retient les touches enfoncées
  (mêmes touches que le jeu) et compose la même direction ; `E`/`Espace` lèvent une demande
  d'interaction, `Échap` arrête l'essai.

Dans les deux cas, la demande d'interaction est **consommée** par le premier pas qui la lit, puis
remise à zéro. Un appui ne se perd donc jamais entre deux pas, et ne se répète pas non plus : la
latence entrée → effet reste bornée à **un pas** (`EX-CTRL-020`).

**Perte de focus.** À un `Alt+Tab` (ou tout basculement de fenêtre), la vue ne reçoit **pas**
d'événement de relâchement pour les touches maintenues. Sans précaution, une direction maintenue
resterait « collée » et le personnage avancerait seul au retour. `hmi::EditorViewport` traite donc
`QEvent::FocusOut` en oubliant toutes les touches tenues.

## Échantillonner plutôt que réagir : `hmi::InputState`

Deux façons classiques d'observer les entrées existent :

- **piloté par les événements** : le code réagit immédiatement à chaque message du système
  d'exploitation (« touche enfoncée », « touche relâchée ») dès qu'il arrive, à un instant
  arbitraire ;
- **échantillonné** (*polling*) : le code lit un **état courant**, mis à jour à intervalle régulier,
  à un instant **prévisible**.

Le clavier arrive par événements Qt ; la **manette**, elle, n'en produit pas : il faut la sonder.
`hmi::InputState` est l'état échantillonné qui reçoit ces relevés — indépendant de toute fenêtre
(aucune dépendance `<Windows.h>` ni Qt dans son en-tête), si bien que les tests y injectent
directement touches et boutons, sans manette réelle (`EX-NFR-010`).

### Détecter les fronts, pas seulement l'état

Savoir qu'un bouton est enfoncé ne suffit pas : une interface doit distinguer *plusieurs* questions
liées mais différentes (`EX-CTRL-011`) :

- « le bouton **vient-il d'être pressé** ? » (le **front montant**) — valide un choix, une seule fois
  par appui ;
- « le bouton est-il **maintenu** ? » — fait défiler une liste tant qu'on tient la croix ;
- « le bouton **vient-il d'être relâché** ? » (le **front descendant**).

`InputState` calcule ces trois fronts en gardant **deux instantanés** : l'état **courant** et celui
du relevé **précédent**. Un bouton est « pressé » (front montant) précisément quand il est enfoncé
maintenant mais ne l'était **pas** au relevé d'avant ; de même, « relâché » (front descendant) quand
il ne l'est plus mais l'était juste avant. Sans conserver cet historique, on ne pourrait connaître
que l'état courant (`keyDown`), jamais le moment précis de transition (`keyPressed`/`keyReleased`) —
pourtant essentiel aux actions **ponctuelles**, à déclencher une seule fois par appui.

```cpp
// keyDown : vraie si enfoncée maintenant, quelle que soit la source (clavier OU manette).
bool InputState::keyDown(Key key) const noexcept {
    return _keysCurrent[index] || _gamepadCurrent[index];
}

// keyPressed (front montant) : enfoncée maintenant, mais ne l'était sur AUCUNE source
// au relevé précédent.
bool InputState::keyPressed(Key key) const noexcept {
    const bool keyboardEdge = _keysCurrent[index] && !_keysPrevious[index];
    const bool gamepadEdge = _gamepadCurrent[index] && !_gamepadPrevious[index];
    return keyboardEdge || gamepadEdge;
}
```

Remarque sur la fusion manette (détaillée plus bas) : `keyPressed` calcule un front **par source**
puis les combine par OU logique — un bouton manette pressé alors que la touche clavier équivalente
était déjà maintenue produit bien un nouveau front (celui de la manette), sans que le clavier
« masque » cette pression.

### Le cycle d'un relevé

1. `hmi::InputState::beginFrame()` recopie l'état **courant** vers l'état **précédent**, ouvrant la
   fenêtre d'observation du relevé ;
2. les sources mettent à jour l'état courant (`onKeyDown`/`onKeyUp`, `onGamepadKeyDown`/…,
   `onGamepadButtonDown`/…) ;
3. le lecteur interroge les fronts et l'état (`keyPressed`, `gamepadButtonPressed`, …).

`InputState::releaseAll()` remet à zéro l'état courant **et** précédent de toutes les touches et de
tous les boutons — sans produire de front « relâché » parasite (courant == précédent == relâché).
C'est ce qu'appelle un lecteur qui cesse d'écouter.

## La manette : une seconde source, fusionnée en lecture (EX-CTRL-002)

`InputState` ne connaît qu'un seul `hmi::Key` par touche, mais **deux** sources indépendantes qui
peuvent l'enfoncer : le clavier (`onKeyDown`/`onKeyUp`) et la manette (`onGamepadKeyDown`/
`onGamepadKeyUp`), chacune avec sa propre paire courant/précédent. `keyDown`/`keyPressed`/
`keyReleased` **combinent** les deux (OU logique) au moment de la lecture — jamais à l'écriture.

**Pourquoi pas une seule table partagée ?** Parce que la manette est **sondée**, pas événementielle
: `hmi::GamepadPoller::poll` interroge XInput à chaque relevé et doit explicitement relâcher
(`onGamepadKeyUp`) chaque touche dont le bouton correspondant n'est plus enfoncé — y compris quand
la manette est débranchée. Si ce relâchement écrivait dans la **même** table que le clavier, il
effacerait une touche clavier réellement maintenue dès que la manette (absente ou relâchée) ne la
tient plus. Deux tables, combinées seulement en lecture, rendent ce bug structurellement
impossible plutôt que de compter sur la discipline du code appelant.

Chaque direction manette synthétise le **même** `Key` fixe que son équivalent clavier (D-pad et
stick gauche → `Left`/`Right`/`Up`/`Down` ; **A** → `Enter` **et** `Space` ; **B**/**Start** →
`Escape`) — câblage en dur dans `hmi::GamepadPoller::poll`.

**Une seconde piste, brute, par bouton.** À partir du **même** relevé XInput, `GamepadPoller::poll`
alimente aussi un état par `hmi::GamepadButton` (`onGamepadButtonDown`/`onGamepadButtonUp`,
`InputState::gamepadButtonDown`/`gamepadButtonPressed`) — dix boutons et directions (D-pad et stick
gauche fusionnés en une seule notion par direction). C'est cette piste que lit le jeu.

**Dans le jeu : `hmi::GamepadNavigator`.** Qt 6 n'a plus de module
manette ; le jeu Qt Quick passe donc par ce pont. Tant qu'il est `active`, il sonde la manette
soixante fois par seconde (`beginFrame`, puis `GamepadPoller::poll`) et émet `pressed` avec le nom
du bouton (`up`, `down`, `left`, `right`, `a`, `b`, `x`, `y`, `lb`, `rb`). La croix et le stick se
répètent quand on les tient (`hmi::ButtonRepeat`) ; les autres boutons n'émettent qu'à l'appui. Il
ne sait rien de ce que l'écran en fait : c'est le jumeau QML de l'écran (le Colisée, la carte du
monde) qui traduit un nom en geste, comme il traduit une touche. Inactif, il ne sonde pas, et
relâche tout (`releaseAll`).

Le sondage XInput lui-même (`<Xinput.h>`) vit dans `hmi::GamepadPoller` (`Input`), jamais dans
`InputState` : `InputState` reste indépendant de toute fenêtre (`EX-NFR-010`), y compris pour
tester la fusion manette ou la piste brute — les tests appellent `onGamepadKeyDown`/
`onGamepadKeyUp`/`onGamepadButtonDown`/`onGamepadButtonUp` directement.

**Sondage espacé quand aucune manette n'est branchée.** `XInputGetState` est notablement
**coûteux** quand le slot interrogé est vide : le pilote énumère les périphériques à chaque appel.
Le sonder à chaque relevé alors qu'aucune manette n'est connectée provoque des micro-saccades chez
un joueur clavier. `hmi::GamepadPoller::poll` ne re-sonde donc un slot resté **déconnecté** qu'une
fois par `GAMEPAD_DISCONNECTED_PROBE_PERIOD` (2 s), décompté en temps réel (`hmi::gamepadProbeDue`)
et non en nombre d'appels ; entre-temps l'état reste « déconnecté ». Dès qu'une manette est
présente, le sondage redevient systématique — un branchement à chaud est détecté au plus tard après
une période.

## Le menu d'options

La page Options du jeu est en QML (`Source/Ui/Screens/OptionsForm.ui.qml`, câblée par
`Source/App/Game/Qml/Screens/Options.qml`) ; elle lit et écrit `hmi::OptionsModel`, qui persiste
chaque réglage : plein écran, **V-Sync** (`EX-REN-022`), volume, **langue**, journaux de
diagnostic. Le jeu ne propose pas de remappage des touches.

## Les raccourcis de l'éditeur : `hmi::EditorKeyBindings`

Les raccourcis de l'éditeur sont **reconfigurables par fichier** (`EX-CTRL-012`) :
`hmi::EditorKeyBindings` associe chaque `hmi::EditorAction` (Sauvegarder, Annuler, Refaire, Copier,
Coller, Essai, Grille, Aide, Renommer) à une touche, lue au démarrage depuis
`Settings/keybindings.json` (section `"editeur"`, à côté de l'exécutable). `hmi::EditorViewport` la
charge ; `hmi::EditorActions::applyShortcuts` en fait les raccourcis des actions Qt du menu et de la
barre d'outils — une seule source, jamais un second traitement dans la vue.

- Le fichier stocke le code VK **brut** de la touche (`hmi::Key` en est déjà un : ses valeurs
  coïncident avec les codes virtuels Windows ; `hmi::hmiKeyToQtKey` fait la conversion vers Qt).
- `setKey` **échange** avec toute autre action déjà sur cette touche plutôt que de dupliquer :
  jamais deux actions sur la même touche.
- Le modificateur `Ctrl` de Sauvegarder/Annuler/Refaire/Copier/Coller reste câblé en dur ; seule la
  touche-lettre est remappable. Certains gestes (`Suppr` de l'outil Entité, recadrage `0`) restent
  câblés en dur.
- Un fichier absent, corrompu, ou une entrée invalide retombe sur les valeurs par défaut pour
  l'entrée concernée (`EX-NFR-040`), jamais bloquant. `save` relit le fichier existant pour
  préserver les autres sections plutôt que de les écraser.

## La langue de l'interface : `hmi::Localization`

Tous les textes d'interface de l'éditeur passent par une **clé** stable (`EX-REN-033`) plutôt que
par un libellé en dur : `hmi::Localization` résout une clé (« action.copy ») vers la chaîne de la
**langue active**, chargée depuis un fichier `<langue>.lang` (format `clé = valeur`, un fichier
par langue dans `Source/Elements/Localization/`). La résolution suit un **repli déterministe** —
langue active, puis langue par défaut, puis la clé elle-même — pour ne jamais planter ni afficher
un vide si une traduction manque (`EX-NFR-040`), au prix, dans ce dernier cas, d'un texte
visiblement « brut » (la clé) plutôt qu'un texte manquant silencieux.

`hmi::MainWindow` détient l'unique `hmi::Localization` de l'éditeur : il charge le français par
défaut, puis la langue enregistrée (`QSettings`, clé `language`) si elle diffère, et propage un
`retranslateUi()` à chaque widget (docks, palette, navigateur, actions). Un échec de chargement
(fichier absent) est **récupérable** : la langue courante est simplement conservée. Le jeu, lui,
traduit ses écrans QML par le mécanisme de Qt (`qsTr`) ; sa langue se choisit dans Options.

## Voir aussi
- `core::ExplorationIntent`, `hmi::WorldModel`, `hmi::EditorViewport`.
- `hmi::InputState`, `hmi::Key`, `hmi::GamepadButton`, `hmi::GamepadPoller`,
  `hmi::GamepadNavigator`, `hmi::ButtonRepeat`.
- `Source/Ui/Screens/MainMenuForm.ui.qml`, `OptionsForm.ui.qml` — le menu principal et la page
  Options, en QML ; `hmi::OptionsModel`.
- `hmi::EditorKeyBindings`, `hmi::EditorActions` — raccourcis de l'éditeur.
- `hmi::Localization`.
- [Boucle de jeu et pas de temps fixe](guide-boucle.md) — à quel moment un pas lit l'intention.
- [IHM Qt — deux applications, deux technologies](guide-ihm-qt.md) — les deux applications qui pilotent tout ceci.
