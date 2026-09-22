+++
id = "LOT-06"
titre = "Déplacement top-down 8 directions"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "Le jeu redevient jouable : un personnage se déplace librement en huit directions sur une carte en vue de dessus, sans gravité, à vitesse égale en diagonale."
prerequis = ["LOT-04", "LOT-05"]
livrables = [
  "`Source/Core/Ecs/Systems/TopDownMovementSystem.{h,cpp}` : intention → vitesse → `sweepAabb` → position.",
  "`Source/Core/Ecs/Components/Actor.h`, remplaçant de `core::Player`, qui porte l'orientation (*facing*).",
  "`Source/Core/Physics/TopDownConfig.h` : vitesse, accélération, friction, en donnée pure.",
  "Branchement du système dans `ExplorationMode`.",
  "Catégorie `EX-EXP-*` déclarée dans `Documentation/Specification/exploration.md`.",
  "Retrait de `core::Player`, des compteurs de sauts et de dashs du HUD et de `Source/Test/Systeme/ScriptedLevelSequence.h`.",
]
criteres = [
  "Le personnage ne traverse **aucun** mur, même à vitesse élevée (le balayage continu s'en charge).",
  "**Vitesse diagonale = vitesse cardinale**, vérifié par un test.",
  "Glissement le long d'un mur pris en biais, sans blocage ni accroche.",
  "Déterminisme : mêmes entrées → mêmes positions au rejeu (`EX-NFR-002`).",
  "Testable **headless**, sans fenêtre ni GPU (`EX-ARCH-001`).",
]
+++

## Pourquoi

Rendre le jeu **jouable à nouveau** : un personnage qui se déplace librement en 8 directions sur
une carte en vue de dessus, sans gravité.

C'est le lot qui referme la parenthèse ouverte par le `LOT-01`, où la physique de plateforme a été
retirée sans remplaçant.

## Périmètre

- `Source/Core/Ecs/Systems/TopDownMovementSystem.{h,cpp}` (~150 lignes attendues) : intention →
  vitesse → `sweepAabb` → position.
- `Source/Core/Ecs/Components/Actor.h` : remplaçant de `core::Player`, sans ses ~30 champs de
  plateforme (contact au sol, coyote time, jump buffering, dash, combos) devenus inertes au
  `LOT-01`. Porte l'orientation (*facing*), utile au sprite et à l'interaction du `LOT-10`.
- `Source/Core/Physics/TopDownConfig.h` : vitesse, accélération, friction — donnée pure, à côté de
  `PhysicsConfig` dont les champs de saut disparaissent.
- Branchement dans `ExplorationMode`.

## Conception

### Ce qui existe déjà, et qu'il ne faut pas réécrire

- **`core::sweepAabb`** (`Source/Core/Physics/SweptCollision.h`) : balayage continu axe par axe
  contre la grille, **sans gravité ni axe privilégié**, avec glissement le long des murs et aucune
  traversée à vitesse élevée. C'est exactement la primitive d'un déplacement top-down. Le `LOT-01`
  l'a débarrassée du suivi de pente ; elle est redevenue un pur balayage de grille.
- **`core::PlayerInput`** porte déjà `moveX` **et** `moveY`, alimentés par `hmi::PlayerInputMapper`
  depuis le clavier et la manette.
- **La normalisation 8 directions existe déjà** : le dash du platformer la faisait
  (`direction.normalized() * speed`). Elle se reprend telle quelle pour la marche.
- La convention de monde est la bonne (`EX-ARCH-020`) : 1 unité = 1 tuile, origine haut-gauche,
  Y vers le bas, 16 px/unité.

Ce lot est donc essentiellement de l'**assemblage**, pas de l'invention.

## Risques et questions ouvertes

### Le piège classique

**La diagonale ne doit pas être plus rapide.** Sans normalisation, aller en diagonale donne
`√2 ≈ 1,41` fois la vitesse cardinale — le défaut le plus courant du genre, et le plus visible en
jeu. C'est un critère d'acceptation, pas un détail de finition.

## Décisions de réalisation

Ce que la réalisation a tranché.

**Retirer `core::Player` coûte plus que l'écrire.** Le composant n'était pas seulement inerte : il
était **lu** par l'animation (clip choisi d'après `grounded`, `dashTimer`, `wallDirection`), par le
HUD (budgets de sauts et de dashs), par la détection d'événements (saut, atterrissage, glissade
murale) et par un harnais de tests système qui rejouait vingt-six tableaux de plateforme supprimés
au `LOT-01`. Le lot a donc dû trancher, pour chacun :

- **Animation** : le clip se choisit désormais sur la **norme** de la vitesse — marcher vers le
  haut est une marche. C'était nécessaire, pas cosmétique : `grounded` valant toujours `false`
  depuis le `LOT-01`, le personnage était figé dans la pose de saut, et le serait resté en marchant.
  Les clips aériens restent **déclarés** (poses procédurales et spritesheets externes s'y
  accrochent) mais plus rien ne les sélectionne ; le `LOT-08` refera ce vocabulaire.
- **HUD** : les compteurs de sauts et de dashs disparaissent, ainsi que leurs clés de traduction.
- **Événements** : `Jumped`, `Landed`, `Dashed` et `WallContactEnter` perdent leur producteur. Les
  valeurs restent **déclarées** — la table de sons et les statistiques d'essai s'y accrochent — et
  c'est le combat (`LOT-21`) puis l'audio (`LOT-28`) qui diront ce que le RPG met à leur place.
- **`Source/Test/Systeme/ScriptedLevelSequence.h`** est supprimé : il scriptait les entrées de
  vingt-six niveaux de démonstration qui n'existent plus, et aucun test ne l'incluait plus.

**Le composant s'appelle `Actor`, pas `Player`.** Rien de ce qu'il porte ne suppose **le**
personnage du joueur : les PNJ (`LOT-15`) et les alliés du groupe (`LOT-29`) porteront le même. Ce
qui distingue l'entité pilotée par les entrées, c'est de recevoir une intention, pas d'être d'un
autre type.

**L'anticipation de la caméra de suivi reste horizontale.** Elle ne lit que la composante `x` de
l'orientation. Anticiper aussi vers le haut et vers le bas est une décision de **cadrage**, pas une
conséquence du déplacement : ce lot ne l'a pas prise.

## Exigences couvertes

Catégorie `EX-EXP-*`, déclarée par ce lot dans `Documentation/Specification/exploration.md` :
déplacement isotrope normalisé, glissement le long des murs, absence
de gravité, orientation conservée à l'arrêt.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest`
à 911/911, lint d'exigences, cahier de test, Doxygen et `clang-format` verts. Le `LOT-04` apportait
la couche de collision, le `LOT-05` le mode d'exploration où brancher le système.
