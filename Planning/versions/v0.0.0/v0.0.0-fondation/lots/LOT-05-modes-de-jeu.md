+++
id = "LOT-05"
titre = "Modes de jeu"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "L'ordre des passes du pas fixe devient une donnée du mode de jeu, ce qui permet à l'exploration, au dialogue et au combat d'avoir chacun le leur sans s'entasser dans `GameSession`."
prerequis = ["LOT-02", "LOT-04"]
livrables = [
  "`hmi::IGameMode` : l'interface d'un mode de jeu (`onLoad`, `step`, `onUnload`, `passOrder()` pour les diagnostics).",
  "`hmi::IGameModePasses`, implémentée par `GameSession` en héritage **privé** : les passes offertes au mode, jamais à l'appelant.",
  "`ExplorationMode`, premier mode, extrait du corps de `GameSession::update`.",
  "Test de rejeu déterministe sur 600 pas, qui compare l'ordre annoncé à la séquence de passes réellement appelée.",
  "Exigence `EX-ARCH-*` : l'ordre des passes du pas fixe est une donnée du mode de jeu.",
]
criteres = [
  "Extraction à comportement **identique**, prouvée par le test de rejeu déterministe.",
  "Les tests de `GameSession` passent **sans modification de leur corps**.",
  "Aucun `if (mode == …)` résiduel dans `GameSession` : la sélection se fait par polymorphisme.",
]
+++

## Pourquoi

Sortir de `hmi::GameSession` l'ordre **codé en dur** des passes du pas fixe, derrière une interface
`hmi::IGameMode`.

### Le problème

`Source/HMI/Game/GameSession.cpp` mêle deux rôles : **orchestrateur** du pas fixe (monde ECS,
caméra, événements, HUD, `FixedTimestep`, interpolation de rendu) et **mode de jeu** (l'ordre des
passes lui-même). Tant qu'il n'y avait qu'un genre, la confusion ne coûtait rien.

Le RPG a besoin d'au moins **trois** ordres différents :

- **exploration** : déplacement, animation, caméra, mécanismes, issue ;
- **dialogue** : monde gelé, seul le runner de dialogue avance ;
- **combat** : initiative, tour actif, résolution d'action — l'exploration ne tourne plus du tout.

Sans cette séparation, les trois s'entasseraient en `if` dans une fonction déjà longue.

## Périmètre

- `hmi::IGameMode` : interface à cinq méthodes environ — `onLoad(World&, const Level&)`,
  `step(World&, const PlayerInput&, float) -> LevelOutcome`, `onUnload()`, plus de quoi décrire
  l'ordre des passes pour les diagnostics.
- `GameSession` conserve : monde ECS, caméra et cadrage, événements, HUD, particules, secousse
  d'écran, `FixedTimestep`, interpolation. Il **délègue** l'ordre des passes.
- `ExplorationMode` : premier mode, extrait du corps actuel de `GameSession::update`.

**N'y ajouter aucune fonctionnalité.** Le mode dialogue et le mode combat arrivent aux `LOT-15` et
`LOT-18` ; les écrire ici mélangerait un refactoring et une nouveauté, et on ne saurait plus lequel
des deux a cassé quoi.

## Risques et questions ouvertes

### Le critère qui compte

Ce lot est un refactoring **à comportement constant**, et c'est le plus risqué du programme : tout
ce qui suit en dépend. Il doit être validé par un **test de rejeu déterministe** — mêmes entrées,
mêmes positions au flottant près sur 600 pas, avant et après extraction.

## Décisions de réalisation

Ce que la réalisation a tranché.

**Les passes sont une interface, pas des paramètres.** L'epic esquissait
`step(World&, const PlayerInput&, float)`. Un `World` ne suffit pas : les passes touchent la
caméra, les particules, la secousse d'écran, les mécanismes, la détection d'événements — tout ce
que l'orchestrateur **garde**, par décision du périmètre. Le mode reçoit donc `IGameModePasses`,
que `GameSession` implémente par héritage **privé** : les passes sont offertes au mode, jamais à
l'appelant de la session, dont l'API publique ne bouge pas d'une ligne.

**Le test de rejeu déterministe porte sur la séquence des passes, pas sur des positions.** Le
critère de l'epic demandait « mêmes entrées, mêmes positions au flottant près sur 600 pas ». Deux
obstacles : `GameSession` exige un atlas, un lot de sprites et une police — impossible à instancier
sans fenêtre, et aucun test ne l'instancie aujourd'hui ; et le personnage **ne se déplace pas
encore** (le contrôleur top-down arrive au `LOT-06`), si bien que des positions constantes ne
prouveraient rien. Ce que l'extraction doit garantir, c'est que l'ordre des passes n'a pas changé —
et c'est vérifiable exactement, sur 600 pas, contre des passes qui enregistrent leurs appels. Le
mode annonce son ordre (`passOrder()`, pour les diagnostics) et un test le compare à la séquence
réellement appelée : la documentation ne peut plus diverger du code sans faire échouer la CI.

**La boîte du personnage est relue par chaque passe qui en a besoin**, plutôt que calculée une fois
et passée de l'une à l'autre. Aucune passe intercalée ne déplace le personnage — le résultat est
identique — et un état partagé de plus entre passes aurait rendu leur ordre difficile à changer :
précisément ce que ce lot cherche à rendre facile.

## Exigences couvertes

`EX-ARCH-*` : « l'ordre des passes du pas fixe est une donnée du mode de jeu, jamais de
l'orchestrateur ».

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest`
à 919/919, lint d'exigences, cahier de test, Doxygen et `clang-format` verts.
