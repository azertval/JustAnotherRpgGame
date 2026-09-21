+++
id = "LOT-09"
titre = "Le Colisée se parcourt : exploration dans le jeu et première carte"
version = "0.0.0"
filiere = "cartes"
statut = "livre"
taille = "XL"
resume = "« Nouvelle partie » pose le héros à la porte du Colisée : on y marche, on y parle, on descend sur le sable pour combattre et l'on revient sur la carte au même endroit."
prerequis = ["LOT-04", "LOT-06", "LOT-10", "LOT-15", "LOT-18", "LOT-50", "LOT-86", "LOT-11", "LOT-92"]
livrables = [
  "`core::ExplorationSession` (marche, collision, interaction, portails) et `hmi::WorldModel`, la partie en singleton qui survit aux écrans.",
  "`hmi::WorldSceneComposer` et `hmi::WorldSceneRenderer`, jumeaux de ceux de l'arène ; géométrie des planches dans `ScenePieces.h` ; `worldCamera` qui suit le héros.",
  "`core::WorldTravel`, `validateWorldGraph`, `validateWorldMap` et cinq cartes de fixture parcourues aller et retour.",
  "`Source/Elements/Levels/coliseum.json` : 40 × 34, 634 cases franchissables, 31 des 36 pièces de l'atelier, cinq PNJ et leurs dialogues, deux points d'arrivée.",
  "`core::CombatZone` et `cropLevelToZone` ; l'arène du catalogue désigne `coliseum.json`, sa zone et sa région.",
  "`ScreenRouter.openDialogue`, action de dialogue `startCombat`, `arenaReturnTo` et le fondu du passage.",
  "Script d'atelier `atelier/carte_colisee.py`, qui pose la carte depuis une description de haut niveau.",
  "Retraits : `arena-of-the-future.json`, les six fonds de test, `generate_test_backgrounds.py`, `nuee-de-rats.json` (remplacée par `colisee-fauves.json`).",
]
criteres = [
  "**À l'écran** : « Nouvelle partie » ouvre le Colisée à sa porte ; on parcourt au clavier et à la manette le hall, un couloir, un vestiaire et une tribune ; on parle à un PNJ et l'écran de dialogue s'ouvre sur son dialogue ; on parle au héraut, le combat se joue sur le sable et l'on revient sur la carte au même endroit ; une capture de référence du Colisée entre dans les tests QML.",
  "**Headless** : un parcours de cinq cartes de fixture, aller et retour, par points d'arrivée nommés ; un portail orphelin et une zone de combat invalide sont refusés au chargement avec un message exploitable ; la session d'arène joue sur la zone déclarée, et les cases hors zone lui sont inconnues.",
  "L'état de la carte (porte ouverte, coffre pris) est **conservé** dans la session au retour du sable — c'est ce qui distingue un monde d'une suite de tableaux. Persisté au [LOT-17](@ref lot-17).",
  "Aucun fichier de test, de démonstration ou de planche provisoire ne subsiste dans `Source/Elements/` ; le Colisée du menu principal joue sur `coliseum.json`, et `ArenaSession` se rejoue à graine fixée comme avant.",
]
+++

## Pourquoi

« Nouvelle partie » pose le personnage à la **porte du Colisée** ; on parcourt au clavier et à la
manette l'entrée, les couloirs, les vestiaires et les tribunes — des lieux où l'on marche et où
l'on parle, et où l'on ne se bat pas — ; on descend sur le sable, qui est la zone de combat ; le
héraut, posé sur la carte à la grille du sable, lance le combat du Colisée **sur cette zone**, et à
l'issue on est de retour sur la carte, au même endroit. Le Colisée cesse d'être une grille nue :
c'est un lieu.

## Périmètre

- **La scène d'exploration dans le jeu Qt Quick.** `hmi::GameSession` compile dans le jeu et non
  plus seulement dans l'éditeur ; un `WorldViewportItem`, jumeau d'`ArenaViewportItem`
  ([LOT-86](@ref lot-86)), dessine la carte courante par le **même** pipeline QRhi et le même
  composeur à calques (sol, objets, personnages) ; la caméra suit le héros (`cameraFraming`,
  `EX-LVL-006`) — le Colisée ne tient plus dans un écran ; le héros est une figurine de l'atelier
  du `LOT-91`. Aucun second moteur de rendu : l'arène et le lieu se dessinent par le même code.
- **Le graphe** : cartes, portails, points d'arrivée **nommés**. Un portail référence
  `(carte cible, nom du point d'arrivée)`, jamais des coordonnées, qui se désynchroniseraient au
  premier redimensionnement. Le portail et le point d'arrivée sont des entités de la table du
  [LOT-10](@ref lot-10) : ils se posent dans l'éditeur, se détruisent et se recréent avec la carte.
  Un portail peut exiger un drapeau (`requiresFlag`), lu ici, posé au [LOT-16](@ref lot-16). Le
  Colisée n'a qu'une carte, mais le graphe est là dès ce lot, avec ses cinq cartes de fixture, pour
  que le `LOT-96` n'ait que des quartiers à y poser.
- **La carte du Colisée, version finale** : `Source/Elements/Levels/coliseum.json`, avec la planche
  du [LOT-92](@ref lot-92). Le sable du `LOT-50` (20 × 14) en est le centre ; autour, les **zones
  neutres** : la porte et le hall, deux couloirs sous les gradins, les vestiaires des deux camps,
  les tribunes et la loge — et des PNJ pour les peupler, pris parmi les cinq figurines déjà livrées
  par l'atelier, avec un dialogue chacun. Le combat y est **interdit** par propriété de zone
  (`EX-LVL-018`), comme le livre le veut hors du sable.
- **Le sable comme zone de combat déclarée** : un rectangle nommé sur la carte (`combatZone`,
  propriété de couche ou entité de zone, `EX-LVL-018`) que `core::ArenaSession`
  ([LOT-50](@ref lot-50)) prend pour grille tactique **à la place de la carte entière**. Les points
  d'entrée des deux camps (`arenaEntry`) restent des entités, dans la zone. Le Colisée ouvert depuis
  le menu principal joue sur cette zone de cette carte : le niveau `arena-of-the-future.json` part,
  et l'arène du catalogue (`World/arena/`) désigne `coliseum.json` et sa zone — et prend enfin sa
  **région** : la Capitale.
- **Du sol au sable, et retour** : le héraut est une entité `npc` posée à la grille ; son dialogue
  (`heraut-colisee.json`, qui perd sa marque *provisoire*) lance la session d'arène par la bascule
  du [LOT-18](@ref lot-18) — la carte est gelée, la session joue sur la zone, la fin de session
  rend la carte à l'exploration, personnage au même endroit, relevé par la Marque. C'est le geste
  que le [LOT-27](@ref lot-27) reprendra pour l'Arène du Destin.
- **Parler depuis la carte** : touche `E` ou bouton de la manette → `core::findInteractionTarget`
  ([LOT-10](@ref lot-10)) → `core::dialogueTriggerFor` ([LOT-15](@ref lot-15)) →
  `hmi::DialogueMode` et l'écran de dialogue, **sur le dialogue du PNJ visé**, plus jamais sur le
  héraut écrit en dur.
- **Transition** : fondu au passage d'un portail et à l'entrée du sable, dans le jumeau QML de la
  vue du jeu, piloté par `hmi::ScreenRouter` — `hmi::ScreenFlow` est la navigation du châssis
  d'édition, pas celle du jeu.
- **Validation au chargement** : un portail dont la carte ou le point d'arrivée n'existe pas, une
  zone de combat qui déborde de la carte ou qui n'est pas un terrain tactique valide, sont des
  erreurs explicites, pas un plantage à la traversée (`EX-NFR-040`).
- **Retraits, en sortie de lot.** La carte finale rend sans objet tout ce qui tenait lieu de
  contenu : le niveau `arena-of-the-future.json` et la planche
  `Coliseum/production_source_atlas.png` du `LOT-50` ; les tuiles, fonds, objets et skins de test
  des `LOT-06` à `LOT-08` (`Assets/Backgrounds/test_*.png` et les scripts `generate_test_*.py` qui
  les produisent) ; le personnage de démonstration du [LOT-13](@ref lot-13)
  (`demonstration-brenna.json`), remplacé par le héros créé à « Nouvelle partie » ; la rencontre de
  démonstration du [LOT-18](@ref lot-18) (`nuee-de-rats.json`), remplacée par une rencontre du
  Colisée sur des blocs du `LOT-33` ; l'ouverture en dur du dialogue du héraut. Les **quatre
  classes provisoires** du `LOT-36` restent : leur retrait est au dernier lot de classe, et elles
  ne sont pas un substitut de contenu mais un échafaudage de règles. En sortie,
  `check_asset_keys.py` et les tests ne connaissent plus aucune clé de test.

## Conception

### Ce que le dépôt contenait à l'ouverture (17 septembre 2026)

Le périmètre écrit dans la feuille de route date du 16 septembre ; une lecture du dépôt en déplace
la charge, sans en changer la cible.

- **`core::WorldGraph` existe déjà** (`Source/Core/World/`, posé au [LOT-11](@ref lot-11) pour la
  vue « graphe du monde » du navigateur de cartes) : cartes, points d'arrivée nommés, statut d'un
  portail (`PortalLinkStatus`). Ce lot n'a pas de graphe à inventer : il lui ajoute le **chargement
  de carte à chaud** et la **traversée**, sans changer son contrat de lecture.
- **La plomberie QRhi du jeu est posée** : `hmi::GameViewportItem` (QML `GameViewport`) crée son
  `QRhi` et sa passe de rendu, mais **ne dessine aucune scène** ; `GameView.qml` est encore en
  `pending: true` (« Aucune carte à jouer »). Le jumeau à recopier est `ArenaViewportItem`, avec
  `ArenaSceneComposer` et `ArenaSceneRenderer` ([LOT-86](@ref lot-86)).
- **`hmi::GameSession` ne compile que dans `LevelEditor`** : il est déclaré dans la cible de
  l'éditeur, pas dans celle du jeu.
- **Les 36 textures du Colisée sont installées** (`Source/Elements/Assets/Scene/coliseum/`,
  [LOT-92](@ref lot-92)) : sables, dalles, pavés de couloir, planches de vestiaire, gradins,
  loge, murs, arches, escaliers, torches, bannières, grilles. La carte se trace avec elles, et
  avec elles seules.
- **`Source/Elements/Levels/` ne porte qu'`arena-of-the-future.json`**, le niveau provisoire du
  [LOT-50](@ref lot-50), que ce lot remplace.

### Décisions d'ouverture

- **La carte se pose par script, puis se retouche dans l'éditeur** (*décision de l'auteur*,
  17 septembre 2026). Un script d'atelier versionné dans le lot écrit `coliseum.json` depuis une
  description de haut niveau (les zones, leurs bords, leurs pièces) ; l'éditeur du
  [LOT-11](@ref lot-11) reste l'outil de retouche, et le fichier produit est un fichier de niveau
  ordinaire, sans marque d'origine. Écrire les cases à la main n'était ni relisible ni rejouable ;
  les tracer entièrement à la souris n'était pas un usage du temps de l'auteur.
- **Une seule PR en fin de lot** (*décision de l'auteur*), six phases commitées sur
  `worktree-lot-09-colisee`.

### Les six phases

| # | Phase | Ce qu'elle livre |
|---|---|---|
| 1 | **Le graphe jouable** | `WorldGraph` étendu : chargement à chaud, traversée par point d'arrivée nommé, validation explicite au chargement (`EX-NFR-040`) ; cinq cartes de fixture et le parcours headless aller-retour |
| 2 | **La scène d'exploration dans le jeu** | `WorldViewportItem`, jumeau d'`ArenaViewportItem` : même pipeline QRhi, même composeur à calques ; la caméra suit le héros (`EX-LVL-006`) ; `GameSession` compile dans `JustAnotherRpgGame` et `GameView.qml` quitte `pending` |
| 3 | **La carte du Colisée, version finale** | `Source/Elements/Levels/coliseum.json` : le sable 20 × 14 au centre, le hall et la porte, deux couloirs sous les gradins, les deux vestiaires, les tribunes et la loge ; les PNJ (`anariel`, `jade`, `lizz`, `nakral`, `xorius`) et leur dialogue |
| 4 | **Le sable, zone de combat déclarée** | `combatZone` sur la carte (`EX-LVL-018`) ; `core::ArenaSession` joue sur la zone et non sur la carte entière ; l'arène du catalogue désigne `coliseum.json`, sa zone et sa région (la Capitale) |
| 5 | **Parler, basculer, revenir** | `E` / bouton de manette → `core::findInteractionTarget` → `core::dialogueTriggerFor` → `hmi::DialogueMode`, sur le dialogue du PNJ visé ; le héraut lance la session d'arène par la bascule du [LOT-18](@ref lot-18) ; retour sur la carte au même endroit, état de carte conservé ; fondu piloté par `hmi::ScreenRouter` |
| 6 | **Les retraits** | Le contenu provisoire quitte `Source/Elements/` ; capture de référence du Colisée dans les tests QML ; `check_asset_keys.py` et les tests sans aucune clé de test |

## Ce que le lot a livré, et ce qu'il n'a pas livré

### Livré, et vérifié

| Ce que le lot promettait | Ce qui a été fait | Comment c'est vérifié |
|---|---|---|
| L'exploration dans le jeu Qt Quick | `core::ExplorationSession` (marche, collision, interaction, portails) et `hmi::WorldModel` — la partie, en singleton, qui survit aux écrans | `test_exploration_session.cpp` (4), `--screen=GameView` |
| Le même rendu que l'arène | `hmi::WorldSceneComposer` + `hmi::WorldSceneRenderer`, jumeaux du composeur et du rendu de l'arène ; la géométrie des planches vit une seule fois (`ScenePieces.h`) | `test_world_scene_composer.cpp` (3), `test_world_scene_renderer.cpp` (3, sur un vrai `QRhi`) |
| La caméra qui suit le héros (`EX-LVL-006`) | `worldCamera` : agrandissement **entier**, suivi borné à la scène | `LaCameraSuitLeHerosSansSortirDeLaCarte` |
| Le graphe jouable et sa validation (`EX-NFR-040`) | `core::WorldTravel`, `validateWorldGraph`, `validateWorldMap` ; cinq cartes de fixture, aller et retour | `test_world_travel.cpp` (6) |
| La carte du Colisée, version finale | `coliseum.json` : 40 × 34, 634 cases franchissables, 31 des 36 pièces de l'atelier, cinq PNJ, deux points d'arrivée | `test_coliseum_map.cpp` (6) |
| Le sable comme zone de combat déclarée (`EX-LVL-018`) | `core::CombatZone`, `cropLevelToZone` ; l'arène du catalogue désigne la carte, sa zone et son lieu | `test_combat_zone.cpp` (3), `--screen=Arena` |
| Parler au PNJ visé | `ScreenRouter.openDialogue`, `dialogueId` ; l'écran de dialogue n'a plus d'identifiant en dur | `OnParleAuPnjQueLOnRegarde`, qmllint |
| Du sol au sable, et retour | action de dialogue `startCombat` (le héraut), `arenaReturnTo` dans la table des écrans, session en singleton | `LeColiseeRevientSurLaCarteQuandLeHerautYEnvoie` |
| Le fondu du passage | un voile dans le jumeau de la vue de jeu, relancé à l'entrée sur une carte et au retour | relu à l'écran |

### Écarté, et pourquoi

Trois retraits que la feuille de route demandait n'ont **pas** été faits. Les écrire ici vaut mieux
que les faire à moitié :

- **Le personnage de démonstration reste** (`demonstration-brenna.json`). La feuille de route le
  remplaçait par « le héros créé à *Nouvelle partie* » — sauf qu'aucune création de personnage
  n'existe encore dans le jeu (`LOT-43` l'a dessinée, personne ne la joue). Le retirer aujourd'hui
  viderait la fiche, l'inventaire, la composition de l'arène et l'interlocuteur des dialogues, qui
  le lisent tous. Il part avec la création de personnage, pas avant. La figurine du héros sur la
  carte (`WorldModel::heroFigure`, `jade`) est provisoire pour la même raison.
- **La planche source du `LOT-50` reste** (`Coliseum/production_source_atlas.png`). Ce n'est pas du
  contenu provisoire : c'est la **source** des pièces que l'arène dessine encore, et
  `extract_coliseum_atlas.py` n'a rien d'autre à découper. Les planches de l'atelier des textures
  (`Scene/*/planche-*.png`) sont commitées pour exactement la même raison.
- **`hmi::GameViewportItem` reste** : l'écran de jeu ne s'en sert plus, mais l'affichage tête haute
  de combat (`CombatHud.qml`) le pose encore. Il partira avec le branchement du combat sur la carte
  (`LOT-27`).

Sont partis, eux : le niveau `arena-of-the-future.json`, les six fonds de test et
`generate_test_backgrounds.py`, la rencontre de démonstration `nuee-de-rats.json` (remplacée par
`colisee-fauves.json`, les fauves du Colisée, sur les bêtes du `LOT-33`) et la marque *provisoire*
du dialogue du héraut.

## Journal

- **17 septembre 2026, ouverture.** Les deux décisions d'ouverture ci-dessus. Le lot **quitte la
  section 11 de la [feuille de route](@ref roadmap) pour cette page dès son ouverture** :
  `lint_lots.py` tient un lot pourvu d'un dossier pour sorti de la page, et l'ancre `{#lot-09}`
  ne peut pas être écrite deux fois. Les deux tableaux de la section 6 ont été régénérés
  (`lint_lots.py --regenerer`) et le diagramme perd les arêtes entrantes du lot, comme pour tout
  lot sorti de la page ; le nœud y reste, marqué « en cours ».
- **17 septembre 2026, phases 1 et 2.** Le graphe se joue (`WorldTravel`), puis la session
  d'exploration et la composition d'un lieu. *Décision de l'auteur* : plutôt que de compiler
  `hmi::GameSession` dans le jeu — ce qui y aurait amené un second moteur de rendu, en tuiles
  carrées, alors que le même paragraphe de la feuille de route demandait le composeur de l'arène —,
  une session d'exploration neuve vit dans `Core`, jumelle de `core::ArenaSession`. `GameSession`
  reste le banc d'essai de l'éditeur.
- **17 septembre 2026, phase 2 (suite).** *Décision de l'auteur* sur ce qu'une case porte : le sol
  par type de tuile, le relief nommé à la case. Les deux autres options étudiées étaient un format
  de niveau `v4` (tout à la case) et une déduction complète par voisinage (rien dans la carte).
- **17 septembre 2026, phase 3.** La carte, posée par script puis relisible dans l'éditeur ; la
  table d'apparence du lieu ; quatre dialogues neufs et leurs 28 clés. *Piège rencontré* :
  l'écrivain de niveau n'émet l'assignation de texture d'une case que si cette case a un **type**
  non vide — une case franchissable qui porte du relief reçoit donc `dirt` dans la grille racine,
  sa matière visible restant dans la couche de sol.
- **17 septembre 2026, phase 4.** La zone de combat, et la carte réduite à la zone. `ArenaSession`
  n'a pas changé d'une ligne : elle reçoit une carte qui **est** la zone.
- **17 septembre 2026, phase 5.** *Piège rencontré, et corrigé* : la pile d'écrans ne garde qu'un
  écran vivant (`Loader`). Une session possédée par l'écran de jeu mourait à l'ouverture du
  dialogue ou du Colisée, et l'on revenait sur une carte neuve, héros à la porte — exactement ce
  que le lot interdit. `hmi::WorldModel` est donc un **singleton** : la partie n'appartient pas à
  l'écran qui la montre.
- **17 septembre 2026, phase 6.** Les retraits, et les trois écarts assumés ci-dessus. La capture
  de référence est un rendu **hors écran** sur un vrai `QRhi` (`test_world_scene_renderer.cpp`), et
  non une image de test QML : la scène du lieu est dessinée par le pipeline 2D, que les tests QML
  ne font pas tourner.

## Bilan

Statut : **livré le 17 septembre 2026** (ouvert le même jour). Vérification automatisée :
construction `/W4 /WX` sans avertissement, `ctest` à **1 317/1 317** (dont le rendu du Colisée
hors écran sur un vrai `QRhi`), toute la batterie de `scripts/check.py` verte hormis
PSScriptAnalyzer, absent du poste et sans rapport avec ce lot (aucun `.ps1` touché). Relu à
l'écran : `--screen=GameView` et `--screen=Arena`, captures à l'appui. Reste la vérification
IHM manuelle — marcher, parler, descendre sur le sable, en revenir.

Ce que chaque prérequis apportait : [LOT-10](@ref lot-10) (les portails et les PNJ sont des entités
de carte), [LOT-15](@ref lot-15) (le dialogue qu'on ouvre depuis la carte), [LOT-18](@ref lot-18)
(la bascule exploration ↔ combat), [LOT-50](@ref lot-50) (le Colisée, dont ce lot livre la version
finale), [LOT-86](@ref lot-86) (le pipeline QRhi et `ArenaViewportItem`, dont la scène
d'exploration est le jumeau), [LOT-11](@ref lot-11) (la carte se trace dans l'éditeur),
[LOT-92](@ref lot-92) (le style de scène et la planche du Colisée).

Alimente : `LOT-96`, [LOT-16](@ref lot-16), [LOT-17](@ref lot-17), [LOT-27](@ref lot-27),
`LOT-42`, `LOT-80`.

Exigences couvertes : `EX-VIS-001`, `EX-VIS-002`, `EX-EXP-*`, `EX-LVL-018`, `EX-LVL-006`,
`EX-NFR-040`.
