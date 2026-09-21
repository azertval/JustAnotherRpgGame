+++
id = "LOT-50"
titre = "Le Colisée : bac à sable de combat"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Un affrontement se compose, se joue et se rejoue à graine fixée sans monter une partie complète, dans une arène qui est un mode du jeu et non un outil de développement."
prerequis = [
  "LOT-19",
  "LOT-20",
]
livrables = [
  "La première carte de `Source/Elements/Levels/` : `arena-of-the-future.json`, avec ses douze entités `arenaEntry`.",
  "Le noyau de l'arène (`Source/Core/Combat/Arena.{h,cpp}`) : `core::arenaEntryPoints`, `core::ArenaSession`, `core::ArenaBout`, `core::loadArenas`, `core::loadHeroicMarks`.",
  "La donnée : `Source/Elements/World/arena/` (trois arènes), `Source/Elements/Rpg/rules/heroic-marks.json` (huit Marques Héroïques) et leurs deux schémas.",
  "L'écran de mise en place et de jeu : `hmi::ArenaModel`, `ArenaForm.ui.qml` et `Arena.qml`, ouvert par « Nouvelle partie » (`hmi::ScreenId::Arena`).",
  "L'habillage : `scripts/extract_coliseum_atlas.py` (138 pièces découpées de `production_source_atlas.png`), `ArenaScene` et `ArenaTile`.",
]
criteres = [
  "Un affrontement se met en place, se joue et se rejoue sans quitter le jeu.",
  "À graine et composition égales, deux exécutions donnent le même déroulé.",
  "Aucun combattant n'y meurt définitivement.",
  "L'arène s'ouvre aussi comme une carte ordinaire depuis le monde (non tenu : reporté au `LOT-42`, que le critère nomme lui-même).",
  "`ctest` : 1090/1090 (1084 avant, plus les six cas de ce lot).",
]
+++

## Pourquoi

Un lieu pour éprouver le combat, encore et encore, sans monter une partie complète à chaque
essai — et qui ne soit **pas** un outil de développement déguisé : les Arènes de Tanares sont une
institution du monde, et l'arène livrée ici est celle que le jeu final gardera.

## Périmètre

- **La première carte** de `Source/Elements/Levels/`, vide depuis le `LOT-01` :
  `arena-of-the-future.json`, une enceinte de murs et de gradins, du sable, deux portes, et
  douze points d'entrée — six par camp — écrits comme des entités `arenaEntry` de la carte,
  jamais en dur.
- **Le noyau de l'arène** (`Source/Core/Combat/Arena.{h,cpp}`) : `core::arenaEntryPoints` lit les
  entrées d'une carte ; `core::ArenaSession` tient la carte, la composition (`core::ArenaBout`) et
  le combat (`core::CombatState`) — monte, lance à graine fixée, joue, **rejoue** ; les catalogues
  `core::loadArenas` et `core::loadHeroicMarks`.
- **La donnée** : `Source/Elements/World/arena/` (trois arènes du Sourcebook : l'Arène du Futur,
  jouable et non létale ; Feargus, létale ; le duel de baguettes de la Magocratie), les huit
  Marques Héroïques dans `Source/Elements/Rpg/rules/heroic-marks.json`, et leurs deux schémas.
- **L'écran de mise en place et de jeu** : `hmi::ArenaModel` (`Source/HMI/Runtime/`), le
  formulaire `ArenaForm.ui.qml` et son jumeau `Arena.qml`, ouvert par « Nouvelle partie » depuis
  le menu principal, l'écran `Arena` de la table de navigation (`hmi::ScreenId::Arena`,
  `OpenArena`/`CloseArena`).

Il ne livre ni attaque, ni IA, ni IHM de combat sur la carte : le coup qui s'y joue est un **coup
d'essai** (ci-dessous), et l'écran dessine la grille lui-même. Il ne livre pas non plus l'ouverture
de l'arène **depuis le monde** comme une carte ordinaire : c'est le critère qui nomme le `LOT-42`,
et le voyage n'existe pas encore.

## Conception

### Le premier lieu où le combat est tenu

Le [LOT-20](@ref lot-20) avait laissé `core::CombatState` hors de tout : vérifié sans fenêtre, tenu
par personne. `core::ArenaSession` est le premier objet du jeu qui le **tient** — la carte, la
composition, la machine, la suite aléatoire, le journal —, et il le fait sans passer par
`hmi::CombatMode` ni par `hmi::GameSession` : une arène est une carte à elle seule, rien n'y est à
geler, et bâtir la session d'exploration autour d'un niveau que la surface de rendu Qt Quick ne
sait pas encore afficher aurait produit du code que rien ne peut vérifier. Brancher la machine sur
une carte d'exploration gelée reste l'affaire de l'IHM de combat (`LOT-24`), qui lira la même
session par les mêmes appels.

L'écran, lui, est ce que la feuille de route demandait : un **écran de développeur, en QML, sans
charte**. Il dessine la grille depuis une liste de cases (`ArenaModel::cells`) plutôt que par la
surface de rendu, parce que celle-ci n'affiche encore aucune scène ; les couleurs viennent des
jetons de la charte v2 pour qu'il ne jure pas au milieu du jeu, sans prétendre à une maquette.

### Rejouable à graine fixée : ce que cela achète

L'acceptation demandait qu'à graine et composition égales, deux exécutions donnent le même
déroulé. La session le garantit par construction : **une seule** suite aléatoire
(`core::DeterministicRandom`) sert l'initiative, les jets d'attaque et les dégâts, et `replay`
remonte la même composition sur la même carte à la même graine. Un test joue une escarmouche à
cinq jusqu'à son issue, la rejoue, et obtient le même journal — initiative, pas, coups, issue —,
puis une autre graine et un autre journal.

C'est ce qui fait de l'arène un banc d'essai : comparer deux versions d'une mécanique, c'est
comparer deux journaux à la même graine.

### Personne n'y meurt, et c'est la fiction qui l'explique

« Les Arènes opposent deux camps d'un conflit, représentés par leurs Héros, pour résoudre les
impasses sans recourir à la guerre ni aux morts. » Le rituel de **Marque Héroïque** relève tout le
monde à la fin du combat : `ArenaSession` s'abonne à `CombatHook::CombatEnded` et rend à chaque
combattant ses points de vie — sauf dans une arène **létale**, qui est l'exception écrite dans la
donnée (`lethal: true`, Feargus). Un test le vérifie dans les deux sens.

Le rituel accorde aussi la **troisième économie d'action** du §4bis : chaque combattant d'une arène
à Marque reçoit `heroicAction` par `core::ActionEconomy::declare`, le crochet posé au
[LOT-20](@ref lot-20). Aucune capacité ne la dépense encore — ce sera l'affaire des classes —, mais
la ressource existe, se voit dans l'écran, et son absence dans une arène sans Marque est testée.

Les **huit rôles** des Marques (Bruiser, Brute, Commander, Controller, Healer, Shooter, Tactician,
Tank) sont une donnée de règle, et l'écran permet d'en revendiquer un par combattant : c'est
l'étiquette contre laquelle les lots de classes vérifieront que chacune remplit le sien.

### Le coup d'essai, provisoire et dit comme tel

Les attaques sont au `LOT-21`. Sans coup, un combat ne finit jamais, et un banc d'essai qui ne
finit jamais ne vérifie rien. `ArenaSession::strike` porte donc un **coup d'essai** : l'attaque se
déclare (`declareAttack`), l'action se dépense, un d20 plus le bonus se jette contre la classe
d'armure, les dés de dégâts tombent, `applyDamage` finit le travail. Le kit (`core::StrikeKit`) se
lit du bloc de bestiaire — la **première** action qui frappe — ou de la fiche — le **coup à mains
nues** du SRD, maîtrise plus Force pour toucher, 1 plus Force en dégâts : la règle du livre, pas
une valeur inventée. L'allonge est d'une case ; la portée est au `LOT-22`.

Ce que le `LOT-21` remplacera est la **façon** de frapper ; ce qu'il gardera est le **lieu** : la
session, la déclaration avant le jet, la suite aléatoire unique, le journal.

### Ce que l'écran compose, et ce qu'il ne compose pas

Le roster est le bestiaire (`LOT-33`, 94 créatures) plus le personnage de démonstration
(`LOT-38`), chargé par le même chemin que la fiche pour qu'il n'y ait qu'une vérité sur ce qu'il
porte. Chaque camp se compose librement — un moine contre trois gobelins n'a pas besoin de
prétexte, c'est la *Law of the Arena* —, une Marque se choisit par combattant, une graine se règle.

La feuille de route parlait de choisir « leur niveau, leur équipement ». La fiche porte son niveau
et son inventaire, et l'arène les prend **tels quels** : régler le niveau d'un personnage est une
montée par l'expérience (`core::gainExperience`), et lire l'arme équipée demande les propriétés
d'arme du `LOT-21`. Le jour où le groupe (`LOT-29`) et les classes (`LOT-47`) existeront, c'est le
roster qui s'élargira, pas l'écran.

### Le montage nomme ses refus

Un combattant sans case demandée va au **prochain point d'entrée libre** de son camp, dans l'ordre
des rangs ; un septième allié sur six entrées est refusé avec `OutOfBounds` — la carte n'a plus de
place, et le dire vaut mieux que le poser dans un mur ; une case demandée dans un pilier est
refusée `Obstructed`. Les refus remontent à l'écran avec leur raison. Un camp vide après montage ne
lance rien.

## Relevé en chemin

- **« Nouvelle partie » ouvre l'arène** (décision de l'auteur, le jour de la livraison). C'est la
  seule carte jouable du jeu tant que le `LOT-27` n'en livre pas d'autre, et l'arène est un mode du
  jeu, pas un outil de vérification : elle se trouve dans le menu, pas dans un sélecteur de
  développement. Le formulaire du menu garde ses six entrées de la maquette ; la vue de jeu reste
  atteignable par `--screen=GameView`, et c'est le seul appel du câblage qui changera le jour où
  une partie s'ouvrira sur le monde.
- **Les commentaires qui disaient `Source/Elements/Levels/` vide** (`GameViewportItem.h`,
  `GameViewForm.ui.qml`, `ScreenProbe.qml`, le README du dossier) disent maintenant ce qu'il porte.
- **`check_rpg_data.py`** connaît deux familles de plus : `arena` (sous `World/`) et la règle
  `heroic-marks`.

## Habillage de l'arène (14 septembre 2026, après livraison)

L'écran reste celui du développeur, mais sa grille devient la scène du Colisée. Demande de
l'auteur : le vrai design du jeu, pas une esquisse — un tileset qui respecte la maquette.

- **La source est une planche de production**, `Source/Elements/Assets/Coliseum/production_source_atlas.png`
  (1536 × 1024), sortie d'un générateur sur la maquette 01 : tuiles isométriques, tileset du Colisée,
  figurines des quatre héros (idle, marche, attaque, touché, mort) et des quatre gladiateurs,
  structures, détails, props, rochers, végétation, effets. Le pack découpé à la main qui
  l'accompagnait (`JustAnotherDnDGame_UI_ASSET_PACK/coliseum_production_pack/`, non suivi) n'était
  pas exploitable : légendes dans les tuiles, fond non détouré derrière les figurines, effets
  découpés dans du texte. **`scripts/extract_coliseum_atlas.py`** redécoupe la planche — détourage
  du fond bleu nuit, grille par profils coupés aux vallées, composantes connexes pour les sections
  irrégulières — en 138 fichiers nommés, avec des bandes d'animation à canevas commun (48 × 64,
  ancre au pied, cinq images par héros, huit par gladiateur) et un manifeste ; `--check` vérifie
  que le dossier suit la planche.
- **Deux briques** : `ArenaScene` (projection isométrique, losange de hauteur 0,62 L comme les
  tuiles de la planche, profondeur `z = colonne + ligne`) et `ArenaTile` (sol de sable ou dalle du
  Colisée, pan de mur ou colonne d'angle, bannières et torches à intervalle, arche sur les deux
  portes, surbrillance en losange de jetons, figurine animée `AnimatedSprite`, jauge d'un ennemi,
  points de vie en texte). Un allié reçoit un héros et un ennemi un gladiateur, choisis d'après le
  nom : une créature du bestiaire enrôlée s'affiche donc en gladiateur — la planche n'a pas de
  bêtes, à produire si l'arène doit en montrer.
- **Ressource** : les pièces sont embarquées comme les illustrations de la charte v2
  (`Source/Ui/CMakeLists.txt`, motif `Coliseum/*/*.png`) ; `check_ui_assets.py` sait que ce
  dossier a son propre manifeste. `ArenaModel::cells` porte en plus `hitPointsRatio`.
- **Le cahier des assets du `LOT-87` n'est pas touché** : ces pièces ne sont pas produites par
  prompt, elles sont découpées d'une planche. Une première version de cet habillage y avait
  ajouté une famille `arena` de neuf images à générer ; elle est retirée.
- **Hors périmètre** : le cadre de l'écran (panneaux, boutons, journal) garde ses contrôles Qt
  Quick nus ; les animations `walk`, `attack`, `hit` sont découpées mais pas encore jouées (le
  coup d'essai n'a pas de phase d'animation) ; l'IHM de combat (`LOT-24`) réemploiera la scène.

## Ce qui reste hors du lot, nommément

- **L'ouverture depuis le monde** (`LOT-42`) : l'arène est aussi un lieu de l'atlas, et s'ouvrira
  comme une carte ordinaire quand le voyage existera. La carte a déjà ses deux portes.
- **Les variantes régionales** sont des données : Feargus et le duel de baguettes existent sans
  carte. Les Braves des débutants et les paris à combat simulé attendent qu'un lot en ait l'usage.
- **La *Heroic Action* elle-même** : la ressource est déclarée ; aucune capacité ne la dépense
  avant les classes.
- **Le dessin du combat sur la carte** et le jeu **à la manette** : l'IHM de combat (`LOT-24`).

## Vérification

- **Un affrontement se met en place, se joue et se rejoue sans quitter le jeu.** ✔ Depuis le menu
  principal, « Nouvelle partie » ; composer, lancer, jouer case par case, rejouer, recomposer.
- **À graine et composition égales, deux exécutions donnent le même déroulé.** ✔ Vérifié par test
  sur une escarmouche à cinq, journal complet ; une autre graine donne un autre journal.
- **Aucun combattant n'y meurt définitivement.** ✔ À l'issue, tous relevés à leurs points de vie
  maximaux ; une arène létale, elle, laisse ses combattants à terre — testé aussi.
- **L'arène s'ouvre aussi comme une carte ordinaire depuis le monde.** ✘ Reporté au `LOT-42`, que
  le critère nomme lui-même : sans voyage, il n'y a pas de monde d'où l'ouvrir.
- `ctest` : **1090/1090** (1084 avant, plus les six cas de ce lot). ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1090/1090 en Debug et en Release, `clang-format`, les lints et le cahier de test verts. Un critère sur cinq n'est pas tenu et part au `LOT-42` (voir « Vérification »). Alimente [LOT-21](@ref lot-21) — les attaques se vérifient à l'œil dans l'arène — et `LOT-51` à `LOT-65`.

Exigences couvertes : la part « rejouable à graine fixée » d'`EX-NFR-002`, et le lieu où `EX-CBT-001` à `EX-CBT-020` se regardent. Aucune exigence ajoutée.
