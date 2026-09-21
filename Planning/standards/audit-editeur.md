# Audit — l'éditeur face au planning : ce qu'il sait, ce qui lui manque

Relevé du 21 septembre 2026, sur `main` à `c892177ac`. Le planning du 20 septembre a détaillé les
fonctions du **jeu**, version par version ; il a laissé l'éditeur « avancer à part ». Or la
définition de « livré » d'une carte est *dessinée dans l'éditeur, `LevelEditor --check` passe* :
chaque lot de la filière **cartes** dépend d'un outil que personne n'a confronté à la 2D HD, à la
nouvelle arborescence, ni aux mécanismes que la quête de la démo introduit. Cet audit le fait, et
les lots qui en sortent sont dans la filière **`editeur`** du planning.

Chaque constat a été lu dans le code ; ce qui est seulement supposé est dit.

## 1. Ce que l'éditeur sait faire

Treize lots sur quatorze sont livrés (`Documentation/Editeur/feuille-de-route.md`). En bref :

| Domaine | État |
|---|---|
| **Peindre** | onze outils (pinceau, rectangle, ligne, seau, gomme, pipette, sélection, entité, forme, mesure, note), miroir, un geste = un pas d'annulation (200 pas) ; palette de la planche du lieu, pièces larges à emprise, collision déduite et cases forcées |
| **Entités et zones** | onze familles tirées de `core::EntityKinds` (`chest`, `sign`, `npc`, `encounter`, `portal`, `spawnPoint`, `combatZone`, `cityBlock`, `zone`, `route`, `arenaEntry`) ; inspecteur sans code par famille ; zones peintes, trajets, multi-sélection, verdict tactique d'une zone de combat |
| **Format** | v1 à v4 lues, v4 écrite de façon canonique, schéma publié, identifiants d'entité jamais réemployés, variantes par `base`, réserve de hauteur (`floor`, `elevation`) |
| **Contrôle** | `--check` en CI : format, pièces et alias, collision = déduction + forcées, références (dialogues, figurines, rencontres, cartes, arrivées, drapeaux), terrain des rencontres, atteignabilité, portails sans retour, clés de traduction des noms de carte |
| **Sans fenêtre** | `--check`, `--migrate`, `--apply`, `--render`, `--rename-map`, `--rename-arrival`, `--rename-id`, `--replace-piece`, `--change-scene`, `--who-cites`, `--link-maps`, `--list-prefabs`, `--save-prefab` |
| **Monde** | onglets à plusieurs cartes, graphe des cartes (un lien pose portail et arrivée des deux côtés), vue de ville en lecture seule, région et ambiance d'une carte, état d'avancement |
| **Essai** | immédiat dans le canevas (marche, portails) ; complet dans le vrai jeu, sur les brouillons, à la case et sous les drapeaux voulus |
| **Atelier** | tampons, préfabriqués, trois modèles de carte, sauvegarde automatique et reprise, garde de fichier modifié sur disque |

Le socle est sain : aucun `TODO` dans `Source/Editor`, une logique en fonctions pures, et le contrat
d'extension par `EntityKinds` tient — un lot du jeu qui déclare une famille ou une propriété la
reçoit dans l'inspecteur et le contrôle **sans code d'éditeur**.

## 2. Ce que le planning casse

### 2.1 La table rase (LOT-102)

Le LOT-102 écrit « l'éditeur s'ouvre sur une carte vierge ; `LevelEditor --check` passe ». Aujourd'hui
c'est **faux** :

| # | Constat | Preuve |
|---|---|---|
| T1 | `--check` rend 1 quand il n'y a **aucune carte** (« error: no map under … ») ; l'étape CI sera rouge | `Source/Editor/Logic/MapFormat.cpp:671-674`, `ci.yml:356-363` |
| T2 | `resolveDataRoot` exige que `Levels/` existe ; git ne garde pas un dossier vide | `DataRoot.cpp:30` |
| T3 | **Dix-huit** fichiers de `Source/Test/Unit/Editor/` lisent Martpart, le Colisée ou Arenarea par `JADG_LEVELS_DIR` / `JADG_ASSETS_DIR` ; l'audit du moteur ne les compte pas | `test_canvas_picking`, `test_canvas_scene`, `test_entity_shapes`, `test_map_format`, `test_scene_painter`, `test_shipped_maps`, `test_gesture_script`, `test_stamps`, `test_map_render`, `test_map_refactor`, et trois tests système ou d'intégration |
| T4 | Le scénario `--apply` de référence (`martpart-rue.json`) vise la carte et la planche livrées, comparées octet pour octet | `Source/Test/Fixtures/Gestures/` |
| T5 | `bench_canvas` charge Martpart en dur et finit en `SkipWithError` : pas une série perdue, un fichier à réécrire | `Source/Benchmark/bench_canvas.cpp:28-40` |
| T6 | La liste des lieux de « New map » sera vide : `scenePlaces()` ne lit que le premier niveau de `Assets/Scene` | `MapFormat.cpp:407-419` |

### 2.2 La nouvelle arborescence (du commun vers le propre)

Le standard veut qu'une carte d'Arenarea puise dans le kit d'Arenarea, **puis** celui de la Capitale,
de l'Empire, du monde. L'éditeur ne connaît qu'**un** lieu et **un** manifeste :

| # | Constat | Preuve |
|---|---|---|
| A1 | Un brouillon porte un seul manifeste ; `changeScene(place, manifest)` n'en prend qu'un | `Core/Levels/LevelDraft.h:207,283,580` |
| A2 | La palette, le catalogue de pièces, la déduction de collision et `--check` cherchent chaque pièce dans ce seul manifeste : une carte **ne peut pas** citer des pièces de deux niveaux | `PalettePanel.cpp:178-183`, `PieceCatalog.h:73`, `MapFormat.cpp:160-171` |
| A3 | Le fichier d'une pièce se déduit de `"Scene/" + lieu + "/" + nom + ".png"` ; le champ `file` du manifeste est ignoré, `"tile"` n'est pas lu | `WorldSceneComposer.cpp:26,42-49`, `ScenePieceManifest.cpp` |
| A4 | Chemins en dur : vignettes, `--change-scene`, lecture des ancres, figurines sous `Npc/` et `Monsters/` | `EditorViewport.cpp:979`, `MapRefactor.cpp:376,911`, `SceneImages.cpp:119` |
| A5 | Les préfabriqués sont rangés par lieu ; un préfabriqué fait du kit de la Capitale devrait servir dans tous ses quartiers — aucun repli n'existe | `Stamps.cpp:628,814` |

Le LOT-102 (point 4) rebranche `MapFormat` et `EntityReferences` ; le LOT-104 promettait « la palette
qui propose les pièces du lieu et de ses niveaux communs ». Ni l'un ni l'autre ne cite `LevelDraft`,
`CollisionDerivation`, `PieceCatalog`, `MapRefactor` (l'outil de la **promotion** d'un asset),
`Stamps` ni `SceneImages`. C'est un lot à part entière : le LOT-124.

### 2.3 Le rendu HD

L'éditeur n'a presque aucune constante d'art en propre : il hérite de `SceneComposition`, que le
LOT-103 corrige. Reste ce qui lui est propre :

| # | Constat | Preuve |
|---|---|---|
| H1 | `SmoothPixmapTransform` **désactivé** au canevas, à la vue et à `--render` : une pièce de 256 px réduite à 0,34 crénelle | `ScenePainter.cpp:93`, `EditorViewport.cpp:253`, `MapRender.cpp:171` |
| H2 | `ARENA_SHEET_TILE_WIDTH_PIXELS = 86`, que l'audit du moteur ne cite pas, fixe l'échelle du canevas et de `--render` | `Core/Combat/IsoProjection.h:55-64` |
| H3 | L'essai immédiat recopie le zoom entier du jeu (`max(1, h / 720)`) | `EditorViewport.cpp:1238-1241` |
| H4 | La marge du cadre vaut **un** losange : un relief HD haut est rogné en `--render`, en vignette et au cadrage (supposé, selon la hauteur des pièces) | `MapRender.cpp:160`, `EditorViewport.cpp:317,326` |
| H5 | `--render` : échelle 1 = 86 px par case, plafond 4 ; la CI rend à 0,5. Une carte HD native de 48 × 40 ferait 11 300 × 7 150 px | `MapRender.cpp:35`, `ci.yml:378` |
| H6 | `SceneImages` garde toute image sans éviction, relit `manifest.json` **à chaque image**, et se recrée à chaque vignette : avec seize fois plus de pixels par pièce, un kit pèse 100 à 200 Mio (estimation) | `SceneImages.cpp:109-127`, `MapRender.cpp:153` |
| H7 | Seule la **composition** est mesurée ; la **peinture** `QPainter`, qui est ce que la HD change, ne l'est pas | `bench_canvas.cpp` |
| H8 | La parité GPU / `QPainter` ne peut plus être exacte : `QPainter` n'a pas de mipmaps | `test_scene_painter.cpp` |

Ce qui ne bouge **pas** : le pointage (en unités du monde, seul le rapport 0,62 compte), le zoom du
canevas (déjà libre), la mini-carte, les vignettes de pièces (déjà lissées).

## 3. Ce que le jeu demandera aux cartes, version par version

| Version | Ce que le jeu introduit | Ce que l'éditeur en sait | Suite |
|---|---|---|---|
| `0.0.1` | PNJ **présents selon un drapeau** (LOT-116) | `portal.requiresFlag` seulement ; le catalogue des drapeaux ne lit que les dialogues, et ignore les **valeurs** | LOT-126 |
| `0.0.1` | « les portes de l'arène sont closes » : un **décor** qui change avec un drapeau | une pièce vit dans une couche, sans condition | LOT-126 |
| `0.0.1` | l'escalier des catacombes, **posé et condamné** (LOT-107) | un portail sans arrivée est une erreur du contrôle | LOT-126 |
| `0.0.1` | déclencheurs de quête, transfert scénarisé vers le vestiaire A | `zone` n'a pas de propriété de déclenchement | LOT-126 |
| `0.0.1` | un dialogue **déclenche** une rencontre (LOT-118) | rien à faire : l'entité `encounter` et la zone de combat existent | — |
| `0.0.1` | le niveau −1 de l'arène **sous** les gradins (LOT-107) | pas d'étages, et c'est voulu : **un niveau est une carte** (décision D-21) — le LOT-107 livre deux cartes reliées | — |
| `0.0.2` | combat à **quatre contre plusieurs**, budget de difficulté (LOT-139) | le verdict d'une zone de combat ne compte ni le groupe ni le budget | LOT-143 |
| `0.0.3` | six quartiers peuplés aux **proportions du livre** ; des dizaines de PNJ par carte | une entité à la fois : ni édition à plusieurs, ni lasso, ni foule | LOT-158 |
| `0.0.3` | une **image d'onglet « Carte »** et une entrée de `world-maps.json` par zone ; sous-zones | cadres de quartier non éditables, région non contrôlée contre l'atlas, `locations` sans distinction quartier / ville | LOT-159 |
| `0.0.3` | promotion d'assets au commun (LOT-151) | `--replace-piece` existe ; il doit suivre les niveaux | LOT-124 |
| `0.0.3` | sauvegarde (LOT-150) | couvert : l'état se range par `id` d'entité | — |
| `0.0.5` | cartes de **plein air** : prairie, route, forêt | borne de taille absente du code (100 dans deux boîtes de dialogue, contournable), composition non bornée au cadrage, aucun outil de terrain | LOT-167, LOT-168 |
| `0.0.5` | horloge jour / nuit (LOT-171) | `route` attend ses horaires | LOT-169 |
| `0.2.0` | marchands, butin, campement, audio, factions | **couvert par le contrat d'extension** : la source `Items` est déjà chargée, l'ambiance de carte existe, une zone porte des propriétés typées | — |
| `0.2.0` | le quartier général, « une carte qui change avec les points posés » (LOT-206) | le décor conditionné du LOT-126 | — |
| `0.17.0` | le plan pénombral : la même carte sous une autre planche | les variantes existent au format et au contrôle, **pas à la souris** | à planifier avec la version |

Ce que l'éditeur ne fera **pas**, et c'est voulu : écrire les dialogues et les quêtes (des données,
contrôlées par `--check` aux LOT-116 et LOT-117), dessiner les planches, régler les rencontres.

### Le mode sans texture

Demandé par l'auteur le 21 septembre : maquetter une carte par sa physique, la jouer, l'habiller ensuite.
L'éditeur sait déjà la **dessiner** (lieu « none », palette des types, collision déduite du type, `--check`
vert) et la **convertir** (`Change sheet…`). Mais le jeu et les deux essais montrent un **écran uniforme** —
`WorldPlay` charge une apparence vide et le composeur n'a aucun repli par type —, un mur est un losange plat,
un PNJ sans figurine est invisible, et `--render` ne dessine aucune entité. `EX-EXP-005` n'est pas tenue.
C'est le [LOT-128](../versions/v0.1.0/v0.0.1-demo/lots/LOT-128-cartes-maquettes.md).

## 4. Les dettes de l'atelier

Relevées dans les dossiers des lots livrés, sous leurs rubriques « pas fait » :

- **La vérification à la main est due sur huit lots** (03 à 10, 13, 14) : les clics postés
  n'atteignent pas Qt. La [définition de « livré »](definition-de-livre.md) en fait désormais un
  critère d'acceptation — LOT-127.
- Deux éditeurs ouverts se proposent les brouillons l'un de l'autre ; les onglets ne sont pas
  rouverts au lancement ; le panneau « Problems » lit les fichiers, pas le brouillon ; un préfabriqué
  ne se renomme ni ne se supprime depuis la fenêtre ; les touches d'outils sont en dur ; pas
  d'insertion d'un point au milieu d'un trajet ; `--apply` ne rejoue ni les couches ni le
  redimensionnement — LOT-166.
- Le jeu ne lit que la **première** couche de décor ; la gêne et l'abri déduits d'une pièce ne sont
  pas joués : ce sont des limites du **jeu**, à lever quand une carte les demande.

## 5. LOT-EDITOR-11 est-il toujours d'actualité ?

**Non, tel qu'écrit.** Il pilotait « le générateur » du `LOT-40` — or le `LOT-40` est **écarté**
(les cartes sont dessinées, zone par zone), aucun générateur de terrain n'existe dans `Core`, et
ses trois promesses (graine et lieu, régions verrouillées, provenance) n'ont de sens qu'avec lui.
Il est **abandonné** sous ce nom.

Le besoin qui reste est plus modeste et bien réel : une forêt de Bak de cent cases de côté ne se
plante pas arbre par arbre. Le [LOT-168](../versions/v0.1.0/v0.0.5-abords-de-la-capitale/lots/LOT-168-semis-assiste.md)
le reprend comme un **outil de dessin** — un semis à graine sur une sélection, rejouable par
`--apply` —, pas comme un générateur de cartes.

## 6. Les lots qui en sortent

| Lot | Version | Objet | Débloque |
|---|---|---|---|
| [LOT-128](../versions/v0.1.0/v0.0.1-demo/lots/LOT-128-cartes-maquettes.md) | `0.0.1` | Les cartes maquettes : dessiner et **jouer** sans texture, jetons vert / jaune / rouge | les trois cartes, LOT-118 |
| [LOT-123](../versions/v0.1.0/v0.0.1-demo/lots/LOT-123-editeur-sur-une-base-vide.md) | `0.0.1` | L'éditeur debout sur une base vide | LOT-102 |
| [LOT-124](../versions/v0.1.0/v0.0.1-demo/lots/LOT-124-editeur-et-arborescence-par-niveaux.md) | `0.0.1` | Une carte puise dans son lieu **et** ses niveaux communs | les trois cartes |
| [LOT-125](../versions/v0.1.0/v0.0.1-demo/lots/LOT-125-canevas-hd.md) | `0.0.1` | Le canevas, les vignettes et `--render` en HD | les trois cartes |
| [LOT-126](../versions/v0.1.0/v0.0.1-demo/lots/LOT-126-ce-que-la-quete-demande-aux-cartes.md) | `0.0.1` | Présence conditionnée, décor conditionné, portail condamné, déclencheurs | LOT-107, LOT-114, LOT-120 |
| [LOT-127](../versions/v0.1.0/v0.0.1-demo/lots/LOT-127-recette-de-l-editeur-a-la-main.md) | `0.0.1` | La recette à la main, due depuis huit lots | LOT-107 |
| [LOT-143](../versions/v0.1.0/v0.0.2-combat/lots/LOT-143-zones-de-combat-pour-un-groupe.md) | `0.0.2` | Le verdict d'une zone de combat pour un groupe | LOT-142 |
| [LOT-158](../versions/v0.1.0/v0.0.3-capitale-intra-muros/lots/LOT-158-peupler-une-zone.md) | `0.0.3` | Peupler une zone : foule, édition à plusieurs, lasso | les zones |
| [LOT-159](../versions/v0.1.0/v0.0.3-capitale-intra-muros/lots/LOT-159-onglet-carte-depuis-l-editeur.md) | `0.0.3` | L'onglet « Carte » depuis l'éditeur | LOT-156 |
| [LOT-166](../versions/v0.1.0/v0.0.4-faubourgs/lots/LOT-166-dettes-de-l-atelier.md) | `0.0.4` | Les dettes de l'atelier | LOT-165 |
| [LOT-167](../versions/v0.1.0/v0.0.5-abords-de-la-capitale/lots/LOT-167-grandes-cartes-de-plein-air.md) | `0.0.5` | Les grandes cartes de plein air | LOT-173 |
| [LOT-168](../versions/v0.1.0/v0.0.5-abords-de-la-capitale/lots/LOT-168-semis-assiste.md) | `0.0.5` | Le semis assisté (reprend `LOT-EDITOR-11`) | la forêt de Bak |
| [LOT-169](../versions/v0.1.0/v0.0.5-abords-de-la-capitale/lots/LOT-169-horaires-et-trajets.md) | `0.0.5` | Horaires et trajets | LOT-174 |

Après la `0.0.5`, l'éditeur est **complet** pour l'Empire central : les versions `0.0.6` à `0.1.0`
et la `0.2.0` ne lui demandent que le contrat d'extension. Le prochain rendez-vous est le plan
pénombral (`0.17.0`), pour les variantes à la souris.
