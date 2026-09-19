# LOT-EDITOR-14 — Renommer et remplacer {#lot-editor-14}

> Statut : **livré le 19 septembre 2026** (ouvert le même jour), sur la branche
> `lot-editor-14-renommer-remplacer`, sur `main` après le `LOT-EDITOR-07`. Vérification
> automatisée : construction `/W4 /WX` sans avertissement, tests unitaires verts, les deux
> critères d'acceptation éprouvés sur une copie des données livrées. **Restent dues** : la
> vérification à la souris, et la planche d'Arenarea (voir plus bas).
> Prérequis : `LOT-EDITOR-06`. Feuille de route : [éditeur](@ref roadmap-editeur).
> Exigences : `EX-EDIT-082`, `EX-EDIT-083`, `EX-EDIT-084` (nouvelles) ; `EX-EDIT-081` révisée.

## Objectif

Un nom qui change ne casse plus rien (constat A7 : l'identifiant d'une carte est son chemin, que
d'autres fichiers citent), et une carte survit à la planche qui change (règle 3 de la feuille de
route).

## Ce que le dépôt contenait à l'ouverture (19 septembre 2026)

- **Renommer une carte** ne changeait que son fichier, dans son dossier, et ajoutait la nouvelle
  clé de son nom aux catalogues : le portail d'Arenarea, les quartiers et portes gardées de
  `World/cities/capital.json` citaient encore l'ancien identifiant, et l'ancienne clé restait.
  Le `--check` le voyait, rien ne le réparait. Aucune carte ne changeait de dossier.
- **Un point d'arrivée, un identifiant d'entité** se renommaient dans l'inspecteur, sans rien
  suivre.
- **Une pièce** ne se remplaçait qu'à la main, case par case ; une planche réextraite ne laissait
  que les `aliases` du manifeste et le damier.
- **Arenarea** emprunte la planche de Martpart (décision du `LOT-96`, à confirmer par l'auteur) ;
  **aucune planche Arenarea n'existe** : l'atelier du [LOT-92](@ref lot-92) ne l'a jamais
  commandée.

## Décisions

- **Un plan, puis l'écriture.** Chaque opération lit tout le projet et calcule chaque fichier à
  récrire (`hmi::RefactorPlan`) sans rien écrire ; `hmi::applyRefactorPlan` écrit ensuite. Une
  opération refusée (nom pris, identifiant invalide, carte du projet illisible, pièce absente)
  n'écrit donc aucun fichier. La fenêtre montre le plan avant de l'écrire.
- **Ce qui cite se lit dans la table des familles**, pas famille par famille : une propriété de
  source `Maps`, `ArrivalPoints` ou `EntityRefs` dans `core::knownEntityKinds` est suivie sans code
  (règle 2). Un point d'arrivée est cité par une propriété `ArrivalPoints` **dont l'entité vise sa
  carte** (le `targetMap` du portail) : deux points homonymes de deux cartes ne se confondent pas.
  Aucune famille ne déclare encore `EntityRefs` ; les quêtes du [LOT-16](@ref lot-16) en seront.
- **Hors des cartes**, trois fichiers citent : les villes (`district.map`, `guard.map`,
  `start.arrival`), récrites en JSON indenté de deux espaces comme le dépôt les écrit ; les
  catalogues `.lang`, où la **clé** du nom change ligne par ligne, texte et place gardés
  (l'ancienne clé ne reste plus, ce qu'annonçait le `LOT-EDITOR-07`) ; l'annexe de la carte, qui la
  suit. `world-maps.json` cite des fiches de lieu, pas des cartes : rien à y faire.
- **Une carte peut changer de dossier** (`coliseum` → `capital/coliseum`) : c'est le renommage
  « sous-dossiers » que le `LOT-EDITOR-06` renvoyait ici. Le dossier qu'elle vide ne reste pas.
  « Rename » (navigateur de cartes et menu *File*) prend désormais l'identifiant complet, et
  `LevelFileOperations::rename` passe par le même plan : il n'existe plus de renommage qui ne
  propage pas.
- **Renommer une pièce, c'est l'affaire de l'atelier** : la disposition la renomme, le manifeste
  garde l'ancien nom en `aliases`, et les cartes continuent de se lire (règle 3). L'éditeur, lui,
  **remplace** : « Replace piece » récrit toutes les cases qui posent la pièce, sous son nom
  courant ou non. Sur la carte ouverte, c'est un pas d'annulation
  (`core::LevelDraft::replacePieces`) ; sur toutes les cartes, un plan. La pièce garde sa case
  d'ancrage et son type (décision D3), la collision suit ; un sol ne se remplace que par un sol.
- **Changer de planche, c'est traduire, pas repeindre.** Chaque pièce posée va à la pièce de même
  nom — ou dont elle est un ancien nom — de la nouvelle planche ; une **table de correspondance**
  donne les autres (dialogue « Change sheet », ou fichier `jadg-piece-table` pour
  `--change-scene`). Une pièce sans correspondant refuse le changement : elle deviendrait un
  damier. Toute la collision se redéduit par le nouveau manifeste
  (`core::LevelDraft::changeScene`). Une variante ne change que sa propre planche (`scene`), et ce
  sont les pièces de sa base qui doivent exister sur la nouvelle.
- **La table d'apparence suit la planche.** Le rendu des îlots (`CityBlockImageProvider`) et les
  cases sans pièce lisent `Assets/Scene/<lieu>/appearance.json`, que l'atelier ne produit pas. Un
  lieu qui n'en a pas reçoit, au changement de planche, celle de l'ancien lieu traduite par la
  même table.
- **Arenarea n'a pas encore sa planche** — *décision de l'auteur, 19 septembre 2026*, entre trois
  options (livrer le mécanisme et la commande, produire d'abord la planche, changer le critère) :
  le mécanisme est livré et éprouvé sur une planche d'essai, la disposition `arenarea` de
  l'atelier est écrite (modèle `quartier`, comme Martpart) et sa commande se prépare. Le passage
  réel attend la planche, comme la vérification à la souris attend l'auteur.
- **Les commandes sans fenêtre** : `--who-cites`, `--rename-map`, `--rename-arrival`,
  `--rename-id`, `--replace-piece`, `--change-scene`, chacune suivie au besoin de `--check`. Pas de
  geste `--apply` pour remplacer ou changer de planche : `--replace-piece` et `--change-scene`
  appellent les mêmes fonctions du brouillon que la fenêtre (règle 4).

## Livraison

| Partie | Contenu |
|---|---|
| `Core` | `LevelDraft::replacePieces`, `LevelDraft::changeScene`, `core::PieceRenaming` : un pas d'annulation chacun, la collision suit |
| `Editor/Logic` | `MapRefactor` : citations, plans, `applyRefactorPlan`, table de correspondance, `runRefactorCommand` ; `LevelFileOperations::rename` passe par le plan |
| `Editor/Ui` | `RefactorDialogs` (citations, confirmation, remplacement, changement de planche) ; menu *Map* ; « Rename » prend un identifiant complet ; `EditorViewport::replacePieces`, `changeScene` (`renameOpenLevel` retiré) |
| `App/Editor` | les commandes sans fenêtre, puis `--check` si demandé |
| Atelier | `LOT-92-atelier-textures/atelier/dispositions/arenarea.json` |
| Tests | `test_map_refactor.cpp` (9, sur une copie des données livrées) ; `test_level_draft_pieces.cpp` (+3) ; `test_level_file_operations.cpp` (cartes sous `Levels/`) |

## Critères d'acceptation

- **Renommer `capital/martpart` laisse le `--check` vert.** ✔
  `Donnees.RenommerMartpartLaisseLeControleVert` : zéro erreur, autant d'avertissements qu'avant
  (les deux points du Colisée) ; portail d'Arenarea, quartier et portes gardées de la Capitale,
  clé du nom dans `fr.lang` et `en.lang` suivent. Même chose en ligne de commande :
  `LevelEditor --data <copie> --rename-map capital/martpart capital/marche --check` sort en 0.
- **Arenarea passe de la planche de Martpart à la sienne sans être repeinte.** ✔ sur une planche
  d'essai ; **dû** sur la vraie. `Donnees.ArenareaChangeDePlancheSansEtreRepeinte` : une planche
  `arenarea` où `street-2` s'appelle `paving-2` et `street-3` est devenue `cobbles` (ancien nom
  `street-3`) ; sans table, le changement est refusé et nomme `street-2` seul ; avec la table,
  la carte nomme sa planche, pose `paving-2` et `cobbles`, sa collision n'a pas bougé d'une case,
  sa table d'apparence est créée, et le contrôle reste vert.

**Pour finir le critère sur la vraie planche** (quand l'auteur la produit) :

1. `py -3.13 scripts/extract_texture_sheet.py commande arenarea 1`, envoyer la planche au
   générateur, l'enregistrer où le dit `A_ENREGISTRER_SOUS.txt` ;
2. `py -3.13 scripts/extract_texture_sheet.py decoupe arenarea <planche-1.png>` ;
3. `LevelEditor --data Source/Elements --change-scene capital/arenarea arenarea --check` — sans
  table : le modèle `quartier` donne à Arenarea les noms de pièces de Martpart. Le contrôle vert,
  relire le rendu (`--render capital/arenarea`) et l'îlot du plan.

**Vérification à la souris, due** : ouvrir Martpart ; menu *Map* › *Who cites this map?* : le
portail d'Arenarea, la ville, les deux clés ; double-cliquer le portail, Arenarea s'ouvre, le
portail sélectionné. *Replace piece…* `street-2` → `street-3` sur la carte : un `Ctrl+Z` le
défait. *Change sheet…* vers `coliseum` : la table montre les trous, *OK* reste grisé. *File* ›
*Rename* `capital/marche`, puis de nouveau `capital/martpart` : le dock « Problems » reste à
0 erreur.

## Ce qui reste hors du lot, nommément

- **La planche d'Arenarea** : atelier du [LOT-92](@ref lot-92), à la main de l'auteur.
- **Changer de planche une variante depuis la fenêtre** : `--change-scene` le fait ; la fenêtre
  n'édite pas encore les variantes.
- **Les `carte#id` des quêtes** : suivis dès qu'une famille déclare la source `EntityRefs`
  ([LOT-16](@ref lot-16)).
- **Relier deux cartes depuis le graphe du monde** : `LOT-EDITOR-09`.
