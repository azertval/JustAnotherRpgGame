# Mathématiques du moteur

Cette page redéfinit, sans présupposer de bagage en algèbre appliquée aux jeux vidéo, les quelques
outils mathématiques dont dépend tout le reste du moteur : vecteurs, rectangles alignés aux axes,
conventions d'unités, comparaison de nombres flottants.

Le moteur n'utilise **aucune** bibliothèque mathématique tierce dans `Core` (pas de DirectXMath, pas
de GLM) : un type minimal, écrit à la main, suffit aux besoins du jeu et garde `Core` totalement
indépendant de DirectX (`EX-ARCH-040`) — la conversion vers les types du GPU se fait uniquement côté
`HMI`, au moment du rendu.

## `core::Vector2` : un point ou une direction dans le monde

Un **vecteur 2D** est simplement une paire de nombres `(x, y)`. Il sert à deux usages différents
selon le contexte, qu'il faut garder à l'esprit en lisant le code :

- comme **position** : « où se trouve cette entité dans le monde ? » (`core::Transform::position`) ;
- comme **direction** : « dans quel sens le joueur veut-il aller ? »
  (`core::ExplorationIntent::move`), qu'une vitesse en unités monde **par seconde** transforme en
  déplacement.

`core::Vector2` porte deux `float`, `x` et `y`, et fournit l'algèbre nécessaire :

- **addition/soustraction** (`+`, `-`) : combiner deux positions ou deux déplacements composante par
  composante — additionner une position et une vitesse × temps donne la nouvelle position ;
- **produit par un scalaire** (`*`, `/`) : allonger ou raccourcir un vecteur sans changer sa
  direction (multiplier une direction par une vitesse scalaire donne un vecteur vitesse) ;
- **opposé** (unaire `-`) : inverser un sens (`-v` va exactement à l'opposé de `v`) ;
- **produit scalaire** ([dot product](https://fr.wikipedia.org/wiki/Produit_scalaire) ⧉, méthode
  `dot`) : `a.dot(b) = a.x*b.x + a.y*b.y`. Géométriquement, ce nombre unique résume la relation
  entre deux directions : positif si elles pointent globalement dans le même sens, négatif si elles
  s'opposent, nul si elles sont perpendiculaires. C'est l'opération fondamentale des calculs
  d'angle et de projection — l'éditeur s'en sert pour mesurer la distance d'un clic à un lien du
  graphe du monde (`Editor/Logic/WorldGraphLayout.cpp`) ;
- **longueur** (`length`, [norme euclidienne](https://fr.wikipedia.org/wiki/Norme_euclidienne) ⧉) :
  la distance entre l'origine et le point `(x, y)`, calculée par le théorème de Pythagore :
  `sqrt(x*x + y*y)` ;
- **normalisation** (`normalized`) : produit un vecteur de **même direction** mais de longueur
  exactement **1** (un « vecteur unitaire »), en divisant chaque composante par la longueur. Utile
  pour obtenir une direction pure, indépendante de la distance — par exemple normaliser l'intention
  de déplacement (`core::ExplorationIntent::move`, de longueur au plus 1) garantit qu'une diagonale
  ne va pas plus vite qu'un mouvement cardinal, alors que `(1, 1)` non normalisé a une longueur de `√2 ≈ 1,41`, soit 41 %
  plus rapide qu'attendu sans cette étape. Cas particulier : normaliser le vecteur nul (longueur
  quasi nulle) n'a pas de direction définie — `normalized()` renvoie alors le vecteur nul plutôt que
  de diviser par zéro.

### `core::Vector2::lengthSquared` : éviter la racine carrée

`lengthSquared()` renvoie `x*x + y*y`, **sans** appeler `sqrt`. La racine carrée est une opération
relativement coûteuse comparée à une multiplication ; or, pour de nombreuses questions, on n'a pas
besoin de la longueur exacte, seulement de **comparer** deux longueurs (« ce vecteur est-il plus
long que celui-là ? », « cette distance est-elle inférieure à un seuil ? »). Comme la fonction
racine carrée est **croissante**, comparer `a.lengthSquared() < b.lengthSquared()` donne exactement
le même résultat que comparer `a.length() < b.length()`, sans jamais calculer de racine — une
optimisation classique et systématique en géométrie appliquée aux jeux.

### Égalité approchée

`operator==` sur deux `Vector2` (comme `core::approximatelyEqual` sur deux `float`, voir plus bas)
compare avec une **tolérance**, pas une égalité binaire exacte. C'est nécessaire parce que
l'arithmétique flottante accumule de minuscules erreurs d'arrondi : deux calculs mathématiquement
équivalents (par exemple `(a + b) + c` et `a + (b + c)`) peuvent produire des `float` légèrement
différents au dernier bit. Comparer de tels résultats avec `==` strict échouerait de façon
imprévisible et intermittente — un piège classique documenté plus bas.

## `core::Rect` : le rectangle aligné aux axes

Une [AABB](https://en.wikipedia.org/wiki/Bounding_volume) ⧉ (*Axis-Aligned Bounding Box*, « boîte
englobante alignée aux axes ») est la forme géométrique la plus simple pour représenter une zone :
un **rectangle dont les côtés sont toujours parallèles aux axes X et Y** — jamais tourné. C'est un
compromis délibéré : une forme tournée ou complexe (cercle, polygone) serait bien plus chère à
tester, pour un gain inutile dans un monde fait de grilles de tuiles alignées aux axes.

`core::Rect` décrit un tel rectangle par son **coin haut-gauche** (`position`) et sa **taille**
(`size`) ; `left()`, `right()`, `top()` et `bottom()` en donnent les bords :

```
  (left, top) ┌──────────────┐
              │              │
              │   rectangle  │
              │              │
              └──────────────┘ (right, bottom) = position + size
```

Deux tests suffisent aux usages du moteur — la zone visible de la caméra
(`hmi::Camera2D::visibleBounds`), l'emprise d'une tuile projetée (`core::IsoProjection`) :

- `contains(point)` est **inclusif** en haut/à gauche et **exclusif** en bas/à droite, pour qu'une
  grille de rectangles jointifs pave le plan sans recouvrement ni trou ;
- `intersects(other)` exige une aire commune strictement positive : deux rectangles qui se
  touchent seulement par un bord ne se recouvrent pas.

## Conventions d'unités et de repère

Trois conventions, fixées une fois pour toutes et valables dans **tout** `Core`, expliquent la
plupart des signes rencontrés dans le code de déplacement et de niveau :

- **Une tuile = une unité monde.** Les positions, tailles et vitesses sont exprimées en
  « unités monde » (ou « unités par seconde » pour les vitesses), **jamais en pixels** à l'intérieur
  de `Core`. Une entité large de `1.0` occupe exactement une case de la grille de niveau. La
  conversion vers les pixels affichés à l'écran (16 pixels par unité dans ce projet) n'a lieu
  **qu'au moment du rendu**, côté `HMI` — `Core` n'a aucune idée de la résolution de la fenêtre ni
  du zoom de la caméra, ce qui le garde testable sans ouvrir de fenêtre.
- **Origine en haut-gauche, `y` vers le bas** (`EX-ARCH-020`) — la convention standard de
  l'affichage écran (héritée du sens de balayage d'un moniteur, ligne du haut en premier), à
  l'opposé de la convention mathématique habituelle où `y` monte. Conséquence directe et
  contre-intuitive pour qui découvre ce domaine : « monter » (aller vers le nord de la carte)
  correspond à une coordonnée `y` qui **diminue** — s'y référer dès qu'un signe surprend.
- **Angles en radians**, pas en degrés (`Transform::rotation`) — la convention native des fonctions
  trigonométriques du C++ standard (`std::sin`, `std::cos`, …), qui évite une conversion à chaque
  appel.

Garder ces trois conventions en tête suffit à expliquer, sans avoir à les redériver, la quasi-
totalité des signes et des sens de déplacement rencontrés dans le moteur.

## Comparaison flottante : pourquoi l'égalité stricte est dangereuse

Les nombres à virgule flottante (`float`) ne représentent pas exactement toutes les valeurs
réelles : ils utilisent une précision finie, et des opérations en apparence anodines (addition
répétée, division) introduisent de minuscules erreurs d'arrondi. Un exemple classique : en C++,
`0.1f + 0.2f == 0.3f` est **faux**, car ni `0.1`, ni `0.2`, ni `0.3` ne sont représentables
exactement en binaire — le résultat de l'addition diffère de `0.3f` de quelques millionièmes.
Comparer deux flottants issus de calculs différents (même mathématiquement équivalents) avec `==`
strict est donc fragile : le test peut échouer de façon imprévisible selon l'ordre des opérations,
l'optimiseur du compilateur, ou l'architecture du processeur.

`core::approximatelyEqual` (dans `Core/Math`) résout ce problème en comparant deux `float` à une
**tolérance** près, à la fois **relative** (proportionnelle à la magnitude des nombres comparés —
utile pour de grandes valeurs) et **absolue** (un plancher minimal — utile près de zéro, où une
tolérance purement relative deviendrait nulle). C'est la fonction que `Vector2::operator==` appelle
composante par composante, et celle que les tests du moteur utilisent systématiquement pour
comparer des résultats de calcul flottant plutôt qu'un `==` direct.

## Voir aussi
- `core::Vector2`, `core::Rect`, `core::approximatelyEqual`.
- [ECS : entités, composants, systèmes](guide-ecs.md) — `Transform`, le composant qui porte ces types.
- [Rendu 2D : de la scène à l'écran](guide-rendu.md) — la conversion des unités monde en pixels.
