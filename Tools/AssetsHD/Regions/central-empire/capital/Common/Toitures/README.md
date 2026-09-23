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

Le contrôle porte sur le chargement, le rendu, les dimensions et les ancres. Il ne constitue pas un essai de déplacement en partie ni une validation artistique par l’utilisateur. Ce kit couvre les toits rectangulaires à deux pans ; les intersections de toits en L avec noues ne font pas partie de ces 112 pièces.

Résultats : six tests ciblés réussis, 112 images et ancres installées identiques à la scène testée, 34 entrées antérieures du kit conservées. Les 112 noms figurent dans la carte complète. Les deux cartes sont construites par les gestes de l’éditeur puis contrôlées et rendues en ×1 et ×2 : zéro erreur moteur ; seul avertissement, l’absence du catalogue de traduction dans les données de test isolées. Budget Capitale : 7,9 Mio. Résumé machine : `validation.json`. Cartes livrées également dans `Source/Test/Fixtures/Storeys/`.
