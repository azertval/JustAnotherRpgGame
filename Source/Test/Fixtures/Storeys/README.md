# Toitures de la Capitale — LOT-129

`roofs.json` : deux îlots de murs sur deux niveaux, toitures dans les deux orientations.

`roofs-all.json` : les 112 modules en 16 assemblages, deux axes, profondeurs 2 à 5, longueurs 5 et 1. Les toits sont présentés seuls à l’étage 1 pour examiner toutes les extrémités.

Cartes construites par les gestes de LevelEditor, avec collisions calculées par l’éditeur. Utilisent la scène `capital` installée. Reconstruction et rendus : `scripts/validate_capital_roofs.py`, options `--all` et `--scale 2`. Les données et PNG de contrôle sont conservés dans `Tools/AssetsHD/Regions/central-empire/capital/Common/Toitures/apercus/`.

`roofs-l.json` : les quatre orientations de L, de largeur 2 à 5.

`roofs-junctions.json` : les quatre orientations de T et les croisements, de largeur 2 à 5.

La démonstration avec façades (`--showcase`) utilise des supports de contrôle réservés à cette carte ; sa racine complète est dans `Tools/AssetsHD/Regions/central-empire/capital/Common/Toitures/EngineJunctions/`.
