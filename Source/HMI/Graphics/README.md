# HMI/Graphics/

Rendu GPU via **QRhi**, la couche d'accès au GPU de Qt, qui retient Direct3D 11 par défaut sous
Windows. Le device, la *swap chain* et la présentation appartiennent à Qt ; ce dossier garde le
pipeline 2D, la caméra et la composition des scènes.

## Pipeline
- `RhiContext` — le `QRhi` courant et le lot de mises à jour de l'image en cours, partagés par tout ce qui crée des textures (`EX-REN-050`).
- `SceneResources` — les ressources QRhi que **toute** surface de rendu du projet possède (`LOT-86`).
- `SpriteBatch` — pipeline 2D : quads texturés (`SpriteQuad`) et segments orientés (`LineQuad`), shaders compilés au build (`Shaders/`), fusion alpha et échantillonnage *nearest* (pixel art) ; `begin`/`draw`/`end` avec *batching*. **Une seule texture liée par `begin`** : le rendu émet une passe par groupe de texture plutôt que de modifier ce contrat.
- `Quad` — primitives de dessin (`SpriteQuad`, `LineQuad`) **sans dépendance GPU** : c'est ce qui permet à la composition d'être testée sans GPU.
- `Camera2D` — projection monde → écran (16 px/unité, Y-bas, zoom) ; matrice de projection, conversions monde↔écran et **cadrage visible** (`visibleBounds`, base du culling).

## Assets et textures
- `TextureLoader` — `decodeImageFile` (décodage `QImage` → RGBA non prémultiplié), `encodeImageFile`, `createTexture`, `loadTextureFromFile` : **point unique** de création de texture GPU (`QRhiTexture`). Jamais d'exception (`EX-NFR-040`).
- `TextureAtlas` — atlas **procédural** de tuiles 16 px, généré en mémoire : une couleur par type de tuile, dont le canevas et la palette de l'éditeur se servent. Aucun fichier n'est lu (`EX-REN-041`/`EX-REN-042`). `tile` est de la pure arithmétique de grille, `static` et testée sans GPU.
- `ProceduralAtlas` — génération CPU déterministe des pixels de cet atlas. Aucune dépendance GPU ni Qt, entièrement testé.
- `EntityMarkers` — le marqueur d'une entité de carte : sa clé d'asset et ses pixels (`LOT-11`, `LOT-39`).
- `AnimationCatalog` — lecture et validation du format `<asset>.anim.json` (bandes d'animation des figurines).
- `AssetContract` — verdict de validation d'un asset contre ses dimensions décodées ; un asset non conforme est refusé avec un message nommant le fichier, le trouvé et l'attendu (`EX-REN-007`).
- `MissingTexture` — damier magenta opaque et déterministe, repli **visible** de toute texture attendue mais absente (`EX-NFR-040`).
- `AssetGallery` / `AssetGalleryRenderer` — la galerie des assets, outil de débug : inventaire et disposition sans GPU, puis rendu QRhi.

## Scènes
- `RenderLayer` — ordonnancement de calques **unique et explicite** du projet (`EX-REN-014`) ; aucun lot ne doit en inventer un concurrent. `TextureHandle` y désigne l'identité **opaque** d'une texture, seule notion dont la composition ait besoin.
- `ComposedScene` — **composition** du rendu : liste ordonnée des primitives d'une image (tri calque → texture → ordre fin, stable ; profondeur quantifiée par le Y du pied), culling par cadrage caméra et compteurs de volume. Logique pure, sans GPU (`LOT-07`, `EX-NFR-004`, `EX-NFR-005`).
- `QuadRecorder` — capture et **inspection** d'une scène composée pour les tests (ordre des calques, contiguïté des groupes de texture, dénombrement, présence d'une primitive). Outil de vérification, jamais un détour du rendu.
- `SpriteRenderer` — **soumet** une scène composée (`submitComposedScene`, une passe `begin/end` par groupe de texture).
- `ScenePieces` — géométrie des pièces de l'atelier des textures (`LOT-92`), commune au Colisée en combat et aux lieux qu'on parcourt.
- `PlaceAppearance` — ce qu'un **lieu** met sur une case : la table qui traduit un type de tuile en pièce de sa planche (`Scene/<lieu>/appearance.json`, `LOT-92`, `LOT-09`).
- `WorldSceneComposer` / `WorldSceneRenderer` — un lieu qu'on parcourt, composé sans GPU puis rendu en QRhi (`LOT-09`) ; dessiné pour le jeu comme pour l'essai de l'éditeur.
- `StaticWorldScene` — un lieu composé **une fois**, indexé par une grille de seaux, puis découpé à la vue à chaque image ; les figurines s'y fusionnent et les étages s'effacent devant le héros. Le coût d'une image dépend de ce qu'on voit, pas de la taille de la carte ([audit de l'affichage d'un lieu](../../../Planning/standards/audit-affichage-lieu.md)).
- La scène de combat n'a plus de chaîne à part : depuis le `LOT-118` le combat se joue sur la carte et se rend par `WorldSceneComposer` / `WorldSceneRenderer` dans `hmi::WorldViewportItem` (la chaîne du Colisée seul, `LOT-50`/`LOT-86`, a été retirée à la recette de la 0.0.1).
- `CityBlockRender` — l'**îlot** vu sur le plan : la carte du quartier telle que le jeu la dessine, cadrée sur un rectangle nommé (`LOT-96`).
- `TileVisuals` — correspondance type de tuile → région d'atlas (`regionForTile`), partagée par le canevas et la palette de l'éditeur.
- `GraphicsLog` — macros de journalisation du module.

Réf. specs : `EX-REN-002`, `EX-REN-010`…`EX-REN-015`, `EX-REN-020`…`EX-REN-022`, `EX-REN-041`…`EX-REN-043`, `EX-REN-004`…`EX-REN-009`, `EX-ARCH-012`, `EX-ARCH-022`, `EX-ARCH-050`, `EX-NFR-004`, `EX-NFR-005`.
