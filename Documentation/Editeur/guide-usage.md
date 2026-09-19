# Faire une carte dans l'éditeur {#guide-usage-editeur}

> Guide d'usage de l'auteur, écrit au [LOT-EDITOR-06](@ref lot-editor-06). Depuis ce lot,
> **l'éditeur fait foi** pour les cartes faites à la main (décision D4) : aucun script ne les écrit
> plus. Le fonctionnement interne de l'outil est dans @ref guide-editeur ; ses commandes, dans le
> README du module, `Source/Editor/README.md`.

Une carte se fait en sept temps : la créer avec son lieu, poser le sol, dresser le relief, placer
l'entrée, poser les entités, l'essayer, l'enregistrer et la contrôler. Exemple suivi : une petite
échoppe de Martpart, 12 × 8 cases.

## 0. Ouvrir l'éditeur sur le dépôt

Construire (`scripts/build.ps1`), puis lancer `build\ninja\bin\LevelEditor.exe`. L'éditeur ouvre
les données de **l'arbre des sources qui l'a construit** — `Source/Elements` —, et non la copie
que la construction refait à côté de l'exécutable : ce qu'on enregistre arrive dans le dépôt, et la
construction suivante ne l'écrase pas. Le titre de la fenêtre dit quel dossier est ouvert ;
`--data <dossier>` en ouvre un autre.

## 1. Créer la carte, avec son lieu

Panneau **Maps**, onglet *List*, bouton **New** : un nom (`echoppe`), une taille en cases, et un
**lieu** — la planche dont la carte prendra ses pièces (`martpart`, `coliseum`…). La carte naît
comme les cartes livrées : une couche de sol `sol` qui nomme le lieu, une couche de décor
`relief`, et une collision déduite où tout, encore vide, arrête la vue — sauf l'entrée, au coin
bas gauche, posée sur une case de terre. Double-cliquer la carte dans la liste l'ouvre.

Sans lieu (« none »), on peint des types en couleurs sur une grille unique : c'est le repli des
cartes générées, pas la façon de faire une carte du jeu.

Une carte d'un quartier va dans un sous-dossier (`capital/…`) : la créer, puis déplacer son fichier
dans `Source/Elements/Levels/capital/`. Le renommage propagé viendra au `LOT-EDITOR-14`.

## 2. Poser le sol

Panneau **Palette**, onglet *Pieces* : la planche du lieu, groupée par classe, avec une recherche.
Une pièce de sol va d'elle-même sur la couche `sol`. Les outils, à leur touche :

| Touche | Outil | Pour |
|---|---|---|
| `R` | rectangle | le sol d'une pièce, d'une rue, d'une place |
| `B` | pinceau | les variantes de pavé, une case à la fois |
| `G` | seau | remplir une surface d'un seul sol |
| `L` | ligne | une rangée droite |
| `E` | gomme | retirer une pièce entière, emprise comprise |
| `I`, ou `Alt` + clic | pipette | reprendre la pièce d'une case |

Un geste, du clic au relâchement, se défait d'un `Ctrl+Z`. La collision suit chaque geste : une case
qui reçoit un sol devient franchissable.

## 3. Dresser le relief

Toujours dans *Pieces* : murs, fenêtres, portes, angles, lanternes, étals. Une pièce debout va sur
`relief`. Une pièce **large** (une devanture, un grand étal) occupe deux cases, depuis la case où
on la pose ; poser une pièce sur une emprise déjà prise retire celle qui y était. Le **miroir**
(`M`, par la case survolée) pose la jumelle de chaque pièce de l'autre côté d'un axe : une façade
symétrique se dresse d'un seul trait.

**F9** bascule entre l'isométrie (ce que le jeu montrera) et la vue à plat, où l'on lit les types
et la collision ; **F8** rend le relief transparent pour voir le sol dessous.

## 4. Placer l'entrée

Panneau **Layers** : choisir la couche *Collision*, puis dans *Palette*, onglet *Types*, le
marqueur **Entry**, et cliquer la case où le héros apparaît. Une carte n'a qu'une entrée : la
reposer la déplace. Peindre la collision ailleurs **force** la case ; la gomme la rend à la
déduction. On ne force qu'en dernier recours : une case forcée ne suit plus les pièces.

## 5. Poser les entités

Outil **Entité** (`O`), puis une famille dans la liste *Place* du panneau **Entities** : un PNJ
(son dialogue, sa figurine), un coffre, un point d'arrivée (`spawnPoint`, nommé), un portail
(`portal` : la carte visée et le point d'arrivée de l'autre côté), une zone de combat qu'on tire
d'un coin à l'autre.
L'inspecteur propose les valeurs que les catalogues connaissent ; ses avertissements disent ce qui
manque, et une zone de combat donne son verdict tactique pendant qu'on la tire. Une entité reçoit
un identifiant (`e12`) qu'elle garde : les quêtes la citeront par `carte#e12`.

Pour relier deux cartes, poser les deux paires : un portail et un point d'arrivée de chaque côté.

## 6. Essayer

**P** lance l'essai depuis l'entrée, **Shift+P** depuis la case survolée : on marche, on passe
les portails. Pour le vrai jeu, dialogues et combats compris, il faudra le `LOT-EDITOR-10`.

## 7. Enregistrer, contrôler, publier

`Ctrl+S` enregistre, après validation : une carte que le jeu refuserait ne s'écrit pas. Puis, dans
un terminal à la racine du dépôt :

```
build\ninja\bin\LevelEditor.exe --check
build\ninja\bin\LevelEditor.exe --render echoppe --scale 0.5 --output echoppe.png
```

`--check` contrôle **toutes** les cartes — format, pièces, collision, identifiants, puis ce qui se
joue : références des entités, rencontres, cases utiles hors d'atteinte, portails sans retour,
clés de traduction — et c'est ce que la CI exige. Dans la fenêtre, le même contrôle remplit le
panneau **Problems**, en bas : au lancement, à chaque enregistrement, et par *Map › Check all
maps*. Un double-clic sur un constat ouvre sa carte, sélectionne l'entité et cerne la case en
magenta. Le panneau lit les cartes **enregistrées** ; les avertissements en direct de la carte
ouverte restent dans le panneau *Entities*.

Le nom d'une carte n'est pas du texte mais une clé, `map.<identifiant>.name` : « New » l'écrit
dans `Source/Elements/Localization/fr.lang` et `en.lang` avec le nom tapé ; il reste à y mettre
le vrai nom, dans chaque langue. `--render` rend la carte en PNG, comme elle rendra dans la PR : la CI publie le
rendu de chaque carte qu'une PR change (artefact `map-renders`). La carte se relit alors en diff :
le fichier est canonique, une retouche ne change que ses cases.

## Retoucher en masse, sans la souris

Ce que la main fait se rejoue par fichier : `LevelEditor --apply gestes.json` appelle les mêmes
fonctions que les outils, dans le même ordre, et ne touche pas au fichier si un geste est refusé.
C'est la façon de faire une retouche relue en diff, ou de répéter un geste sur plusieurs cartes.
Les trois retouches du `LOT-EDITOR-06` en sont des exemples
(`Documentation/Editeur/LOT-EDITOR-06-fin-des-scripts/retouches/`), le format est décrit dans
`Source/Editor/Logic/GestureScript.h`.

## Ce qui ne se fait pas encore dans l'éditeur

- Changer une carte de lieu (sa planche), renommer une carte citée par d'autres : `LOT-EDITOR-14`.
- Copier un ensemble de pièces et d'entités comme un tampon : `LOT-EDITOR-08`.
- Relier deux cartes depuis le graphe du monde : `LOT-EDITOR-09`.
