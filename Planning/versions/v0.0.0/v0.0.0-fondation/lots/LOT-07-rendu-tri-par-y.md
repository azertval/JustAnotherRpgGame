+++
id = "LOT-07"
titre = "Rendu top-down et tri par Y"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "Le monde en vue de dessus devient crédible : le personnage passe devant ce qui est au-dessus de lui à l'écran et derrière ce qui est en dessous, sans scintillement."
prerequis = ["LOT-04", "LOT-06"]
livrables = [
  "`sortOrder` de `hmi::ComposedScene` alimenté par le Y monde du **pied** du sprite, dans les calques `Object` et `Player`.",
  "Caméra de suivi sans biais vertical.",
  "Tests `QuadRecorder` sans GPU sur l'ordre de sortie et la stabilité du tri.",
  "Exigences `EX-REN-*` : tri par Y, décor devant ou derrière selon le pied du sprite, sprites directionnels.",
]
criteres = [
  "Test `QuadRecorder` (sans GPU, `EX-NFR-004`) : trois sprites à Y croissants sortent dans l'ordre attendu.",
  "Aucun scintillement d'ordre entre deux images à Y égal — le tri est déjà stable, un test le fige.",
  "Le personnage passe visiblement derrière un arbre situé plus bas, devant un arbre situé plus haut.",
]
+++

## Pourquoi

Rendre un monde en vue de dessus **crédible** : le personnage passe **devant** ce qui est au-dessus
de lui à l'écran, **derrière** ce qui est en dessous.

### Jalon

C'est ici que le projet redevient **montrable** : une carte, un personnage qui s'y déplace et s'y
insère visuellement. À viser tôt — c'est ce qui fait apparaître les frictions réelles bien avant
qu'on ait investi dans les règles et le combat.

## Périmètre

- Alimenter `sortOrder` depuis le Y monde dans les calques `Object` et `Player` — en pratique le
  **pied** du sprite, pas son coin haut : c'est le point de contact avec le sol qui décide de la
  profondeur.
- Sprites **4 directions** et animations de marche, en réutilisant `AnimationCatalog` et
  `SkinCatalog` (données `.anim.json`, aucun code d'animation nouveau).
- Caméra : suivi sans biais vertical (le platformer décalait la vue vers le haut pour anticiper les
  sauts — sans objet ici).
- Repli procédural du personnage 4 directions dans `ProceduralAtlas`, pour que le jeu reste
  lançable sans aucun asset.

## Conception

### Ce qui existe déjà

`hmi::ComposedScene` trie **de façon stable** par `(calque, texture, sortOrder)`, et `sortOrder`
est un `std::int32_t` **par quad**. Le tri par Y n'est donc pas une refonte du rendu : c'est
**alimenter un champ qui existe déjà** avec le Y monde quantifié.

La stabilité du tri compte autant que le tri lui-même : à Y égal, deux sprites doivent garder un
ordre constant d'une image à l'autre, sinon ils scintillent.

## Exigences couvertes

`EX-REN-*` : tri par Y, décor devant/derrière selon le pied du sprite, sprites directionnels.

## Bilan

Statut écrit dans l'epic d'origine : **partiellement fait** — le tri par profondeur et la caméra
sont livrés, les sprites 4 directions (et leur repli procédural) restaient à faire. L'epic
renvoyait à une section « Ce que la réalisation a tranché » qui n'a jamais été écrite. La fiche est
tenue pour livrée sur ce qui a été fait ; les sprites 4 directions ne l'ont pas été dans ce lot.

Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 921/921, lint
d'exigences, cahier de test, Doxygen et `clang-format` verts. Le `LOT-04` apportait les couches, le
`LOT-06` un personnage qui bouge.
