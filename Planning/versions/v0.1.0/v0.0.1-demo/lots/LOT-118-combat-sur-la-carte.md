+++
id = "LOT-118"
titre = "Le combat sur la carte"
version = "0.0.1"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Une rencontre engagée sur une carte d'exploration se joue **sur place** : la carte se fige, la grille paraît, le combat se joue, l'exploration reprend."
prerequis = ["LOT-103", "LOT-128"]
reprend = ["LOT-27 (le combat posé sur la carte)", "LOT-24 (hérité)"]
livrables = [
  "`core::beginEncounter` monte une session de combat sur la zone de combat de la carte, gelée.",
  "L'interface de combat du `LOT-24` branchée sur la carte d'exploration, et non plus sur le seul Colisée.",
  "Les issues : victoire (retour à l'exploration, drapeau posé), défaite (écran de mort), fuite.",
  "Un dialogue peut **déclencher** une rencontre.",
]
criteres = [
  "Un combat se joue sur une carte de test, du déclenchement par dialogue au retour à l'exploration, au clavier et à la manette.",
  "Le rejeu à graine fixée donne le même combat qu'en arène.",
  "Aucune régression du mode arène.",
]
+++

## Sur une maquette

La carte de test du combat est une **carte maquette** ([LOT-128](LOT-128-cartes-maquettes.md)) : sols et murs
en couleurs, jetons vert et rouge. Le combat sur la carte n'attend donc aucun asset.

## Pourquoi

L'interface de combat a été livrée sur le Colisée parce que le jeu n'avait pas d'exploration.
Le `LOT-09` l'a donnée ; il reste à poser le combat **dessus**. C'est la pièce de moteur la plus
lourde de la démo.

## Décisions de réalisation

### D1 — Une seule interface de combat, deux montages

Le Colisée (`hmi::ArenaModel`) et le combat sur la carte (`hmi::EncounterModel`) dérivent d'une
même vue-modèle, `hmi::CombatModel` : curseur, actions du tour, gestes du joueur, tours de l'IA,
ce que la grille montre. Le Colisée n'y ajoute que la composition des deux camps et sa carte
d'arène ; la carte n'y ajoute que la rencontre montée sur sa zone et la file des mouvements. Le
calque tactique de l'écran (`TacticalLayer`) est de même partagé. C'est ce qui tient le critère
« aucune régression du mode arène » : le code du Colisée a été **déplacé**, pas recopié.

### D2 — Le combat se joue sur la zone de combat, jamais sur une fenêtre inventée

`core::prepareMapEncounter` choisit la zone de combat qui contient le déclencheur, à défaut celle
qui contient le héros, et refuse sinon : une grille tactique se pose sur une zone que l'éditeur a
posée et contrôlée (`--check`), pas sur un rectangle découpé autour du héros qui mettrait la
moitié d'une maison dans le combat. Le héros garde sa case si elle est libre dans la zone, chaque
combattant celle que sa formation lui donne ; une place impossible se rapproche de ce qu'elle
voulait, et le journal le dit.

### D3 — Le héros reste où le combat l'a laissé

Le `LOT-18` restituait la position d'exploration après le combat, parce que le combat se jouait
*ailleurs*. Ici la carte **est** le champ de bataille : ramener le héros à la case d'avant
contredirait ce que le joueur vient de voir. L'instantané d'exploration (`core::ExplorationSnapshot`)
ne restitue plus que l'orientation ; la position est celle de la grille, ramenée sur la carte.

### D4 — Ce que le combat décide se rejoue dans le temps

La session est instantanée : un tour de l'IA se joue en un appel. Dessiner la grille telle
quelle montre des combattants qui se téléportent — c'est ce que l'auteur voyait quand « l'IA ne se
déplaçait jamais vers le joueur ». La file des mouvements (`hmi::CombatCueTrack`) reçoit les faits
au moment où ils se produisent — un pas et son chemin (`core::ArenaSession::setMoveObserver`),
une attaque, un coup encaissé, une chute (`core::CombatHook`) — et les rejoue à la vitesse du
monde : deux cases par seconde, une action le temps de sa bande, le coup qui porte au milieu du
geste. Les tours de l'IA se jouent un par un, chacun après que le précédent s'est vu ; tant qu'un
mouvement joue, les gestes attendent, et `Entrée` saute l'animation.

### D5 — Les six bandes se jouent, sur la carte gelée

Les combattants sont dessinés par la surface de rendu de l'exploration, à la place de ses
figurines (`hmi::WorldModel::setCombatFigures`) : même pipeline, même caméra. Une bande jouée une
fois (attaque, sort, touché, mort) se fige sur sa dernière image (`SceneTexture::loop`, lu dans le
`.anim.json`) ; un combattant précharge ses six bandes. Le sort (`cast`) est prêt et n'est appelé
par rien : les sorts arrivent avec les classes. Le repos respire désormais à l'arrêt, en
exploration aussi, chaque PNJ à son rythme.

### D6 — Un dialogue engage la rencontre ici même

L'action de dialogue `startEncounter` (champ `encounter`) s'ajoute à `startCombat` (le Colisée) :
le maître d'arène engage le combat nommé, sur la zone de combat de la carte, au point du PNJ. Une
entité `encounter` de la carte fait de même ; sa clé de drapeau (`core::encounterTriggerFor`) la
fait disparaître après une victoire, quand un combat engagé par un dialogue s'en remet aux
drapeaux de la quête (`LOT-116`).

### D7 — Les issues, et ce qui attend le LOT-119

Victoire : le drapeau est posé (`core::endEncounter`), l'exploration reprend. Fuite : idem, sans
drapeau, si la rencontre le permet. Défaite : le combat sur la carte est **létal** — la démo s'y
termine — et, tant que l'écran de mort n'existe pas, le panneau de l'issue ramène au menu. Les
points de vie du héros ne survivent pas au combat : il n'y a pas encore d'état de partie qui les
porte (la sauvegarde est à la `0.0.3`).

### D8 — Les mannequins tiennent la place des figurines absentes

Une créature de la rencontre sans figurine se dessine par le mannequin de sa silhouette
(`hmi::FigureResolver`, [LOT-145](../../v0.0.2-combat/lots/LOT-145-mannequins-de-remplacement.md)), et les PNJ des cartes
aussi. Le mannequin humanoïde SE-v1 de l'atelier est installé (kit `Common@2`), sa seule
orientation servie aux quatre.

## Audit de l'IA tactique

Demandé avec le lot : « dans des versions ultérieures elle ne se déplaçait jamais vers le
joueur ». Ce que la lecture et les tests ont établi :

- `core::planTurn` **approche bien** un joueur hors d'atteinte : l'approche la plus courte
  (`approcheLaPlusCourte`) pèse dans la clé de comparaison avant le score, et le test
  `SansAttaquePossibleChaqueProfilAvance` le garde pour les cinq profils. Un nouveau test le
  garde aussi sur la vraie zone de combat d'une carte, par la vue-modèle
  (`LEnnemiDeLIaMarcheVersLeJoueur`) : le sanglier marche vers le personnage à chaque tour.
- Les seuls cas où elle **tient sa place** sont voulus : aucun chemin vers une case adjacente à
  un ennemi (murs, allié qui bouche, gabarit Grand dans un couloir, vitesse nulle), ou une case
  d'approche qui excède les menaces tolérées du profil (`toleratedThreats`). Une créature sans
  attaque typée — le rat d'essai, avant ce lot — n'a rien à faire une fois au contact.
- Ce qui donnait l'**impression** d'une IA immobile venait de l'affichage : tous les tours de
  l'IA se jouaient d'un bloc dans `endTurn`, un seul instantané était pris ensuite, et aucune
  bande de marche n'était jouée — les ennemis sautaient à leur case d'arrivée. C'est la décision
  D4.
- Deux corrections : un déplacement refusé par la grille s'écrit désormais au journal
  (`deplacement refuse vers c,r`) au lieu de se perdre en silence ; et les profils de l'IA
  (`Rpg/rules/behaviors.json`) se chargent toujours à côté de l'exécutable, quel que soit le
  contenu.
- Rappel : dans le jeu livré, le Colisée n'a **aucune carte** depuis le `LOT-102` — la session
  n'existe pas, et la surface de l'arène lit un dossier `Assets/Coliseum` supprimé. Le combat sur
  la carte ne passe plus par là.

## Livraison

Livré le 25 septembre 2026, **PR #132**. Les trois critères tiennent par les tests (sans fenêtre :
`EncounterModelTest`, `MapEncounterTest`, `CombatCuesTest`, le rejeu à graine fixée par la session
commune, aucune régression du Colisée par `ArenaModelTest`). Reste due par l'auteur : la passe à
l'écran de `CombatHud` au clavier, à la manette et à la souris, sur la carte de test.

**Jouer la carte de test.** Le contenu livré n'a encore ni maître d'arène ni rencontre posée : la
carte de test est le `donjon` de la racine d'essai, que le jeu joue par l'option `--data=`
(comme l'éditeur par `LevelEditor --data`) :

```
JustAnotherRpgGame.exe --data=Source/Test/Fixtures/GameData --map=donjon@sable --at=24,19
```

Le héros paraît devant le maître d'arène d'essai (24, 20), dans la zone « salle » ; `E` lui parle,
« Qu'on les lâche » engage les rats sur la carte. L'éditeur ouvre la même carte :
`LevelEditor --data Source/Test/Fixtures/GameData`.
