# World/cities/

Le **plan** d'une ville jouable : ses quartiers, celui par lequel on entre, et pour chaque quartier
fermé la carte où se tient la sentinelle qui en garde la porte (`LOT-96`, `core::loadCityPlan`).

> **Vidé au `LOT-102`.** `capital.json` nommait douze quartiers et deux cartes, que la table rase a
> emportées ; le format exige qu'un départ ait sa carte, et une ville dont aucun quartier ne
> s'ouvre n'est pas une ville. Le plan de la Capitale revient au `LOT-121`, sur les cartes 2D HD.
> Sans lui, le jeu le dit sans planter — « La ville de départ ne s'ouvre pas » (`EX-NFR-040`).

Ce `README.md` est le gardien du dossier : git ne garde pas un dossier vide, et `check_rpg_data.py`
comme le jeu cherchent `World/cities/`.
