# LOT-EDITOR-09 — Le monde : onglets, portails, ville {#lot-editor-09}

> Statut : **livré le 21 septembre 2026** (ouvert le même jour), sur la branche
> `lot-editor-09-monde`. Vérification automatisée : construction `/W4 /WX` sans avertissement,
> 899 tests unitaires verts (Release), `--check` des six cartes livrées à 0 erreur, le critère
> d'acceptation éprouvé sur les données livrées. **Reste due** : la vérification à la souris.
> Prérequis : `LOT-EDITOR-05`, `LOT-EDITOR-13`. Feuille de route : [éditeur](@ref roadmap-editeur).
> Exigences : `EX-EDIT-088` à `EX-EDIT-092` (nouvelles).

## Objectif

Jusqu'ici l'éditeur ouvrait **une** carte : en ouvrir une autre demandait d'abandonner ou
d'enregistrer la première, et comparer deux quartiers était impossible. Le monde, lui, ne se lisait
que d'un côté — le graphe des portails se **regardait**, et relier deux cartes demandait de poser à
la main un portail ici, un point d'arrivée là, puis de recommencer dans l'autre sens sans se
tromper de nom. Ce lot ouvre les cartes en **onglets**, rend le graphe **éditable au geste**, pose
les quartiers d'une ville sur son **plan**, et donne à chaque carte ses **propriétés** et son
**état** — à cent cartes, c'est le tableau de bord du monde.

## Ce que le dépôt contenait à l'ouverture (21 septembre 2026)

- **Un seul canevas**, membre de la fenêtre (`MainWindow::_viewport`), branché une fois pour
  toutes : une quarantaine de liaisons Qt le nommaient directement. Ouvrir une carte demandait
  « abandonner les modifications ? », puis remplaçait le brouillon.
- **Un seul brouillon de reprise à la fois** : `offerRecovery` mettait de côté les suivants, faute
  de place pour les ouvrir (« Une seule carte ouverte à la fois »).
- **Le graphe du monde** (`hmi::WorldGraphView`, `LOT-11`) montrait cartes et portails, et ouvrait
  une carte au double-clic ; aucun geste ne l'**écrivait**.
- **`world-maps.json`** portait déjà les cadres des quartiers (`LOT-96`) et `World/cities/*.json`
  le graphe d'une ville, mais seuls l'écran « Carte » du jeu les lisait.
- **Une carte ne portait aucune propriété qui lui soit propre** : le format v4 n'a de propriétés
  libres que sur les couches et les entités. Le lieu (`scene`) vit sur les couches ; la région et
  l'ambiance n'existaient nulle part, alors que le [LOT-28](@ref lot-28) les attend.
- **L'annexe** (`<carte>.editor.json`) ne portait que les notes d'auteur.

## Décisions

- **Un onglet, un brouillon vivant** (décision de l'auteur, 21 septembre) : chaque onglet a son
  canevas, son historique, sa sauvegarde automatique et sa garde de disque. *Écarté* : une barre
  d'onglets qui ne serait qu'une bascule sur un brouillon unique — on reperdrait le cadrage et la
  sélection à chaque aller-retour, et deux cartes ne se compareraient pas.
- **Les panneaux parlent au canevas actif, pas à un canevas donné.** Toutes les liaisons
  panneau → canevas passent désormais par `_viewport`, si bien qu'un changement d'onglet ne
  rebranche personne ; seules les liaisons canevas → fenêtre sont refaites, et elles sont gardées
  dans une liste pour être défaites d'un bloc (`bindViewport`, `unbindViewport`).
- **Le dernier onglet ne se ferme pas.** La fenêtre a toujours un canevas : aucun code n'a de cas
  « aucune carte ouverte », et la barre d'état, les panneaux et les actions gardent un sens.
- **Une carte n'est ouverte qu'une fois** : l'ouvrir de nouveau revient à son onglet. Deux
  brouillons de la même carte se contrediraient à l'enregistrement.
- **Le garde-fou des modifications quitte l'ouverture pour la fermeture.** Ouvrir n'écrase plus
  rien : c'est fermer un onglet, ou la fenêtre, qui demande quoi faire de chaque brouillon modifié
  — et l'onglet en question passe devant, on ne répond pas d'une carte qu'on ne voit pas.
- **Un renommage exige que *toutes* les cartes ouvertes soient enregistrées**, et chaque onglet se
  relit ensuite là où sa carte est ; celui qui portait la carte déplacée l'y suit (`renamedFrom`,
  `renamedTo`). Un brouillon non enregistré, dans n'importe quel onglet, écraserait le fichier que
  le plan vient de récrire.
- **Tirer un lien entre deux cartes pose quatre entités** : sur chacune, le portail qui mène à
  l'autre et le point d'arrivée où l'autre fait arriver. Un aller sans retour n'existe pas ici —
  c'est précisément ce que le contrôle du `LOT-EDITOR-07` reproche à une carte.
- **Le point d'arrivée dit d'où l'on vient** : `from-<dernier segment>` (`from-martpart`), suffixé
  `-2`, `-3` s'il est pris. *Écarté* : un nom demandé à l'auteur à chaque lien — un geste du graphe
  doit rester un geste ; le renommage propagé (`LOT-EDITOR-14`) le change en une commande.
- **La paire se pose au plus près de l'entrée**, sur deux cases **libres** (ni solides, ni
  occupées, ni l'entrée) et **atteignables depuis l'entrée** — à distance égale, la plus petite
  ligne puis la plus petite colonne. Deux cartes reliées se traversent donc **tout de suite**, et
  l'outil « Entité » déplace ensuite la porte où l'auteur la veut. *Écarté* : la première case
  libre en lecture de la carte, qui tombait dans un coin ; demander la case à l'auteur, qui ferait
  du geste du graphe une suite de clics sur deux cartes.
- **Un lien est un plan, comme un renommage** : `hmi::planLinkMaps` rend un `hmi::RefactorPlan` —
  la fenêtre montre ce qu'il écrit, `--link-maps` l'écrit sans fenêtre, et un refus (carte
  illisible, carte reliée à elle-même, carte sans deux cases libres) n'écrit rien.
- **La vue de ville se regarde, elle ne s'édite pas** (décision de l'auteur) : les cadres de
  `world-maps.json` et les quartiers de la ville, le nom de chacun, un double-clic qui ouvre sa
  carte. Un quartier sans carte est tireté et le dit. *Écarté* : tirer les cadres pour les
  repositionner et récrire `world-maps.json` — ce serait un second format écrit par l'éditeur, avec
  sa garde et ses tests, pour des cadres notés provisoires.
- **La région et l'ambiance sont des propriétés de la carte**, pas de l'annexe (décision de
  l'auteur) : `core::LevelData` reçoit une `PropertyMap` de **carte**, et le chargeur y range toute
  clé racine qu'il ne connaît pas — comme il le fait déjà pour une couche ou une entité. Le jeu
  peut donc les lire, ce qu'attend le [LOT-28](@ref lot-28). Une propriété qui porte le nom d'un
  champ du format est ignorée à l'écriture : le format fait foi.
- **Le lieu se voit dans les propriétés, il ne s'y édite pas** : en changer repeint la carte, et
  c'est *Map* › *Change sheet…* (`LOT-EDITOR-14`), avec sa table de correspondance.
- **Où en est une carte est une note d'auteur** : `generated`, `retouched`, `finished` vivent dans
  l'annexe, jamais dans la carte — le jeu n'en a que faire. Un mot inconnu vaut « rien dit ».
- **La vignette d'une carte est rendue par le peintre du canevas** (`hmi::renderMap`,
  `EX-EDIT-059`) et gardée sous la clé `fichier@horodatage` : une carte récrite perd la sienne, les
  autres la gardent. Les vignettes ne se rendent que si l'auteur les demande.
- **Un onglet neuf naît vierge** ; seul le premier montre la carte de départ, comme l'éditeur l'a
  toujours fait. `--map=` remplace cette carte de départ au lieu de s'ouvrir à côté d'elle.

## Livraison

| Partie | Contenu |
|---|---|
| `Core` | `LevelData::properties` et `Level::properties()` (propriétés de **carte**) ; le chargeur range toute clé racine inconnue, l'écrivain les réémet ; `LevelDraft::setProperty` (annulable) et `properties()` |
| `Editor/Logic` | `MapDocuments` : `OpenDocument`, `documentLabel`, `documentOf`, `documentAfterClose`, `dirtyDocuments` ; `WorldLinks` : `MapLink`, `arrivalNameFrom`, `linkCells`, `planLinkMaps` ; `CityView` : `CityView`, `buildCityView`, `cityIds`, `cityOfMap`, `districtAt`, `worldRegionIds` ; `EditorSidecar` : `MapState` et son mot dans l'annexe ; `runRefactorCommand` : `--link-maps` |
| `Editor/Ui` | `MainWindow` : les onglets (`addDocument`, `bindViewport`, `activateDocument`, `closeDocument`, `openMap`), la sauvegarde automatique et la garde de disque par onglet, *File* › *Close tab*, *Map* › *Map properties…*, `linkMaps` ; `WorldGraphView` : tirer un lien (`linkRequested`) ; `CityMapView` (nouveau) : le plan et ses quartiers ; `MapPropertiesDialog` (nouveau) ; `LevelBrowserPanel` : onglet « City », filtre d'état, vignettes ; `EditorViewport` : `StartContent`, `setMapProperty`, `setMapProperties`, `setMapState` |
| `App/Editor` | `--link-maps <carte> <carte>` ; `--map=` ouvre dans l'onglet du démarrage |
| Tests | `test_world_links.cpp` (4), `test_city_view.cpp` (3), `test_map_documents.cpp` (3) ; l'état dans `test_editor_sidecar.cpp`, les propriétés de carte dans `test_level_writer.cpp` |

## Critères d'acceptation

- **Relier deux cartes depuis le graphe produit deux cartes valides qu'on traverse dans les deux
  sens.** ✔ `DonneesLiens.RelierDeuxCartesSeTraverseDansLesDeuxSens` : sur une copie des données
  livrées, `capital/martpart` et `coliseum` reliées d'un geste ; chaque carte reçoit son portail
  vers l'autre (**résolu**) et le point d'arrivée que ce portail cite (`from-martpart`,
  `from-coliseum`), chacun **atteignable depuis l'entrée** de sa carte ; le contrôle du contenu ne
  rend aucune erreur de plus.
- **Deux liens entre les mêmes cartes se distinguent.** ✔
  `DonneesLiens.DeuxLiensEntreLesMemesCartesSeDistinguent` : le second nomme ses points d'arrivée
  `from-…-2`, et les quatre portails restent résolus.
- **La Capitale se voit par quartiers.** ✔ `VueDeVille.LaCapitaleSeVoitParQuartiers` : les
  quartiers de la ville, leur nom d'atlas, les cadres de `world-maps.json`, la carte de chacun et
  celles qui manquent ; `VueDeVille.UnClicSurLePlanDesigneUnQuartier` : un point du plan désigne le
  quartier dont le cadre le contient.
- **Une carte garde sa région, son ambiance et ses clés inconnues.** ✔
  `LevelWriterTest.UneCarteGardeSaRegionSonAmbianceEtSesClesInconnues` : elles traversent l'écriture
  et la relecture, le brouillon les défait d'un `Ctrl+Z`, et une valeur vide retire la propriété.
- **Les six cartes livrées restent canoniques et sans erreur** : `LevelEditor --check` (0 erreur,
  5 avertissements, ceux d'avant).

**Vérification à la souris, due** : lancer l'éditeur, ouvrir Martpart depuis le navigateur — un
onglet de plus, le premier reste. Peindre une case dans chacun, passer de l'un à l'autre : chaque
carte garde son cadrage, son outil et son historique, et le titre d'onglet porte l'étoile.
Onglet **Graph** du navigateur : tirer de `capital/martpart` à `coliseum` — la fenêtre montre les
quatre entités à écrire, puis le graphe porte les deux flèches. `P` pour l'essai, franchir le
portail dans un sens puis dans l'autre. Onglet **City** : le plan de la Capitale, Martpart et
Arenarea cadrés, les autres tiretés ; double-clic sur Arenarea, elle s'ouvre dans son onglet.
*Map* › *Map properties…* : mettre la région `central-empire`, l'ambiance `market`, l'état
*Retouched* ; `Ctrl+S`, puis cocher **Thumbnails** dans le navigateur et filtrer sur *Retouched* —
Martpart seule, avec sa vignette. Fermer un onglet modifié : la question porte sur **sa** carte.

## Ce qui reste hors du lot, nommément

- **Les cadres de quartier ne s'éditent pas** : ils sont notés provisoires dans `world-maps.json`,
  et c'est l'auteur qui les relève (`Ctrl+clic` sur l'écran « Carte » du jeu, `LOT-94`).
- **L'ambiance n'est pas jouée** : le [LOT-28](@ref lot-28) la lira ; ici elle s'écrit et se relit.
- **La région n'est pas contrôlée** contre l'atlas : une carte peut précéder sa région, et une
  valeur hors liste reste saisissable. Le jour où le voyage (`LOT-42`) s'en sert, le contrôle du
  contenu (`LOT-EDITOR-07`) la vérifiera.
- **Un lien ne choisit pas ses cases** : il les pose au plus près de l'entrée, et l'outil
  « Entité » les déplace. Poser une porte au bon endroit du premier coup reste un geste de la main.
- **Les onglets ne se rouvrent pas au lancement suivant** : une session ne garde pas sa liste de
  cartes ouvertes. Ce qui doit survivre est enregistré.
