# LOT-129 — Toitures peintes de la Capitale

Quatre matières produites avec **imagegen intégré** le 23 septembre 2026 : tuiles romaines de terre cuite rouge bourgogne, faîtage, égout à antéfixes et corniche de pignon. Sources originales dans `Sources/`, commandes exactes dans `prompts.json`. La référence fournie par l’utilisateur est conservée dans `References/capital-user.png` : ambiance architecturale seulement. La planche Arenarea et le calcaire V4 fixent la facture du jeu ; aucune portion du panorama n’est utilisée comme texture.

Les pignons utilisent sans modification `../V4/Sources/wall-surface.png`. La géométrie existante est conservée : grille 256 × 159, pente et débords validés, deux axes, profondeurs 2 à 5, quatre positions, soit **112 PNG**. Les bandes sont cadrées à leur contenu visible au chargement, avec deux pixels de marge et conservation de leur alpha ; les sources restent intactes.

## Livraison et reproduction

- `Scene/` : pièces et manifeste de contrôle.
- `Calibres/` : pièces avant réduction.
- `install.json` : installation dans `Source/Elements/Assets/Regions/central-empire/capital/Common/Scene/`.
- `BeforeMaterials/` : scène provisoire, ancien aperçu, script et manifeste installé antérieurs.
- `apercus/roofs.png` et `roofs-2x.png` : deux bâtiments sur deux niveaux.
- `apercus/roofs-all.png` et `roofs-all-2x.png` : les 112 modules, assemblés en 16 toits, avec leurs cartes et journaux de contrôle moteur.

Depuis la racine du dépôt :

```text
python scripts/build_capital_roofs.py --install
python scripts/validate_capital_roofs.py
python scripts/validate_capital_roofs.py --scale 2
python scripts/validate_capital_roofs.py --all
python scripts/validate_capital_roofs.py --all --scale 2
python -m pytest scripts/tests/test_build_capital_roofs.py -q
python scripts/check_hd_assets.py
```

Le contrôle porte sur le chargement, le rendu, les dimensions et les ancres. Il ne constitue pas un essai de déplacement en partie ni une validation artistique par l’utilisateur. Le kit initial de 112 pièces droites est complété par les raccords L, T et X : voir `JONCTIONS.md` pour le catalogue de 598 pièces et les règles de placement.

Résultats : six tests ciblés réussis, 112 images et ancres installées identiques à la scène testée, 34 entrées antérieures du kit conservées. Les 112 noms figurent dans la carte complète. Les deux cartes sont construites par les gestes de l’éditeur puis contrôlées et rendues en ×1 et ×2 : zéro erreur moteur ; seul avertissement, l’absence du catalogue de traduction dans les données de test isolées. Budget Capitale : 7,9 Mio. Résumé machine : `validation.json`. Cartes livrées également dans `Source/Test/Fixtures/Storeys/`.

## Extension L, T et X — 23 septembre 2026

216 modules L (quatre orientations), 216 modules T (quatre branches possibles) et 54 modules X, pour des ailes de largeur identique de 2 à 5 cases. Les pans internes de la jonction sont retirés de l’union géométrique pour éviter qu’ils dépassent sous une rive. Les 112 pièces droites sont préservées ; les anciens PNG des L sont conservés dans `BeforeJunctions/Scene/`.

20 tests ciblés couvrent notamment les emprises, la connexité des ailes, l’absence de découpe accidentelle et la suppression des faces internes. Les aperçus `roofs-junctions*` montrent toutes les nouvelles variantes ; `roofs-buildings*` les montre sur des façades à deux niveaux. Une racine autonome d’essai de ces bâtiments est conservée dans `EngineJunctions/`.

Bilan de l’extension : 598 PNG installés identiques aux PNG de contrôle, ancres identiques ; les cartes couvrent les 598 identifiants. Les 112 modules droits conservent leur manifeste et le hash de leur source calibrée. Budget Capitale après extension : 34,5 Mio. Résumé machine : `validation-junctions.json`.
