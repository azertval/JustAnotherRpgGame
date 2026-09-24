+++
id = "LOT-124"
titre = "L'éditeur et l'arborescence par niveaux"
version = "0.0.1"
filiere = "editeur"
statut = "livre"
taille = "L"
resume = "Une carte puise dans son lieu **et** dans ses niveaux communs — sous-zone, zone, ville, région, monde — et l'éditeur le montre, le contrôle et le réécrit."
prerequis = ["LOT-102", "LOT-123"]
livrables = [
  "`core` : un **catalogue de pièces résolu** pour un lieu hiérarchique (`central-empire/capital/arenarea/arena-of-fate`), qui empile les manifestes du plus propre au plus commun et garde, pour chaque pièce, son dossier d'origine ; le fichier vient du champ `file` du manifeste, plus du nom.",
  "`LevelDraft`, `PieceCatalog`, `CollisionDerivation` et `--check` lisent ce catalogue : une pièce du kit de la Capitale posée sur Arenarea déduit sa collision et passe le contrôle.",
  "La palette groupée par **niveau** (« Arena of Fate », « Arenarea », « Capital », « Central Empire », « World »), avec la recherche à travers tous ; une pièce propre qui **masque** une pièce commune de même clé est signalée.",
  "« New map » propose l'arbre des lieux, et range la carte sous le même chemin dans `Levels/`.",
  "`--replace-piece`, `--who-cites piece` et `--change-scene` suivent les niveaux : **promouvoir** une pièce au commun ne réécrit que les cartes qui changent vraiment.",
  "Les préfabriqués et les modèles rangés par niveau, proposés à tout lieu qui en descend.",
]
criteres = [
  "Une carte de test sous `capital/arenarea/` cite une pièce de chacun des quatre niveaux ; `--check` passe, `--render` la montre, l'essai dans le jeu l'affiche.",
  "Une pièce déplacée de la zone vers la ville, sous la même clé, ne change **aucun** octet des cartes ; sous une autre clé, `--replace-piece` réécrit les cartes qui la citent et elles seules.",
  "`git grep '\"Scene/\"' Source/Editor Source/HMI/Graphics` ne trouve plus de chemin d'asset composé à la main.",
  "Un scénario `--apply` couvre la pose d'une pièce commune ; l'auteur a composé une rue de douze cases avec le kit seul (critère du LOT-105) **dans l'éditeur**.",
]
+++

## Pourquoi

L'[arborescence des assets](../../../../standards/arborescence-assets.md) repose sur une règle — du
commun vers le propre — que l'éditeur ne sait pas jouer : un brouillon porte **un** manifeste, la
palette **un** dossier, et `--check` cherche chaque pièce dans ce seul manifeste
([audit](../../../../standards/audit-editeur.md), constats A1 à A5). Sans ce lot, chaque quartier
devrait recopier le pavé de la Capitale dans sa planche : exactement ce que le standard interdit.

## Périmètre

Le [LOT-102](LOT-102-table-rase-assets-et-cartes.md) pose la résolution d'une clé côté **moteur**
(son point 4) ; ce lot la fait lire par tout ce qui, dans `Core` et dans l'éditeur, supposait un
manifeste unique. Le livrable « palette » que portait le LOT-104 est **ici**.

**Pas dedans** : l'affichage HD des pièces (LOT-125) ; l'installation des assets (LOT-104).

## Risques

- `appearance.json` (type de tuile → pièce par défaut) n'a pas de repli non plus : même règle, le
  plus propre gagne.
- Le format de carte ne change pas : une couche garde **un** `scene`, qui devient un chemin. Si une
  carte devait nommer le niveau d'une pièce, ce serait une révision de format — à refuser.

## Réalisation — 24 septembre 2026

Branche `lot-124-arborescence-par-niveaux`. Une seule question se pose désormais à une pièce —
`find(nom)` — et c'est le **catalogue résolu** du lieu qui y répond : les consommateurs d'un
manifeste (brouillon, déduction de collision, palette, contrôle, rendu) n'ont pas changé de forme,
ils reçoivent un manifeste empilé au lieu d'un manifeste seul.

| Livrable | Ce qui est fait |
|---|---|
| Catalogue résolu dans `core` | `core::sceneLevelCandidates` (`Core/Resources/ScenePlace.h`) donne les niveaux d'un lieu, du plus propre au monde ; `core::ScenePieceManifest::resolve` empile leurs manifestes. Chaque pièce garde son dossier (`ScenePiece::directory`) et son niveau ; son image est `directory/file`, le champ `file` du manifeste, plus le nom. Une pièce commune masquée est gardée (`masked`). |
| `LevelDraft`, `CollisionDerivation`, `PieceCatalog`, `--check` | `hmi::loadPlaceAssets` rend le catalogue résolu et la table empilée (`hmi::PlaceAppearance::loadForPlace`) ; le jeu (`WorldPlay`, arène, îlots du plan) lit la même table. Les tables `appearance.json` s'empilent par type de tuile : la plus propre gagne. |
| Palette par niveau | Un en-tête par niveau (« Arenarea », « Capital », « Central Empire », « World »), les groupes par classe ou par dossier dessous ; la recherche traverse tout ; une pièce propre dit « ⚠ masks Capital », la pièce masquée paraît éteinte. |
| « New map » | La liste des lieux est l'arbre des zones et sous-zones (`core::scenePlaces`), sous leur chemin lisible (« Central Empire › Capital › Arenarea ») ; la carte se range sous `Levels/<chemin du lieu sans son dernier segment>/` et prend par défaut le nom du lieu. |
| `--replace-piece`, `--who-cites piece`, `--change-scene` | Une citation dit de quel niveau la carte tient la pièce ; `--who-cites piece <pièce> [<dossier de niveau>]` et `--replace-piece … --level <dossier de niveau>` ne visent que les cartes qui la tiennent de ce niveau ; « Replace piece… » de la fenêtre prend le niveau de la pièce de la carte ouverte. `--change-scene` crée une table seulement si le nouveau lieu n'en lit aucune, à aucun niveau. |
| Figurines par niveau (en plus de la fiche, demandé par l'auteur) | Un PNJ se cherche comme une pièce : `core::resolveFigures` lit les listes `npcs` des `Characters/` du lieu, de ses communs et du monde (`Common/Characters`), le plus propre gagnant. Le jeu, le canevas et `--render` dessinent chaque figurine sous son niveau (`WorldSceneSnapshot::figureDirectories`) ; l'inspecteur et `--check` n'acceptent un PNJ nommé que sur les cartes de sa zone. |
| Préfabriqués et modèles par niveau | Un préfabriqué se range au plus bas niveau qui voit **toutes** ses pièces (`hmi::prefabLevel`) : fait du kit de la Capitale, il va sous `Editor/Prefabs/central-empire/capital/` et sert à tous ses quartiers. Les modèles se rangent sous `Editor/Templates/<lieu>/`. Un lieu se voit proposer les siens et ceux de ses niveaux communs. |

### Ce que la réalisation a tranché

**D-124-1 — Le lieu d'une carte est un chemin, sans révision de format.** La propriété `scene` d'une
couche vaut `central-empire/capital/arenarea` ; aucune carte ne nomme le niveau d'une pièce. Un lieu
**sans barre** (`bourg`) reste un lieu d'essai à plat, sous `Assets/Scene/<lieu>/`, qui remonte au
monde comme les autres : c'est la forme de la racine `GameData`, que ce lot **ne déplace pas**,
contrairement à ce qu'annonçait le [LOT-123](LOT-123-editeur-sur-une-base-vide.md). La déplacer
aurait récrit une trentaine de tests sans rien prouver de plus ; l'arborescence est prouvée par une
racine à elle, `Source/Test/Fixtures/LevelTree`. Le chemin `Scene/` n'est plus composé qu'à un
endroit, `ScenePlace.cpp`.

**D-124-2 — Le monde, ce sont `Common/Terrain`, `Common/Nature` et `Common/Props`.** Ni `Fx` (des
effets, pas des pièces de carte) ni `Characters`. À chaque préfixe du chemin, le `Scene/` propre
puis le `Common/Scene` sont candidats : l'arbre dit ce qu'est un dossier, pas le code.

**D-124-3 — Un manifeste illisible à un niveau commun fait échouer la résolution**, message préfixé
de son dossier. Passer le niveau en silence ferait tomber la collision de toutes les cartes qui en
descendent.

**D-124-4 — Un niveau se nomme par son dossier**, relatif à `Assets/`
(`Regions/central-empire/capital/arenarea/Scene`), sur la ligne de commande comme dans les
citations : c'est le seul nom qui ne soit pas ambigu (le monde a trois dossiers).

**D-124-5 — Le fichier d'une pièce est relatif à `Assets/`.** `PlaceAppearance::pieceFile` rend
`Regions/…/floors/floor-paving-01.png` ; le rendu du jeu et le cache d'images de l'éditeur étaient
déjà adressés depuis `Assets/`, rien d'autre n'a bougé.

**D-124-6 — Une figurine se nomme par son slug, relatif au `Characters/` qui la range**
(`anariel`, `citizen`, `Peoples/human/guard`) : une promotion de PNJ, comme celle d'une pièce, ne
réécrit aucune carte. Un slug introuvable garde les deux replis d'avant — un chemin depuis `Assets/`
(`Common/Characters/Heroes/brawler`, le héros), sinon l'atelier à plat `Npc/<slug>` des racines
d'essai. Le PNJ qui remplace un champion de l'arène (`Npc/manifest.json`, champ `replaces`) reste à
plat : c'est un mécanisme du kit d'arène, pas d'une carte.

### Critères

- **Une carte sous `capital/arenarea/` cite une pièce de chacun des quatre niveaux** ✔ —
  `LevelTree/Levels/central-empire/capital/arenarea.json` : `--check` passe sans case forcée
  (`LevelTreeTest.UneCarteQuiCiteQuatreNiveauxPasseLeControle`), `--render` la montre sans une image
  manquante (`MapRenderTest.UneCarteQuiPuiseDansQuatreNiveauxSeRend`), l'essai dans le jeu la compose
  (`ExplorationCarteIntegration.UneCarteQuiPuiseDansQuatreNiveauxSeJoue`). Vérifié aussi sur les
  assets **livrés** : une rue de 12 cases posée en Arenarea avec le kit de la Capitale et la bannière
  de l'Empire se contrôle et se rend (carte jetable, non versionnée).
- **Promouvoir une pièce** ✔ — sous la même clé, aucun octet des cartes ne change
  (`Arborescence.PromouvoirSousSaCleNeChangeAucuneCarte`) ; sous une autre clé, `--replace-piece
  --level` réécrit l'Arenarea et laisse le Martpart, qui pose la fontaine de la ville sous le même nom
  (`Arborescence.PromouvoirSousUneAutreCleNeRecritQueSesCartes`).
- **`git grep '"Scene/"' Source/Editor Source/HMI/Graphics`** ✔ — aucun résultat.
- **Un scénario `--apply` couvre la pose d'une pièce commune** ✔ — `Fixtures/Gestures/niveaux.json`
  refait l'Arenarea de `LevelTree` pièce à pièce, octet pour octet. La rue de douze cases avec le
  seul kit, vérifiée sur les assets livrés ; l'auteur a déclaré le lot fini.

## Livraison

Livré le 24 septembre 2026, sur décision de l'auteur, figurines comprises.
