# Audit — quitter le pixel art : ce que ça casse, ce qu'il faut changer

Relevé du 20 septembre 2026, sur `main` à `f6fbbbb2c`. Il répond à deux questions : *peut-on
supprimer tous les assets et toutes les cartes ?* et *le moteur affiche-t-il de la 2D HD sans
retouche ?*

**Oui** à la première, à condition de le faire en un lot et non en un `git rm` : le moteur se
dégrade proprement (damiers et marqueurs engendrés), mais trois règles CMake, quatre scripts de
contrôle et une trentaine de tests lisent les fichiers supprimés.

**Non** à la seconde. L'hypothèse de départ — « aucune révision du moteur n'est nécessaire » — ne
tient pas : quatre hypothèses du pixel art sont **compilées** dans le rendu, et une pièce HD
s'afficherait à la mauvaise taille, rognée et scintillante. Ce sont les lots
[LOT-102](../versions/v0.1.0/v0.0.1-demo/lots/LOT-102-table-rase-assets-et-cartes.md) et
[LOT-103](../versions/v0.1.0/v0.0.1-demo/lots/LOT-103-rendu-hd.md).

## 1. Ce que le rendu suppose du pixel art

| Où | Hypothèse | Effet sur une pièce HD |
|---|---|---|
| `Source/HMI/Graphics/ScenePieces.h:36-40` | le losange fait **68 × 42 pixels d'art** (`SCENE_TILE_WIDTH_PIXELS`) ; l'échelle de toute pièce de relief s'en déduit | une pièce dessinée pour un losange de 256 px s'affiche **3,8 fois trop grande** |
| `ScenePieces.h:43-60`, `WorldSceneComposer.cpp:154-163`, `ArenaSceneComposer.cpp:214-222` | une figurine fait **64 px de haut** : `SceneTexture` n'a pas de `frameHeight`, la constante en tient lieu | toute figurine plus haute est **rognée** — c'est déjà le cas du lion (96 px) aujourd'hui |
| `Source/HMI/Graphics/SpriteBatch.cpp:92-95` | **un seul** échantillonneur, au plus proche voisin, sans mipmap, pour tous les sprites de scène | une image HD réduite à l'écran **scintille** |
| `WorldSceneRenderer.cpp:43`, `Camera2D.h:100-120` | le zoom est **entier** (`max(1, hauteur / 720)`) | l'art HD ne peut pas être cadré à la taille de la fenêtre |
| `ScenePieces.h:47` | `FIGURE_SCALE = 1.25` : les figurines sont grossies d'un quart | un art livré à sa taille finale n'en veut pas |
| `CityBlockRender.cpp:26` | la pièce la plus haute fait **135 px** | le cadrage des îlots est faux dès qu'une pièce HD arrive |
| `AssetGallery.h:24` | cellule de galerie de **68 px** | les pièces HD débordent |
| `Source/Ui/Screens/BlockMapForm.ui.qml:55` | `smooth: false` | plan de quartier crénelé |

Ce qui est **déjà** indépendant de la résolution : le sol (`composeFloor` étire la texture sur le
losange projeté), le chargement des textures (aucune limite de taille), et le manifeste de pièces,
qui lit `width`, `height` et l'ancre dans les données — il porte même un champ `"tile": [68, 42]`
que le C++ ne lit pas. **C'est le crochet naturel** : l'échelle de l'art devient une donnée du lieu.

Le rapport du losange (0,62, `IsoProjection.h:49`) peut rester : la planche de référence
d'Arenarea le respecte. Le [standard 2D HD](style-2d-hd.md) le garde.

## 2. Ce que la suppression casse

**À la configuration.** `Source/Ui/CMakeLists.txt:140-146` et `:153-159` embarquent `Assets/Maps`
et `Assets/Coliseum` sans garde : une liste vide fait échouer CMake. `CopyGameData`
(`Source/HMI/CMakeLists.txt:138-159`) exige que `Levels/`, `Maps/`, `World/` et `Assets/` existent.

**Dans les contrôles Python** (job `lint-exigences`) : `check_map_assets.py` lit
`Assets/Maps/manifest.json` sans condition et exige une carte par région de l'atlas ;
`check_rpg_data.py` exige que chaque quartier de `World/cities/capital.json` pointe sur une carte
de `Levels/` ; `check_asset_keys.py` exige `Assets/Entities/families.json` ; `generate_cahier_test.py
--check` échoue tant que le cahier n'est pas régénéré ; deux tests pytest portent sur la carte
supprimée (`test_arena_map_integration.py`, `test_level_schema.py`).

**Dans les tests C++.** Une trentaine de fichiers nomment une carte ou un asset livré. Cinq sont à
supprimer (ils ne testent que le contenu retiré), les autres à **rebrancher sur des données de
test** : leur mécanisme vaut d'être gardé — la parité GPU / QPainter de `test_scene_painter.cpp` en
tête. Quatre captures de référence QML sont à régénérer.

**Dans les spécifications.** `EX-VIS-008` prescrit le pixel art (losange 68 × 42, 64 couleurs,
alpha binaire, plus proche voisin) et `EX-REN-013` un agrandissement entier : toutes deux sont à
réécrire. `EX-VIS-009` sépare une scène en pixel art d'une interface peinte : la séparation perd
son motif. `EX-CNT-042` (tout asset livré paraît dans la galerie) **reste**, et s'applique à la
nouvelle arborescence.

## 3. Ce qui ne doit **pas** partir

| À garder | Pourquoi |
|---|---|
| `Assets/UI/`, `Assets/Fonts/` | l'interface à la charte v2 n'est pas du pixel art |
| `Assets/Entities/families.json` | familles d'icônes RPG, sans rapport avec la scène ; `check_asset_keys.py` l'exige |
| `World/regions/`, `World/locations/` | l'atlas : 13 régions, 107 fiches, lu par les tests et par cette planification |
| `World/dialogues/`, `Rpg/`, `Localization/`, `Credits/` | règles et textes, indépendants du style |
| `Assets/Maps/` et `Maps/world-maps.json` | **à trancher** : les seize cartes peintes par l'auteur ne sont pas du pixel art ; le LOT-102 propose de garder le monde et les régions, et de refaire les plans de ville avec leurs quartiers |

## 4. Ce que `Tools/AssetsHD/` contient déjà

| Dossier | Contenu | Usage |
|---|---|---|
| `Arenarea/arenarea-planche-reference-v2.png` | planche de **référence** (1536 × 1024) : scène, palette de huit teintes, quatre matières, dix familles de pièces | la **bible de style** du [standard 2D HD](style-2d-hd.md) — pas une planche de production |
| `Colisee/Sols/` | deux planches de 1254 px : sable, pavé, deux bordures | sols de la pré-carte ; franges de détourage à nettoyer |
| `Colisee/Murs/` | mur en U et en V (avec et sans usure), angle rentrant, angle sortant, 1254 px | l'enceinte de l'arène |
| `Colisee/Decors/` | deux bandes de foule (1983 × 793), deux gardiens (1024 × 1536) | gradins et entrée ; les bandes de foule sont encore en **pixel art** — à refaire |
| `God Statues/` | dix-neuf statues et leurs consignes (`briefs.json`) | décor des parvis ; plusieurs consignes disent encore « pixel art » |
| `Bannieres/` | treize emblèmes de région **extraits du Sourcebook** | **référence seulement** : aucune image du corpus n'entre dans le jeu — à redessiner |

De quoi composer une **pré-carte** de l'Arena of Fate : sol, enceinte fermée, deux gardiens à
l'entrée. Il manque toute figurine HD, un manifeste (clés, emprises, ancres) et la carte elle-même.
Les pièces font 1254 px pour un losange cible de 256 : elles se **réduisent** d'un facteur 4,9 à
l'installation, et leurs ancres se mesurent — le canevas est carré, l'art y flotte.
