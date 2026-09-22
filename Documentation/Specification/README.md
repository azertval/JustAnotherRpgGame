# Spécifications

Les spécifications disent **quoi** faire, et surtout **pourquoi** — indépendamment de la façon dont
le code s'y prend. C'est la partie de la documentation qui **engage** : le [guide](../Guide/README.md)
peut être réécrit quand le code change, une exigence ne change que par une décision.

Chaque affirmation qui engage porte un identifiant `EX-…`, cité ensuite par les
[lots](../../Planning/README.md) qui la réalisent, par le code qui l'applique et par les tests qui
la vérifient. C'est ce fil — exigence → lot → code → test — qui permet de répondre à « pourquoi le
moteur fait-il ça ? » autrement que par « parce que c'est écrit comme ça ».

## Les documents

Ils se lisent dans cet ordre : du périmètre général vers les détails techniques, puis les
conventions. **Cet ordre-ci est la seule source d'ordre** — le menu du site et les liens
« précédent / suivant » en découlent, et les noms de fichiers ne portent aucun numéro.

### Ce que le jeu est

| Document | Ce qu'il fixe |
|---|---|
| [Vision & périmètre](vision.md) | Le concept, les décisions de cadrage, les deux identités visuelles, ce qui est **hors** périmètre |
| [Gameplay](gameplay.md) | Ce que toute carte et tout écran supposent : le monde, la partie, la sauvegarde |

### Les règles

| Document | Ce qu'il fixe |
|---|---|
| [Règles d20](regles-d20.md) | Le dé, le jet, la difficulté, l'avantage, les conditions |
| [Personnage et progression](rpg.md) | Caractéristiques, espèce, classe, niveaux, fiche |
| [Combat tactique](combat.md) | Bascule, initiative, espace, attaque et dégâts, agonie, adversaire |
| [Inventaire et économie](inventaire.md) | Possession, emplacements d'équipement, encombrement, monnaie |

### Le monde et sa matière

| Document | Ce qu'il fixe |
|---|---|
| [Contenu et données](contenu.md) | Les catalogues JSON, leur schéma, leur provenance, la localisation |
| [Cartes & format](niveaux.md) | Le format de carte versionné, ses couches, ses entités |
| [Exploration](exploration.md) | Le déplacement temps réel, les portails, le déclenchement des rencontres |

### Ce qu'on voit et ce qu'on touche

| Document | Ce qu'il fixe |
|---|---|
| [Contrôles & entrées](controles.md) | Clavier, souris, manette, et la traduction en actions logiques |
| [Rendu & cible technique](rendu-technique.md) | La projection isométrique, l'ordre de tri, la cible de performance |
| [Interface utilisateur (IHM)](interface-ihm.md) | Les écrans du jeu, le HUD, la charte, l'outil interne qu'est l'éditeur |
| [Éditeur de cartes](editeur-niveaux.md) | Le document, les gestes, le contrôle du contenu |

### Ce qui tient le tout

| Document | Ce qu'il fixe |
|---|---|
| [Exigences non fonctionnelles](exigences-non-fonctionnelles.md) | Déterminisme, performance, qualité, outillage |
| [Architecture (décisions dimensionnantes)](architecture.md) | Le sens des dépendances, l'ECS, la frontière simulation ↔ présentation |
| [Conventions de code](conventions.md) | Nommage, style, erreurs, tests |
| [Exigences retirées](exigences-retirees.md) | Ce qui a été **abandonné**, et pourquoi — pour que rien ne revienne par inadvertance |

## Lire une exigence

Une exigence se déclare par une puce qui s'ouvre sur son identifiant en gras, suivi d'un tiret
cadratin et de son énoncé — et **nulle part ailleurs** que dans la page de son domaine. La forme
exacte est donnée par [Écrire la documentation](../Guide/guide-documentation.md) ; les puces de
[Combat tactique](combat.md) en sont l'exemple vivant.

La famille dit le domaine, et une seule page la déclare. Le numéro, lui, ne se réutilise jamais :

| Famille | Domaine | Déclarée dans |
|---|---|---|
| `EX-VIS` | Vision, périmètre, identités visuelles | [vision.md](vision.md) |
| `EX-GP` | Gameplay, monde, partie, sauvegarde | [gameplay.md](gameplay.md) |
| `EX-REG` | Règles d20 | [regles-d20.md](regles-d20.md) |
| `EX-RPG` | Personnage et progression | [rpg.md](rpg.md) |
| `EX-CBT` | Combat tactique | [combat.md](combat.md) |
| `EX-INV` | Inventaire et économie | [inventaire.md](inventaire.md) |
| `EX-CNT` | Contenu et données | [contenu.md](contenu.md) |
| `EX-LVL` | Cartes et format | [niveaux.md](niveaux.md) |
| `EX-EXP` | Exploration | [exploration.md](exploration.md) |
| `EX-CTRL` | Contrôles et entrées | [controles.md](controles.md) |
| `EX-REN` | Rendu et cible technique | [rendu-technique.md](rendu-technique.md) |
| `EX-IHM` | Interface, écrans, charte | [interface-ihm.md](interface-ihm.md) |
| `EX-EDIT` | Éditeur de cartes | [editeur-niveaux.md](editeur-niveaux.md) |
| `EX-NFR`, `EX-BUILD` | Qualité, performance, outillage | [exigences-non-fonctionnelles.md](exigences-non-fonctionnelles.md) |
| `EX-ARCH` | Architecture | [architecture.md](architecture.md) |
| `EX-DEC`, `EX-IA` | Familles **closes** : leurs exigences sont toutes retirées | [exigences-retirees.md](exigences-retirees.md) |

Une exigence bien écrite dit **ce qui casse si on fait autrement**. C'est la règle de rédaction de
cette partie : la phrase qui suit le tiret énonce l'engagement, et celle d'après dit le dégât que
la solution naïve aurait causé. Sans cette seconde phrase, l'exigence sera un jour « simplifiée »
par quelqu'un qui ne saura pas ce qu'elle protégeait.

Le statut en tête de chaque page dit où en est sa **réalisation** — ce qui est livré, ce qui reste
à faire, et quel lot le porte. Une exigence écrite n'est pas une exigence tenue.

## Les maquettes

Une maquette est un **dessin d'intention** : ce qu'un écran ou une mécanique **doit** être, avant
que le code ne le fasse — et donc ce contre quoi on juge le résultat. Elles vivent dans
`Documentation/Specification/maquettes/`, en SVG écrit à la main, et la page du domaine les montre
là où une phrase ne suffit pas à cadrer une disposition : la
[grille de combat](combat.md), le [HUD et les écrans](interface-ihm.md), la
[projection isométrique](rendu-technique.md), les [couches d'une carte](niveaux.md),
l'[enchaînement portail → rencontre](exploration.md) et la
[fenêtre de l'éditeur](editeur-niveaux.md).

> **Attention** — Une maquette n'est pas une capture : elle montre l'**intention**, pas l'état du
> code. Ce que le jeu affiche réellement se photographie, et ces captures-là sont dans le
> [guide](../Guide/README.md). Aucune image du corpus (les livres de Tanares) ne s'affiche ni ne se
> décalque ici.

## Vérifier

```
python scripts/lint_exigences.py          # déclarée une fois, citée au moins une fois ailleurs
python scripts/lint_exigences.py --next   # le prochain numéro libre d'une famille
python Documentation/outils/lint_docs.py  # liens, ancres, images, pages orphelines
```

Ajouter une exigence, c'est donc : la déclarer dans la page de son domaine avec le prochain numéro
libre, et la **citer** là où elle s'applique — un lot, une autre exigence, une page du guide. Une
exigence que rien ne cite est une exigence que rien ne réalise : le lint la refuse.

Retirer une exigence, c'est la déplacer dans [les exigences retirées](exigences-retirees.md) avec
la raison de son abandon. Son numéro reste mort : le réemployer ferait pointer d'anciens lots et
d'anciens tests vers une règle qui n'est plus la leur.
