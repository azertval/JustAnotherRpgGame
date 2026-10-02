# Core · Resources

Tests unitaires — **34 cas** (12 bloquants, 6 critiques, 15 majeurs, 1 mineur). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_asset_keys.cpp`](#test-asset-keyscpp) | 9 | - | 6 | 3 | - |
| [`test_mesh_file.cpp`](#test-mesh-filecpp) | 7 | 3 | - | 3 | 1 |
| [`test_scene_piece_manifest.cpp`](#test-scene-piece-manifestcpp) | 3 | 2 | - | 1 | - |
| [`test_scene_place.cpp`](#test-scene-placecpp) | 9 | 3 | - | 6 | - |
| [`test_skeleton.cpp`](#test-skeletoncpp) | 6 | 4 | - | 2 | - |

## test_asset_keys.cpp

### AssetKeyTest.UneCleNEstJamaisUnCheminDeFichier

*Critique · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:40`

Une cle d'asset n'est jamais un chemin de fichier.

**Étapes**

1. Valider des cles bien formees.
2. Valider des chemins et des cles malformees.

**Résultat attendu**

- Vérifie que `core::isValidAssetKey("beast/wolf")` est vrai.
- Vérifie que `core::isValidAssetKey("item/corde-en-chanvre-15-m")` est vrai.
- Vérifie que `core::isValidAssetKey("weapon/epee-longue")` est vrai.
- Vérifie que `core::isValidAssetKey("Assets/Entities/wolf.png")` est faux.
- Vérifie que `core::isValidAssetKey("beast/wolf/token")` est faux.
- Vérifie que `core::isValidAssetKey("beast/wolf.png")` est faux.
- Vérifie que `core::isValidAssetKey("../beast/wolf")` est faux.
- Vérifie que `core::isValidAssetKey("wolf")` est faux.
- Vérifie que `core::isValidAssetKey("Beast/wolf")` est faux.
- Vérifie que `core::isValidAssetKey("beast/")` est faux.
- Vérifie que `core::isValidAssetKey("beast/-wolf")` est faux.
- Vérifie que `core::isValidAssetKey("beast/wolf-")` est faux.
- Vérifie que `core::isValidAssetKey("beast/gris--loup")` est faux.
- Vérifie que `core::isValidAssetKey("")` est faux.

### AssetKeyTest.UneCleSeDecomposeEtSeRecompose

*Majeur · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:72`

Une cle se decompose et se recompose sans perte.

**Étapes**

1. Decomposer une cle valide.
2. La reformer depuis ses deux morceaux.

**Résultat attendu**

- Vérifie que `decomposee.has_value()` est vrai.
- Vérifie que `decomposee->family` vaut `"beast"`.
- Vérifie que `decomposee->id` vaut `"giant-rat"`.
- Vérifie que `core::defaultAssetKeyFor(decomposee->family, decomposee->id)` vaut `"beast/giant-rat"`.
- Vérifie que `core::parseAssetKey("pas une cle").has_value()` est faux.
- Vérifie que `core::defaultAssetKeyFor("beast", "Loup").empty()` est vrai.

### AssetKeyTest.LaTableDesFamillesLivreeSeCharge

*Critique · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:92`

La table des familles livree se charge.

**Étapes**

1. Charger Assets/Entities/families.json.

**Résultat attendu**

- Vérifie que `familles().ok()` est vrai.
- Vérifie que `famille.name.empty()` est faux.
- Vérifie que `famille.width` est strictement supérieur à `0`.
- Vérifie que `famille.height` est strictement supérieur à `0`.
- Vérifie que `famille.catalogues.empty()` est faux.
- Vérifie que `core::isValidAssetKey(famille.name + "/essai")` est vrai.
- Vérifie que `familles().find("beast")` diffère de `nullptr`.
- Vérifie que `familles().find("famille-inexistante")` vaut `nullptr`.

### AssetKeyTest.AucuneCleOrphelineDansLesCataloguesLivres

*Critique · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:116`

Aucune cle d'asset orpheline dans les catalogues livres.

**Étapes**

1. Deriver le manifeste des catalogues reels.
2. Relever les erreurs.

**Résultat attendu**

- Vérifie que `erreurs.empty()` est vrai.
- Vérifie que `attendues.size()` est strictement supérieur à `300U`.

### AssetKeyTest.LeManifesteDeriveNAPasDeDoublon

*Critique · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:135`

Le manifeste derive n'a aucun doublon et chaque cle est bien formee.

**Étapes**

1. Deriver le manifeste.
2. Verifier l'unicite et la syntaxe de chaque cle.

**Résultat attendu**

- Vérifie que `attendues.empty()` est faux.
- Vérifie que `core::isValidAssetKey(attendue.key)` est vrai.
- Vérifie que `familles().find(attendue.family)` diffère de `nullptr`.
- Vérifie que `vues.insert(attendue.key).second` est vrai.

### AssetMarkerTest.UnMarqueurEstDeterministe

*Critique · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:163`

Un marqueur est deterministe.

**Étapes**

1. Peindre deux fois le marqueur d'une meme cle.
2. Peindre celui d'une autre cle.

**Résultat attendu**

- Vérifie que `premier.isEmpty()` est faux.
- Vérifie que `premier.pixels` vaut `second.pixels`.
- Vérifie que `autre.isEmpty()` est faux.
- Vérifie que `premier.pixels` diffère de `autre.pixels`.
- Vérifie que `core::stableAssetHash("beast/wolf")` vaut `core::stableAssetHash("beast/wolf")`.
- Vérifie que `core::stableAssetHash("beast/wolf")` diffère de `core::stableAssetHash("beast/wolt")`.

### AssetMarkerTest.UnMarqueurRespecteSesDimensionsEtSeVoit

*Majeur · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:191`

Un marqueur respecte ses dimensions et se voit comme un marqueur.

**Étapes**

1. Peindre un marqueur aux dimensions d'une famille.
2. Comparer un pixel de la diagonale a un pixel de coin.

**Résultat attendu**

- Vérifie que `marqueur.isEmpty()` est faux.
- Vérifie que `marqueur.width` vaut `128`.
- Vérifie que `marqueur.height` vaut `128`.
- Vérifie que `marqueur.pixels.size()` vaut `128U * 128U`.
- Vérifie que `centre` diffère de `bord`.
- Vérifie que `centre.a` vaut `255`.

### AssetMarkerTest.UneCleMalformeeNeRecoitPasDeMarqueur

*Majeur · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:215`

Une cle malformee ne recoit pas de marqueur.

**Étapes**

1. Demander le marqueur d'une cle malformee, puis de dimensions nulles.

**Résultat attendu**

- Vérifie que `core::assetMarker("Assets/wolf.png", 96, 96).isEmpty()` est vrai.
- Vérifie que `core::assetMarker("beast/wolf", 0, 96).isEmpty()` est vrai.
- Vérifie que `core::assetMarker("beast/wolf", 96, -1).isEmpty()` est vrai.

### AssetMarkerTest.TouteCleAttendueObtientUnMarqueur

*Critique · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_asset_keys.cpp:231`

Toute cle attendue obtient un marqueur.

**Étapes**

1. Deriver le manifeste des catalogues reels.
2. Peindre le marqueur de chaque cle aux dimensions de sa famille.

**Résultat attendu**

- Vérifie que `attendues.empty()` est faux.
- Vérifie que `famille` diffère de `nullptr`.
- Vérifie que `marqueur.isEmpty()` est faux.
- Vérifie que `marqueur.width` vaut `famille->width`.
- Vérifie que `peints` vaut `static_cast<int>(attendues.size())`.

## test_mesh_file.cpp

### MeshFileTest.UnMaillageDuStandardSeLitEnEntier

*Bloquant · Unitaire · Chargeur de maillages* — `Source/Test/Unit/Core/Resources/test_mesh_file.cpp:114`

Un maillage du standard se lit en entier.

**Étapes**

1. Lire `wall.glb`, le bloc de mur de la carte d'essai.
2. Relever ses sommets, ses triangles, sa boîte et sa matière.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `mesh.vertices.size()` vaut `20U`.
- Vérifie que `mesh.triangleCount()` vaut `10U`.
- Vérifie que `mesh.minimum[0]` vaut `-0.75F`, à `TOLERANCE` près.
- Vérifie que `mesh.maximum[0]` vaut `0.75F`, à `TOLERANCE` près.
- Vérifie que `mesh.minimum[1]` vaut `0.0F`, à `TOLERANCE` près.
- Vérifie que `mesh.maximum[1]` vaut `2.36573F`, à `1e-4F` près.
- Vérifie que `mesh.minimum[2]` vaut `-0.75F`, à `TOLERANCE` près.
- Vérifie que `mesh.maximum[2]` vaut `0.75F`, à `TOLERANCE` près.
- Vérifie que `length` vaut `1.0F`, à `TOLERANCE` près.
- Vérifie que `index` est strictement inférieur à `mesh.vertices.size()`.
- Vérifie que `mesh.primitiveCount` vaut `1`.
- Vérifie que `mesh.materialCount` vaut `1`.
- Vérifie que `mesh.skinned` est faux.
- Vérifie que `mesh.imageMimeType` vaut `"image/png"`.
- Vérifie que `mesh.image.size()` est strictement supérieur à `8U`.
- Vérifie que `std::to_integer<int>(mesh.image[1])` vaut `'P'`.
- Vérifie que `mesh.baseColor[3]` vaut `1.0F` (comparaison flottante).

### MeshFileTest.LaTransformationDuNoeudPorteLeMaillage

*Majeur · Unitaire · Chargeur de maillages* — `Source/Test/Unit/Core/Resources/test_mesh_file.cpp:158`

La transformation du noeud porte le maillage.

**Étapes**

1. Bâtir un triangle sans indices ni matière, porté par un nœud translaté de (10, 2, 0) et agrandi deux fois.
2. Le lire.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `read.mesh.vertices.size()` vaut `3U`.
- Vérifie que `read.mesh.triangleCount()` vaut `1U`.
- Vérifie que `read.mesh.vertices[0].position[0]` vaut `10.0F`, à `TOLERANCE` près.
- Vérifie que `read.mesh.vertices[0].position[1]` vaut `2.0F`, à `TOLERANCE` près.
- Vérifie que `read.mesh.vertices[1].position[0]` vaut `12.0F`, à `TOLERANCE` près.
- Vérifie que `read.mesh.vertices[2].position[2]` vaut `2.0F`, à `TOLERANCE` près.
- Vérifie que `read.mesh.maximum[0]` vaut `12.0F`, à `TOLERANCE` près.
- Vérifie que `read.mesh.image.empty()` est vrai.
- Vérifie que `read.mesh.baseColor[0]` vaut `1.0F` (comparaison flottante).
- Vérifie que `read.mesh.materialCount` vaut `1`.

### MeshFileTest.LesNoeudsSEnchainentEtLesNormalesTournent

*Majeur · Unitaire · Chargeur de maillages* — `Source/Test/Unit/Core/Resources/test_mesh_file.cpp:185`

Les noeuds s'enchainent et les normales tournent.

**Étapes**

1. Bâtir un triangle du sol, de normale +Y, porté par un nœud enfant d'un parent tourné d'un quart de tour autour de X.
2. Le lire.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `read.mesh.vertices.size()` vaut `3U`.
- Vérifie que `read.mesh.vertices[0].normal[0]` vaut `0.0F`, à `TOLERANCE` près.
- Vérifie que `read.mesh.vertices[0].normal[1]` vaut `0.0F`, à `TOLERANCE` près.
- Vérifie que `read.mesh.vertices[0].normal[2]` vaut `1.0F`, à `TOLERANCE` près.

### MeshFileTest.CeQuIlNeLitPasEstRefuseEtNomme

*Bloquant · Unitaire · Chargeur de maillages* — `Source/Test/Unit/Core/Resources/test_mesh_file.cpp:218`

Ce que le chargeur ne lit pas est refuse et nomme.

**Étapes**

1. Présenter des octets qui ne sont pas un `.glb`, un glTF 1, un fichier tronqué.
2. Présenter un `.glb` valide qui exige une extension, un accesseur creux, un tampon hors du fichier.

**Résultat attendu**

- Vérifie que `core::readMeshFromGlb(junk).error` vaut `core::MeshFileError::ParseError`.
- Vérifie que `core::readMeshFromGlb({}).error` vaut `core::MeshFileError::ParseError`.
- Vérifie que `core::readMeshFromGlb(glb("{}", {}, 1)).error` vaut `core::MeshFileError::ParseError`.
- Vérifie que `core::readMeshFromGlb(glb("[1, 2]")).error` vaut `core::MeshFileError::ParseError`.
- Vérifie que `core::readMeshFromGlb(truncated).error` vaut `core::MeshFileError::ParseError`.
- Vérifie que `draco.error` vaut `core::MeshFileError::Unsupported`.
- Vérifie que `draco.message.find("KHR_draco_mesh_compression")` diffère de `std::string::npos`.
- Vérifie que `external.error` vaut `core::MeshFileError::Unsupported`.
- Vérifie que `sparse.error` vaut `core::MeshFileError::Unsupported`.

### MeshFileTest.UnFichierIncoherentEstMalForme

*Bloquant · Unitaire · Chargeur de maillages* — `Source/Test/Unit/Core/Resources/test_mesh_file.cpp:268`

Un fichier incoherent est mal forme, sans lecture hors bornes.

**Étapes**

1. Présenter un accesseur qui dépasse sa vue, une vue qui dépasse le bloc binaire, un indice au-delà des sommets, un graphe de nœuds en boucle, une scène sans triangle.

**Résultat attendu**

- Vérifie que `withAccessor(R"({"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3"})", R"({"buffer": 0, "byteLength": 36})") .error` vaut `core::MeshFileError::MalformedStructure`.
- Vérifie que `withAccessor(R"({"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"})", R"({"buffer": 0, "byteLength": 4096})") .error` vaut `core::MeshFileError::MalformedStructure`.
- Vérifie que `withAccessor(R"({"bufferView": 0, "componentType": 5126, "count": 18446744073709551615, "type": "VEC3"})", R"({"buffer": 0, "byteLength": 36})") .error` vaut `core::MeshFileError::MalformedStructure`.
- Vérifie que `beyond.error` vaut `core::MeshFileError::MalformedStructure`.
- Vérifie que `beyond.mesh.vertices.empty()` est vrai.
- Vérifie que `loop.error` vaut `core::MeshFileError::MalformedStructure`.
- Vérifie que `empty.error` vaut `core::MeshFileError::MalformedStructure`.

### MeshFileTest.UnFichierAbsentEstUneErreur

*Majeur · Unitaire · Chargeur de maillages* — `Source/Test/Unit/Core/Resources/test_mesh_file.cpp:339`

Un fichier absent est une erreur, et un maillage se reconnait a son extension.

**Étapes**

1. Lire un fichier qui n'existe pas.
2. Demander si des chemins sont des maillages.

**Résultat attendu**

- Vérifie que `missing.error` vaut `core::MeshFileError::FileNotFound`.
- Vérifie que `missing.message.empty()` est faux.
- Vérifie que `core::isMeshPath("Scene/ilot/wall.glb")` est vrai.
- Vérifie que `core::isMeshPath("WALL.GLB")` est vrai.
- Vérifie que `core::isMeshPath("Scene/ilot/paving.png")` est faux.
- Vérifie que `core::isMeshPath("glb")` est faux.
- Vérifie que `core::isMeshPath("")` est faux.

### MeshFileTest.LesModelesReelsDeLAtelierSeLisent

*Mineur · Unitaire · Chargeur de maillages* — `Source/Test/Unit/Core/Resources/test_mesh_file.cpp:363`

Les modeles reels de l'atelier se lisent.

**Étapes**

1. Lire chaque `.glb` du dossier que nomme la variable `JADG_MESH_SAMPLES`.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `read.mesh.triangleCount()` est strictement supérieur à `0U`.
- Vérifie que `read.mesh.image.empty()` est faux.
- Vérifie que `count` est strictement supérieur à `0U`.

## test_scene_piece_manifest.cpp

### ScenePieceManifestTest.LeManifesteDUnLieuSeLit

*Bloquant · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_scene_piece_manifest.cpp:29`

Le manifeste des pieces d'un lieu se lit dans Core.

**Étapes**

1. Lire le manifeste du lieu d'essai.
2. Chercher une piece de sol, une piece large et son miroir.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `manifest.place()` vaut `"bourg"`.
- Vérifie que `manifest.pieces().empty()` est faux.
- Vérifie que `street` diffère de `nullptr`.
- Vérifie que `street->key` vaut `"scene/bourg/street"`.
- Vérifie que `street->file` vaut `"street.png"`.
- Vérifie que `street->pieceClass` vaut `core::ScenePieceClass::Floor`.
- Vérifie que `street->footprintColumns` vaut `1`.
- Vérifie que `street->footprintRows` vaut `1`.
- Vérifie que `street->width` vaut `68`.
- Vérifie que `street->height` vaut `42`.
- Vérifie que `street->anchorX` vaut `34`.
- Vérifie que `street->anchorY` vaut `0`.
- Vérifie que `street->mirrorOf.empty()` est vrai.
- Vérifie que `front` diffère de `nullptr`.
- Vérifie que `front->pieceClass` vaut `core::ScenePieceClass::Wide`.
- Vérifie que `front->footprintColumns` vaut `2`.
- Vérifie que `front->footprintRows` vaut `1`.
- Vérifie que `front->mirrorOf` vaut `"front-left"`.
- Vérifie que `manifest.find(front->mirrorOf)` diffère de `nullptr`.
- Vérifie que `manifest.find("piece-inconnue")` vaut `nullptr`.

### ScenePieceManifestTest.ChaquePieceDeclareeASonImage

*Bloquant · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_scene_piece_manifest.cpp:73`

Toute piece declaree par un lieu a son image.

**Étapes**

1. Lire les manifestes des deux lieux d'essai.
2. Chercher le fichier de chaque piece dans le dossier du lieu.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `std::filesystem::is_regular_file(sceneDirectory(place) / piece.file)` est vrai.
- Vérifie que `piece.footprintColumns` est supérieur ou égal à `1`.
- Vérifie que `piece.footprintRows` est supérieur ou égal à `1`.

### ScenePieceManifestTest.UneEntreeFautiveNeFaitPasPerdreLesAutres

*Majeur · Unitaire · Assets* — `Source/Test/Unit/Core/Resources/test_scene_piece_manifest.cpp:98`

Le manifeste des pieces tolere une entree fautive sans perdre les autres.

**Étapes**

1. Lire un manifeste dont une entree n'a pas d'image et une autre une classe inconnue.
2. Lire un manifeste sans `textures`, puis un fichier absent.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `read.manifest.pieces().size()` vaut `1U`.
- Vérifie que `piece.name` vaut `"estrade"`.
- Vérifie que `piece.pieceClass` vaut `core::ScenePieceClass::Other`.
- Vérifie que `piece.className` vaut `"podium"`.
- Vérifie que `piece.footprintColumns` vaut `1`.
- Vérifie que `piece.footprintRows` vaut `1`.
- Vérifie que `core::ScenePieceManifest::loadFromString(R"({"version": 1})").error` vaut `core::ScenePieceManifestError::MalformedStructure`.
- Vérifie que `core::ScenePieceManifest::loadFromFile(sceneDirectory("absent") / "manifest.json").error` vaut `core::ScenePieceManifestError::FileNotFound`.

## test_scene_place.cpp

### ScenePlaceTest.LesNiveauxDUneSousZoneVontJusquAuMonde

*Bloquant · Unitaire · Assets · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:44`

Les niveaux d'un lieu vont de la sous-zone au monde.

**Étapes**

1. Demander les niveaux candidats de `central-empire/capital/arenarea/arena-of-fate`.

**Résultat attendu**

- Vérifie que `directoriesOf(levels)` vaut `(std::vector<std::string>{ "Regions/central-empire/capital/arenarea/arena-of-fate/Scene", "Regions/central-empire/capital/arenarea/arena-of-fate/Common/Scene", "Regions/central-empire/capital/arenarea/Scene", "Regions/central-empire/capital/arenarea/Common/Scene", "Regions/central-empire/capital/Scene", "Regions/central-empire/capital/Common/Scene", "Regions/central-empire/Common/Scene", "Common/Terrain", "Common/Nature", "Common/Props", })`.
- Vérifie que `levels.front().label` vaut `"Arena of Fate"`.
- Vérifie que `levels.front().place` vaut `"central-empire/capital/arenarea/arena-of-fate"`.
- Vérifie que `levels[5].label` vaut `"Capital"`.
- Vérifie que `levels[5].place` vaut `"central-empire/capital"`.
- Vérifie que `levels[6].label` vaut `"Central Empire"`.
- Vérifie que `levels.back().label` vaut `"World"`.
- Vérifie que `levels.back().place.empty()` est vrai.

### ScenePlaceTest.UnLieuAPlatEtUnLieuMalForme

*Majeur · Unitaire · Assets · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:81`

Un lieu a plat et un lieu mal forme.

**Étapes**

1. Demander les niveaux de `bourg`, puis de lieux vides ou contenant `..`.

**Résultat attendu**

- Vérifie que `levels.size()` vaut `4U`.
- Vérifie que `levels.front().directory` vaut `"Scene/bourg"`.
- Vérifie que `levels.front().label` vaut `"Bourg"`.
- Vérifie que `core::fallbackScenePiecePath("bourg", "street")` vaut `"Scene/bourg/street.png"`.
- Vérifie que `core::fallbackScenePiecePath(ARENAREA, "stall")` vaut `"Regions/central-empire/capital/arenarea/Scene/stall.png"`.
- Vérifie que `core::isValidScenePlace(bad)` est faux.
- Vérifie que `core::sceneLevelCandidates(bad).empty()` est vrai.
- Vérifie que `core::fallbackScenePiecePath(bad, "x").empty()` est vrai.

### ScenePlaceTest.UnLieuDescendDeSesPrefixes

*Majeur · Unitaire · Assets · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:106`

Un lieu descend de ses prefixes et du monde.

**Étapes**

1. Comparer des lieux a leurs prefixes, et lister la filiation d'une zone.

**Résultat attendu**

- Vérifie que `core::scenePlaceDescendsFrom(ARENAREA, "central-empire/capital")` est vrai.
- Vérifie que `core::scenePlaceDescendsFrom(ARENAREA, ARENAREA)` est vrai.
- Vérifie que `core::scenePlaceDescendsFrom(ARENAREA, "")` est vrai.
- Vérifie que `core::scenePlaceDescendsFrom(ARENAREA, "central-empire/capital/martpart")` est faux.
- Vérifie que `core::scenePlaceDescendsFrom(ARENAREA, "central-empire/cap")` est faux.
- Vérifie que `core::scenePlaceAncestry(ARENAREA)` vaut `(std::vector<std::string>{ARENAREA, "central-empire/capital", "central-empire", ""})`.
- Vérifie que `core::scenePlaceAncestry("bourg")` vaut `(std::vector<std::string>{"bourg", ""})`.

### ScenePlaceTest.LeCatalogueDUnLieuEmpileSesNiveaux

*Bloquant · Unitaire · Assets · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:128`

Le catalogue d'un lieu empile ses niveaux.

**Étapes**

1. Resoudre le lieu `central-empire/capital/arenarea` de la racine LevelTree.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `catalog.place()` vaut `ARENAREA`.
- Vérifie que `catalog.tileWidth()` vaut `68`.
- Vérifie que `piece` diffère de `nullptr`.
- Vérifie que `piece->level` vaut `expected.level`.
- Vérifie que `piece->path()` vaut `expected.path`.
- Vérifie que `std::filesystem::is_regular_file(treeAssets() / piece->path())` est vrai.
- Vérifie que `catalog.find("sand")` vaut `nullptr`.
- Vérifie que `catalog.masked().size()` vaut `1U`.
- Vérifie que `catalog.masked().front().piece.name` vaut `"fountain"`.
- Vérifie que `catalog.masked().front().piece.level` vaut `"Capital"`.
- Vérifie que `catalog.masked().front().by` vaut `"Arenarea"`.
- Vérifie que `std::ranges::count(catalog.pieces(), std::string{"fountain"}, &core::ScenePiece::name)` vaut `1`.
- Vérifie que `levels` vaut `(std::vector<std::string>{"Arenarea", "Capital", "Central Empire", "World", "World"})`.

### ScenePlaceTest.UneZoneSansPieceProprePuiseDansLaVille

*Majeur · Unitaire · Assets · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:189`

Une zone sans piece propre puise dans la ville.

**Étapes**

1. Resoudre `central-empire/capital/martpart`.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `fountain` diffère de `nullptr`.
- Vérifie que `fountain->directory` vaut `"Regions/central-empire/capital/Common/Scene"`.
- Vérifie que `read.manifest.masked().empty()` est vrai.

### ScenePlaceTest.LArbreDesLieuxListeLesZones

*Majeur · Unitaire · Assets · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:208`

L'arbre des lieux liste les zones et sous-zones.

**Étapes**

1. Lister les lieux de la racine LevelTree.

**Résultat attendu**

- Vérifie que `core::scenePlaces(treeAssets())` vaut `(std::vector<std::string>{ARENAREA, "central-empire/capital/arenarea/arena-of-fate", "central-empire/capital/martpart"})`.

### ScenePlaceTest.UnNiveauCommunIllisibleFaitEchouerLaResolution

*Majeur · Unitaire · Assets · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:225`

Un niveau commun illisible fait echouer la resolution.

**Étapes**

1. Ecrire une zone lisible sous une ville dont le manifeste est casse, et la resoudre.
2. Resoudre un lieu qui n'a aucun manifeste.

**Résultat attendu**

- Vérifie que `broken.ok()` est faux.
- Vérifie que `broken.message.find("Regions/r/v/Common/Scene/manifest.json")` diffère de `std::string::npos`.
- Vérifie que `none.error` vaut `core::ScenePieceManifestError::FileNotFound`.

### ScenePlaceTest.LesFiguresDUnLieuViennentDeSesNiveaux

*Bloquant · Unitaire · Assets · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:259`

Les figurines d'un lieu viennent de ses niveaux.

**Étapes**

1. Lister les niveaux de figurines de l'Arenarea.
2. Resoudre ses figurines, puis celles du Martpart.
3. Demander le dossier d'une figurine inconnue, d'un chemin, d'un slug seul.

**Résultat attendu**

- Vérifie que `directories` vaut `(std::vector<std::string>{ "Regions/central-empire/capital/arenarea/Characters", "Regions/central-empire/capital/arenarea/Common/Characters", "Regions/central-empire/capital/Characters", "Regions/central-empire/capital/Common/Characters", "Regions/central-empire/Common/Characters", "Common/Characters", })`.
- Vérifie que `core::characterLevelCandidates("bourg").size()` vaut `1U`.
- Vérifie que `core::figureDirectory(arena, "anariel")` vaut `"Regions/central-empire/capital/arenarea/Characters/anariel"`.
- Vérifie que `core::figureDirectory(arena, "citizen")` vaut `"Regions/central-empire/capital/arenarea/Characters/citizen"`.
- Vérifie que `core::figureDirectory(arena, "Peoples/human/guard")` vaut `"Common/Characters/Peoples/human/guard"`.
- Vérifie que `std::filesystem::is_regular_file(treeAssets() / directory / "character.json")` est vrai.
- Vérifie que `core::figureDirectory(mart, "citizen")` vaut `"Regions/central-empire/capital/Common/Characters/citizen"`.
- Vérifie que `mart.contains("anariel")` est faux.
- Vérifie que `core::figureDirectory({}, "Monsters/lion")` vaut `"Monsters/lion"`.
- Vérifie que `core::figureDirectory({}, "figurant")` vaut `"Npc/figurant"`.
- Vérifie que `core::figureDirectory({}, "")` vaut `""`.

### ScenePlaceTest.UnPortraitDAttenteEstUneFigurineNommee

*Majeur · Unitaire · Arborescence* — `Source/Test/Unit/Core/Resources/test_scene_place.cpp:312`

Un personnage sans modele garde son dossier.

**Étapes**

1. Ecrire le manifeste d'un dossier Characters/ du monde : un modele sous `npcs`, un portrait d'attente sous `portraits`.
2. Resoudre les figurines.

**Résultat attendu**

- Vérifie que `core::figureDirectory(figures, "bandit")` vaut `"Common/Characters/bandit"`.
- Vérifie que `core::figureDirectory(figures, "thug")` vaut `"Common/Characters/thug"`.

## test_skeleton.cpp

### SkeletonTest.LeChargeurLitLeSqueletteEtLesClips

*Bloquant · Unitaire · Maillages · Squelette* — `Source/Test/Unit/Core/Resources/test_skeleton.cpp:75`

Le chargeur lit le squelette et les clips d'un modele lie.

**Étapes**

1. Lire le pantin de la carte d'essai.

**Résultat attendu**

- Vérifie que `mesh.skinned` est vrai.
- Vérifie que `mesh.rig.joints.size()` vaut `3U`.
- Vérifie que `mesh.rig.joints[0].name` vaut `"Root"`.
- Vérifie que `mesh.rig.joints[1].name` vaut `"spine_01"`.
- Vérifie que `mesh.rig.joints[2].name` vaut `"head"`.
- Vérifie que `mesh.rig.joints[0].parent` vaut `-1`.
- Vérifie que `mesh.rig.joints[1].parent` vaut `0`.
- Vérifie que `mesh.rig.joints[2].parent` vaut `1`.
- Vérifie que `mesh.rig.order` vaut `(std::vector<std::uint16_t>{0, 1, 2})`.
- Vérifie que `mesh.rig.joints[2].translation[1]` vaut `0.6F`, à `TOLERANCE` près.
- Vérifie que `mesh.rig.joints[2].inverseBind[13]` vaut `-1.5F`, à `TOLERANCE` près.
- Vérifie que `mesh.skin.size()` vaut `mesh.vertices.size()`.
- Vérifie que `mesh.skin[index].joints[0]` est strictement inférieur à `3U`.
- Vérifie que `mesh.skin[index].weights[0]` vaut `1.0F` (comparaison flottante).
- Vérifie que `mesh.skin[index].joints[0]` vaut `2U`.
- Vérifie que `mesh.skin[index].joints[0]` vaut `0U`.
- Vérifie que `mesh.rig.clips.size()` vaut `6U`.
- Vérifie que `walk` diffère de `nullptr`.
- Vérifie que `walk->duration` vaut `0.5F` (comparaison flottante).
- Vérifie que `walk->tracks.size()` vaut `3U`.
- Vérifie que `walk->tracks[1].rotation.times.size()` vaut `4U`.
- Vérifie que `walk->tracks[0].rotation.empty()` est vrai.
- Vérifie que `death` diffère de `nullptr`.
- Vérifie que `death->duration` vaut `0.8F` (comparaison flottante).
- Vérifie que `death->tracks[0].translation.times.size()` vaut `2U`.
- Vérifie que `core::findClip(mesh.rig, "dance")` vaut `nullptr`.

### SkeletonTest.LaPoseDUnOsAUnInstant

*Bloquant · Unitaire · Maillages · Squelette* — `Source/Test/Unit/Core/Resources/test_skeleton.cpp:131`

La pose d'un os a un instant d'un clip.

**Étapes**

1. Poser le pantin au repos.
2. Le poser a 0,4 s de son attaque (le buste penche de 60 degres autour de la hanche, a 0,9 m), puis a 0,2 s (30 degres).
3. Le poser a la fin de sa chute (la racine basculee de 90 degres en arriere, soulevee de 0,2 m).

**Résultat attendu**

- Au repos, chaque point reste ou il est. A 0,4 s, le sommet du crane (0 ; 1,8 ; 0) est en (0 ; 1,35 ; 0,7794) et les pieds n'ont pas bouge ; a 0,2 s, la rotation est a mi-chemin. A la fin de la chute, le sommet du crane est en (0 ; 0,2 ; -1,8).

### SkeletonTest.LInstantDUnClip

*Majeur · Unitaire · Maillages · Squelette* — `Source/Test/Unit/Core/Resources/test_skeleton.cpp:171`

L'instant d'un clip : ramene s'il boucle, fige sinon.

**Étapes**

1. Demander l'instant d'un clip de 0,5 s a 1,7 s, en boucle puis joue une fois.
2. Demander un instant negatif, et un clip sans duree.

**Résultat attendu**

- Vérifie que `core::clipTime(1.7F, 0.5F, true)` vaut `0.2F`, à `TOLERANCE` près.
- Vérifie que `core::clipTime(1.7F, 0.5F, false)` vaut `0.5F` (comparaison flottante).
- Vérifie que `core::clipTime(-0.1F, 0.5F, true)` vaut `0.4F`, à `TOLERANCE` près.
- Vérifie que `core::clipTime(-0.1F, 0.5F, false)` vaut `0.0F` (comparaison flottante).
- Vérifie que `core::clipTime(3.0F, 0.0F, true)` vaut `0.0F` (comparaison flottante).

### SkeletonTest.LaDescriptionDUnSqueletteSeLit

*Bloquant · Unitaire · Maillages · Squelette* — `Source/Test/Unit/Core/Resources/test_skeleton.cpp:190`

La description d'un squelette se lit, et ses defauts sont dits.

**Étapes**

1. Lire la description du squelette du pantin.
2. Lire cinq descriptions fautives : un os dont le parent n'est pas declare avant lui, un os declare deux fois, un clip sans duree, une image cle hors du clip, une version inconnue.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `read.skeleton.silhouette` vaut `"pantin"`.
- Vérifie que `read.skeleton.bones.size()` vaut `3U`.
- Vérifie que `read.skeleton.bones[2]` vaut `(core::SkeletonBone{.name = "head", .parent = "spine_01"})`.
- Vérifie que `read.skeleton.clips.size()` vaut `6U`.
- Vérifie que `walk` diffère de `nullptr`.
- Vérifie que `walk->loop` est vrai.
- Vérifie que `walk->duration` vaut `0.5F` (comparaison flottante).
- Vérifie que `walk->key.has_value()` est faux.
- Vérifie que `attack` diffère de `nullptr`.
- Vérifie que `attack->loop` est faux.
- Vérifie que `attack->duration` vaut `0.8F` (comparaison flottante).
- Vérifie que `attack->key.has_value()` est vrai.
- Vérifie que `*attack->key` vaut `0.4F` (comparaison flottante).
- Vérifie que `read.skeleton.clip("dance")` vaut `nullptr`.
- Vérifie que `broken(root, R"({"name":"idle","duration":1.0,"loop":true})").ok()` est vrai.
- Vérifie que `broken(R"({"name":"head","parent":"spine"})", "").ok()` est faux.
- Vérifie que `broken(root + "," + root, "").ok()` est faux.
- Vérifie que `broken(root, R"({"name":"idle","loop":true})").ok()` est faux.
- Vérifie que `broken(root, R"({"name":"attack","duration":0.5,"key":0.9})").ok()` est faux.
- Vérifie que `broken(root, "", 2).ok()` est faux.
- Vérifie que `core::readSkeletonDescription("pas du json").ok()` est faux.
- Vérifie que `core::readSkeletonFile(assets() / "absent.json").ok()` est faux.

### SkeletonTest.LaFicheDUnPersonnage

*Majeur · Unitaire · Maillages · Squelette* — `Source/Test/Unit/Core/Resources/test_skeleton.cpp:241`

La fiche d'un personnage nomme son modele et son squelette.

**Étapes**

1. Lire la fiche du pantin.
2. Lire une fiche sans modele, une fiche sans squelette, une fiche dont le modele est un chemin.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `read.sheet` vaut `(core::CharacterSheetFile{.model = "pantin.glb", .skeleton = "pantin"})`.
- Vérifie que `core::readCharacterSheet(R"({"version":1,"skeleton":"humanoid"})").ok()` est faux.
- Vérifie que `core::readCharacterSheet(R"({"version":1,"model":"a.glb"})").ok()` est faux.
- Vérifie que `core::readCharacterSheet(R"({"version":1,"model":"../a.glb","skeleton":"humanoid"})").ok()` est faux.

### SkeletonTest.LeModeleDEssaiPorteLeSqueletteCommun

*Bloquant · Unitaire · Maillages · Squelette* — `Source/Test/Unit/Core/Resources/test_skeleton.cpp:265`

Le modele d'essai porte les 53 os et les six clips du squelette commun.

**Étapes**

1. Lire le mannequin d'essai et la description du squelette humanoide.
2. Comparer os et clips.
3. Poser le modele a huit instants de chaque clip et deformer tous ses sommets.

**Résultat attendu**

- Vérifie que `read.ok()` est vrai.
- Vérifie que `declared.ok()` est vrai.
- Vérifie que `mesh.rig.joints.size()` vaut `53U`.
- Vérifie que `skeleton.bones.size()` vaut `53U`.
- Vérifie que `mesh.skin.size()` vaut `mesh.vertices.size()`.
- Vérifie que `found` diffère de `mesh.rig.joints.end()`.
- Vérifie que `parent` vaut `bone.parent`.
- Vérifie que `skeleton.clips.size()` vaut `6U`.
- Vérifie que `mesh.rig.clips.size()` vaut `6U`.
- Vérifie que `curves` diffère de `nullptr`.
- Vérifie que `curves->duration` vaut `clip.duration`, à `1e-3F` près.
- Vérifie que `std::isfinite(point[0]) && std::isfinite(point[1]) && std::isfinite(point[2])` est vrai.
- Vérifie que `lowest` est strictement supérieur à `-0.005F`.
- Vérifie que `lowest` est strictement inférieur à `0.02F`.
