# L'image de la sous-zone pour l'onglet « Carte »

`arena-of-fate.jpg` : la carte de l'Arena of Fate, sous-zone d'Arenarea (D-16) **rendue** par
l'éditeur, telle que le jeu la dessine (`LOT-121`), en attendant une carte peinte par l'auteur, qui
viendra avec la carte définitive de la zone.

```
LevelEditor --render central-empire/capital/arenarea/arena-of-fate --canvas 1920x1080 --layers floors,relief --output Source/Elements/Assets/Regions/central-empire/capital/arenarea/arena-of-fate/Map/arena-of-fate.jpg
```

La commande écrit aussi la **grille** de l'image (`grid {"origin": …, "column": …, "row": …}`) :
elle se recopie dans `Maps/world-maps.json`, à côté du chemin de l'image, et c'est par elle que
l'onglet pose le héros et les repères sur la carte. Toute retouche de la carte de niveau se
re-rend, et l'image repart dans le kit de la zone (`scripts/release/publish_asset_kit.py`).
