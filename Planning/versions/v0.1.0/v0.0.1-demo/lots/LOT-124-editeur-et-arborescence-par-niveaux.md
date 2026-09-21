+++
id = "LOT-124"
titre = "L'éditeur et l'arborescence par niveaux"
version = "0.0.1"
filiere = "editeur"
statut = "a-faire"
taille = "L"
resume = "Une carte puise dans son lieu **et** dans ses niveaux communs — sous-zone, zone, ville, région, monde — et l'éditeur le montre, le contrôle et le réécrit."
prerequis = ["LOT-102", "LOT-123"]
livrables = [
  "`core` : un **catalogue de pièces résolu** pour un lieu hiérarchique (`central-empire/capital/arenarea/arena-of-fate`), qui empile les manifestes du plus propre au plus commun et garde, pour chaque pièce, son dossier d'origine ; le fichier vient du champ `file` du manifeste, plus du nom.",
  "`LevelDraft`, `PieceCatalog`, `CollisionDerivation` et `--check` lisent ce catalogue : une pièce du kit de la Capitale posée sur Arenarea déduit sa collision et passe le contrôle.",
  "La palette groupée par **niveau** (« Arena of Fate », « Arenarea », « Capital », « Central Empire », « World »), avec la recherche à travers tous ; une pièce propre qui **masque** une pièce commune de même clé est signalée.",
  "« New map » propose l'arbre des lieux, et range la carte sous le même chemin dans `Levels/`.",
  "`--replace-piece`, `--who-cites piece` et `--change-scene` suivent les niveaux : **promouvoir** une pièce au commun ne réécrit que les cartes qui changent vraiment.",
  "Les préfabriqués et les modèles rangés par niveau, proposés à tout lieu qui en descend.",
]
criteres = [
  "Une carte de test sous `capital/arenarea/` cite une pièce de chacun des quatre niveaux ; `--check` passe, `--render` la montre, l'essai dans le jeu l'affiche.",
  "Une pièce déplacée de la zone vers la ville, sous la même clé, ne change **aucun** octet des cartes ; sous une autre clé, `--replace-piece` réécrit les cartes qui la citent et elles seules.",
  "`git grep '\"Scene/\"' Source/Editor Source/HMI/Graphics` ne trouve plus de chemin d'asset composé à la main.",
  "Un scénario `--apply` couvre la pose d'une pièce commune ; l'auteur a composé une rue de douze cases avec le kit seul (critère du LOT-105) **dans l'éditeur**.",
]
+++

## Pourquoi

L'[arborescence des assets](../../../../standards/arborescence-assets.md) repose sur une règle — du
commun vers le propre — que l'éditeur ne sait pas jouer : un brouillon porte **un** manifeste, la
palette **un** dossier, et `--check` cherche chaque pièce dans ce seul manifeste
([audit](../../../../standards/audit-editeur.md), constats A1 à A5). Sans ce lot, chaque quartier
devrait recopier le pavé de la Capitale dans sa planche : exactement ce que le standard interdit.

## Périmètre

Le [LOT-102](LOT-102-table-rase-assets-et-cartes.md) pose la résolution d'une clé côté **moteur**
(son point 4) ; ce lot la fait lire par tout ce qui, dans `Core` et dans l'éditeur, supposait un
manifeste unique. Le livrable « palette » que portait le LOT-104 est **ici**.

**Pas dedans** : l'affichage HD des pièces (LOT-125) ; l'installation des assets (LOT-104).

## Risques

- `appearance.json` (type de tuile → pièce par défaut) n'a pas de repli non plus : même règle, le
  plus propre gagne.
- Le format de carte ne change pas : une couche garde **un** `scene`, qui devient un chemin. Si une
  carte devait nommer le niveau d'une pièce, ce serait une révision de format — à refuser.
