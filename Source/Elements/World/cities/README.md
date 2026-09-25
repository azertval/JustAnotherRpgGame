# World/cities/

Le **plan** d'une ville jouable : ses quartiers, celui par lequel on entre, et pour chaque quartier
fermé la carte où se tient la sentinelle qui en garde la porte (`LOT-96`, `core::loadCityPlan`).

> **Vidé au `LOT-102`**, puis **réduit à la démo au `LOT-120`.** `capital.json` nommait douze
> quartiers et deux cartes, que la table rase a emportées. Il revient ici **provisoire**
> (`status`, `EX-CNT-032`) : deux quartiers ouverts sur les cartes de principe (`LOT-146`), Martpart
> en quartier de départ, par le point d'arrivée `market-gate` — c'est ce que « Nouvelle partie »
> ouvre. Les dix autres quartiers, leurs portes gardées et leurs sentinelles viennent avec le plan
> complet de la Capitale (`LOT-121`, `0.0.3`) ; `check_rpg_data.py` n'exige un plan complet que
> d'une ville qui ne se dit pas provisoire. Sans plan, le jeu le dit sans planter — « La ville de
> départ ne s'ouvre pas » (`EX-NFR-040`).

Ce `README.md` est le gardien du dossier : git ne garde pas un dossier vide, et `check_rpg_data.py`
comme le jeu cherchent `World/cities/`.
