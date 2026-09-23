# Elements/Levels/

Cartes du jeu, un fichier **JSON** par carte (`EX-LVL-001`, `EX-LVL-003`).

> **Vidé au `LOT-102`.** Le jeu quitte le pixel art : `coliseum.json` (`LOT-09`) et les quartiers
> de `capital/` (`LOT-96`) sont partis avec l'art qu'ils posaient. Les cartes 2D HD se dessinent à
> l'éditeur, sous le **même découpage que les assets** — `<région>/<ville>/<zone>.json`, la
> sous-zone sous son dossier de zone :
> `central-empire/capital/arenarea.json` (`LOT-109`), `martpart.json` (`LOT-111`),
> `arenarea/arena-of-fate.json` (`LOT-107`). Une carte a pour identifiant son **chemin relatif** :
> un portail vise `central-empire/capital/martpart`, et le graphe du monde comme le navigateur de
> l'éditeur lisent les sous-dossiers.

> **Format v4, au `LOT-EDITOR-12`.** Toute carte écrite ici est gardée en CI par
> `LevelEditor --data Source/Elements --check`, et une carte d'un format antérieur se migre par
> `LevelEditor --migrate`.
> Schéma : `Documentation/Specification/level.schema.json` ; spécification :
> `Documentation/Specification/niveaux.md`.

> **L'éditeur fait foi, au `LOT-EDITOR-06`.** Ces cartes se modifient dans `LevelEditor` — à la
> souris, ou par `LevelEditor --apply` —, qui ouvre ce dossier-ci et non la copie de la
> construction. Aucun script n'y écrit. Faire une carte : guide d'usage
> `Documentation/Guide/Manuel/utiliser-l-editeur.md`.

> **Ce `README.md` est le gardien du dossier (`LOT-123`).** Git ne garde pas un dossier vide : sans
> lui, un `Levels/` sans aucune carte disparaîtrait du dépôt, et `hmi::resolveDataRoot` cesserait de
> reconnaître l arbre des sources — la fenêtre se rabattrait en silence sur la copie que la
> construction refait à côté de l exécutable. Un dossier **sans aucune carte** est un état légitime :
> `LevelEditor --check` le dit (« 0 map ») et rend 0.

- Le **`name`** d'une carte est une clé de traduction, `map.<identifiant>.name`
  (`map.central-empire.capital.martpart.name`), que le bandeau du jeu traduit et que chaque catalogue de
  `Localization/` doit porter : `LevelEditor --check` le vérifie (`LOT-EDITOR-07`).
- Une carte est un objet JSON : `version`, `name`, `width`, `height`, et une liste **`tiles`**
  d'objets `{ "x", "y", "type" }` — la grille de **collision**, entrée comprise, **déduite** des
  pièces posées (`EX-LVL-020`). Les cases **vides** ne sont pas listées (absence = vide). Là où
  l'auteur veut s'écarter de la déduction, la case figure dans **`forced`**.
- Types de tuiles : `empty`, `solid`, `entry`, et le terrain du RPG (`LOT-08`) : `grass`, `dirt`,
  `sand`, `water`, `deepWater`, `wall`, `cliff`, `bridge`, `stairs` ; et le vocabulaire de la
  maquette : `pavement`, `alley`, `planks`, `flagstone`, `snow`, `mud`, `rubble`, `door`, `bush`,
  `tree`, `rock`, `fence`, `lowWall`, `stall`, `crate`, `column`, `roof`, `tiers`, `pit`, `lava`
  (leur règle de pas : `Documentation/Specification/niveaux.md`). Une carte porte **exactement
  une** case `entry`, le point d'arrivée par défaut du héros.
- `"layers"` : couches visibles `{ "name", "kind", …, "tiles" }` (`kind` : `ground`, `decor`), dont
  chaque case `{ "x", "y", "type", "piece" }` nomme la **pièce** de la planche du lieu qu'on y voit
  (`EX-LVL-019`) ; la propriété de couche `scene` nomme le lieu.
- `"entities"` : entités `{ "id", "type", "x", "y", … }` — PNJ, portails, zones —, un `id` unique
  jamais réemployé (`"nextEntityId"`, `EX-LVL-021`), leurs autres champs étant des propriétés libres
  (`EX-LVL-017`, `EX-LVL-018`).
- Le format est **versionné** (`"version"`, cf. `core::LEVEL_FORMAT_VERSION`) : toute version
  passée se lit, le chargeur refuse proprement une version qu'il ne connaît pas, et l'écriture est
  canonique — une case par ligne, un geste = une ligne de diff.
