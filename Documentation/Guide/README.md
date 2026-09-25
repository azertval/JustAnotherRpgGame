# Guide du développeur

Ce guide explique **toutes les notions couvertes par le moteur** et **comment le code les
implémente**. Objectif : un développeur ayant des notions de C++ mais **aucune expérience du
développement de jeu vidéo** comprend l'ensemble du code **en autonomie** — chaque page part des
définitions de base (qu'est-ce qu'une boucle de jeu ? un ECS ? une couche de tuiles ? un lot
de sprites ?) avant d'entrer dans l'implémentation, plutôt que de présupposer ce vocabulaire acquis.
Chaque page décrit les fonctions clés, leurs invariants, le **pourquoi** des choix de conception
(pas seulement le *quoi*), et renvoie aux **explications mathématiques/algorithmiques** derrière
les concepts.

## Comment lire ce guide

- Les noms de types et de fonctions écrits avec leur espace de noms (`core::World`,
  `core::ExplorationSession`, …) sont **cliquables** sur le site : ils mènent à la
  [référence du code](../reference/index.html), l'annexe de ce guide (signature, doc détaillée).
- Le guide s'ouvre sur le [manuel utilisateur](Manuel/README.md) : on comprend mieux un moteur
  quand on a vu ce qu'il fait tourner.
- Les liens externes (⧉) pointent vers les **fondements mathématiques/algorithmiques**.
- Le *quoi* et le *pourquoi* vivent dans les [spécifications](../Specification/README.md) ; ce guide
  couvre le *comment* — y compris les notions de game dev prérequises pour le comprendre.
- Les pages sont **indépendantes mais s'appuient les unes sur les autres** (voir leurs sections
  « Voir aussi ») : [Mathématiques du moteur](guide-maths.md) pose le vocabulaire (vecteurs, rectangles, unités) réutilisé
  par [Niveaux : modèle, couches, entités, chargement](guide-niveaux.md) et [Rendu 2D : de la scène à l'écran](guide-rendu.md) ; [Boucle de jeu et pas de temps fixe](guide-boucle.md) et [ECS : entités, composants, systèmes](guide-ecs.md) posent les deux
  piliers d'architecture (déterminisme, données/logique) sur lesquels tout le reste s'appuie. En cas
  de doute sur un terme, remonter à la page qui le définit plutôt que de le supposer connu.

## Architecture en deux couches

Le moteur sépare strictement :

- **`Core`** — logique pure : ECS, mathématiques, modèle de carte, exploration, combat, règles, temps.
  **Aucune dépendance** à DirectX ni à la fenêtre → testable sans GPU (`EX-NFR-010`).
- **`HMI`** — présentation : deux applications Qt depuis le `LOT-86` — le **jeu** en Qt Quick,
  l'**éditeur de niveaux** en Qt Widgets —, le rendu de scène sur QRhi (Direct3D 11 par défaut sous
  Windows) et les entrées. Dépend de `Core`, **jamais l'inverse** (`EX-ARCH-010`).
- **`Source/Ui`** — ce que la **conception** modifie : les écrans du jeu en QML déclaratif, leurs
  jetons et leurs ornements. Ne connaît de `HMI` que ses vues-modèles (`EX-IHM-100`).

La règle d'or : **la simulation est dans `Core`, déterministe et testée** ; `HMI` orchestre et
affiche. Cette frontière est ce qui rend le moteur analysable domaine par domaine.

## Plan du guide

### Prendre en main

- [Manuel utilisateur](Manuel/README.md) — pour qui **joue** ou **fait des cartes**, sans lire de code :
  [télécharger et lancer](Manuel/telecharger-et-lancer.md), [jouer](Manuel/jouer.md),
  [utiliser l'éditeur de cartes](Manuel/utiliser-l-editeur.md),
  [créer et partager une carte](Manuel/partager-un-niveau.md).

### Le cœur de simulation (`Core`)

- [Boucle de jeu et pas de temps fixe](guide-boucle.md) — la boucle de jeu et le **pas de temps fixe** (déterminisme).
- [ECS : entités, composants, systèmes](guide-ecs.md) — l'**ECS** maison (entités, composants, systèmes, vues).
- [Mathématiques du moteur](guide-maths.md) — `Vector2`, `Rect`, unités, hasard déterministe.
- [Niveaux : modèle, couches, entités, chargement](guide-niveaux.md) — les **cartes** : modèle, couches, entités, format JSON.
- [Monde et exploration](guide-monde.md) — la session d'exploration, les portails, le graphe du monde, les dialogues et les faits de la partie.
- [Règles d20 et personnages](guide-regles.md) — dés, jets, caractéristiques, fiche, inventaire, équipement.
- [Combat tactique](guide-combat.md) — grille, initiative, économie d'actions, attaques, portée et ligne de vue, zones, IA, arène.
- [Données, corpus et ressources](guide-donnees.md) — catalogues JSON, schémas, clés d'assets, chaîne d'extraction, localisation.
- [Journalisation et assertions](guide-journalisation.md) — niveaux, sinks, macros, rapport de plantage.

### La présentation (`HMI`)

- [Entrées et actions logiques](guide-entrees.md) — clavier, souris, manette, et leur traduction en **actions**.
- [Rendu 2D : de la scène à l'écran](guide-rendu.md) — QRhi, lot de sprites, atlas, caméra, composition de scène isométrique.
- [Écrans, navigation et boucle de jeu](guide-ecrans.md) — table de transitions, routeur et pile d'écrans QML, pause.
- [IHM Qt — deux applications, deux technologies](guide-ihm-qt.md) — le jeu en Qt Quick, l'éditeur en Widgets, les vues-modèles.
- [Concevoir les écrans dans Qt Design Studio](guide-conception-qds.md) — ce qui se modifie sans ouvrir un fichier source.
- [Système de design et architecture de l'information](guide-design-ihm.md) — jetons, échelle, panneaux, barre d'état.
- [Audio](guide-audio.md) — moteur Qt Multimedia et réglage du volume.

### L'éditeur de cartes

- [Éditeur de niveaux](guide-editeur.md) — le document, les gestes, le canevas isométrique, le contrôle du contenu, le mode sans fenêtre.

### Construire, vérifier, documenter

- [Build, tests et intégration continue](guide-outils.md) — la chaîne de build, les scripts, les lints, la CI, la page qualité.
- [Outils de développement du jeu](guide-outils-developpement.md) — le menu de développement (<kbd>F9</kbd>), le sélecteur d'écrans, la ligne de commande, la racine d'essai, les captures.
- [Écrire la documentation](guide-documentation.md) — le format des pages, les conventions, les figures, la publication.

### Annexe

- [Référence du code](../reference/index.html) — classes, espaces de noms et fichiers de `Source/`, engendrés par
  Doxygen depuis les commentaires du code.
