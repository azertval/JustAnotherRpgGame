# Rendu 2D : de la scène à l'écran {#guide-rendu}

Cette page explique comment un lieu qu'on parcourt, l'arène du Colisée ou le brouillon de l'éditeur
finissent par apparaître comme une image à l'écran, en partant des notions de base du rendu temps
réel pour qui n'en a jamais écrit. Tout le rendu vit dans `Source/HMI/Graphics`, sur une surface
fournie par Qt (l'éditeur dans `Source/Editor/Ui`, le jeu dans `Source/HMI/Runtime`) ; c'est la
seule partie du moteur qui dépend du GPU, via **QRhi** (voir plus bas — `Core` en reste totalement
indépendant, @ref guide-boucle et `EX-ARCH-040`).

## Vocabulaire de base : GPU, swap chain, back buffer

Un jeu ne dessine pas directement sur l'écran : il dessine dans une zone mémoire dédiée sur la
carte graphique (le **GPU**, *Graphics Processing Unit*, un processeur spécialisé dans le calcul
massivement parallèle nécessaire pour colorier des millions de pixels par seconde), puis cette
image est transmise à l'écran. Dessiner directement dans l'image **actuellement affichée**
provoquerait un artefact visible (*tearing* : une moitié d'image montre l'ancien contenu, l'autre
le nouveau, si l'écran est en train de la rafraîchir pendant qu'on la modifie). La solution
standard, le **double buffering**, utilise **deux** images en mémoire :

- le **front buffer** : l'image actuellement montrée à l'écran, intouchable ;
- le **back buffer** : une image « en coulisse », sur laquelle le jeu dessine librement la frame
  suivante.

Une fois le back buffer entièrement dessiné, une opération de **présentation** échange les deux
rôles (le back buffer devient le front buffer et inversement) — idéalement au moment précis où
l'écran finit de rafraîchir l'image précédente, ce qui s'appelle la **synchronisation verticale**
(V-Sync) : elle évite le *tearing* en alignant l'échange sur le rythme de rafraîchissement de
l'écran, au prix d'attendre ce moment si le jeu est plus rapide que l'écran. L'ensemble
« back buffer(s) + mécanisme d'échange » s'appelle une **swap chain**.

## QRhi : une couche d'accès au GPU, pas un changement de cible

Le projet ne parle pas à Direct3D 11 directement : il passe par **QRhi**, la couche d'abstraction
de rendu de Qt. La cible technique reste Direct3D 11 — QRhi retient ce backend par défaut sous
Windows (`EX-REN-002`) — mais le device, la *swap chain* et la présentation appartiennent à Qt
(`EX-REN-022`).

Ce que le projet conserve en propre :

- `hmi::SpriteBatch` : le pipeline 2D (tampons de sommets et d'indices, tampon uniforme,
  échantillonneur, états de mélange) et l'émission des lots de dessin ;
- `hmi::TextureLoader` : la création des textures GPU à partir de pixels décodés ;
- les **shaders** (`Source/HMI/Graphics/Shaders/sprite.vert`, `sprite.frag`), écrits une fois en
  GLSL et compilés en `.qsb` par l'outil `qsb` — un `.qsb` contient plusieurs traductions (SPIR-V,
  HLSL, MSL), ce qui permet à QRhi de choisir son backend à l'exécution sans que le projet livre un
  shader par API.

Deux contraintes de QRhi façonnent le code, et méritent d'être connues avant de le lire :

1. **Un téléversement ne se déclare pas pendant une passe de rendu.** Les données (sommets,
   pixels de texture) transitent par un `QRhiResourceUpdateBatch`, soumis **avant** l'ouverture de
   la passe. `hmi::SpriteBatch` enregistre donc toute l'image côté CPU, puis téléverse une fois et
   dessine (`SpriteBatch::submit`) — au lieu de réécrire son tampon entre deux appels de dessin.
   `hmi::RhiContext` porte le lot de mises à jour de l'image en cours, partagé par tout ce qui crée
   des textures.
2. **L'espace de clip du shader est celui d'OpenGL**, quelle que soit la cible : la matrice de
   projection est multipliée par `QRhi::clipSpaceCorrMatrix()`, qui la ramène à la convention du
   backend retenu.

## Les surfaces de dessin : un élément composé avec l'interface

Le rendu n'est jamais présenté dans une fenêtre native embarquée (`EX-REN-050`) : un élément frère
d'une fenêtre native ne se dessine pas de façon fiable par-dessus elle. Trois surfaces existent,
toutes composées avec le reste de l'interface :

- `hmi::EditorViewport` (`Source/Editor/Ui`) : le canevas de l'éditeur, une **`QGraphicsView`**
  (`LOT-EDITOR-02`). Il ne parle pas au GPU : il peint par `QPainter` la **même** scène composée
  que le jeu soumet (`hmi::paintComposedScene`), en édition comme en essai immédiat (@ref
  guide-editeur), et reçoit les événements clavier/souris **Qt** (@ref guide-entrees). Un test
  compare son image au rendu QRhi du jeu, cadrage pour cadrage.
- `hmi::WorldViewportItem` et `hmi::ArenaViewportItem` (`Source/HMI/Runtime`) : le lieu qu'on
  parcourt et l'arène, dans le jeu Qt Quick. Ce sont des **`QQuickRhiItem`**, exposés au QML
  (@ref guide-ihm-qt).

Les trois possèdent les mêmes ressources graphiques — lot de sprites, atlas, registre de textures —
regroupées dans `hmi::SceneResources`. Le regroupement existe pour l'**ordre de libération** : ce
qui tient une texture doit mourir avant la texture, et la texture avant le pipeline qui
l'échantillonne. `SceneResources::release` fixe cet ordre une fois pour toutes ; libérer dans le
désordre ne produit pas une erreur nette mais un plantage à la fermeture, intermittent selon le
pilote.

Côté Qt Quick, le modèle et sa session vivent sur le fil graphique, le dessin sur le fil de rendu.
Le seul instant où les deux se parlent est `synchronize()`, pendant que le fil graphique est
bloqué : n'y traversent que des **valeurs** — un instantané de la scène (`hmi::WorldSceneSnapshot`,
`hmi::ArenaSceneSnapshot`), jamais un pointeur vers la carte ou la session.

## Unités monde et pixels : \ref hmi::Camera2D "hmi::Camera2D"

`Core` ne connaît que des **unités monde** (@ref guide-maths) — jamais de pixels. Le rendu doit
donc **convertir** une position monde en position d'écran avant de dessiner quoi que ce soit ;
c'est le rôle de `hmi::Camera2D`. Deux paramètres gouvernent cette conversion :

- `PIXELS_PER_UNIT = 16` : l'échelle de base, fixée par convention du projet (`EX-ARCH-021`) — une
  unité monde occupe 16 pixels à l'écran avant tout zoom ;
- le **zoom** (`setZoom`) : un multiplicateur additionnel de cette échelle. Un zoom **entier** est
  recommandé (`EX-ARCH-022`) : avec un style *pixel art*, un zoom non entier (1,5× par exemple)
  étirerait certains pixels source sur 1 pixel écran et d'autres sur 2, brisant la netteté des
  contours voulue par ce style visuel.

La caméra a aussi un **centre** (`setCenter`, en unités monde) : le point qui apparaît au milieu de
l'écran. `projectionMatrix()` combine centre, échelle et dimensions de la fenêtre (le *viewport*)
en une **matrice de projection orthographique** : une transformation mathématique standard en rendu
2D/3D qui convertit une position monde en position « clip » — l'espace normalisé que le GPU attend
en sortie du *vertex shader* (voir plus bas). C'est cette matrice, et non une conversion manuelle
pixel par pixel, que le pipeline de dessin applique à chaque sommet ; `worldToScreen`/`screenToWorld`
exposent la même conversion côté CPU, pour des besoins hors dessin (par exemple convertir une
position de souris en position monde).

### Cadrer une scène : `fitZoom`, `worldCamera`, `arenaCamera`

`Camera2D::fitZoom` calcule le zoom qui fait tenir un rectangle donné (en unités monde) dans une
surface disponible (en pixels), sans jamais laisser de zone hors champ : zoom **entier** tant que le
rectangle tient déjà à l'échelle ×1 (netteté pixel art, `EX-ARCH-022`), fractionnaire seulement si
nécessaire pour l'englober malgré tout. Fonction pure, partagée par le canevas de l'éditeur (niveau
entier, avec pan/zoom manuel, `EX-EDIT-013`) et l'arène.

Les deux scènes du jeu ont chacune **une** fonction de cadrage, qui est la seule géométrie de la
scène à l'écran :

- `hmi::arenaCamera` (`ArenaSceneRenderer.h`) cadre le Colisée **entier**, centré, au zoom arrondi
  à l'entier par `fitZoom` ;
- `hmi::worldCamera` (`WorldSceneRenderer.h`) **suit** le héros dans le lieu qu'on parcourt
  (`EX-REN-013`) : agrandissement entier (l'art est dessiné pour `WORLD_ART_HEIGHT_PIXELS` = 720
  lignes ; au-delà, on double), centré sur le point suivi, puis ramené dans la scène — sur un axe
  où la scène est plus petite que la vue, la caméra reste centrée, faute de quoi la carte collerait
  à un bord.

> **Écart avec la spécification, ouvert depuis le `LOT-101`.** `EX-REN-013` ne demande plus
> d'agrandissement entier : elle veut un facteur libre, déduit de la définition de la fenêtre (une
> case = hauteur de la fenêtre / 10,8). Ce que décrit ce paragraphe est le code **d'aujourd'hui**,
> hérité du pixel art ; c'est le `LOT-103` qui le met à l'exigence, en même temps qu'il donne à
> `SpriteBatch` un échantillonneur bilinéaire avec mipmaps.

L'élément Qt Quick publie ce cadrage à son calque d'interface QML et s'en sert pour traduire le
pointeur en case : deux cadrages recalculés chacun de leur côté ne tombent jamais au même pixel.

## Le pipeline de dessin de sprites : \ref hmi::SpriteBatch "hmi::SpriteBatch"

### Pourquoi « batcher » plutôt que dessiner un sprite à la fois

Chaque appel de dessin adressé au GPU (un *draw call*) a un coût fixe non négligeable, indépendant
du nombre de pixels dessinés — piloté par la communication CPU → GPU, pas par le travail du GPU
lui-même. Une carte de plusieurs milliers de cases dessinées par des appels **individuels**
saturerait ce coût fixe avant même de saturer le GPU. Le **batching** (« dessin par lots ») regroupe
un grand nombre de sprites partageant la **même texture** en un minimum d'appels de dessin :
l'usage est `beginFrame()`, puis pour chaque lot `begin(projection, texture)`, un ou plusieurs
`draw(quad)` et `end()`, puis un unique `submit(...)` qui téléverse et dessine l'image entière.

### \ref hmi::SpriteQuad "SpriteQuad" : un rectangle texturé

Un **quad** est simplement un rectangle (deux triangles, en pratique — un GPU ne sait dessiner que
des triangles). `hmi::SpriteQuad` (`HMI/Graphics/Quad.h`, sans dépendance GPU) en décrit un par sa
position/taille en **unités monde** (`x, y, width, height`), la portion de texture à échantillonner
en **coordonnées UV normalisées** (`u0, v0, u1, v1`, chacune dans `[0, 1]` — la convention
universelle du rendu temps réel pour désigner un point dans une texture indépendamment de sa
résolution en pixels), et une teinte RVBA (`r, g, b, a`) multipliée avec la texture au dessin — une
teinte blanche opaque (1,1,1,1) laisse la texture inchangée, une teinte plus sombre ou colorée
module son apparence sans créer de variante de texture séparée.

### Sommets, shaders, et échantillonnage *nearest*

En interne, chaque quad devient 4 **sommets** (`Vertex` : position, UV, couleur), envoyés au GPU
avec deux petits programmes qui s'exécutent **sur le GPU** lui-même :

- le **vertex shader** transforme chaque position de sommet (unités monde) vers l'espace clip, via
  la matrice de projection de la caméra ;
- le **pixel shader** (aussi appelé *fragment shader*) calcule la couleur finale de chaque pixel
  couvert par les triangles, en échantillonnant la texture à la coordonnée UV interpolée et en la
  multipliant par la couleur du sommet.

L'échantillonnage utilise le mode ***nearest*** (au lieu du filtrage *bilinéaire*, plus courant
ailleurs) : il choisit le pixel de texture le **plus proche** de la coordonnée demandée, sans
mélanger ses voisins. C'est délibéré pour un rendu **pixel art** : le filtrage bilinéaire
adoucirait/flouterait les contours nets des sprites, un effet indésirable dans ce style visuel.

Le pipeline gère aussi la **transparence** (mélange `SrcAlpha`/`OneMinusSrcAlpha`) : sans un état
de *blending* configuré, le canal alpha d'un quad serait ignoré et chaque sprite dessinerait un
rectangle plein — une figurine n'aurait plus de silhouette.

### \ref hmi::LineQuad "LineQuad" : un segment orienté

`SpriteQuad` décrit toujours un rectangle **aligné aux axes** : impossible d'en tirer un trait en
diagonale. `hmi::LineQuad` couvre ce cas sans nouveau pipeline ni nouveau shader — même tampon,
même `draw`, juste une seconde façon de calculer les 4 sommets : au lieu d'un rectangle, deux
**extrémités** (`ax, ay, bx, by`, unités monde) et une **épaisseur** perpendiculaire au segment
(`thickness`). `SpriteBatch::draw(const LineQuad&)` calcule la direction normalisée du segment, en
déduit une normale (perpendiculaire, longueur `thickness / 2`), et pousse directement les 4 sommets
décalés de part et d'autre des deux extrémités — le même tampon d'indices (deux triangles par quad)
s'applique sans changement, quelle que soit l'orientation. Un segment dégénéré (les deux extrémités
confondues) ne pousse aucun sommet.

## Les textures : atlas procédural, fichiers et replis

Un **atlas de texture** (ou *spritesheet*) regroupe **plusieurs** images dans une **seule** grande
texture, à des positions connues. C'est ce qui permet le batching décrit plus haut :
`SpriteBatch::begin` ne prend **qu'une seule** texture par lot, donc dessiner des sprites différents
dans le même appel exige qu'ils proviennent tous du même atlas.

### \ref hmi::TextureAtlas "hmi::TextureAtlas" : l'atlas des couleurs plates

`hmi::TextureAtlas` porte une grille de régions de 16 pixels de côté (`TILE_SIZE`), **générée en
code** par `hmi::buildProceduralAtlasImage` (`HMI/Graphics/ProceduralAtlas.h`, logique **pure**,
sans GPU ni Qt, entièrement testée). `tile(colonne, ligne)` renvoie la **région** (rectangle en
pixels, `core::AtlasRegion`) d'une case de cette grille — pure arithmétique, `static`. Le canevas de
l'éditeur s'en sert : `hmi::regionForTile` (`HMI/Graphics/TileVisuals.h`) est l'**unique**
correspondance type de tuile → région, partagée par `hmi::DraftRenderer` et la palette de l'éditeur,
si bien que la vignette de la palette et la case peinte ont toujours la même couleur. Elle ne dépend
que de la géométrie de grille, jamais d'une texture chargée : la palette peut l'appeler sans
contexte GPU.

### Les textures depuis fichiers

Les scènes du jeu se dessinent à partir de **fichiers image** (`EX-REN-041`, `EX-REN-042`) : les
pièces de l'atelier des textures (`Assets/Scene/<lieu>/<pièce>.png`) et les
bandes d'animation des figurines (`Assets/Npc/<slug>/<bande>.png`). Le chargement
(`HMI/Graphics/TextureLoader.h`) se déroule en deux étapes :

1. **Décodage** (`decodeImageFile`) : `QImage::load` puis `convertToFormat(Format_RGBA8888)`.
   `RGBA8888` est choisi **non prémultiplié** : le mélange de `SpriteBatch` utilise
   `SrcAlpha`/`OneMinusSrcAlpha` (alpha simple), pas `One` — un format prémultiplié donnerait des
   couleurs assombries aux bords transparents.
2. **Upload GPU** (`createTexture`) : `QRhi::newTexture` (`QRhiTexture::RGBA8`, un seul niveau de
   mip) puis `QRhiResourceUpdateBatch::uploadTexture`. L'atlas procédural et les marqueurs générés
   passent par la même fonction : il n'existe qu'un seul endroit qui crée une texture sur le GPU.

`loadTextureFromFile` enchaîne les deux. `hmi::ArenaSceneRenderer` charge d'avance les pièces de son
catalogue ; `hmi::WorldSceneRenderer` charge **à la demande**, quand l'instantané réclame une pièce
qu'il n'a pas encore — la carte change au passage d'un portail, ses pièces avec.

### Ce qui manque se voit

Un asset absent ou illisible n'interrompt jamais le rendu (`EX-REN-007`, `EX-NFR-040`) :

- `hmi::buildMissingTextureImage` (`HMI/Graphics/MissingTexture.h`) génère un **damier magenta**
  opaque et déterministe — impossible à confondre avec un asset réel ou avec un trou de rendu.
  `hmi::ScenePieceTextures::resolve` y retombe pour tout chemin non chargé ;
- une entité de carte sans illustration est dessinée par son **marqueur généré**
  (`hmi::entityMarkerKey`, `HMI/Graphics/EntityMarkers.h`), dont `hmi::TextureCache` crée et garde
  la texture à la demande — une clé, une image, toujours la même ;
- `hmi::AnimationCatalog::validateAgainstTexture` confronte une description d'animation (voir
  plus bas) aux dimensions du PNG décodé : une bande qui n'y correspond pas est refusée avec un
  verdict (`hmi::AssetValidation`, `HMI/Graphics/AssetContract.h`) dont le message nomme le
  fichier, le trouvé et l'attendu, plutôt que de produire des artefacts silencieux.

### L'animation : des clips en données

Une figurine s'anime par **bandes d'images** (`EX-REN-012`), décrites par des **données** plutôt
que codées en dur (`EX-REN-005`). Un clip (`core::AnimationClip`, `Core/Ecs/AnimationClip.h`) est
une donnée pure : un nom, une suite d'indices d'images, une durée par image, bouclé ou joué une
fois (`core::ClipEndMode`). Plusieurs clips forment un `core::ClipSet`, adressable par nom.

La description vit à côté de l'image, dans un fichier `nom-asset.anim.json` lu par
`hmi::AnimationCatalog` : bande **horizontale**, largeur et hauteur d'une image, clips. Un asset
sans fichier d'animation reste une **image fixe**, sans erreur ni avertissement. Au Colisée,
`hmi::ArenaAnimationDriver` fait avancer l'animation de chaque combattant au **temps réel du rendu**
(`ArenaSceneRenderer::render` reçoit le temps écoulé), et non au pas fixe : c'est de la
présentation pure, qui ne lit jamais la session de combat (`EX-ARCH-012`).

## Composer, puis soumettre

C'est ici que les fils se rejoignent. Le rendu se fait en **deux temps distincts**, et cette
séparation est le point le plus important de la page :

1. la **composition** produit une `hmi::ComposedScene` : une liste ordonnée de quads en unités
   monde, chacun avec son calque et sa texture. C'est de la logique **pure** : aucun appel GPU. Trois
   compositeurs existent — `hmi::composeWorldScene` (lieu qu'on parcourt, `WorldSceneComposer.h`),
   `hmi::composeArenaScene` (Colisée, `ArenaSceneComposer.h`) et `hmi::DraftRenderer` (brouillon de
   l'éditeur) ;
2. la **soumission** (`hmi::submitComposedScene`, `SpriteRenderer.h`) parcourt cette liste et
   l'envoie au `SpriteBatch`, une passe `begin`/`end` par groupe **contigu** de même texture. C'est
   le seul endroit du rendu qui reconvertit une `hmi::TextureHandle` (identité opaque) en ressource
   GPU.

Pourquoi couper en deux ? Parce que la première moitié devient **testable sans GPU**
(`EX-NFR-004`) : `hmi::QuadRecorder` capture la liste composée et permet d'**asserter** l'ordre des
calques, le regroupement par texture ou l'effet du culling, là où il faudrait sinon regarder
l'écran et juger à l'œil. Un critère d'acceptation du type « le rendu n'a pas changé » cesse d'être
une impression pour devenir un test.

Le lieu et l'arène se composent par le **même** code : même projection isométrique
(`core::IsoProjection`), mêmes planches de l'atelier des textures (`HMI/Graphics/ScenePieces.h` :
losange de 68 × 42 pixels d'art, chaque pièce posée par son **ancre**, le sommet haut du losange de
sa case), même tri par profondeur. Seule la **source** change : l'arène lit une grille de combat,
le lieu une carte (`core::Level`) — sa couche de sol, ses assignations de texture, ses entités,
traduites en pièces par la table du lieu (`hmi::PlaceAppearance`, `EX-REN-010`).

### Les calques : un ordonnancement unique

L'ordre de dessin est d'abord celui des **calques**, `hmi::RenderLayer` (`EX-REN-014`), un jeu
**nommé** et unique dont aucun code ne doit inventer un concurrent :

    Background · Shadow · Tile · Object · Player · UI · EditorOverlay

L'ordre de déclaration **est** l'ordre de dessin. Dans les scènes du jeu, le sol va sur `Tile`, le
relief (murs, torches, bancs…) sur `Object`, les figurines sur `Player` ; le canevas de l'éditeur
pose ses aides (grille, sélection, voile d'aperçu) sur `EditorOverlay`, ce qui les place au-dessus
du reste par construction. `Core` ignore complètement l'existence des calques : c'est une notion de
présentation.

### La profondeur : trier par le pied (`LOT-07`)

En vue de dessus, un calque ne suffit pas : un mur plus bas à l'écran doit passer **devant** une
figurine, un mur plus haut **derrière** (`EX-REN-018`). `Object` et `Player` forment donc une
**bande de profondeur** commune (`hmi::sortsByDepth`, `hmi::renderBand`), à l'intérieur de laquelle
l'ordre vient de la profondeur et non du calque.

La profondeur se lit au **pied** du quad — le point de contact avec le sol —, pas à son coin haut :
deux sprites de hauteurs différentes posés sur la même case doivent s'ordonner de la même façon.
`hmi::depthSortOrder` la quantifie au pixel (`DEPTH_SUBDIVISIONS_PER_UNIT = 16`) : départager deux
pieds distants de moins d'un pixel ne ferait que les faire scintiller au gré des arrondis flottants.
Les compositeurs y ajoutent un **rang** qui départage les pièces d'une même case
(`hmi::arenaDepthSortOrder`, `hmi::worldDepthSortOrder`) — le relief, puis la figurine posée
dessus ; laissé à égalité, le tri trancherait par rang de texture, qui dépend de la première case
composée.

Le tri de la scène composée (`ComposedScene::sort`) est donc d'abord la **bande de calque**. Dans
la bande de profondeur, la **profondeur** tranche, puis la **texture** (regroupement, dans l'ordre
de première apparition) ; dans les autres calques, la texture d'abord, puis l'ordre fin. Regrouper
par texture ne doit sous aucun prétexte faire passer une primitive devant une primitive d'un calque
inférieur. Le tri est **stable**, ce qui
garantit qu'à clé égale l'ordre de composition est préservé d'une image à l'autre.

### Ne dessiner que ce qui se voit : le culling

La composition écarte toute primitive dont la boîte englobante n'intersecte pas le cadrage de la
caméra (`hmi::Camera2D::visibleBounds`, transmis par `ComposedScene::setVisibleBounds`), élargi
d'une **marge d'une case** (`CULLING_MARGIN_UNITS`) pour qu'une entité à cheval sur la frontière ne
disparaisse pas prématurément. Le test porte sur la boîte englobante **réelle** et non sur la
position d'ancrage : un mur haut dont la case est hors champ mais dont le sommet dépasse dans
l'écran reste composé. Le culling est purement visuel — une entité écartée continue d'être simulée
normalement (`EX-ARCH-012`). Les compteurs de l'image (composées, écartées, soumises, passes) sont
exposés par `hmi::ComposedScene::statistics` (`EX-NFR-005`).

### Lecture seule

La composition **lit** l'état du jeu mais ne le modifie **jamais** (`EX-ARCH-012`) — le rendu est
un simple observateur, jamais une source de vérité. L'arène n'est vue que par
`const core::ArenaSession&`, et n'est lue qu'une fois, par `hmi::snapshotArenaScene` ; le lieu, par
`hmi::snapshotWorldScene`. Le rendu est aussi **découplé** de la simulation au pas fixe
(`EX-REN-021`), cohérent avec la séparation décrite en @ref guide-boucle : la simulation avance par
pas fixes, discrets ; le rendu, lui, redessine le dernier instantané une fois par **frame** réelle,
qu'un pas ait eu lieu ou non entre deux frames.

## Assembler la frame complète

Les deux rendus de scène du jeu, `hmi::WorldSceneRenderer` et `hmi::ArenaSceneRenderer`, suivent
les trois mêmes temps, que l'élément Qt Quick ne fait que relayer :

1. `ensureResources(rhi)`, sur le fil de rendu : crée les `SceneResources` et les textures, ou les
   recrée si l'interface QRhi a changé ;
2. `setSnapshot(...)`, depuis `synchronize()` : remplace la scène à dessiner, **en valeurs**, sans
   toucher au GPU ;
3. `render(commandBuffer, target, ...)`, sur le fil de rendu : `SpriteBatch::beginFrame`, cadrage
   (`worldCamera`/`arenaCamera`), composition, `submitComposedScene`, puis `SpriteBatch::submit`,
   qui téléverse les sommets et les textures accumulés et émet l'unique passe de l'image — le fond
   y est effacé à la couleur fournie, même si aucun lot n'a été enregistré.

C'est ce découpage qui rend le rendu testable hors écran : tout ce qui peut casser — l'ordre de
création, l'ordre de libération, la recréation sur une autre interface QRhi — vit dans ces classes,
qu'un test fait tourner sur un vrai `QRhi` sans fenêtre. Le même rendu sert aussi hors écran à
`hmi::renderCityBlock` (`CityBlockRender.h`), qui peint l'îlot d'un quartier pour l'écran « Carte ».

Le canevas de l'éditeur partage la **composition**, pas la soumission : `hmi::composeWorldScene`
compose le brouillon (vue iso) ou la carte jouée par `hmi::WorldPlay` (essai), puis
`hmi::paintComposedScene` la peint par `QPainter` — échantillonnage au plus proche, remplissage
texturé qui prend le centre des pixels comme le GPU. La composition vit dans la cible
`SceneComposition`, sans GPU ni Qt, que `HmiLib` et l'éditeur lient. En essai, c'est la boucle
décrite en @ref guide-boucle : des pas de simulation fixes, puis **une** peinture.

## Voir aussi
- `hmi::SpriteBatch`, `hmi::SpriteQuad`, `hmi::LineQuad`, `hmi::RhiContext`,
  `hmi::SceneResources`, `hmi::Camera2D`.
- `hmi::EditorViewport`, `hmi::WorldViewportItem`, `hmi::ArenaViewportItem` — les surfaces de
  dessin (`EX-REN-050`).
- `hmi::RenderLayer`, `hmi::ComposedScene`, `hmi::QuadRecorder`, `hmi::submitComposedScene`,
  `hmi::depthSortOrder` — composition, calques, profondeur et culling (`EX-REN-014`,
  `EX-REN-018`, `EX-REN-043`, `EX-NFR-004`, `EX-NFR-005`).
- `hmi::composeWorldScene`, `hmi::WorldSceneRenderer`, `hmi::worldCamera`,
  `hmi::composeArenaScene`, `hmi::ArenaSceneRenderer`, `hmi::arenaCamera`, `hmi::PlaceAppearance`
  — les scènes du jeu (`EX-REN-010`, `EX-REN-011`, `EX-REN-013`).
- `hmi::paintComposedScene`, `hmi::SceneImages`, `hmi::DraftRenderer`, `hmi::regionForTile`,
  `hmi::buildProceduralAtlasImage` — le canevas de l'éditeur, peint par `QPainter`.
- `hmi::TextureLoader` (`decodeImageFile`, `createTexture`, `loadTextureFromFile`),
  `hmi::TextureCache`, `hmi::AssetValidation`, `hmi::buildMissingTextureImage` — textures depuis
  fichiers et replis (`EX-REN-041`, `EX-REN-042`, `EX-REN-007`).
- `core::AnimationClip`, `core::ClipSet`, `hmi::AnimationCatalog`, `hmi::ArenaAnimationDriver` —
  l'animation par données (`EX-REN-005`, `EX-REN-012`).
- @ref guide-boucle — où le rendu s'insère dans la boucle de jeu.
- @ref guide-maths — les unités monde converties en pixels par la caméra.
