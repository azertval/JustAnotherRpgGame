+++
id = "LOT-EDITOR-12"
titre = "Le format v4 et sa garde en CI"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "M"
resume = "La seule révision de format du module : chaque case porte sa pièce, la collision se déduit des pièces, entités à identifiant, zones à forme, variantes et réserve de hauteur — le tout gardé en CI par `--check`."
prerequis = ["LOT-EDITOR-01"]
livrables = [
  "`Core` : `TileLayer` à pièces et hauteurs, `MapEntity` à identifiant, hauteur et cases, `Level` à cases forcées, compteur et variante ; chargeur v4 (v3 lue pour toujours) et écrivain canonique.",
  "`PieceFootprint`, type tactique et alias au manifeste, `core::deriveCollision`, `core::canonicalCollisionTile`, `LevelVariant` ; `BattleGrid::zonesAt` lit les zones peintes.",
  "`Editor/Logic/MapFormat` : `LevelEditor --migrate` et `--check` (`hmi::runMapCommand`), branchés dans `main` et dans `ci.yml` (`LevelEditor --data Source/Elements --check`).",
  "Les trois cartes migrées (Colisée, Martpart, Arenarea), les types tactiques du Colisée, les scripts des cartes passés par `--migrate`.",
  "`Documentation/Editeur/level.schema.json` et son test Python ; fuzz du chargeur amorcé en v4, variantes comprises.",
  "Fixtures v0 à v4 et variante (`Source/Test/Fixtures/Levels`) ; tests `FormatV4Test`, `MapFormatTest`.",
  "`EX-LVL-019` à `EX-LVL-024`, `EX-EDIT-062`.",
]
criteres = [
  "Les trois cartes migrées se jouent à l'identique (même instantané de scène, même `BattleGrid`).",
  "Une carte v1, v2 et v3 gardée en fixture se charge pour toujours.",
  "Charger puis enregistrer une carte intacte rend le même fichier.",
  "Le fuzz du chargeur couvre la v4.",
  "Une valeur `elevation` non nulle survit à un aller-retour et sort en avertissement du `--check`.",
]
+++

## Pourquoi

La seule révision de format du module, faite tant qu'il n'y a que trois cartes (décisions D3, D8,
D10, D11, D12 et D13 de la feuille de route) : la pièce quitte la grille de collision pour sa
couche, la collision se déduit des pièces, les entités ont un identifiant, les zones une forme, les
cartes des variantes, et la hauteur une place réservée. Et une garde en CI, avant que les scripts
des cartes ne partent (constat A12).

Feuille de route : [éditeur](@ref roadmap-editeur). Le lot alimente `LOT-EDITOR-03` et
`LOT-EDITOR-05`.

## Ce que le dépôt contenait à l'ouverture (19 septembre 2026)

- **Le format v3** : la pièce d'une case vivait sur la grille de collision (`"texture"` des tuiles
  racine), et seulement pour le relief ; le sol sortait de la table d'apparence du lieu.
- **La collision était peinte à part** : rien ne la tenait d'accord avec les pièces posées.
- **L'emprise des pièces larges n'était lue par personne** (A4) ; le composeur triait une pièce 2 × 1
  au pied de sa seule case d'ancrage.
- **Aucune garde des cartes en CI** : le `--check` des scripts `carte_colisee.py` et
  `carte_quartiers.py` ne tournait nulle part. Celui de `carte_quartiers.py` échouait d'ailleurs
  depuis le `LOT-93`, qui avait changé la figurine des sentinelles dans les cartes sans le suivre.

## Périmètre

### Livraison

| Commit | Contenu |
|---|---|
| 1 — phase 1 | `TileLayer` à pièces et hauteurs, `MapEntity` à identifiant, hauteur et cases, `Level` à cases forcées, compteur et variante ; chargeur v4 (v3 lue, `texture` rangée sur le décor) ; écrivain canonique ; `PieceFootprint`, type tactique et alias au manifeste, `deriveCollision`, `LevelVariant` ; `BattleGrid::zonesAt` lit les zones peintes ; la composition lit la pièce de chaque couche, les alias et l'emprise ; fixtures v0 à v4 et variante ; tests |
| 2 — phase 2 | `Editor/Logic/MapFormat` (`--migrate`, `--check`), branchement dans `main` et dans `ci.yml` ; les trois cartes migrées ; types tactiques du Colisée ; scripts des cartes par `--migrate` ; tests d'acceptation |
| 3 — phase 3 | `Documentation/Editeur/level.schema.json` et son test Python ; fuzz du chargeur amorcé en v4, variantes comprises |
| 4 — documentation | le dossier du lot, la feuille de route, `niveaux.md`, `editeur-niveaux.md`, le README des niveaux, le cahier de tests |

### Ce qui reste hors du lot, nommément

- **Peindre les pièces et voir les cases forcées** : `LOT-EDITOR-03`.
- **Les zones peintes et les variantes à la souris** : `LOT-EDITOR-05`, puis le `LOT-27` pour la
  première variante réelle (l'Arène du Destin).
- **Jouer la gêne et l'abri depuis une pièce** : une règle du jeu, pas du format ; la grille de
  collision devra alors savoir les écrire.
- **La borne de taille** (128 × 128, règle 5 de la feuille de route) : non posée ici.

## Conception

- **La grille racine reste la collision, sous le même nom.** `"tiles"` porte toujours la grille de
  collision et l'entrée ; le jeu la lit sans manifeste et ne change pas d'une ligne. Ce qui change :
  elle est **déduite** (`core::deriveCollision`) et vérifiée, hors des cases **forcées**
  (`"forced"`).
- **La collision se compare par ce qu'elle oppose, pas par son nom.** Une case `dirt` écrite
  s'accorde avec une case vide déduite (`core::canonicalCollisionTile`) : la v3 écrivait `dirt` sous
  toute pièce franchissable, uniquement pour que l'assignation de texture soit émise. La
  migration réécrit ces cases dans le vocabulaire canonique (`wall`, `cliff`, vide).
- **La règle de déduction**, case par case, la contribution la plus forte de : chaque pièce dont
  l'**emprise** couvre la case (son type tactique au manifeste) ; le type d'une case de couche sans
  pièce (mur et matière pleine arrêtent la vue, eau profonde et falaise le pas) ; et **une case que
  rien ne couvre est un mur** — on ne se tient pas là où il n'y a pas de sol.
- **Cinq types tactiques, dont deux pas encore joués.** Le manifeste déclare `open`, `difficult`,
  `cover`, `obstacle`, `solid` ; à défaut, un sol passe et une pièce debout arrête la vue. La grille
  de collision ne sait écrire que `wall`, `cliff` et vide : gêne et abri se déduisent
  franchissables, et `--check` le signale — même parti que la réserve de hauteur, plutôt qu'une
  demi-fonction.
- **La migration nomme la pièce de chaque case** d'après la table du lieu : ce qu'on voyait est
  désormais écrit, le type ne garde que son sens de règle (D3), et `appearance.json` ne sert plus
  que de défaut (cartes générées, case sans pièce). Les types des trois cartes sont restés tels
  quels : `solid` y désigne encore la place du marché de Martpart. Ils ne comptent plus pour rien,
  la pièce l'emportant partout ; les rendre justes est un travail d'auteur, pas de migration.
- **Les champs du format v4 viennent en fin de `core::MapEntity`.** Un `id` ajouté en tête décalait
  en silence toute initialisation positionnelle `{type, position, properties}` : deux tests s'en
  sont aperçus, un type `"chest"` devenant un identifiant.
- **Identifiants `e<n>`**, donnés par un compteur écrit dans la carte (`"nextEntityId"`) qui ne
  recule jamais, pas même à l'annulation : un identifiant retiré n'est jamais redonné (D8).
- **Une variante se résout au chargement du fichier** : `LevelLoader::loadFromFile` cherche
  `<dossier>/<base>.json` du dossier de la variante vers la racine ; `loadFromString` prend un
  résolveur, ou refuse. La variante chargée garde `base` et `scene` et se réécrit sans ses cases.
- **Zones** : une entité `zone`, rectangle (`width`, `height`) ou peinte (`cells`) ;
  `BattleGrid::zonesAt` la lit après les zones de couche. `combatZone` reste un rectangle.
- **Écriture canonique maison.** `nlohmann::ordered_json` pour l'ordre des champs (celui du
  format, pas l'alphabet), et un émetteur qui écrit **une case par ligne** dans `tiles`, `forced` et
  `cells` : poser une pièce change une ligne du diff. Martpart passe de 190 à 126 Ko, ses
  1 172 pièces nommées comprises.
- **`--migrate` et `--check` vivent dans `Editor/Logic/MapFormat`**, logique pure appelée par
  `main` avant toute construction Qt (`hmi::runMapCommand`), et par les tests. Racine des données :
  `--data`, le dossier de l'exécutable à défaut. La CI lance
  `LevelEditor --data Source/Elements --check` dans le job ninja.
- **Les scripts des cartes passent par `--migrate`** : ils tracent toujours du v3, et l'éditeur le
  convertit. Aucune règle de migration n'est donc dupliquée en Python ; les scripts demandent un
  éditeur construit, jusqu'à leur retrait au `LOT-EDITOR-06`.
- **Le type tactique naît dans la disposition de l'atelier.** Le manifeste est produit par
  `extract_texture_sheet.py` : une valeur ajoutée à la main s'y perdrait à la prochaine
  extraction. La disposition du Colisée déclare `"tactical": "open"`, le script la recopie (un
  miroir hérite de sa source), et le manifeste livré la porte déjà.
- **L'acceptation sur les vraies cartes compare aux v3 de l'historique.** Garder 470 Ko de v3 en
  fixture pour un seul test aurait été lourd ; le test
  `MapFormatTest.LesTroisCartesMigreesSeJouentALIdentique` lit les v3 dans `JADG_V3_MAPS_DIR`
  (tirées de `git show 376c541da:…`) et s'ignore sans elle. Une v0, v1, v2 et v3 de quelques cases
  restent, elles, en fixture pour toujours.

## Risques et questions ouvertes

### Ce que la migration a trouvé, à trancher par l'auteur

- **Le Colisée garde 540 cases forcées** — *libérées au `LOT-EDITOR-03`, décision de l'auteur*. 538 sont du **vide** — ni sol ni pièce — que la v3
  laissait franchissable ; la déduction en fait des murs, et la migration les a forcées pour que le
  jeu ne change pas. Deux sont des **piliers** posés sur des cases franchissables, là où deux autres
  piliers sont des obstacles. Martpart et Arenarea migrent sans aucune case forcée. Le masque des
  cases forcées se verra au `LOT-EDITOR-03` ; les retirer d'un geste rendra le vide infranchissable.
- **Des pièces larges se recouvrent** (trois avertissements du `--check`) : la loge du Colisée
  pose `box-left` (1 × 2) et `box-right` (2 × 1) sur des cases voisines dont les emprises se
  croisent en (16, 6) et (16, 7), et `gate-left` / `gate-right` en (21, 33). L'emprise n'étant lue
  par personne jusqu'ici (A4), cela ne se voyait pas.
- **Franchir un banc, un brasero ou un râtelier** : la v3 les rendait franchissables, et leur type
  tactique dit désormais `open` pour garder le jeu identique. S'ils doivent gêner ou arrêter, c'est
  une ligne de la disposition du Colisée.

## Vérification

- **Les trois cartes migrées se jouent à l'identique (même instantané de scène, même
  `BattleGrid`).** ✔ `MapFormatTest.LesTroisCartesMigreesSeJouentALIdentique`, lancé avec les v3
  de l'historique : instantané égal, grille tactique égale case par case (au sol, en vol, vue,
  terrain difficile, zones), même entrée — pour le Colisée, Martpart et Arenarea. Sur la fixture v3,
  en permanence : `MapFormatTest.LaMigrationNeChangePasCeQueLeJeuJoue`.
- **Une carte v1, v2 et v3 gardée en fixture se charge pour toujours.** ✔
  `FormatV4Test.UneCarteDeChaqueVersionSeCharge` (v0 à v4, `Source/Test/Fixtures/Levels`).
- **Charger puis enregistrer une carte intacte rend le même fichier.** ✔
  `FormatV4Test.UneV4CanoniqueRessortOctetPourOctet`, `…UneVarianteReprendLesCasesDeSaBase`, et
  `--check` sur toutes les cartes livrées, en CI.
- **Le fuzz du chargeur couvre la v4.** ✔ amorces v4 et variantes, résolveur de base dans le
  harnais ; une minute en local : 3 194 entrées, 287 nouvelles, aucun défaut.
- **Une valeur `elevation` non nulle survit à un aller-retour et sort en avertissement du
  `--check`.** ✔ `FormatV4Test.LaReserveDeHauteurSurvitALAllerRetour`,
  `MapFormatTest.ChaqueDefautSortEtLeControleEchoue`.

## Bilan

**Livré le 19 septembre 2026** (ouvert le même jour), sur la branche `lot-editor-12-format-v4`,
en quatre commits : le format dans `Core` (phase 1), `--migrate` et `--check` avec les trois
cartes migrées (phase 2), le schéma et le fuzz (phase 3), puis la documentation. Vérification
automatisée : construction `/W4 /WX` sans avertissement, tests unitaires verts (788), acceptation
sur les trois cartes livrées, `LevelEditor --check` vert, les deux scripts de cartes verts, une
minute de fuzz sans défaut.

Exigences : `EX-LVL-019`, `EX-LVL-020`, `EX-LVL-021`, `EX-LVL-022`, `EX-LVL-023`,
`EX-LVL-024`, `EX-EDIT-062` (nouvelles) ; `EX-LVL-003`, `EX-LVL-005`, `EX-LVL-016` et
`EX-EDIT-043` révisées.
