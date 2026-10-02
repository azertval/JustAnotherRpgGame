+++
id = "LOT-1003"
titre = "Maillages et profondeur"
version = "0.0.2.5"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Le moteur dessine des volumes : une pièce de décor peut être un maillage, départagé des autres par le tampon de profondeur, sous la même vue isométrique."
prerequis = ["LOT-1001", "LOT-1002"]
livrables = [
  "Une **passe de maillages** en QRhi (`Source/HMI/Graphics`) : sommets, normales, coordonnées de texture ; tampon de profondeur ; shader non éclairé (la lumière est au LOT-1007).",
  "La **caméra du lieu** en 3D : orthographique, 45° / 38,3°, même cadrage et même zoom que le `LOT-103` (une case = 100 px à 1080p).",
  "Un **chargeur `.glb`** sans dépendance à Qt Quick 3D, et la mention de sa bibliothèque dans `THIRD-PARTY-NOTICES.md`.",
  "Le **manifeste du lieu** : une clé de pièce cite un maillage (`\"mesh\"`) ou une image ; `hmi::PlaceAppearance` et `core::ScenePieceManifest` lisent les deux.",
  "Les **images dans la scène 3D** : un sol en image se pose à plat ; toute autre image se dresse face à la caméra, triée comme aujourd'hui, et teste la profondeur contre les maillages.",
  "Des données d'essai (`Source/Test/Fixtures/Meshes/`) : un îlot de murs, un sol, un toit ; tests de composition sans GPU et de rendu hors écran.",
]
criteres = [
  "Les quatre cartes livrées, dont aucune pièce n'est encore un maillage, se rendent **au pixel près** comme avant le lot, aux seuils d'image déjà écrits.",
  "Sur la carte d'essai, une figurine passe devant puis derrière un mur en maillage sans qu'aucun code de tri ne décide : le tampon de profondeur seul.",
  "Le pointage d'une case au sol donne la même case qu'avant, sur toute la carte d'Arenarea.",
  "`bench_world_frame` ne régresse pas sur la carte d'Arenarea.",
  "L'éditeur montre la carte d'essai en maillages sans une ligne de code propre : il partage le rendu (LOT-1002).",
  "`git grep -n \"Quick3D\" Source` ne trouve rien.",
]
+++

## Pourquoi

QRhi est une API 3D ; le pipeline actuel en coupe la profondeur (`setDepthTest(false)`) et trie les
primitives à la main, du fond vers l'avant. Ce lot ajoute la forme manquante — le maillage — sans
rien retirer de ce qui s'affiche : à sa livraison le jeu est **identique à l'écran**, et c'est son
premier critère.

## Périmètre

Dedans : la passe, la caméra, le chargeur, le manifeste, les images dressées.

Dehors, nommément :

- aucun asset livré ne change : les maillages du kit sont au
  [LOT-1004](LOT-1004-kit-de-la-capitale-en-maillages.md) ;
- la lumière et les ombres ([LOT-1007](LOT-1007-eclairage-et-cycle-jour-nuit.md)) ;
- la déformation par os ([LOT-1005](LOT-1005-squelette-et-animations.md)) : les figurines restent des
  bandes ;
- Qt Quick 3D, écarté : il n'est distribué que sous GPLv3 ou licence commerciale, ce qui ne
  s'accorde pas avec la licence du dépôt.

## À supprimer

Presque rien, et c'est voulu : tant qu'une seule pièce de décor est une image, l'ordre du peintre
sert encore.

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| `hmi::Camera2D` comme caméra **du lieu** | `WorldSceneRenderer`, `WorldViewportItem`, `CityBlockRender` | remplacée par la caméra 3D ; `Camera2D` ne reste que là où la vue est vraiment plane (galerie) — si plus rien ne l'emploie, elle part avec son test. **Fait** : plus rien ne l'employait (la galerie dessine en pixels d'écran, par `screenProjectionMatrix`) ; elle est partie, `hmi::PlaceCamera` et `test_place_camera.cpp` la remplacent |
| La bande réservée aux murs du fond (`ARENA_WALL_RISE`) | `Source/Core/Combat/IsoProjection.h` | c'est une marge de cadrage 2D ; la boîte de la scène se calcule désormais en volume. À confirmer à l'ouverture du lot : elle ne part que si plus aucun cadrage ne la lit. **Elle reste** : `worldCamera` borne encore la caméra du jeu à `sceneSize()`, qui la compte, et l'origine de la grille en dépend — la retirer déplacerait chaque carte de 0,85 case, contre le premier critère. Son retrait va avec le cadrage en volume, quand plus aucune pièce de décor n'est une image |

Ce qui **reste**, avec sa date de retrait :

| Quoi | Retiré par |
|---|---|
| Le tri par le pied et les calques de `hmi::ComposedScene` pour les pièces en image | le retrait de la dernière image de décor, à la `0.0.3` |
| Le mécanisme d'étage 2D (rang de tri, rectangle d'occlusion, effacement devant le héros) | le [LOT-1004](LOT-1004-kit-de-la-capitale-en-maillages.md), quand les toits deviennent des maillages |

## Conception

- **Deux passes** par image : les maillages opaques, profondeur écrite ; puis les images triées,
  qui testent la profondeur sans l'écrire (leurs bords sont adoucis).
- **Une image dressée face à une caméra orthographique fixe occupe exactement ses pixels
  d'aujourd'hui.** C'est ce qui permet le critère « au pixel près » et la tolérance des images
  jusqu'à la `0.0.3`.
- **La composition reste une fonction pure**, testée sans GPU : elle produit une liste de maillages
  placés à côté de la liste de quads.
- Le pointage au sol reste `screenToWorld` puis `worldToTile` : la projection est toujours affine.

## Tranché à l'ouverture (2 octobre 2026)

- **Deux objets pour la caméra.** `hmi::IsoView` porte l'orientation — 45°, élévation de sinus
  0,62 : où tombe un point élevé, à quelle profondeur, la pose d'un maillage. `hmi::PlaceCamera`
  cadre : c'est `Camera2D` renommée, sa matrice gardée coefficient pour coefficient sur le plan de
  l'image, avec un troisième axe pour la profondeur. Sans étendue de profondeur fixée, la matrice
  est celle d'avant.
- **La profondeur n'existe que si l'image a un maillage.** Sans lui, le pipeline des quads est
  celui d'avant — ni test ni écriture —, les sommets sont à la profondeur zéro et aucun n'est
  découpé. Le premier critère tient par construction, et il est mesuré (ci-dessous).
- **Une image « dressée » est un plan vertical**, tourné vers la caméra, posé sur la ligne de son
  pied ; au-dessous du pied, le sol. Un plan perpendiculaire au regard occupe les mêmes pixels
  mais penche en arrière : une figurine de 1,80 m à moins de 0,88 m devant un mur aurait la tête
  dedans (1,80 m × cos 38,3° × sin 38,3°). Le pied d'une pièce est celui de son tri — le sommet bas
  de son emprise, ou son `depthOffset` ; celui d'une figurine, ses pieds, au centre de sa position.
  Une image dressée que son pied traverse est soumise en deux rectangles.
- **Les marques passent devant tout.** Jetons, tracés et aides d'édition (`RenderLayer::UI`,
  `EditorOverlay`) ne testent rien : un jeton à demi masqué par un mur ne dit plus où est le PNJ.
- **Le repère d'un maillage de décor dans la grille : +X le long des colonnes, +Z le long des
  lignes**, l'origine au centre de l'emprise. Le standard fixe l'unité, la hauteur et l'origine,
  pas cet axe : c'est le choix qui ne retourne pas le maillage (une rotation, sans miroir). **À
  faire entrer au standard par l'auteur** (`style-3d.md`, §1) avant que le LOT-1004 ne modèle le
  kit ; le moteur et la carte d'essai le supposent.
- **Le chargeur n'a pas de bibliothèque à lui.** `core::readMeshFile` lit l'enveloppe du `.glb` à
  la main et son bloc JSON par `nlohmann/json`, déjà lié ; `QImage` décode la texture. Aucune
  dépendance n'entre, le chargeur est dans `Core` et une cible de fuzzing l'éprouve (`fuzz_mesh`).
  Il lit ce que le standard fixe et ce que Meshy exporte ; il **refuse et nomme** un tampon hors du
  fichier, un accesseur creux, une extension requise (Draco, meshopt, KTX). `THIRD-PARTY-NOTICES.md`
  le dit.
- **Aucune face n'est écartée par son sens.** La passe dessine les deux côtés et laisse la
  profondeur décider : un modèle généré n'a pas toujours un sens de face fiable. À revoir au
  LOT-1007, qui éclaire.
- **Un maillage dont le fichier manque retombe sur le damier**, comme une image : la pièce se voit.
- **L'opacité des calques de l'éditeur vaut pour les maillages** : `setQuadOpacity` est demandée
  pour chaque maillage, par son calque et son étage. C'est ce qui tient le cinquième critère sans
  code dans l'éditeur.

Ce que le lot **ne fait pas**, et qui le dit :

- l'**effacement d'un étage devant le héros** ne vaut que pour les images : un toit en maillage
  masque le héros qui passe dessous. C'est le mécanisme d'étage 2D, que le LOT-1004 remplace ;
- la **galerie des assets** et la **palette de l'éditeur** ne montrent pas de vignette d'un
  maillage (la palette affiche le damier) : c'est l'atelier du LOT-1008 ;
- un maillage se charge **sur le fil de rendu**, à l'entrée dans la carte : sans conséquence sur
  des pièces de décor, à mesurer au LOT-1005 sur un personnage.

## Mesures (poste de l'auteur, 2 octobre 2026)

**Les quatre cartes livrées, au pixel.** `LevelEditor --render` sur `Source/Elements`, avant le
lot (`origin/main`, `12d8b4ae9`) et après :

| Carte | Image | Pixels différents |
|---|---|---:|
| `arenarea` | 1901 × 1285 | 0 |
| `arenarea/arena-of-fate` | 2950 × 1937 | 0 |
| `arenarea/arena-of-fate/undercroft` | 1100 × 787 | 0 |
| `martpart` | 1801 × 1221 | 0 |

**`bench_world_frame`, carte d'Arenarea** (Release, médiane de sept passes) :

| Mesure | Avant | Après |
|---|---:|---:|
| `ArenareaSnapshot` | 0,098 ms | 0,096 ms |
| `ArenareaComposeWholeMap` | 0,125 ms | 0,123 ms |
| `ArenareaTexturePaths` | 0,063 ms | 0,062 ms |
| `ArenareaBuildStaticScene` | 0,123 ms | 0,124 ms |
| `ArenareaFrame1080p` | 3,35 µs | 3,31 µs |

Aucune régression. `ArenareaFrame1080p` compose **zéro** primitive, avant comme après : il cadre
la case (60, 42) d'une carte qui n'en fait plus que 24 × 13 depuis sa refonte. La mesure est à
recaler sur la carte d'aujourd'hui — hors de ce lot, qui ne devait pas la faire bouger.

**La figurine devant, puis derrière** (`test_mesh_render`, carte d'essai, une case à 64 px) :
384 pixels de la figurine témoin visibles devant l'îlot ; **0** derrière, alors qu'elle est soumise
au dessin comme devant ; 384 de nouveau derrière, les maillages éteints.

**Le pointage** : sur les 312 cases de la carte d'Arenarea, cinq points par case, la position à
l'écran est identique au bit près à celle de la formule de la caméra 2D, et chaque point revient à
sa case.

**Les kits portent les modèles.** `asset_kits.py`, `check_binary_files.py` et `.gitignore`
traitent un `.glb` comme une image : hors de Git, dans l'archive de son kit. Le premier publié est
`Common@6`, qui reçoit les deux mannequins de l'auteur (`Common/Characters/Mannequins/`, inscrits
au manifeste sous `models`) — en attente du [LOT-1006](LOT-1006-corps-de-reference.md), qui les
lie au squelette : rien ne les affiche encore.

**Les modèles réels.** Les 48 `.glb` de l'atelier (`Tools/Assets3D`, exports Meshy des personnages
et des armes de la démo) se lisent tous, sans erreur : un maillage, une primitive, une matière,
`POSITION` / `NORMAL` / `TEXCOORD_0`, texture JPEG ou PNG incorporée, aucune extension. Ils ne sont
**pas** au budget du standard, et ce lot ne les installe pas :

| Relevé | Valeur |
|---|---|
| Triangles | de 10 318 à **2 582 422** ; 38 fichiers sur 48 dépassent 103 000 |
| Images incorporées | trois par fichier pour 45 d'entre eux (couleur, métal-rugosité, relief) : le PBR que le standard écarte (§4) ; le moteur n'en lit que la couleur |
| Origine | au centre du modèle (de −0,95 m à +0,95 m), pas au sol : la normalisation est l'étape 4 de la chaîne |
| Lecture | de 79 ms à 2,0 s par fichier (Debug) |
| Doublon | `thug` et `wolf` contiennent le même fichier (`Ironbound_Brawler`) |

Décision de l'auteur le même jour, sur une planche comparant maître et copie : la mise au standard
se fait par script (`scripts/assetsGeneration/reduce_model.py`, voir
[personnages 3D](../../../../standards/personnages-3d.md), §4). Les modèles de l'atelier y sont
passés, sans écart ; les copies attendent dans `Tools/Assets3D/Standard/`. La liaison au squelette
et l'installation restent au LOT-1006 et au LOT-1009.

## Risques et questions ouvertes

- **Mesuré sur les cartes livrées** : 14 des 51 pièces de relief posées sur Arenarea ont une
  emprise de plus d'une case (sept de 3 × 3, deux de 2 × 2, cinq de 1 × 2 ou 1 × 3) ; 43 sur 102
  dans l'Arena of Fate, 21 sur 66 à Martpart. Chacune est un seul plan, dressé au sommet bas de
  son emprise : devant tout volume placé dans son emprise ou derrière elle, mais un mur en
  maillage **à côté** d'elle, plus près de la caméra que ses cases du fond, sera recouvert là où
  les deux se superposent à l'écran. Rien ne se croise encore — aucune de ces cartes n'a de
  maillage ; le LOT-1004 le verra sur le kit, et la parade reste d'écrire le `depthOffset` de la
  pièce ou de ne garder en image que des emprises 1 × 1.

- Une grande image (fontaine 3 × 3, bâtiment) est un seul plan : elle peut se croiser avec un mur
  en volume. À mesurer sur la carte d'Arenarea ; la parade est de ne garder en image que des
  emprises 1 × 1.
- Le contour sombre n'est **pas** dans ce lot : le standard le laisse ouvert, et l'auteur le
  tranche au [LOT-1004](LOT-1004-kit-de-la-capitale-en-maillages.md), sur le kit rendu avec et sans
  ([D-40](../../../../vision/decisions.md)). S'il est gardé, la passe s'ajoute là-bas.
