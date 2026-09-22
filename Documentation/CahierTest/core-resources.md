# Core · Resources

Tests unitaires — **12 cas** (2 bloquants, 6 critiques, 4 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_asset_keys.cpp`](#test-asset-keyscpp) | 9 | - | 6 | 3 | - |
| [`test_scene_piece_manifest.cpp`](#test-scene-piece-manifestcpp) | 3 | 2 | - | 1 | - |

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
