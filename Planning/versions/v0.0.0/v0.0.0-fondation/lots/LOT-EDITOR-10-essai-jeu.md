+++
id = "LOT-EDITOR-10"
titre = "L'essai complet dans le jeu"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "S"
resume = "Un bouton lance le vrai jeu sur la carte ouverte, à la case voulue et dans l'état de partie voulu, les brouillons de tous les onglets passant devant les cartes du binaire."
prerequis = ["LOT-EDITOR-01"]
livrables = [
  "`Source/App/Game/Main.cpp` (build de développement) : `--map=` entre directement dans la vue de jeu, `--at=<colonne>,<ligne>`, `--flags=<a>,<b>`, `--levels=<dossier>;<dossier>`.",
  "`hmi::WorldModel` : `setLevelDirectories`, `setStartCell`, `setStartFlags` ; `core::WorldTravel::directoriesLoader`, le chargeur à plusieurs dossiers.",
  "`HmiLib` : `hmi::gameLaunchArguments`, `hmi::parseStartCell`, `parseWorldFlags`, `parseLevelDirectories` — la ligne de commande s'écrit et se relit au même endroit.",
  "`Editor/Logic/GameLaunch.h` : `hmi::playtestDirectory`, `hmi::writeDraftMaps`, `hmi::gameExecutable`.",
  "`Editor/Ui/RunInGameDialog.h` (`hmi::askRunInGame`) et `MainWindow::runInGame` : *Run in game* (F5), *from hovered cell* (Maj+F5), *Run in game…* (Ctrl+F5).",
  "Tests : `test_launch_options.cpp`, `test_game_launch.cpp`, `test_world_travel.cpp` (13 tests neufs) ; `EX-EDIT-093` à `EX-EDIT-095`.",
]
criteres = [
  "Critère écrit à l'ouverture du module : parler à Myr puis engager une rencontre sur Martpart — tenu pour sa première moitié (le dialogue du PNJ abordé s'ouvre), la rencontre attend le LOT-27.",
  "Le jeu lancé avec `--map=`, `--at=`, `--flags=` et `--levels=` s'ouvre sur la carte, héros à la case demandée, sans passer par le menu.",
  "Un brouillon posé dans le dossier d'essai est servi avant la carte du dépôt ; une carte volontairement cassée y fait échouer l'ouverture, au lieu de retomber en silence sur celle d'à côté.",
  "La ligne de commande écrite puis relue, le dossier d'essai et son nettoyage, et le chargeur à plusieurs dossiers ont chacun leurs tests, verts.",
]
+++

## Pourquoi

L'éditeur savait **marcher** sur la carte qu'il édite : l'essai immédiat du canevas
(`LOT-EDITOR-04`) fait parcourir le brouillon, franchit ses portails, et dit en barre d'état ce
que le jeu ferait — *« dialogue X would open »*, *« encounter Y would start »*. C'est le bon outil
pour vérifier qu'un couloir passe et qu'une porte mène quelque part ; ce n'en est pas un pour
vérifier qu'une carte **se joue**. Ce lot ajoute l'autre moitié : un bouton qui lance
`JustAnotherRpgGame` sur la carte ouverte, là où on veut, dans l'état de partie qu'on veut.

Feuille de route : [éditeur](@ref roadmap-editeur).

## Ce que le dépôt contenait à l'ouverture (21 septembre 2026)

- **Le jeu savait déjà ouvrir une carte imposée** : `--map=<carte>[@<arrivée>]` (`LOT-96`), en
  build de développement, remplace la carte de « Nouvelle partie ». Mais il fallait encore
  traverser le menu pour la voir, et rien ne disait *où* sur la carte, ni dans quel état de
  partie.
- **Le jeu ne lit ses cartes qu'à un seul endroit** : `Levels/` à côté de son exécutable, que la
  construction recopie (`CopyGameData`). L'éditeur, lui, ouvre l'arbre des sources
  (`Source/Elements`, `LOT-EDITOR-06`) : ce qu'on vient de peindre n'existait nulle part pour le
  jeu tant qu'on n'avait pas enregistré **et** reconstruit.
- **`--screen=<Nom>`** ouvre le jeu sur un écran précis, mais **court-circuite le routeur** : la
  pile d'écrans garde cet écran quoi qu'il arrive. Lancer ainsi la vue de jeu aurait empêché le
  dialogue de s'ouvrir — l'écran forcé serait resté.
- **Les drapeaux de monde** (`core::WorldFlags`) vivent sur la session d'exploration et
  survivent au changement de carte ; rien ne permettait d'en poser au lancement.

## Périmètre

### Ce qui a été fait

#### Côté jeu — trois options, rien d'autre

`Source/App/Game/Main.cpp`, en build de développement seulement :

| Option | Effet |
|---|---|
| `--map=<carte>[@<arrivée>]` | la carte d'ouverture (déjà là), **et** l'entrée directe dans la vue de jeu |
| `--at=<colonne>,<ligne>` | le héros y est posé au lieu de l'entrée |
| `--flags=<a>,<b>` | les drapeaux de monde acquis avant le premier pas |
| `--levels=<dossier>;<dossier>` | les dossiers de cartes, cherchés dans l'ordre, avant celui du binaire |

`hmi::WorldModel` reçoit `setLevelDirectories`, `setStartCell` et `setStartFlags` ; le chargeur à
plusieurs dossiers est dans `Core` (`core::WorldTravel::directoriesLoader`), où sont déjà les
autres.

#### Côté éditeur — un bouton, un dialogue, un dossier

`hmi::playtestDirectory`, `hmi::writeDraftMaps` et `hmi::gameExecutable`
(`Editor/Logic/GameLaunch.h`) disent où poser les brouillons et où trouver le jeu ;
`hmi::askRunInGame` (`Editor/Ui/RunInGameDialog.h`) demande la case et les drapeaux ;
`MainWindow::runInGame` écrit, construit la ligne de commande et lance le `QProcess`.

### Ce que le lot ne fait pas

- **La rencontre ne s'engage pas encore.** Le critère d'acceptation écrit à l'ouverture du module
  (« parler à Myr puis engager une rencontre sur Martpart ») tient pour sa première moitié : le
  jeu ouvre le dialogue du PNJ qu'on aborde (`LOT-09`). La seconde attend le [LOT-27](@ref
  lot-27) : `hmi::WorldModel` **émet** `encounterRequested`, et aucun écran ne l'écoute — le
  combat depuis une carte d'exploration n'est branché nulle part (le Colisée, lui, est un mode à
  part). Ce lot n'ouvre pas ce chantier : il ne change du jeu que sa ligne de commande.
- **Myr n'existe pas.** Les PNJ de Martpart sont les sentinelles Ironhand (`LOT-93`), et c'est
  sur l'une d'elles que l'essai se vérifie.

## Conception

- **Une carte imposée ouvre le jeu sur la carte**, sans passer par le menu : `--map=` fait
  désormais entrer le routeur dans la vue de jeu (`ScreenRouter::openGame`), exactement comme
  « Nouvelle partie ». *Écarté* : lancer l'éditeur avec `--screen=GameView`. L'écran forcé fige la
  pile ; le premier PNJ n'aurait ouvert aucun dialogue, et l'essai n'aurait rien prouvé de ce
  qu'on vient y vérifier. Un lancement avec `--screen=` garde son comportement : il l'emporte
  toujours sur le routeur.
- **Les brouillons passent devant les cartes du binaire.** `--levels=<dossier>;<dossier>` donne
  les dossiers où chercher une carte, **dans l'ordre** (`core::WorldTravel::directoriesLoader`) ;
  le `Levels/` de l'exécutable vient toujours en dernier. L'éditeur passe deux dossiers : celui de
  l'essai, puis son propre `Levels/` — le monde autour de la carte éditée est donc celui du dépôt,
  pas celui de la dernière construction.
- **Une carte présente mais illisible arrête la recherche.** Passer au dossier suivant ferait
  jouer en silence la version d'avant de la carte qu'on vient de casser — le pire des
  comportements pour un outil de vérification.
- **Tous les onglets sont écrits, pas seulement la carte jouée.** Un portail mène sur la carte
  d'à côté, et c'est son brouillon qu'on veut voir. Une carte qu'aucun onglet ne porte reste celle
  du dépôt.
- **Le dossier d'essai vit hors du dépôt** (`<temp>/JustAnotherRpgGame-playtest`) et il est
  **vidé à chaque essai** : une carte fermée depuis le dernier lancement passerait sinon devant
  celle du jeu sans que rien ne le dise. Un essai n'enregistre rien ; ce qu'on joue n'est pas ce
  qu'on livre tant qu'on n'a pas enregistré.
- **Le jeu lit ses *catalogues* dans sa propre copie.** Dialogues, rencontres, figurines, villes
  et assets restent ceux du binaire : le lot ne change que les **cartes**. Un dialogue écrit à
  l'instant demande donc encore une reconstruction. *Écarté* : un `--data=<racine>` qui
  déplacerait tout l'arbre de données du jeu — c'est un autre lot, et l'essai n'en a pas besoin
  pour ce qu'il vérifie.
- **La ligne de commande s'écrit et se relit au même endroit** (`hmi::gameLaunchArguments`,
  `hmi::parseStartCell`, `parseWorldFlags`, `parseLevelDirectories`, dans `HmiLib`). Deux moitiés
  écrites chacune de son côté auraient divergé au premier ajout, et le défaut ne se serait vu
  qu'à l'essai suivant.
- **Le point-virgule sépare les dossiers**, la virgule les drapeaux et les coordonnées : un chemin
  de Windows porte un deux-points, et souvent une virgule.
- **Trois commandes, aucune remappable.** *Run in game* (F5) part de l'entrée de la carte (ou du
  dernier choix), *Run in game from hovered cell* (Maj+F5) de la case survolée, *Run in game…*
  (Ctrl+F5) demande d'abord la case et les drapeaux. Les touches de fonction ne sont pas dans
  `hmi::Key` (F8, F9 ne le sont pas non plus) : ouvrir le remappage à F5 aurait demandé de
  toucher `InputState`, `QtKeyMap` et leurs tests pour un raccourci qu'on ne remappe pas.
- **Les drapeaux choisis tiennent la session.** Le dialogue coche dans la liste des drapeaux que
  les dialogues du jeu **posent** (`EditorReferences::flags`), et laisse saisir les autres — une
  quête peut précéder son dialogue. Ce qui est choisi sert aux essais suivants, sans être
  persisté : un état de partie fabriqué n'est pas un réglage de l'éditeur.
- **Un seul essai à la fois** : lancer de nouveau remplace le jeu en cours. Deux jeux sur les
  mêmes brouillons, ce sont deux mondes qui divergent, et l'on ne sait plus lequel montre la
  retouche qu'on vient de faire. Le jeu est un processus **enfant** de l'éditeur : il se ferme
  avec lui, et ses échecs de démarrage se lisent en barre d'état.
- **Une carte jamais enregistrée ne se joue pas** : le jeu ouvre les cartes par identifiant. Un
  brouillon qui ne se convertit pas en niveau est refusé de la même façon, avec sa raison — le jeu
  l'aurait refusé aussi, mais dans **son** journal à lui.
- **Une case hors de la carte ne fait pas échouer le lancement** (`EX-NFR-040`) : le héros part de
  l'entrée, et le journal dit pourquoi.

## Vérification

- **923 tests** verts, dont 13 neufs : la ligne de commande écrite puis relue
  (`test_launch_options.cpp`), le dossier d'essai et son nettoyage (`test_game_launch.cpp`), et le
  chargeur à plusieurs dossiers (`test_world_travel.cpp`).
- **À la main, sur les données livrées** : `JustAnotherRpgGame --map=capital/martpart --at=36,38
  --flags=… --levels=…` ouvre le jeu sur Martpart, héros à côté de la sentinelle, sans passer par
  le menu ; un brouillon posé dans le dossier d'essai est bien servi **avant** la carte du dépôt
  (une carte volontairement cassée y fait échouer l'ouverture, au lieu de retomber en silence sur
  celle d'à côté).
- **Reste due** : la vérification à la souris, comme pour les lots précédents du module.

## Bilan

**Livré le 21 septembre 2026** (ouvert le même jour), sur la branche `lot-editor-10-essai-jeu`.
Vérification automatisée : construction `/W4 /WX` sans avertissement, 923 tests verts (CTest,
Debug), et le jeu lancé à la main sur `capital/martpart` avec `--map=`, `--at=`, `--flags=` et
`--levels=` — il s'ouvre sur la carte, héros à la case demandée, et sert bien le brouillon du
dossier d'essai avant la carte du dépôt. **Reste due** : la vérification à la souris (le bouton
*Run in game* et son dialogue).

Exigences : `EX-EDIT-093` à `EX-EDIT-095` (nouvelles).
