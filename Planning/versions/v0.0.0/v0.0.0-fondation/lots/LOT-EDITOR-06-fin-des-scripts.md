+++
id = "LOT-EDITOR-06"
titre = "Les cartes quittent leurs scripts"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "S"
resume = "L'éditeur devient la source des cartes faites à la main : le Colisée, Martpart et Arenarea se modifient, s'enregistrent et se rechargent dans l'éditeur, sans perte et sans script."
prerequis = ["LOT-EDITOR-05", "LOT-EDITOR-13"]
livrables = [
  "`coliseum.json`, `capital/martpart.json` et `capital/arenarea.json` retouchées par `--apply` ; fichiers de gestes versionnés dans `retouches/`.",
  "`carte_colisee.py` et `carte_quartiers.py` gardés comme trace : note de retrait, `--check` retiré, `--sortie` exigé, dossier des cartes refusé.",
  "`core::LevelDraft::setLayerProperty`.",
  "`Editor/Logic` : `DataRoot` (`resolveDataRoot`, `editorDataRoot`), `scenePlaces`, `LevelFileOperations::create` avec lieu.",
  "La fenêtre ouvre `editorDataRoot()` et l'affiche ; dialogue « New map » (nom, taille, lieu) ; `Main.cpp` résout la racine une fois.",
  "Tests : `test_shipped_maps.cpp` (3), `test_level_file_operations.cpp` (+2), `test_level_draft_editing.cpp` (+1).",
  "Le guide d'usage de l'éditeur, `editeur-niveaux.md` §15, README du module et de `Levels/` ; `EX-EDIT-077`, `EX-EDIT-078`.",
]
criteres = [
  "Les trois cartes se modifient, s'enregistrent et se rechargent dans l'éditeur sans perte et sans script.",
  "Dernière génération des scripts, puis retrait de leur `--check`.",
  "Une vraie retouche faite dans l'éditeur sur chacune des trois cartes.",
  "Guide d'usage : créer une carte de bout en bout.",
]
+++

## Pourquoi

L'éditeur devient la source des cartes faites à la main (décision D4) : le Colisée, Martpart et
Arenarea se modifient, s'enregistrent et se rechargent dans l'éditeur, sans perte et sans script.

**Premier jalon du module** : les cartes du jeu se font désormais dans l'éditeur. Feuille de
route : [éditeur](@ref roadmap-editeur). Le lot alimente `LOT-EDITOR-07` (contrôles),
`LOT-EDITOR-14` (renommer, remplacer), et les cartes du [LOT-27](@ref lot-27).
Guide : @subpage guide-usage-editeur.

## Ce que le dépôt contenait à l'ouverture (19 septembre 2026)

- **Deux scripts d'atelier écrivaient les cartes** : `carte_colisee.py` (`LOT-09`) et
  `carte_quartiers.py` (`LOT-96`), chacun avec un `--check` qui comparait la carte commitée à son
  tracé. Les deux étaient verts : les cartes commitées étaient exactement leur dernière génération.
  Aucun des deux `--check` ne tournait en CI (constat du `LOT-EDITOR-12`) ; `LevelEditor --check`
  y tournait déjà.
- **`LevelEditor --check` relevait trois avertissements**, tous au Colisée : deux emprises de
  pièces larges qui se recouvraient, en trois cases.
- **La fenêtre de l'éditeur n'éditait pas le dépôt.** Elle ouvrait les cartes à côté de
  l'exécutable, dans `build/ninja/bin/Levels`, la copie que la cible `CopyGameData` refait à
  chaque construction : ce qu'on y enregistrait n'atteignait jamais `Source/Elements`, et la
  construction suivante l'écrasait. Les commandes sans fenêtre avaient la même racine par défaut
  (la CI passe `--data Source/Elements`).
- **Une nouvelle carte ne pouvait pas prendre de lieu.** « New » créait une grille unique vide,
  sans couche ni planche ; rien dans l'éditeur ne pose la propriété `scene` d'une couche. La
  palette n'y montrait donc que les types en couleurs : aucune carte du jeu ne pouvait naître dans
  l'éditeur.

Les deux derniers constats bloquaient le jalon : ils sont traités dans ce lot, sans lot à part.

## Périmètre

### Livraison

| Partie | Contenu |
|---|---|
| Cartes | `coliseum.json`, `capital/martpart.json`, `capital/arenarea.json` retouchées par `--apply` ; fichiers de gestes dans `retouches/` |
| Scripts | `carte_colisee.py`, `carte_quartiers.py` : note de retrait, `--check` retiré, `--sortie` exigé, dossier des cartes refusé |
| `Core` | `LevelDraft::setLayerProperty` |
| `Editor/Logic` | `DataRoot` (`resolveDataRoot`, `editorDataRoot`) ; `scenePlaces` ; `LevelFileOperations::create` avec lieu |
| `Editor/Ui`, `App/Editor` | la fenêtre ouvre `editorDataRoot()` et l'affiche ; dialogue « New map » (nom, taille, lieu) ; `Main.cpp` résout la racine une fois |
| Tests | `test_shipped_maps.cpp` (3) ; `test_level_file_operations.cpp` (+2) ; `test_level_draft_editing.cpp` (+1) |
| Documentation | guide d'usage @ref guide-usage-editeur ; `editeur-niveaux.md` §15 ; README du module et de `Levels/` |

### Ce qui reste hors du lot, nommément

- **Changer le lieu d'une carte existante**, et créer directement une carte dans un sous-dossier :
  `LOT-EDITOR-14`.
- **Le fichier annexe de Martpart et d'Arenarea** : aucune note d'auteur n'y a été écrite.
- **`CityDistrictModel.h`** cite encore `carte_quartiers.py` pour dire que les quartiers sont
  tracés nord en haut : c'est vrai des cartes livrées, et la règle vaut pour les suivantes.

## Conception

- **Dernière génération : celle qui est commitée.** Relancés à l'ouverture, les deux scripts
  rendaient les cartes commitées octet pour octet ; il n'y avait rien à régénérer. La chaîne a été
  vérifiée de bout en bout : tracé du script, puis fichier de retouche rejoué par `--apply`, égale
  la carte livrée, pour chacune des trois.
- **Les scripts restent comme trace et n'écrivent plus dans le dépôt.** Leur `--check` est retiré ;
  ils exigent `--sortie DOSSIER` et **refusent** le dossier des cartes du jeu, avec un message qui
  renvoie à l'éditeur. Les supprimer aurait perdu la façon dont les cartes ont été posées ; les
  laisser écrire aurait permis d'écraser une retouche d'un seul lancement.
- **Les retouches se font par `--apply`**, avec les fonctions mêmes que la souris
  (`LOT-EDITOR-13`), et leurs fichiers de gestes sont versionnés dans le dossier du lot
  (`retouches/`) : elles se relisent, se rejouent, et la CI en publie le rendu. La main de l'auteur
  ne se simule pas ; la vérification à la souris reste due.
- **Une retouche utile par carte, pas un habillage.** L'habillage des quartiers appartient au
  [LOT-27](@ref lot-27) ; chaque retouche corrige un défaut que l'éditeur montre, ou donne au lieu
  ce que son nom promet, sans toucher aux entités, aux portails ni aux zones :
  - **Colisée** — les trois chevauchements de `--check`. La porte sud était deux pièces larges
    croisées, `gate-left` (emprise 1 × 2, dont une case hors de la carte) et `gate-right` (2 × 1,
    sur l'angle voisin) : une seule `gate-right` couvre désormais les deux seuils. La loge empilait
    trois `box-left` d'emprise 1 × 2 sur trois cases : deux loges par côté, aux lignes 5 et 7, en
    symétrie. `--check` : zéro avertissement.
  - **Martpart** — le marché prend ses boutiques : deux devantures (`front-right`) sur la façade
    nord de la place et une (`front-left`) sur la façade ouest, chacune à la place de deux pans de
    mur nus, et le grand étal (`feature-1`) au sud de la place. Aucune de ces pièces de la planche
    n'était posée.
  - **Arenarea** — un angle de mur isolé, posé sans sol au débouché de la ruelle est du parvis
    (32, 13), laissait un trou dans le pavé : il part, la case reçoit la ruelle.
- **L'éditeur ouvre l'arbre des sources qui l'a construit.** `hmi::resolveDataRoot` choisit, dans
  l'ordre : `--data` ; le `Source/Elements` que la construction lui a donné
  (`JADG_EDITOR_SOURCE_DATA`), s'il existe sur le poste ; le dossier de l'exécutable. La fenêtre
  et les commandes sans fenêtre partagent cette racine, et le titre de la fenêtre la montre. La
  copie de `CopyGameData` reste ce que lit le jeu.
- **Une nouvelle carte choisit son lieu** (`EX-EDIT-077`) : « New » demande un nom, une taille et
  un lieu parmi les planches qui ont un manifeste (`hmi::scenePlaces`). Avec un lieu, la carte naît
  comme les cartes livrées — couches `sol` (propriété `scene`) et `relief`, collision déduite, le
  vide en mur, l'entrée sur une case de terre — et passe `--check` telle quelle. Sans lieu, elle
  reste une grille unique. `core::LevelDraft::setLayerProperty` pose la propriété, en un pas
  d'annulation. Changer le lieu d'une carte existante reste au `LOT-EDITOR-14` (il faut une table
  de correspondance des pièces).

## Vérification

- **Les trois cartes se modifient, s'enregistrent et se rechargent dans l'éditeur sans perte et
  sans script.** ✔
  `ShippedMapsTest.ChaqueCarteSOuvreEtSEnregistreALIdentique` : chaque carte ouverte en brouillon
  puis enregistrée rend son fichier octet pour octet.
  `ShippedMapsTest.UneRetoucheSEnregistreSeRechargeEtSeDefait` : une pièce gommée par geste
  s'enregistre, disparaît au rechargement, se réenregistre à l'identique, et l'annulation rend le
  fichier livré. Les trois retouches réelles ont suivi ce chemin, et `--check` passe sans
  avertissement.
- **Dernière génération des scripts, puis retrait de leur `--check`.** ✔ Tracés à l'ouverture =
  cartes commitées ; tracé + retouche = carte livrée ; lancés sur le dossier des cartes, les
  scripts refusent (code 1).
- **Une vraie retouche faite dans l'éditeur sur chacune des trois cartes.** ✔ Par `--apply`, voir
  « Conception » ; rendus relus à l'œil. La CI publie le rendu des trois cartes dans `map-renders` :
  c'est la première PR qui éprouve cette étape du `LOT-EDITOR-13`.
- **Guide d'usage : créer une carte de bout en bout.** ✔ @ref guide-usage-editeur ;
  `LevelFileOps.UneCarteCreeeAvecUnLieuASesDeuxCouches` garde le premier pas, et
  `ContentCheckTest.UneCarteNeuveASonNomDansChaqueCatalogue` le contrôle (`LOT-EDITOR-07`).

**Vérification à la souris, due** : lancer `LevelEditor` sans argument — le titre montre
`…\Source\Elements` ; « New » sur une carte `essai`, lieu `martpart` ; poser un rectangle de pavés,
un mur, l'entrée, un coffre ; `Ctrl+S`, et `git status` montre `Source/Elements/Levels/essai.json`.
Puis `P` pour l'essai, et supprimer la carte.

## Bilan

**Livré le 19 septembre 2026** (ouvert le même jour), sur la branche
`lot-editor-06-fin-des-scripts`. **Premier jalon du module** : les cartes du jeu se font
désormais dans l'éditeur. Vérification automatisée : construction `/W4 /WX` sans avertissement,
tests unitaires verts, `LevelEditor --check` sans erreur ni avertissement sur les trois cartes,
rendus relus à l'œil. **Vérification à la souris due** (voir « Vérification »).

Exigences : `EX-EDIT-077`, `EX-EDIT-078` (nouvelles).
