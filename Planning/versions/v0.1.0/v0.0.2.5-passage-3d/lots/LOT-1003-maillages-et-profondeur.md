+++
id = "LOT-1003"
titre = "Maillages et profondeur"
version = "0.0.2.5"
filiere = "moteur"
statut = "a-faire"
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
| `hmi::Camera2D` comme caméra **du lieu** | `WorldSceneRenderer`, `WorldViewportItem`, `CityBlockRender` | remplacée par la caméra 3D ; `Camera2D` ne reste que là où la vue est vraiment plane (galerie) — si plus rien ne l'emploie, elle part avec son test |
| La bande réservée aux murs du fond (`ARENA_WALL_RISE`) | `Source/Core/Combat/IsoProjection.h` | c'est une marge de cadrage 2D ; la boîte de la scène se calcule désormais en volume. À confirmer à l'ouverture du lot : elle ne part que si plus aucun cadrage ne la lit |

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

## Risques et questions ouvertes

- Une grande image (fontaine 3 × 3, bâtiment) est un seul plan : elle peut se croiser avec un mur
  en volume. À mesurer sur la carte d'Arenarea ; la parade est de ne garder en image que des
  emprises 1 × 1.
- Le contour sombre, si le standard le garde, demande une passe de plus.
