# Core · Levels

Tests unitaires — **117 cas** (24 critiques, 76 majeurs, 17 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_format_v4.cpp`](#test-format-v4cpp) | 19 | - | 8 | 7 | 4 |
| [`test_layer_pieces.cpp`](#test-layer-piecescpp) | 9 | - | 3 | 6 | - |
| [`test_level.cpp`](#test-levelcpp) | 6 | - | - | 6 | - |
| [`test_level_draft.cpp`](#test-level-draftcpp) | 25 | - | 1 | 21 | 3 |
| [`test_level_draft_editing.cpp`](#test-level-draft-editingcpp) | 14 | - | 1 | 7 | 6 |
| [`test_level_draft_pieces.cpp`](#test-level-draft-piecescpp) | 14 | - | 6 | 7 | 1 |
| [`test_level_loader.cpp`](#test-level-loadercpp) | 5 | - | 1 | 4 | - |
| [`test_level_writer.cpp`](#test-level-writercpp) | 4 | - | - | 3 | 1 |
| [`test_map_layers.cpp`](#test-map-layerscpp) | 14 | - | - | 12 | 2 |
| [`test_rpg_terrain.cpp`](#test-rpg-terraincpp) | 4 | - | 2 | 2 | - |
| [`test_tile_type_name.cpp`](#test-tile-type-namecpp) | 3 | - | 2 | 1 | - |

## test_format_v4.cpp

### FormatV4Test.UneCarteDeChaqueVersionSeCharge

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:107`

Une carte de chaque version se charge.

**Étapes**

1. Charger `format-v0.json` à `format-v4.json`.

**Résultat attendu**

- Vérifie que `level.entry()` vaut `(GridPosition{0, 0})`.

### FormatV4Test.UneV4CanoniqueRessortOctetPourOctet

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:125`

Une v4 canonique ressort octet pour octet.

**Étapes**

1. Charger `format-v4.json`.
2. L'écrire.

**Résultat attendu**

- Vérifie que `core::LevelWriter::toJsonString(chargerFichier(path))` vaut `lire(path)`.

### FormatV4Test.UneV3RessortEnV4EtSeStabilise

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:140`

Une v3 ressort en v4, et se stabilise.

**Étapes**

1. Charger `format-v3.json` et l'écrire.
2. Relire et réécrire.

**Résultat attendu**

- Vérifie que `premiere.find("\"version\": 4")` diffère de `std::string::npos`.
- Vérifie que `premiere.find(R"({"x": 3, "y": 0, "type": "wall", "piece": "wall-left"})")` diffère de `std::string::npos`.
- Vérifie que `premiere.find("texture")` vaut `std::string::npos`.
- Vérifie que `core::LevelWriter::toJsonString(charger(premiere))` vaut `premiere`.

### FormatV4Test.LaReserveDeHauteurSurvitALAllerRetour

*Majeur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:161`

La réserve de hauteur survit à l'aller-retour.

**Étapes**

1. Charger `format-v4.json`.

**Résultat attendu**

- Vérifie que `level.layers()[2].floor` vaut `1`.
- Vérifie que `level.layers()[1].elevationAt(2, 1)` vaut `1`.
- Vérifie que `level.entities().front().elevation` vaut `2`.

### FormatV4Test.DeuxEntitesDuMemeIdSontRefusees

*Majeur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:180`

Deux entités du même id sont refusées.

**Étapes**

1. Charger une carte dont deux entités portent `e1`.

**Résultat attendu**

- Vérifie que `loaded.errorCode` vaut `core::LevelValidationError::DuplicateEntityId`.

### FormatV4Test.UnIdentifiantDonneNEstJamaisRedonne

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:200`

Un identifiant donné n'est jamais redonné.

**Étapes**

1. Poser un coffre dans `format-v4.json` (compteur à 4).
2. Annuler, poser un autre coffre.

**Résultat attendu**

- Vérifie que `premier.has_value()` est vrai.
- Vérifie que `draft.entities()[*premier].id` vaut `"e4"`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `second.has_value()` est vrai.
- Vérifie que `draft.entities()[*second].id` vaut `"e5"`.
- Vérifie que `draft.toJson().find("\"nextEntityId\": 6")` diffère de `std::string::npos`.

### FormatV4Test.UnIdENSeLitEtSEcrit

*Mineur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:226`

Un id e&lt;n&gt; se lit et s'écrit.

**Étapes**

1. Écrire et relire `e12`, puis lire `herald` et `e`.

**Résultat attendu**

- Vérifie que `core::entityIdFor(12)` vaut `"e12"`.
- Vérifie que `core::entityIdNumber("e12")` vaut `12`.
- Vérifie que `core::entityIdNumber("herald").has_value()` est faux.
- Vérifie que `core::entityIdNumber("e").has_value()` est faux.
- Vérifie que `core::entityIdNumber("e1x").has_value()` est faux.

### FormatV4Test.LesCasesForceesSeRangentSansDoublon

*Majeur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:243`

Les cases forcées se rangent sans doublon.

**Étapes**

1. Charger des cases forcées en désordre, dont un doublon.

**Résultat attendu**

- Vérifie que `level.forcedCollision()` vaut `(std::vector<GridPosition>{{1, 0}, {0, 1}})`.

### FormatV4Test.UneCaseForceeHorsCarteEstRefusee

*Mineur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:261`

Une case forcée hors carte est refusée.

**Étapes**

1. Charger une case forcée en (5, 5) sur une carte 2 × 2.

**Résultat attendu**

- Vérifie que `loaded.errorCode` vaut `core::LevelValidationError::OutOfBounds`.

### FormatV4Test.UneVarianteReprendLesCasesDeSaBase

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:282`

Une variante reprend les cases de sa base.

**Étapes**

1. Charger `variante/quartier/nuit.json`, variante de `base`.
2. La réécrire.

**Résultat attendu**

- Vérifie que `variante.base()` vaut `"base"`.
- Vérifie que `variante.tileMap().width()` vaut `3`.
- Vérifie que `variante.tileMap().tile(2, 1)` vaut `TileType::Wall`.
- Vérifie que `variante.entry()` vaut `base.entry()`.
- Vérifie que `variante.entities().size()` vaut `1U`.
- Vérifie que `variante.entities().front().type` vaut `"spawnPoint"`.
- Vérifie que `scene` diffère de `variante.layers()[1].properties.end()`.
- Vérifie que `std::get<std::string>(scene->second)` vaut `"coliseum-doom"`.
- Vérifie que `variante.layers()[1].pieceAt(1, 0)` vaut `"sand-2"`.
- Vérifie que `core::LevelWriter::toJsonString(variante)` vaut `lire(path)`.

### FormatV4Test.UneVarianteSansBaseEstRefusee

*Majeur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:311`

Une variante sans base est refusée.

**Étapes**

1. Charger la variante depuis une chaîne, sans résolveur.
2. Déclarer une base introuvable depuis un fichier.

**Résultat attendu**

- Vérifie que `core::LevelLoader::loadFromString(texte).errorCode` vaut `core::LevelValidationError::MissingBase`.
- Vérifie que `sansBase.errorCode` vaut `core::LevelValidationError::MissingBase`.

### FormatV4Test.UneVarianteQuiPorteDesCasesEstRefusee

*Mineur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:332`

Une variante qui porte des cases est refusée.

**Étapes**

1. Charger une variante qui déclare `tiles`.

**Résultat attendu**

- Vérifie que `loaded.errorCode` vaut `core::LevelValidationError::ParseError`.

### FormatV4Test.UneZonePeinteEstLueParLaGrilleTactique

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:353`

Une zone peinte est lue par la grille tactique.

**Étapes**

1. Charger `format-v4.json` (zone peinte en (1, 1) et (2, 2)).
2. Construire sa `BattleGrid`.

**Résultat attendu**

- Vérifie que `porteLaZone({1, 1})` est vrai.
- Vérifie que `porteLaZone({2, 2})` est vrai.
- Vérifie que `porteLaZone({2, 1})` est faux.
- Vérifie que `grid.isDifficult({2, 2})` est vrai.
- Vérifie que `grid.isDifficult({2, 1})` est faux.

### FormatV4Test.UneZoneRectangleCouvreSonRectangle

*Majeur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:379`

Une zone rectangle couvre son rectangle.

**Étapes**

1. Demander les cases d'une zone 2 × 1 en (1, 0).

**Résultat attendu**

- Vérifie que `core::zoneCells(zone)` vaut `(std::vector<GridPosition>{{1, 0}, {2, 0}})`.

### FormatV4Test.UneEmpriseSEtendDepuisSonAncre

*Majeur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:400`

Une emprise s'étend depuis son ancre.

**Étapes**

1. Demander les cases et le pied d'une emprise 2 × 1 ancrée en (3, 4).

**Résultat attendu**

- Vérifie que `core::footprintCells({3, 4}, etal)` vaut `(std::vector<GridPosition>{{3, 4}, {4, 4}})`.
- Vérifie que `core::footprintFootCorner({3, 4}, etal)` vaut `(GridPosition{5, 5})`.
- Vérifie que `core::footprintFootCorner({3, 4}, {})` vaut `(GridPosition{4, 5})`.

### FormatV4Test.LeManifesteDitCeQuUnePieceOppose

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:419`

Le manifeste dit ce qu'une pièce oppose.

**Étapes**

1. Lire le manifeste d'essai.

**Résultat attendu**

- Vérifie que `pieces.find("street")->tactical` vaut `PieceTactical::Open`.
- Vérifie que `pieces.find("wall-left")->tactical` vaut `PieceTactical::Solid`.
- Vérifie que `pieces.find("bench")->tactical` vaut `PieceTactical::Open`.
- Vérifie que `pieces.find("stall")->tactical` vaut `PieceTactical::Obstacle`.
- Vérifie que `pieces.find("odd")->tactical` vaut `PieceTactical::Solid`.
- Vérifie que `pieces.find("low-wall")` diffère de `nullptr`.
- Vérifie que `pieces.find("low-wall")->name` vaut `"parapet"`.

### FormatV4Test.LaCollisionSeDeduitDesPieces

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:442`

La collision se déduit des pièces.

**Étapes**

1. Poser un étal 2 × 1 en (0, 0), un mur en (3, 0), un banc en (2, 0).
2. Déduire.

**Résultat attendu**

- Vérifie que `derivee.collision.tile(0, 0)` vaut `TileType::Cliff`.
- Vérifie que `derivee.collision.tile(1, 0)` vaut `TileType::Cliff`.
- Vérifie que `derivee.collision.tile(2, 0)` vaut `TileType::Empty`.
- Vérifie que `derivee.collision.tile(3, 0)` vaut `TileType::Wall`.
- Vérifie que `derivee.collision.tile(column, 1)` vaut `TileType::Wall`.
- Vérifie que `derivee.unknownPieces.empty()` est vrai.
- Vérifie que `derivee.unplayed.empty()` est vrai.

### FormatV4Test.TypesPiecesInconnuesGeneEtAbri

*Majeur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:475`

Types, pièces inconnues, gêne et abri.

**Étapes**

1. Poser de l'eau profonde sans pièce, une pièce inconnue sur un mur, une flaque, un muret par son ancien nom.
2. Déduire.

**Résultat attendu**

- Vérifie que `derivee.collision.tile(0, 1)` vaut `TileType::Cliff`.
- Vérifie que `derivee.collision.tile(1, 1)` vaut `TileType::Wall`.
- Vérifie que `derivee.collision.tile(2, 1)` vaut `TileType::Empty`.
- Vérifie que `derivee.collision.tile(3, 1)` vaut `TileType::Empty`.
- Vérifie que `derivee.unknownPieces` vaut `(std::vector<GridPosition>{{1, 1}})`.
- Vérifie que `derivee.unplayed` vaut `(std::vector<GridPosition>{{2, 1}, {3, 1}})`.

### FormatV4Test.LEntreeSAccordeAvecUneCaseVide

*Mineur · Unitaire · Format v4* — `Source/Test/Unit/Core/Levels/test_format_v4.cpp:505`

L'entrée s'accorde avec une case vide.

**Étapes**

1. Comparer une entrée à du vide, un mur à du vide.

**Résultat attendu**

- Vérifie que `core::collisionAgrees(ecrite, deduite, 0, 0)` est vrai.
- Vérifie que `core::collisionAgrees(ecrite, deduite, 1, 0)` est faux.

## test_layer_pieces.cpp

### LayerPiecesTest.UneCaseDeCouchePorteSonTypeEtSaPiece

*Critique · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:51`

Une case de couche porte son type et sa pièce.

**Étapes**

1. Charger une carte v4 dont le décor nomme une pièce.
2. Lire la case.

**Résultat attendu**

- Vérifie que `level.layers().size()` vaut `3U`.
- Vérifie que `level.layers()[1].pieceAt(0, 0)` vaut `"street"`.
- Vérifie que `level.layers()[DECOR].tiles.tile(1, 1)` vaut `TileType::Wall`.
- Vérifie que `level.layers()[DECOR].pieceAt(1, 1)` vaut `"wall-red"`.
- Vérifie que `level.layers()[DECOR].pieceAt(2, 2).empty()` est vrai.

### LayerPiecesTest.LAssignationDUneCarteV3DevientUnePieceDuDecor

*Critique · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:71`

L'assignation d'une carte v3 devient une pièce du décor.

**Étapes**

1. Charger une carte v3 dont la grille racine porte une texture.
2. Lire la couche de décor.

**Résultat attendu**

- Vérifie que `level.layers()[DECOR].pieceAt(2, 1)` vaut `"wall-left"`.
- Vérifie que `level.tileMap().tile(2, 1)` vaut `TileType::Wall`.

### LayerPiecesTest.UneV3SansDecorRecoitUneCoucheDeRelief

*Majeur · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:95`

Une v3 sans décor reçoit une couche de relief.

**Étapes**

1. Charger une carte v2 plate dont une tuile porte une texture.
2. Lire ses couches.

**Résultat attendu**

- Vérifie que `level.layers().size()` vaut `2U`.
- Vérifie que `level.layers()[1].kind` vaut `LayerKind::Decor`.
- Vérifie que `level.layers()[1].name` vaut `"relief"`.
- Vérifie que `level.layers()[1].pieceAt(2, 2)` vaut `"crate"`.

### LayerPiecesTest.UneV4QuiPorteUneTextureRacineEstRefusee

*Majeur · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:118`

Une v4 qui porte une texture racine est refusée.

**Étapes**

1. Charger une v4 dont une tuile racine porte `texture`.

**Résultat attendu**

- Vérifie que `loaded.ok()` est faux.
- Vérifie que `loaded.errorCode` vaut `core::LevelValidationError::ParseError`.

### LayerPiecesTest.UnAutreTypeRetireLaPieceLeMemeLaGarde

*Majeur · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:138`

Un autre type retire la pièce, le même la garde.

**Étapes**

1. Repeindre `wall` sur la case habillée.
2. Peindre `dirt` dessus.

**Résultat attendu**

- Vérifie que `draft.paintLayerTile(DECOR, 1, 1, TileType::Wall)` est faux.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 1)` vaut `"wall-red"`.
- Vérifie que `draft.paintLayerTile(DECOR, 1, 1, TileType::Dirt)` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 1).empty()` est vrai.

### LayerPiecesTest.UneRegionRepeinteRetireLaPieceLAnnulationLaRend

*Majeur · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:158`

Une région repeinte retire la pièce ; l'annulation la rend.

**Étapes**

1. Peindre un bloc de sable sur la case habillée.
2. Annuler.

**Résultat attendu**

- Vérifie que `draft.paintLayerRegion(DECOR, 1, 1, {{TileType::Sand, TileType::Sand}})` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 1).empty()` est vrai.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 1)` vaut `"wall-red"`.

### LayerPiecesTest.ReduireLaGrilleTronqueLesPiecesHorsBornes

*Majeur · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:177`

Réduire la grille tronque les pièces hors bornes.

**Étapes**

1. Réduire la carte à 2 × 2, puis à 1 × 1.

**Résultat attendu**

- Vérifie que `draft.layers()[DECOR].pieceAt(1, 1)` vaut `"wall-red"`.
- Vérifie que `draft.layers()[DECOR].hasPieces()` est faux.

### LayerPiecesTest.PiecesEtHauteursSurviventALAllerRetour

*Critique · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:196`

Pièces et hauteurs survivent à l'aller-retour.

**Étapes**

1. Donner une hauteur à une case habillée.
2. Écrire puis relire.

**Résultat attendu**

- Vérifie que `json.find("\"texture\"")` vaut `std::string::npos`.
- Vérifie que `relue.layers()[DECOR].pieceAt(1, 1)` vaut `"wall-red"`.
- Vérifie que `relue.layers()[DECOR].elevationAt(1, 1)` vaut `2`.
- Vérifie que `relue.layers()[1].pieceAt(0, 0)` vaut `"street"`.

### LayerPiecesTest.UneCaseVideQuiNommeUnePieceSEcrit

*Majeur · Unitaire · Pièces de couche* — `Source/Test/Unit/Core/Levels/test_layer_pieces.cpp:219`

Une case vide qui nomme une pièce s'écrit.

**Étapes**

1. Nommer une pièce sur une case sans type.
2. Écrire puis relire.

**Résultat attendu**

- Vérifie que `relue.layers()[DECOR].pieceAt(3, 3)` vaut `"banner"`.
- Vérifie que `relue.layers()[DECOR].tiles.tile(3, 3)` vaut `TileType::Empty`.

## test_level.cpp

### TileMapTest.GrilleNeuveVideAuxBonnesDimensions

*Majeur · Unitaire · Tile Map* — `Source/Test/Unit/Core/Levels/test_level.cpp:17`

Une grille neuve a les bonnes dimensions et n'est composée que de cases Empty.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `map.width()` vaut `4`.
- Vérifie que `map.height()` vaut `3`.
- Vérifie que `map.tile(column, row)` vaut `core::TileType::Empty`.

### TileMapTest.EcritureLectureDUneTuile

*Majeur · Unitaire · Tile Map* — `Source/Test/Unit/Core/Levels/test_level.cpp:40`

Écrire puis lire une tuile restitue le type posé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `map.tile(2, 1)` vaut `core::TileType::Solid`.
- Vérifie que `map.tile(0, 0)` vaut `core::TileType::Empty`.

### TileMapTest.Bornes

*Majeur · Unitaire · Tile Map* — `Source/Test/Unit/Core/Levels/test_level.cpp:58`

Les bornes de la grille sont correctement détectées.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `map.inBounds(0, 0)` est vrai.
- Vérifie que `map.inBounds(3, 2)` est vrai.
- Vérifie que `map.inBounds(4, 2)` est faux.
- Vérifie que `map.inBounds(3, 3)` est faux.
- Vérifie que `map.inBounds(-1, 0)` est faux.

### TileMapTest.Solidite

*Majeur · Unitaire · Tile Map* — `Source/Test/Unit/Core/Levels/test_level.cpp:78`

Seules les tuiles solides bloquent statiquement.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `map.isSolid(0, 0)` est vrai.
- Vérifie que `map.isSolid(1, 0)` est faux.
- Vérifie que `map.isSolid(2, 0)` est faux.

### TileMapTest.IsSolidParType

*Majeur · Unitaire · Tile Map* — `Source/Test/Unit/Core/Levels/test_level.cpp:99`

`isSolid(TileType)` : vrai seulement pour Solid.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `core::isSolid(core::TileType::Solid)` est vrai.
- Vérifie que `core::isSolid(core::TileType::Empty)` est faux.
- Vérifie que `core::isSolid(core::TileType::Entry)` est faux.
- Vérifie que `core::isSolid(core::TileType::Grass)` est faux.

### LevelTest.RestitueSesComposantes

*Majeur · Unitaire · Level* — `Source/Test/Unit/Core/Levels/test_level.cpp:116`

Un Level restitue ses composantes (nom, grille, entrée).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `level.name()` vaut `"Tutoriel"`.
- Vérifie que `level.tileMap().width()` vaut `5`.
- Vérifie que `level.entry()` vaut `(core::GridPosition{1, 1})`.

## test_level_draft.cpp

### LevelDraftTest.PaintTilePoseLeType

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:28`

paintTile pose le type demandé sur la case visée.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Solid`.

### LevelDraftTest.SetEntryDeplaceLEntreeExistante

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:44`

Poser une seconde entrée déplace la première (unicité, EX-EDIT-004).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.entry().has_value()` est vrai.
- Vérifie que `*draft.entry()` vaut `(GridPosition{0, 0})`.
- Vérifie que `*draft.entry()` vaut `(GridPosition{2, 2})`.
- Vérifie que `draft.tileMap().tile(0, 0)` vaut `TileType::Empty`.
- Vérifie que `draft.tileMap().tile(2, 2)` vaut `TileType::Entry`.

### LevelDraftTest.PeindrePardessusLEntreeLInvalide

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:66`

Peindre par-dessus l'entrée invalide la position d'entrée mémorisée.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.entry().has_value()` est faux.
- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Solid`.

### LevelDraftTest.AgrandirConserveLeContenu

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:84`

Agrandir la grille conserve le contenu existant et complète en cases vides.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.tileMap().width()` vaut `5`.
- Vérifie que `draft.tileMap().height()` vaut `5`.
- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Entry`.
- Vérifie que `*draft.entry()` vaut `(GridPosition{1, 1})`.
- Vérifie que `draft.tileMap().tile(4, 4)` vaut `TileType::Empty`.

### LevelDraftTest.ReduireTronqueEtInvalideLEntreePerdue

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:107`

Réduire la grille tronque le contenu hors bornes et invalide l'entrée perdue.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.tileMap().width()` vaut `3`.
- Vérifie que `draft.tileMap().height()` vaut `3`.
- Vérifie que `draft.entry().has_value()` est faux.
- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Solid`.

### LevelDraftTest.ToLevelSansEntreeEchoueProprement

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:131`

toLevel() sur un brouillon sans entrée échoue avec un message récupérable (EX-EDIT-007).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.error.empty()` est faux.

### LevelDraftTest.FromLevelRestitueLeContenu

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:152`

Un brouillon reconstruit depuis un niveau existant restitue son contenu.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `draft.name()` vaut `"Depart"`.
- Vérifie que `*draft.entry()` vaut `(GridPosition{0, 0})`.
- Vérifie que `draft.tileMap().tile(2, 2)` vaut `TileType::Wall`.

### LevelDraftTest.BrouillonNeufSansHistorique

*Mineur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:174`

Un brouillon neuf ne peut ni annuler ni refaire.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.canUndo()` est faux.
- Vérifie que `draft.canRedo()` est faux.

### LevelDraftTest.UndoApresPeintureRestitueLEtatPrecedent

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:190`

undo() après une peinture restitue l'état exact précédent.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.canUndo()` est vrai.
- Vérifie que `undone` est vrai.
- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Empty`.
- Vérifie que `draft.canUndo()` est faux.
- Vérifie que `draft.canRedo()` est vrai.

### LevelDraftTest.RedoApresUndoRestitueLEtatMute

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:213`

redo() après un undo() restitue l'état muté.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `redone` est vrai.
- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Solid`.
- Vérifie que `draft.canUndo()` est vrai.
- Vérifie que `draft.canRedo()` est faux.

### LevelDraftTest.SequenceDeMutationsPuisUndoRestitueLEtatInitial

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:236`

Une séquence de N mutations suivie de N undo() restitue l'état initial exact.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.canUndo()` est faux.
- Vérifie que `draft.tileMap().tile(0, 0)` vaut `TileType::Empty`.
- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Empty`.
- Vérifie que `draft.tileMap().tile(2, 2)` vaut `TileType::Empty`.
- Vérifie que `draft.entry().has_value()` est faux.

### LevelDraftTest.MutationApresUndoInvalideLeRefaire

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:264`

Une nouvelle mutation après un undo() invalide la branche de refaire.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.canRedo()` est vrai.
- Vérifie que `draft.canRedo()` est faux.
- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Dirt`.

### LevelDraftTest.UndoRedoSurPileVideSansEffet

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:286`

undo()/redo() sur une pile vide est sans effet (pas de plantage).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.undo()` est faux.
- Vérifie que `draft.redo()` est faux.
- Vérifie que `draft.tileMap().tile(0, 0)` vaut `TileType::Empty`.

### LevelDraftTest.PaintRegionAppliqueLeBlocEntier

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:303`

paintRegion applique un bloc homogène comme une succession de paintTile équivalente.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.tileMap().tile(1, 1)` vaut `TileType::Solid`.
- Vérifie que `draft.tileMap().tile(2, 1)` vaut `TileType::Solid`.
- Vérifie que `draft.tileMap().tile(1, 2)` vaut `TileType::Solid`.
- Vérifie que `draft.tileMap().tile(2, 2)` vaut `TileType::Solid`.
- Vérifie que `draft.tileMap().tile(0, 0)` vaut `TileType::Empty`.

### LevelDraftTest.PaintRegionUnSeulSnapshotUndo

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:329`

paintRegion ne pousse qu'un seul snapshot undo pour tout le bloc.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.canUndo()` est faux.
- Vérifie que `draft.tileMap().tile(0, 0)` vaut `TileType::Empty`.
- Vérifie que `draft.tileMap().tile(1, 0)` vaut `TileType::Empty`.
- Vérifie que `draft.tileMap().tile(2, 0)` vaut `TileType::Empty`.

### LevelDraftTest.PaintRegionDecoupeAuxBords

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:353`

paintRegion découpe silencieusement le bloc aux bords de la grille.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.tileMap().tile(1, 2)` vaut `TileType::Solid`.
- Vérifie que `draft.tileMap().tile(2, 2)` vaut `TileType::Solid`.

### LevelDraftTest.PaintRegionDeplaceLEntree

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:376`

paintRegion qui inclut une position d'entrée déplace l'entrée existante.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.tileMap().tile(0, 0)` vaut `TileType::Empty`.
- Vérifie que `*draft.entry()` vaut `(GridPosition{2, 2})`.

### LevelDraftTest.PaintRegionBlocVideSansEffet

*Mineur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:396`

paintRegion avec un bloc vide est sans effet.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.canUndo()` est faux.

### LevelDraftTest.WouldResizeDropContentDetecteLaPerte

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:412`

wouldResizeDropContent détecte la perte de l'entrée.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.wouldResizeDropContent(3, 3)` est vrai.
- Vérifie que `draft.wouldResizeDropContent(2, 5)` est vrai.
- Vérifie que `draft.wouldResizeDropContent(10, 10)` est faux.
- Vérifie que `draft.wouldResizeDropContent(5, 5)` est faux.

### LevelDraftTest.WouldResizeDropContentFauxSurBrouillonVierge

*Mineur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:436`

wouldResizeDropContent est faux sur un brouillon vierge, quelle que soit la taille visee.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.wouldResizeDropContent(1, 1)` est faux.

### LevelDraftTest.UndoApresRedimensionnementRestitueLesDimensions

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:453`

L'annulation d'un redimensionnement restitue les dimensions et le contenu précédents.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `draft.entry().has_value()` est faux.
- Vérifie que `draft.tileMap().width()` vaut `5`.
- Vérifie que `draft.tileMap().height()` vaut `5`.
- Vérifie que `draft.entry().has_value()` est vrai.
- Vérifie que `*draft.entry()` vaut `(GridPosition{4, 4})`.

### LevelDraftTest.LaRevisionSuitLHistorique

*Critique · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:481`

La révision du brouillon suit l'historique d'annulation.

**Étapes**

1. Relever la révision d'un brouillon neuf.
2. Peindre, défaire, refaire.

**Résultat attendu**

- Vérifie que `painted` diffère de `initial`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.revision()` vaut `initial`.
- Vérifie que `draft.redo()` est vrai.
- Vérifie que `draft.revision()` vaut `painted`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.revision()` diffère de `initial`.
- Vérifie que `draft.revision()` diffère de `painted`.

### LevelDraftTest.RepeindreLeMemeTypeNeModifieRien

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:509`

Repasser le pinceau sur une case déjà du bon type ne modifie pas la carte.

**Étapes**

1. Peindre une case, relever la révision.
2. Repeindre la même case du même type, et un bloc qui ne change rien.

**Résultat attendu**

- Vérifie que `draft.revision()` vaut `revision`.
- Vérifie que `draft.undoDepth()` vaut `depth`.

### LevelDraftTest.LHistoriqueEstPlafonne

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:533`

L'historique d'annulation est plafonné.

**Étapes**

1. Enchaîner plus de mutations que le plafond.
2. Défaire autant que possible.

**Résultat attendu**

- Vérifie que `draft.undoDepth()` vaut `LevelDraft::UNDO_HISTORY_LIMIT`.
- Vérifie que `draft.tileMap().tile(static_cast<int>(index % WIDTH), static_cast<int>(index / WIDTH))` vaut `TileType::Solid`.
- Vérifie que `draft.tileMap().tile(static_cast<int>(extra % WIDTH), static_cast<int>(extra / WIDTH))` vaut `TileType::Empty`.

### LevelDraftTest.ToJsonRendUnBrouillonIncomplet

*Majeur · Unitaire · Level Draft* — `Source/Test/Unit/Core/Levels/test_level_draft.cpp:566`

Le brouillon se sérialise sans validation, pour la sauvegarde automatique.

**Étapes**

1. Sérialiser un brouillon sans entrée.
2. Poser une entrée, sérialiser, recharger.

**Résultat attendu**

- Vérifie que `incomplete.empty()` est faux.
- Vérifie que `core::LevelLoader::loadFromString(incomplete).ok()` est faux.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `reloaded.level->entry()` vaut `(GridPosition{.column = 1, .row = 1})`.

## test_level_draft_editing.cpp

### EditionDeCarteTest.PremiereCoucheVisuellePromeutLaGrilleUnique

*Majeur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:79`

Ajouter la premiere couche promeut la grille unique.

**Étapes**

1. Ouvrir une carte sans couche declaree.
2. Ajouter une couche de sol.
3. Reconvertir en niveau.

**Résultat attendu**

- Vérifie que `draft.layers().size()` vaut `1U`.
- Vérifie que `draft.layers().front().kind` vaut `core::LayerKind::Legacy`.
- Vérifie que `ground.has_value()` est vrai.
- Vérifie que `draft.layers().size()` vaut `2U`.
- Vérifie que `draft.layers().front().kind` vaut `core::LayerKind::Collision`.
- Vérifie que `tiles.tile(0, 0)` vaut `core::TileType::Wall`.
- Vérifie que `tiles.tile(1, 0)` vaut `core::TileType::Grass`.
- Vérifie que `tiles.tile(2, 0)` vaut `core::TileType::Empty`.
- Vérifie que `tiles.tile(1, 1)` vaut `core::TileType::Empty`.
- Vérifie que `rebuilt.ok()` est vrai.
- Vérifie que `rebuilt.level->layers().size()` vaut `2U`.
- Vérifie que `rebuilt.level->layers().front().kind` vaut `core::LayerKind::Collision`.
- Vérifie que `rebuilt.level->layers().front().tiles.tile(1, 1)` vaut `core::TileType::Entry`.
- Vérifie que `rebuilt.level->layers()[1].tiles.tile(0, 0)` vaut `core::TileType::Wall`.

### EditionDeCarteTest.BrouillonViergeRecoitLaCoucheDeCollision

*Mineur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:114`

Un brouillon vierge s'aligne sur le chargeur.

**Étapes**

1. Creer un brouillon vierge et y poser un mur.
2. Ajouter un sol puis un decor.

**Résultat attendu**

- Vérifie que `draft.addLayer(core::LayerKind::Ground, "sol")` vaut `std::optional<std::size_t>{1}`.
- Vérifie que `draft.addLayer(core::LayerKind::Decor, "decor")` vaut `std::optional<std::size_t>{2}`.
- Vérifie que `draft.layers().size()` vaut `3U`.
- Vérifie que `draft.layers()[0].kind` vaut `core::LayerKind::Collision`.
- Vérifie que `draft.layers()[1].tiles.tile(0, 0)` vaut `core::TileType::Wall`.
- Vérifie que `draft.layers()[2].tiles.tile(0, 0)` vaut `core::TileType::Empty`.

### EditionDeCarteTest.CoucheNonVisuelleRefusee

*Majeur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:136`

Une couche de collision ne se cree pas.

**Étapes**

1. Demander une couche Collision, puis Legacy.

**Résultat attendu**

- Vérifie que `draft.addLayer(core::LayerKind::Collision, "x").has_value()` est faux.
- Vérifie que `draft.addLayer(core::LayerKind::Legacy, "x").has_value()` est faux.
- Vérifie que `draft.canUndo()` est faux.

### EditionDeCarteTest.PeindreUneCoucheVisuelleNeTouchePasLaCollision

*Majeur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:153`

Peindre le sol laisse la collision intacte.

**Étapes**

1. Peindre de l'eau sur le sol.
2. Tenter une entree sur le sol.
3. Reconvertir.

**Résultat attendu**

- Vérifie que `draft.paintLayerTile(ground, 2, 1, core::TileType::Water)` est vrai.
- Vérifie que `draft.paintLayerTile(ground, 2, 1, core::TileType::Entry)` est faux.
- Vérifie que `draft.paintLayerTile(ground, 9, 9, core::TileType::Water)` est faux.
- Vérifie que `rebuilt.ok()` est vrai.
- Vérifie que `rebuilt.level->tileMap().tile(2, 1)` vaut `core::TileType::Empty`.
- Vérifie que `rebuilt.level->layers()[ground].tiles.tile(2, 1)` vaut `core::TileType::Water`.
- Vérifie que `rebuilt.level->layers()[decor].tiles.tile(2, 1)` vaut `core::TileType::Empty`.

### EditionDeCarteTest.GesteDeCoucheSansEffetNEmpileRien

*Mineur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:180`

Un geste de couche refuse n'empile rien.

**Étapes**

1. Peindre l'entree de collision par paintLayerTile.
2. Repeindre l'herbe deja posee sur le sol.

**Résultat attendu**

- Vérifie que `draft.paintLayerTile(collision, 0, 0, core::TileType::Wall)` est faux.
- Vérifie que `draft.paintLayerTile(ground, 0, 0, core::TileType::Grass)` est faux.
- Vérifie que `draft.canUndo()` est faux.

### EditionDeCarteTest.BlocDeCoucheEnUnPasEtRefuseSiMixte

*Majeur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:200`

Un bloc de couche est un seul pas d'annulation.

**Étapes**

1. Peindre un bloc 2x2 de sable qui deborde du bord droit.
2. Annuler.
3. Peindre un bloc melant sable et entree.

**Résultat attendu**

- Vérifie que `draft.paintLayerRegion(decor, 3, 0, sand)` est vrai.
- Vérifie que `draft.layers()[decor].tiles.tile(3, 0)` vaut `core::TileType::Sand`.
- Vérifie que `draft.layers()[decor].tiles.tile(3, 1)` vaut `core::TileType::Sand`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.layers()[decor].tiles.tile(3, 0)` vaut `core::TileType::Empty`.
- Vérifie que `draft.canUndo()` est faux.
- Vérifie que `draft.paintLayerRegion(decor, 0, 0, mixed)` est faux.
- Vérifie que `draft.layers()[decor].tiles.tile(0, 0)` vaut `core::TileType::Empty`.

### EditionDeCarteTest.DeplacerUneCoucheNeFranchitPasLaCollision

*Mineur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:230`

Une couche se deplace entre couches visuelles.

**Étapes**

1. Reculer le decor.
2. Reculer encore le decor devenu premier visuel.
3. Avancer le dernier, puis la collision.

**Résultat attendu**

- Vérifie que `draft.layers()[1].kind` vaut `core::LayerKind::Ground`.
- Vérifie que `draft.layers()[2].kind` vaut `core::LayerKind::Decor`.
- Vérifie que `draft.moveLayer(2, /*forward=*/false)` vaut `std::optional<std::size_t>{1}`.
- Vérifie que `draft.layers()[1].kind` vaut `core::LayerKind::Decor`.
- Vérifie que `draft.moveLayer(1, /*forward=*/false)` vaut `std::optional<std::size_t>{1}`.
- Vérifie que `draft.moveLayer(2, /*forward=*/true)` vaut `std::optional<std::size_t>{2}`.
- Vérifie que `draft.moveLayer(0, /*forward=*/true).has_value()` est faux.

### EditionDeCarteTest.RetirerLesCouchesVisuellesRendLaGrilleUnique

*Majeur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:254`

Retirer toutes les couches visuelles rend la grille unique.

**Étapes**

1. Retirer le decor puis le sol.
2. Reconvertir.

**Résultat attendu**

- Vérifie que `draft.removeLayer(2)` est vrai.
- Vérifie que `draft.removeLayer(1)` est vrai.
- Vérifie que `draft.removeLayer(0)` est faux.
- Vérifie que `draft.layers().size()` vaut `1U`.
- Vérifie que `draft.layers().front().kind` vaut `core::LayerKind::Legacy`.
- Vérifie que `rebuilt.ok()` est vrai.
- Vérifie que `rebuilt.level->layers().size()` vaut `1U`.
- Vérifie que `rebuilt.level->layers().front().kind` vaut `core::LayerKind::Legacy`.

### EditionDeCarteTest.NomEtRoleDUneCoucheSEditent

*Mineur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:277`

Nom et role d'une couche s'editent.

**Étapes**

1. Renommer le sol, puis en faire un decor.
2. Reconvertir.
3. Annuler deux fois.

**Résultat attendu**

- Vérifie que `draft.renameLayer(1, "prairie")` est vrai.
- Vérifie que `draft.renameLayer(1, "prairie")` est faux.
- Vérifie que `draft.setLayerKind(1, core::LayerKind::Decor)` est vrai.
- Vérifie que `draft.setLayerKind(1, core::LayerKind::Collision)` est faux.
- Vérifie que `rebuilt.ok()` est vrai.
- Vérifie que `rebuilt.level->layers()[1].name` vaut `"prairie"`.
- Vérifie que `rebuilt.level->layers()[1].kind` vaut `core::LayerKind::Decor`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.layers()[1].name` vaut `"sol"`.
- Vérifie que `draft.layers()[1].kind` vaut `core::LayerKind::Ground`.

### EditionDeCarteTest.UneProprieteDeCoucheSEditeEtSeDefait

*Majeur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:307`

Une propriete de couche s'edite et se defait.

**Étapes**

1. Donner `scene = martpart` a la couche de sol, puis la meme valeur encore.
2. Tenter une cle vide et la grille de collision.
3. Reconvertir, puis annuler.

**Résultat attendu**

- Vérifie que `draft.setLayerProperty(1, "scene", std::string{"martpart"})` est vrai.
- Vérifie que `draft.setLayerProperty(1, "scene", std::string{"martpart"})` est faux.
- Vérifie que `draft.setLayerProperty(1, "", std::string{"martpart"})` est faux.
- Vérifie que `draft.setLayerProperty(0, "scene", std::string{"martpart"})` est faux.
- Vérifie que `rebuilt.ok()` est vrai.
- Vérifie que `proprietes.contains("scene")` est vrai.
- Vérifie que `std::get<std::string>(proprietes.at("scene"))` vaut `"martpart"`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.layers()[1].properties.contains("scene")` est faux.

### EditionDeCarteTest.EntiteSePoseSeDeplaceEtSeRenseigne

*Majeur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:335`

Une entite se pose, se deplace et se renseigne.

**Étapes**

1. Poser un coffre en (0, 2).
2. Le deplacer en (1, 2).
3. Lui donner une propriete.
4. Reconvertir, puis annuler trois fois.

**Résultat attendu**

- Vérifie que `chest` vaut `std::optional<std::size_t>{1}`.
- Vérifie que `draft.moveEntity(*chest, {.column = 1, .row = 2})` est vrai.
- Vérifie que `draft.setEntityProperty(*chest, "loot", std::string{"potion"})` est vrai.
- Vérifie que `rebuilt.ok()` est vrai.
- Vérifie que `rebuilt.level->entities().size()` vaut `2U`.
- Vérifie que `relu.type` vaut `"chest"`.
- Vérifie que `relu.position` vaut `(core::GridPosition{.column = 1, .row = 2})`.
- Vérifie que `relu.properties.at("loot")` vaut `core::PropertyValue{std::string{"potion"}}`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.entities().size()` vaut `1U`.

### EditionDeCarteTest.GesteDEntiteSansEffetNEmpileRien

*Mineur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:368`

Un geste d'entite refuse n'empile rien.

**Étapes**

1. Poser hors de la grille.
2. Deplacer sur place, hors grille, un rang inconnu.
3. Reassigner la meme propriete, retirer une propriete absente.

**Résultat attendu**

- Vérifie que `draft .placeEntity(core::MapEntity{ .type = "chest", .position = {.column = 4, .row = 0}, .properties = {}}) .has_value()` est faux.
- Vérifie que `draft.moveEntity(0, {.column = 2, .row = 2})` est faux.
- Vérifie que `draft.moveEntity(0, {.column = -1, .row = 2})` est faux.
- Vérifie que `draft.moveEntity(7, {.column = 0, .row = 0})` est faux.
- Vérifie que `draft.setEntityProperty(0, "dialogue", std::string{"heraut"})` est faux.
- Vérifie que `draft.removeEntityProperty(0, "absente")` est faux.
- Vérifie que `draft.removeEntity(3)` est faux.
- Vérifie que `draft.canUndo()` est faux.

### EditionDeCarteTest.CasePartageeDesigneLEntiteDuDessus

*Mineur · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:394`

La case partagee designe l'entite du dessus.

**Étapes**

1. Poser un panneau sur la case du PNJ.
2. Designer la case.
3. Retirer le dialogue du PNJ puis annuler.

**Résultat attendu**

- Vérifie que `sign.has_value()` est vrai.
- Vérifie que `draft.entityAt({.column = 2, .row = 2})` vaut `sign`.
- Vérifie que `draft.entityAt({.column = 0, .row = 0}).has_value()` est faux.
- Vérifie que `draft.removeEntityProperty(0, "dialogue")` est vrai.
- Vérifie que `draft.entities()[0].properties.contains("dialogue")` est faux.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.entities()[0].properties.contains("dialogue")` est vrai.

### EditionDeCarteTest.UnGesteSeDefaitEnUnPas

*Critique · Unitaire · Edition de carte* — `Source/Test/Unit/Core/Levels/test_level_draft_editing.cpp:418`

Un geste = un pas d'annulation.

**Étapes**

1. Ouvrir un geste, peindre trois cases l'une après l'autre, le fermer.
2. Ouvrir un geste sans rien changer.
3. Annuler, refaire.

**Résultat attendu**

- Vérifie que `draft.inGesture()` est vrai.
- Vérifie que `draft.revision()` diffère de `apresUne`.
- Vérifie que `draft.inGesture()` est faux.
- Vérifie que `draft.undoDepth()` vaut `1U`.
- Vérifie que `draft.undoDepth()` vaut `1U`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.revision()` vaut `avant`.
- Vérifie que `draft.tileMap().tile(2, 0)` vaut `core::TileType::Empty`.
- Vérifie que `draft.tileMap().tile(3, 1)` vaut `core::TileType::Empty`.
- Vérifie que `draft.redo()` est vrai.
- Vérifie que `draft.revision()` vaut `apres`.
- Vérifie que `draft.tileMap().tile(3, 1)` vaut `core::TileType::Wall`.

## test_level_draft_pieces.cpp

### LevelDraftPiecesTest.PoserUnePieceEcritSaCoucheSaPieceEtSaCollision

*Critique · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:83`

Poser une pièce écrit sa couche, sa pièce et sa collision.

**Étapes**

1. Poser un pilier en (2, 1) du décor.
2. Défaire.

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 2, .row = 1}, "pillar", TileType::Wall)` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(2, 1)` vaut `"pillar"`.
- Vérifie que `draft.layers()[DECOR].tiles.tile(2, 1)` vaut `TileType::Wall`.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Wall`.
- Vérifie que `draft.undoDepth()` vaut `1U`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(2, 1).empty()` est vrai.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Empty`.

### LevelDraftPiecesTest.PoserPuisGommerUnEtalOccupePuisLibereSesDeuxCases

*Critique · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:109`

Un étal 2 × 1 occupe puis libère ses deux cases.

**Étapes**

1. Poser l'étal en (1, 1).
2. Gommer sa deuxième case, (2, 1).

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 1, .row = 1}, "stall", TileType::Wall)` est vrai.
- Vérifie que `collision(draft, 1, 1)` vaut `TileType::Wall`.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Wall`.
- Vérifie que `collision(draft, 3, 1)` vaut `TileType::Empty`.
- Vérifie que `draft.layers()[DECOR].pieceAt(2, 1).empty()` est vrai.
- Vérifie que `draft.pieceAnchorAt(DECOR, {.column = 2, .row = 1})` vaut `(GridPosition{.column = 1, .row = 1})`.
- Vérifie que `draft.eraseLayerRegion(DECOR, {.column = 2, .row = 1}, {.column = 2, .row = 1})` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 1).empty()` est vrai.
- Vérifie que `draft.layers()[DECOR].tiles.tile(1, 1)` vaut `TileType::Empty`.
- Vérifie que `collision(draft, 1, 1)` vaut `TileType::Empty`.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Empty`.

### LevelDraftPiecesTest.ReposerLaMemePieceNeModifiePasLaCarte

*Majeur · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:137`

Reposer la même pièce ne modifie pas la carte.

**Étapes**

1. Reposer `street` sur une case qui la porte déjà.

**Résultat attendu**

- Vérifie que `draft.placePiece(SOL, {.column = 3, .row = 2}, "street", TileType::Dirt)` est faux.
- Vérifie que `draft.revision()` vaut `revision`.
- Vérifie que `draft.canUndo()` est faux.

### LevelDraftPiecesTest.UnePieceRetireCellesQuElleCouvrirait

*Majeur · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:157`

Une pièce retire celles qu'elle couvrirait.

**Étapes**

1. Poser un étal en (0, 1) et un pilier en (3, 1).
2. Poser un étal en (1, 1).

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 0, .row = 1}, "stall", TileType::Wall)` est vrai.
- Vérifie que `draft.placePiece(DECOR, {.column = 3, .row = 1}, "pillar", TileType::Wall)` est vrai.
- Vérifie que `draft.placePiece(DECOR, {.column = 1, .row = 1}, "stall", TileType::Wall)` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(0, 1).empty()` est vrai.
- Vérifie que `collision(draft, 0, 1)` vaut `TileType::Empty`.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 1)` vaut `"stall"`.
- Vérifie que `draft.layers()[DECOR].pieceAt(3, 1)` vaut `"pillar"`.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Wall`.
- Vérifie que `collision(draft, 3, 1)` vaut `TileType::Wall`.

### LevelDraftPiecesTest.UneEmpriseQuiDebordeEstRefusee

*Mineur · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:182`

Une emprise qui déborde est refusée.

**Étapes**

1. Poser l'étal 2 × 1 sur la dernière colonne.

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 3, .row = 0}, "stall", TileType::Wall)` est faux.
- Vérifie que `draft.canUndo()` est faux.

### LevelDraftPiecesTest.LeRectanglePaveAuPasDeLEmprise

*Majeur · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:198`

Le rectangle pave au pas de l'emprise.

**Étapes**

1. Paver d'étals le rectangle (0, 0)–(2, 1).

**Résultat attendu**

- Vérifie que `draft.placePieceRegion(DECOR, {.column = 0, .row = 0}, {.column = 2, .row = 1}, "stall", TileType::Wall)` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(0, 0)` vaut `"stall"`.
- Vérifie que `draft.layers()[DECOR].pieceAt(0, 1)` vaut `"stall"`.
- Vérifie que `draft.layers()[DECOR].pieceAt(2, 0).empty()` est vrai.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Empty`.
- Vérifie que `draft.undoDepth()` vaut `1U`.
- Vérifie que `draft.placePieceRegion(DECOR, {.column = 0, .row = 0}, {.column = 2, .row = 1}, "stall", TileType::Wall)` est faux.

### LevelDraftPiecesTest.UneFosseArreteLePasEtLEntreeResteLEntree

*Majeur · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:224`

Une fosse arrête le pas ; l'entrée reste l'entrée.

**Étapes**

1. Poser la fosse 1 × 2 en (0, 0), sur l'entrée.

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 0, .row = 0}, "pit", TileType::Cliff)` est vrai.
- Vérifie que `collision(draft, 0, 0)` vaut `TileType::Entry`.
- Vérifie que `collision(draft, 0, 1)` vaut `TileType::Cliff`.

### LevelDraftPiecesTest.UnTypePeintSurUneCoucheLaCollisionSuit

*Majeur · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:243`

Un type peint sur une couche : la collision suit.

**Étapes**

1. Poser un étal en (1, 1).
2. Peindre `dirt` sur son ancre.
3. Peindre `water` profonde sur le sol en (3, 2).

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 1, .row = 1}, "stall", TileType::Wall)` est vrai.
- Vérifie que `draft.paintLayerTile(DECOR, 1, 1, TileType::Dirt)` est vrai.
- Vérifie que `collision(draft, 1, 1)` vaut `TileType::Empty`.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Empty`.
- Vérifie que `draft.paintLayerTile(SOL, 3, 2, TileType::DeepWater)` est vrai.
- Vérifie que `collision(draft, 3, 2)` vaut `TileType::Cliff`.

### LevelDraftPiecesTest.PeindreLaCollisionForceOuLibereLaCase

*Critique · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:266`

Peindre la collision force ou libère la case.

**Étapes**

1. Peindre un mur de collision en (2, 2).
2. Y repeindre du vide.

**Résultat attendu**

- Vérifie que `draft.isCollisionForced(cell)` est vrai.
- Vérifie que `draft.forcedCollision().size()` vaut `1U`.
- Vérifie que `draft.isCollisionForced(cell)` est faux.
- Vérifie que `draft.forcedCollision().empty()` est vrai.

### LevelDraftPiecesTest.UneCaseForceeTientPuisSeLibere

*Critique · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:289`

Une case forcée tient, puis se libère.

**Étapes**

1. Forcer un mur de collision en (1, 1).
2. Poser un étal en (0, 1).
3. Libérer la case.

**Résultat attendu**

- Vérifie que `draft.isCollisionForced({.column = 1, .row = 1})` est vrai.
- Vérifie que `draft.placePiece(DECOR, {.column = 0, .row = 1}, "stall", TileType::Wall)` est vrai.
- Vérifie que `draft.isCollisionForced({.column = 1, .row = 1})` est vrai.
- Vérifie que `draft.unforceCollision({{.column = 1, .row = 1}, {.column = 3, .row = 2}})` est vrai.
- Vérifie que `draft.isCollisionForced({.column = 1, .row = 1})` est faux.
- Vérifie que `collision(draft, 1, 1)` vaut `TileType::Wall`.
- Vérifie que `draft.eraseLayerRegion(DECOR, {.column = 0, .row = 1}, {.column = 0, .row = 1})` est vrai.
- Vérifie que `collision(draft, 1, 1)` vaut `TileType::Empty`.
- Vérifie que `draft.unforceCollision({{.column = 1, .row = 1}})` est faux.

### LevelDraftPiecesTest.UnGesteNeToucheQueSesCases

*Majeur · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:318`

Un geste ne touche que ses cases.

**Étapes**

1. Charger une carte dont la collision s'écarte de la déduction en (3, 0), sans la forcer.
2. Poser un pilier en (1, 2).

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `draft.placePiece(DECOR, {.column = 1, .row = 2}, "pillar", TileType::Wall)` est vrai.
- Vérifie que `collision(draft, 1, 2)` vaut `TileType::Wall`.
- Vérifie que `collision(draft, 3, 0)` vaut `TileType::Wall`.

### LevelDraftPiecesTest.RemplacerUnePieceEnUnPasLaCollisionSuit

*Critique · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:346`

Remplacer une pièce : un pas, la collision suit.

**Étapes**

1. Poser deux piliers.
2. Remplacer `pillar` par `pit`, une fosse 1 × 2.
3. Défaire.

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 1, .row = 0}, "pillar", TileType::Wall)` est vrai.
- Vérifie que `draft.placePiece(DECOR, {.column = 3, .row = 0}, "pillar", TileType::Wall)` est vrai.
- Vérifie que `draft.replacePieces({{"pillar", "pit"}})` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 0)` vaut `"pit"`.
- Vérifie que `draft.layers()[DECOR].pieceAt(3, 0)` vaut `"pit"`.
- Vérifie que `draft.layers()[DECOR].tiles.tile(1, 0)` vaut `TileType::Wall`.
- Vérifie que `collision(draft, 1, 0)` vaut `TileType::Cliff`.
- Vérifie que `collision(draft, 1, 1)` vaut `TileType::Cliff`.
- Vérifie que `draft.undoDepth()` vaut `depth + 1`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 0)` vaut `"pillar"`.
- Vérifie que `collision(draft, 1, 1)` vaut `TileType::Empty`.

### LevelDraftPiecesTest.UnRemplacementQuiDebordeEstRefuseEnEntier

*Majeur · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:378`

Un remplacement qui déborde est refusé en entier.

**Étapes**

1. Poser un pilier au milieu et un sur la dernière ligne.
2. Remplacer `pillar` par `pit` (1 × 2).
3. Remplacer une pièce que la carte ne pose pas.

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 1, .row = 0}, "pillar", TileType::Wall)` est vrai.
- Vérifie que `draft.placePiece(DECOR, {.column = 2, .row = 2}, "pillar", TileType::Wall)` est vrai.
- Vérifie que `draft.replacePieces({{"pillar", "pit"}})` est faux.
- Vérifie que `draft.replacePieces({{"stall", "pillar"}})` est faux.
- Vérifie que `draft.layers()[DECOR].pieceAt(1, 0)` vaut `"pillar"`.
- Vérifie que `draft.undoDepth()` vaut `depth`.

### LevelDraftPiecesTest.ChangerDePlancheTraduitLesPiecesEtRededuitLaCollision

*Critique · Unitaire · Pièces du brouillon* — `Source/Test/Unit/Core/Levels/test_level_draft_pieces.cpp:402`

Changer de planche : lieu, pièces et collision en un pas.

**Étapes**

1. Poser un pilier en (2, 1).
2. Passer à une planche où le pavé s'appelle `paving` et où le pilier, gardant son nom, se franchit.
3. Défaire.

**Résultat attendu**

- Vérifie que `draft.placePiece(DECOR, {.column = 2, .row = 1}, "pillar", TileType::Wall)` est vrai.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Wall`.
- Vérifie que `draft.changeScene("autre", manifest, {{"street", "paving"}})` est vrai.
- Vérifie que `std::get<std::string>(draft.layers()[SOL].properties.at("scene"))` vaut `"autre"`.
- Vérifie que `draft.layers()[SOL].pieceAt(3, 2)` vaut `"paving"`.
- Vérifie que `draft.layers()[DECOR].pieceAt(2, 1)` vaut `"pillar"`.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Empty`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.layers()[SOL].properties.count("scene")` vaut `0U`.
- Vérifie que `draft.layers()[SOL].pieceAt(3, 2)` vaut `"street"`.
- Vérifie que `collision(draft, 2, 1)` vaut `TileType::Wall`.

## test_level_loader.cpp

### LevelLoaderTest.ChargeUnNiveauValide

*Majeur · Unitaire · Level Loader* — `Source/Test/Unit/Core/Levels/test_level_loader.cpp:36`

Un niveau valide est chargé avec ses dimensions, ses tuiles et son entrée.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `result.errorCode` vaut `core::LevelValidationError::None`.
- Vérifie que `level.name()` vaut `"Tutoriel"`.
- Vérifie que `level.tileMap().width()` vaut `4`.
- Vérifie que `level.tileMap().height()` vaut `3`.
- Vérifie que `level.tileMap().tile(0, 0)` vaut `core::TileType::Solid`.
- Vérifie que `level.tileMap().tile(1, 1)` vaut `core::TileType::Entry`.
- Vérifie que `level.tileMap().tile(3, 0)` vaut `core::TileType::Wall`.
- Vérifie que `level.entry()` vaut `(core::GridPosition{1, 1})`.

### LevelLoaderTest.NiveauSansVersionSeChargeSansErreur

*Majeur · Unitaire · Level Loader* — `Source/Test/Unit/Core/Levels/test_level_loader.cpp:63`

Un niveau sans champ version se charge sans erreur.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `result.errorCode` vaut `core::LevelValidationError::None`.

### LevelLoaderTest.VersionSuperieureALaVersionGereeEchoueProprement

*Majeur · Unitaire · Level Loader* — `Source/Test/Unit/Core/Levels/test_level_loader.cpp:80`

Un niveau dont la version depasse celle geree echoue proprement.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `core::LevelValidationError::UnsupportedFormatVersion`.

### LevelLoaderTest.UneCarteDoitPorterExactementUneEntree

*Critique · Unitaire · Level Loader* — `Source/Test/Unit/Core/Levels/test_level_loader.cpp:104`

Une carte doit porter exactement une entrée.

**Étapes**

1. Charger une carte sans tuile d'entrée.
2. Charger une carte à deux tuiles d'entrée.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `core::LevelValidationError::InvalidEntryCount`.

### LevelLoaderTest.TypeDeTuileInconnuRefuse

*Majeur · Unitaire · Level Loader* — `Source/Test/Unit/Core/Levels/test_level_loader.cpp:131`

Un type de tuile inconnu est refusé au chargement.

**Étapes**

1. Charger une carte dont une tuile porte le type « exit ».

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `core::LevelValidationError::UnknownTileType`.

## test_level_writer.cpp

### LevelWriterTest.LeJsonProduitPorteLaVersionEtAucuneCleRetiree

*Mineur · Unitaire · Level Writer* — `Source/Test/Unit/Core/Levels/test_level_writer.cpp:42`

Le JSON produit porte la version et aucune clé retirée du format.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `json.find("\"background\"")` vaut `std::string::npos`.
- Vérifie que `json.find("\"skinSet\"")` vaut `std::string::npos`.
- Vérifie que `json.find("\"cameraFraming\"")` vaut `std::string::npos`.
- Vérifie que `json.find("\"version\"")` diffère de `std::string::npos`.

### LevelWriterTest.SaveToFileEcritUnFichierRechargeable

*Majeur · Unitaire · Level Writer* — `Source/Test/Unit/Core/Levels/test_level_writer.cpp:65`

saveToFile écrit un fichier qui se recharge à l'identique (round-trip disque).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `core::LevelWriter::saveToFile(*loaded.level, path)` est vrai.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `reloaded.level->name()` vaut `loaded.level->name()`.
- Vérifie que `reloaded.level->entry()` vaut `loaded.level->entry()`.
- Vérifie que `reloaded.level->tileMap().tile(column, row)` vaut `loaded.level->tileMap().tile(column, row)`.

### LevelWriterTest.SaveToFileVersDossierInexistantEchoueProprement

*Majeur · Unitaire · Level Writer* — `Source/Test/Unit/Core/Levels/test_level_writer.cpp:98`

saveToFile vers un dossier inexistant échoue proprement (récupérable, EX-NFR-040).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `core::LevelWriter::saveToFile(*loaded.level, path)` est faux.

### LevelWriterTest.UneCarteGardeSaRegionSonAmbianceEtSesClesInconnues

*Majeur · Unitaire · Level Writer* — `Source/Test/Unit/Core/Levels/test_level_writer.cpp:118`

Une carte garde sa région, son ambiance et ses clés inconnues.

**Étapes**

1. Charger une carte qui porte `region`, `ambience` et une clé inconnue.
2. L'écrire, la relire.
3. Changer l'ambiance sur un brouillon, puis défaire.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `std::get<std::string>( loaded.level->properties().at(std::string{core::MAP_REGION_PROPERTY}))` vaut `"central-empire"`.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `reloaded.level->properties()` vaut `loaded.level->properties()`.
- Vérifie que `std::get<std::string>(reloaded.level->properties().at("authoredBy"))` vaut `"valentin"`.
- Vérifie que `draft.setProperty(std::string{core::MAP_AMBIENCE_PROPERTY}, std::string{"night"})` est vrai.
- Vérifie que `draft.setProperty(std::string{core::MAP_AMBIENCE_PROPERTY}, std::string{"night"})` est faux.
- Vérifie que `draft.toJson().find("\"ambience\": \"night\"")` diffère de `std::string::npos`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `std::get<std::string>(draft.properties().at(std::string{core::MAP_AMBIENCE_PROPERTY}))` vaut `"market"`.
- Vérifie que `draft.setProperty(std::string{core::MAP_REGION_PROPERTY}, std::string{})` est vrai.
- Vérifie que `draft.properties().contains(std::string{core::MAP_REGION_PROPERTY})` est faux.
- Vérifie que `draft.toJson().find("\"region\"")` vaut `std::string::npos`.

## test_map_layers.cpp

### CouchesDeCarteTest.CarteVersion2PromueEnCoucheLegacyUnique

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:91`

Une carte version 2 est promue en couche unique.

**Étapes**

1. Charger une carte au format version 2, sans tableau 'layers'.
2. Inspecter les couches du niveau obtenu.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `loaded.level->layers().size()` vaut `1u`.
- Vérifie que `layer.kind` vaut `core::LayerKind::Legacy`.
- Vérifie que `layer.tiles.width()` vaut `loaded.level->tileMap().width()`.
- Vérifie que `layer.tiles.height()` vaut `loaded.level->tileMap().height()`.
- Vérifie que `layer.tiles.tile(0, 0)` vaut `core::TileType::Solid`.
- Vérifie que `loaded.level->entities().empty()` est vrai.

### CouchesDeCarteTest.CarteVersion2ReecriteSansTableauDeCouches

*Mineur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:116`

Une carte version 2 reecrite ne gagne pas de tableau 'layers'.

**Étapes**

1. Charger une carte version 2.
2. La reserialiser.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `json.find("\"layers\"")` vaut `std::string::npos`.
- Vérifie que `json.find("\"entities\"")` vaut `std::string::npos`.

### CouchesDeCarteTest.AllerRetourSurTroisCouchesEtDeuxEntites

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:135`

Une carte a trois couches et deux entites survit a l'aller-retour.

**Étapes**

1. Charger une carte version 3 a trois couches et deux entites.
2. La reserialiser puis la recharger.
3. Comparer couches et entites au niveau d'origine.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `reloaded.level->layers().size()` vaut `loaded.level->layers().size()`.
- Vérifie que `after.name` vaut `before.name`.
- Vérifie que `after.kind` vaut `before.kind`.
- Vérifie que `after.tiles.width()` vaut `before.tiles.width()`.
- Vérifie que `after.tiles.height()` vaut `before.tiles.height()`.
- Vérifie que `after.tiles.tile(column, row)` vaut `before.tiles.tile(column, row)`.
- Vérifie que `reloaded.level->entities().size()` vaut `2u`.
- Vérifie que `reloaded.level->entities()[0].type` vaut `"npc"`.
- Vérifie que `reloaded.level->entities()[0].position` vaut `(core::GridPosition{2, 2})`.
- Vérifie que `reloaded.level->entities()[1].type` vaut `"chest"`.
- Vérifie que `reloaded.level->entities()[1].position` vaut `(core::GridPosition{0, 2})`.

### CouchesDeCarteTest.ChampsInconnusDUneCouchePreservesALaReecriture

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:177`

Les champs inconnus d'une couche sont preserves a la reecriture.

**Étapes**

1. Charger une couche portant quatre cles inconnues (booleen, entier, reel, chaine).
2. Reserialiser puis recharger.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `decor` diffère de `nullptr`.
- Vérifie que `decor->properties.size()` vaut `4u`.
- Vérifie que `std::get<bool>(decor->properties.at("difficultTerrain"))` vaut `true`.
- Vérifie que `std::get<std::int64_t>(decor->properties.at("coverBonus"))` vaut `2`.
- Vérifie que `std::get<double>(decor->properties.at("heightMeters"))` vaut `1.5` (comparaison flottante).
- Vérifie que `std::get<std::string>(decor->properties.at("note"))` vaut `"tapis"`.

### CouchesDeCarteTest.ChampsInconnusDUneEntitePreservesALaReecriture

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:205`

Les champs inconnus d'une entite sont preserves a la reecriture.

**Étapes**

1. Charger une entite portant un dialogue, un niveau et un drapeau.
2. Reserialiser puis recharger.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `reloaded.level->entities().size()` vaut `2u`.
- Vérifie que `npc.properties.size()` vaut `3u`.
- Vérifie que `std::get<std::string>(npc.properties.at("dialogue"))` vaut `"bonjour"`.
- Vérifie que `std::get<std::int64_t>(npc.properties.at("level"))` vaut `3`.
- Vérifie que `std::get<bool>(npc.properties.at("hostile"))` vaut `false`.
- Vérifie que `reloaded.level->entities().back().properties.empty()` est vrai.

### CouchesDeCarteTest.LaCoucheDeCollisionEstLaGrilleDuGameplay

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:233`

La couche de collision est la grille du gameplay.

**Étapes**

1. Charger une carte dont le decor pose un mur en (2, 1), absent de la collision.
2. Lire la grille du niveau.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `loaded.level->tileMap().tile(0, 0)` vaut `core::TileType::Solid`.
- Vérifie que `loaded.level->tileMap().tile(2, 1)` vaut `core::TileType::Empty`.
- Vérifie que `decor` diffère de `nullptr`.
- Vérifie que `decor->tiles.tile(2, 1)` vaut `core::TileType::Wall`.
- Vérifie que `collision` diffère de `nullptr`.
- Vérifie que `&loaded.level->layers().front()` vaut `collision`.
- Vérifie que `collision->tiles.tile(1, 1)` vaut `core::TileType::Entry`.

### CouchesDeCarteTest.CarteDUneVersionFutureRefuseeAvecUnMessageExplicite

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:261`

Une carte d'une version future est refusee explicitement.

**Étapes**

1. Charger une carte declarant la version suivant la version courante.

**Résultat attendu**

- Vérifie que `loaded.ok()` est faux.
- Vérifie que `loaded.errorCode` vaut `core::LevelValidationError::UnsupportedFormatVersion`.
- Vérifie que `loaded.error.find(future)` diffère de `std::string::npos`.

### CouchesDeCarteTest.TuileHorsBornesDansUneCoucheRefusee

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:289`

Une tuile hors bornes dans une couche est refusee.

**Étapes**

1. Charger une carte dont une couche pose une tuile en dehors de la grille.

**Résultat attendu**

- Vérifie que `loaded.ok()` est faux.
- Vérifie que `loaded.errorCode` vaut `core::LevelValidationError::OutOfBounds`.
- Vérifie que `loaded.error.find("sol")` diffère de `std::string::npos`.

### CouchesDeCarteTest.EntiteHorsBornesRefusee

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:317`

Une entite hors bornes est refusee.

**Étapes**

1. Charger une carte dont une entite est posee en dehors de la grille.

**Résultat attendu**

- Vérifie que `loaded.ok()` est faux.
- Vérifie que `loaded.errorCode` vaut `core::LevelValidationError::OutOfBounds`.

### CouchesDeCarteTest.RoleDeCoucheInconnuRetombeSurLeSol

*Mineur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:343`

Un role de couche inconnu ne fait pas echouer la carte.

**Étapes**

1. Charger une carte dont une couche declare un role inconnu.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `loaded.level->layers().size()` vaut `2u`.
- Vérifie que `loaded.level->layers().back().kind` vaut `core::LayerKind::Ground`.
- Vérifie que `loaded.level->layers().back().name` vaut `"brouillard"`.

### CouchesDeCarteTest.BrouillonDEditionPreserveCouchesEtEntites

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:372`

Le brouillon d'edition preserve couches et entites.

**Étapes**

1. Charger une carte version 3 et en faire un brouillon d'edition.
2. Reconvertir le brouillon en niveau, sans rien modifier.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `draft.layers().size()` vaut `3u`.
- Vérifie que `draft.entities().size()` vaut `2u`.
- Vérifie que `rebuilt.ok()` est vrai.
- Vérifie que `rebuilt.level->layers().size()` vaut `3u`.
- Vérifie que `rebuilt.level->entities().size()` vaut `2u`.

### CouchesDeCarteTest.TuilePeinteAtteintLaCoucheDeCollision

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:397`

Une tuile peinte dans l'editeur atteint la couche de collision.

**Étapes**

1. Ouvrir une carte version 3 en brouillon.
2. Peindre un solide en (2, 0).
3. Reconvertir le brouillon en niveau.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `rebuilt.ok()` est vrai.
- Vérifie que `collision` diffère de `nullptr`.
- Vérifie que `collision->tiles.tile(2, 0)` vaut `core::TileType::Solid`.
- Vérifie que `ground` diffère de `nullptr`.
- Vérifie que `ground->tiles.tile(2, 0)` vaut `core::TileType::Empty`.
- Vérifie que `ground->tiles.tile(1, 0)` vaut `core::TileType::Solid`.

### CouchesDeCarteTest.RedimensionnementEmporteCouchesEtEntites

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:428`

Un redimensionnement emporte couches et entites.

**Étapes**

1. Ouvrir une carte version 3 de 4x3 en brouillon.
2. La redimensionner en 2x2.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `draft.wouldResizeDropContent(2, 2)` est vrai.
- Vérifie que `layer.tiles.width()` vaut `2`.
- Vérifie que `layer.tiles.height()` vaut `2`.
- Vérifie que `draft.entities().empty()` est vrai.

### CouchesDeCarteTest.CoucheDeCollisionDeclareeRefusee

*Majeur · Unitaire · Couches de carte* — `Source/Test/Unit/Core/Levels/test_map_layers.cpp:455`

Une couche 'collision' declaree est refusee.

**Étapes**

1. Charger une carte declarant une couche de role 'collision'.

**Résultat attendu**

- Vérifie que `loaded.ok()` est faux.
- Vérifie que `loaded.errorCode` vaut `core::LevelValidationError::ParseError`.
- Vérifie que `loaded.error.find("tiles")` diffère de `std::string::npos`.

## test_rpg_terrain.cpp

### TerrainRpgTest.AllerRetourSurChaqueTypeDeTerrain

*Critique · Unitaire · Terrain RPG* — `Source/Test/Unit/Core/Levels/test_rpg_terrain.cpp:49`

Chaque type de terrain survit a l'aller-retour de format.

**Étapes**

1. Pour chacun des neuf types, charger une carte qui le porte.
2. La reserialiser puis la recharger.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `loaded.level->tileMap().tile(2, 0)` vaut `type`.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `reloaded.level->tileMap().tile(2, 0)` vaut `type`.

### TerrainRpgTest.FranchissabiliteDeChaqueTerrain

*Critique · Unitaire · Terrain RPG* — `Source/Test/Unit/Core/Levels/test_rpg_terrain.cpp:76`

La franchissabilite de chaque terrain est celle que son nom promet.

**Étapes**

1. Interroger isSolid sur les neuf types.

**Résultat attendu**

- Vérifie que `core::isSolid(core::TileType::Wall)` est vrai.
- Vérifie que `core::isSolid(core::TileType::Cliff)` est vrai.
- Vérifie que `core::isSolid(core::TileType::DeepWater)` est vrai.
- Vérifie que `core::isSolid(core::TileType::Grass)` est faux.
- Vérifie que `core::isSolid(core::TileType::Dirt)` est faux.
- Vérifie que `core::isSolid(core::TileType::Sand)` est faux.
- Vérifie que `core::isSolid(core::TileType::Water)` est faux.
- Vérifie que `core::isSolid(core::TileType::Bridge)` est faux.
- Vérifie que `core::isSolid(core::TileType::Stairs)` est faux.

### TerrainRpgTest.LaRiveDistingueLesDeuxEaux

*Majeur · Unitaire · Terrain RPG* — `Source/Test/Unit/Core/Levels/test_rpg_terrain.cpp:100`

L'eau peu profonde se traverse, l'eau profonde non.

**Étapes**

1. Comparer la franchissabilite de Water et DeepWater.

**Résultat attendu**

- Vérifie que `core::isSolid(core::TileType::Water)` diffère de `core::isSolid(core::TileType::DeepWater)`.

### TerrainRpgTest.BorneDeLEnumerationDerivee

*Majeur · Unitaire · Terrain RPG* — `Source/Test/Unit/Core/Levels/test_rpg_terrain.cpp:114`

La borne de l'enumeration reste derivee du dernier type.

**Étapes**

1. Comparer TILE_TYPE_COUNT au dernier enumerateur.
2. Verifier que chaque valeur jusqu'a cette borne porte un nom.

**Résultat attendu**

- Vérifie que `core::TILE_TYPE_COUNT` vaut `static_cast<int>(core::TileType::Stairs) + 1`.
- Vérifie que `core::TILE_TYPE_COUNT` vaut `12`.
- Vérifie que `name.empty()` est faux.
- Vérifie que `std::adjacent_find(sorted.begin(), sorted.end())` vaut `sorted.end()`.

## test_tile_type_name.cpp

### TileTypeNameTest.AllerRetourSurTousLesTypes

*Critique · Unitaire · Nom de type de tuile* — `Source/Test/Unit/Core/Levels/test_tile_type_name.cpp:28`

Chaque type de tuile fait l'aller-retour par son nom textuel sans perte.

**Étapes**

1. Pour tous les types, convertir le type en nom, puis le nom en type.

**Résultat attendu**

- Vérifie que `parsed.has_value()` est vrai.
- Vérifie que `*parsed` vaut `type`.

### TileTypeNameTest.LesNomsSontUniques

*Critique · Unitaire · Nom de type de tuile* — `Source/Test/Unit/Core/Levels/test_tile_type_name.cpp:48`

Deux types de tuiles distincts ne portent jamais le meme nom.

**Étapes**

1. Collecter les noms de tous les types dans un ensemble.

**Résultat attendu**

- Vérifie que `names.size()` vaut `static_cast<std::size_t>(LAST_TILE_TYPE + 1)`.

### TileTypeNameTest.NomInconnuRefuse

*Majeur · Unitaire · Nom de type de tuile* — `Source/Test/Unit/Core/Levels/test_tile_type_name.cpp:66`

Un nom de type inconnu est refuse au lieu d'etre devine.

**Étapes**

1. Convertir une chaine vide, un nom inexistant et un nom de casse differente.

**Résultat attendu**

- Vérifie que `core::parseTileType("").has_value()` est faux.
- Vérifie que `core::parseTileType("mur").has_value()` est faux.
- Vérifie que `core::parseTileType("Solid").has_value()` est faux.
