# Éditeur de niveaux

> **Binaire séparé depuis le `LOT-86`** (`LevelEditor`), **module à part depuis le
> `LOT-EDITOR-01`** (`Source/Editor`). Il n'héberge aucun écran du jeu, qui vit en Qt Quick dans
> `JustAnotherRpgGame` : sa fenêtre s'ouvre directement sur la carte de départ, et son widget
> central est le canevas. C'est un outil interne : style Fusion, textes anglais, widgets construits
> en code. Son programme : la [feuille de route de l'éditeur](../../Planning/vision/archives/feuille-de-route-editeur.md).


Cette page explique comment l'éditeur transforme le modèle de carte déjà vu dans [Niveaux : modèle, couches, entités, chargement](guide-niveaux.md)
en un **outil de création de contenu**, sans écrire un second moteur. Le **modèle d'édition**
(mutabilité, validation, annuler/refaire, sérialisation) vit dans
`Source/Core/Levels/LevelDraft.*`/`LevelWriter.*` ; l'**interaction** (peinture souris, outils,
essai, garde-fous) vit dans le canevas Qt `Source/Editor/Ui/EditorViewport.*`. L'habillage de
l'IHM Qt lui-même — fenêtre, docks (Palette, Niveaux, Couches, Entités), arbre de palette,
navigateur de fichiers — est décrit dans [IHM Qt — deux applications, deux technologies](guide-ihm-qt.md) ; cette page se concentre sur ce qui est
**propre à l'édition**.

## Le problème : éditer une carte sans (re)coder le moteur

[Niveaux : modèle, couches, entités, chargement](guide-niveaux.md) a montré que `core::Level` est **immuable** une fois construit : ses champs sont
posés au constructeur, sans mutateur. C'est un choix délibéré — une carte **en cours de jeu** ne
doit jamais changer de forme sous les pieds du joueur. Mais un **éditeur**, par nature, fait
exactement l'inverse : poser une tuile, la retirer, déplacer l'entrée, doivent être des opérations
courantes, répétées des dizaines de fois par minute. Réutiliser `Level` tel quel pour l'édition
obligerait soit à le rendre mutable (fragilisant l'invariant « une carte chargée est valide », dont
dépend tout le reste du moteur), soit à dupliquer sa logique dans un second type — exactement ce
que `EX-EDIT-010` interdit (« aucune duplication de la logique de niveau »).

La solution retenue : un type **distinct**, `core::LevelDraft`, qui porte toute la mutabilité, et
qui ne redevient un `Level` **validé** qu'au moment décisif (l'enregistrement ou l'essai), en
repassant par le chemin de validation déjà existant plutôt que d'en écrire un second.

## `core::LevelDraft` : une carte qu'on peut défaire

`LevelDraft` reprend les mêmes données qu'un `Level` (nom, grille de tuiles, entrée, couches,
entités, pièces assignées) mais expose des **mutateurs** : `paintTile`, `paintRegion`, `setEntry`,
`resize`, ceux des couches visuelles (`addLayer`, `paintLayerTile`, `moveLayer`…) et ceux des
entités (`placeEntity`, `moveEntity`, `setEntityProperty`…). Deux invariants structurent tout le
reste de la page :

- **La grille de tuiles reste l'unique source de vérité.** Exactement comme pour `Level`
  ([Niveaux : modèle, couches, entités, chargement](guide-niveaux.md)), une case `Entry` dans `core::TileMap` **est** la donnée — `entry()` n'est
  qu'un accès en cache, toujours resynchronisé par les mutateurs. Peindre autre chose par-dessus
  l'entrée l'invalide automatiquement, et peindre un type différent sur une case retire la pièce
  qui lui était assignée : il ne peut jamais exister d'état où la grille dit une chose et le cache
  une autre.
- **`LevelDraft::toLevel()` ne réimplémente aucune règle de validation.** Plutôt que de vérifier
  « y a-t-il une entrée ? » une seconde fois, `toLevel()` **sérialise** le brouillon en JSON (via
  `core::LevelWriter`, ci-dessous) puis le fait passer par `core::LevelLoader::loadFromString` — le
  **même** chemin qu'un fichier chargé depuis le disque. Un brouillon incomplet produit donc
  exactement le même message d'erreur qu'un fichier de carte mal formé (`EX-LVL-004`), sans qu'une
  seule règle de `LevelLoader` n'ait été dupliquée.

## `core::LevelWriter` : l'inverse du chargement

Écrire une carte est l'inverse exact de `LevelLoader::loadFromString` ([Niveaux : modèle, couches, entités, chargement](guide-niveaux.md)) —
parcourir la grille ligne par ligne, émettre un objet JSON par tuile non vide (avec sa pièce
assignée, champ `"texture"`), puis les couches, les entités et leurs propriétés libres. Deux
conventions gardent l'aller-retour exact : l'entrée n'est écrite qu'**en tant que tuile** (jamais à
part), et une carte à grille unique (`Legacy`) n'émet aucun tableau `"layers"` — elle ressort telle
qu'elle est entrée.

Cette sérialisation sert `EX-EDIT-011` : sérialiser puis recharger une carte produit une carte
**équivalente**, jamais une carte différente — la propriété qui rend `toLevel()` fiable.

## Peindre, c'est convertir un pixel en case

Le canevas d'édition (`hmi::EditorViewport`, `LOT-EDITOR-02`) montre **le lieu tel qu'on le
jouera** : une `QGraphicsView` dont l'élément unique parcourt la liste de primitives que compose le
jeu (`hmi::composeWorldScene`, [Rendu 2D : de la scène à l'écran](guide-rendu.md)) et la peint par `QPainter`
(`hmi::paintComposedScene`), en ne touchant que la partie visible (`EX-EDIT-059`). Aucun second
moteur : la composition est celle du jeu, dans la cible `SceneComposition`, et un test compare
l'image du canevas au rendu GPU du jeu. Par-dessus viennent les aides d'édition : quadrillage en
losanges, case survolée, aperçu des outils, marqueurs d'entité, et le masque de collision quand on
peint la collision.

`F9` bascule vers la **vue à plat** (`hmi::DraftRenderer`) : une case par unité, une couleur par
type, les couches visuelles dans leur ordre et la collision en masque teinté — la vue qui lit les
types et la collision. `F8` passe les reliefs **en transparence**, pour voir ce qu'on pointe
derrière un mur ; le panneau des couches **grise** ou **verrouille** une couche, et la mini-carte
(« Overview ») montre toute la carte et le cadre de la vue (`EX-EDIT-061`).

La seule nouveauté conceptuelle est l'**interaction** : convertir une position souris en case de
grille. La vue la ramène en unités monde (`QGraphicsView::mapToScene`), puis le pointage pur
(`Editor/Logic/CanvasPicking.h`) désigne la case dont le **losange** est sous le pointeur — jamais
l'image qui la couvre : sous un mur haut, on pointe la case de derrière (`EX-EDIT-060`). À plat,
c'est `std::floor` (une position `4.7` désigne la case `4`). C'est le rôle de
`EditorViewport::cellAt` ; `paintAt` applique ensuite le type actif à la case survolée, dont la barre
d'état donne les coordonnées et les pièces.

### La palette et les outils : des panneaux Qt séparés du canevas

Palette et outils vivent dans des **panneaux dockables distincts** (`QDockWidget`) et dans la barre
d'outils, physiquement hors du canevas : aucune logique de priorité de clic n'est nécessaire pour ne
pas peindre une case cachée sous un panneau.

- la **Palette** (`hmi::PalettePanel`, un `QTreeView` alimenté par la taxonomie pure
  `hmi::tileTaxonomy`) émet le type sélectionné, que `MainWindow` relaie au canevas via
  `EditorViewport::setActiveTile` ;
- l'**outil actif** est choisi depuis la barre d'outils (`hmi::EditorActions`) et relayé via
  `EditorViewport::setTool` ;
- les panneaux **Couches** (`hmi::LayersPanel`) et **Entités** (`hmi::EntityPanel`) choisissent la
  couche peinte et renseignent l'entité sélectionnée ; ils demandent, le canevas — seul
  propriétaire du brouillon — applique.

La barre d'outils ne porte **que** la sélection d'outil et quatre commandes à usage continu —
Save, Playtest, Undo, Redo. Tout le reste vit dans la barre de menus, organisée par nature d'action
(File, Edit, Map, View, Help) : voir [Système de design et architecture de l'information](guide-design-ihm.md).

Le canevas ne reçoit donc que des **clics de grille** ; il n'a jamais à arbitrer entre « peindre »
et « cliquer un panneau ». Détail de ces widgets Qt : [IHM Qt — deux applications, deux technologies](guide-ihm-qt.md).

### Quatre outils, une même grille : `hmi::EditorTool`

Au-delà du pinceau (`Paint`, peindre case par case), l'éditeur propose **Rectangle** (glisser
définit un rectangle, rempli du type sélectionné au relâchement), **Sélection** (glisser mémorise
une zone, copier/coller la déplacent ailleurs) et **Entité** (poser, sélectionner, déplacer,
retirer une entité de carte — geste pur `hmi::resolveEntityPress`/`resolveEntityRelease`,
`Suppr` retire l'entité sélectionnée). Les quatre sont un simple `enum class hmi::EditorTool` ;
le canevas en tient l'état courant (`_tool`) et la mécanique de glisser (`_painting` pour le
pinceau, `_dragging` + `_dragStart` pour Rectangle/Sélection, `applyRectangle`,
`copySelection`/`pasteClipboard`). Changer d'outil **pendant** un glisser en cours l'annule plutôt
que de l'appliquer à moitié — un choix délibéré pour qu'un changement d'avis ne produise jamais de
mutation partielle et surprenante.

### Peindre par lot sans dupliquer la logique de peinture : `core::LevelDraft::paintRegion`

Remplissage rectangulaire et collage partagent le même besoin : appliquer un **bloc** de types de
tuiles en une seule fois, plutôt qu'une case. Une implémentation naïve dupliquerait la sémantique
de `paintTile` (déplacement de l'entrée, retrait des pièces assignées) pour chaque case du bloc.
`LevelDraft::paintRegion` évite cela en factorisant cette sémantique dans une méthode privée sans
`pushUndo()` (`paintTileInternal`), appelée une fois par case du bloc ; `paintTile` n'en est que la
version à une case. Résultat : remplir un rectangle de 50 cases ou peindre une seule case suivent
**exactement** le même chemin de code, et un bloc ne pousse **qu'un seul** instantané sur la pile
d'annulation pour toute l'opération — cohérent avec le principe « un geste = une mutation
undoable ».

Le **copier** n'a besoin d'aucun ajout à `Core` : le canevas lit directement la grille de la
couche active pour construire un presse-papiers local (`std::vector<std::vector<TileType>>`,
`_clipboard`). Seul le **coller** repasse par le brouillon.

## Annuler/refaire : pourquoi des instantanés complets

Chaque mutateur de `LevelDraft` empile, **avant** de s'appliquer, une copie complète de l'état du
brouillon (`snapshot()`) sur une pile d'annulation ; `undo()` la restitue et bascule l'état courant
sur la pile de refaire, symétriquement pour `redo()` (`Ctrl+Z`/`Ctrl+Y`). Une nouvelle mutation
après un `undo()` vide la pile de refaire — l'historique reste **linéaire**, jamais arborescent,
comme dans la plupart des éditeurs grand public.

Le choix d'un **instantané complet** plutôt que d'un enregistrement différentiel (« quelle case a
changé ») est délibéré : un différentiel serait plus économe en mémoire, mais demande une logique
d'inversion propre à **chaque** type de mutation (annuler un redimensionnement n'est pas l'inverse
symétrique de le refaire, par exemple). Les cartes de ce projet restent de taille modeste (quelques
milliers de cases au plus) : copier l'état entier à chaque étape est largement assez rapide, et la
garantie de correction (« l'état restitué est identique à l'octet près ») est bien plus simple à
établir qu'avec des deltas.

L'historique est **plafonné** à `LevelDraft::UNDO_HISTORY_LIMIT` pas (200) : au-delà, le plus ancien
est oublié (`EX-EDIT-058`). Un geste sans effet — repeindre une case déjà du bon type — n'empile
rien. Chaque instantané porte la **révision** du brouillon (`LevelDraft::revision`) : une mutation
en donne une neuve, `undo` et `redo` rendent celle de l'état qu'ils restaurent. C'est elle qui dit
si la carte est modifiée.

## Maquetter, jouer, puis habiller {#guide-editeur-maquette}

Une carte se dessine d'abord par sa **physique** — où l'on marche, ce qui bloque, qui attend où,
par où l'on sort — et se **joue** telle quelle, sans une seule pièce d'atelier (`LOT-128`,
`EX-EXP-005`). Les textures viennent après, sans rien refaire. C'est l'ordre de travail normal, pas
un mode dégradé : une erreur de tracé se paie alors en minutes, et non en pièces redessinées.

**1. Maquetter.** « New map » sans lieu — l'entrée *(none: colored tile types)* — ou le modèle
**Blockout**, qui pose déjà une enceinte et son ouverture. La carte reçoit ses couches dans tous
les cas : elle pourra recevoir un lieu plus tard. On peint ensuite avec la palette *Types*, dont
chaque vignette montre exactement la couleur que la case prendra, et l'on pose entités, portails et
zones comme sur n'importe quelle carte. La collision se déduit du type et suit chaque geste : il
n'y a rien à déclarer.

Ce que la maquette montre :

| Sur la carte | Rendu |
|---|---|
| une case qui ne nomme aucune pièce | un **losange plein**, à la teinte de son type (`hmi::maquetteColor`) |
| `wall`, `solid`, `cliff` | un **bloc** de trois faces, haut d'une case, qui masque ce qui est derrière |
| `deepWater` | un losange plat, plus sombre que l'eau vive : elle arrête le pas, elle n'arrête pas la vue |
| une entité sans figurine | un **jeton** rond à lettre — vert le joueur, jaune le PNJ qui parle, rouge l'hostile, gris le PNJ muet, gris-bleu coffre et panneau, or le portail |
| un portail | son jeton, surmonté d'une **flèche** |
| une zone, un îlot, une zone de combat | le **contour** de chacune de ses cases |
| un trajet | la **ligne brisée** de ses points de passage |

Les jetons paraissent dès qu'une figurine manque, maquette ou non. Contours, trajets et flèches ne
paraissent, eux, que sur une carte **sans lieu** : une carte finie ne montre pas ses déclencheurs.

**2. Jouer.** `P` pour l'essai immédiat, `F5` pour l'essai complet dans le vrai jeu : les deux
montrent la maquette, puisque c'est la **même** composition. On marche, on bute sur les murs, on
franchit les portails. C'est là que se voient une rue trop étroite ou un escalier mal placé.

`LevelEditor --render <carte>` en donne une image hors écran, jetons compris.
`--render --plan` la rend au **vocabulaire des plans de principe** du planning : blocs couchés à
plat, pastilles, et une légende des types et des natures de jeton employés. Un plan se lit, il ne
se joue pas — l'extrusion y cacherait justement ce qu'on vient y voir.

**3. Habiller.** `Change sheet…` donne un lieu à la carte. Collision, entités, portails et zones ne
changent **pas d'un octet** : seules les couches gagnent leur propriété `scene`, et chaque case
prend la pièce que la table du lieu donne à son type. Une case que la table ne couvre pas garde son
rendu de maquette, et `--check` le signale — un avertissement, pas une erreur : la case se voit,
elle n'est simplement pas encore habillée.

L'annexe `<carte>.editor.json` note où en est la carte : `blockout` avant `retouched` et
`finished`. « Livré », pour une carte, veut toujours dire *avec son lieu et ses pièces*.

## Essai immédiat : jouer sans quitter l'éditeur

Appuyer sur `P` lance une **vraie** exploration sur la carte en cours d'édition, puis, à `Échap`,
**revient exactement où l'édition en était**. Le canevas a deux états, jamais mêlés
(`EditorViewport::startPlaytest`/`stopPlaytest`) : en édition, il peint le brouillon ; en essai,
la carte est jouée par `hmi::WorldPlay` et composée comme dans le jeu, caméra sur le héros — la
**même** mise en scène que le jeu, partagée avec `hmi::WorldModel` (`EX-EDIT-055`). Un essai qui montrerait
autre chose que le jeu ne vérifierait rien.

Pendant l'essai, le canevas lit lui-même le clavier avec les touches du jeu : flèches, `ZQSD` ou
`WASD` pour marcher, `E` ou `Espace` pour interagir, `Échap` pour revenir à l'édition. L'éditeur
n'ouvre ni dialogue ni combat : ce que le jeu ferait (dialogue, rencontre, portail verrouillé…)
est annoncé dans la barre d'état, et l'essai continue.

**Transmettre la carte, pas un chemin de fichier.** `startPlaytest` valide le brouillon
(`draft.toLevel()`) et donne à `WorldPlay` un chargeur qui sert **d'abord le brouillon** sous
l'identifiant de sa carte, puis toute autre carte depuis le disque, comme en jeu : aucun fichier
temporaire n'est écrit, et un portail qui ramène ici retrouve le brouillon, pas le fichier d'avant.
Le `LevelDraft` et son historique ne sont, à aucun moment, touchés.

## Enregistrer : valider avant d'écrire, jamais l'inverse

`Ctrl+S` (`EditorViewport::save`) appelle `draft.toLevel()` en premier. Si la validation échoue,
**aucun fichier n'est écrit** — le brouillon invalide reste en mémoire, et le motif du refus est
émis via le signal `statusMessage` (barre d'état de la fenêtre). Si elle réussit,
`LevelWriter::saveToFile` écrit le JSON dans le dossier `Levels` de l'application — le **même**
dossier que le jeu lit ([Niveaux : modèle, couches, entités, chargement](guide-niveaux.md)), garantissant qu'une carte enregistrée est
immédiatement jouable. Avant d'écrire, la fenêtre vérifie que le fichier n'a pas changé sur disque
depuis sa lecture (voir ci-dessous).

## Garde-fous contre la perte de travail

**Ouvrir ou fermer en écrasant un travail non enregistré.** Le canevas est « modifié » quand la
révision du brouillon n'est plus celle de l'ouverture ou du dernier enregistrement (`isDirty()`,
`EX-EDIT-058`) : ouvrir une autre carte pose alors une confirmation, et fermer la fenêtre demande
Save, Discard ou Cancel.

**Planter.** Deux secondes après le dernier geste, un brouillon modifié est écrit dans
`%LOCALAPPDATA%\JustAnotherRpgGame\Editor\autosave` (`hmi::AutosaveStore`, `EX-EDIT-056`) ; le
fichier part à l'enregistrement et à la fermeture voulue. Au démarrage, un fichier restant est la
trace d'une session interrompue : l'éditeur propose « Recover » ou « Discard », et ce qui est écarté
va dans `autosave\conflicts\`, jamais effacé. `LevelEditor --crash-test` plante juste après la
première sauvegarde automatique : c'est la façon d'éprouver la reprise.

**Voir sa carte changer sur disque.** L'éditeur retient l'empreinte de contenu du fichier qu'il a
lu ou écrit (`hmi::FileFingerprint`, `EX-EDIT-057`) et la compare quand le fichier est signalé
changé, quand la fenêtre reprend la main et avant d'enregistrer. Brouillon intact : la carte est
relue. Brouillon modifié : « Reload from disk » ou « Keep my version », et l'autre version est mise
de côté dans `autosave\conflicts\` avant tout. Un script ou Claude peut donc retoucher une carte
ouverte sans que l'éditeur l'écrase en silence.

**Redimensionner en perdant du contenu.** Détecter un redimensionnement destructeur ne duplique
aucune règle de `resize` : une requête pure, `LevelDraft::wouldResizeDropContent(largeur, hauteur)`
(exposée par le canevas via `wouldResizeDrop`), inspecte les positions actuelles de l'entrée, des
entités et des pièces assignées contre les nouvelles bornes, **sans rien modifier** — la fenêtre
l'interroge avant d'appeler `resizeLevel`, et pose une confirmation si du contenu serait perdu.
Réimplémenter cette détection côté `HMI` aurait demandé de rederiver la même logique de troncature
que `resize` porte déjà ; l'exposer comme requête pure évite cette duplication (`EX-EDIT-010`).

## Cadrer une carte plus grande que la fenêtre

Le cadrage automatique du canevas passe par une fonction **pure**, `Camera2D::fitZoom(largeurDisponible,
hauteurDisponible, largeurContenu, hauteurContenu, marge)` :

```cpp
const float rawZoom = (std::min)(fitX, fitY) * margin;
return rawZoom >= 1.0F ? std::floor(rawZoom) : rawZoom;
```

Zoom **entier** (netteté des pixels) tant que l'ajustement brut reste `≥ 1` ; zoom
**fractionnaire** (la valeur brute, sans `floor`) uniquement lorsque c'est strictement nécessaire
pour qu'une carte plus grande tienne malgré tout. La placer dans `Camera2D` ([Rendu 2D : de la scène à l'écran](guide-rendu.md))
plutôt que dans le canevas la rend en prime testable sans GPU.

**Caméra manuelle et grille de repère.** Molette (zoom) et glisser prennent le relais du cadrage
automatique (`updateEditCamera`) ; `0` le rétablit (`resetCamera`), et `F10` bascule un
**quadrillage de repère** — fines lignes à chaque bord de case. La capture de ces touches passe par
les actions Qt uniques de `hmi::EditorActions` ([Entrées et actions logiques](guide-entrees.md)), jamais par un second
traitement dans le canevas.

## Gérer ses fichiers de niveaux

Le panneau **Niveaux** (`hmi::LevelBrowserPanel`) liste les fichiers `.json` du dossier `Levels` et
offre **créer / renommer / dupliquer / supprimer** ; un second onglet montre le **graphe du monde**
(`hmi::WorldGraphView`), les cartes du dossier et leurs portails. Le nommage est validé par
`hmi::isValidLevelName` — refus d'un nom vide ou contenant un caractère interdit par le système de
fichiers Windows (liste **noire** minimale ; les accents restent autorisés). Les opérations
fichiers elles-mêmes sont une couche **pure et testée**, `hmi::LevelFileOperations`
(créer/renommer/dupliquer/supprimer, sans dépendance Qt) — la même séparation « logique pure /
accès disque » que `LevelLoader`/`Core` appliquent à la validation. Détail du panneau :
[IHM Qt — deux applications, deux technologies](guide-ihm-qt.md).

## Voir aussi
- `core::LevelDraft` (dont `paintRegion`, `wouldResizeDropContent`), `core::LevelWriter`,
  `core::LevelLoader` (dont `LevelValidationError`).
- `hmi::EditorViewport`, `hmi::EditorTool`, `hmi::PalettePanel`, `hmi::tileTaxonomy`,
  `hmi::LayersPanel`, `hmi::EntityPanel`, `hmi::LevelBrowserPanel`, `hmi::LevelFileOperations`,
  `hmi::isValidLevelName`.
- `hmi::WorldPlay`, `hmi::composeWorldScene` — la mise en scène partagée par le jeu et l'essai.
- `hmi::paintComposedScene`, `hmi::SceneImages`, `hmi::pickIsoCell`, `hmi::isoBandOpacity`,
  `hmi::MiniMap` — le canevas qui montre le lieu (`LOT-EDITOR-02`).
- [IHM Qt — deux applications, deux technologies](guide-ihm-qt.md) — l'IHM Qt : fenêtre, docks, arbre de palette, navigateur de fichiers, canevas.
- [Système de design et architecture de l'information](guide-design-ihm.md) — l'éditeur outil interne, la barre d'état, le regroupement des panneaux et l'unicité des
  commandes de l'éditeur.
- [Niveaux : modèle, couches, entités, chargement](guide-niveaux.md) — le modèle de carte immuable, la validation et le format JSON réutilisés sans
  duplication.
- [Rendu 2D : de la scène à l'écran](guide-rendu.md) — la composition d'un lieu (`hmi::ComposedScene`, `hmi::composeWorldScene`), que
  le canevas peint par `QPainter` comme le jeu la soumet au GPU.
