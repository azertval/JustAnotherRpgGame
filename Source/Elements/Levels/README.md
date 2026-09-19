# Elements/Levels/

Cartes du jeu, un fichier **JSON** par carte (`EX-LVL-001`, `EX-LVL-003`).

> **Peuplé au `LOT-09`.** La première carte est `coliseum.json`, **le Colisée en version finale** : 40 × 34 cases, le sable du
> `LOT-50` (20 × 14) au centre comme **zone de combat déclarée** (`combatZone`, `EX-LVL-018`),
> l'enceinte et ses quatre portes, deux couloirs sous les gradins, deux vestiaires, les tribunes du
> nord et du sud, la loge impériale, le grand escalier, le hall et la porte. Elle a été posée par
> un script d'atelier (`Documentation/Lot/LOT-09-colisee-premiere-carte/atelier/carte_colisee.py`),
> retiré depuis ; `arena-of-the-future.json`, la piste nue du `LOT-50`, est partie avec elle. Le contenu du *vertical slice* arrive avec le `LOT-27`.

> **`capital/`, au `LOT-96`** : les quartiers de la Capitale qui ont leur carte — `martpart.json`
> (le quartier du marché, 48 × 40) et `arenarea.json` (le quartier des arènes, 48 × 40, sur la
> planche de Martpart faute de planche propre), reliés par l'avenue ; « Nouvelle partie » ouvre
> le jeu à la porte de l'Est de Martpart, que nomme `World/cities/capital.json`. Même méthode que le Colisée : un script d'atelier les a
> posées (`Documentation/Lot/LOT-96-quartiers-capitale/atelier/carte_quartiers.py`), retiré
> depuis. Une carte d'un sous-dossier a pour
> identifiant son **chemin relatif** : un portail vise `capital/martpart`, et le graphe du monde
> comme le navigateur de l'éditeur lisent les sous-dossiers.

> **Format v4, au `LOT-EDITOR-12`.** Les trois cartes ont été migrées par
> `LevelEditor --migrate` et sont gardées en CI par `LevelEditor --data Source/Elements --check`.
> Schéma : `Documentation/Editeur/level.schema.json` ; spécification :
> `Documentation/Specification/niveaux.md`.

> **L'éditeur fait foi, au `LOT-EDITOR-06`.** Ces cartes se modifient dans `LevelEditor` — à la
> souris, ou par `LevelEditor --apply` —, qui ouvre ce dossier-ci et non la copie de la
> construction. Aucun script n'y écrit plus : les deux scripts d'atelier restent dans leurs dossiers
> de lot, comme trace, et refusent d'écrire ici. Faire une carte : guide d'usage
> `Documentation/Editeur/guide-usage.md`.

- Le **`name`** d'une carte est une clé de traduction, `map.<identifiant>.name`
  (`map.capital.martpart.name`), que le bandeau du jeu traduit et que chaque catalogue de
  `Localization/` doit porter : `LevelEditor --check` le vérifie (`LOT-EDITOR-07`).
- Une carte est un objet JSON : `version`, `name`, `width`, `height`, et une liste **`tiles`**
  d'objets `{ "x", "y", "type" }` — la grille de **collision**, entrée comprise, **déduite** des
  pièces posées (`EX-LVL-020`). Les cases **vides** ne sont pas listées (absence = vide). Là où
  l'auteur veut s'écarter de la déduction, la case figure dans **`forced`**.
- Types de tuiles : `empty`, `solid`, `entry`, et le terrain du RPG (`LOT-08`) : `grass`, `dirt`,
  `sand`, `water`, `deepWater`, `wall`, `cliff`, `bridge`, `stairs`. Une carte porte **exactement
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
