+++
id = "LOT-1008"
titre = "L'atelier des assets 3D"
version = "0.0.2.5"
filiere = "editeur"
statut = "a-faire"
taille = "L"
resume = "Dans l'éditeur, une fenêtre où l'on lie les éléments entre eux : écrire la fiche d'un personnage, composer un ensemble de décor qui paraît dans l'onglet « Prefabs »."
prerequis = ["LOT-1006"]
livrables = [
  "La fenêtre **Asset workshop** de `LevelEditor`, à deux vues, avec un aperçu 3D commun rendu par le rendu du jeu : lecture des six animations, quart de tour, heure du jour.",
  "Vue **Character** : choisir le modèle, le portrait, le jeton et la silhouette ; elle écrit la fiche du personnage sous `Characters/`.",
  "Vue **Scenery** : déclarer un maillage comme pièce du lieu (emprise en cases, ancre, hauteur d'étage, collision, lumière émise) ; assembler plusieurs pièces en un ensemble, étage par étage, écrit comme **préfabriqué** dans la bibliothèque du lieu (`hmi::writePrefab`).",
  "La logique en fonctions pures sous `Source/Editor/Logic`, et les mêmes gestes **sans fenêtre** : un scénario `--apply` par vue, comparé à un fichier attendu.",
  "Le **contrôle** (`--check`) : squelette inconnu, modèle, portrait ou jeton absent, clip manquant, maillage sans emprise.",
  "Le guide d'usage de l'éditeur, chapitre « Asset workshop ».",
]
criteres = [
  "L'auteur écrit à la main la fiche d'un personnage complet — modèle, portrait, jeton — sans ouvrir un fichier, et le retrouve dans le jeu et dans la galerie de débug.",
  "Un ensemble composé dans la vue Scenery paraît dans l'onglet « Prefabs » de la palette avec sa vignette, et se pose au tampon sur une carte ; `Ctrl+Z` le défait d'un geste.",
  "Chaque scénario `--apply` redonne son fichier attendu ; une fiche écrite par la fenêtre et la même écrite sans fenêtre sont identiques à l'octet.",
  "Aucune fiche de personnage ni de pièce du dépôt n'est plus écrite à la main : toutes se rouvrent et se réenregistrent par l'atelier sans différence.",
]
+++

## Pourquoi

Un personnage est une fiche qui lie un modèle, son portrait et son jeton ; un bâtiment,
un ensemble de maillages posés sur la grille. Écrire ces liens à la main — un os, un décalage, une rotation —
ne se fait pas à l'aveugle. L'auteur a demandé que cela se fasse dans l'éditeur
([D-33](../../../../vision/decisions.md)), et que la vue de décor alimente l'onglet « Prefabs ».

> **Amendé au `LOT-1001`** (1er octobre 2026) : les huit corps et la bibliothèque de pièces
> portées n'existent plus ([D-38](../../../../vision/decisions.md)), et les modèles sont sans
> arme (D-42) : la vue **Piece** est retirée, la vue Character n'assemble plus un corps et une
> texture. Le lot passe de trois vues à deux, de XL à L.

## Périmètre

Dedans : les deux vues, leur logique, leurs scénarios sans fenêtre, le contrôle.

Dehors, nommément :

- **modeler** : l'atelier lie, il ne sculpte pas. Les maillages viennent de Blender ;
- le **kit de la Capitale** : ses pièces sortent du script du kit (LOT-151, depuis la clôture du
  LOT-1004 — [D-43](../../../../vision/decisions.md)), pas de la vue Scenery,
  qui sert aux maillages importés un par un et à la composition des ensembles ;
- le **placement hors grille** : le format v4 ancre une pièce sur une case. Poser un maillage entre
  deux cases ou le tourner d'un angle libre est une évolution du format, à décider à part.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| La saisie à la main des fiches de personnage et de pièce | les fiches écrites aux LOT-1000, LOT-1005 et LOT-1006 : **réenregistrées** par l'atelier, plus de champ que l'outil ne connaît pas | dernier critère : une seule façon d'écrire une fiche |
| La part « personnage » de l'installateur | `scripts/assetsGeneration/install_hd_asset.py` : ce qui range un personnage sous `Characters/` et écrit son manifeste | l'atelier l'écrit ; l'installateur ne garde que les images et les maillages de décor |
| Le descripteur d'installation des portraits | atelier local : `Tools/AssetsHD/Common/Characters/Heroes/install-portraits.json` | le portrait se choisit dans la vue Character |

Aucun code du jeu n'est supprimé par ce lot : il ajoute un outil.

## Conception

- **La vue Scenery n'invente pas de format.** Un préfabriqué est déjà un rectangle de carte complet
  (`hmi::Stamp`, `LOT-EDITOR-08`) : couches, pièces ancrées, entités, collision. La vue compose ce
  rectangle en 3D et l'écrit par le mécanisme existant ; la vignette est rendue, pas enregistrée.
- **L'aperçu** est le rendu du jeu (LOT-1002) : ce qu'on y voit est ce qu'on jouera.
- Textes en anglais, hors charte v2, comme le reste de l'éditeur.

## Risques et questions ouvertes

- **La taille.** S'il faut couper le lot : Character d'abord (les personnages du LOT-1009 en
  dépendent), Scenery dans un second lot qui peut glisser à la `0.0.3`.
- **La fiche de liaison** d'un personnage — ses articulations dans son maillage — se règle
  aujourd'hui en nombres, à la main, pour un script Blender (`personnages-3d.md`, §6). La placer
  à la souris dans la vue Character serait le vrai gain de l'atelier, mais demande que l'éditeur
  lance Blender : à trancher par l'auteur à l'ouverture du lot.
