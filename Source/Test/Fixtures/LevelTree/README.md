# Racine d'essai : l'arborescence par niveaux

Racine de données d'essai du [LOT-124](../../../../Planning/versions/v0.1.0/v0.0.1-demo/lots/LOT-124-editeur-et-arborescence-par-niveaux.md) :
une carte puise dans son lieu **et** dans ses niveaux communs. Elle a la forme de
`Source/Elements` et suit l'[arborescence des assets](../../../../Planning/standards/arborescence-assets.md).
Comme `GameData`, elle prouve des **mécanismes**, pas du contenu : son art est la planche
synthétique de `GameData/Assets/Scene/bourg`, recopiée pièce à pièce.

## Ce qu'elle porte

| Niveau | Dossier | Pièces |
|---|---|---|
| Monde | `Assets/Common/Terrain`, `Assets/Common/Props` | `grass` (sol), `crate` |
| Région | `Assets/Regions/central-empire/Common/Scene` | `banner-lion` |
| Ville | `Assets/Regions/central-empire/capital/Common/Scene` | `paving`, `wall-arcade`, `fountain` (rangées en sous-dossiers) |
| Zone | `Assets/Regions/central-empire/capital/arenarea/Scene` | `cobbles`, `fountain` (masque celle de la ville), `stall` (2 × 1) |
| Sous-zone | `…/arenarea/arena-of-fate/Scene` | `sand` |
| Zone | `…/capital/martpart/Scene` | aucune : tout lui vient de la ville |

Les figurines se rangent de même : `anariel` (PNJ nommé) et `citizen` dans le `Characters/` de
l'Arenarea — ce citadin masque celui de `capital/Common/Characters` —, `Peoples/human/guard` dans
`Common/Characters`. Leur art est la figurine d'essai `GameData/Assets/Npc/figurant`.

Les tables d'apparence s'empilent de même : le monde traduit `grass` et `pavement`, la ville
`pavement` (elle masque le repli du monde) et `wall`, la zone `flagstone`.

Deux cartes, sous le chemin de leur lieu :

- `Levels/central-empire/capital/arenarea.json` cite une pièce de **chacun** des quatre niveaux, et
  pose le PNJ nommé, le citadin de la zone et le garde du monde ;
- `Levels/central-empire/capital/martpart.json` pose la fontaine **de la ville** sous le même nom
  court que celle de l'Arenarea : c'est elle qui prouve qu'une promotion ne réécrit que les cartes
  qui changent vraiment. Son citadin est celui de la ville.

Les modèles : `Editor/Templates/street.json` sert partout, `Editor/Templates/central-empire/capital/plaza.json`
aux seuls quartiers de la Capitale.

## Règles

- La collision des cartes égale la déduction (`LevelEditor --check` : aucune case forcée).
- `Fixtures/Gestures/niveaux.json` refait l'Arenarea geste par geste et doit la rendre octet pour
  octet : une carte qui change se régénère par `LevelEditor --data <cette racine> --apply …`,
  jamais à la main.
