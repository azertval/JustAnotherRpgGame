# Cahier de recette de l'éditeur, à la main

Le [LOT-127](../../lots/LOT-127-recette-de-l-editeur-a-la-main.md) passe à la main, sans
script, les gestes des lots de l'éditeur livrés avec la mention « vérification à la souris due ».
Il y a une ligne par geste.

L'auteur a fait cette recette le **24 septembre 2026**, à la souris et au clavier, en construisant
les cartes 2D HD. Il y a posé les pièces installées de la Capitale et d'Arenarea. La recette a été
éprouvée ensuite dans le jeu (`F5`). **Toutes les lignes sont OK** : aucune anomalie n'est à
inscrire au [LOT-166](../../../v0.0.4-faubourgs/lots/LOT-166-dettes-de-l-atelier.md).

Les cartes des gestes d'origine (Martpart, Colisée de l'ancien style) sont parties à la table rase
du [LOT-102](../../lots/LOT-102-table-rase-assets-et-cartes.md). Chaque geste a donc été fait sur
une carte HD, avec les pièces de son lieu.

## Le critère du lot : une salle fermée, montée à la souris

| # | Geste | Résultat |
|---|---|---|
| R-01 | Monter à la souris seule une salle fermée : sols, murs, angles, une porte, une entité, une zone | ✔ OK |
| R-02 | L'essayer dans le jeu (`F5`) | ✔ OK |

## Pièces : poser, glisser, gommer ([LOT-EDITOR-03](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-03-pieces.md))

| # | Geste | Résultat |
|---|---|---|
| R-03 | Choisir une pièce de 2 × 1 dans la palette et la poser | ✔ OK |
| R-04 | Glisser sur sa deuxième case : rien ne bouge | ✔ OK |
| R-05 | La gommer, pièce entière, par l'une de ses cases | ✔ OK |
| R-06 | Couche collision active, peindre un mur sur une rue : la case forcée paraît en magenta | ✔ OK |
| R-07 | La gommer : le magenta part, la case reprend la déduction | ✔ OK |

## Outils : ligne, miroir, pipette, seau, mesure, notes ([LOT-EDITOR-04](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-04-outils.md))

| # | Geste | Résultat |
|---|---|---|
| R-08 | `M` au-dessus d'un angle, puis `L` et une ligne de mur : le reflet paraît | ✔ OK |
| R-09 | `Alt` + clic sur une façade : la palette la montre (pipette) | ✔ OK |
| R-10 | `G` sur une place avec une variante de sol, puis un seul `Ctrl+Z` | ✔ OK |
| R-11 | `D` entre deux lanternes : la mesure en cases et en pieds | ✔ OK |
| R-12 | `N` sur une case, puis rouvrir la carte : la pastille de note revient | ✔ OK |
| R-13 | `Shift+P` sur une rue | ✔ OK |

## Entités et zones ([LOT-EDITOR-05](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-05-entites-zones.md))

| # | Geste | Résultat |
|---|---|---|
| R-14 | Outil Entité (`O`) : cliquer dans une zone de combat, tirer sa poignée ; le verdict se met à jour ; `Ctrl+Z` | ✔ OK |
| R-15 | `Maj` + clic sur deux entités, puis les glisser ensemble | ✔ OK |
| R-16 | Choisir « zone », tirer un rectangle, puis `Z` et peindre des cases, `Ctrl` + glisser pour en gommer | ✔ OK |
| R-17 | Choisir « route », tirer un segment, puis `Z` et cliquer pour le prolonger | ✔ OK |
| R-18 | Taper « portal » dans le filtre de la liste des familles | ✔ OK |
| R-19 | Étiquettes des îlots et des portails visibles, PNJ par leur figurine, sans marqueur par-dessus | ✔ OK |

## Faire une carte sans script ([LOT-EDITOR-06](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-06-fin-des-scripts.md))

| # | Geste | Résultat |
|---|---|---|
| R-20 | Lancer `LevelEditor` sans argument : le titre montre `…\Source\Elements` | ✔ OK |
| R-21 | *New* : une carte d'essai avec son lieu ; poser des pavés, un mur, l'entrée, un coffre | ✔ OK |
| R-22 | `Ctrl+S` : `git status` montre la carte sous `Source/Elements/Levels/` | ✔ OK |
| R-23 | `P` pour l'essai, puis supprimer la carte | ✔ OK |

## Contrôle du contenu ([LOT-EDITOR-07](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-07-controle-contenu.md))

| # | Geste | Résultat |
|---|---|---|
| R-24 | Le dock « Problems » montre le bilan des cartes enregistrées | ✔ OK |
| R-25 | Double-clic sur un constat : sa carte s'ouvre, l'entité est sélectionnée, sa case cernée de magenta | ✔ OK |
| R-26 | Poser un PNJ dans un mur, enregistrer : l'erreur paraît ; `Ctrl+Z`, enregistrer : elle part | ✔ OK |

## Tampons et préfabriqués ([LOT-EDITOR-08](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-08-tampons-prefabriques.md))

| # | Geste | Résultat |
|---|---|---|
| R-27 | Outil Sélection (`S`), tirer autour d'une pièce, `Ctrl+C` : la barre d'état dit ce que porte le tampon | ✔ OK |
| R-28 | `Ctrl+V` sur une case vide, puis un `Ctrl+Z` le retire | ✔ OK |
| R-29 | `Ctrl+Maj+V` : le tampon posé reflété | ✔ OK |
| R-30 | *Edit* › *Save selection as prefab…* : l'onglet **Prefabs** montre sa vignette ; le reposer sur une autre carte | ✔ OK |
| R-31 | *File* › *New map* depuis un modèle : la carte naît avec ses couches et son tampon, 0 erreur | ✔ OK |

## Le monde ([LOT-EDITOR-09](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-09-monde.md))

| # | Geste | Résultat |
|---|---|---|
| R-32 | Ouvrir une seconde carte : un onglet de plus, chacune garde cadrage, outil, historique, étoile | ✔ OK |
| R-33 | Onglet **Graph** : tirer d'une carte à une autre ; les entités à écrire sont montrées, puis les flèches | ✔ OK |
| R-34 | `P` : franchir le portail dans un sens puis dans l'autre | ✔ OK |
| R-35 | Onglet **City** : le plan, les quartiers cadrés ; double-clic, le quartier s'ouvre dans son onglet | ✔ OK |
| R-36 | *Map* › *Map properties…* : région, ambiance, état ; **Thumbnails** et le filtre par état | ✔ OK |
| R-37 | Fermer un onglet modifié : la question porte sur **sa** carte | ✔ OK |

## L'essai dans le jeu ([LOT-EDITOR-10](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-10-essai-jeu.md))

| # | Geste | Résultat |
|---|---|---|
| R-38 | *Run in game* (`F5`) : le jeu s'ouvre sur la carte, à la case voulue, dans l'état de partie voulu | ✔ OK |
| R-39 | Un brouillon non enregistré est servi avant la carte du dépôt | ✔ OK |

## Les gestes sans fenêtre, refaits à la main ([LOT-EDITOR-13](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-13-sans-fenetre.md))

| # | Geste | Résultat |
|---|---|---|
| R-40 | Outil Entité : poser un coffre, le glisser, tirer une zone de combat | ✔ OK |
| R-41 | `Maj` + clic sur une deuxième entité, déplacer le groupe, `Suppr` | ✔ OK |
| R-42 | Outil Sélection : `Ctrl+C` puis `Ctrl+V` sur la collision | ✔ OK |
| R-43 | Choisir une pièce dans la palette : la couche active suit | ✔ OK |
| R-44 | Chaque geste ci-dessus se défait d'un `Ctrl+Z` | ✔ OK |

## Renommer et remplacer ([LOT-EDITOR-14](../../../../v0.0.0/v0.0.0-fondation/lots/LOT-EDITOR-14-renommer-remplacer.md))

| # | Geste | Résultat |
|---|---|---|
| R-45 | *Map* › *Who cites this map?* : la liste ; double-clic, la carte qui cite s'ouvre, l'entité sélectionnée | ✔ OK |
| R-46 | *Replace piece…* sur la carte : un `Ctrl+Z` le défait | ✔ OK |
| R-47 | *Change sheet…* vers une planche sans correspondance : la table montre les trous, *OK* reste grisé | ✔ OK |
| R-48 | *File* › *Rename*, puis retour au nom d'origine : le dock « Problems » reste à 0 erreur | ✔ OK |
