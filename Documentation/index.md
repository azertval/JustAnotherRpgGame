# JustAnotherRpgGame

RPG 2D **en vue de dessus** développé **from scratch** en **C++20 / Qt QRhi** (Direct3D 11 sous
Windows), sans moteur tiers : exploration en temps réel, et rencontres en **combat tactique au tour
par tour** régi par un système **d20** maison.

Cette documentation rassemble, en un seul endroit, les **spécifications** du
projet, les **lots de travail**, le **manuel utilisateur** et la **référence de
code** générée à partir des sources.

## En bref
- **Langage & rendu** : C++20, Qt QRhi (Direct3D 11 sous Windows), scène dessinée en isométrique.
- **Architecture** : un cœur de simulation (`Core`) **indépendant** de la présentation (`HMI`) ;
  le jeu en Qt Quick, l'éditeur de cartes en Qt Widgets ; les données et assets vivent dans
  `Elements`.
- **Qualité** : build sans avertissement (`/W4 /WX`), tests unitaires et d'intégration
  (GoogleTest), documentation Doxygen et CI GitHub Actions.

## Avancement

Le détail de chaque lot (objectifs, tâches, avancement) est dans la rubrique
[Lots](@ref lots) ; l'historique des livraisons est dans `CHANGELOG.md`.

## Navigation
- @subpage guide — **Guide du développeur** : comprendre tout le moteur (concepts, code, maths).
- @subpage cahiertest — **Cahier de test** : tous les cas de test (catégorie, criticité, étapes).
- @subpage specifications — besoins, contraintes et exigences (`EX-…`), conventions de code.
- @subpage lots — les lots **livrés** (`LOT-01` à `LOT-96`) et l'ancienne feuille de route, figée.
- **Planification** — les versions et les lots **à venir**, leur avancement et leurs maquettes :
  <a href="planning/index.html">site de planification</a> (source : dossier `Planning/` du dépôt).
- @subpage manuel — manuel utilisateur (télécharger et lancer le jeu).
- **Référence de code** — classes, espaces de noms et fichiers de `Source/` : voir
  l'arbre de navigation (menu latéral) et les onglets *Namespaces* / *Classes*.
