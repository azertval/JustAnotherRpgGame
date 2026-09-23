# Editor

Tests unitaires — **179 cas** (15 bloquants, 43 critiques, 94 majeurs, 27 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_autosave.cpp`](#test-autosavecpp) | 5 | - | 3 | 2 | - |
| [`test_brush_gesture.cpp`](#test-brush-gesturecpp) | 7 | - | 3 | 3 | 1 |
| [`test_canvas_picking.cpp`](#test-canvas-pickingcpp) | 5 | 2 | - | 3 | - |
| [`test_canvas_scene.cpp`](#test-canvas-scenecpp) | 5 | 1 | - | 3 | 1 |
| [`test_city_view.cpp`](#test-city-viewcpp) | 3 | - | 1 | 1 | 1 |
| [`test_content_check.cpp`](#test-content-checkcpp) | 5 | 3 | 1 | 1 | - |
| [`test_disk_guard.cpp`](#test-disk-guardcpp) | 3 | - | 2 | 1 | - |
| [`test_editor_key_bindings.cpp`](#test-editor-key-bindingscpp) | 7 | - | - | 7 | - |
| [`test_editor_sidecar.cpp`](#test-editor-sidecarcpp) | 4 | - | 1 | 3 | - |
| [`test_editor_status.cpp`](#test-editor-statuscpp) | 8 | - | 3 | 4 | 1 |
| [`test_entity_editing.cpp`](#test-entity-editingcpp) | 8 | - | - | 7 | 1 |
| [`test_entity_shapes.cpp`](#test-entity-shapescpp) | 11 | 1 | - | 8 | 2 |
| [`test_game_launch.cpp`](#test-game-launchcpp) | 6 | - | 1 | 4 | 1 |
| [`test_gesture_script.cpp`](#test-gesture-scriptcpp) | 5 | 1 | 2 | 2 | - |
| [`test_level_file_operations.cpp`](#test-level-file-operationscpp) | 10 | 1 | 5 | 4 | - |
| [`test_level_name_validation.cpp`](#test-level-name-validationcpp) | 5 | - | - | 3 | 2 |
| [`test_map_documents.cpp`](#test-map-documentscpp) | 3 | - | - | 2 | 1 |
| [`test_map_format.cpp`](#test-map-formatcpp) | 8 | - | 5 | 2 | 1 |
| [`test_map_refactor.cpp`](#test-map-refactorcpp) | 9 | - | 5 | 3 | 1 |
| [`test_map_render.cpp`](#test-map-rendercpp) | 4 | - | - | 3 | 1 |
| [`test_paint_tools.cpp`](#test-paint-toolscpp) | 8 | - | 5 | 2 | 1 |
| [`test_panel_focus.cpp`](#test-panel-focuscpp) | 3 | - | - | 3 | - |
| [`test_piece_catalog.cpp`](#test-piece-catalogcpp) | 6 | - | 1 | 4 | 1 |
| [`test_scene_painter.cpp`](#test-scene-paintercpp) | 3 | 2 | - | 1 | - |
| [`test_shipped_maps.cpp`](#test-shipped-mapscpp) | 4 | 4 | - | - | - |
| [`test_stamps.cpp`](#test-stampscpp) | 9 | - | 2 | 6 | 1 |
| [`test_thumbnail_geometry.cpp`](#test-thumbnail-geometrycpp) | 3 | - | 2 | - | 1 |
| [`test_tile_taxonomy.cpp`](#test-tile-taxonomycpp) | 2 | - | - | 2 | - |
| [`test_world_graph_layout.cpp`](#test-world-graph-layoutcpp) | 16 | - | - | 8 | 8 |
| [`test_world_links.cpp`](#test-world-linkscpp) | 4 | - | 1 | 2 | 1 |

## test_autosave.cpp

### AutosaveTest.UnBrouillonSeRelitALIdentique

*Critique · Unitaire · Éditeur, reprise après plantage* — `Source/Test/Unit/Editor/test_autosave.cpp:45`

Un brouillon sauvegardé automatiquement se relit à l'identique.

**Étapes**

1. Écrire le brouillon de la Place.
2. Lister les brouillons en attente.
3. Vérifier qu'il revient tel quel.

**Résultat attendu**

- Vérifie que `store.write(laPlace())` est vrai.
- Vérifie que `pending.size()` vaut `1U`.
- Vérifie que `pending.front()` vaut `laPlace()`.

### AutosaveTest.LeNomDeFichierEstPlat

*Majeur · Unitaire · Éditeur, reprise après plantage* — `Source/Test/Unit/Editor/test_autosave.cpp:63`

Le nom du fichier de reprise aplatit le sous-dossier de la carte.

**Étapes**

1. Nommer le fichier de `bourg/place`, puis d'un identifiant vide.

**Résultat attendu**

- Vérifie que `hmi::autosaveFileName("bourg/place")` vaut `"bourg~place.autosave.json"`.
- Vérifie que `hmi::autosaveFileName("donjon")` vaut `"donjon.autosave.json"`.
- Vérifie que `hmi::autosaveFileName("")` vaut `"untitled.autosave.json"`.

### AutosaveTest.ReecrireRemplaceEtRetirerVide

*Critique · Unitaire · Éditeur, reprise après plantage* — `Source/Test/Unit/Editor/test_autosave.cpp:78`

Une nouvelle sauvegarde remplace l'ancienne ; enregistrer la carte la retire.

**Étapes**

1. Écrire deux fois le même brouillon, modifié.
2. Vérifier qu'un seul reste, le dernier, sans temporaire.
3. Le retirer.

**Résultat attendu**

- Vérifie que `store.write(record)` est vrai.
- Vérifie que `store.write(record)` est vrai.
- Vérifie que `pending.size()` vaut `1U`.
- Vérifie que `pending.front().draftJson` vaut `record.draftJson`.
- Vérifie que `files` vaut `1U`.
- Vérifie que `store.pending().empty()` est vrai.

### AutosaveTest.UnFichierIlliblesEstIgnore

*Majeur · Unitaire · Éditeur, reprise après plantage* — `Source/Test/Unit/Editor/test_autosave.cpp:109`

Un fichier de reprise illisible est ignoré, pas effacé.

**Étapes**

1. Lister un dossier absent.
2. Déposer un fichier tronqué et un fichier d'un autre format.
3. Lister : rien ; les fichiers sont toujours là.

**Résultat attendu**

- Vérifie que `store.pending().empty()` est vrai.
- Vérifie que `store.pending().empty()` est vrai.
- Vérifie que `std::filesystem::exists(dir / "a.autosave.json")` est vrai.
- Vérifie que `std::filesystem::exists(dir / "b.autosave.json")` est vrai.
- Vérifie que `hmi::parseAutosave("pas du json").has_value()` est faux.

### AutosaveTest.UneVersionEcarteeEstGardeeDeCote

*Critique · Unitaire · Éditeur, garde de fichier modifié sur disque* — `Source/Test/Unit/Editor/test_autosave.cpp:132`

La version écartée par un choix de l'auteur est gardée de côté.

**Étapes**

1. Mettre de côté la version disque de la Place.
2. Relire la copie.
3. Vérifier qu'elle n'est pas proposée à la reprise.

**Résultat attendu**

- Vérifie que `kept.has_value()` est vrai.
- Vérifie que `kept->filename().string()` vaut `"bourg~place.disk.20260918-142501.json"`.
- Vérifie que `kept->parent_path().filename().string()` vaut `"conflicts"`.
- Vérifie que `content` vaut `"contenu disque"`.
- Vérifie que `store.pending().empty()` est vrai.

## test_brush_gesture.cpp

### BrushGestureTest.RepeindreUneRueEtUneFacadeRendLeMemeFichier

*Critique · Unitaire · Pinceau* — `Source/Test/Unit/Editor/test_brush_gesture.cpp:99`

Repeindre une rue et une façade rend le même fichier.

**Étapes**

1. Ouvrir la carte d essai.
2. Gommer une case de rue (`street-2`) et un pan de façade (`window-left`), puis les reposer du pinceau de pièce, le décor actif.
3. Écrire le brouillon.

**Résultat attendu**

- Vérifie que `rue && facade` est vrai.
- Vérifie que `carte.draft.toJson()` vaut `carte.fichier`.
- Vérifie que `hmi::applyBrush(carte.draft, GOMME, sol, vue, *rue, *rue).changed` est vrai.
- Vérifie que `hmi::applyBrush(carte.draft, GOMME, decor, vue, *facade, *facade).changed` est vrai.
- Vérifie que `carte.draft.tileMap().tile(rue->column, rue->row)` vaut `TileType::Wall`.
- Vérifie que `carte.draft.toJson()` diffère de `carte.fichier`.
- Vérifie que `hmi::applyBrush(carte.draft, pinceau(carte, "street-2", true), decor, vue, *rue, *rue) .changed` est vrai.
- Vérifie que `hmi::applyBrush(carte.draft, pinceau(carte, "window-left", false), decor, vue, *facade, *facade) .changed` est vrai.
- Vérifie que `carte.draft.toJson()` vaut `carte.fichier`.

### BrushGestureTest.PoserPuisGommerUnEtalSurLaCarteDEssai

*Critique · Unitaire · Pinceau* — `Source/Test/Unit/Editor/test_brush_gesture.cpp:136`

Un étal 2 × 1 posé puis gommé sur la carte d'essai.

**Étapes**

1. Ouvrir la carte d essai.
2. Poser `feature-1` (2 × 1) sur deux cases libres de la place.
3. Le gommer par sa deuxième case.

**Résultat attendu**

- Vérifie que `place` est vrai.
- Vérifie que `hmi::applyBrush(carte.draft, pinceau(carte, "feature-1", false), std::nullopt, vue, *place, *place) .changed` est vrai.
- Vérifie que `carte.draft.layers()[decor].pieceAt(place->column, place->row)` vaut `"feature-1"`.
- Vérifie que `carte.draft.tileMap().tile(place->column, place->row)` vaut `TileType::Wall`.
- Vérifie que `carte.draft.tileMap().tile(seconde.column, seconde.row)` vaut `TileType::Wall`.
- Vérifie que `hmi::applyBrush(carte.draft, GOMME, decor, vue, seconde, seconde).changed` est vrai.
- Vérifie que `carte.draft.tileMap().tile(place->column, place->row)` vaut `TileType::Empty`.
- Vérifie que `carte.draft.tileMap().tile(seconde.column, seconde.row)` vaut `TileType::Empty`.
- Vérifie que `carte.draft.toJson()` vaut `carte.fichier`.

### BrushGestureTest.GlisserUnEtalNeLeDecalePas

*Majeur · Unitaire · Pinceau* — `Source/Test/Unit/Editor/test_brush_gesture.cpp:184`

Glisser un étal ne le décale pas.

**Étapes**

1. Poser `feature-1` au clic.
2. Prolonger le glisser sur sa deuxième case.

**Résultat attendu**

- Vérifie que `hmi::applyBrush(carte.draft, pinceau(carte, "feature-1", false), decor, vue, ancre, ancre) .changed` est vrai.
- Vérifie que `hmi::applyBrush(carte.draft, pinceau(carte, "feature-1", false), decor, vue, seconde, seconde, true) .changed` est faux.
- Vérifie que `carte.draft.layers()[decor].pieceAt(ancre.column, ancre.row)` vaut `"feature-1"`.

### BrushGestureTest.CoucheVerrouilleeOuAbsenteLeGesteEstRefuse

*Majeur · Unitaire · Pinceau* — `Source/Test/Unit/Editor/test_brush_gesture.cpp:210`

Couche verrouillée ou absente : le geste est refusé.

**Étapes**

1. Verrouiller le décor, poser une pièce debout.
2. Poser une pièce debout sur une carte sans décor.

**Résultat attendu**

- Vérifie que `verrou.changed` est faux.
- Vérifie que `verrou.refusal` vaut `"The decor layer is locked."`.
- Vérifie que `absente.changed` est faux.
- Vérifie que `absente.refusal.find("no decor layer")` diffère de `std::string::npos`.
- Vérifie que `carte.draft.canUndo()` est faux.

### BrushGestureTest.SurLaCollisionLePinceauForceEtLaGommeLibere

*Critique · Unitaire · Pinceau* — `Source/Test/Unit/Editor/test_brush_gesture.cpp:242`

Sur la collision, le pinceau force et la gomme libère.

**Étapes**

1. Peindre un mur sur la collision d'une case de rue.
2. La gommer, la collision active.

**Résultat attendu**

- Vérifie que `hmi::applyBrush(carte.draft, mur, std::nullopt, vue, rue, rue).changed` est vrai.
- Vérifie que `carte.draft.isCollisionForced(rue)` est vrai.
- Vérifie que `hmi::applyBrush(carte.draft, GOMME, std::nullopt, vue, rue, rue).changed` est vrai.
- Vérifie que `carte.draft.isCollisionForced(rue)` est faux.
- Vérifie que `carte.draft.toJson()` vaut `carte.fichier`.

### BrushGestureTest.UnTypeVaSurLaCoucheActiveJamaisLEntree

*Majeur · Unitaire · Pinceau* — `Source/Test/Unit/Editor/test_brush_gesture.cpp:268`

Un type va sur la couche active, jamais l'entrée.

**Étapes**

1. Peindre `entry` sur le sol de la carte d'essai.

**Résultat attendu**

- Vérifie que `result.changed` est faux.
- Vérifie que `result.refusal.find("entry lives in the collision grid")` diffère de `std::string::npos`.

### BrushGestureTest.LePinceauSeNomme

*Mineur · Unitaire · Pinceau* — `Source/Test/Unit/Editor/test_brush_gesture.cpp:291`

Le pinceau se nomme.

**Étapes**

1. Nommer un pinceau de type, de pièce, la gomme.

**Résultat attendu**

- Vérifie que `hmi::brushLabel( {.kind = BrushKind::Type, .type = TileType::Grass, .piece = {}, .floor = false})` vaut `"grass"`.
- Vérifie que `hmi::brushLabel({.kind = BrushKind::Piece, .type = TileType::Wall, .piece = "wall-left", .floor = false})` vaut `"wall-left"`.
- Vérifie que `hmi::brushLabel(GOMME)` vaut `"Eraser"`.

## test_canvas_picking.cpp

### CanvasPickingTest.LePointageEstJusteAuxQuatreCoins

*Bloquant · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_picking.cpp:68`

Le pointage iso est juste aux quatre coins de la carte.

**Étapes**

1. Projeter la carte d'essai (48 x 40 cases).
2. Pointer le centre des quatre cases de coin, puis un point pres de chacun de leurs sommets, a l'interieur du losange.
3. Pointer juste au-dela du sommet exterieur de chaque coin.

**Résultat attendu**

- Vérifie que `projection.columns()` vaut `48`.
- Vérifie que `projection.rows()` vaut `40`.
- Vérifie que `hmi::pickIsoCell(projection, center)` vaut `corner.cell`.
- Vérifie que `hmi::pickIsoCell(projection, toward(vertex, center, 0.05F))` vaut `corner.cell`.
- Vérifie que `hmi::pickIsoCell(projection, outside).has_value()` est faux.
- Vérifie que `hmi::clampedIsoCell(projection, outside)` vaut `corner.cell`.

### CanvasPickingTest.SousUnMurHautOnPointeLaCaseDerriere

*Bloquant · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_picking.cpp:111`

Sous un mur haut, le pointage designe la case par son pied.

**Étapes**

1. Composer la carte d'essai avec la taille des pieces de son manifeste.
2. Prendre une piece de relief plus haute que deux losanges, et la case juste derriere elle (colonne - 1, ligne - 1).
3. Pointer le centre du losange de cette case.

**Résultat attendu**

- Vérifie que `appearance.ok()` est vrai.
- Vérifie que `manifest.ok()` est vrai.
- Vérifie que `contains(bounds, point)` est vrai.
- Vérifie que `hmi::pickIsoCell(projection, point)` vaut `behind`.
- Vérifie que `checked` est vrai.

### CanvasPickingTest.LaHauteurSePrendEnParametre

*Majeur · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_picking.cpp:178`

Le pointage prend la hauteur en parametre.

**Étapes**

1. Elever d'un niveau le losange d'une case.
2. Pointer son centre avec la hauteur 1, puis avec la hauteur 0.

**Résultat attendu**

- Vérifie que `hmi::pickIsoCell(projection, center, 1)` vaut `cell`.
- Vérifie que `hmi::pickIsoCell(projection, center, 0)` vaut `(core::GridPosition{.column = 4, .row = 4})`.
- Vérifie que `hmi::isoCellDiamond(projection, cell, 0)[0]` vaut `projection.gridToWorld({5.0F, 5.0F})`.

### CanvasPickingTest.OnNeParcourtQueLesCasesVisibles

*Majeur · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_picking.cpp:199`

Le canevas ne parcourt que les cases visibles.

**Étapes**

1. Demander les cases couvertes par la scene entiere de la carte d'essai.
2. Demander celles d'un petit rectangle autour du centre d'une case.
3. Faire de meme en vue a plat.

**Résultat attendu**

- Vérifie que `all` vaut `(hmi::CellRange{.firstColumn = 0, .firstRow = 0, .lastColumn = 47, .lastRow = 39})`.
- Vérifie que `detail.contains(cell)` est vrai.
- Vérifie que `detail.lastColumn - detail.firstColumn` est inférieur ou égal à `2`.
- Vérifie que `detail.lastRow - detail.firstRow` est inférieur ou égal à `2`.
- Vérifie que `hmi::flatCellsCovering(core::Rect{{2.5F, 3.2F}, {2.0F, 1.0F}}, 48, 40)` vaut `(hmi::CellRange{.firstColumn = 2, .firstRow = 3, .lastColumn = 4, .lastRow = 4})`.
- Vérifie que `hmi::flatCellsCovering(core::Rect{{60.0F, 3.0F}, {2.0F, 1.0F}}, 48, 40).empty()` est vrai.

### CanvasPickingTest.EnVueAPlatUneCaseParUnite

*Majeur · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_picking.cpp:232`

Le pointage a plat designe la case sous le point.

**Étapes**

1. Pointer dans la grille, puis juste a gauche de la colonne 0.

**Résultat attendu**

- Vérifie que `hmi::pickFlatCell({3.7F, 1.2F}, 10, 5)` vaut `(core::GridPosition{.column = 3, .row = 1})`.
- Vérifie que `hmi::pickFlatCell({-0.2F, 1.0F}, 10, 5).has_value()` est faux.
- Vérifie que `hmi::clampedFlatCell({-0.2F, 1.0F}, 10, 5)` vaut `(core::GridPosition{.column = 0, .row = 1})`.
- Vérifie que `hmi::clampedFlatCell({42.0F, 9.0F}, 10, 5)` vaut `(core::GridPosition{.column = 9, .row = 4})`.

## test_canvas_scene.cpp

### CanvasSceneTest.LaCarteDEssaiSeComposeCommeDansLeJeu

*Bloquant · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_scene.cpp:94`

Le canevas iso compose une carte comme le jeu.

**Étapes**

1. Ouvrir la carte d'essai comme brouillon d'editeur et prendre l'instantane du canevas.
2. Entrer dans la carte d'essai avec le moteur du jeu (`hmi::WorldPlay`) et prendre son instantane.
3. Composer les deux avec les memes textures.

**Résultat attendu**

- Vérifie que `play.enter("bourg/place", {})` est vrai.
- Vérifie que `game.figures.empty()` est faux.
- Vérifie que `editor.place` vaut `"bourg"`.
- Vérifie que `editor` vaut `game`.
- Vérifie que `editor.figures.empty()` est faux.
- Vérifie que `fromGame.size()` vaut `fromEditor.size() + 1`.
- Vérifie que `matched` vaut `fromEditor.size()`.
- Vérifie que `fromEditor.size()` est strictement supérieur à `1000U`.

### CanvasSceneTest.LesReglagesDeCoucheAgissentSurLeurBande

*Majeur · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_scene.cpp:145`

Masquer, griser, voir a travers : chaque reglage agit sur sa bande.

**Étapes**

1. Une carte a couche de sol et couche de decor, reglages par defaut.
2. Masquer le decor, griser le sol, activer les reliefs en transparence.
3. Rendre la collision active.

**Résultat attendu**

- Vérifie que `defaults` vaut `(hmi::IsoBandOpacity{ .floors = 1.0F, .relief = 1.0F, .figures = 1.0F, .collision = 0.0F})`.
- Vérifie que `changed.relief` vaut `0.0F` (comparaison flottante).
- Vérifie que `changed.floors` vaut `hmi::DIMMED_LAYER_OPACITY` (comparaison flottante).
- Vérifie que `hmi::bandOpacity(changed, hmi::RenderLayer::Object)` vaut `0.0F` (comparaison flottante).
- Vérifie que `hmi::bandOpacity(changed, hmi::RenderLayer::Tile)` vaut `hmi::DIMMED_LAYER_OPACITY` (comparaison flottante).
- Vérifie que `hmi::isoBandOpacity(layers, view, 0U, true).relief` vaut `hmi::SEE_THROUGH_RELIEF_OPACITY` (comparaison flottante).
- Vérifie que `collision.collision` vaut `hmi::DEFAULT_COLLISION_OVERLAY_OPACITY * hmi::COLLISION_MASK_OPACITY` (comparaison flottante).

### CanvasSceneTest.UneGrilleUniqueEstLImage

*Majeur · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_scene.cpp:185`

Une grille unique est l'image du canevas iso.

**Étapes**

1. Une carte sans couche visuelle ; masquer puis montrer sa grille.

**Résultat attendu**

- Vérifie que `hmi::isoBandOpacity({}, view, std::nullopt, false).floors` vaut `1.0F` (comparaison flottante).
- Vérifie que `hmi::isoBandOpacity({}, view, std::nullopt, false).collision` vaut `0.0F` (comparaison flottante).
- Vérifie que `hmi::isoBandOpacity({}, view, std::nullopt, false).floors` vaut `0.0F` (comparaison flottante).

### CanvasSceneTest.GriserEtVerrouillerSontDesReglagesDEditeur

*Majeur · Unitaire · Editeur · Couches* — `Source/Test/Unit/Editor/test_canvas_scene.cpp:202`

Griser et verrouiller une couche, puis tout oublier.

**Étapes**

1. Verrouiller la collision, griser une couche visuelle.
2. Remettre les reglages a zero.

**Résultat attendu**

- Vérifie que `view.display(std::nullopt, true).locked` est vrai.
- Vérifie que `view.display(0U, true).dimmed` est vrai.
- Vérifie que `view.display(0U, true).effectiveOpacity()` vaut `hmi::DIMMED_LAYER_OPACITY` (comparaison flottante).
- Vérifie que `view.display(std::nullopt, true).locked` est faux.
- Vérifie que `view.display(0U, true).dimmed` est faux.

### CanvasSceneTest.LesPiecesDUneCaseSeLisent

*Mineur · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_canvas_scene.cpp:228`

Les pieces d'une case se lisent dans la barre d'etat.

**Étapes**

1. Un instantane dont une case a un sol et un relief, une autre un sol seul.

**Résultat attendu**

- Vérifie que `hmi::cellPieces(snapshot, {.column = 0, .row = 0})` vaut `"street · wall-left"`.
- Vérifie que `hmi::cellPieces(snapshot, {.column = 1, .row = 0})` vaut `"square"`.
- Vérifie que `hmi::cellPieces(snapshot, {.column = 5, .row = 0})` vaut `""`.

## test_city_view.cpp

### VueDeVille.UneVilleSeVoitParQuartiers

*Critique · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_city_view.cpp:39`

Une ville se voit par quartiers.

**Étapes**

1. Bâtir la vue de la ville `bourg`.

**Résultat attendu**

- Vérifie que `ville.ok()` est vrai.
- Vérifie que `ville.location` vaut `"test-city"`.
- Vérifie que `ville.image` vaut `"city-test-city.jpg"`.
- Vérifie que `ville.districts.size()` est supérieur ou égal à `2U`.
- Vérifie que `place` diffère de `nullptr`.
- Vérifie que `place->mapId` vaut `"bourg/place"`.
- Vérifie que `place->mapExists` est vrai.
- Vérifie que `place->framed` est vrai.
- Vérifie que `place->name.empty()` est faux.
- Vérifie que `place->name` diffère de `place->id`.
- Vérifie que `nord` diffère de `nullptr`.
- Vérifie que `nord->mapId.empty()` est vrai.
- Vérifie que `nord->guardMapId` vaut `"bourg/place"`.
- Vérifie que `nord->mapExists` est faux.
- Vérifie que `std::all_of(sansCarte, ville.districts.end(), [](const hmi::CityDistrictView& d) { return d.mapId.empty(); })` est vrai.

### VueDeVille.UnClicSurLePlanDesigneUnQuartier

*Majeur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_city_view.cpp:77`

Un clic sur le plan désigne un quartier.

**Étapes**

1. Pointer le centre du cadre de la Place, puis un coin du plan.

**Résultat attendu**

- Vérifie que `ville.ok()` est vrai.
- Vérifie que `vise.has_value()` est vrai.
- Vérifie que `ville.districts[*vise].id` vaut `"test-city-place"`.
- Vérifie que `hmi::districtAt(ville, hmi::MapPoint{.x = 0.01, .y = 0.99}).has_value()` est faux.

### VueDeVille.UneVilleInconnueLeDit

*Mineur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_city_view.cpp:99`

Une ville inconnue le dit.

**Étapes**

1. Bâtir la vue d'une ville qui n'existe pas.
2. Demander la ville de `bourg/place`, puis celle d'une carte hors ville.

**Résultat attendu**

- Vérifie que `aucune.ok()` est faux.
- Vérifie que `aucune.error.find("atlantide")` diffère de `std::string::npos`.
- Vérifie que `hmi::cityOfMap(elements(), "bourg/place")` vaut `"bourg"`.
- Vérifie que `hmi::cityOfMap(elements(), "donjon")` vaut `""`.
- Vérifie que `hmi::cityIds(elements()).empty()` est faux.

## test_content_check.cpp

### ContentCheckTest.ChaqueDefautDeContenuSort

*Bloquant · Unitaire · Contrôle du contenu* — `Source/Test/Unit/Editor/test_content_check.cpp:145`

Chaque défaut de contenu sort, et la CI échoue.

**Étapes**

1. Un projet de trois cartes et une variante : une carte fautive (nom qui n'est pas une clé, îlot sans clé, dialogue inconnu, drapeau que rien ne pose, famille inconnue, rencontre dont un rat tombe hors de la carte, PNJ muré, coffre dans un mur, zone hors d'atteinte, portail sans retour, point d'arrivée orphelin), sa cible, et la variante d'une base dont un mur est tombé sur son PNJ.
2. Lancer `--check`.

**Résultat attendu**

- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "map name \"fautive\" is not a translation key of en.lang, fr.lang " "(expected map.fautive.name)")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "key city_block.ilot-sans-cle is missing")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "nuit", "map name \"map.nuit.name\" is not a translation key of en.lang")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "dialogue \"inconnu\"")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "no dialogue sets flag \"jamais")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Warning, "fautive", "\"mystere\" is unknown")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "Encounter \"rats-du-donjon\": \"rat-d-essai\" would stand off the map")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "npc e2 cannot be reached from the entry or an arrival point")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "chest e3 stands on a blocked cell")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "zone e8: no cell of it can be reached")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "fautive", "npc e1 cannot be reached")` est faux.
- Vérifie que `signale(constats, MapCheckSeverity::Warning, "fautive", "portal e4 (autre) has no way back")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Warning, "fautive", "spawnPoint e5 (orpheline) is named by no portal and no city start")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Warning, "autre", "quai")` est faux.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "nuit", "npc e1 stands on a blocked cell")` est vrai.
- Vérifie que `mure` diffère de `constats.end()`.
- Vérifie que `mure->entityId` vaut `"e2"`.
- Vérifie que `mure->cell` vaut `(core::GridPosition{.column = 6, .row = 2})`.
- Vérifie que `hmi::runMapCommand({"--check", "--data", projet.racine().string()}, {}, sortie)` vaut `1`.

### ContentCheckTest.UnSeulDefautFaitEchouerLaCi

*Bloquant · Unitaire · Contrôle du contenu* — `Source/Test/Unit/Editor/test_content_check.cpp:226`

Un PNJ muré fait échouer la CI.

**Étapes**

1. Une carte canonique, au nom traduit, dont le seul défaut est un PNJ muré.
2. Lancer `--check`.

**Résultat attendu**

- Vérifie que `hmi::runMapCommand({"--check", "--data", projet.racine().string()}, {}, sortie)` vaut `1`.
- Vérifie que `sortie.find("seule (2, 0): error: npc e1 cannot be reached")` diffère de `std::string::npos`.
- Vérifie que `sortie.find("1 errors")` diffère de `std::string::npos`.

### ContentCheckTest.UnPointDArriveeNommeEstUnDepart

*Critique · Unitaire · Contrôle du contenu* — `Source/Test/Unit/Editor/test_content_check.cpp:251`

On atteint une carte par ses points d'arrivée.

**Étapes**

1. Une carte coupée en deux : l'entrée d'un côté, un point d'arrivée nommé par le portail d'une autre carte de l'autre, et un PNJ près de lui.
2. Contrôler.

**Résultat attendu**

- Vérifie que `bilan.ok()` est vrai.
- Vérifie que `bilan.count(MapCheckSeverity::Warning)` vaut `0U`.

### ContentCheckTest.LesCataloguesSeCompletentSansRienPerdre

*Majeur · Unitaire · Contrôle du contenu* — `Source/Test/Unit/Editor/test_content_check.cpp:282`

Une clé ajoutée garde les traductions qu'elle reprend.

**Étapes**

1. Former la clé de `bourg/place`.
2. Ajouter une clé nouvelle, puis une clé qui reprend une clé existante, à deux catalogues dont l'un n'a pas de fin de ligne.

**Résultat attendu**

- Vérifie que `hmi::mapNameKey("bourg/place")` vaut `"map.bourg.place.name"`.
- Vérifie que `hmi::mapNameKey("donjon")` vaut `"map.donjon.name"`.
- Vérifie que `hmi::addTranslation(dossier, "map.b.name", "b")` est vrai.
- Vérifie que `hmi::addTranslation(dossier, "map.c.name", "c", "map.a.name")` est vrai.
- Vérifie que `hmi::addTranslation(dossier, "map.a.name", "a")` est vrai.
- Vérifie que `lire(dossier / "fr.lang")` vaut `"# Catalogue\nmap.a.name = Ancienne\nmap.b.name = b\nmap.c.name = Ancienne\n"`.
- Vérifie que `lire(dossier / "en.lang")` vaut `"map.a.name = Old\nmap.b.name = b\nmap.c.name = Old\n"`.
- Vérifie que `hmi::languagesMissing(catalogues, "map.c.name").empty()` est vrai.
- Vérifie que `hmi::languagesMissing(catalogues, "map.z.name")` vaut `(std::vector<std::string>{"en", "fr"})`.

### ContentCheckTest.UneCarteNeuveASonNomDansChaqueCatalogue

*Bloquant · Unitaire · Contrôle du contenu* — `Source/Test/Unit/Editor/test_content_check.cpp:316`

Une carte neuve a son nom dans chaque catalogue.

**Étapes**

1. Créer « Echoppe » au lieu `bourg` dans un projet à deux catalogues.
2. La contrôler ; la renommer « Etal », la dupliquer.

**Résultat attendu**

- Vérifie que `cree.ok()` est vrai.
- Vérifie que `lue.ok()` est vrai.
- Vérifie que `lue.level->name()` vaut `"map.Echoppe.name"`.
- Vérifie que `bilan.ok()` est vrai.
- Vérifie que `renomme.ok()` est vrai.
- Vérifie que `core::LevelLoader::loadFromFile(renomme.path).level->name()` vaut `"map.Etal.name"`.
- Vérifie que `catalogues.at("en").at("map.Etal.name")` vaut `"The stall"`.
- Vérifie que `catalogues.at("fr").at("map.Etal.name")` vaut `"Echoppe"`.
- Vérifie que `copie.ok()` est vrai.
- Vérifie que `core::LevelLoader::loadFromFile(copie.path).level->name()` vaut `hmi::mapNameKey(copie.path.stem().string())`.
- Vérifie que `hmi::checkAllMaps(projet.racine()).ok()` est vrai.

## test_disk_guard.cpp

### DiskGuardTest.LEmpreinteSuitLeContenu

*Critique · Unitaire · Éditeur, garde de fichier modifié sur disque* — `Source/Test/Unit/Editor/test_disk_guard.cpp:40`

L'empreinte suit le contenu du fichier, pas sa date.

**Étapes**

1. Écrire une carte et relever son empreinte.
2. La réécrire à l'identique, puis changer un octet, puis la supprimer.
3. Comparer à chaque étape.

**Résultat attendu**

- Vérifie que `known.exists` est vrai.
- Vérifie que `known` vaut `hmi::fingerprintOf(R"({"name": "La Place"})")`.
- Vérifie que `hmi::compareFingerprints(known, hmi::fingerprintFile(file))` vaut `hmi::DiskChange::None`.
- Vérifie que `hmi::compareFingerprints(known, hmi::fingerprintFile(file))` vaut `hmi::DiskChange::Modified`.
- Vérifie que `hmi::fingerprintFile(file).exists` est faux.
- Vérifie que `hmi::compareFingerprints(known, hmi::fingerprintFile(file))` vaut `hmi::DiskChange::Deleted`.

### DiskGuardTest.UnFichierApparuEstUnChangement

*Majeur · Unitaire · Éditeur, garde de fichier modifié sur disque* — `Source/Test/Unit/Editor/test_disk_guard.cpp:68`

Un fichier apparu sous une carte jamais enregistrée est un changement.

**Étapes**

1. Comparer une empreinte absente à celle d'un contenu.

**Résultat attendu**

- Vérifie que `hmi::compareFingerprints(hmi::FileFingerprint{}, hmi::fingerprintOf("{}"))` vaut `hmi::DiskChange::Modified`.
- Vérifie que `hmi::compareFingerprints(hmi::FileFingerprint{}, hmi::FileFingerprint{})` vaut `hmi::DiskChange::None`.

### DiskGuardTest.LAuteurNEstInterrogeQueSIlAQuelqueChoseAPerdre

*Critique · Unitaire · Éditeur, garde de fichier modifié sur disque* — `Source/Test/Unit/Editor/test_disk_guard.cpp:83`

Brouillon modifié et fichier changé : l'éditeur demande ; sinon il relit.

**Étapes**

1. Croiser chaque changement avec un brouillon intact et un brouillon modifié.

**Résultat attendu**

- Vérifie que `hmi::reactToDiskChange(DiskChange::None, true)` vaut `DiskReaction::Ignore`.
- Vérifie que `hmi::reactToDiskChange(DiskChange::Modified, false)` vaut `DiskReaction::ReloadQuietly`.
- Vérifie que `hmi::reactToDiskChange(DiskChange::Modified, true)` vaut `DiskReaction::AskReloadOrKeep`.
- Vérifie que `hmi::reactToDiskChange(DiskChange::Deleted, false)` vaut `DiskReaction::WarnDeleted`.
- Vérifie que `hmi::reactToDiskChange(DiskChange::Deleted, true)` vaut `DiskReaction::WarnDeleted`.

## test_editor_key_bindings.cpp

### EditorKeyBindingsTest.ValeursParDefautALaConstruction

*Majeur · Unitaire · Editor Key Bindings* — `Source/Test/Unit/Editor/test_editor_key_bindings.cpp:19`

Les dix actions ont leurs valeurs par défaut (S/Z/Y/C/V/P/F10/F1/F2/T) à la construction.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `bindings.key(hmi::EditorAction::Save)` vaut `hmi::Key::S`.
- Vérifie que `bindings.key(hmi::EditorAction::Undo)` vaut `hmi::Key::Z`.
- Vérifie que `bindings.key(hmi::EditorAction::Redo)` vaut `hmi::Key::Y`.
- Vérifie que `bindings.key(hmi::EditorAction::Copy)` vaut `hmi::Key::C`.
- Vérifie que `bindings.key(hmi::EditorAction::Paste)` vaut `hmi::Key::V`.
- Vérifie que `bindings.key(hmi::EditorAction::Playtest)` vaut `hmi::Key::P`.
- Vérifie que `bindings.key(hmi::EditorAction::ToggleGrid)` vaut `hmi::Key::F10`.
- Vérifie que `bindings.key(hmi::EditorAction::ToggleHelp)` vaut `hmi::Key::F1`.
- Vérifie que `bindings.key(hmi::EditorAction::Rename)` vaut `hmi::Key::F2`.

### EditorKeyBindingsTest.SetKeyEchangeSurConflit

*Majeur · Unitaire · Editor Key Bindings* — `Source/Test/Unit/Editor/test_editor_key_bindings.cpp:44`

`setKey` sur une touche déjà liée à une autre action échange les deux touches.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `bindings.key(hmi::EditorAction::Rename)` vaut `hmi::Key::S`.
- Vérifie que `bindings.key(hmi::EditorAction::Save)` vaut `hmi::Key::F2`.

### EditorKeyBindingsTest.ResetToDefaultsRestaureLesDefauts

*Majeur · Unitaire · Editor Key Bindings* — `Source/Test/Unit/Editor/test_editor_key_bindings.cpp:62`

`resetToDefaults` restaure les valeurs par défaut après un ou plusieurs remaps.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `bindings.key(hmi::EditorAction::Playtest)` vaut `hmi::Key::P`.

### EditorKeyBindingsTest.SaveEtLoadAllerRetour

*Majeur · Unitaire · Editor Key Bindings* — `Source/Test/Unit/Editor/test_editor_key_bindings.cpp:80`

Sauvegarder puis recharger préserve un jeu de bindings personnalisé (aller-retour JSON).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `original.save(path)` est vrai.
- Vérifie que `reloaded.key(hmi::EditorAction::Playtest)` vaut `hmi::Key::F1`.
- Vérifie que `reloaded.key(hmi::EditorAction::Save)` vaut `hmi::Key::S`.

### EditorKeyBindingsTest.LoadFichierAbsentRenvoieLesDefauts

*Majeur · Unitaire · Editor Key Bindings* — `Source/Test/Unit/Editor/test_editor_key_bindings.cpp:108`

Charger depuis un fichier absent renvoie les valeurs par défaut, sans erreur.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `bindings.key(hmi::EditorAction::Save)` vaut `hmi::Key::S`.

### EditorKeyBindingsTest.SavePreserveLaSectionJeu

*Majeur · Unitaire · Editor Key Bindings* — `Source/Test/Unit/Editor/test_editor_key_bindings.cpp:128`

Sauvegarder les touches d'éditeur préserve une section « jeu » déjà présente.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `bindings.save(path)` est vrai.
- Vérifie que `content.find("\"jeu\"")` diffère de `std::string::npos`.
- Vérifie que `content.find("sauter")` diffère de `std::string::npos`.

### EditorKeyBindingsTest.SavePreserveLaSectionManette

*Majeur · Unitaire · Editor Key Bindings* — `Source/Test/Unit/Editor/test_editor_key_bindings.cpp:162`

Sauvegarder les touches d'editeur preserve une section manette deja presente.

**Étapes**

1. Ecrire un fichier avec une section manette.
2. Sauvegarder des touches d'editeur dessus.
3. Relire le fichier.

**Résultat attendu**

- Vérifie que `bindings.save(path)` est vrai.
- Vérifie que `content.find("\"manette\"")` diffère de `std::string::npos`.
- Vérifie que `content.find("sauter")` diffère de `std::string::npos`.

## test_editor_sidecar.cpp

### EditorSidecarTest.LAnnexeVitACoteDeLaCarte

*Majeur · Unitaire · Notes d'auteur* — `Source/Test/Unit/Editor/test_editor_sidecar.cpp:52`

L'annexe vit à côté de la carte.

**Étapes**

1. Nommer l'annexe de `bourg/place.json`.
2. Lister un dossier de cartes qui contient une carte et son annexe.

**Résultat attendu**

- Vérifie que `hmi::sidecarPath(std::filesystem::path("bourg") / "place.json")` vaut `std::filesystem::path("bourg") / "place.editor.json"`.
- Vérifie que `hmi::isSidecarFile("place.editor.json")` est vrai.
- Vérifie que `hmi::isSidecarFile("place.json")` est faux.
- Vérifie que `operations.create("place", 4, 4).ok()` est vrai.
- Vérifie que `hmi::writeSidecar(hmi::sidecarPath(dossier.chemin() / "place.json"), annexe)` est vrai.
- Vérifie que `cartes.size()` vaut `1U`.
- Vérifie que `cartes.front().filename()` vaut `"place.json"`.

### EditorSidecarTest.LesNotesSEcriventSeRelisentEtSeRetirent

*Critique · Unitaire · Notes d'auteur* — `Source/Test/Unit/Editor/test_editor_sidecar.cpp:80`

Les notes s'écrivent, se relisent et se retirent.

**Étapes**

1. Écrire deux notes, en réécrire une, écrire l'annexe.
2. La relire.
3. Effacer les deux notes, réécrire.

**Résultat attendu**

- Vérifie que `hmi::setNote(annexe, {.column = 5, .row = 2}, "door to the tavern")` est vrai.
- Vérifie que `hmi::setNote(annexe, {.column = 1, .row = 0}, "gate")` est vrai.
- Vérifie que `hmi::setNote(annexe, {.column = 1, .row = 0}, "gate")` est faux.
- Vérifie que `hmi::setNote(annexe, {.column = 1, .row = 0}, "north gate")` est vrai.
- Vérifie que `hmi::setNote(annexe, {.column = 9, .row = 9}, " ")` est faux.
- Vérifie que `annexe.notes.size()` vaut `2U`.
- Vérifie que `annexe.notes.front().cell` vaut `(GridPosition{1, 0})`.
- Vérifie que `hmi::noteAt(annexe, {.column = 5, .row = 2})` diffère de `nullptr`.
- Vérifie que `hmi::noteAt(annexe, {.column = 5, .row = 2})->text` vaut `"door to the tavern"`.
- Vérifie que `hmi::noteAt(annexe, {.column = 0, .row = 0})` vaut `nullptr`.
- Vérifie que `hmi::sidecarJson(annexe)` vaut `R"({ "notes": [ { "column": 1, "row": 0, "text": "north gate" }, { "column": 5, "row": 2, "text": "door to the tavern" } ], "version": 1 } )"`.
- Vérifie que `hmi::writeSidecar(fichier, annexe)` est vrai.
- Vérifie que `relue.warning.empty()` est vrai.
- Vérifie que `relue.sidecar` vaut `annexe`.
- Vérifie que `hmi::setNote(annexe, {.column = 1, .row = 0}, "")` est vrai.
- Vérifie que `hmi::setNote(annexe, {.column = 5, .row = 2}, "\n")` est vrai.
- Vérifie que `hmi::writeSidecar(fichier, annexe)` est vrai.
- Vérifie que `std::filesystem::exists(fichier)` est faux.

### EditorSidecarTest.LAnnexeTolereEtGardeCeQuElleNeConnaitPas

*Majeur · Unitaire · Notes d'auteur* — `Source/Test/Unit/Editor/test_editor_sidecar.cpp:135`

L'annexe tolère et garde ce qu'elle ne connaît pas.

**Étapes**

1. Lire une annexe avec une clé `lockedRegions` et une note sans texte.
2. Lire un texte qui n'est pas du JSON.
3. Lire un fichier absent.

**Résultat attendu**

- Vérifie que `lue.warning.empty()` est faux.
- Vérifie que `lue.sidecar.notes.size()` vaut `1U`.
- Vérifie que `hmi::sidecarJson(lue.sidecar).find("\"lockedRegions\"")` diffère de `std::string::npos`.
- Vérifie que `illisible.warning.empty()` est faux.
- Vérifie que `illisible.sidecar.empty()` est vrai.
- Vérifie que `absente.warning.empty()` est vrai.
- Vérifie que `absente.sidecar.empty()` est vrai.

### EditorSidecarTest.LAnnexeGardeOuEnEstLaCarte

*Majeur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_editor_sidecar.cpp:167`

L'annexe garde où en est la carte.

**Étapes**

1. Écrire une annexe sans état, puis avec « finished ».
2. Relire une annexe dont l'état est un mot inconnu.

**Résultat attendu**

- Vérifie que `annexe.empty()` est vrai.
- Vérifie que `hmi::sidecarJson(annexe).find("\"state\"")` vaut `std::string::npos`.
- Vérifie que `annexe.empty()` est faux.
- Vérifie que `relue.warning.empty()` est vrai.
- Vérifie que `relue.sidecar.state` vaut `hmi::MapState::Finished`.
- Vérifie que `hmi::parseSidecar(R"({"state": "polished", "version": 1})").sidecar.state` vaut `hmi::MapState::Unset`.
- Vérifie que `hmi::mapStateKey(hmi::MapState::Generated)` vaut `"generated"`.
- Vérifie que `hmi::mapStateFromKey("retouched")` vaut `hmi::MapState::Retouched`.
- Vérifie que `hmi::mapStateLabel(hmi::MapState::Unset)` vaut `"not stated"`.
- Vérifie que `hmi::knownMapStates().size()` vaut `4U`.
- Vérifie que `hmi::mapStateKey(hmi::MapState::Blockout)` vaut `"blockout"`.
- Vérifie que `hmi::mapStateFromKey("blockout")` vaut `hmi::MapState::Blockout`.
- Vérifie que `hmi::mapStateLabel(hmi::MapState::Blockout)` vaut `"Blockout"`.
- Vérifie que `hmi::knownMapStates()[1]` vaut `hmi::MapState::Blockout`.

## test_editor_status.cpp

### EditorStatusTest.AucunNiveauOuvertNAfficheRien

*Critique · Unitaire · Barre d'etat de l'editeur* — `Source/Test/Unit/Editor/test_editor_status.cpp:30`

Aucun niveau ouvert n'affiche aucune zone ni aide.

**Étapes**

1. Construire un contexte sans niveau.
2. Calculer les lignes.

**Résultat attendu**

- Vérifie que `lines.permanent.size()` vaut `hmi::EDITOR_STATUS_ZONE_COUNT`.
- Vérifie que `zone` vaut `""`.
- Vérifie que `lines.help` vaut `""`.

### EditorStatusTest.AucuneCaseSurvoleeLaisseLaZoneVide

*Majeur · Unitaire · Barre d'etat de l'editeur* — `Source/Test/Unit/Editor/test_editor_status.cpp:49`

Aucune case survolee laisse la zone de coordonnees vide.

**Étapes**

1. Construire un niveau sans case survolee.
2. Calculer les lignes.

**Résultat attendu**

- Vérifie que `lines.permanent[0]` vaut `"Map: Salle des epreuves"`.
- Vérifie que `lines.permanent[3]` vaut `""`.

### EditorStatusTest.IndicateurDeModificationSuitLEtatDirty

*Critique · Unitaire · Barre d'etat de l'editeur* — `Source/Test/Unit/Editor/test_editor_status.cpp:68`

L'indicateur de modification suit l'etat "dirty" du niveau.

**Étapes**

1. Calculer les lignes avec dirty=true, puis dirty=false.
2. Comparer la zone d'indicateur.

**Résultat attendu**

- Vérifie que `hmi::editorStatusLines(dirtyContext).permanent[1]` vaut `"Modified"`.
- Vérifie que `hmi::editorStatusLines(cleanContext).permanent[1]` vaut `""`.

### EditorStatusTest.AideChangeAvecLOutilActif

*Critique · Unitaire · Barre d'etat de l'editeur* — `Source/Test/Unit/Editor/test_editor_status.cpp:91`

L'aide contextuelle change avec l'outil actif.

**Étapes**

1. Calculer les lignes avec l'outil Pinceau, puis Entite.
2. Comparer l'aide.

**Résultat attendu**

- Vérifie que `hmi::editorStatusLines(paintContext).help.rfind("Paint:", 0)` vaut `0U`.
- Vérifie que `hmi::editorStatusLines(entityContext).help.rfind("Entity:", 0)` vaut `0U`.

### EditorStatusTest.MemeContexteProduitLaMemeAide

*Majeur · Unitaire · Barre d'etat de l'editeur* — `Source/Test/Unit/Editor/test_editor_status.cpp:115`

La decision est deterministe : meme contexte, meme resultat.

**Étapes**

1. Calculer deux fois les lignes pour le meme contexte.
2. Comparer.

**Résultat attendu**

- Vérifie que `first.help` vaut `second.help`.
- Vérifie que `first.permanent` vaut `second.permanent`.

### EditorStatusTest.LesPiecesEtLaVueSeLisent

*Majeur* — `Source/Test/Unit/Editor/test_editor_status.cpp:135`

La barre d'etat nomme les pieces survolees et la vue du canevas. cat Unitaire · Barre d'etat de l'editeur crit Mineur etapes 1. Survoler une case qui porte un sol et un relief, en vue iso. 2. Passer en vue a plat. attendu Les coordonnees sont suivies des pieces ; la zone du zoom finit par « Iso », puis par « Flat » (LOT-EDITOR-02).

**Résultat attendu**

- Vérifie que `lines.permanent[3]` vaut `"(12, 7) street · wall-left"`.
- Vérifie que `lines.permanent[4]` vaut `"Zoom: 100% · Iso"`.
- Vérifie que `lines.permanent[4]` vaut `"Zoom: 100% · Flat"`.

### EditorStatusTest.LePinceauEtLesCasesForceesSeLisent

*Majeur* — `Source/Test/Unit/Editor/test_editor_status.cpp:163`

La barre d'etat dit le pinceau et les cases forcees. cat Unitaire · Barre d'etat de l'editeur crit Mineur etapes 1. Armer la piece `wall-left`, survoler une case forcee, la collision active. 2. Passer a l'outil Entite. attendu « Brush · wall-left », « · forced collision », et l'aide de la collision ; l'outil Entite ne nomme plus de pinceau.

**Résultat attendu**

- Vérifie que `lines.permanent[2]` vaut `"Brush · wall-left"`.
- Vérifie que `lines.permanent[3]` vaut `"(3, 4) · forced collision"`.
- Vérifie que `lines.help.find("the eraser releases it")` diffère de `std::string::npos`.
- Vérifie que `lines.permanent[2]` vaut `"Entity"`.
- Vérifie que `lines.help.find("the eraser releases it")` vaut `std::string::npos`.

### EditorStatusTest.LeMiroirLaMesureEtLaNoteSeLisent

*Mineur · Unitaire · Barre d'etat de l'editeur* — `Source/Test/Unit/Editor/test_editor_status.cpp:195`

La barre d'etat dit le miroir, la mesure et la note.

**Étapes**

1. Outil Ligne, pinceau `wall-left`, miroir actif, une note sous le curseur.
2. Outil Mesure, une mesure en cours.

**Résultat attendu**

- Vérifie que `lines.permanent[2]` vaut `"Line · wall-left · Mirror"`.
- Vérifie que `lines.permanent[3]` vaut `"(3, 4) · Note: well"`.
- Vérifie que `lines.permanent[2]` vaut `"Measure · Mirror · 7 × 4 · 6 cells = 30 ft"`.
- Vérifie que `lines.help.find("1 cell = 5 ft")` diffère de `std::string::npos`.

## test_entity_editing.cpp

### EditionEntitesTest.LignesDuPanneauCouchesSuiventLOrdreDeDessin

*Majeur · Unitaire · Edition de couches* — `Source/Test/Unit/Editor/test_entity_editing.cpp:75`

Les lignes du panneau Couches suivent l'ordre de dessin.

**Étapes**

1. Construire les lignes d'une carte sans couche visuelle.
2. Puis d'une carte a collision, sol et decor.

**Résultat attendu**

- Vérifie que `hmi::layerRows({layer({}, core::LayerKind::Legacy)})` vaut `(std::vector<hmi::LayerRow>{ {.slot = std::nullopt, .kind = core::LayerKind::Legacy, .name = {}}})`.
- Vérifie que `hmi::layerRows(layers)` vaut `(std::vector<hmi::LayerRow>{ {.slot = std::nullopt, .kind = core::LayerKind::Collision, .name = {}}, {.slot = 1, .kind = core::LayerKind::Ground, .name = "sol"}, {.slot = 2, .kind = core::LayerKind::Decor, .name = "arbres"}})`.

### EditionEntitesTest.CoucheActiveInvalideRetombeSurLaRacine

*Majeur · Unitaire · Edition de couches* — `Source/Test/Unit/Editor/test_entity_editing.cpp:100`

La couche active invalide retombe sur la collision.

**Étapes**

1. Valider le rang 2, puis 5, puis 0 (collision) sur trois couches.

**Résultat attendu**

- Vérifie que `hmi::validActiveLayer(layers, 2)` vaut `hmi::LayerSlot{2}`.
- Vérifie que `hmi::validActiveLayer(layers, 5)` vaut `hmi::LayerSlot{}`.
- Vérifie que `hmi::validActiveLayer(layers, 0)` vaut `hmi::LayerSlot{}`.

### EditionEntitesTest.ReglagesDAffichageDesCouches

*Mineur · Unitaire · Edition de couches* — `Source/Test/Unit/Editor/test_entity_editing.cpp:119`

Les reglages d'affichage des couches.

**Étapes**

1. Lire l'opacite de la racine avec et sans couche visuelle.
2. Regler une opacite hors bornes et une non finie.
3. Masquer le rang 1 puis l'echanger avec le rang 2.

**Résultat attendu**

- Vérifie que `view.display(std::nullopt, true).opacity` vaut `hmi::DEFAULT_COLLISION_OVERLAY_OPACITY` (comparaison flottante).
- Vérifie que `view.display(std::nullopt, false).opacity` vaut `1.0F` (comparaison flottante).
- Vérifie que `view.display(1, true).opacity` vaut `1.0F` (comparaison flottante).
- Vérifie que `view.display(1, true).opacity` vaut `0.25F` (comparaison flottante).
- Vérifie que `view.display(1, true).visible` est vrai.
- Vérifie que `view.display(2, true).visible` est faux.
- Vérifie que `view.display(2, true).opacity` vaut `0.25F` (comparaison flottante).
- Vérifie que `view.display(2, true)` vaut `hmi::LayerDisplay{}`.

### EditionEntitesTest.GesteDeLOutilEntite

*Majeur · Unitaire · Edition d'entites* — `Source/Test/Unit/Editor/test_entity_editing.cpp:154`

Le geste de l'outil Entite.

**Étapes**

1. Appuyer sur la case du coffre et du panneau, avec et sans Ctrl, avec Maj.
2. Appuyer sur une case libre, avec et sans famille choisie.
3. Appuyer hors de la grille.

**Résultat attendu**

- Vérifie que `hmi::resolveEntityPress(map, shared, none, "npc", {})` vaut `(hmi::EntityGestureDecision{.action = hmi::EntityGestureAction::Grab, .entityIndex = 1, .cell = shared, .handle = std::nullopt})`.
- Vérifie que `hmi::resolveEntityPress(map, shared, none, "npc", {.force = true, .toggle = false}).action` vaut `hmi::EntityGestureAction::Place`.
- Vérifie que `hmi::resolveEntityPress(map, shared, none, "", {.force = false, .toggle = true})` vaut `(hmi::EntityGestureDecision{.action = hmi::EntityGestureAction::Toggle, .entityIndex = 1, .cell = shared, .handle = std::nullopt})`.
- Vérifie que `hmi::resolveEntityPress(map, libre, none, "npc", {})` vaut `(hmi::EntityGestureDecision{.action = hmi::EntityGestureAction::Place, .entityIndex = 0, .cell = libre, .handle = std::nullopt})`.
- Vérifie que `hmi::resolveEntityPress(map, libre, none, "", {}).action` vaut `hmi::EntityGestureAction::Deselect`.
- Vérifie que `hmi::resolveEntityPress(map, {.column = 9, .row = 9}, none, "npc", {}).action` vaut `hmi::EntityGestureAction::Ignore`.

### EditionEntitesTest.GlisserDeplaceLEntiteSaisie

*Majeur · Unitaire · Edition d'entites* — `Source/Test/Unit/Editor/test_entity_editing.cpp:196`

Un glisser deplace l'entite saisie, ou le groupe.

**Étapes**

1. Glisser le coffre d'une case, puis sur place.
2. Glisser le coffre et le panneau ensemble, puis hors de la carte.

**Résultat attendu**

- Vérifie que `moved.replaced.size()` vaut `1U`.
- Vérifie que `moved.replaced[0].first` vaut `0U`.
- Vérifie que `moved.replaced[0].second.position` vaut `to`.
- Vérifie que `hmi::dragEntities(one, map.entities(), from, 5, 4).empty()` est vrai.
- Vérifie que `hmi::dragEntities(both, map.entities(), to, 5, 4).replaced.size()` vaut `2U`.
- Vérifie que `refused.refused` est vrai.
- Vérifie que `refused.replaced.empty()` est vrai.

### EditionEntitesTest.ChoixProposesParLePanneau

*Majeur · Unitaire · Edition d'entites* — `Source/Test/Unit/Editor/test_entity_editing.cpp:231`

Les choix proposes par le panneau Entites.

**Étapes**

1. Construire un contexte : une carte « foret » au point « lisiere », la carte editee « village » dont le brouillon pose le point « puits ».
2. Demander les choix du camp d'une entree d'arene, des cartes, et des points d'arrivee vers le village puis sans cible.

**Résultat attendu**

- Vérifie que `arena` diffère de `nullptr`.
- Vérifie que `portal` diffère de `nullptr`.
- Vérifie que `hmi::entityChoices(*arena->find("side"), gate, context)` vaut `(std::vector<std::string>{"allies", "enemies"})`.
- Vérifie que `hmi::entityChoices(*portal->find("targetMap"), gate, context)` vaut `(std::vector<std::string>{"foret", "village"})`.
- Vérifie que `hmi::entityChoices(*portal->find("arrival"), gate, context).empty()` est vrai.
- Vérifie que `hmi::entityChoices(*portal->find("arrival"), gate, context)` vaut `(std::vector<std::string>{"puits"})`.

### EditionEntitesTest.LesCataloguesAlimententLEditeur

*Majeur · Unitaire · Edition d'entites* — `Source/Test/Unit/Editor/test_entity_editing.cpp:276`

Les catalogues alimentent l'editeur.

**Étapes**

1. Lire les references de la racine d essai.
2. Valider les entites des deux cartes.

**Résultat attendu**

- Vérifie que `std::ranges::find(references.dialogues, "garde-du-bourg")` diffère de `references.dialogues.end()`.
- Vérifie que `references.encounters.find("rats-du-donjon")` diffère de `nullptr`.
- Vérifie que `references.world.find("donjon")` diffère de `nullptr`.
- Vérifie que `std::ranges::binary_search(references.figures, "figurant")` est vrai.
- Vérifie que `std::ranges::binary_search(references.figures, "Monsters/sentinelle")` est vrai.
- Vérifie que `references.locations.empty()` est faux.
- Vérifie que `references.items.empty()` est faux.
- Vérifie que `map.ok()` est vrai.
- Vérifie que `context.entityRefs.contains(std::string{mapId} + "#e1")` est vrai.

### EditionEntitesTest.AvertissementsDeLEditeur

*Majeur · Unitaire · Edition d'entites* — `Source/Test/Unit/Editor/test_entity_editing.cpp:313`

Les avertissements de l'editeur.

**Étapes**

1. Traduire un dialogue inconnu, un combattant dans un mur et une zone trop etroite.

**Résultat attendu**

- Vérifie que `lines.size()` vaut `3U`.
- Vérifie que `lines[0]` vaut `(hmi::EditorDiagnostic{.kind = hmi::EditorDiagnosticKind::Reference, .entityIndex = 0, .cell = {.column = 1, .row = 1}, .message = "npc: dialogue \"absent\" does not " "exist, or was rejected when loading."})`.
- Vérifie que `lines[1].message` vaut `"Encounter \"colisee-fauves\": \"rat\" would stand on an obstacle."`.
- Vérifie que `lines[1].cell` vaut `(core::GridPosition{.column = 4, .row = 2})`.
- Vérifie que `lines[2].message` vaut `"Encounter \"colisee-fauves\": area too narrow to fight in (5 free cells, 24 " "required)."`.

## test_entity_shapes.cpp

### EntitesAFormeTest.LaFormeVientDeLaTable

*Majeur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:66`

Les formes et leurs poignees viennent de la table.

**Étapes**

1. Lire la forme d'une zone de combat, d'un ilot, d'une zone, d'un trajet, d'un coffre et d'une famille inconnue.
2. Lister les poignees d'un rectangle 5 x 4, d'un rectangle 2 x 1, d'un trajet a trois points, d'un coffre.

**Résultat attendu**

- Vérifie que `shape("combatZone")` vaut `core::EntityShape::Rectangle`.
- Vérifie que `shape("cityBlock")` vaut `core::EntityShape::Rectangle`.
- Vérifie que `shape("zone")` vaut `core::EntityShape::Area`.
- Vérifie que `shape("route")` vaut `core::EntityShape::Path`.
- Vérifie que `shape("chest")` vaut `core::EntityShape::Point`.
- Vérifie que `shape("dragon")` vaut `core::EntityShape::Point`.
- Vérifie que `handles.size()` vaut `8U`.
- Vérifie que `hmi::handleAt(big, {.column = 6, .row = 6})->kind` vaut `hmi::HandleKind::SouthEast`.
- Vérifie que `hmi::handleAt(big, {.column = 4, .row = 3})->kind` vaut `hmi::HandleKind::North`.
- Vérifie que `hmi::handleAt(big, {.column = 4, .row = 4})` est faux.
- Vérifie que `hmi::entityHandles(rectangleEntity("cityBlock", {}, 2, 1)).size()` vaut `4U`.
- Vérifie que `hmi::entityHandles( route({{.column = 1, .row = 1}, {.column = 4, .row = 1}, {.column = 4, .row = 5}})) .size()` vaut `3U`.
- Vérifie que `hmi::entityHandles(core::MapEntity{.type = "chest", .position = {}}).empty()` est vrai.

### EntitesAFormeTest.PoigneesDUnRectangle

*Majeur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:105`

Redimensionner un rectangle par ses poignees.

**Étapes**

1. Tirer le coin bas-droit d'un rectangle (2, 3) 5 x 4 en (8, 9).
2. Tirer le cote nord en (4, 1).
3. Tirer le cote ouest au-dela du cote est, en (9, 4).

**Résultat attendu**

- Vérifie que `hmi::resizeRectangle(rect, hmi::HandleKind::SouthEast, {.column = 8, .row = 9})` vaut `(hmi::CellRect{.origin = {.column = 2, .row = 3}, .columns = 7, .rows = 7})`.
- Vérifie que `hmi::resizeRectangle(rect, hmi::HandleKind::North, {.column = 4, .row = 1})` vaut `(hmi::CellRect{.origin = {.column = 2, .row = 1}, .columns = 5, .rows = 6})`.
- Vérifie que `hmi::resizeRectangle(rect, hmi::HandleKind::West, {.column = 9, .row = 4})` vaut `(hmi::CellRect{.origin = {.column = 6, .row = 3}, .columns = 4, .rows = 4})`.
- Vérifie que `hmi::rectangleBetween({.column = 5, .row = 1}, {.column = 2, .row = 4})` vaut `(hmi::CellRect{.origin = {.column = 2, .row = 1}, .columns = 4, .rows = 4})`.

### EntitesAFormeTest.UneZoneSePeint

*Majeur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:128`

Peindre une zone de regles case par case.

**Étapes**

1. Ajouter la case (3, 0) a une zone rectangle (1, 0) 2 x 1.
2. En retirer (1, 0), sa case.
3. Retirer toutes ses cases.

**Résultat attendu**

- Vérifie que `zone.cells` vaut `(std::vector<GridPosition>{ {.column = 1, .row = 0}, {.column = 2, .row = 0}, {.column = 3, .row = 0}})`.
- Vérifie que `zone.properties.contains("width")` est faux.
- Vérifie que `zone.properties.contains("height")` est faux.
- Vérifie que `hmi::entityRectangle(zone)` est faux.
- Vérifie que `zone.cells.size()` vaut `2U`.
- Vérifie que `zone.position` vaut `(GridPosition{.column = 2, .row = 0})`.
- Vérifie que `core::zoneCells(zone)` vaut `zone.cells`.
- Vérifie que `kept.cells` vaut `zone.cells`.

### EntitesAFormeTest.UnTrajetSeTrace

*Majeur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:161`

Tracer un trajet point par point.

**Étapes**

1. Prolonger un trajet d'un point, puis du meme.
2. Deplacer son premier point.
3. Retirer des points jusqu'au dernier.

**Résultat attendu**

- Vérifie que `path.cells.size()` vaut `3U`.
- Vérifie que `path.position` vaut `(GridPosition{.column = 0, .row = 2})`.
- Vérifie que `path.cells.front()` vaut `path.position`.
- Vérifie que `path.position` vaut `(GridPosition{.column = 4, .row = 1})`.
- Vérifie que `path.cells` vaut `(std::vector<GridPosition>{{.column = 4, .row = 1}})`.

### EntitesAFormeTest.LEntiteSousLeCurseur

*Majeur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:189`

L'entite sous le curseur.

**Étapes**

1. Poser un ilot 10 x 10, une zone 3 x 3 dedans, un coffre dans la zone.
2. Designer la case du coffre, une case de la zone, une case de l'ilot seul, le coin de la zone selectionnee.

**Résultat attendu**

- Vérifie que `hmi::pickEntity(entities, {.column = 5, .row = 5}, {})` vaut `(hmi::EntityPick{.index = 2, .handle = std::nullopt, .body = false})`.
- Vérifie que `hmi::pickEntity(entities, {.column = 6, .row = 6}, {})` vaut `(hmi::EntityPick{.index = 1, .handle = std::nullopt, .body = true})`.
- Vérifie que `hmi::pickEntity(entities, {.column = 8, .row = 8}, {})` vaut `(hmi::EntityPick{.index = 0, .handle = std::nullopt, .body = true})`.
- Vérifie que `corner` est vrai.
- Vérifie que `corner->index` vaut `1U`.
- Vérifie que `corner->handle` est vrai.
- Vérifie que `corner->handle->kind` vaut `hmi::HandleKind::NorthWest`.
- Vérifie que `hmi::pickEntity(entities, {.column = 12, .row = 12}, {})` est faux.

### EntitesAFormeTest.TirerUneZoneOuUnTrajet

*Majeur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:221`

Tirer une zone, un trajet.

**Étapes**

1. Appuyer avec la famille zone de combat, puis tirer de (1, 1) a (3, 2).
2. Tirer un trajet de (0, 0) a (4, 3), puis sans bouger.
3. Appuyer dans le corps d'une zone avec la famille coffre.

**Résultat attendu**

- Vérifie que `map.placeEntity(rectangleEntity("zone", {.column = 4, .row = 2}, 3, 3))` vaut `0U`.
- Vérifie que `hmi::resolveEntityPress(map, {.column = 1, .row = 1}, {}, "combatZone", {}).action` vaut `hmi::EntityGestureAction::Draw`.
- Vérifie que `hmi::resolveEntityPress(map, {.column = 5, .row = 3}, {}, "chest", {}).action` vaut `hmi::EntityGestureAction::Place`.
- Vérifie que `zone.placed` est vrai.
- Vérifie que `hmi::entityRectangle(*zone.placed)` vaut `(hmi::CellRect{.origin = {.column = 1, .row = 1}, .columns = 3, .rows = 2})`.
- Vérifie que `hmi::dragEntities(drawRoute, map.entities(), {.column = 4, .row = 3}, 8, 6) .placed->cells.size()` vaut `2U`.
- Vérifie que `hmi::dragEntities(drawRoute, map.entities(), {.column = 0, .row = 0}, 8, 6) .placed->cells.empty()` est vrai.

### EntitesAFormeTest.RemplacerGardeLIdentifiant

*Majeur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:262`

Remplacer une entite garde son identifiant.

**Étapes**

1. Poser un trajet, le remplacer par une valeur au type et a l'identifiant changes.
2. Le remplacer par la meme valeur ; par une valeur dont un point sort de la carte.
3. Annuler.

**Résultat attendu**

- Vérifie que `map.placeEntity(original)` vaut `0U`.
- Vérifie que `map.replaceEntity(0, changed)` est vrai.
- Vérifie que `map.entities()[0].type` vaut `core::ROUTE_ENTITY_TYPE`.
- Vérifie que `map.entities()[0].id` vaut `id`.
- Vérifie que `map.entities()[0].cells.size()` vaut `3U`.
- Vérifie que `map.replaceEntity(0, map.entities()[0])` est faux.
- Vérifie que `map.replaceEntity(0, hmi::withWaypointAdded(map.entities()[0], {.column = 9, .row = 1}))` est faux.
- Vérifie que `map.entities()[0].cells` vaut `original.cells`.

### EntitesAFormeTest.ListeFiltrableEtSelection

*Mineur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:294`

La liste filtrable des entites.

**Étapes**

1. Filtrer trois entites par « PORTAL », par « place », par « e2 », par rien.
2. Basculer le rang 2 dans une selection {0}, puis l'en retirer.

**Résultat attendu**

- Vérifie que `hmi::filterEntities(entities, "PORTAL")` vaut `(std::vector<std::size_t>{1})`.
- Vérifie que `hmi::filterEntities(entities, "place")` vaut `(std::vector<std::size_t>{1})`.
- Vérifie que `hmi::filterEntities(entities, "e2")` vaut `(std::vector<std::size_t>{1})`.
- Vérifie que `hmi::filterEntities(entities, "").size()` vaut `3U`.
- Vérifie que `hmi::entityLabel(entities[1])` vaut `"bourg/place"`.
- Vérifie que `hmi::toggledSelection({0}, 2)` vaut `(std::vector<std::size_t>{0, 2})`.
- Vérifie que `hmi::toggledSelection({0, 2}, 2)` vaut `(std::vector<std::size_t>{0})`.

### EntitesAFormeTest.GesteDeLOutilForme

*Majeur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:325`

Le geste de l'outil Forme.

**Étapes**

1. Appuyer sur une zone, avec et sans Ctrl.
2. Sur un trajet : sur un point, avec Ctrl, ailleurs.
3. Sur une zone de combat.

**Résultat attendu**

- Vérifie que `hmi::resolveShapePress(zone, {}, false).action` vaut `hmi::ShapeGestureAction::PaintCells`.
- Vérifie que `hmi::resolveShapePress(zone, {}, true).action` vaut `hmi::ShapeGestureAction::EraseCells`.
- Vérifie que `hmi::resolveShapePress(path, {.column = 3, .row = 1}, false)` vaut `(hmi::ShapeGestureDecision{.action = hmi::ShapeGestureAction::GrabWaypoint, .waypoint = 1})`.
- Vérifie que `hmi::resolveShapePress(path, {.column = 3, .row = 1}, true).action` vaut `hmi::ShapeGestureAction::RemoveWaypoint`.
- Vérifie que `hmi::resolveShapePress(path, {.column = 5, .row = 5}, false).action` vaut `hmi::ShapeGestureAction::AppendWaypoint`.
- Vérifie que `hmi::resolveShapePress(rectangleEntity("combatZone", {}, 3, 3), {}, false).action` vaut `hmi::ShapeGestureAction::Ignore`.

### EntitesAFormeTest.LaFormationParSesFigurines

*Mineur · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:353`

La formation par ses figurines.

**Étapes**

1. Une formation d'un lion et d'un rat ; l'atelier n'a que le lion.

**Résultat attendu**

- Vérifie que `figures.size()` vaut `1U`.
- Vérifie que `figures[0].figure` vaut `"Monsters/lion"`.
- Vérifie que `figures[0].point.x` vaut `2.5F` (comparaison flottante).
- Vérifie que `figures[0].point.y` vaut `3.5F` (comparaison flottante).

### EntitesAFormeTest.AcceptationRedimensionnerLaZoneDuColisee

*Bloquant · Unitaire · Entites a forme* — `Source/Test/Unit/Editor/test_entity_shapes.cpp:376`

Tirer la zone du Colisee change son verdict.

**Étapes**

1. Ouvrir le Colisee livre ; lire le verdict de sa zone « sable ».
2. Prendre sa poignee est et la tirer de la colonne 29 a la colonne 20 ; ecrire le glisser.
3. Relire le verdict et les avertissements ; annuler.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `zoneIndex` est strictement inférieur à `map.entities().size()`.
- Vérifie que `before.issue` est faux.
- Vérifie que `before.zone.columns` vaut `20`.
- Vérifie que `before.entriesInside.size()` vaut `8U`.
- Vérifie que `before.entriesOutside.empty()` est vrai.
- Vérifie que `countIssues(hmi::editorDiagnostics(map.entities(), {}, {}, {before}), "outside")` vaut `0U`.
- Vérifie que `press.action` vaut `hmi::EntityGestureAction::Grab`.
- Vérifie que `press.handle` est vrai.
- Vérifie que `press.handle->kind` vaut `hmi::HandleKind::East`.
- Vérifie que `result.replaced.size()` vaut `1U`.
- Vérifie que `map.replaceEntity(index, entity)` est vrai.
- Vérifie que `map.undoDepth()` vaut `steps + 1`.
- Vérifie que `after.issue` est faux.
- Vérifie que `after.zone.columns` vaut `11`.
- Vérifie que `after.zone.rows` vaut `14`.
- Vérifie que `after.entriesInside.size()` vaut `4U`.
- Vérifie que `after.entriesOutside.size()` vaut `4U`.
- Vérifie que `countIssues(lines, "outside every combat zone")` vaut `4U`.
- Vérifie que `hmi::combatZoneSummary(after).find("salle: 11 x 14")` diffère de `std::string::npos`.
- Vérifie que `verdict().zone.columns` vaut `20`.

## test_game_launch.cpp

### GameLaunch.LIdentifiantDonneLeChemin

*Critique · Unitaire · Essai complet* — `Source/Test/Unit/Editor/test_game_launch.cpp:40`

L'identifiant d'une carte donne son chemin dans le dossier d'essai.

**Étapes**

1. Ecrire le brouillon de `bourg/place` dans un dossier d'essai neuf.

**Résultat attendu**

- Vérifie que `erreur.empty()` est vrai.
- Vérifie que `std::filesystem::exists(fichier)` est vrai.
- Vérifie que `contenu` vaut `"{\"a\":1}"`.

### GameLaunch.LeDossierNeGardeRienDeLEssaiPrecedent

*Majeur · Unitaire · Essai complet* — `Source/Test/Unit/Editor/test_game_launch.cpp:61`

Le dossier d'essai ne garde rien de l'essai precedent.

**Étapes**

1. Ecrire deux brouillons, puis un seul.

**Résultat attendu**

- Vérifie que `hmi::writeDraftMaps(dir, {hmi::DraftMap{.mapId = "bourg/place", .json = "{}"}, hmi::DraftMap{.mapId = "donjon", .json = "{}"}}) .empty()` est vrai.
- Vérifie que `std::filesystem::exists(dir / "donjon.json")` est vrai.
- Vérifie que `hmi::writeDraftMaps(dir, {hmi::DraftMap{.mapId = "bourg/place", .json = "{}"}}) .empty()` est vrai.
- Vérifie que `std::filesystem::exists(dir / "bourg" / "place.json")` est vrai.
- Vérifie que `std::filesystem::exists(dir / "donjon.json")` est faux.

### GameLaunch.SansBrouillonLeDossierEstVide

*Mineur · Unitaire · Essai complet* — `Source/Test/Unit/Editor/test_game_launch.cpp:83`

Sans brouillon, le dossier d'essai est vide et l'ecriture reussit.

**Étapes**

1. Ecrire une liste de brouillons vide.

**Résultat attendu**

- Vérifie que `hmi::writeDraftMaps(dir, {}).empty()` est vrai.
- Vérifie que `std::filesystem::exists(dir / "bourg")` est faux.

### GameLaunch.LeDossierDEssaiEstTemporaire

*Majeur · Unitaire · Essai complet* — `Source/Test/Unit/Editor/test_game_launch.cpp:97`

Le dossier d'essai vit hors du depot.

**Étapes**

1. Demander le dossier de l'essai complet.

**Résultat attendu**

- Vérifie que `essai.filename()` vaut `std::filesystem::path{"JustAnotherRpgGame-playtest"}`.
- Vérifie que `std::filesystem::equivalent(essai.parent_path(), std::filesystem::temp_directory_path())` est vrai.

### GameLaunch.SansJeuAcoteLeCheminEstVide

*Majeur · Unitaire · Essai complet* — `Source/Test/Unit/Editor/test_game_launch.cpp:117`

Sans jeu construit a cote, le chemin rendu est vide.

**Étapes**

1. Chercher le jeu dans un dossier qui ne le contient pas.

**Résultat attendu**

- Vérifie que `hmi::gameExecutable(dir).empty()` est vrai.

### GameLaunch.LeJeuEstCherchePresDeLEditeur

*Majeur · Unitaire · Essai complet* — `Source/Test/Unit/Editor/test_game_launch.cpp:131`

Le jeu est cherche a cote de l'editeur.

**Étapes**

1. Poser un exécutable du nom du jeu dans un dossier, puis l'y chercher.

**Résultat attendu**

- Vérifie que `hmi::gameExecutable(dir)` vaut `jeu`.

## test_gesture_script.cpp

### GestureScriptTest.UnScenarioParOutilRendLeFichierAttendu

*Bloquant · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_gesture_script.cpp:119`

Un scénario --apply par outil rend le fichier attendu.

**Étapes**

1. Pour chacun des onze outils, rejouer `Fixtures/Gestures/<outil>.json` sur `terrain.json`.

**Résultat attendu**

- Vérifie que `rendu.script.ok()` est vrai.
- Vérifie que `rendu.script.gestures` vaut `scenario.gestes`.
- Vérifie que `rendu.script.steps` vaut `scenario.pas`.
- Vérifie que `rendu.mapText` vaut `lire(gestures() / (std::string{scenario.outil} + ".attendu.json"))`.

### GestureScriptTest.LaMesureEtLesNotesRendentLeurCompteRendu

*Majeur · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_gesture_script.cpp:146`

La mesure et les notes rendent leur compte rendu et leur annexe.

**Étapes**

1. Rejouer `measure.json` puis `note.json`.

**Résultat attendu**

- Vérifie que `mesure.script.ok()` est vrai.
- Vérifie que `mesure.script.log.size()` vaut `1U`.
- Vérifie que `mesure.script.log.front().find("7 × 4 · 6 cells = 30 ft")` diffère de `std::string::npos`.
- Vérifie que `mesure.sidecar.has_value()` est faux.
- Vérifie que `notes.script.ok()` est vrai.
- Vérifie que `notes.sidecar.has_value()` est vrai.
- Vérifie que `hmi::sidecarJson(*notes.sidecar)` vaut `lire(gestures() / "note.attendu.editor.json")`.

### GestureScriptTest.UneRueRefaiteRendLaCarteALOctet

*Critique · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_gesture_script.cpp:180`

Une rue refaite par --apply rend la carte, octet pour octet.

**Étapes**

1. Rejouer `Fixtures/Gestures/rue.json` sur `bourg/place`.

**Résultat attendu**

- Vérifie que `rendu.script.ok()` est vrai.
- Vérifie que `rendu.mapId` vaut `"bourg/place"`.
- Vérifie que `rendu.script.gestures` vaut `10U`.
- Vérifie que `rendu.script.steps` vaut `10U`.
- Vérifie que `rendu.mapText` vaut `lire(dataRoot() / "Levels" / "bourg" / "place.json")`.

### GestureScriptTest.UnGesteRefuseRendUneErreurLisible

*Majeur · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_gesture_script.cpp:200`

Un geste refusé rend une erreur lisible.

**Étapes**

1. Rejouer un geste sur une couche verrouillée, hors de la carte, avec une pièce inconnue, sur un identifiant absent, un déplacement qui sort une entité, un outil inconnu, un fichier d'un autre format.

**Résultat attendu**

- Vérifie que `message.find(attendu)` diffère de `std::string::npos`.
- Vérifie que `hmi::applyGestureScript(json{{"format", "autre"}}, carte.draft, carte.annexe, carte.lieu) .error.find("not a gesture file")` diffère de `std::string::npos`.

### GestureScriptTest.UnGesteRefuseNeTouchePasAuFichier

*Critique · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_gesture_script.cpp:238`

Un geste refusé ne touche pas au fichier.

**Étapes**

1. `LevelEditor --apply` d'un fichier dont le deuxième geste est refusé, avec `--output`.
2. Le même, avec des notes seulement.

**Résultat attendu**

- Vérifie que `hmi::runMapCommand({"--data", dataRoot().string(), "--apply", gestes.string(), temoin, "--output", sortie.string()}, {}, compteRendu)` vaut `1`.
- Vérifie que `compteRendu.find("nothing written")` diffère de `std::string::npos`.
- Vérifie que `std::filesystem::exists(sortie)` est faux.
- Vérifie que `hmi::runMapCommand( {"--data", dataRoot().string(), "--apply", (gestures() / "note.json").string(), temoin, "--output", sortie.string()}, {}, compteRendu)` vaut `0`.
- Vérifie que `lire(sortie)` vaut `lire(gestures() / "note.attendu.json")`.
- Vérifie que `lire(hmi::sidecarPath(sortie))` vaut `lire(gestures() / "note.attendu.editor.json")`.

## test_level_file_operations.cpp

### LevelFileOps.CreeUnNiveauValide

*Critique · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:49`

Créer un niveau écrit un fichier valide, immédiatement listé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `std::filesystem::exists(result.path)` est vrai.
- Vérifie que `ops.list().size()` vaut `1U`.

### LevelFileOps.UneCarteCreeeAvecUnLieuASesDeuxCouches

*Bloquant · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:68`

Une carte creee avec un lieu a ses deux couches.

**Étapes**

1. Créer une carte 12 × 8 au lieu `bourg`.
2. La relire.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `lu.ok()` est vrai.
- Vérifie que `hmi::scenePlaceOf(*lu.level)` vaut `"bourg"`.
- Vérifie que `sols` vaut `1`.
- Vérifie que `decors` vaut `1`.

### LevelFileOps.UneCarteCreeeSansLieuASesDeuxCouches

*Majeur* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:96`

Une carte creee sans lieu a ses deux couches, et aucun lieu. cat Unitaire · Opérations sur fichiers de niveau crit Bloquant etapes 1. Créer une carte 12 × 8 sans lieu. 2. La relire. attendu Une couche de sol et une de décor ; aucune propriété `scene` ; la case de l'entrée porte un sol.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `lu.ok()` est vrai.
- Vérifie que `hmi::scenePlaceOf(*lu.level).empty()` est vrai.
- Vérifie que `sols` vaut `1`.
- Vérifie que `decors` vaut `1`.
- Vérifie que `instantane.typeAt({.column = 0, .row = 7})` vaut `core::TileType::Dirt`.

### ScenePlacesTest.LesLieuxProposesOntUnManifeste

*Majeur · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:129`

Les lieux proposes ont un manifeste.

**Étapes**

1. Lister les lieux de la racine d'essai.

**Résultat attendu**

- Vérifie que `std::ranges::is_sorted(lieux)` est vrai.
- Vérifie que `std::ranges::find(lieux, "bourg")` diffère de `lieux.end()`.
- Vérifie que `std::ranges::find(lieux, "hameau")` diffère de `lieux.end()`.
- Vérifie que `std::filesystem::is_regular_file(donnees / "Assets" / "Scene" / lieu / "manifest.json")` est vrai.

### LevelFileOps.LaPlusPetiteCarteFaitUneCase

*Majeur · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:151`

La plus petite carte créable est 1×1.

**Étapes**

1. Créer une carte 1×1.
2. Tenter une carte de largeur nulle, puis de hauteur nulle.

**Résultat attendu**

- Vérifie que `single.ok()` est vrai.
- Vérifie que `ops.create("Vide", 0, 1).ok()` est faux.
- Vérifie que `ops.create("Plate", 1, 0).ok()` est faux.
- Vérifie que `ops.list().size()` vaut `1U`.

### LevelFileOps.FichierDeSequenceExcluDeLaListe

*Majeur · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:172`

Un fichier de séquence n'apparaît jamais dans la liste des niveaux.

**Étapes**

1. Créer un niveau valide, puis écrire à côté un fichier `sequence-demo.json` quelconque.
2. Lister le dossier.

**Résultat attendu**

- Vérifie que `ops.create("MonNiveau", 10, 6).ok()` est vrai.
- Vérifie que `listed.size()` vaut `1U`.
- Vérifie que `listed.front().filename().string()` vaut `"MonNiveau.json"`.

### LevelFileOps.RefuseNomInvalideEtCollision

*Critique · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:196`

Un nom invalide ou déjà pris est refusé, sans écraser le niveau existant.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `ops.create("a/b", 10, 6).ok()` est faux.
- Vérifie que `ops.create("Niveau", 10, 6).ok()` est vrai.
- Vérifie que `ops.create("Niveau", 10, 6).ok()` est faux.

### LevelFileOps.RenommeEtDeplaceLeFichier

*Critique · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:213`

Renommer déplace le fichier : l'ancien chemin disparaît, le nouveau existe.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `created.ok()` est vrai.
- Vérifie que `renamed.ok()` est vrai.
- Vérifie que `std::filesystem::exists(created.path)` est faux.
- Vérifie que `std::filesystem::exists(renamed.path)` est vrai.

### LevelFileOps.DupliqueSousUnNomUnique

*Critique · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:233`

Dupliquer deux fois le même niveau produit deux copies distinctes.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `base.ok()` est vrai.
- Vérifie que `first.ok()` est vrai.
- Vérifie que `second.ok()` est vrai.
- Vérifie que `first.path` diffère de `second.path`.
- Vérifie que `ops.list().size()` vaut `3U`.

### LevelFileOps.SupprimeLeFichier

*Critique · Unitaire · Opérations sur fichiers de niveau* — `Source/Test/Unit/Editor/test_level_file_operations.cpp:255`

Supprimer retire effectivement le fichier du disque.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `created.ok()` est vrai.
- Vérifie que `ops.remove(created.path).ok()` est vrai.
- Vérifie que `std::filesystem::exists(created.path)` est faux.

## test_level_name_validation.cpp

### LevelNameValidationTest.NomSimpleValide

*Majeur · Unitaire · Level Name Validation* — `Source/Test/Unit/Editor/test_level_name_validation.cpp:15`

Un nom simple, sans caractère interdit, est valide.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `hmi::isValidLevelName("Niveau 1")` est vrai.
- Vérifie que `hmi::isValidLevelName("Foret enchantee")` est vrai.

### LevelNameValidationTest.NomVideOuEspacesInvalide

*Majeur · Unitaire · Level Name Validation* — `Source/Test/Unit/Editor/test_level_name_validation.cpp:30`

Un nom vide ou composé uniquement d'espaces est invalide.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `hmi::isValidLevelName("")` est faux.
- Vérifie que `hmi::isValidLevelName(" ")` est faux.

### LevelNameValidationTest.CaractereInterditInvalide

*Majeur · Unitaire · Level Name Validation* — `Source/Test/Unit/Editor/test_level_name_validation.cpp:45`

Un nom contenant un caractère interdit par le système de fichiers Windows est invalide.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `hmi::isValidLevelName("Niveau:1")` est faux.
- Vérifie que `hmi::isValidLevelName("Niveau/1")` est faux.
- Vérifie que `hmi::isValidLevelName("Niveau\\1")` est faux.
- Vérifie que `hmi::isValidLevelName("Niveau*1")` est faux.
- Vérifie que `hmi::isValidLevelName("Niveau?1")` est faux.
- Vérifie que `hmi::isValidLevelName("Niveau\"1")` est faux.
- Vérifie que `hmi::isValidLevelName("Niveau<1")` est faux.
- Vérifie que `hmi::isValidLevelName("Niveau>1")` est faux.
- Vérifie que `hmi::isValidLevelName("Niveau|1")` est faux.

### LevelNameValidationTest.NomAccentueValide

*Mineur · Unitaire · Level Name Validation* — `Source/Test/Unit/Editor/test_level_name_validation.cpp:70`

Un nom accentué (Unicode) reste valide.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `hmi::isValidLevelName("Fort\xC3\xA9resse")` est vrai.

### LevelNameValidationTest.TrimRetireLesEspacesDeBord

*Mineur · Unitaire · Level Name Validation* — `Source/Test/Unit/Editor/test_level_name_validation.cpp:84`

trimLevelName retire les espaces de bord sans toucher au contenu.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `hmi::trimLevelName(" Niveau 1 ")` vaut `"Niveau 1"`.
- Vérifie que `hmi::trimLevelName("Niveau 1")` vaut `"Niveau 1"`.
- Vérifie que `hmi::trimLevelName(" ")` vaut `""`.

## test_map_documents.cpp

### CartesEnOnglets.UnOngletDitSaCarteEtSesModifications

*Majeur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_map_documents.cpp:16`

Un onglet dit sa carte et ses modifications.

**Étapes**

1. Demander le libellé d'une carte de sous-dossier, propre puis modifiée.

**Résultat attendu**

- Vérifie que `hmi::documentLabel("bourg/place", false)` vaut `"place"`.
- Vérifie que `hmi::documentLabel("bourg/place", true)` vaut `"place *"`.
- Vérifie que `hmi::documentLabel("donjon", false)` vaut `"donjon"`.
- Vérifie que `hmi::documentLabel("", true)` vaut `"untitled *"`.

### CartesEnOnglets.UneCarteDejaOuverteNeSouvrePasDeuxFois

*Majeur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_map_documents.cpp:32`

Une carte déjà ouverte ne s'ouvre pas deux fois.

**Étapes**

1. Chercher une carte ouverte, une carte absente, puis une carte sans identifiant.

**Résultat attendu**

- Vérifie que `hmi::documentOf(ouvertes, "bourg/place")` vaut `1U`.
- Vérifie que `hmi::documentOf(ouvertes, "bourg/absente").has_value()` est faux.
- Vérifie que `hmi::documentOf(ouvertes, "").has_value()` est faux.
- Vérifie que `hmi::dirtyDocuments(ouvertes)` vaut `(std::vector<std::string>{"bourg/place", ""})`.

### CartesEnOnglets.FermerUnOngletDonneLaMainAuVoisin

*Mineur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_map_documents.cpp:53`

Fermer un onglet donne la main au voisin.

**Étapes**

1. Fermer le premier, puis le dernier, puis le seul onglet de trois, trois, un.

**Résultat attendu**

- Vérifie que `hmi::documentAfterClose(3, 0)` vaut `0U`.
- Vérifie que `hmi::documentAfterClose(3, 1)` vaut `1U`.
- Vérifie que `hmi::documentAfterClose(3, 2)` vaut `1U`.
- Vérifie que `hmi::documentAfterClose(1, 0).has_value()` est faux.
- Vérifie que `hmi::documentAfterClose(3, 7).has_value()` est faux.

## test_map_format.cpp

### MapFormatTest.LaMigrationNeChangePasCeQueLeJeuJoue

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Editor/test_map_format.cpp:161`

La migration ne change pas ce que le jeu joue.

**Étapes**

1. Migrer `format-v3.json` avec la planche d'essai.
2. Relire la v4.

**Résultat attendu**

- Vérifie que `migration.ok()` est vrai.
- Vérifie que `instantane(v4)` vaut `instantane(v3)`.
- Vérifie que `memeGrilleTactique(v4, v3)` est vrai.
- Vérifie que `migration.namedPieces` vaut `3U`.
- Vérifie que `lieu.appearance.has_value()` est vrai.
- Vérifie que `v4.layers()[1].pieceAt(2, 0)` vaut `lieu.appearance->floorPiece(core::TileType::Solid, {.column = 2, .row = 0})`.
- Vérifie que `migration.newIds` vaut `2U`.
- Vérifie que `v4.entities()[1].id` vaut `"e2"`.
- Vérifie que `v4.nextEntityId()` vaut `3`.
- Vérifie que `migration.newForcedCells` vaut `7U`.

### MapFormatTest.LaMigrationEstIdempotente

*Majeur · Unitaire · Format v4* — `Source/Test/Unit/Editor/test_map_format.cpp:195`

La migration est idempotente.

**Étapes**

1. Migrer `format-v3.json`.
2. Relire et migrer encore.

**Résultat attendu**

- Vérifie que `seconde.text` vaut `premiere.text`.
- Vérifie que `seconde.namedPieces + seconde.newIds + seconde.newForcedCells` vaut `0U`.

### MapFormatTest.LesCartesLivreesPassentLeControle

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Editor/test_map_format.cpp:214`

Les cartes livrées passent le contrôle.

**Étapes**

1. Contrôler `Source/Elements`.

**Résultat attendu**

- Vérifie que `constat.severity` diffère de `MapCheckSeverity::Error`.

### MapFormatTest.ChaqueDefautSortEtLeControleEchoue

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Editor/test_map_format.cpp:233`

Chaque défaut sort, et le contrôle échoue.

**Étapes**

1. Écrire une v4 non canonique, sans id, à pièce absente, à collision écartée, avec une hauteur et un portail vers nulle part.
2. Lancer `--check`.

**Résultat attendu**

- Vérifie que `signale(constats, MapCheckSeverity::Error, "not canonical")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "has no id")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "piece missing")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "collision differs")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Warning, "elevation")` est vrai.
- Vérifie que `code.has_value()` est vrai.
- Vérifie que `*code` vaut `1`.
- Vérifie que `sortie.find("does not exist")` diffère de `std::string::npos`.

### MapFormatTest.UnTypeNonCouvertParLeLieuEstSignale

*Majeur* — `Source/Test/Unit/Editor/test_map_format.cpp:273`

Un type absent de la table du lieu est signale par le controle. cat Unitaire · Format v4 crit Majeur etapes 1. Ecrire une carte du lieu d'essai portant une case d'eau, que sa table ne couvre pas. 2. La controler. attendu Un avertissement nomme le type et la table ; aucune erreur ne vise le type.

**Résultat attendu**

- Vérifie que `signale(constats, MapCheckSeverity::Warning, "tile type \"water\" is not covered")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Warning, "shown as a mock-up")` est vrai.
- Vérifie que `signale(constats, MapCheckSeverity::Error, "tile type")` est faux.

### MapFormatTest.MigrerRendUneCarteQueLeControleAccepte

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Editor/test_map_format.cpp:303`

--migrate rend une carte que --check accepte.

**Étapes**

1. Copier `format-v3.json` dans un dossier de données.
2. `--migrate` puis `--check`.

**Résultat attendu**

- Vérifie que `hmi::runMapCommand({"--migrate", "rue", "--data", donnees.racine().string()}, {}, sortie)` vaut `0`.
- Vérifie que `hmi::runMapCommand({"--check", "--data", donnees.racine().string()}, {}, sortie)` vaut `0`.
- Vérifie que `texte.find("\"version\": 4")` diffère de `std::string::npos`.

### MapFormatTest.SansCommandeLEditeurOuvreSaFenetre

*Mineur · Unitaire · Format v4* — `Source/Test/Unit/Editor/test_map_format.cpp:330`

Sans commande, l'éditeur ouvre sa fenêtre.

**Étapes**

1. Passer `--crash-test` seul.

**Résultat attendu**

- Vérifie que `hmi::runMapCommand({"--crash-test"}, {}, sortie).has_value()` est faux.
- Vérifie que `sortie.empty()` est vrai.

### MapFormatTest.ChaqueCarteMigreeSeJoueALIdentique

*Critique · Unitaire · Format v4* — `Source/Test/Unit/Editor/test_map_format.cpp:349`

Chaque carte migrée se joue à l'identique.

**Étapes**

1. Extraire les v3 : `git show 376c541da:Source/Elements/Levels/…` (le dernier commit de main avant ce lot).
2. `JADG_V3_MAPS_DIR=<dossier>` puis lancer ce test.

**Résultat attendu**

- Vérifie que `instantane(v4)` vaut `instantane(v3)`.
- Vérifie que `memeGrilleTactique(v4, v3)` est vrai.
- Vérifie que `v4.entry()` vaut `v3.entry()`.

## test_map_refactor.cpp

### Donnees.RenommerUneCarteLaisseLeControleVert

*Critique · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:129`

Renommer une carte laisse le contrôle vert.

**Étapes**

1. Copier la racine d'essai.
2. Renommer `bourg/place` en `bourg/marche`.
3. Contrôler toutes les cartes.

**Résultat attendu**

- Vérifie que `avant.ok()` est vrai.
- Vérifie que `apres.ok()` est vrai.
- Vérifie que `apres.count(hmi::MapCheckSeverity::Warning)` vaut `avant.count(hmi::MapCheckSeverity::Warning)`.
- Vérifie que `std::filesystem::exists(carte("bourg/place"))` est faux.
- Vérifie que `core::LevelLoader::loadFromFile(carte("bourg/marche")).level->name()` vaut `"map.bourg.marche.name"`.
- Vérifie que `lire(carte("cave")).find(R"("targetMap": "bourg/marche")")` diffère de `std::string::npos`.
- Vérifie que `ville.find("bourg/place")` vaut `std::string::npos`.
- Vérifie que `ville.find(R"("map": "bourg/marche")")` diffère de `std::string::npos`.
- Vérifie que `fr.find("map.bourg.marche.name = La Place")` diffère de `std::string::npos`.
- Vérifie que `fr.find("map.bourg.place.name")` vaut `std::string::npos`.

### Donnees.UneCarteChangeDeDossierSonAnnexeLaSuit

*Majeur · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:163`

Une carte change de dossier, son annexe la suit.

**Étapes**

1. Donner une note d'auteur au Donjon.
2. Le renommer `arenes/donjon`, puis de nouveau `donjon`.

**Résultat attendu**

- Vérifie que `hmi::setNote(notes, {.column = 3, .row = 4}, "loge")` est vrai.
- Vérifie que `hmi::writeSidecar(hmi::sidecarPath(carte("donjon")), notes)` est vrai.
- Vérifie que `std::filesystem::exists(hmi::sidecarPath(carte("donjon")))` est faux.
- Vérifie que `hmi::noteAt(lues.sidecar, {.column = 3, .row = 4})` diffère de `nullptr`.
- Vérifie que `hmi::checkAllMaps(racine).ok()` est vrai.
- Vérifie que `std::filesystem::exists(hmi::sidecarPath(carte("donjon")))` est vrai.
- Vérifie que `std::filesystem::exists(racine / "Levels" / "arenes")` est faux.

### Donnees.UnRenommageImpossibleNEcritRien

*Critique · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:192`

Un renommage impossible n'écrit rien.

**Étapes**

1. Renommer la Place en Cave, puis en `a:b`.
2. Ajouter une carte illisible et renommer la Place.

**Résultat attendu**

- Vérifie que `pris.ok()` est faux.
- Vérifie que `pris.edits.empty()` est vrai.
- Vérifie que `hmi::planRenameMap(racine, "bourg/place", "a:b").ok()` est faux.
- Vérifie que `illisible.ok()` est faux.
- Vérifie que `illisible.error.find("cassee")` diffère de `std::string::npos`.
- Vérifie que `illisible.edits.empty()` est vrai.

### Donnees.RenommerUnPointDArriveeSuitPortailsEtVille

*Critique · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:218`

Renommer un point d'arrivée suit portails et ville.

**Étapes**

1. Renommer le point `porte-est` de la Place, départ de la ville.
2. Renommer son point `cave`, où mène le portail de la Cave.

**Résultat attendu**

- Vérifie que `lire(racine / "World" / "cities" / "bourg.json").find(R"("arrival": "porte-orientale")")` diffère de `std::string::npos`.
- Vérifie que `portails.size()` vaut `1U`.
- Vérifie que `portails.front().mapId` vaut `"cave"`.
- Vérifie que `lire(carte("cave")).find(R"("arrival": "vers-la-place")")` diffère de `std::string::npos`.
- Vérifie que `hmi::planRenameArrival(racine, "bourg/place", "vers-la-place", "porte-orientale") .ok()` est faux.
- Vérifie que `hmi::checkAllMaps(racine).ok()` est vrai.

### Donnees.RenommerUnIdentifiantDEntite

*Majeur · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:248`

Renommer un identifiant d'entité, et ses refus.

**Étapes**

1. Renommer `e6` de la Place en `e1`, `e999`, `a#b`.
2. Le renommer `depart-est`.

**Résultat attendu**

- Vérifie que `hmi::planRenameEntityId(racine, "bourg/place", "e6", "e1").ok()` est faux.
- Vérifie que `hmi::planRenameEntityId(racine, "bourg/place", "e6", "e999").ok()` est faux.
- Vérifie que `hmi::planRenameEntityId(racine, "bourg/place", "e6", "a#b").ok()` est faux.
- Vérifie que `lue.ok()` est vrai.
- Vérifie que `std::ranges::any_of(lue.level->entities(), [](const auto& entity) { return entity.id == "depart-est"; })` est vrai.
- Vérifie que `hmi::checkAllMaps(racine).ok()` est vrai.

### Donnees.QuiCiteUneCarteQuiPoseUnePiece

*Majeur · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:272`

Qui cite une carte, qui pose une pièce.

**Étapes**

1. Demander qui cite la Place.
2. Demander qui pose `wall-corner`.

**Résultat attendu**

- Vérifie que `cite(carteCitee, "portal e2: targetMap")` est vrai.
- Vérifie que `cite(carteCitee, "district test-city-place: map")` est vrai.
- Vérifie que `cite(carteCitee, "guard map")` est vrai.
- Vérifie que `cite(carteCitee, "map.bourg.place.name")` est vrai.
- Vérifie que `std::ranges::any_of(piece, [id](const hmi::Citation& c) { return c.mapId == id; })` est vrai.
- Vérifie que `std::filesystem::exists(carte("bourg/place"))` est vrai.

### Donnees.RemplacerUnePieceSurToutesLesCartes

*Critique · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:299`

Remplacer une pièce sur toutes les cartes.

**Étapes**

1. Remplacer `street` par `wall-left`, puis par `nope`.
2. Remplacer `street-2` par `street-3` partout.

**Résultat attendu**

- Vérifie que `hmi::planReplacePiece(racine, "street", "wall-left", {}).ok()` est faux.
- Vérifie que `hmi::planReplacePiece(racine, "street", "nope", {}).ok()` est faux.
- Vérifie que `hmi::citationsOfPiece(racine, "street-2").empty()` est faux.
- Vérifie que `hmi::citationsOfPiece(racine, "street-2").empty()` est vrai.
- Vérifie que `hmi::checkAllMaps(racine).ok()` est vrai.

### Donnees.UneCarteChangeDePlancheSansEtreRepeinte

*Critique · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:324`

Une carte change de planche sans être repeinte.

**Étapes**

1. Installer une planche `caveau`, copie de la planche commune, où `street-2` devient `paving-2` et `street-3` devient `cobbles` (ancien nom `street-3`).
2. Changer de planche sans table, puis avec la table `street-2 → paving-2`.

**Résultat attendu**

- Vérifie que `at` diffère de `std::string::npos`.
- Vérifie que `sansTable.ok()` est faux.
- Vérifie que `sansTable.error.find("street-2")` diffère de `std::string::npos`.
- Vérifie que `sansTable.error.find("street-3")` vaut `std::string::npos`.
- Vérifie que `lue.ok()` est vrai.
- Vérifie que `apres.ok()` est vrai.
- Vérifie que `hmi::scenePlaceOf(apres.level->layers())` vaut `"caveau"`.
- Vérifie que `apres.level->tileMap().tile(column, row)` vaut `avant.tile(column, row)`.
- Vérifie que `apparence.find(R"("place": "caveau")")` diffère de `std::string::npos`.
- Vérifie que `apparence.find("paving-2")` diffère de `std::string::npos`.
- Vérifie que `hmi::citationsOfPiece(racine, "street-2").size()` vaut `1U`.
- Vérifie que `hmi::citationsOfPiece(racine, "paving-2").empty()` est faux.
- Vérifie que `hmi::citationsOfPiece(racine, "cobbles").empty()` est faux.
- Vérifie que `hmi::checkAllMaps(racine).ok()` est vrai.

### Donnees.UneTableMalFormeeEstRefusee

*Mineur · Unitaire · Renommer et remplacer* — `Source/Test/Unit/Editor/test_map_refactor.cpp:384`

Une table de correspondance mal formée est refusée.

**Étapes**

1. Lire une table sans `format`, puis une pièce sans remplaçante.

**Résultat attendu**

- Vérifie que `hmi::readPieceTable(table).ok()` est faux.
- Vérifie que `vide.ok()` est faux.
- Vérifie que `vide.error.find("table.json")` diffère de `std::string::npos`.

## test_map_render.cpp

### MapRenderTest.UneCarteSeRendSansFenetre

*Majeur · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_map_render.cpp:52`

--render peint une carte hors écran.

**Étapes**

1. Rendre la Place au quart de l'échelle, bandes par défaut.
2. La rendre avec la collision en plus.

**Résultat attendu**

- Vérifie que `carte.ok()` est vrai.
- Vérifie que `lieu.width()` vaut `989`.
- Vérifie que `lieu.height()` vaut `648`.
- Vérifie que `peinte(lieu, options.background)` est strictement supérieur à `1.0 / 5.0`.
- Vérifie que `collision` diffère de `lieu`.

### MapRenderTest.LesBandesSeLisentParLeurNom

*Mineur · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_map_render.cpp:81`

--layers lit les bandes du canevas.

**Étapes**

1. Lire `floors,collision`, puis `floors,toit`.

**Résultat attendu**

- Vérifie que `bands.has_value()` est vrai.
- Vérifie que `*bands` vaut `(hmi::IsoBandOpacity{ .floors = 1.0F, .relief = 0.0F, .figures = 0.0F, .collision = 1.0F})`.
- Vérifie que `hmi::parseRenderLayers("floors,toit").has_value()` est faux.

### MapRenderTest.RenderEcritUneImageParCarte

*Majeur · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_map_render.cpp:98`

--render écrit une image par carte.

**Étapes**

1. `--render bourg/place donjon --scale 0.125 --output <dossier>`.

**Résultat attendu**

- Vérifie que `code.has_value()` est vrai.
- Vérifie que `*code` vaut `0`.
- Vérifie que `std::filesystem::exists(dossier / "bourg-place.png")` est vrai.
- Vérifie que `std::filesystem::exists(dossier / "donjon.png")` est vrai.
- Vérifie que `hmi::runRenderCommand({"--check"}, {}, sortie).has_value()` est faux.

### MapRenderTest.LePlanCoucheLesBlocsEtLegende

*Majeur · Unitaire · Editeur · Sans fenetre* — `Source/Test/Unit/Editor/test_map_render.cpp:126`

--plan couche les blocs et ajoute une legende.

**Étapes**

1. Rendre une carte de maquette, une fois ordinairement, une fois en plan.

**Résultat attendu**

- Vérifie que `carte.ok()` est vrai.
- Vérifie que `plan.isNull()` est faux.
- Vérifie que `plan.size()` vaut `ordinaire.size()`.
- Vérifie que `plan` diffère de `ordinaire`.
- Vérifie que `peinte(plan.copy(coin), options.background)` est strictement supérieur à `peinte(ordinaire.copy(coin), options.background)`.

## test_paint_tools.cpp

### PaintToolsTest.LaLignePoseUneCaseParPas

*Majeur · Unitaire · Outils du peintre* — `Source/Test/Unit/Editor/test_paint_tools.cpp:115`

La ligne pose une case par pas.

**Étapes**

1. Tracer une ligne droite, une diagonale, une pente raide, un point.

**Résultat attendu**

- Vérifie que `hmi::lineCells({.column = 1, .row = 2}, {.column = 4, .row = 2})` vaut `(Cells{{1, 2}, {2, 2}, {3, 2}, {4, 2}})`.
- Vérifie que `hmi::lineCells({.column = 3, .row = 3}, {.column = 1, .row = 1})` vaut `(Cells{{3, 3}, {2, 2}, {1, 1}})`.
- Vérifie que `hmi::lineCells({.column = 0, .row = 0}, {.column = 1, .row = 3})` vaut `(Cells{{0, 0}, {0, 1}, {1, 2}, {1, 3}})`.
- Vérifie que `hmi::lineCells({.column = 5, .row = 5}, {.column = 5, .row = 5})` vaut `(Cells{{5, 5}})`.

### PaintToolsTest.UnTraitEstUnSeulPas

*Critique · Unitaire · Outils du peintre* — `Source/Test/Unit/Editor/test_paint_tools.cpp:135`

Un trait est un seul pas d'annulation.

**Étapes**

1. Sur La carte d'essai, tracer une ligne de `light` sur six cases de rue.
2. Annuler.

**Résultat attendu**

- Vérifie que `result.changed` est vrai.
- Vérifie que `carte.pieceEn(carte.decor, cell.column, cell.row)` vaut `"light"`.
- Vérifie que `carte.draft.undoDepth()` vaut `1U`.
- Vérifie que `carte.draft.undo()` est vrai.
- Vérifie que `carte.draft.toJson()` vaut `carte.fichier`.

### PaintToolsTest.LeSeauRemplitUneRegionEnUnPas

*Critique · Unitaire · Outils du peintre* — `Source/Test/Unit/Editor/test_paint_tools.cpp:160`

Le seau remplit une région, en un pas.

**Étapes**

1. Sur La carte d'essai, verser `square` sur une case de la place, le sol actif… puis `street-3` sur la place.
2. Annuler.

**Résultat attendu**

- Vérifie que `carte.pieceEn(carte.sol, graine.column, graine.row)` vaut `"square"`.
- Vérifie que `static_cast<int>(place.size())` vaut `carres`.
- Vérifie que `result.changed` est vrai.
- Vérifie que `carte.pieceEn(carte.sol, cell.column, cell.row)` vaut `"street-3"`.
- Vérifie que `carte.pieceEn(carte.sol, 0, 3)` vaut `"street"`.
- Vérifie que `collisionSuitLesPieces(carte.draft)` est vrai.
- Vérifie que `carte.draft.undoDepth()` vaut `1U`.
- Vérifie que `carte.draft.undo()` est vrai.
- Vérifie que `carte.draft.toJson()` vaut `carte.fichier`.

### PaintToolsTest.LeSeauEtLesPiecesLarges

*Majeur · Unitaire · Outils du peintre* — `Source/Test/Unit/Editor/test_paint_tools.cpp:199`

Le seau et les pièces larges.

**Étapes**

1. Verser `feature-1` (2 × 1).
2. Poser un étal, chercher la région de sa case.

**Résultat attendu**

- Vérifie que `refus.changed` est faux.
- Vérifie que `refus.refusal.find("single-cell pieces")` diffère de `std::string::npos`.
- Vérifie que `carte.draft.canUndo()` est faux.
- Vérifie que `carte.draft.placePiece(carte.decor, {.column = 20, .row = 16}, "feature-1", TileType::Wall)` est vrai.
- Vérifie que `hmi::floodRegion(carte.draft, carte.decor, {.column = 21, .row = 16})` vaut `(std::vector<GridPosition>{{20, 16}, {21, 16}})`.

### PaintToolsTest.LaPipettePrendCeQuOnVoit

*Critique · Unitaire · Outils du peintre* — `Source/Test/Unit/Editor/test_paint_tools.cpp:224`

La pipette prend ce qu'on voit.

**Étapes**

1. Sur La carte d'essai, piquer un mur, la collision active puis le sol actif, et une case vide.
2. Poser un étal, piquer sa deuxième case.

**Résultat attendu**

- Vérifie que `collision` est vrai.
- Vérifie que `collision->brush.kind` vaut `BrushKind::Type`.
- Vérifie que `collision->brush.type` vaut `TileType::Wall`.
- Vérifie que `collision->layer` est faux.
- Vérifie que `surLeSol` est vrai.
- Vérifie que `surLeSol->brush` vaut `carte.piece("street", true)`.
- Vérifie que `surLeSol->layer` vaut `carte.sol`.
- Vérifie que `devant` est vrai.
- Vérifie que `devant->brush` vaut `carte.piece("wall-right", false)`.
- Vérifie que `devant->layer` vaut `carte.decor`.
- Vérifie que `hmi::pickBrush(carte.draft, carte.decor, {.column = 30, .row = 0}, table)` est faux.
- Vérifie que `hmi::pickBrush(carte.draft, carte.decor, {.column = -1, .row = 0}, table)` est faux.
- Vérifie que `carte.draft.placePiece(carte.decor, {.column = 20, .row = 16}, "feature-1", TileType::Wall)` est vrai.
- Vérifie que `etal` est vrai.
- Vérifie que `etal->brush.piece` vaut `"feature-1"`.

### PaintToolsTest.LeMiroirPoseLaJumelle

*Critique · Unitaire · Outils du peintre* — `Source/Test/Unit/Editor/test_paint_tools.cpp:267`

Le miroir pose la jumelle, de l'autre côté de l'axe.

**Étapes**

1. Refléter des cases, nommer les jumelles.
2. Tracer une ligne de `wall-right`, le miroir actif.
3. Poser un `wall-left` sur l'axe, une façade qui le traverse.

**Résultat attendu**

- Vérifie que `axe.offset` vaut `25`.
- Vérifie que `hmi::mirrorCell(axe, {.column = 26, .row = 5})` vaut `(GridPosition{30, 1})`.
- Vérifie que `hmi::mirrorCell(axe, {.column = 30, .row = 5})` vaut `(GridPosition{30, 5})`.
- Vérifie que `hmi::mirrorPieceName(manifeste, "wall-left")` vaut `"wall-right"`.
- Vérifie que `hmi::mirrorPieceName(manifeste, "wall-right")` vaut `"wall-left"`.
- Vérifie que `hmi::mirrorPieceName(manifeste, "front-left")` vaut `"front-right"`.
- Vérifie que `hmi::mirrorPieceName(manifeste, "light")` vaut `"light"`.
- Vérifie que `hmi::mirrorPieceName(nullptr, "wall-left")` vaut `"wall-left"`.
- Vérifie que `hmi::applyStroke(carte.draft, carte.piece("wall-right", false), std::nullopt, vue, hmi::lineCells({.column = 26, .row = 5}, {.column = 28, .row = 5}), false, carte.contexte(axe)) .changed` est vrai.
- Vérifie que `carte.pieceEn(carte.decor, column, 5)` vaut `"wall-right"`.
- Vérifie que `carte.pieceEn(carte.decor, 30, column - 25)` vaut `"wall-left"`.
- Vérifie que `carte.draft.undoDepth()` vaut `1U`.
- Vérifie que `hmi::applyStroke(carte.draft, carte.piece("wall-corner", false), std::nullopt, vue, {{.column = 30, .row = 5}}, false, carte.contexte(axe)) .changed` est vrai.
- Vérifie que `carte.pieceEn(carte.decor, 30, 5)` vaut `"wall-corner"`.
- Vérifie que `hmi::applyStroke(carte.draft, carte.piece("front-left", false), std::nullopt, vue, {{.column = 24, .row = 0}}, false, carte.contexte(coupe)) .changed` est vrai.
- Vérifie que `carte.pieceEn(carte.decor, 24, 0)` vaut `"front-left"`.
- Vérifie que `carte.pieceEn(carte.decor, 23, 1)` vaut `""`.

### PaintToolsTest.LaMesureEnCasesEtEnPieds

*Mineur · Unitaire · Outils du peintre* — `Source/Test/Unit/Editor/test_paint_tools.cpp:318`

La mesure en cases et en pieds.

**Étapes**

1. Mesurer de (2, 1) à (8, 4), puis une case sur elle-même.

**Résultat attendu**

- Vérifie que `mesure` vaut `(hmi::Measure{.columns = 6, .rows = 3, .cells = 6, .feet = 30})`.
- Vérifie que `hmi::measureLabel(mesure)` vaut `"7 × 4 · 6 cells = 30 ft"`.
- Vérifie que `hmi::measureLabel(hmi::measureBetween({.column = 3, .row = 3}, {.column = 3, .row = 3}))` vaut `"1 × 1 · 0 cells = 0 ft"`.

### PaintToolsTest.UneMaisonEnMoinsDeDixGestes

*Critique · Unitaire · Outils du peintre* — `Source/Test/Unit/Editor/test_paint_tools.cpp:338`

Une maison de la carte d'essai en six gestes.

**Étapes**

1. Sur un terrain vide de la carte d'essai, le décor actif, le miroir par l'angle (30, 5) :
2. rectangle de `street` (25, 0)–(29, 4) ;
3. ligne de `wall-right` (25, 5)–(29, 5) ;
4. pinceau `wall-corner` sur l'angle ;
5. pinceau `door-right` en (27, 5) ;
6. pinceau `doorstep` en (27, 4) ;
7. annuler puis refaire le dernier geste.

**Résultat attendu**

- Vérifie que `carte.pieceEn(carte.sol, column, row)` vaut `""`.
- Vérifie que `carte.pieceEn(carte.decor, column, row)` vaut `""`.
- Vérifie que `hmi::applyRectangleStroke(carte.draft, carte.piece("street", true), active, vue, {.column = 25, .row = 0}, {.column = 29, .row = 4}, miroir) .changed` est vrai.
- Vérifie que `hmi::applyStroke(carte.draft, carte.piece("wall-right", false), active, vue, hmi::lineCells({.column = 25, .row = 5}, {.column = 29, .row = 5}), false, miroir) .changed` est vrai.
- Vérifie que `pinceau(carte.piece("wall-corner", false), {.column = 30, .row = 5})` est vrai.
- Vérifie que `pinceau(carte.piece("door-right", false), {.column = 27, .row = 5})` est vrai.
- Vérifie que `pinceau(carte.piece("doorstep", true), {.column = 27, .row = 4})` est vrai.
- Vérifie que `gestes` est strictement inférieur à `10`.
- Vérifie que `carte.draft.undoDepth()` vaut `static_cast<std::size_t>(gestes)`.
- Vérifie que `carte.pieceEn(carte.sol, column, row)` vaut `seuil ? "doorstep" : "street"`.
- Vérifie que `carte.pieceEn(carte.decor, i, 5)` vaut `porte ? "door-right" : "wall-right"`.
- Vérifie que `carte.pieceEn(carte.decor, 30, i - 25)` vaut `porte ? "door-left" : "wall-left"`.
- Vérifie que `carte.pieceEn(carte.decor, 30, 5)` vaut `"wall-corner"`.
- Vérifie que `carte.draft.tileMap().tile(27, 5)` vaut `TileType::Wall`.
- Vérifie que `carte.draft.tileMap().tile(27, 4)` vaut `TileType::Empty`.
- Vérifie que `carte.draft.forcedCollision().empty()` est vrai.
- Vérifie que `collisionSuitLesPieces(carte.draft)` est vrai.
- Vérifie que `carte.draft.undo()` est vrai.
- Vérifie que `carte.pieceEn(carte.sol, 27, 4)` vaut `"street"`.
- Vérifie que `carte.pieceEn(carte.sol, 29, 2)` vaut `"street"`.
- Vérifie que `carte.draft.redo()` est vrai.
- Vérifie que `carte.pieceEn(carte.sol, 29, 2)` vaut `"doorstep"`.

## test_panel_focus.cpp

### PanelFocusTest.OutilEntiteMetEnAvantLePanneauEntites

*Majeur · Unitaire · Mise en avant des panneaux* — `Source/Test/Unit/Editor/test_panel_focus.cpp:16`

L'outil Entite met en avant le panneau Entites.

**Étapes**

1. Interroger la table pour l'outil Entite.
2. Verifier le panneau retourne.

**Résultat attendu**

- Vérifie que `hmi::panelForTool(hmi::EditorTool::Entity)` vaut `hmi::PanelId::Entities`.

### PanelFocusTest.OutilsSansPanneauDedieNeMettentRienEnAvant

*Majeur · Unitaire · Mise en avant des panneaux* — `Source/Test/Unit/Editor/test_panel_focus.cpp:30`

Les outils sans panneau dedie ne mettent rien en avant.

**Étapes**

1. Interroger la table pour chaque outil sans panneau dedie.
2. Verifier l'absence de resultat.

**Résultat attendu**

- Vérifie que `hmi::panelForTool(tool)` vaut `std::nullopt`.

### PanelFocusTest.AucunDoublonDOutilDansLaTable

*Majeur · Unitaire · Mise en avant des panneaux* — `Source/Test/Unit/Editor/test_panel_focus.cpp:47`

La table ne contient aucun doublon d'outil.

**Étapes**

1. Parcourir toutes les paires d'entrees de la table.
2. Comparer leurs outils.

**Résultat attendu**

- Vérifie que `catalog[i].tool` diffère de `catalog[j].tool`.

## test_piece_catalog.cpp

### PieceCatalogTest.LeCatalogueGroupeParClasseEtGardeLesAbsentes

*Critique · Unitaire · Palette des pièces* — `Source/Test/Unit/Editor/test_piece_catalog.cpp:73`

Le catalogue groupe par classe et garde les absentes.

**Étapes**

1. Construire le catalogue d'une planche de cinq pièces pour une carte qui cite deux pièces inconnues.

**Résultat attendu**

- Vérifie que `catalog.size()` vaut `5U`.
- Vérifie que `catalog[0].label` vaut `"Floors"`.
- Vérifie que `noms(catalog[0])` vaut `(std::vector<std::string>{"street", "street-2"})`.
- Vérifie que `catalog[0].pieces[0].floor` est vrai.
- Vérifie que `catalog[1].label` vaut `"Standing"`.
- Vérifie que `catalog[2].label` vaut `"Wide"`.
- Vérifie que `catalog[2].pieces[0].footprint` vaut `(core::PieceFootprint{.columns = 2, .rows = 1})`.
- Vérifie que `catalog[2].pieces[0].floor` est faux.
- Vérifie que `catalog[3].label` vaut `"Other"`.
- Vérifie que `catalog[4].label` vaut `hmi::MISSING_PIECES_GROUP`.
- Vérifie que `noms(catalog[4])` vaut `(std::vector<std::string>{"cobble", "fountain"})`.
- Vérifie que `catalog[4].pieces[0].missing` est vrai.
- Vérifie que `catalog[4].pieces[0].floor` est vrai.
- Vérifie que `catalog[4].pieces[1].floor` est faux.

### PieceCatalogTest.SansLieuSeulesLesPiecesCiteesRestent

*Majeur · Unitaire · Palette des pièces* — `Source/Test/Unit/Editor/test_piece_catalog.cpp:105`

Sans lieu, seules les pièces citées restent.

**Étapes**

1. Construire le catalogue sans manifeste.

**Résultat attendu**

- Vérifie que `catalog.size()` vaut `1U`.
- Vérifie que `noms(catalog[0])` vaut `(std::vector<std::string>{"cobble", "fountain", "old-wall"})`.
- Vérifie que `hmi::pieceCatalog(nullptr, {}).empty()` est vrai.

### PieceCatalogTest.LaRechercheFiltreParNomEtParClasse

*Majeur · Unitaire · Palette des pièces* — `Source/Test/Unit/Editor/test_piece_catalog.cpp:123`

La recherche filtre par nom et par classe.

**Étapes**

1. Chercher `STREET`, puis `wide`, puis rien.

**Résultat attendu**

- Vérifie que `streets.size()` vaut `1U`.
- Vérifie que `noms(streets[0])` vaut `(std::vector<std::string>{"street", "street-2"})`.
- Vérifie que `wide.size()` vaut `1U`.
- Vérifie que `noms(wide[0])` vaut `(std::vector<std::string>{"stall"})`.
- Vérifie que `hmi::filterPieceCatalog(catalog, "")` vaut `catalog`.
- Vérifie que `hmi::filterPieceCatalog(catalog, "nothing like it").empty()` est vrai.

### PieceCatalogTest.LaBulleDAideDecritLaPiece

*Mineur · Unitaire · Palette des pièces* — `Source/Test/Unit/Editor/test_piece_catalog.cpp:148`

La bulle d'aide décrit la pièce.

**Étapes**

1. Décrire l'étal.

**Résultat attendu**

- Vérifie que `hmi::pieceDescription(catalog[2].pieces[0])` vaut `"stall — wide, 2 × 1, solid"`.

### PieceCatalogTest.UnePieceViseSaCouche

*Majeur · Unitaire · Palette des pièces* — `Source/Test/Unit/Editor/test_piece_catalog.cpp:165`

Une pièce vise sa couche.

**Étapes**

1. Chercher la couche d'un sol, puis d'une pièce debout, puis sans couche de décor.

**Résultat attendu**

- Vérifie que `hmi::pieceTargetLayer(layers, true)` vaut `std::optional<std::size_t>{1}`.
- Vérifie que `hmi::pieceTargetLayer(layers, false)` vaut `std::optional<std::size_t>{2}`.
- Vérifie que `hmi::pieceTargetLayer(solSeul, false)` vaut `std::nullopt`.

### PieceCatalogTest.LeTypeDUnePieceVientDeLaTableDuLieu

*Majeur · Unitaire · Palette des pièces* — `Source/Test/Unit/Editor/test_piece_catalog.cpp:184`

Le type d'une pièce vient de la table du lieu.

**Étapes**

1. Demander le type de `street-2`, de `stall`, d'un sol inconnu, et de `old-wall` (ancien nom).

**Résultat attendu**

- Vérifie que `table.ok()` est vrai.
- Vérifie que `hmi::pieceCellType(&table.appearance, "street-2", true)` vaut `core::TileType::Dirt`.
- Vérifie que `hmi::pieceCellType(&table.appearance, "stall", false)` vaut `core::TileType::Wall`.
- Vérifie que `hmi::pieceCellType(&table.appearance, "cobble", true)` vaut `core::TileType::Empty`.
- Vérifie que `hmi::pieceCellType(&table.appearance, "old-wall", false)` vaut `core::TileType::Solid`.
- Vérifie que `hmi::pieceCellType(nullptr, "street", true)` vaut `core::TileType::Empty`.

## test_scene_painter.cpp

### ScenePainterTest.UneCartePeinteEgaleLeRenduDuJeu

*Bloquant · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_scene_painter.cpp:216`

Le canevas de l'editeur peint une carte comme le jeu la dessine.

**Étapes**

1. Composer la carte d'essai, ses PNJ compris.
2. La rendre hors ecran par le rendu QRhi du jeu, cadree sur trois points (grand- place, coin nord, porte est).
3. La peindre par le peintre QPainter de l'editeur avec la meme camera.

**Résultat attendu**

- Pour chaque cadrage, moins de 2,5 % des pixels different de plus de 48 sur un canal ; l'image est peinte sur plus de la moitie de sa surface.

### ScenePainterTest.LaSecondeCartePeinteEgaleLeRenduDuJeu

*Majeur · Unitaire · Editeur · Canevas* — `Source/Test/Unit/Editor/test_scene_painter.cpp:240`

Le canevas de l'editeur peint la seconde carte comme le jeu la dessine.

**Étapes**

1. Composer le donjon.
2. Le rendre par le jeu et par l'editeur, cadre sur sa porte.

**Résultat attendu**

- Moins de 2,5 % des pixels different au-dela de la tolerance.

### ScenePainterTest.UneCarteSansAucuneImageSeVoitDansLesDeuxRendus

*Bloquant · Unitaire · Rendu de maquette* — `Source/Test/Unit/Editor/test_scene_painter.cpp:303`

Une carte sans aucun fichier d'image se voit, pareillement dans les deux rendus.

**Étapes**

1. Batir en memoire une carte sans lieu : sols, eau, enceinte de murs, quatre entites.
2. La rendre hors ecran par le rendu QRhi du jeu, puis par le peintre de l'editeur.

**Résultat attendu**

- Vérifie que `maquette.place.empty()` est vrai.
- Vérifie que `hmi::parseMaquetteTokenPath(path).has_value()` est vrai.

## test_shipped_maps.cpp

### ShippedMapsTest.ChaqueCarteSOuvreEtSEnregistreALIdentique

*Bloquant · Unitaire · Editeur · Cartes livrées* — `Source/Test/Unit/Editor/test_shipped_maps.cpp:107`

Les cartes livrées se rechargent sans perte.

**Étapes**

1. Pour chaque carte de `Source/Elements/Levels`, l'ouvrir en brouillon avec le manifeste de son lieu.
2. L'enregistrer sans la toucher.

**Résultat attendu**

- Vérifie que `enregistrer(ouvrir(livre).draft)` vaut `livre`.

### ShippedMapsTest.UneRetoucheSEnregistreSeRechargeEtSeDefait

*Bloquant · Unitaire · Editeur · Cartes livrées* — `Source/Test/Unit/Editor/test_shipped_maps.cpp:127`

Une retouche s'enregistre, se recharge et se défait.

**Étapes**

1. Pour chaque carte livrée, gommer sa première pièce dressée par un geste `--apply`.
2. Enregistrer, recharger, réenregistrer.
3. Défaire le geste.

**Résultat attendu**

- Vérifie que `piece.has_value()` est vrai.
- Vérifie que `rejoue.ok()` est vrai.
- Vérifie que `rejoue.steps` vaut `1U`.
- Vérifie que `retouche` diffère de `livre`.
- Vérifie que `gommee` est vrai.
- Vérifie que `enregistrer(relue.draft)` vaut `retouche`.
- Vérifie que `carte.draft.undo()` est vrai.
- Vérifie que `enregistrer(carte.draft)` vaut `livre`.

### DataRootTest.LEditeurOuvreLesDonneesDeLArbreDesSources

*Bloquant · Unitaire · Editeur · Cartes livrées* — `Source/Test/Unit/Editor/test_shipped_maps.cpp:176`

L'éditeur ouvre les données de l'arbre des sources.

**Étapes**

1. Résoudre la racine avec `--data`, puis sans, l'arbre des sources présent.
2. Puis avec un arbre des sources absent ou inconnu.

**Résultat attendu**

- Vérifie que `hmi::resolveDataRoot({"--data", "ailleurs", "--check"}, executable, sources)` vaut `std::filesystem::path{"ailleurs"}`.
- Vérifie que `hmi::resolveDataRoot({"--check"}, executable, sources)` vaut `sources`.
- Vérifie que `hmi::resolveDataRoot({}, executable, sources / "absent")` vaut `executable`.
- Vérifie que `hmi::resolveDataRoot({}, executable, {})` vaut `executable`.
- Vérifie que `hmi::resolveDataRoot({"--data"}, executable, {})` vaut `executable`.

### DataRootTest.UnDossierDeNiveauxVideResteLArbreDesSources

*Bloquant · Unitaire · Editeur · Cartes livrées* — `Source/Test/Unit/Editor/test_shipped_maps.cpp:203`

Un dossier de niveaux vide reste l'arbre des sources.

**Étapes**

1. Batir une racine dont `Levels/` ne porte que son `README.md`.
2. Resoudre la racine sans `--data`.
3. Controler cette racine.

**Résultat attendu**

- Vérifie que `hmi::resolveDataRoot({"--check"}, "bin", racine)` vaut `racine`.
- Vérifie que `code.has_value()` est vrai.
- Vérifie que `*code` vaut `0`.
- Vérifie que `sortie.find("no map under")` diffère de `std::string::npos`.
- Vérifie que `sortie.find("checked 0 maps")` diffère de `std::string::npos`.

## test_stamps.cpp

### StampsTest.LeTamponPrendLaPieceEntiere

*Majeur · Unitaire · Tampons* — `Source/Test/Unit/Editor/test_stamps.cpp:108`

Le tampon prend l'étal entier.

**Étapes**

1. Découper la seule case d'ancrage de l'étal 2 × 1 de la carte d'essai.

**Résultat attendu**

- Vérifie que `stamp.width` vaut `2`.
- Vérifie que `stamp.height` vaut `1`.
- Vérifie que `stamp.place` vaut `"bourg"`.
- Vérifie que `relief.pieces.size()` vaut `1U`.
- Vérifie que `pieceEn(relief, {.column = 0, .row = 0})` vaut `"feature-1"`.
- Vérifie que `couche(stamp, "sol").types.size()` vaut `2U`.

### StampsTest.UnePieceAncreeDehorsNestPasPrise

*Majeur · Unitaire · Tampons* — `Source/Test/Unit/Editor/test_stamps.cpp:130`

Une pièce ancrée dehors n'est pas prise.

**Étapes**

1. Découper la seconde case de l'étal, celle que son emprise couvre sans l'ancrer.

**Résultat attendu**

- Vérifie que `stamp.width` vaut `1`.
- Vérifie que `stamp.height` vaut `1`.
- Vérifie que `couche(stamp, "relief").pieces.empty()` est vrai.

### StampsTest.UnePoseSeDefaitDUnSeulPas

*Critique · Unitaire · Tampons* — `Source/Test/Unit/Editor/test_stamps.cpp:148`

Une pose se défait d'un seul pas.

**Étapes**

1. Découper un morceau de la carte d'essai, le reposer ailleurs, puis annuler.

**Résultat attendu**

- Vérifie que `result.refusal.empty()` est vrai.
- Vérifie que `result.changed` est vrai.
- Vérifie que `draft.undoDepth()` vaut `pas + 1`.
- Vérifie que `draft.layers()[decor(draft)].pieceAt(ailleurs.column, ailleurs.row)` vaut `"feature-1"`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.toJson()` vaut `avant`.

### StampsTest.LeMiroirTransposeLeTampon

*Majeur · Unitaire · Tampons* — `Source/Test/Unit/Editor/test_stamps.cpp:174`

Le miroir transpose le tampon.

**Étapes**

1. Découper la façade `front-left` (1 × 2), la refléter.

**Résultat attendu**

- Vérifie que `stamp.width` vaut `1`.
- Vérifie que `stamp.height` vaut `2`.
- Vérifie que `pieceEn(couche(stamp, "relief"), {.column = 0, .row = 0})` vaut `"front-left"`.
- Vérifie que `mirrored.width` vaut `2`.
- Vérifie que `mirrored.height` vaut `1`.
- Vérifie que `pieceEn(couche(mirrored, "relief"), {.column = 0, .row = 0})` vaut `"front-right"`.
- Vérifie que `hmi::mirrorStamp(mirrored, draft.pieceManifest())` vaut `stamp`.

### StampsTest.UnePoseRefuseeNecritRien

*Majeur · Unitaire · Tampons* — `Source/Test/Unit/Editor/test_stamps.cpp:199`

Une pose refusée n'écrit rien.

**Étapes**

1. Verrouiller le décor, poser. 2. Poser un tampon dont la couche n'existe pas.

**Résultat attendu**

- Vérifie que `locked.changed` est faux.
- Vérifie que `locked.refusal.find("locked")` diffère de `std::string::npos`.
- Vérifie que `absent.changed` est faux.
- Vérifie que `absent.refusal.find("relief")` diffère de `std::string::npos`.
- Vérifie que `dehors.changed` est faux.
- Vérifie que `dehors.refusal.find("outside")` diffère de `std::string::npos`.
- Vérifie que `draft.toJson()` vaut `avant`.

### StampsTest.UnPrefabriqueFaitLAllerRetour

*Majeur · Unitaire · Préfabriqués* — `Source/Test/Unit/Editor/test_stamps.cpp:237`

Un préfabriqué se relit tel qu'il a été écrit.

**Étapes**

1. Écrire un tampon dans une bibliothèque temporaire, le relire.

**Résultat attendu**

- Vérifie que `stamp.entities.size()` vaut `1U`.
- Vérifie que `stamp.entities.front().id.empty()` est vrai.
- Vérifie que `hmi::writePrefab(racine, "bourg", "etal", stamp)` vaut `""`.
- Vérifie que `hmi::prefabNames(racine, "bourg")` vaut `std::vector<std::string>{"etal"}`.
- Vérifie que `error.empty()` est vrai.
- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `*relu` vaut `stamp`.
- Vérifie que `hmi::writePrefab(racine, "bourg", "Étal du marché", stamp).empty()` est faux.
- Vérifie que `hmi::writePrefab(racine, "bourg", "etal-vide", Stamp{}).empty()` est faux.

### DonneesPrefabriques.UnEtalSeReposeAvecSonMarchand

*Critique · Unitaire · Préfabriqués* — `Source/Test/Unit/Editor/test_stamps.cpp:279`

Un étal de la Place se repose sur le Donjon.

**Étapes**

1. Poser un marchand sur l'étal du marché, l'enregistrer comme préfabriqué. 2. Ouvrir le Donjon, poser le préfabriqué. 3. Annuler.

**Résultat attendu**

- Vérifie que `etal.width` vaut `2`.
- Vérifie que `etal.entities.size()` vaut `1U`.
- Vérifie que `hmi::writePrefab(racine, "bourg", "etal-du-marche", etal)` vaut `""`.
- Vérifie que `prefabrique.has_value()` est vrai.
- Vérifie que `pose.refusal.empty()` est vrai.
- Vérifie que `pose.changed` est vrai.
- Vérifie que `cible.undoDepth()` vaut `pas + 1`.
- Vérifie que `cible.layers()[decor(cible)].pieceAt(place.column, place.row)` vaut `"feature-1"`.
- Vérifie que `pose.entities.size()` vaut `1U`.
- Vérifie que `marchand.type` vaut `"npc"`.
- Vérifie que `marchand.position` vaut `place`.
- Vérifie que `marchand.id` vaut `idLibre`.
- Vérifie que `marchand.id` diffère de `etal.entities.front().id`.
- Vérifie que `cible.entities().size()` vaut `entitesAvant + 1`.
- Vérifie que `cible.undo()` est vrai.
- Vérifie que `cible.undoDepth()` vaut `pas`.
- Vérifie que `sansCompteur(cible.toJson())` vaut `sansCompteur(avant)`.
- Vérifie que `cible.entities().size()` vaut `entitesAvant`.
- Vérifie que `cible.nextEntityId()` est strictement supérieur à `entityIdNumberOf(idLibre)`.

### DonneesPrefabriques.LesModelesLivresSeLisent

*Majeur · Unitaire · Modèles de carte* — `Source/Test/Unit/Editor/test_stamps.cpp:345`

La bibliothèque livrée se lit.

**Étapes**

1. Lire les modèles de `Editor/Templates`. 2. Contrôler toute la bibliothèque.

**Résultat attendu**

- Vérifie que `modele.layers.empty()` est faux.
- Vérifie que `modele.width` est strictement supérieur à `0`.
- Vérifie que `modele.height` est strictement supérieur à `0`.
- Vérifie que `std::ranges::any_of( modele.layers, [](const hmi::MapTemplateLayer& couche) { return couche.scene; })` est vrai.
- Vérifie que `identifiants` vaut `(std::vector<std::string>{"arena", "blockout", "interior", "street"})`.

### StampsTest.UnModeleNeNommeAucunePiece

*Mineur · Unitaire · Modèles de carte* — `Source/Test/Unit/Editor/test_stamps.cpp:376`

Un modèle qui nomme une pièce est refusé.

**Étapes**

1. Lire un modèle dont le tampon pose une pièce.

**Résultat attendu**

- Vérifie que `hmi::mapTemplateFromJson(json, error).has_value()` est faux.
- Vérifie que `error.find("piece")` diffère de `std::string::npos`.

## test_thumbnail_geometry.cpp

### ThumbnailGeometryTest.DimensionnementAuxFacteursUsuels

*Critique · Unitaire · Vignettes* — `Source/Test/Unit/Editor/test_thumbnail_geometry.cpp:17`

Le dimensionnement produit la taille en pixels reels attendue.

**Étapes**

1. Calculer la taille en pixels pour une taille logique de 48 aux facteurs 1, 1.25, 1.5 et 2.

**Résultat attendu**

- Vérifie que `hmi::thumbnailPixelSize(48, 1.0)` vaut `48`.
- Vérifie que `hmi::thumbnailPixelSize(48, 1.25)` vaut `60`.
- Vérifie que `hmi::thumbnailPixelSize(48, 1.5)` vaut `72`.
- Vérifie que `hmi::thumbnailPixelSize(48, 2.0)` vaut `96`.

### ThumbnailGeometryTest.FacteurNonEntierNeProduitJamaisZero

*Critique · Unitaire · Vignettes* — `Source/Test/Unit/Editor/test_thumbnail_geometry.cpp:35`

Un facteur non entier ne produit jamais de dimension nulle.

**Étapes**

1. Calculer la taille en pixels pour la plus petite icone (16) a 1.25 et a un facteur tres faible (0.1).

**Résultat attendu**

- Vérifie que `hmi::thumbnailPixelSize(16, 1.25)` est strictement supérieur à `0`.
- Vérifie que `hmi::thumbnailPixelSize(1, 0.1)` est strictement supérieur à `0`.
- Vérifie que `hmi::thumbnailPixelSize(0, 2.0)` est strictement supérieur à `0`.

### ThumbnailGeometryTest.FonctionPure

*Mineur · Unitaire · Vignettes* — `Source/Test/Unit/Editor/test_thumbnail_geometry.cpp:51`

Le dimensionnement est une fonction pure.

**Étapes**

1. Appeler deux fois avec les memes entrees.

**Résultat attendu**

- Vérifie que `hmi::thumbnailPixelSize(32, 1.5)` vaut `hmi::thumbnailPixelSize(32, 1.5)`.

## test_tile_taxonomy.cpp

### TileTaxonomy.ChaqueTypeFigureExactementUneFois

*Majeur · Unitaire · Taxonomie des tuiles* — `Source/Test/Unit/Editor/test_tile_taxonomy.cpp:44`

Chaque type de tuile figure exactement une fois dans la taxonomie.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `types.size()` vaut `TILE_TYPE_COUNT`.
- Vérifie que `unique.size()` vaut `types.size()`.
- Vérifie que `unique.size()` vaut `TILE_TYPE_COUNT`.

### TileTaxonomy.ChaqueEntreeAUnLibelle

*Majeur · Unitaire · Taxonomie des tuiles* — `Source/Test/Unit/Editor/test_tile_taxonomy.cpp:66`

Chaque catégorie, sous-groupe et tuile de la taxonomie porte un libellé non vide.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `category.label.empty()` est faux.
- Vérifie que `entry.label.empty()` est faux.
- Vérifie que `subgroup.label.empty()` est faux.
- Vérifie que `entry.label.empty()` est faux.

## test_world_graph_layout.cpp

### WorldGraphLayout.GrapheVideSansNoeudNiFleche

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:50`

Un graphe vide ne dessine rien.

**Étapes**

1. Disposer un graphe sans carte ni portail.

**Résultat attendu**

- Vérifie que `disposition.nodes.empty()` est vrai.
- Vérifie que `disposition.edges.empty()` est vrai.
- Vérifie que `disposition.circleRadius` vaut `0.0f` (comparaison flottante).

### WorldGraphLayout.UneSeuleCarteAuCentre

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:66`

Une carte seule est au centre.

**Étapes**

1. Disposer un graphe d'une seule carte.

**Résultat attendu**

- Vérifie que `disposition.nodes.size()` vaut `1U`.
- Vérifie que `disposition.nodes[0].center` vaut `core::Vector2(0.0f, 0.0f)`.
- Vérifie que `disposition.nodes[0].name` vaut `"Nom seule"`.
- Vérifie que `disposition.nodes[0].ghost` est faux.

### WorldGraphLayout.CartesSurLeCercleTrieesParIdentifiantPremiereEnHaut

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:86`

Les cartes sont rangees sur le cercle.

**Étapes**

1. Disposer trois cartes donnees dans le desordre.

**Résultat attendu**

- Vérifie que `disposition.nodes.size()` vaut `3U`.
- Vérifie que `disposition.nodes[0].mapId` vaut `"a"`.
- Vérifie que `disposition.nodes[1].mapId` vaut `"b"`.
- Vérifie que `disposition.nodes[2].mapId` vaut `"c"`.
- Vérifie que `noeud.center.length()` vaut `disposition.circleRadius`, à `0.01f` près.
- Vérifie que `disposition.nodes[0].center.x` vaut `0.0f`, à `0.01f` près.
- Vérifie que `disposition.nodes[0].center.y` est strictement inférieur à `0.0f`.
- Vérifie que `disposition.nodes[1].center.x` est strictement supérieur à `0.0f`.

### WorldGraphLayout.RayonPlancherPuisCroissantPourQueLesEtiquettesNeSeChevauchentPas

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:112`

Le rayon du cercle espace les cartes.

**Étapes**

1. Calculer le rayon de 0 a 41 cartes.

**Résultat attendu**

- Vérifie que `hmi::worldGraphCircleRadius(0)` vaut `0.0f` (comparaison flottante).
- Vérifie que `hmi::worldGraphCircleRadius(1)` vaut `0.0f` (comparaison flottante).
- Vérifie que `hmi::worldGraphCircleRadius(2)` vaut `hmi::WORLD_GRAPH_MIN_CIRCLE_RADIUS` (comparaison flottante).
- Vérifie que `corde + 0.01f` est supérieur ou égal à `hmi::WORLD_GRAPH_NODE_SPACING`.
- Vérifie que `hmi::worldGraphCircleRadius(n + 1)` est supérieur ou égal à `rayon`.

### WorldGraphLayout.DispositionDeterministe

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:135`

La disposition est deterministe.

**Étapes**

1. Disposer deux fois le meme graphe, fantome compris.

**Résultat attendu**

- Vérifie que `premiere.nodes.size()` vaut `seconde.nodes.size()`.
- Vérifie que `premiere.nodes[i].mapId` vaut `seconde.nodes[i].mapId`.
- Vérifie que `premiere.nodes[i].center` vaut `seconde.nodes[i].center`.
- Vérifie que `premiere.edges.size()` vaut `seconde.edges.size()`.

### WorldGraphLayout.CarteIllisibleResteUnNoeudMarque

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:158`

Une carte illisible reste un noeud marque.

**Étapes**

1. Disposer une carte en erreur de chargement et une carte saine.

**Résultat attendu**

- Vérifie que `disposition.nodes.size()` vaut `2U`.
- Vérifie que `disposition.nodes[0].unreadable` est vrai.
- Vérifie que `disposition.nodes[0].loadError` vaut `"JSON invalide"`.
- Vérifie que `disposition.nodes[1].unreadable` est faux.

### WorldGraphLayout.CiblesInconnuesRegroupeesEnUnFantomeParIdentifiant

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:177`

Les cibles inconnues deviennent des fantomes.

**Étapes**

1. Disposer quatre portails : deux vers une carte inconnue, deux sans cible.

**Résultat attendu**

- Vérifie que `disposition.nodes.size()` vaut `4U`.
- Vérifie que `disposition.nodes[2].ghost` est vrai.
- Vérifie que `disposition.nodes[2].mapId` vaut `""`.
- Vérifie que `disposition.nodes[3].ghost` est vrai.
- Vérifie que `disposition.nodes[3].mapId` vaut `"perdue"`.
- Vérifie que `disposition.edges.size()` vaut `4U`.
- Vérifie que `disposition.edges[0].to` vaut `3U`.
- Vérifie que `disposition.edges[1].to` vaut `3U`.
- Vérifie que `disposition.edges[2].to` vaut `2U`.
- Vérifie que `disposition.edges[3].to` vaut `2U`.
- Vérifie que `fleche.broken` est vrai.
- Vérifie que `disposition.nodes[3].center.length()` vaut `disposition.circleRadius`, à `0.01f` près.

### WorldGraphLayout.PortailsDUneMemePaireOrdonneeDessinesUneFoisAvecLeurCompte

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:213`

Les portails d'une meme paire font une fleche.

**Étapes**

1. Disposer deux portails de a vers b et un de b vers a.

**Résultat attendu**

- Vérifie que `disposition.edges.size()` vaut `2U`.
- Vérifie que `disposition.edges[0].from` vaut `0U`.
- Vérifie que `disposition.edges[0].to` vaut `1U`.
- Vérifie que `disposition.edges[0].count()` vaut `2U`.
- Vérifie que `disposition.edges[0].portals` vaut `(std::vector<std::size_t>{0, 1})`.
- Vérifie que `disposition.edges[0].broken` est faux.
- Vérifie que `disposition.edges[0].status` vaut `PortalLinkStatus::Resolved`.
- Vérifie que `disposition.edges[1].from` vaut `1U`.
- Vérifie que `disposition.edges[1].to` vaut `0U`.
- Vérifie que `disposition.edges[1].count()` vaut `1U`.

### WorldGraphLayout.FlecheCasseeDesQuUnPortailLEstStatutDuPremierNonResolu

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:240`

Une fleche est cassee des qu'un portail l'est.

**Étapes**

1. Disposer trois portails de a vers b, dont deux non resolus.

**Résultat attendu**

- Vérifie que `disposition.edges.size()` vaut `1U`.
- Vérifie que `disposition.edges[0].broken` est vrai.
- Vérifie que `disposition.edges[0].status` vaut `PortalLinkStatus::UnknownArrival`.
- Vérifie que `disposition.edges[0].count()` vaut `3U`.

### WorldGraphLayout.CibleIllisibleVisePasUnFantomeMaisLaCarte

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:261`

Une cible illisible reste la carte.

**Étapes**

1. Disposer un portail vers une carte en erreur de chargement.

**Résultat attendu**

- Vérifie que `disposition.nodes.size()` vaut `2U`.
- Vérifie que `disposition.edges.size()` vaut `1U`.
- Vérifie que `disposition.edges[0].to` vaut `1U`.
- Vérifie que `disposition.edges[0].broken` est vrai.

### WorldGraphLayout.PortailVersSaPropreCarteEstUneBoucle

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:281`

Un portail vers sa propre carte est une boucle.

**Étapes**

1. Disposer un portail de a vers a.
2. Calculer son trace.

**Résultat attendu**

- Vérifie que `disposition.edges.size()` vaut `1U`.
- Vérifie que `disposition.edges[0].selfLoop` est vrai.
- Vérifie que `trace.loopRadius` est strictement supérieur à `0.0f`.
- Vérifie que `trace.loopCenter.y` est strictement inférieur à `0.0f`.
- Vérifie que `trace.loopCenter.x` est strictement supérieur à `0.0f`.
- Vérifie que `distance(trace.loopCenter, disposition.nodes[0].center)` vaut `hmi::WORLD_GRAPH_NODE_RADIUS`, à `0.01f` près.

### WorldGraphLayout.TraceDUneFlecheDuBordSourceAuBordCible

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:305`

Une fleche va de bord a bord.

**Étapes**

1. Calculer le trace d'une fleche de a vers b.

**Résultat attendu**

- Vérifie que `distance(trace.start, disposition.nodes[0].center)` vaut `hmi::WORLD_GRAPH_NODE_RADIUS`, à `0.01f` près.
- Vérifie que `distance(trace.end, disposition.nodes[1].center)` vaut `hmi::WORLD_GRAPH_NODE_RADIUS`, à `0.01f` près.

### WorldGraphLayout.FlechesOpposeesDecaleesPourNePasSeSuperposer

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:326`

Les fleches opposees se decalent.

**Étapes**

1. Calculer les traces de a vers b et de b vers a.

**Résultat attendu**

- Vérifie que `distance(aller.badge, retour.badge)` est strictement supérieur à `1.0f`.

### WorldGraphLayout.NodeAtTrouveLeNoeudSousLePointeur

*Majeur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:345`

Le pointeur designe le noeud survole.

**Étapes**

1. Designer le centre d'un noeud, puis son bord, puis un point vide.

**Résultat attendu**

- Vérifie que `hmi::nodeAt(disposition, centreB, hmi::WORLD_GRAPH_NODE_RADIUS)` vaut `std::optional<std::size_t>(1)`.
- Vérifie que `hmi::nodeAt(disposition, centreB + core::Vector2(hmi::WORLD_GRAPH_NODE_RADIUS, 0.0f), hmi::WORLD_GRAPH_NODE_RADIUS)` vaut `std::optional<std::size_t>(1)`.
- Vérifie que `hmi::nodeAt(disposition, core::Vector2(0.0f, 0.0f), hmi::WORLD_GRAPH_NODE_RADIUS)` vaut `std::nullopt`.

### WorldGraphLayout.NodeAtDepartageParLePlusPetitIndice

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:368`

Deux noeuds recouverts se departagent.

**Étapes**

1. Designer le point equidistant de deux noeuds, puis un point plus proche du second.

**Résultat attendu**

- Vérifie que `hmi::nodeAt(disposition, core::Vector2(0.0f, 0.0f), 20.0f)` vaut `std::optional<std::size_t>(0)`.
- Vérifie que `hmi::nodeAt(disposition, core::Vector2(4.0f, 0.0f), 20.0f)` vaut `std::optional<std::size_t>(1)`.

### WorldGraphLayout.EdgeAtTrouveLaFlecheEtLaBoucle

*Mineur · Unitaire · Graphe du monde* — `Source/Test/Unit/Editor/test_world_graph_layout.cpp:389`

Le pointeur designe une fleche ou une boucle.

**Étapes**

1. Designer la pastille d'une fleche, celle d'une boucle, puis un point lointain.

**Résultat attendu**

- Vérifie que `hmi::edgeAt(disposition, fleche.badge, 4.0f)` vaut `std::optional<std::size_t>(0)`.
- Vérifie que `hmi::edgeAt(disposition, boucle.badge, 4.0f)` vaut `std::optional<std::size_t>(1)`.
- Vérifie que `hmi::edgeAt(disposition, core::Vector2(500.0f, 500.0f), 4.0f)` vaut `std::nullopt`.

## test_world_links.cpp

### DonneesLiens.RelierDeuxCartesSeTraverseDansLesDeuxSens

*Critique · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_world_links.cpp:148`

Relier deux cartes se traverse dans les deux sens.

**Étapes**

1. Copier la racine d'essai.
2. Relier `bourg/place` et `donjon`.
3. Relire le graphe du monde et contrôler toutes les cartes.

**Résultat attendu**

- Vérifie que `avant.ok()` est vrai.
- Vérifie que `lien.from.arrivalName` vaut `"from-donjon"`.
- Vérifie que `lien.to.arrivalName` vaut `"from-place"`.
- Vérifie que `aller` diffère de `nullptr`.
- Vérifie que `retour` diffère de `nullptr`.
- Vérifie que `aller->status` vaut `core::PortalLinkStatus::Resolved`.
- Vérifie que `retour->status` vaut `core::PortalLinkStatus::Resolved`.
- Vérifie que `aller->arrival` vaut `"from-place"`.
- Vérifie que `retour->arrival` vaut `"from-donjon"`.
- Vérifie que `surLeDonjon.has_value()` est vrai.
- Vérifie que `surLaPlace.has_value()` est vrai.
- Vérifie que `atteignable(carte("donjon"), *surLeDonjon)` est vrai.
- Vérifie que `atteignable(carte("bourg/place"), *surLaPlace)` est vrai.
- Vérifie que `apres.count(hmi::MapCheckSeverity::Error)` vaut `0U`.

### DonneesLiens.UnLienImpossibleNecritRien

*Majeur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_world_links.cpp:193`

Un lien impossible n'écrit rien.

**Étapes**

1. Relier une carte à elle-même.
2. Relier une carte inconnue.

**Résultat attendu**

- Vérifie que `memeCarte.ok()` est faux.
- Vérifie que `memeCarte.edits.empty()` est vrai.
- Vérifie que `memeCarte.error.find("itself")` diffère de `std::string::npos`.
- Vérifie que `inconnue.ok()` est faux.
- Vérifie que `inconnue.edits.empty()` est vrai.
- Vérifie que `inconnue.error.find("cannot be read")` diffère de `std::string::npos`.

### DonneesLiens.DeuxLiensEntreLesMemesCartesSeDistinguent

*Majeur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_world_links.cpp:215`

Deux liens entre les mêmes cartes se distinguent.

**Étapes**

1. Relier deux fois `bourg/place` et `donjon`.

**Résultat attendu**

- Vérifie que `second.from.arrivalName` vaut `"from-donjon-2"`.
- Vérifie que `second.to.arrivalName` vaut `"from-place-2"`.
- Vérifie que `resolus("bourg/place", "donjon")` vaut `2`.
- Vérifie que `resolus("donjon", "bourg/place")` vaut `2`.

### LiensDuMonde.UnPointDarriveeDitDouLonVient

*Mineur · Unitaire · Le monde* — `Source/Test/Unit/Editor/test_world_links.cpp:244`

Un point d'arrivée dit d'où l'on vient.

**Étapes**

1. Demander le nom pour `bourg/place` sans rien de pris, puis avec.

**Résultat attendu**

- Vérifie que `hmi::arrivalNameFrom("bourg/place", {})` vaut `"from-place"`.
- Vérifie que `hmi::arrivalNameFrom("bourg/place", {"from-place"})` vaut `"from-place-2"`.
- Vérifie que `hmi::arrivalNameFrom("donjon", {"from-donjon", "from-donjon-2"})` vaut `"from-donjon-3"`.
