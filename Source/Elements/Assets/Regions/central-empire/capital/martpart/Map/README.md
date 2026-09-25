# L'image de la zone pour l'onglet « Carte »

`martpart.jpg` : la carte de Martpart **rendue** par l'éditeur, telle que le jeu la dessine
(`LOT-121`), en attendant une carte peinte par l'auteur, qui viendra avec la carte définitive de la
zone.

```
LevelEditor --render central-empire/capital/martpart --canvas 1920x1080 --layers floors,relief --output Source/Elements/Assets/Regions/central-empire/capital/martpart/Map/martpart.jpg
```

La commande écrit aussi la **grille** de l'image (`grid {"origin": …, "column": …, "row": …}`) :
elle se recopie dans `Maps/world-maps.json`, à côté du chemin de l'image, et c'est par elle que
l'onglet pose le héros et les repères sur la carte. Toute retouche de la carte de niveau se
re-rend, et l'image repart dans le kit de la zone (`scripts/release/publish_asset_kit.py`).
