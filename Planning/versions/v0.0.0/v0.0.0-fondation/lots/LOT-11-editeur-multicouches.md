+++
id = "LOT-11"
titre = "Éditeur multi-couches et placement d'entités"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "XL"
resume = "L'éditeur hérité édite les trois couches et pose des entités avec leurs propriétés : le contenu du RPG se produit sans écrire de JSON à la main."
prerequis = ["LOT-04", "LOT-08", "LOT-10"]
livrables = [
  "Les mutateurs de `core::LevelDraft` (couches visuelles et entités), tous annulables ; `core::isVisualLayerKind` et `core::isVisualLayerTileType` (`TileLayer.h`).",
  "La table des familles d'entités (`Core/World/EntityKinds.{h,cpp}`), `makeEntity` et `core::validateMapEntities` ; le contrat `core::PORTAL_ENTITY_TYPE` / `core::SPAWN_POINT_ENTITY_TYPE`.",
  "Le terrain tactique (`Core/Combat/TacticalTerrain.{h,cpp}`, `core::analyzeEncounterTerrain`).",
  "Le graphe du monde statique (`Core/World/WorldGraph.{h,cpp}`, `core::loadWorldGraph`, `unreachableFrom`) et sa vue dans le navigateur de cartes (onglet « Graphe »).",
  "Dans `LevelEditor` : le panneau « Couches », le rendu multi-couches du brouillon (`hmi::DraftRenderer`), l'outil « Entité » et le panneau « Entités » avec leurs avertissements.",
  "Les marqueurs générés par famille (`hmi::entityMarkerKey`, `TextureCache::markerTexture`), la zone et la formation de la rencontre dessinées sur la carte.",
  "L'essai immédiat avec entités et les catalogues de références (`hmi::loadEditorReferences`).",
  "Correctif `fix(hmi)` : `MainWindow` ne construit plus ses panneaux deux fois.",
]
criteres = [
  "**Édition des trois couches avec undo/redo complet.** ✔ `EditionDeCarteTest` (douze tests : promotion, peinture, refus, bloc en un pas, ordre, retrait, nom et rôle, entités) et le parcours système, qui annule tout jusqu'à la carte vierge.",
  "**Pose d'un PNJ, d'un coffre et d'un portail, avec leurs propriétés, puis essai immédiat.** ✔ en automatique pour la chaîne complète (`ParcoursEditionSysteme.ProduitUneCarteDuRpgSansEcrireDeJson` : couches, PNJ au dialogue du héraut, coffre, point d'arrivée et portail, rencontres, validation contre les catalogues livrés, enregistrement, graphe du monde, peuplement ECS) et pour les comptes rendus de l'essai (`PlaytestInteractionTest`) ; **à vérifier à la main** dans `LevelEditor`.",
  "**Aucune régression sur l'édition existante.** ✔ `ctest` vert, `LevelDraftTest` et `CouchesDeCarteTest` inchangés ; une carte à grille unique se dessine comme avant.",
  "**Avertissement visible quand une zone de rencontre n'est pas un terrain tactique valide.** ✔ `TacticalTerrainTest` (dix tests) et `EditionEntitesTest.AvertissementsDeLEditeur` ; la zone et la formation se voient sur la carte.",
  "**Vérification IHM manuelle par l'utilisateur.** ⏳ À faire (parcours détaillé dans la rubrique « Vérification »).",
]
+++

## Pourquoi

Rendre l'éditeur hérité capable d'éditer les **trois couches** et de poser des **entités** avec
leurs propriétés — c'est-à-dire de produire le contenu du RPG sans écrire de JSON à la main.

## Périmètre

### Ce que ce lot livre, dans le `Core`

- **Les mutateurs du brouillon** (`core::LevelDraft`) : couches visuelles ajoutées, retirées,
  renommées, changées de rôle, réordonnées et peintes (case ou bloc) ; entités posées, déplacées,
  retirées, renseignées (`setEntityProperty`, `removeEntityProperty`), désignées par case
  (`entityAt`). **Tous annulables**, et aucun geste refusé ou sans effet n'empile un pas.
- **`core::isVisualLayerKind` et `core::isVisualLayerTileType`** (`TileLayer.h`) : ce qu'une couche
  d'image accepte, décidé par un `switch` exhaustif.
- **La table des familles d'entités** (`Core/World/EntityKinds.{h,cpp}`) : coffre, panneau, PNJ,
  rencontre, portail, point d'arrivée, entrée d'arène, avec leurs propriétés, leur nature (texte,
  entier, booléen, choix) et la source de leurs choix ; `makeEntity` ; et
  **`core::validateMapEntities`**, qui relève sans refuser les références cassées (dialogue,
  rencontre, carte, point d'arrivée), les propriétés manquantes, mal typées ou hors liste, et les
  points d'arrivée en double.
- **Le terrain tactique** (`Core/Combat/TacticalTerrain.{h,cpp}`,
  `core::analyzeEncounterTerrain`) : pour chaque rencontre posée, la formation placée par la règle
  même du montage, et la zone atteignable en un déplacement.
- **Le graphe du monde** (`Core/World/WorldGraph.{h,cpp}`, `core::loadWorldGraph`) : les cartes d'un
  dossier, leurs points d'arrivée, leurs portails et le statut de chacun ; `unreachableFrom`. La
  partie **statique** du fichier que le `LOT-09` prévoyait : il y ajoutera le chargement à chaud et
  la traversée.

### Ce que ce lot livre, dans `LevelEditor`

- **Le panneau « Couches »** : la grille racine en tête — « Grille unique (image et collision) » ou
  « Collision » —, puis les couches visuelles de la plus en avant à la plus en arrière ; couche
  active, case de visibilité, curseur d'opacité, ajout d'un sol ou d'un décor, retrait (confirmé,
  annulable), montée, descente, renommage en place.
- **Le rendu multi-couches** du brouillon (`hmi::DraftRenderer`) : les couches visuelles comme en
  jeu, chacune à son opacité, et la collision **par-dessus en masque coloré par catégorie**.
- **L'outil « Entité »** (icône d'épingle) et **le panneau « Entités »** : la famille à poser, la
  liste des entités, le formulaire de l'entité sélectionnée, et les avertissements — références
  cassées et terrain tactique —, un double-clic sélectionnant l'entité en cause.
- **Les marqueurs générés** du [LOT-39](LOT-39-cles-assets.md) : chaque famille a le sien
  (`hmi::entityMarkerKey`, `TextureCache::markerTexture`), dans le canevas comme dans l'essai.
- **La zone et la formation** de la rencontre sélectionnée, sur la carte, avec l'outil « Entité » :
  la zone en bleu (en orange si elle est trop étroite), chaque case voulue en vert, ou en rouge si
  le montage la refuserait.
- **La vue du graphe du monde** dans le navigateur de cartes (onglet « Graphe »).
- **L'essai immédiat avec entités** : les entités posées s'y voient et répondent à la touche
  d'interaction — coffre ouvert une fois, panneau lu, PNJ et son dialogue, portail et sa
  destination — par une ligne au HUD qui dit aussi ce que l'essai ne sait pas encore faire.
- **Les catalogues** (`hmi::loadEditorReferences`) : lus au démarrage, puis à chaque enregistrement
  de carte et à chaque rechargement des assets.

### Ce qui reste hors du lot, nommément

- **La traversée des portails, le chargement à chaud, la validation au chargement** : le
  [LOT-09](LOT-09-colisee-premiere-carte.md), sur le contrat et le graphe statique posés ici.
- **Ouvrir le dialogue d'un PNJ dans l'essai** : l'essai dit quel dialogue le PNJ ouvrirait ; le
  jouer est l'affaire de l'exploration du jeu Qt Quick (`LOT-27`).
- **Le contenu du coffre** : le butin du `LOT-26`.
- **Le terrain difficile peint en zone** (`difficultTerrain` d'une couche) n'entre pas dans le
  calcul de la zone tactique, qui lit la grille de collision seule.
- **« Modifié » affiché à l'ouverture de l'éditeur**, sans modification : observé sur la version
  d'avant le lot, donc hors de celui-ci.

## Conception

### La décision préalable, tranchée

La feuille de route interdisait de démarrer avant que le §8 ait tranché **où vit l'édition** : dans
la scène, depuis le jeu (ce que voulait le §10), ou dans `LevelEditor`, l'exécutable Qt Widgets que
le [LOT-86](LOT-86-refonte-hmi-quick.md) a séparé du jeu (la recommandation de l'audit).

**L'auteur a tranché le 16 septembre 2026 : retarger `LevelEditor`.** `EX-EDIT-030` est refondue en
conséquence — l'outil d'auteur est un exécutable distinct, qui partage le code du jeu mais pas sa
technologie d'interface, et le mode intégré au jeu est l'arène du [LOT-50](LOT-50-colisee.md), un bac à
sable de débogage. `EX-EDIT-031` ne parle plus de Direct3D 11, que le rendu atteint depuis le
`LOT-69` hérité au travers de QRhi. La boucle « poser → essayer » que l'édition dans la scène
promettait reste servie par l'essai immédiat, que ce lot étend aux entités (`EX-EDIT-055`).

### Ce que la feuille de route supposait, et ce que le dépôt contenait

L'état des lieux fait au démarrage a trouvé six écarts. Ils décident de la forme de la livraison.

- **Les portails et les points d'arrivée n'existaient nulle part** — ni dans la donnée, ni dans le
  code : le mot « portail » ne vivait que dans des commentaires, et le seul « point d'apparition »
  était la tuile d'entrée. Le lot devait « poser et éditer les portails (carte cible, point
  d'arrivée) » : il en a d'abord écrit le **contrat** (`core::PORTAL_ENTITY_TYPE`,
  `core::SPAWN_POINT_ENTITY_TYPE`, `niveaux.md`), que le [LOT-09](LOT-09-colisee-premiere-carte.md) traversera.
- **`core::LevelDraft` portait couches et entités sans aucun mutateur** — « en attendant
  l'outillage d'édition du `LOT-11` » —, et l'éditeur ne peignait que la grille racine.
- **Aucun code de l'interface ne lisait les entités d'une carte**, pas même l'essai immédiat : un
  coffre posé à la main dans le JSON ne se voyait nulle part hors du jeu Qt Quick, qui n'a pas
  d'exploration.
- **Le panneau « Propriétés » avait été retiré** au `LOT-01` ; il n'en restait qu'un commentaire.
- **Le navigateur de cartes était une liste plate**, et aucun graphe de cartes n'existait : seul
  l'atlas des régions (`core::Atlas`) relie des lieux.
- **`MainWindow` construisait tous ses panneaux deux fois.** Le bloc du constructeur qui appelle
  `buildUi()` et branche les panneaux y figurait deux fois, à l'identique : chaque signal était
  relié deux fois, et la barre d'outils montrait chaque outil en double (captures avant et après).
  Corrigé dans un commit à part (`fix(hmi)`), la seconde copie retirée.

### Les gestes

| Geste | Effet |
|---|---|
| Clic sur une ligne du panneau « Couches » | Rend la couche active : pinceau, rectangle, copie et collage la visent |
| Case à cocher, curseur d'opacité | Montre, masque, estompe la couche (aide d'édition, rien d'enregistré) |
| Double-clic sur le nom d'une couche visuelle | La renomme (annulable) |
| Choisir une famille dans « Poser » | Arme l'outil « Entité » |
| Outil « Entité » : clic sur une case libre | Pose la famille choisie, et la sélectionne |
| Outil « Entité » : clic sur une entité, glisser | La sélectionne, puis la déplace au relâchement |
| `Ctrl` + clic sur une case occupée | Pose quand même |
| `Suppr` (outil « Entité ») ou « Retirer » | Retire l'entité sélectionnée |
| Double-clic sur un avertissement | Sélectionne l'entité en cause |
| Double-clic sur une carte du graphe | L'ouvre, avec le garde-fou des modifications non enregistrées |
| `P`, puis la touche d'interaction face à une entité | Essai immédiat, et compte rendu de l'interaction |

## Décisions de réalisation

Décisions prises en route.

- **La collision reste la grille racine**, la seule qui porte l'entrée, la sortie et les
  mécanismes ; une couche visuelle **refuse** ce qui porte une règle (entrée, sortie, mécanismes,
  danger, bloc) et le dit. Une porte peinte sur le décor serait une porte qu'aucun interrupteur
  n'ouvre, et rien ne le montrerait.
- **Ajouter la première couche visuelle recopie l'image de la grille unique**, types de règle mis à
  part. Dès qu'une carte a une couche visuelle, le jeu ne dessine plus sa grille racine
  (`core::buildLevelScene`) : sans la recopie, le premier clic sur « Ajouter un sol » effacerait
  toute la carte à l'écran. Retirer la dernière couche visuelle rend à la grille son rôle d'image.
- **Visibilité et opacité suivent le rang des couches**, pas leur identité, et ne sont ni annulables
  ni enregistrées : le format ne donne pas d'identité à une couche, et en inventer une pour une aide
  d'édition ne se justifiait pas. Une annulation qui réordonne les couches peut laisser un réglage
  sur la voisine ; un clic le répare.
- **La collision se montre en masque, pas en image**, par-dessus les couches visuelles, à 55 %
  d'opacité par défaut : rouge l'obstacle, orange le danger, vert l'entrée, bleu la sortie, jaune le
  mécanisme. Deux images superposées de la même case ne se liraient pas.
- **La table des familles rassemble, elle n'invente pas.** Ses types et ses propriétés sont ceux que
  le gameplay lit déjà (`knownInteractableKinds`, `dialogueTriggerFor`, `encounterTriggerFor`,
  `arenaEntryPoints`) ; le coffre n'a pas de contenu avant le butin du `LOT-26`. Un type absent de
  la table reste légal : l'éditeur le montre, et en transporte les propriétés sans les éditer.
- **Un portail désigne `(carte, point d'arrivée nommé)`**, jamais des coordonnées, et
  **l'identifiant d'une carte est le nom de son fichier** : c'est déjà ce qu'écrit l'enregistrement
  (`<nom>.json`) et ce que liste le navigateur.
- **Avertir, ne pas refuser.** Une carte s'écrit dans le désordre — le portail vers la forêt avant
  la forêt. Refuser d'enregistrer imposerait un ordre de travail ; c'est au chargement du graphe
  (`LOT-09`) qu'un portail orphelin deviendra une erreur. Les points d'arrivée de la carte éditée
  sont pris **dans le brouillon**, pour qu'un portail vers un point qu'on vient de poser ne soit pas
  signalé avant l'enregistrement.
- **Une seule règle pour « se tenir ici ».** Le terrain tactique place la formation par
  `core::BattleGrid::place`, exactement ce que fait le montage d'une rencontre : un avertissement
  de l'éditeur et un refus en jeu ne peuvent pas diverger. La zone est une `core::ReachableArea` :
  diagonales et coins suivent la règle du [LOT-19](LOT-19-grille-tactique.md).
- **Trois seuils nommés** pour « trop étroit » : un groupe de **quatre**, une zone d'**un
  déplacement** (6 cases, 30 pieds), **quatre** cases libres par combattant. Ce sont des seuils
  d'auteur, pas des règles du Manuel, écrits là où ils se règlent (`TacticalTerrain.h`).
- **Le formulaire est dérivé de la table** : une famille ou une propriété ajoutée à
  `knownEntityKinds` apparaît dans le panneau sans le toucher. Un choix de catalogue reste
  **éditable** — on nomme une carte qu'on écrira ensuite —, un choix fixe ne l'est pas.
- **Un portail n'est interactif qu'en essai.** L'essai lui donne un `core::Interactable` pour qu'on
  vérifie sa destination ; la table du jeu (`knownInteractableKinds`) n'est pas touchée, et le jeu
  ne gagne pas une interaction qui ne ferait rien. Le `LOT-09` l'y ajoutera.
- **Les marqueurs de l'essai se dessinent au-dessus du personnage**, dans une scène à part triée par
  profondeur entre eux : les entrelacer avec le personnage demanderait un crochet dans
  `hmi::SpriteRenderer`, hors du périmètre. Une limite d'essai, sans effet sur le jeu.

## Vérification

**Vérification IHM manuelle par l'utilisateur.** ⏳ À faire : ouvrir `LevelEditor`, ouvrir l'arène
du futur ; ajouter un sol, le peindre, en baisser l'opacité ; choisir « PNJ » dans « Poser », le
poser, lui choisir le dialogue du héraut ; poser un coffre, un point d'arrivée nommé et un portail
qui y mène ; poser une rencontre dans un recoin et lire l'avertissement ; enregistrer, regarder
l'onglet « Graphe » ; `P`, marcher jusqu'au coffre et au portail, interagir.

## Bilan

Statut : **livré, en attente de la vérification IHM manuelle**. Vérification automatisée : build
Debug `/W4 /WX` sans avertissement, `ctest` à **1278/1278** (1145 au `LOT-24`), lints d'exigences
et de lots verts, cahier de test régénéré ; captures de `LevelEditor` relues.

Ce que chaque prérequis apportait : [LOT-04](LOT-04-format-v3-multicouches.md) (le format à couches et entités),
[LOT-08](LOT-08-tuiles-rpg.md) (le vocabulaire de terrain), [LOT-10](LOT-10-entites-de-carte.md) (les entités de carte et
l'interaction). Alimente [LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27) (le contenu du slice se produit dans l'éditeur),
`LOT-40` (le générateur écrit des cartes que l'éditeur ouvre) et `LOT-69` (retrait de l'atelier
pixel art).

Exigences couvertes : `EX-EDIT-030` et `EX-EDIT-031` (**refondues**) ; `EX-EDIT-048` (trois
couches), `EX-EDIT-049` (visibilité et opacité), `EX-EDIT-050` (outil et panneau « Entités »),
`EX-EDIT-051` (références proposées et averties), `EX-EDIT-052` (portails et points d'arrivée),
`EX-EDIT-053` (graphe du monde), `EX-EDIT-054` (terrain tactique), `EX-EDIT-055` (essai avec
entités) — **ajoutées**, `editeur-niveaux.md` §12 ; `EX-LVL-016`, `EX-LVL-017`, `EX-EDIT-011`
(transport de ce que l'éditeur ne sait pas éditer).
