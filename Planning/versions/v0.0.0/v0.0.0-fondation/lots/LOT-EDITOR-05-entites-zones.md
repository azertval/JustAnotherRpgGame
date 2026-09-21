+++
id = "LOT-EDITOR-05"
titre = "Entités et zones sur le canevas"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "M"
resume = "Les zones se tirent à la souris et les entités montrent figurine, étiquette et liens, sans qu'aucune famille ait de code propre dans le canevas : tout vient d'un schéma typé."
prerequis = ["LOT-EDITOR-02", "LOT-EDITOR-12"]
livrables = [
  "`core::EntityKinds` en schéma typé : forme (`Point`, `Rectangle`, `Area`, `Path`), étiquette, figurine, bornes, cinq sources de choix, familles `zone` et `route`, six défauts nouveaux.",
  "`CombatZone` : `combatZoneOf`, `core::analyzeCombatZones` (verdict tactique) ; `LevelDraft::replaceEntity` ; `WorldMapNode::entityIds`.",
  "`Editor/Logic` : `EntityShapes` (formes, poignées, zone peinte, trajet, choix sous le curseur, filtre), `EntityGesture` refait, `EntityReferences` (figurines, drapeaux, lieux, objets, `carte#id`), `EditorDiagnostics`, `CanvasScene::formationFigures`, outil `Shape`.",
  "`Editor/Ui` : formes, marqueur ou figurine, étiquettes, huit poignées, aperçu du glisser, verdict de zone, outil Forme (`Z`) ; panneau filtrable à sélection étendue, entiers bornés.",
  "Tests : `test_entity_shapes.cpp` (11, dont l'acceptation), `test_entity_kinds.cpp` (+3, dont le test bloquant des familles), `test_entity_editing.cpp`.",
  "`EX-EDIT-070` à `EX-EDIT-073`.",
]
criteres = [
  "Redimensionner la zone de combat du Colisée à la souris met à jour son verdict tactique.",
  "Aucune famille n'a de code propre dans le canevas.",
  "Un test bloque toute famille lue par le jeu et absente de `EntityKinds`.",
]
+++

## Pourquoi

Les zones se tirent à la souris, les entités montrent leur figurine et leurs liens. Aucune famille
n'a de code propre dans le canevas : tout vient de la table des familles, `core::knownEntityKinds`,
qui devient un **schéma typé**.

Feuille de route : [éditeur](../../../../vision/archives/feuille-de-route-editeur.md). Le lot alimente `LOT-EDITOR-06` (fin des
scripts) et `LOT-EDITOR-09` (monde).

## Ce que le dépôt contenait à l'ouverture (19 septembre 2026)

- **Des entités ponctuelles seulement.** Le canevas dessinait le marqueur de chaque entité à sa
  case ; une zone de combat ou un îlot n'était qu'un coin, sa taille deux nombres dans
  l'inspecteur. Déplacer une zone peinte ne déplaçait que sa case, pas ses cases.
- **La famille `zone` manquait à la table** alors que `core::BattleGrid` la lit depuis le
  `LOT-EDITOR-12` : l'éditeur ne savait ni la poser ni l'inspecter. C'est exactement ce que la
  règle 2 de la feuille de route interdit, et rien ne le signalait.
- **Un inspecteur à trois sources de choix** (dialogues, rencontres, cartes) ; la figurine d'un PNJ
  et le quartier d'une sentinelle se tapaient à la main, sans contrôle ; aucun entier n'était
  borné.
- **Une sélection d'une entité**, une liste non filtrable sans identifiant.

## Périmètre

### Livraison

| Partie | Contenu |
|---|---|
| `Core` | `EntityKinds` : forme, étiquette, figurine, bornes, cinq sources de choix, familles `zone` et `route`, six défauts nouveaux ; `CombatZone` : `combatZoneOf`, `analyzeCombatZones` ; `LevelDraft::replaceEntity` ; `WorldMapNode::entityIds` |
| `Editor/Logic` | `EntityShapes` (formes, poignées, zone peinte, trajet, choix sous le curseur, filtre) ; `EntityGesture` refait (prise, bascule, tracé, glisser de groupe, outil Forme) ; `EntityReferences` : figurines, drapeaux, lieux, objets, `carte#id` ; `EditorDiagnostics` : zones de combat et entrées d'arène ; `CanvasScene::formationFigures` ; outil `Shape` |
| `Editor/Ui` | canevas : formes, marqueur ou figurine, étiquettes, poignées, aperçu du glisser, verdict de zone, outil Forme ; panneau : filtre, colonnes identifiant et étiquette, sélection étendue, entiers bornés, verdict ; `Z` |
| Tests | `test_entity_shapes.cpp` (11, dont l'acceptation), `test_entity_kinds.cpp` (+3, dont le test bloquant), `test_entity_editing.cpp` (gestes refaits, trois cartes livrées sans avertissement) |

### Ce qui reste hors du lot, nommément

- **Les horaires d'un trajet** et la ronde dessinée fermée : avec l'horloge du `LOT-70`.
- **Insérer un point au milieu d'un trajet** : on le prolonge, on en porte un point, on en retire
  un ; l'insertion viendra si le tracé des rondes la réclame.
- **Éditer une propriété sur toute la sélection à la fois** : l'inspecteur montre l'entité
  principale.
- **Une sélection au lasso** sur le canevas : `Maj` + clic et la liste filtrée suffisent aux cartes
  d'aujourd'hui.
- **Le catalogue des lieux ne distingue pas les quartiers** des autres fiches de l'atlas : une
  sentinelle peut nommer une ville. Le contrôle du contenu (`LOT-EDITOR-07`) pourra resserrer.

## Conception

- **La forme, pas le type.** `core::EntityKind` gagne `shape` (`Point`, `Rectangle`, `Area`,
  `Path`), `labelProperty` et `figureProperty`. Le canevas, les poignées, le déplacement et
  l'inspecteur ne lisent que ça : ajouter une famille à la table suffit (§5, règle 2).
- **Quatre formes.** Le rectangle (zone de combat, îlot) est ce que le jeu lit : l'outil Forme ne le
  change pas en cases. La zone de règles (`zone`, décision D13) est rectangle **ou** peinte, et
  devient peinte à sa première retouche : ses cases sont copiées, `width` et `height` partent, les
  cases restent triées, la case de l'entité reste une case de la zone. Le trajet est une ligne
  brisée dont les points vivent dans `cells`, **dans l'ordre**, le premier sur la case de l'entité
  — aucune révision du format : l'écrivain gardait déjà l'ordre.
- **La famille `route` entre dans la table avant que le jeu ne la lise** (`LOT-70`, `LOT-82`) :
  la feuille de route la demande ici, et le format lui garde sa place comme à la hauteur. Ses
  propriétés : `name` (requis) et `loop` (une ronde). Les horaires attendent l'horloge du `LOT-70`.
- **Tirer au lieu de poser.** Une famille à forme se tire du clic au relâchement : un rectangle
  entre deux coins, un trajet de deux points. Une famille ponctuelle se pose au clic, comme avant.
- **Ce qui est sous le curseur**, dans l'ordre : une poignée d'une entité sélectionnée ; une entité
  dont c'est la case ou un point de passage, la dernière posée d'abord ; enfin le **corps** de la
  plus petite forme qui couvre la case. Le corps n'empêche pas de poser : on pose les entrées
  d'arène dans la zone de combat. Un îlot de Martpart couvre des centaines de cases ; sans « la plus
  petite d'abord », on ne prendrait jamais une zone posée dedans.
- **Huit poignées par rectangle** : les coins, puis le milieu de chaque côté, omis quand il
  tomberait sur un coin. Tirer au-delà du côté opposé retourne le rectangle, qui garde une case.
- **Un geste, un pas**, comme au `LOT-EDITOR-04` : le brouillon gagne `replaceEntity` (type et
  identifiant intouchables, refus si une case sort de la carte), et le canevas écrit le résultat
  d'un glisser — un groupe déplacé, une zone tirée — dans un seul `core::GestureScope`. Pendant le
  glisser, le canevas dessine la valeur que le relâchement écrira : l'aperçu et l'écriture sont la
  même fonction pure (`hmi::dragEntities`).
- **Le groupe bouge entier ou pas du tout** : une entité qui sortirait de la carte refuse tout le
  déplacement. Un groupe écartelé contre un bord ne se remet pas en place.
- **Multi-sélection** : `Maj` + clic bascule une entité ; prendre une entité de la sélection emporte
  la sélection ; `Suppr` la retire en un pas. L'inspecteur montre l'entité **principale**, la
  dernière prise. La liste se filtre (famille, identifiant, valeurs) et se sélectionne à plusieurs.
- **L'outil Forme** (`Z`) retouche l'entité principale : il peint la zone (`Ctrl` gomme), prolonge
  le trajet d'un clic, en porte un point au glisser, le retire par `Ctrl` + clic. Il ne vide jamais
  une zone, ni un trajet de son dernier point.
- **La figurine à la place du marqueur**, quand la famille en nomme une et que l'atelier l'a : la
  scène du jeu la dessine déjà (PNJ, `LOT-09`), le marqueur ne se superpose plus.
- **L'étiquette** de la famille s'écrit au-dessus de sa case, à taille fixe à l'écran : la carte
  cible d'un portail, le nom d'une zone, d'un point d'arrivée, d'un trajet, la rencontre d'un
  déclencheur, le camp d'une entrée d'arène. Une teinte par famille, tirée de son type, colore les
  formes sans table de couleurs.
- **La formation d'une rencontre** sélectionnée se dessine par les figurines de l'atelier des
  monstres (`Monsters/<créature>`) ajoutées à la scène composée, triées avec elle ; une créature
  sans figurine garde sa case colorée.
- **Le verdict tactique d'une zone de combat** est une fonction de `Core`,
  `core::analyzeCombatZones` : cases libres et pleines, entrées d'arène dedans et dehors, et le
  défaut qui l'empêche d'être jouée. `core::validateCombatZones` la reprend, sans changer ce qu'il
  signale au chargement. L'éditeur l'affiche sur la zone sélectionnée et dans l'inspecteur,
  recalculé **sur l'aperçu** pendant le glisser ; une entrée d'arène hors de **toute** zone avertit
  — `core::cropLevelToZone` l'écarterait sans bruit.
- **Le schéma typé.** Un entier a ses bornes (`minimum`, `maximum`) : largeur et hauteur d'au moins
  1, rang d'arène d'au moins 1. Cinq catalogues s'ajoutent : **figurines** (les slugs de l'atelier
  des PNJ, `Monsters/<slug>` de celui des monstres), **drapeaux** (ceux qu'un dialogue pose, et le
  drapeau d'une quête démarrée), **lieux de l'atlas** (`World/locations`), **objets**
  (`Rpg/items`) et **`carte#id`** (décision D8 ; la carte éditée se lit dans le brouillon). La
  figurine d'un PNJ, son quartier gardé et le drapeau d'un portail deviennent des choix. Les objets
  et `carte#id` n'ont pas encore de propriété qui les cite : le `LOT-26` et le `LOT-16` les
  trouveront prêts.
- **Le contrat d'extension a son test bloquant** : il relève dans les sources du jeu (`Core`,
  `HMI`, `App`) chaque constante `*_ENTITY_TYPE`, les familles interactives et les types des cartes
  livrées, et exige chacune dans la table. Il a trouvé `zone`.
- **Les entités restent peintes** dans l'élément unique du canevas, avec les autres aides
  d'édition, plutôt qu'en `QGraphicsItem` : le pointage est pur (`hmi::pickEntity`), la même
  peinture sert les deux vues, et la décision D2 n'imposait les éléments qu'aux besoins.

## Vérification

- **Redimensionner la zone de combat du Colisée à la souris met à jour son verdict tactique.** ✔
  `EntitesAFormeTest.AcceptationRedimensionnerLaZoneDuColisee` : la zone « sable » (20 × 14, huit
  entrées dedans) sélectionnée, l'appui sur sa poignée est la prend ; tirée de la colonne 29 à la
  colonne 20, elle s'écrit en un pas (11 × 14) ; son verdict compte alors quatre entrées dehors et
  quatre avertissements ; `Ctrl+Z` la rend.
- **Aucune famille n'a de code propre dans le canevas.** ✔ `EditorViewport` et `EntityPanel` ne
  nomment aucun type d'entité ; formes, poignées, étiquettes et figurines viennent de la table
  (`EntitesAFormeTest.LaFormeVientDeLaTable`, `FamillesDEntitesTest.LaTableEstCoherenteAvecSesFormes`).
  Le verdict de zone est une donnée de `Core`, rangée par entité.
- **Un test bloque toute famille lue par le jeu et absente de `EntityKinds`.** ✔
  `FamillesDEntitesTest.ToutFamilleLueParLeJeuEstDansLaTable`.

**Vérification à la souris, due** : sur le Colisée, outil Entité (`O`), cliquer dans le sable puis
tirer la poignée est de la zone vers l'ouest (le verdict passe au rouge sur les entrées ennemies),
`Ctrl+Z` ; `Maj` + clic sur deux entrées d'arène puis les glisser ensemble ; choisir « zone » dans
la liste des familles, tirer un rectangle, puis `Z` et peindre quelques cases, `Ctrl` + glisser
pour en gommer ; choisir « route », tirer un segment, puis `Z` et cliquer pour le prolonger ;
taper « portal » dans le filtre de la liste. Sur Martpart, les étiquettes des îlots et du portail
paraissent, et les PNJ se voient par leur figurine sans marqueur par-dessus. Les clics postés à la
fenêtre n'atteignent pas Qt (`LOT-EDITOR-04`) : cette vérification demande la main de l'auteur.

## Bilan

**Livré le 19 septembre 2026** (ouvert le même jour), sur la branche
`lot-editor-05-entites-zones`. Vérification automatisée : construction `/W4 /WX` sans
avertissement, tests unitaires verts (838, dont deux ignorés comme avant), acceptation sur le
Colisée. **Vérification à la souris due** (voir « Vérification »).

Exigences : `EX-EDIT-070` à `EX-EDIT-073` (nouvelles) ; `EX-EDIT-050` révisée.
