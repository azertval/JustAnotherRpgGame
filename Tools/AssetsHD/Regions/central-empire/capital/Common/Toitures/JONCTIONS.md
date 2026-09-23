# Toitures modulaires — L, T et croisements

Les matières peintes du lot 129 sont conservées. Les intersections sont calculées en volume : les pans se rencontrent en noues, les faîtages se rejoignent, les égouts internes sont retirés. La découpe par case intervient après le calcul des surfaces visibles, afin de ne pas faire réapparaître sous une aile des morceaux qu’elle cache.

## Catalogue

| Famille | Noms | Nombre |
|---|---|---:|
| Droits existants | `roof-u-dD-rR`, `roof-v-dD-cC`, variantes start/gable/single | 112 |
| L, carré de raccord | `roof-l-dD-ORIENTATION-cCrR` | 216 |
| T, carré de raccord | `roof-t-dD-DIRECTION-cCrR` | 216 |
| Croisement, carré de raccord | `roof-x-dD-all-cCrR` | 54 |
| Total | | 598 |

`D` est la largeur commune des ailes : 2, 3, 4 ou 5 cases. Chaque raccord est un carré de D × D pièces ; C et R vont de 0 à D−1. Chaque PNG a une emprise d’une case, une ancre calculée, et le type tactique `open`. Les 112 pièces droites sont préservées. Les L préexistants sont corrigés pour supprimer les pans internes visibles sous les pignons ; leurs PNG précédents sont sauvegardés dans `BeforeJunctions/Scene/`.

## Placement

Les directions sont celles des coordonnées de la carte, avant projection isométrique : nord = rangées décroissantes, est = colonnes croissantes.

| Raccord | Ailes qui sortent du carré |
|---|---|
| L ne | ouest et sud |
| L nw | est et sud |
| L se | ouest et nord |
| L sw | est et nord |
| T n | ouest, est et nord |
| T e | nord, sud et est |
| T s | ouest, est et sud |
| T w | nord, sud et ouest |
| X all | les quatre directions |

1. Poser toutes les pièces du carré de raccord, en respectant C et R, sur la couche de toit au-dessus du dernier étage.
2. Prolonger ses sorties avec les modules droits de la même largeur D : U à l’est et à l’ouest, V au nord et au sud.
3. Terminer l’extrémité ouest/nord par `start`, l’extrémité est/sud par `gable`.
4. Pour rejoindre un autre raccord, conserver les modules droits sans terminaison entre les deux carrés : aucun pignon à l’intérieur du bâtiment.

Les L et T peuvent ainsi se combiner en U, H ou plans à plusieurs ailes ; le X traite une intersection à quatre branches. Les carrés de raccord ne doivent pas se superposer. Cette livraison concerne les plans orthogonaux et les ailes de même largeur et hauteur : elle ne prétend pas couvrir les diagonales, les courbes ou les rencontres entre largeurs différentes.

## Fabrication et contrôle

```text
python scripts/build_capital_roofs.py --connections --install --jobs 3
python scripts/validate_capital_roofs.py --l
python scripts/validate_capital_roofs.py --junctions
python scripts/validate_capital_roofs.py --junctions --scale 2
python scripts/validate_capital_roofs.py --showcase
python scripts/validate_capital_roofs.py --showcase --scale 2
```

`--connections` reconstruit uniquement les raccords L, T et X. `--junctions` reconstruit uniquement les T et X. Une construction complète sans ce filtre produit les 598 pièces. `--only` conserve les autres entrées du manifeste et du descripteur d’installation.

Les aperçus de toutes les largeurs sont dans `apercus/roofs-l*` et `apercus/roofs-junctions*`. `apercus/roofs-buildings*` montre des bâtiments en L et T sur deux niveaux. Leurs façades de contrôle sont projetées depuis le calcaire V4, réservées à cette carte et sauvegardées dans `EngineJunctions/` ; elles ne remplacent aucun mur du kit de production.

Les scripts et manifestes antérieurs à l’ajout sont conservés dans `BeforeJunctions/`. Les sources peintes et les prompts d’origine ne changent pas.
