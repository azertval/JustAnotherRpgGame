# Audit : l'affichage d'un lieu qu'on parcourt

Relevé du 24 septembre 2026, sur `lot-109-carte-arenarea` (`4b0da8007`). Première carte à
l'échelle d'un quartier, Arenarea (`LOT-109`, 128 × 88 cases, 14 700 primitives sur sept
couches), elle met en défaut le critère « 60 images par seconde à 1080p ». La première image
arrive après 82 s dans le build Debug, contre 4 s pour une carte de 8 × 6 cases.

La question posée est de savoir pourquoi, et ce qu'il faut changer pour que le coût d'une image
dépende de **ce qu'on voit**, pas de la taille de la carte.

## 1. Le chemin d'une image

```
WorldModel::step (16 ms)  ─►  ++sceneRevision à chaque pas du héros
WorldViewportRenderer::synchronize  ─►  model->snapshot()  (instantané complet, fil graphique bloqué)
WorldSceneRenderer::render
    ├─ ensureTextures(worldTexturePaths(snapshot))   toutes les cases, toutes les couches
    ├─ composeWorldScene(…)                          toutes les cases, toutes les couches
    ├─ ComposedScene::sort()                         toute la carte
    └─ submitComposedScene / SpriteBatch::submit     une passe et un bloc uniforme par texture
```

## 2. Constats

| # | Où | Constat | Coût mesuré |
|---|---|---|---|
| A1 | `WorldSceneRenderer.cpp:186-192` | À **chaque image**, la carte entière est composée puis triée, alors que la caméra n'en montre qu'une vingtaine de cases sur onze. `ComposedScene::setVisibleBounds` existe mais seul l'éditeur l'appelle (`DraftRenderer.cpp:50`). | 8,0 ms (Release) |
| A2 | `WorldSceneRenderer.cpp:186`, `WorldSceneComposer.cpp:992` | Les chemins de texture sont recalculés à chaque image : une chaîne et une insertion dans un `std::set` par case et par couche. Ils ne changent pourtant qu'avec la carte. | 3,1 ms |
| A3 | `WorldModel.cpp:190`, `WorldViewportItem.cpp:67-71` | Un pas du héros fait avancer `sceneRevision`, et l'instantané de **toute** la carte est reconstruit puis copié dans `synchronize()`, pendant que le fil graphique attend. Le héros et les PNJ sont pourtant la seule partie qui bouge. | 4,7 ms par pas |
| A4 | `ComposedScene.cpp:42-49` | Le rang de texture se cherche linéairement dans la liste des textures déjà vues, en supposant qu'elles « se comptent sur les doigts d'une main ». Une carte HD en a 357 : jusqu'à 357 comparaisons par primitive. | compris dans A1 |
| A5 | `WorldSceneRenderer.cpp:106-148` | Toutes les textures d'une carte sont décodées **à la première image**, une à une, sur le fil de rendu : 357 PNG, 33 Mio. | la première image |
| A6 | `WorldSceneRenderer.cpp:145`, `SceneTextureTraits.cpp:51-61` | Pour chaque texture, les traits (ancre, losange, étage) relisent le `manifest.json` du lieu : 862 Kio pour le commun, sans cache. Et même avec un cache, `manifestOf` **rend une copie** du document. `entryOf` parcourt les 3 041 entrées en copiant chaque nom, trois fois par texture. | l'essentiel des 82 s en Debug |
| A7 | `SpriteBatch.cpp:258-265`, `:415-421` | Chaque passe de dessin pousse **sa propre copie** de la matrice de projection, identique pour toute l'image : un bloc uniforme téléversé par changement de texture, soit des milliers par image dans la bande de profondeur. | GPU et pilote |
| A8 | `SpriteBatch.cpp:444` | Une passe de plus de 16 384 quads est **tronquée** en silence (indices 16 bits). Une grande carte peut y arriver : un défaut latent, que le découpage à la vue rend improbable sans le supprimer. | — |
| A9 | `EditorViewport.cpp:1150-1156` | L'essai immédiat de l'éditeur (`P`) refait instantané, composition et tri de toute la carte à chaque pas du héros : le même défaut, par un autre chemin. | comme A1 + A3 |

Ce qui est sain et reste en place :
- la composition est une fonction pure, testée sans GPU ;
- l'ordre de dessin (bande, profondeur, texture) est unique et stable ;
- l'arène compose son champ de bataille **une fois** (`ArenaSceneRenderer.cpp:202`) ;
- l'éditeur ne recompose qu'au geste, et peint sa scène découpée à la vue.

## 3. La cause, en une phrase

Rien ne sépare ce qui **ne change qu'avec la carte** (sols, reliefs, étages, jetons : 99,9 % des
primitives) de ce qui change à chaque image (les figurines, l'effacement des étages devant le
héros, la caméra). Tout est donc refait à chaque image, et sur toute la carte.

## 4. Ce qu'on change

1. **Une scène statique composée une fois** (`hmi::StaticWorldScene`, logique pure). Elle
   compose les pièces de la carte, les trie une fois et range chaque primitive dans une grille de
   seaux en unités monde. Une image n'en prend que les seaux sous la caméra, dans l'ordre déjà
   trié. Les figurines, composées à chaque image, s'y **fusionnent** par le même comparateur.
   L'effacement d'un étage devant le héros s'applique au passage. Le résultat est, primitive pour
   primitive, la composition d'avant privée de ce qui est hors cadre. `composeWorldScene` passe
   désormais par elle : jeu, essai de l'éditeur, canevas et tests suivent un seul chemin.
2. **Un instantané statique partagé.** `WorldPlay` le garde sous `std::shared_ptr<const …>` et ne
   le refait qu'à l'entrée d'une carte, au changement d'un drapeau (un PNJ paraît, une porte se
   ferme) ou de figurine du héros. Un pas ne touche plus que les figurines.
   `WorldModel` distingue la révision de la scène de celle des figurines, et `synchronize()` ne
   copie plus qu'un pointeur.
3. **Les chemins de texture** se calculent avec la scène statique, une fois par carte.
4. **Le rang de texture** passe par une table de hachage : le coût ne dépend plus du nombre de
   textures du lieu.
5. **Les traits lus une fois par lieu.** Le cache des manifestes rend une référence et indexe les
   entrées par fichier. Il vit avec le rendu du lieu, celui de l'arène et les images de l'éditeur.
6. **Le décodage en parallèle.** À l'entrée d'une carte, les PNG se décodent sur tous les cœurs,
   puis les textures se créent sur le fil de rendu, dans le lot de l'image.
7. **Une projection par image.** `SpriteBatch` ne pousse une nouvelle matrice que si elle change.
   Une passe trop longue pour les indices 16 bits se **découpe** au lieu d'être tronquée.
8. **Une mesure dans le dépôt** : `Source/Benchmark/bench_world_frame.cpp` mesure sur la carte
   livrée d'Arenarea l'instantané, la composition de toute la carte et une image cadrée à 1080p.

## 5. Ce qui n'est pas fait ici

- **Un atlas de textures.** La bande de profondeur change de texture presque à chaque pièce,
  d'où une passe par pièce visible, quelques centaines à l'écran. D3D11 les tient. Un atlas ou
  un tableau de textures les ramènerait à quelques passes, mais c'est un chantier du moteur
  entier (arène, galerie, éditeur). Il est à ouvrir si la mesure GPU le demande.
- **Le chargement progressif.** Une carte charge toutes ses textures à l'entrée. Le décodage
  parallèle suffit pour un quartier ; un monde ouvert demanderait un chargement par région.

## 6. Résultats

Mesures du 24 septembre 2026 sur le poste de référence (RTX 4060 Ti, 12 cœurs), carte livrée
d'Arenarea. Le banc est `Source/Benchmark/bench_world_frame.cpp`, en Release.

| Mesure | Avant | Après |
|---|---|---|
| CPU d'une image à 1080p (composition, tri, chemins de texture) | 11,1 ms | **0,07 ms** |
| Primitives soumises par image | 14 699 | **1 136** |
| Passes de dessin par image | 2 859 | **329** |
| Blocs uniformes téléversés par image | 2 859 | **1** |
| CPU d'un pas du héros (instantané de toute la carte) | 4,7 ms | **0** (figurines seules) |
| Ouverture de la carte, build Debug, jusqu'à la première image | 82 s | **5 s** |
| Cadence en jeu, build Debug, 1920 × 1080 | — | **165 ips** (la fréquence de l'écran) |

La composition complète d'une carte (8,4 ms) ne se fait plus qu'une fois, à l'entrée ou quand un
drapeau change ce qui s'y dessine. C'est aussi le coût du canevas de l'éditeur à chaque geste, et
il n'a pas bougé.

Ce qui garde le résultat :

- `StaticWorldSceneTest` vérifie que, sans cadrage, l'image est la composition complète
  primitive pour primitive. Avec un cadrage, elle en est exactement la part qui touche la vue.
  L'effacement des étages se refait à chaque image.
- `ExplorationCarteIntegration.UnPasNeRefaitPasLaCarte` vérifie qu'un pas du héros ne refait pas
  la carte et qu'un drapeau la refait.
- `WorldSceneRendererTest.UnLieuDevientDesPixels` vérifie que la carte entière est composée, sans
  damier, et que l'image en écarte une part.
- Les captures de référence d'images restent identiques.

