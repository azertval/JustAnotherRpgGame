+++
id = "LOT-126"
titre = "Ce que la quête demande aux cartes"
version = "0.0.1"
filiere = "editeur"
statut = "en-cours"
taille = "M"
resume = "Tout ce que « Des pommes pour l'arène » pose sur une carte se pose, se voit et se contrôle dans l'éditeur : un PNJ présent selon un drapeau, une porte close, un escalier condamné, un déclencheur."
prerequis = ["LOT-116"]
livrables = [
  "La **condition de présence** du LOT-116 déclarée dans `EntityKinds` pour toute famille : drapeau, opérateur, **valeur** ; l'inspecteur propose les valeurs que la quête déclare.",
  "Le catalogue des drapeaux lit les **quêtes**, plus seulement les dialogues ; `--check` refuse une valeur qu'aucune quête ne déclare, et suit les `carte#id` que les quêtes citent (source `EntityRefs`, prête depuis le `LOT-EDITOR-05`).",
  "Une famille **`prop`** : une pièce du lieu posée comme entité, avec sa condition de présence et la collision de son emprise — les portes de l'arène, closes sous `condamne`.",
  "`portal.sealed` : un portail **condamné**, sans arrivée, que le contrôle accepte et que le graphe des cartes montre en pointillé.",
  "`zone.trigger` : ce qu'une zone déclenche à l'entrée — un dialogue, un drapeau posé, un transfert vers `carte` + arrivée.",
  "Le canevas **sous un état de partie** : un sélecteur de valeurs de drapeaux grise ce qui est absent ; l'essai (`P`, `F5`) part de cet état.",
]
criteres = [
  "Sur une carte de test, le garde et l'enfant paraissent sous `acceptee` et disparaissent sous `enfant-libere`, au canevas comme dans l'essai, sans recharger.",
  "`--check` passe sur une carte qui porte un escalier condamné, et échoue sur la même carte si `sealed` est retiré.",
  "`--check` atteint le vestiaire A par le **transfert** de la zone du parvis : une case que seul un déclencheur dessert n'est pas « inatteignable ».",
  "Aucune ligne de code par famille dans `Source/Editor` : le test bloquant du contrat d'extension reste vert.",
]
+++

## Pourquoi

La [quête de la démo](../quete-demo.md) introduit quatre choses qu'aucune carte n'a jamais portées,
et que l'éditeur ne sait pas écrire ([audit](../../../../standards/audit-editeur.md), §3) : un PNJ
conditionné, un **décor** conditionné (une pièce vit dans une couche, sans condition), un portail
volontairement sans arrivée, un déclencheur. Sans ce lot, elles s'écriraient à la main dans le
JSON — le retour des scripts que le `LOT-EDITOR-06` a retirés.

## Périmètre

Le **côté carte** du mécanisme. Le LOT-116 livre les drapeaux, les quêtes et la condition dans
`core` ; ce lot les déclare au contrat, les fait contrôler et les montre. La part « jeu » de `prop`
et de `zone.trigger` (les composer, les jouer) est dedans : ce sont des familles d'entité, pas des
fonctions d'éditeur.

**Pas dedans** : écrire les dialogues et les quêtes — des données. Les quêtes entreront dans
l'éditeur par un mode à elles, le [LOT-144](../../v0.0.2-combat/lots/LOT-144-mode-quetes-de-l-editeur.md)
(décision de l'auteur du 24 septembre 2026, [D-24](../../../../vision/decisions.md)).

## Risques

- `prop` recoupe les couches de décor. La règle : ce qui **change** en cours de partie est une
  entité, tout le reste une pièce de couche. Écrite dans le guide du monde et le manuel de
  l'éditeur.

## Décisions de réalisation

Réalisé le 24 septembre 2026, branche `lot-126-quete-cartes`. La fiche reste `en-cours` jusqu'à la
vérification à la main de l'auteur (inspecteur, « World state… », graphe en pointillé).

1. **La présence est une propriété commune, pas une propriété de chaque famille.**
   `core::commonEntityProperties` la déclare une fois ; `core::inspectedProperties` la joint aux
   propriétés de la famille pour l'inspecteur. Elle n'est pas posée à la création : une entité
   sans condition est toujours là. Son contrôle reste celui du `LOT-116`, fait une fois.
2. **Trois sources de choix nouvelles** : `FlagValues` (les valeurs que la quête déclare pour le
   drapeau que nomme une autre propriété, `relatedKey`), `WrittenFlags` (un drapeau que l'entité
   pose, jamais « non posé ») et `Pieces` (le catalogue résolu du lieu). `ArrivalPoints` lit aussi
   `relatedKey` : le point d'arrivée d'un transfert suit `triggerMap`, sans code par famille.
3. **Une valeur non déclarée est une erreur**, qu'on la lise (`presenceValue`) ou qu'on la pose
   (`triggerValue`) : `UndeclaredFlagValue`. Une valeur posée sur un drapeau déclaré est
   **requise** (`writesFlag`) — `core::WorldFlags::set` refuserait le drapeau seul.
4. **Portail condamné** : `sealed` lève le caractère requis de la cible et de l'arrivée
   (`EntityPropertySpec::waivedBy`) ; le graphe lui donne un statut à lui, `Sealed`, qui n'est ni
   une erreur ni un chemin. S'il nomme une cible, le graphe la montre, en pointillé.
5. **Le déclencheur est une zone qui agit à l'entrée**, pas à l'arrivée : un transfert qui dépose
   dans une zone ne boucle pas. Ordre : drapeau, dialogue, transfert. `triggerOnce` garde sa trace
   dans un fait fabriqué (`keyForEntity`), comme un coffre ouvert. La présence de la zone est sa
   condition : la zone du parvis n'existe que sous `acceptee`.
6. **Le transfert est une arête du graphe** (`WorldLinkKind::Transfer`) : validé comme un
   portail, suivi par l'atteignabilité — le point d'arrivée qu'il nomme est un départ, ce qui rend
   le vestiaire A atteignable (critère 3). Les drapeaux que posent les zones
   (`WorldMapNode::triggerFlags`, `core::flagsSetByEntities`) comptent parmi ceux qu'on pose, au
   contrôle des cartes comme à celui du récit.
7. **`prop` a la forme d'un rectangle** : son emprise est `width` × `height`, que l'inspecteur
   prend au manifeste quand on choisit la pièce (un seul pas d'annulation) ; `--check` avertit si
   elle diffère de la pièce. Présent, il se compose comme une pièce de décor à sa case
   (`EntityKind::pieceProperty`) et arrête le pas sur son emprise
   (`ExplorationSession::blockedByProps`) ; sans pièce dessinable — une maquette —, il s'extrude
   en mur. La collision écrite de la carte ne le connaît pas : elle reste celle du passage ouvert.
8. **Un seul état de partie pour le canevas et les deux essais.** L'état s'écrit comme
   `--flags=` (`fait`, `drapeau=valeur`, `Editor/Logic/WorldState.h`) ; *Map* › *World state…*
   le règle, « Run in game… » le partage. Montré au canevas, il retire de la scène ce qui est
   absent et grise son marqueur ; l'essai immédiat déclare les quêtes et part de lui, comme le jeu.

## Ce qui n'est pas ici

- **Suivre les `carte#id` que les quêtes citent** : le format de quête du `LOT-116` n'en cite
  aucun. Le contrôle existe (source `EntityRefs`) ; il servira au champ `at` d'une étape, prévu au
  [LOT-144](../../v0.0.2-combat/lots/LOT-144-mode-quetes-de-l-editeur.md).
- L'atteignabilité du `--check` ignore les décors : elle juge la carte **ouverte**. Une porte close
  qui enferme n'est pas relevée.
- Un héros qui se tient sur l'emprise d'un décor au moment où il paraît n'est pas déplacé.
- Le panneau « Problems » et la vue à plat ne grisent pas selon l'état de partie ; la vue iso le
  fait.
