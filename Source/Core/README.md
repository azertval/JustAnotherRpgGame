# Source/Core/

Fonctions **back** : le moteur et la logique du jeu.

## Périmètre
- Cadencement du temps de simulation (`Time/`), diagnostic (`Diagnostics/`), mathématiques (`Math/`).
- Entités et composants (`Ecs/`), cartes et leur format JSON (`Levels/`), lecture JSON (`Data/`).
- Règles du jeu de rôle : fiches, jets, équipement, dialogues (`Rpg/`) ; combat tactique (`Combat/`).
- Monde et exploration : atlas, graphe des lieux, quartiers, session d'exploration (`World/`) ;
  interactions, drapeaux de monde et quêtes (`Gameplay/`, dont `Quest.h`).
- Clés d'assets d'entité, marqueurs de substitution, manifestes des pièces et niveaux d'un lieu —
  le catalogue résolu, du plus propre au monde (`Resources/`, `LOT-124`).

Ce dossier est indépendant de la présentation (`../HMI/`) : il expose un état, il ne l'affiche pas.
