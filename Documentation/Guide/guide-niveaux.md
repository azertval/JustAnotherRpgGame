# Niveaux : modèle, couches, entités, chargement

Une carte (un « niveau », dans le vocabulaire du code) est une **grille de tuiles typées** plus des
métadonnées : un nom, une entrée, des couches visibles, des entités et des pièces assignées par
case. Le modèle en mémoire, son chargement et son écriture vivent dans `Source/Core/Levels` ; ce
qui **fait vivre** une carte — l'exploration, le passage d'une carte à l'autre — vit dans
`Source/Core/World`.

## Le modèle en mémoire

### Deux systèmes de coordonnées à ne pas confondre

Le moteur manipule **deux** représentations de position différentes, et les confondre est une
source d'erreurs fréquente :

- `core::GridPosition` : une paire d'**entiers** `(column, row)` — désigne une **case** de la
  grille, sans unité de mesure continue. C'est ce que manipule tout ce qui parle de la carte comme
  d'un plateau discret (entrée, position d'une entité, pièce assignée).
- une position **continue**, en flottants : `core::CellPoint` pour le héros de l'exploration
  (`{1.5, 2.5}` est le centre de la case `(1, 2)`), `core::Vector2` ([Mathématiques du moteur](guide-maths.md)) ailleurs.

Comme une tuile = 1 unité, convertir l'un vers l'autre est une simple conversion de type
(`core::cellOf`, `core::cellCenter`) — mais les deux ne sont **jamais** interchangeables dans le
code : une case n'a pas de position « à mi-chemin », un personnage si.

### `core::TileType` : le vocabulaire des cases

Chaque case de la grille a l'un de ces douze types :

| Type | Rôle |
|------|------|
| `Empty` | Case traversable, par défaut (aucun contenu). |
| `Solid` | Matière générique, bloquante. |
| `Entry` | Point d'arrivée par défaut du héros sur la carte. |
| `Grass`, `Dirt`, `Sand` | Sols traversables : herbe, terre battue, sable. |
| `Water` | Eau **peu profonde** : traversable — on y patauge. |
| `DeepWater` | Eau **profonde** : bloquante tant qu'aucune règle de nage n'existe. |
| `Wall` | Mur : la matière pleine bâtie, distincte de `Solid` pour pouvoir être vêtue autrement. |
| `Cliff` | Falaise : l'obstacle **naturel** du décor extérieur. |
| `Bridge` | Pont : franchit l'eau ou un ravin. Traversable. |
| `Stairs` | Escalier : traversable. Ce qu'il **relie** est une donnée du graphe de cartes, jamais du type. |

`core::isSolid(TileType)` est la **seule** définition de ce qui bloque : `Solid`, `Wall`, `Cliff`
et `DeepWater`. Le jour où une règle de nage existera, c'est là, et seulement là, que `DeepWater`
changera de camp.

Ce qui **agit** sur une carte — PNJ, coffres, portails, rencontres — n'est pas un type de tuile
mais une **entité** (voir plus bas) : une grille ne retient qu'un type par case, sans métadonnée.

### `core::TileMap` : la grille

`core::TileMap` est une grille dense `width × height` de `TileType`, origine **haut-gauche** (même
convention que tout le moteur, [Mathématiques du moteur](guide-maths.md)). Elle expose `tile(colonne, ligne)` (lecture),
`setTile`, `inBounds` et `isSolid(colonne, ligne)`. C'est une donnée pure, sans dépendance à un
fichier ou à un rendu — testable isolément.

### `core::Level` : la carte assemblée

`core::Level` se construit à partir d'un agrégat nommé, `core::LevelData` (C++20 *designated
initializers*), et regroupe :

- un **nom** et une **`TileMap`** — la grille de **collision**, celle qui porte aussi l'entrée ;
- des **couches** (`core::TileLayer`, `LOT-04`) dans leur ordre de superposition ;
- des **entités** (`core::MapEntity`) ;
- l'**entrée** (`GridPosition`, relecture de la case `Entry`) ;
- des **pièces assignées par case** (`core::TileTextureOverride`, `EX-EDIT-043`) : le nom d'une
  pièce de la planche du lieu, dessinée sur cette case à la place de celle que la table
  d'apparence de son type choisirait.

C'est l'objet que le chargeur produit et que le reste du moteur (exploration, combat, rendu)
consomme en lecture seule : un `Level` n'a **aucun** mutateur. L'édition passe par un type
distinct, `core::LevelDraft` ([Éditeur de niveaux](guide-editeur.md)).

### Couches : ce qu'on voit n'est pas ce qui bloque

Un RPG en vue de dessus a besoin de séparer l'**image** de la **règle** : un tapis se traverse, un
tonneau non, et les deux peuvent reposer sur la même dalle. `core::LayerKind` distingue donc :

- `Ground` — ce qu'on voit sous les pieds ;
- `Decor` — dessiné au-dessus du sol ;
- `Collision` — le masque, **indépendant du visuel** ;
- `Legacy` — la grille unique d'une carte qui ne déclare aucune couche : elle vaut alors à la fois
  image et collision.

Il n'existe **jamais** deux grilles de collision à tenir d'accord (`EX-LVL-016`) : la collision
d'une carte est sa `TileMap` racine, promue par le chargeur en couche de tête (`Collision` si la
carte déclare des couches visibles, `Legacy` sinon). Un consommateur boucle donc sur `layers()`
sans cas particulier. `core::isVisualLayerTileType` dit ce qui se peint sur une couche visuelle :
tout le terrain, mais pas l'`Entry`, qui porte une **règle** et n'a de sens que dans la grille de
collision.

### Entités et propriétés libres

`core::MapEntity` est ce qui n'est **pas** une tuile : un `type` libre (`"npc"`, `"chest"`,
`"portal"`, `"encounter"`…), une case, et des **propriétés libres** (`core::PropertyMap`,
`EX-LVL-017`/`EX-LVL-018`). `Core/Levels` ne connaît **aucune** sémantique de `type` : c'est le
gameplay qui l'interprète, et les familles que l'éditeur sait poser sont rassemblées dans
`core::knownEntityKinds` (`Source/Core/World/EntityKinds.h`). Une entité de type inconnu est une
erreur de conception tolérée, pas une carte invalide (`EX-NFR-040`).

`core::PropertyMap` est un `std::map` (ordonné, pour une écriture **déterministe**) de valeurs
`bool`, entier, réel ou chaîne. C'est aussi là que le chargeur range **toute clé qu'il ne reconnaît
pas** dans une couche ou une entité, et que l'écrivain la réémet : un fichier produit par une
version ultérieure de l'éditeur traverse une version antérieure sans rien perdre. La propriété de
couche `scene`, par exemple, nomme le **lieu** dont la carte porte les planches
(`Assets/Scene/<lieu>/`).

## Chargement JSON

Une carte est décrite dans un fichier texte au format [JSON](https://www.json.org/json-fr.html) ⧉,
parsé par la bibliothèque **nlohmann/json**, dont l'usage est confiné aux fichiers `.cpp` du
chargeur et de l'écrivain — le reste du moteur ne dépend jamais directement de cette bibliothèque.
La classe `core::LevelLoader` expose deux points d'entrée statiques : `loadFromFile` (depuis un
chemin) et `loadFromString` (depuis du texte déjà en mémoire, pratique pour les tests).
`core::LevelWriter` en est le symétrique (`toJsonString`, `saveToFile`, `buildJson`).

### Exemple concret

```json
{
  "version": 3,
  "name": "Village",
  "width": 12,
  "height": 8,
  "tiles": [
    { "x": 1, "y": 1, "type": "entry" },
    { "x": 4, "y": 4, "type": "wall", "texture": "puits" }
  ],
  "layers": [
    { "name": "sol", "kind": "ground", "scene": "village",
      "tiles": [{ "x": 4, "y": 4, "type": "dirt" }] }
  ],
  "entities": [
    { "type": "npc", "x": 6, "y": 3, "dialogue": "bonjour" }
  ]
}
```

(exemple illustratif, repris de [`niveaux.md`](../Specification/niveaux.md) — les cartes réelles sont dans
`Source/Elements/Levels/`, par exemple `coliseum.json`). À lire ainsi :

- `version` est le numéro de format (`core::LEVEL_FORMAT_VERSION`, aujourd'hui `3`, `EX-LVL-005`) ;
  un fichier sans ce champ se lit comme la version initiale, un fichier d'une version **supérieure**
  est refusé plutôt que lu au mieux ;
- `name`, `width`, `height` décrivent la carte et les dimensions de sa grille ;
- `tiles` est une **liste éparse** : seules les cases **non vides** sont listées, chacune par ses
  coordonnées `x`/`y` (colonne/ligne) et son `type`. Une case absente de la liste est
  implicitement `Empty`. Une tuile peut porter `"texture"`, sa pièce assignée ;
- `layers` et `entities` sont **optionnels** : une carte sans couche garde sa grille unique, promue
  `Legacy`, et ressort de l'écrivain **telle qu'elle est entrée**.

### Validation

Le chargement **valide** le contenu (`EX-LVL-004`) avant de produire un `Level` utilisable :
tuiles toutes dans les bornes de la grille, aucune case dupliquée, type de tuile connu, couches aux
dimensions de la carte, pas de couche `collision` déclarée, et **exactement une** tuile `Entry`
(une carte sans entrée, ou avec deux, est une erreur de contenu, pas une situation ambiguë à
tolérer).

En cas d'échec — JSON malformé, champ manquant, type de tuile inconnu, échec d'une des validations
ci-dessus — le chargeur ne lève **jamais d'exception** vers l'appelant (`EX-NFR-040`) : il renvoie
un `core::LevelLoadResult`, `{ optional<Level> level, std::string error, LevelValidationError
errorCode }`. `ok()` indique le succès ; en cas d'échec, `level` est vide, `error` décrit le
problème de façon exploitable et `errorCode` le **catégorise** (`ParseError`, `InvalidEntryCount`,
`UnsupportedFormatVersion`…), pour qu'un appelant ne dépende jamais du texte exact du message. Ce
choix — résultat récupérable plutôt qu'exception — garde la gestion d'erreur explicite à chaque
site d'appel, cohérent avec le reste du moteur qui ne s'appuie pas sur les exceptions pour son flux
de contrôle normal.

## Qui lit la carte

Le `Level` n'est, en lui-même, qu'une donnée : il ne bouge pas et ne s'affiche pas.
`core::ExplorationSession` (`Source/Core/World`) le fait vivre — un héros qui marche, parle à ce
qu'il regarde et franchit les portails —, et `core::WorldTravel` sert les cartes du dossier des
niveaux (`<dossier>/<identifiant>.json`) quand un portail en désigne une autre.

Point important : la `TileMap` racine reste la **source de vérité** de tout ce qui touche à la
**collision**. L'exploration interroge directement `isSolid(colonne, ligne)` sur `tileMap()`,
jamais une couche visuelle ni ce que le rendu en a tiré. Repeindre le sol ne change donc rien à ce
qui bloque ; seul le masque de collision le fait.

## Voir aussi
- `core::Level`, `core::LevelData`, `core::TileMap`, `core::TileType`, `core::TileLayer`,
  `core::MapEntity`, `core::PropertyMap`, `core::TileTextureOverride`.
- `core::LevelLoader`, `core::LevelLoadResult`, `core::LevelWriter`.
- `core::ExplorationSession`, `core::WorldTravel`, `core::knownEntityKinds`.
- [Éditeur de niveaux](guide-editeur.md) — le brouillon mutable, qui repasse par ce chargeur pour valider.
- [Écrans, navigation et boucle de jeu](guide-ecrans.md) — les écrans du jeu qui mettent une carte à l'écran.
- [`niveaux.md`](../Specification/niveaux.md) — le format de carte et ses exigences.
