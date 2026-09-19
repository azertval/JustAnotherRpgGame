# LOT-EDITOR-07 — Contrôle du contenu {#lot-editor-07}

> Statut : **livré le 19 septembre 2026** (ouvert le même jour), sur la branche
> `lot-editor-07-controle-contenu`. Vérification automatisée : construction `/W4 /WX` sans
> avertissement, 860 tests unitaires verts, `LevelEditor --check` sans erreur sur les trois cartes
> (deux avertissements, exacts), panneau capturé à l'écran. **Vérification à la souris due** (voir
> plus bas).
> Prérequis : `LOT-EDITOR-06`. Feuille de route : [éditeur](@ref roadmap-editeur).
> Alimente : `LOT-EDITOR-11` (génération assistée : « le contrôle du 07 reste vert »), et les cartes
> du [LOT-27](@ref lot-27).
> Exigences : `EX-EDIT-079`, `EX-EDIT-080`, `EX-EDIT-081` (nouvelles).

## Objectif

Un panneau « Problems » couvre toutes les cartes du projet, et le contrôle de la CI s'étend : une
carte n'est plus seulement **bien écrite** (le format, `LOT-EDITOR-12`), elle **se joue**.

## Ce que le dépôt contenait à l'ouverture (19 septembre 2026)

- **`LevelEditor --check` ne regardait que le format** : version, écriture canonique, pièces,
  collision, identifiants, et les portails d'une carte à l'autre (`core::validateWorldGraph`).
- **Les références des entités et le terrain des rencontres** (le contrôle du `LOT-11`) ne se
  voyaient que dans la fenêtre, pour la **seule carte ouverte**, et ne faisaient rien échouer.
- **Aucune atteignabilité** n'était vérifiée nulle part, sauf par deux tests écrits pour les
  quartiers de la Capitale (`LOT-96`).
- **Aucune carte ne citait de clé de traduction.** Le seul texte de carte que le jeu affiche est
  son `name` (« Le Colisée »), montré tel quel par le bandeau ; le nom des îlots, lui, est déjà lu
  sous la clé `city_block.<nom>`.
- **Aucune variante** (décision D12) n'est livrée ; le chargeur les lit, sans rien qui dise que la
  base a changé sous elles.

## Décisions

- **Un seul contrôle, deux entrées.** `hmi::checkAllMaps` fait le format puis le contenu
  (`Editor/Logic/ContentCheck`) ; `--check` et le panneau l'appellent tous deux. Les catalogues
  (dialogues, rencontres, figurines, graphe du monde, traductions) se lisent **une fois** pour
  toutes les cartes (`hmi::ContentContext`) : trois cartes se contrôlent en moins de deux secondes,
  lancement compris.
- **Gravités.** Est une **erreur**, et fait échouer la CI : une référence cassée (dialogue,
  rencontre, figurine, lieu, objet, `carte#id`, propriété requise ou hors bornes), un drapeau de
  monde que rien ne pose, une rencontre dont la formation ne tient pas, une case utile hors
  d'atteinte ou dans un mur, une clé de traduction absente. Est un **avertissement** : une famille
  d'entité inconnue (légale, transportée, `EX-NFR-040`), une entrée d'arène hors de toute zone, un
  portail sans retour (une trappe peut être voulue), un point d'arrivée que rien ne nomme.
- **Rien n'est dit deux fois.** Un portail vers une carte absente, un point d'arrivée inconnu ou en
  double, une zone de combat dégénérée restent dits par le contrôle du format ; le contenu les
  écarte.
- **L'atteignabilité suit la règle de marche du jeu** (`core::ExplorationReach`, dans `Core`) : le
  héros d'`ExplorationSession` tient dans une case et se déplace axe par axe, donc il passe sur
  toute case non solide et ne passe **jamais par un coin**. D'où un parcours en quatre voisins ; un
  `static_assert` lie la règle au gabarit du héros. Ce n'est pas la `ReachableArea` du combat
  (`LOT-19`), qui coupe les diagonales.
- **Les départs** : l'entrée de la carte, et chaque point d'arrivée **nommé d'ailleurs** — par un
  portail d'une autre carte, ou par la porte de départ d'une ville (`World/cities`). Un point que
  rien ne nomme n'est pas un départ : c'est un point orphelin, et il doit lui-même être atteint.
- **Les cases utiles** : portail, point d'arrivée, rencontre, et les familles qu'on aborde en
  exploration (coffre, panneau, PNJ, `core::knownInteractableKinds`) se tiennent sur leur case ;
  l'interaction visant une case voisine **par un côté** et non solide, cela revient au même. Une
  zone de combat ou de règles demande une case atteinte. Îlots, trajets et entrées d'arène ne se
  parcourent pas : ils ne sont pas contrôlés.
- **Le nom d'une carte est une clé** — *décision de l'auteur, 19 septembre 2026*, entre trois
  options (nom de carte en clé, texte du panneau, mécanisme seul) : `map.<identifiant>.name`, les
  barres obliques devenant des points (`map.capital.martpart.name`). Le bandeau du jeu la traduit
  (`WorldModel::mapName` → `hmi::ruleLabel`) ; un nom qui n'est pas une clé s'affiche tel quel.
  Le contrôle exige la clé dans **chaque** catalogue `.lang`, comme `city_block.<nom>` pour les
  îlots. Sans aucun catalogue sous la racine (un projet de test), il le dit en avertissement.
- **Une carte créée, renommée ou dupliquée complète les catalogues.** Créer écrit la clé avec le
  nom tapé pour texte, dans toutes les langues ; renommer et dupliquer reprennent les traductions
  de l'ancienne clé, langue par langue. L'ancienne clé reste : la propager est au `LOT-EDITOR-14`.
  Dans l'éditeur, l'**identifiant** remplace le nom partout où le nom servait d'identité (barre
  d'état, boîte « Rename », graphe du monde).
- **Une variante se contrôle telle que le jeu la charge**, sur les cases de sa base : une base qui
  change sous ses entités (un PNJ muré, un portail hors d'atteinte) se voit sur la variante. Aucune
  empreinte de la base n'est ajoutée au format : on contrôle les conséquences, pas le changement.
- **Le panneau contrôle les fichiers**, pas le brouillon : sa ligne de bilan le dit (« checked as
  saved »). Il se remplit au lancement, après chaque enregistrement et sur « Check all maps » (menu
  *Map* ou bouton). Un double-clic ouvre la carte du constat (en demandant quoi faire des
  modifications en cours), sélectionne son entité — le constat porte son `id` — et cerne sa case
  d'un repère magenta.

## Livraison

| Partie | Contenu |
|---|---|
| `Core` | `World/ExplorationReach` : les cases qu'un héros atteint, règle de marche du jeu |
| `Editor/Logic` | `ContentCheck` (`loadContentContext`, `checkMapContent`) ; `MapTexts` (clé du nom, catalogues) ; `MapCheckFinding::entityId` ; `checkMapFile` et `checkAllMaps` font le contenu ; `LevelFileOperations` écrit la clé et complète les catalogues |
| `Editor/Ui` | `ProblemsPanel` (dock « Problems ») ; `EditorViewport::revealCell` ; fenêtre : « Check all maps », contrôle au lancement et à l'enregistrement, aller au constat ; identifiant au lieu du nom (barre d'état, « Rename », graphe) |
| Jeu | `WorldModel::mapName` traduit la clé |
| Données | les trois cartes nommées par clé ; `fr.lang` et `en.lang` : `map.coliseum.name`, `map.capital.martpart.name`, `map.capital.arenarea.name` |
| Tests | `test_content_check.cpp` (5) ; `test_exploration_reach.cpp` (3) ; `test_map_format.cpp`, `test_level_file_operations.cpp`, `test_capital_maps.cpp`, `test_coliseum_map.cpp` ajustés |

## Critères d'acceptation

- **Une carte de test avec un défaut de chaque sorte les fait tous sortir.** ✔
  `ContentCheckTest.ChaqueDefautDeContenuSort` : nom qui n'est pas une clé, îlot sans clé,
  dialogue inconnu, drapeau que rien ne pose, famille inconnue, rencontre dont un loup tombe hors
  de la carte, PNJ muré, coffre dans un mur, zone hors d'atteinte, portail sans retour, point
  d'arrivée orphelin, et une variante dont la base a muré le PNJ — chacun à sa gravité, avec son
  entité.
- **La CI échoue sur l'un d'eux.** ✔ `ContentCheckTest.UnSeulDefautFaitEchouerLaCi` : une carte
  canonique dont le seul défaut est un PNJ muré fait sortir `--check` en 1. La CI lance déjà
  `LevelEditor --data Source/Elements --check`.
- **Les cartes livrées passent.** ✔ `MapFormatTest.LesCartesLivreesPassentLeControle` : zéro
  erreur. Restent deux avertissements, exacts : les points d'arrivée `porte` et `sable` du Colisée,
  qu'aucun portail ne nomme encore — le Colisée ne sera relié au monde qu'au [LOT-27](@ref lot-27).
- **Double-clic = aller à la case.** Branché ; les clics ne se simulent pas sur ce poste (constat
  du `LOT-EDITOR-02`) : vérification à la souris due.

**Vérification à la souris, due** : lancer `LevelEditor` ; le dock « Problems » montre « 3 maps
checked as saved: 0 errors, 2 warnings ». Ouvrir Martpart, puis double-cliquer l'avertissement
`spawnPoint e16 (sable)` : le Colisée s'ouvre, le point d'arrivée est sélectionné, sa case cernée
de magenta. Puis poser un PNJ dans un mur, enregistrer : une erreur apparaît ; `Ctrl+Z`,
enregistrer, elle disparaît.

## Ce qui reste hors du lot, nommément

- **Contrôler le brouillon non enregistré** : le panneau lit les fichiers. Le panneau « Entities »
  garde les avertissements en direct de la carte ouverte.
- **Propager une clé renommée**, retirer l'ancienne : `LOT-EDITOR-14`.
- **Traduire le texte d'un panneau (`sign`)** : la famille n'a pas de propriété que le jeu lise.
- **Relier le Colisée au monde** (ce qui lèvera ses deux avertissements) : [LOT-27](@ref lot-27).
