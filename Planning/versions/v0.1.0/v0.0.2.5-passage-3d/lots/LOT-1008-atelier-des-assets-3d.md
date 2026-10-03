+++
id = "LOT-1008"
titre = "L'atelier des assets 3D"
version = "0.0.2.5"
filiere = "editeur"
statut = "livre"
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

> **Décision de l'auteur à l'ouverture du lot, 2 octobre 2026** ([D-44](../../../../vision/decisions.md)) :
> « on fait un import Blender dans l'édition des assets pour que l'utilisateur puisse travailler
> avec précision les squelettes et mouvements des pièces ». La question ouverte ci-dessous est
> tranchée : l'éditeur lance Blender, et ce qui en revient est une donnée que la chaîne rejoue.
> Le lot se livre sur sa seule vue **Character** ; la vue **Scenery** glisse à la `0.0.3` avec le
> kit en maillages (D-43) : il n'existe aucun maillage de décor à déclarer ni à composer.

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
| La saisie à la main des fiches de personnage et de pièce | les fiches écrites aux LOT-1000, LOT-1005 et LOT-1006 : **réenregistrées** par l'atelier, plus de champ que l'outil ne connaît pas — fait, sans différence | dernier critère : une seule façon d'écrire une fiche |
| La part « personnage » de l'installateur | `scripts/assetsGeneration/install_hd_asset.py` : ce qui range un personnage sous `Characters/` et écrit son manifeste — **retirée**, avec ses tests | l'atelier l'écrit ; l'installateur ne garde que les images et les maillages de décor |
| Le descripteur d'installation des portraits | atelier local : `Tools/AssetsHD/Common/Characters/Heroes/install-portraits.json`, et les `Tools/Assets3D/Lies/install*.json` — à archiver par l'auteur, remplacés par les fiches d'atelier | le portrait se choisit dans la vue Character |

Aucun code du jeu n'est supprimé par ce lot : il ajoute un outil.

## Conception

- **La vue Scenery n'invente pas de format.** Un préfabriqué est déjà un rectangle de carte complet
  (`hmi::Stamp`, `LOT-EDITOR-08`) : couches, pièces ancrées, entités, collision. La vue compose ce
  rectangle en 3D et l'écrit par le mécanisme existant ; la vignette est rendue, pas enregistrée.
- **L'aperçu** est le rendu du jeu (LOT-1002) : ce qu'on y voit est ce qu'on jouera.
- Textes en anglais, hors charte v2, comme le reste de l'éditeur.

## Décisions de réalisation

Livré le 3 octobre 2026, **PR #173**. Ce qui est livré :

- la fenêtre **Asset workshop** (*Assets* › *Asset workshop…*, `hmi::AssetWorkshop`) : les
  personnages installés, la **fiche d'atelier** d'un personnage (`hmi::CharacterDraft`, un JSON
  `jadg-editor-character` rangé avec ses sources, dont les chemins partent d'une racine et
  s'inscrivent au manifeste tels qu'ils sont écrits), l'**aperçu** par le rendu du jeu — un
  damier de maquette, le modèle au centre, clip par clip, quart de tour —, l'**installation**
  (`hmi::planCharacter`, tout vérifié avant la première écriture ; un fichier déjà à l'identique
  n'est pas réécrit) et le **contrôle** des personnages installés ;
- la même chose **sans fenêtre** : `LevelEditor --apply <fiche d'atelier>` (plusieurs fiches à la
  suite), les fichiers attendus de `Source/Test/Fixtures/Workshop/` ; `--check` contrôle les
  personnages installés (`EX-EDIT-104`) ;
- l'**aller-retour par Blender** (`EX-EDIT-103`, D-44) : `scripts/assetsGeneration/retouch_character.py`
  (`open`, `import`), la fiche de retouche lue par `rig_character.py --retouch`, les boutons
  *Edit in Blender* et *Import from Blender* ; la chaîne a été éprouvée sur le bandit (une
  articulation déplacée, un clip modifié, le modèle relié passe le contrôle) ;
- le [standard des personnages](../../../../standards/personnages-3d.md) complété (§2, §6 bis,
  §7) ; `AGENTS.md` amendé sur la retouche des animations ;
- les trois fiches livrées (mannequin, brawler, bandit) **rouvertes et réenregistrées par
  l'atelier** sans différence (`CharacterWorkshopDelivered`, en CI sur toutes les fiches du dépôt) ;
  seule l'empreinte de la source du squelette, au manifeste du commun, change une fois : elle
  signe désormais le texte aux fins de ligne du dépôt, la même sur tout poste.

Ce que le lot a tranché en le faisant :

- **L'atelier ne retaille rien.** Le portrait (512 × 512) et le jeton (128 × 128) se donnent aux
  tailles du standard ; l'atelier les range et les vérifie. Le retaillage et le détourage que
  faisait l'installateur Python partent avec sa part « personnage » (D-32) ; les portraits de
  l'atelier local (`export_portraits.py`) sont déjà à ces tailles.
- **Pas d'heure du jour dans l'aperçu** : elle vient avec l'éclairage (LOT-1007).
- **Un clip joué une fois se tient juste avant sa fin** dans l'aperçu : un modèle de l'atelier n'a
  pas de fiche à côté de lui, le rendu le fait boucler, et l'instant exact de la fin serait celui
  du début.
- **Un descripteur `install.json` dont la cible est un `Characters/` est refusé**, avec le geste
  qui le remplace. Les descripteurs de l'atelier local (`Tools/Assets3D/Lies/install*.json`) sont
  remplacés par une fiche d'atelier par personnage (`Tools/Assets3D/Fiches/`), que le LOT-1009
  écrit pour les dix-sept autres.

## Risques et questions ouvertes

- **La taille.** S'il faut couper le lot : Character d'abord (les personnages du LOT-1009 en
  dépendent), Scenery dans un second lot qui peut glisser à la `0.0.3`.
- **La fiche de liaison** d'un personnage — ses articulations dans son maillage — se règle
  aujourd'hui en nombres, à la main, pour un script Blender (`personnages-3d.md`, §6). La placer
  à la souris dans la vue Character serait le vrai gain de l'atelier, mais demande que l'éditeur
  lance Blender : **tranché par l'auteur à l'ouverture du lot** (D-44, ci-dessus) — l'éditeur
  lance Blender, et relit la fiche.
