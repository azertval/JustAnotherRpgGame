# JustAnotherRpgGame

RPG 2D **en vue isométrique** développé **from scratch** en **C++20 / Qt QRhi** (Direct3D 11 sous
Windows), sans moteur tiers : exploration en temps réel, et rencontres en **combat tactique au tour
par tour** régi par un système **d20** maison, dans l'univers de Tanares.

Cette documentation tient en un seul site, et chaque partie répond à une question :

| Partie | La question | La source |
|---|---|---|
| [Guide](Guide/README.md) | **Comment** ça marche, et comment s'en servir ? Le manuel du joueur et de l'auteur de cartes, puis le moteur domaine par domaine. | `Documentation/Guide/` |
| [Spécifications](Specification/README.md) | **Quoi**, et **pourquoi** ? Les exigences `EX-…`, tracées jusqu'au code et aux tests. | `Documentation/Specification/` |
| [Cahier de test](CahierTest/README.md) | Qu'est-ce qui est **vérifié** ? Un cas par test automatisé. | engendré depuis `Source/Test/` |
| [Planification](../Planning/README.md) | **Quand**, et dans quel ordre ? Les versions, les lots livrés et à venir. | `Planning/` |
| [Référence du code](reference/index.html) | Que fait **ce symbole** ? L'annexe du guide, engendrée par Doxygen. | les commentaires de `Source/` |

## En bref

- **Langage et rendu** : C++20, Qt QRhi (Direct3D 11 sous Windows), scène dessinée en isométrique.
- **Architecture** : un cœur de simulation (`Core`) **indépendant** de la présentation (`HMI`) ;
  le jeu en Qt Quick, l'éditeur de cartes en Qt Widgets ; les données et les assets vivent dans
  `Source/Elements`.
- **Qualité** : build sans avertissement (`/W4 /WX`), tests unitaires, d'intégration et système
  (GoogleTest), CI GitHub Actions, couverture et performances publiées sous *Qualité*.

## Comment cette documentation est faite

Toutes les pages sont du **Markdown nu**, lisible tel quel dans le dépôt, et rendues par un seul
moteur — celui de la planification (`Documentation/outils/build_docs_site.py`). Doxygen ne garde
que le code. Les conventions d'écriture (liens, exigences, figures, encadrés) sont dans
[Écrire la documentation](Guide/guide-documentation.md) ; l'historique des livraisons est dans
`CHANGELOG.md`.
