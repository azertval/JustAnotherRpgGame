+++
id = "LOT-78"
titre = "Désambiguïsation des numéros de lots hérités"
version = "0.0.0"
filiere = "standard"
statut = "livre"
taille = "S"
resume = "Tout renvoi `LOT-NN` d'une spécification désigne sans ambiguïté un lot de ce programme ou, préfixé `LOT-H-`, un lot hérité — et le lint empêche l'ambiguïté de se reformer."
prerequis = []
livrables = [
  "**201 renvois** préfixés en `LOT-H-NN` dans dix fichiers de spécification.",
  "La **convention** écrite en tête de `specifications.md`.",
  "La **règle 12** de `scripts/lint_lots.py` : tout `LOT-NN` d'une spécification désigne un lot existant de ce programme.",
]
criteres = [
  "Aucun `LOT-NN` d'une spécification ne désigne un lot hérité sans préfixe `LOT-H-`.",
  "Le lint **refuse** un renvoi vers un numéro de lot inexistant — vérifié en injectant `LOT-99` dans `vision.md` et en observant l'échec, puis en restaurant.",
  "La convention est écrite là où un rédacteur la lira : en tête de `specifications.md`.",
  "Doxygen reste vert, sans ancre en double avec l'archive (qui reste hors `INPUT`).",
]
+++

## Pourquoi

Rendre non ambigu tout renvoi `LOT-NN` d'une spécification, et empêcher l'ambiguïté de se reformer.

Ce dépôt est dérivé d'un jeu de plateforme livré après **74 lots**. Les
deux numérotations repartent de `LOT-01`. Tant que le programme RPG s'arrêtait à `LOT-29`, on
pouvait écrire que les deux ensembles ne se croisaient jamais. La feuille de route atteignant
`LOT-84`, la plage héritée est **entièrement recouverte** : `LOT-54` désigne désormais un atelier
pixel art livré *et* le lot Magicien à faire, et rien ne dit lequel.

C'est le pire genre de défaut : **ambigu sans être cassé**. Ni le lint d'exigences ni Doxygen ne le
signalent, et seul un lecteur qui connaît les deux programmes peut trancher.

## Périmètre

**201 renvois préfixés** en `LOT-H-NN`, dans **dix** fichiers de spécification :

| Fichier | Renvois hérités |
|---|---|
| `editeur-niveaux.md` | 42 |
| `rendu-technique.md` | 34 |
| `interface-ihm.md` | 26 |
| `decors.md` | 21 |
| `gameplay.md` | 20 |
| `exigences-non-fonctionnelles.md` | 17 |
| `niveaux.md` | 15 |
| `controles.md` | 13 |
| `architecture.md` | 9 |
| `conventions.md` | 4 |

Les **49 renvois restants** désignent de vrais lots de ce programme et gardent leur écriture nue :
`vision.md`, `exploration.md`, les cinq documents du [LOT-77](LOT-77-specification-rpg.md), plus les renvois au
`LOT-01`, au `LOT-04` (format v3), au `LOT-05` (modes de jeu) et au `LOT-07` (tri par Y).

S'y ajoutent la **convention** écrite en tête de `specifications.md`, et la **règle 12** de
`scripts/lint_lots.py`.

## Conception

### Comment la classification a été faite

Renvoi par renvoi, par couple **(fichier, numéro)** — pas fichier par fichier. C'était nécessaire :
`rendu-technique.md` cite le `LOT-07` **courant** (« l'ordre des calques reste souverain. Concrétisé
en `LOT-07` », le tri par Y) et le `LOT-08` **hérité** (« une caméra fixe cadrant le tableau depuis
`LOT-08` », vocabulaire de plateforme) à onze lignes d'écart.

Deux signaux ont suffi dans la quasi-totalité des cas, tous deux vérifiés contre les titres des
epics archivés (retirés du dépôt au `LOT-88`) :

- **Le temps du verbe.** Aucun lot du programme RPG au-delà du `LOT-08` n'est commencé. « Concrétisé
  en », « Depuis le », « Introduit en », « livré » ne peuvent donc désigner que l'hérité.
- **Le vocabulaire.** « Tableau », « salle », « plateforme », « saut », « dash », « parallaxe »,
  « atelier pixel art » appartiennent au jeu d'origine ; « région », « fiche », « d20 », « case »
  au RPG.

Trois cas ont demandé une vérification directe : `architecture.md:20` (`LOT-05` = modes de jeu,
**courant**), `rendu-technique.md:22` (`LOT-08` = caméra cadrant un tableau, **hérité**) et les trois
`LOT-27` d'`editeur-niveaux.md` (palette organisée par catégories, **hérités** — le `LOT-27` courant
est le *vertical slice*).

### Ce que la règle 12 empêche

`scripts/lint_lots.py` refuse désormais tout `LOT-NN` d'une spécification qui ne désigne pas un lot
**existant de ce programme** — livré (il a son dossier) ou inscrit à la feuille de route. Un renvoi
à l'hérité doit porter le préfixe.

Sans cette règle, le travail ci-dessus se déferait au premier renvoi ajouté de mémoire. Et il
faudrait le refaire en entier, puisque rien ne dirait lesquels des renvois ont déjà été classés.

> **Un défaut du lint a été trouvé en le vérifiant.** La règle 12 a d'abord été écrite avec une
> expression régulière contenant un caractère de contrôle invisible, qui l'empêchait de jamais
> correspondre : le lint passait au vert sur un renvoi délibérément faux. Un garde-fou qu'on n'a pas
> vu échouer n'est pas un garde-fou — la règle n'a été considérée acquise qu'après avoir observé
> `vision.md:98 cite LOT-99, qui n'existe pas dans ce programme`.

## Bilan

Statut : **fait**. Vérification automatisée : `scripts/lint_lots.py` vert, règle 12 comprise ; `scripts/checks/lint_exigences.py` vert ; Doxygen vert ; tous les critères d'acceptation sont cochés dans l'epic d'origine. Ce lot est prérequis du démarrage de tout lot de la filière.
