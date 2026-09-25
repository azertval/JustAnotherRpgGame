# Core/Resources/

**Clés d'assets d'entité** et **marqueurs** de substitution, sans dépendance au rendu (`LOT-39`).

- `AssetKey` — comment une donnée désigne son illustration : familles d'assets et contrat de
  dimensions (`AssetFamilyDefinition`, `AssetFamilyTable`, `loadAssetFamilies`), clé d'asset
  (`EX-CNT-040`, `EX-CNT-041`).
- `AssetMarker` — le marqueur qui tient lieu d'illustration tant qu'aucune n'existe : pixels RVBA
  peints depuis la clé (`assetMarker`, `stableAssetHash`), sans Qt ni GPU (`EX-ARCH-001`).

Il n'y a pas de `ResourceManager` unique dans `Core` (`EX-ARCH-080` dans sa formulation
d'origine) : un gestionnaire de textures obligerait `Core` à connaître le GPU (`EX-NFR-010`,
`EX-ARCH-010`). La gestion des ressources vit du côté qui les possède :

- **Textures** → `HMI/Graphics` : décodage et upload (`hmi::TextureLoader`) ; les pixels d'un
  marqueur d'entité (`hmi::EntityMarkers`) deviennent une texture par la bibliothèque du rendu
  qui les dessine (`hmi::WorldSceneRenderer::textures()`).
- **Cartes** → `Core/Levels` : `core::LevelLoader`/`core::LevelWriter`, avec validation
  (`EX-LVL-004`).

Réf. specs : `EX-ARCH-080` (amendée en conséquence).
