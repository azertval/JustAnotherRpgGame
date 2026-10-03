# Le standard 3D

Le jeu garde sa vue isométrique et change de matière le 30 septembre 2026
([D-29](../vision/decisions.md)) : le décor d'architecture devient des **maillages**, les
personnages des **modèles animés par un squelette**. Ce document fixe ce qui remplace le
[standard 2D HD](archives/style-2d-hd.md), archivé le 1er octobre 2026 : il vaut pour **tout ce
qui se modèle**. Les personnages ont leur page, [personnages 3D](personnages-3d.md).

Il est **normatif**, et il ne contient que deux sortes de valeurs : celles que la preuve du
[LOT-1000](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1000-preuve-de-la-chaine-de-personnages.md)
a **mesurées**, et les **décisions datées** de l'auteur. Ce qui n'est ni l'un ni l'autre est écrit
comme **ouvert**, avec le lot qui le tranche ([§8](#8-ce-qui-reste-ouvert)) : le standard ne devine
pas.

## 1. La géométrie — ce qui ne change pas

| Règle | Valeur | D'où elle vient |
|---|---|---|
| Une case | **1,5 m** de côté, la grille du *Manuel* | inchangé : le combat est réglé dessus |
| Unité | le **mètre** ; un modèle s'exporte à l'échelle 1, sans facteur à l'import | mesuré au LOT-1000 : le corps normalisé fait **1,80 m** |
| Repère d'un modèle | origine **au sol, sous le centre de l'emprise** ; hauteur vers le haut ; un personnage se modèle **face à −Y** dans Blender (hauteur +Z), l'export glTF le tourne (hauteur +Y) | mesuré au LOT-1000 (`render_character_strips.py`, `lacet_orientation`) |
| Caméra | **orthographique**, tournée de **45°**, inclinée de **asin 0,62 ≈ 38,3°** ; aucune fuyante, pas de rotation libre | décision de l'auteur (D-29) ; c'est le losange de rapport 0,62 d'`core::IsoProjection` |
| Échelle de la vue | **120,7 px d'art par mètre** : une case fait un losange de 256 × 159 px, un corps de 1,80 m fait 170 px de haut | mesuré au LOT-1000 : la caméra du rendu redonne les valeurs du standard 2D sans réglage |
| Zoom | une case occupe **100 px à 1080p, 200 px à 2160p** ; toutes les définitions cadrent la même étendue de monde | inchangé (`EX-REN-013`) |

La hauteur d'un étage reste celle que déclare le manifeste du kit (`"storey": 224` px d'art, soit
**2,37 m** sous cette caméra) ; le [LOT-1004](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1004-kit-de-la-capitale-en-maillages.md)
l'écrit en mètres en modelant le kit.

## 2. Le format d'échange

| Règle | Valeur | D'où elle vient |
|---|---|---|
| Fichier | **`.glb`** (glTF 2.0 binaire), **un modèle par fichier**, autonome : géométrie, texture **incorporée**, et pour un personnage squelette et clips | mesuré au LOT-1000 (export `Khronos glTF Blender I/O`, Blender 5.2) |
| Attributs d'un sommet | `POSITION`, `NORMAL`, `TEXCOORD_0` ; pour un modèle animé `JOINTS_0` et `WEIGHTS_0`, soit **quatre os au plus par sommet**, poids normalisés | mesuré au LOT-1000 (écart à 1 inférieur à 3 × 10⁻⁸) |
| Maillage | **un seul maillage, une seule primitive, une seule matière** par modèle ; triangles indexés | mesuré au LOT-1000 |
| Atelier | **Blender, piloté par script, sans fenêtre** ; aucune retouche à la main d'un maillage : ce qui ne sort pas du script se recommande | mesuré au LOT-1000 (Blender 5.2.2) |
| Stockage | comme une image de kit : **hors de Git**, publié en archive et verrouillé ([arborescence](arborescence-assets.md#le-stockage)) | règle du LOT-108, inchangée |

Le moteur lit le `.glb` par son propre chargeur, sans Qt Quick 3D
([LOT-1003](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1003-maillages-et-profondeur.md)).

## 3. Le poids d'un modèle

Ce que la preuve a **mesuré**, sur ses deux personnages :

| Mesure | Brawler | Scoundrel |
|---|---:|---:|
| Triangles | 102 894 | 102 988 |
| Os | 53 | 53 |
| Texture incorporée | 4096 × 4096, JPEG | 4096 × 4096, JPEG |
| Fichier `.glb` | 15,1 Mio | 15,5 Mio |

**Le budget d'un modèle animé**, fixé au
[LOT-1005](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1005-squelette-et-animations.md) sur ce
que le moteur a mesuré en animant huit modèles à l'écran (2 octobre 2026, poste de l'auteur,
1080p, carte d'Arenarea) :

| Poste | Budget | Ce qui a été mesuré |
|---|---|---|
| Triangles | **100 000** au plus | huit modèles de 100 000 triangles : + 0,5 ms par image, relecture comprise (`CanvasBenchmarks`, `WorldFrameEightModels1080p` : 6,3 ms contre 5,9 ms en bandes, pour 16,7 ms disponibles) |
| Texture | **2048 × 2048** au plus, couleur de base seule | celle des modèles de l'atelier ; huit modèles tiennent 237 Mio de mémoire graphique, mipmaps comprises |
| Os | **64** au plus par squelette : la taille du bloc d'os du shader (`hmi::MeshBatch::MAX_BONES`) | le squelette `humanoid` en a 53 ; la pose de huit squelettes coûte 42 µs par image (`bench_world_frame`, `ArenareaFrame1080pEightModels`) |
| Influences | **quatre** os par sommet, poids de somme 1 | celles du format (`JOINTS_0`, `WEIGHTS_0`) |

Une texture de 4096 px pèse quatre fois plus (85 Mio par modèle) et n'a pas été mesurée à huit
modèles : elle sort du budget tant qu'une mesure ne l'y fait pas entrer. Les mesures sont celles
d'une carte graphique dédiée ; le rendu logiciel de la CI (WARP) ne dessine que le modèle d'essai,
de 4 000 triangles.

Le plafond de **5 Mio par fichier** (`check_binary_files.py`) porte sur les fichiers **suivis par
Git** : un `.glb` de kit n'y est pas, il n'est donc pas borné par lui. Aucun budget de poids par
zone ([D-23](../vision/decisions.md)), inchangé.

Le budget d'un maillage **de décor** n'a encore aucune mesure : le [LOT-151](../versions/v0.1.0/v0.0.3-capitale-intra-muros/lots/LOT-151-kit-commun-intra-muros.md) le relève
sur le kit de la Capitale (le LOT-1004, qui devait le faire, est clos sans modification —
[D-43](../vision/decisions.md), 2 octobre 2026).

## 4. Les matières et la lumière

- **Une matière par modèle**, non métallique : **la couleur de base seule**, dans une texture. Pas
  de carte de relief, de rugosité ni de métal — mesuré au LOT-1000 (texture Meshy sans PBR).
- **Peint, sans grain** : aplats modelés, matières lisibles à 100 px par case. Le détail plus fin
  est perdu, donc inutile.
- **La lumière vient du moteur**, plus de la texture
  ([LOT-1007](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1007-eclairage-et-cycle-jour-nuit.md)) :
  une texture ne porte **ni ombre portée, ni ombre propre marquée, ni reflet de lumière orientée**.
  Jusqu'au LOT-1007 le shader n'éclaire pas ; un modèle se juge alors à sa seule couleur de base.
- **sRGB, 8 bits par canal**, sans palette imposée par image ; la cohérence vient de la palette du
  lieu ci-dessous.
- **Le contour sombre** du standard 2D n'existe pas en 3D sans une passe de rendu de plus. Il n'est
  **ni gardé ni abandonné ici** : l'auteur le tranche au LOT-151, sur le kit de la Capitale rendu
  avec et sans (décision du 1er octobre 2026, reportée avec le kit par D-43). Le LOT-1003 ne prévoit pas cette passe.

### La palette de l'Empire central

Inchangée — relevée sur la planche de référence d'Arenarea, écrite dans `region.json`. Chaque
région aura la sienne, dans son référentiel.

| Teinte | Usage |
|---|---|
| **Ivoire** `#efe6d2` | marbre, enduits, lumière |
| **Sable** `#d9c7a3` | pierre calcaire, sols |
| **Gris chaud** `#9c948a` | ombres de pierre, pavés |
| **Bronze foncé** `#5c4a2a` | ferronnerie, bois sombre |
| **Bourgogne** `#8e2335` | l'accent impérial : bannières, tuiles, auvents |
| **Or vieilli** `#c9a45c` | emblèmes, chapiteaux, dorures |
| **Vert feuillage** `#3f6b34` | cyprès, haies, jardins |
| **Eau sourde** `#2f7f86` | fontaines, bassins |

## 5. Critères de qualité validés par l'auteur

Confirmés le **23 septembre 2026** sur le LOT-105 V4 (après la reprise du relief des sols, des
balustres et des haies), et repris ici parce qu'ils ne dépendent pas de la matière : ils valent
pour un kit en maillages comme ils valaient pour un kit peint.

- **Fidélité au style d'origine** : garder la facture peinte et les références approuvées ; pas de
  variante générique. L'emblème du lieu se reprend de sa référence, il ne s'invente pas.
- **Grille exacte** : emprises en cases entières, échelle cohérente, origine au sol. Aucun
  ajustement par placement pour masquer une mauvaise calibration.
- **Continuité entre modules** : deux pièces voisines d'une même famille se prolongent — joints
  alignés, phase commune, bords compatibles ; ni couture, ni damier.
- **Vraies pièces d'angle** : angles rentrants et sortants des murs, balustrades et haies, sans
  trou, chevauchement ni étirement de texture.
- **Matière propre, relief lisible** : pas de bruit ni de microdétail, mais la pierre taillée, ses
  biseaux et ses joints creux se lisent. En 3D le relief est **dans le maillage**, pas peint dessus.
- **Volume réel** : balustres modelés, haies au dessus arrondi, raccords continus jusque dans les
  angles. C'était un critère d'image ; c'est désormais la définition de la pièce.
- **Validation en assemblage** : une carte qui réunit toutes les pièces, avec répétitions de sols
  et raccords droits et d'angle, **rendue par le moteur** à plusieurs échelles. Une planche de
  pièces isolées ne suffit pas.
- **Retouches ciblées et réversibles** : le reste d'un kit validé se préserve ; les versions
  précédentes, les sources et les scripts se gardent dans l'atelier.

La référence acceptée reste le rendu du LOT-105 V4
(`Tools/AssetsHD/Regions/central-empire/capital/Common/V4/`) : le LOT-151 compare son kit en
maillages à ce rendu, carte pour carte.

## 6. Les familles de pièces d'un lieu

Les dix familles restent le **gabarit d'inventaire** d'une zone : un lot d'assets de zone les passe
en revue et dit, pour chacune, ce qu'il prend au **commun** et ce qu'il produit en **propre**
([arborescence](arborescence-assets.md), [gabarit de commande](gabarit-commande-zone.md)).

| # | Famille | Emprise type | Forme visée (kit repris à la `0.0.3`, LOT-151) |
|---|---|---|---|
| 01 | Sols | 1 × 1 | **maillage** (LOT-151) |
| 02 | Façades et murs | 2 × 1, 3 × 1 | **maillage** pour les murs ; le reste se classe au LOT-151 |
| 03 | Colonnes | 1 × 1, 3 × 1 | se classe au LOT-151 |
| 04 | Accès | 2 × 1 | se classe au LOT-151 |
| 05 | Balustrades | 2 × 1, angle | **maillage** (LOT-151) |
| 06 | Pièces maîtresses | 3 × 3 et plus | **image tolérée** jusqu'à la `0.0.3` ([D-30](../vision/decisions.md)) |
| 07 | Végétal | 1 × 1 à 3 × 1 | **maillage** pour les haies ; le reste se classe au LOT-151 |
| 08 | Mobilier | 1 × 1 | **image tolérée** jusqu'à la `0.0.3` (D-30) |
| 09 | Bâtiments | 3 × 2 et plus | se classe au LOT-151 ; les **toits** et les **escaliers** sont des maillages |
| 10 | Seuils | 3 × 3 et plus | se classe au LOT-151 |

« Se classe au LOT-151 » n'est pas une échappatoire : la fiche de ce lot doit donner, pièce par
pièce, la forme de tout ce que le kit de la Capitale contient. **Dans la `0.0.2.5`, tout le kit
reste en images** : le LOT-1004, qui devait le convertir, est clos sans modification (D-43). Une pièce d'architecture gardée en
image y est une **dette nommée**, avec le lot qui la retire.

La règle de la **dalle de fond** demeure : une zone livre d'abord un sol répétable, sans bordure,
en **trois variantes au moins**, avant ses panneaux décoratifs.

## 7. Les images tolérées

Quatre sortes d'images restent dans une scène en maillages (les bandes de figurine, la cinquième,
ont été retirées au LOT-1006). Elles gardent, et elles seules, les
règles du standard 2D HD — dont le texte entier est aux [archives](archives/style-2d-hd.md).

| Ce qui reste une image | Jusqu'à | Pourquoi |
|---|---|---|
| Le **mobilier** et les **pièces maîtresses** du kit de la Capitale ; les kits d'**Arenarea**, de l'**Arena of Fate** et de **Martpart** | la `0.0.3` | D-30 |
| L'**architecture** du kit de la Capitale : sols, murs, balustrades, haies, escaliers, toits | la `0.0.3` | D-43 : l'intégralité des assets se reprend à la phase suivante |
| Les **effets** de `Common/Fx/` | sans date | un éclair ou un soin est une image animée, pas un volume |
| Les **portraits** et les **jetons** | sans date | D-30 : ils restent peints |

Les règles qu'elles gardent :

| Règle | Valeur |
|---|---|
| Losange de sol | **256 × 159 px** ; l'échelle de l'art reste une donnée du lieu (`"tile"` du manifeste) |
| Alpha | continu, 8 bits, **prémultiplié** au chargement |
| Filtrage | bilinéaire et mipmaps |
| Ancre | déclarée par le manifeste ; le **pied** de l'image, posé sur sa case |
| Fichier | PNG 32 bits, une pièce par fichier, 4096 px de côté au plus |
| Bande animée | huit images, **8 px de marge** entre deux images et autour ; durée dans le `.anim.json` |
| Portrait | 512 × 512 |
| Jeton | 128 × 128, détouré en rond |
| Agrandissement | jamais : une image trop petite se refait |

Une image tolérée se **dresse face à la caméra** : sous une caméra orthographique fixe elle occupe
exactement les pixels qu'elle occupait. Elle reçoit la teinte de l'heure, pas le soleil. Elle se
commande toujours par les blocs de la [consigne archivée](archives/consigne-2d-hd.md), figée :
plus rien ne s'y ajoute.

## 8. Ce qui reste ouvert

| Question | Tranchée par | Quand |
|---|---|---|
| Le **contour sombre** : passe dédiée, ou abandon | l'auteur, sur le kit rendu avec et sans | LOT-151 |
| La **forme** — maillage ou image — de chaque pièce des familles 02, 03, 04, 07, 09, 10 | la fiche du lot, pièce par pièce | LOT-151 |
| Le budget d'un maillage **de décor** | la mesure sur le kit de la Capitale | LOT-151 |
| Le squelette `quadruped`, ses clips, son image de référence | le lion et le loup de l'arène | LOT-1011 |
| La silhouette `flying` : un squelette par morphologie | ses créatures | avec le premier lot qui en produit une |

## 9. Ce que le standard interdit

- **Aucune image du corpus** dans le jeu, ni affichée, ni décalquée, ni donnée en référence à un
  générateur (règle du `LOT-94`, inchangée).
- **Aucun asset mort** : un lot qui remplace une pièce la supprime dans sa PR
  ([D-32](../vision/decisions.md)) ; `scripts/checks/check_orphans.py` le vérifie en CI.
- **Aucune retouche à la main** d'un maillage, d'une texture projetée ou d'une animation : la
  chaîne se rejoue par script, ou la pièce se recommande.
- **Aucune figurine ne se commande plus en bandes** : un personnage est un modèle
  ([personnages 3D](personnages-3d.md)).
