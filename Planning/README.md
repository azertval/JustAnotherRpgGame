# Planification

La planification du jeu, de la démo `0.0.1` au monde complet `1.0.0`. Ce dossier est la **source
unique** de ce qui reste à faire : les versions, leurs lots, leurs livrables, leurs maquettes, et
les référentiels de contenu sur lesquels ils s'appuient. Le site de planification le **lit** ; rien
ne s'édite ailleurs.

Trois choses ont changé le 20 septembre 2026, et ce dossier en découle :
le jeu quitte le **pixel art** pour la **2D HD** ; le référentiel `0.1.0` n'est plus le monde entier
mais **une seule région**, l'Empire central ; et la `0.0.1` n'est plus un *vertical slice* ambitieux
mais une **démo basique** — deux quartiers et un donjon, une quête. Le détail est dans
[les décisions](vision/decisions.md).

## S'y retrouver

| Dossier | Ce qu'il porte |
|---|---|
| [`vision/`](vision/README.md) | La trajectoire de `0.0.1` à `1.0.0`, les décisions de planification, les risques |
| [`versions/`](versions/README.md) | Une version par dossier : son périmètre, ses **lots** (une fiche chacun), ses **maquettes** |
| [`referentiels/`](referentiels/README.md) | Ce que les livres donnent à faire : zones, PNJ nommés, peuples, monstres, classes, espèces, régions |
| [`standards/`](standards/README.md) | Les règles communes : le style 2D HD, l'arborescence des assets, le gabarit d'un lot, ce que « livré » veut dire |
| `outils/` | Le lint du dossier et le générateur du site (Python, bibliothèque standard seule) |
| `site/` | La feuille de style et le script du site |

## Les gestes

```
python Planning/outils/lint_planning.py                              # le dossier est-il cohérent ?
python Planning/outils/build_planning_site.py --out build/planning-site   # le site, en local
```

- **Démarrer un lot** : passer sa fiche à `statut = "en-cours"`.
- **Livrer un lot** : `statut = "livre"`, et le numéro de PR en fin de fiche. L'avancement, l'ordre
  calculé et le « prochain lot » du site suivent.
- **Ajouter un lot** : copier le [gabarit](standards/gabarit-lot.md) dans le dossier `lots/` de sa
  version, avec le prochain numéro libre.
- **Détailler une version prévisionnelle** : lui donner un dossier, des objectifs, des critères de
  sortie et des lots, puis passer sa `nature` à `detaillee` dans
  [`versions.toml`](versions/versions.toml).

## Le site

Publié avec la documentation, sous `planning/` du site du projet, à chaque intégration sur `main`.
Il montre la trajectoire, l'avancement de la version en cours, l'ordre calculé des lots, le graphe
de leurs prérequis, les fiches, les maquettes et les référentiels.

## Et l'ancienne feuille de route ?

`Documentation/Lot/roadmap.md` est **figée** : elle reste l'histoire des lots `LOT-01` à `LOT-96`,
dont les dossiers gardent ce que leur réalisation a tranché. Les lots qu'elle annonçait et qui
n'ont pas été livrés sont **repris** ici sous de nouveaux numéros, à partir de `LOT-100` ; chaque
fiche dit ce qu'elle reprend (`reprend = [...]`), et la
[table de correspondance](vision/correspondance-ancienne-roadmap.md) donne la vue d'ensemble.
La feuille de route de l'**éditeur** (`Documentation/Editeur/`) n'est pas concernée : l'outil
avance à part.
