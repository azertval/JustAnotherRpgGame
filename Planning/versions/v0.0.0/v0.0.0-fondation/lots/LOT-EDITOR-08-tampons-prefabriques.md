+++
id = "LOT-EDITOR-08"
titre = "Tampons et préfabriqués"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "S"
resume = "Un morceau de carte composé une fois se repose ailleurs, se garde dans la bibliothèque de son lieu, et une carte neuve part d'un modèle plutôt que du vide."
prerequis = ["LOT-EDITOR-04"]
livrables = [
  "`Editor/Logic` `Stamps` : `Stamp`, `cutStamp`, `mirrorStamp`, `pasteStamp`, `stampLabel` ; `copyTypeBlock` retirée.",
  "Le format d'un préfabriqué (`stampToJson`, `stampFromJson`, `writePrefab`, `readPrefab`, `prefabNames`), rangé par lieu dans `Editor/Prefabs/<lieu>/<nom>.json` ; `checkEditorLibrary`, `runPrefabCommand`.",
  "Les modèles de carte (`MapTemplate`, `mapTemplates`) : `Editor/Templates/{interior,street,arena}.json` ; `LevelFileOperations::create` prend un modèle.",
  "`Editor/Ui` : le presse-papier est un tampon, « Paste mirrored » (`Ctrl+Maj+V`), « Save selection as prefab… », onglet « Prefabs » de la palette à vignette générée (`MapRender::renderStamp`), modèle au choix de « New map ».",
  "`App/Editor` : `--list-prefabs`, `--save-prefab` ; `GestureScript` : `prefab` s'arme, `selection … then: copy`, `paste` prend `flip`.",
  "`core::MapEntity::operator==` ; tests `test_stamps.cpp` (9), scénario `selection.json` refait.",
  "`EX-EDIT-085`, `EX-EDIT-086`, `EX-EDIT-087`.",
]
criteres = [
  "Un étal de Martpart se repose ailleurs avec son marchand.",
  "Les modèles livrés se lisent, et la bibliothèque livrée ne laisse aucun constat.",
]
+++

## Pourquoi

Une maison de Martpart demande une dizaine de gestes (`LOT-EDITOR-04`) ; la deuxième en demandait
autant. Ce lot fait qu'un morceau de carte composé une fois **se repose** ailleurs, **se garde**
dans la bibliothèque de son lieu, et qu'une carte neuve **part d'un modèle** plutôt que du vide.

Feuille de route : [éditeur](../../../../vision/archives/feuille-de-route-editeur.md).

## Ce que le dépôt contenait à l'ouverture (20 septembre 2026)

- **`Ctrl+C` ne copiait que des types**, sur la **couche active** : `hmi::copyTypeBlock` rendait un
  bloc de `core::TileType` que `hmi::paintTypeBlock` repeignait. Les pièces — c'est-à-dire tout ce
  qu'on voit depuis le `LOT-EDITOR-03` — et les entités restaient sur place. Copier l'étal du
  marché rendait deux cases de type `wall`, sans l'étal.
- **Aucune bibliothèque** : rien ne survivait à la fermeture de la carte, et rien ne passait d'une
  carte à l'autre.
- **Une carte neuve naissait vide** (`LevelFileOperations::create`, `LOT-EDITOR-06`) : deux couches
  en dur nommées `sol` et `relief`, toute la grille pleine, une case de terre sous l'entrée.
- Le miroir existait déjà, mais **pour un geste**, pas pour un morceau : `hmi::mirrorPieceName` et
  la diagonale `colonne − ligne = k` du `LOT-EDITOR-04`.

## Périmètre

### Livraison

| Partie | Contenu |
|---|---|
| `Core` | `MapEntity::operator==` |
| `Editor/Logic` | `Stamps` : `Stamp`, `cutStamp`, `mirrorStamp`, `pasteStamp`, `stampLabel`, le format d'un préfabriqué (`stampToJson`, `stampFromJson`, `writePrefab`, `readPrefab`, `prefabNames`), les modèles (`MapTemplate`, `mapTemplates`), `checkEditorLibrary`, `runPrefabCommand` ; `LevelFileOperations::create` prend un modèle ; `GestureScript` : `prefab` s'arme, `selection … then: copy` découpe un tampon, `paste` prend `flip` ; `copyTypeBlock` retirée |
| `Editor/Ui` | `EditorViewport` : le presse-papier est un tampon (`copySelection`, `pasteClipboard`, `pasteMirroredClipboard`, `setClipboardStamp`, `selectionStamp`) ; `PalettePanel` : onglet « Prefabs » ; `MainWindow` : « Paste mirrored », « Save selection as prefab… », la bibliothèque du lieu ; `LevelBrowserPanel` : le modèle au choix de « New map » ; `MapRender::renderStamp` |
| `App/Editor` | `--list-prefabs`, `--save-prefab` |
| Données | `Editor/Templates/{interior,street,arena}.json` |
| Tests | `test_stamps.cpp` (9) ; `selection.json` refait (tampon, reflet) |

### Ce qui reste hors du lot, nommément

- **Les références entre entités d'un même tampon** (`carte#id`) ne sont pas réécrites à la pose :
  aucune famille ne déclare encore de propriété de source `EntityRefs` (`LOT-EDITOR-14`) ; les
  quêtes du [LOT-16](../../../../vision/archives/feuille-de-route-jeu.md#lot-16) en seront, et c'est là que la question se posera.
- **Aucun préfabriqué n'est livré** : la bibliothèque est celle de l'auteur, et un étal figé dans
  le dépôt vieillirait avec la planche. `--save-prefab` la remplit en une commande.
- **La suppression et le renommage d'un préfabriqué** ne se font pas depuis la fenêtre : ce sont
  des fichiers, que l'explorateur suffit à déplacer.
- **Le tampon ne survit pas à la fermeture de l'éditeur** : ce qui doit durer devient un
  préfabriqué.

## Conception

- **Le tampon est un rectangle de carte complet** (`hmi::Stamp`) : les types de **chaque** couche
  visuelle, les pièces qui y sont ancrées, les entités qui s'y tiennent, les cases dont la
  collision est forcée. Pas « ce qu'il y a sur la couche active » : un étal est un sol, un décor et
  un marchand à la fois, et n'en prendre qu'une bande ne servirait personne.
- **Une pièce est prise entière ou pas du tout.** Elle l'est si sa case d'**ancrage** est dans le
  rectangle, qui s'agrandit alors jusqu'à contenir son emprise — sinon un étal 2 × 1 choisi au bord
  perdrait sa moitié droite. Une pièce qui couvre le rectangle mais est ancrée dehors n'est pas
  prise : elle appartient à ce qu'on laisse. *Écarté* : découper une pièce, ce qui n'a pas de sens
  (une pièce est une image, pas des cases) ; refuser le découpage, ce qui obligerait à viser juste.
- **Une entité est prise si sa case est dans le rectangle**, avec sa forme entière (`cells`), qui
  peut déborder : une zone est à elle-même sa propre étendue, et le rectangle ne s'agrandit pas
  pour elle. Son **identifiant ne suit pas** : le tampon ne porte que ce qu'elle est, et chaque
  pose en donne un neuf (décision D8) — c'est le critère d'acceptation du lot.
- **L'entrée n'est jamais prise.** Elle est unique dans une carte ; deux poses la déplaceraient
  sans qu'on l'ait demandé.
- **Un tampon remplace ce qu'il couvre**, il ne s'y ajoute pas : chaque case de son rectangle reçoit
  le type qu'il porte, vide compris. C'est la seule règle qui rende une pose prévisible ; l'auteur
  choisit son rectangle serré.
- **Une couche va à la couche de même nom**, à défaut à la première couche visuelle de même rôle,
  à défaut la pose est refusée et le dit. Une couche n'a pas d'identité dans le format : son nom
  est ce qui s'en rapproche le plus, et les cartes livrées le partagent (`sol`, `relief`).
- **Toute la pose est un geste** (`core::GestureScope`) : un `Ctrl+Z` la défait, entités comprises.
- **Le miroir d'un tampon est le sien** : la diagonale de son coin haut gauche, celle que
  connaissent les planches (`LOT-EDITOR-04`) — la case (c, r) passe en (r, c), un tampon w × h
  devient h × w, chaque pièce prend sa jumelle. Le miroir **armé** du canevas, lui, ne s'applique
  pas à une pose : un tampon est déjà une composition, et la refléter au loin la poserait ailleurs
  qu'au curseur. `Ctrl+Maj+V` pose le reflet.
- **La bibliothèque est rangée par lieu** (`Editor/Prefabs/<lieu>/<nom>.json`, format
  `jadg-editor-prefab`, version 1) : un préfabriqué nomme des pièces, et une pièce n'existe que
  dans la planche d'un lieu. Un nom est fait de minuscules, de chiffres, de tirets et de tirets bas
  — un nom de fichier sûr sur tout poste.
- **La vignette est générée, pas enregistrée** : la palette pose le tampon sur une carte jetable de
  sa taille et la rend par le peintre du canevas (`hmi::renderStamp`, `EX-EDIT-059`), puis la garde
  en mémoire. *Écarté* : un PNG à côté du JSON — un binaire de plus à versionner, et qui mentirait
  le jour où le préfabriqué change.
- **Un modèle de carte ne nomme aucune pièce** (`Editor/Templates/<id>.json`, format
  `jadg-editor-map-template`, version 1) : il sert tous les lieux, et son tampon ne pose que des
  types. Il donne les couches, la taille, l'entrée — choisir un modèle reprend sa taille, que
  l'auteur peut encore changer ; ce que le modèle ne couvre pas reste plein.
- **La bibliothèque a ses commandes sans fenêtre** (règle 4) : `--list-prefabs` et `--save-prefab`
  appellent `hmi::cutStamp` et `hmi::writePrefab`, les fonctions mêmes de la fenêtre. C'est ainsi
  qu'un préfabriqué livré se fabrique, et qu'un scénario `--apply` peut en armer un.
- **`copyTypeBlock` est retirée** : le tampon la remplace partout, et une fonction que plus rien
  n'appelle est une fausse piste.
- **`core::MapEntity` reçoit son `operator==`** : un tampon relu se compare à celui qu'on a écrit.

## Vérification

- **Un étal de Martpart se repose ailleurs avec son marchand.** ✔
  `DonneesPrefabriques.UnEtalDeMartpartSeReposeAvecSonMarchand` : un marchand posé sur l'étal du
  marché (`feature-1`, 2 × 1, ancré en (21, 24)), la case d'ancrage seule sélectionnée, le tampon
  fait 2 × 1 et emporte l'étal entier, son sol et le marchand ; enregistré, relu, il se pose sur
  **Arenarea** — qui prend ses pièces de la planche de Martpart (`LOT-96`) —, le marchand y reçoit
  l'identifiant libre suivant, et **un seul `Ctrl+Z`** rend la carte. Au compteur d'identifiants
  près, qui ne recule pas : un identifiant donné n'est jamais réemployé (décision D8), et c'est la
  seule différence que le test laisse passer.
- **Les modèles livrés se lisent, et la bibliothèque livrée ne laisse aucun constat.** ✔
  `DonneesPrefabriques.LesModelesLivresSeLisent` : intérieur, rue, arène, chacun avec ses couches
  et sa couche qui nomme le lieu ; `hmi::checkEditorLibrary` ne rend rien sur les données livrées.

**Vérification à la souris, faite** au [LOT-127](../../../v0.1.0/v0.0.1-demo/lots/LOT-127-recette-de-l-editeur-a-la-main.md), le 24 septembre 2026 : ouvrir Martpart, outil **Sélection** (`S`), tirer autour de
l'étal du marché ; `Ctrl+C` — la barre d'état dit `2 × 1 · 1 piece`. Survoler une rue vide,
`Ctrl+V` : l'étal est là, un `Ctrl+Z` le retire. `Ctrl+Maj+V` : il est posé dans l'autre sens.
*Edit* › *Save selection as prefab…*, le nommer `etal-du-marche` : l'onglet **Prefabs** de la
palette s'allume et montre sa vignette. Ouvrir Arenarea, cliquer le préfabriqué, `Ctrl+V`.
*File* › *New map*, choisir le modèle **Interior** : la taille suit, la carte naît avec ses quatre
murs et sa porte, et le dock « Problems » reste à 0 erreur.

## Bilan

**Livré le 20 septembre 2026** (ouvert le même jour), sur la branche
`lot-editor-08-tampons-prefabriques`, sur `main` après le `LOT-EDITOR-14`. Vérification
automatisée : construction `/W4 /WX` sans avertissement, tests unitaires du module verts, les
deux critères d'acceptation éprouvés sur les données livrées. **Faite au [LOT-127](../../../v0.1.0/v0.0.1-demo/lots/LOT-127-recette-de-l-editeur-a-la-main.md)** : la vérification à
la souris.

Exigences : `EX-EDIT-085`, `EX-EDIT-086`, `EX-EDIT-087` (nouvelles).
