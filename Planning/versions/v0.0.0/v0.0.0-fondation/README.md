# Version 0.0.0 — Fondation du moteur

Cette version n'a pas été *planifiée* ici : elle **rassemble** ce qui a été livré avant la refonte du
20 septembre 2026, quand le projet se pilotait par deux feuilles de route — celle du jeu (`LOT-01` à
`LOT-96`) et celle de l'éditeur de cartes (`LOT-EDITOR-01` à `LOT-EDITOR-14`). Chaque lot **démarré**
y a sa fiche, au même format que les lots à venir : un en-tête lisible par le lint et le site, puis
le récit du lot — pourquoi, périmètre, conception, décisions de réalisation, vérification, bilan.

## Ce que la fondation a posé

| Chantier | Lots | Ce qui en reste dans le dépôt |
|---|---|---|
| **Moteur** | `LOT-01` à `LOT-11`, `LOT-15`, `LOT-18`, `LOT-50`, `LOT-79`, `LOT-88` | `Source/Core` (ECS, cartes multicouches, exploration, dialogues, bascule vers le combat), le rendu isométrique trié, le chargement de données |
| **Règles et combat** | `LOT-12` à `LOT-14`, `LOT-19` à `LOT-23` | jets d20, fiche, inventaire, grille tactique, initiative, attaques, portée et ligne de vue, IA tactique |
| **Corpus et données** | `LOT-30` à `LOT-43` | chaîne d'extraction, schémas, bestiaire, équipement, espèces et classes, atlas des régions, clés d'assets |
| **Interface** | `LOT-24`, `LOT-38`, `LOT-66` à `LOT-76`, `LOT-86`, `LOT-87`, `LOT-94` | le jeu en Qt Quick, la charte v2, les écrans RPG, l'écran Carte à trois niveaux |
| **Ateliers d'assets et cartes** | `LOT-09`, `LOT-91` à `LOT-93`, `LOT-96` | des méthodes (PNJ, textures, monstres) ; les assets pixel art et les cartes ont été retirés par la table rase du `LOT-102` |
| **Éditeur de cartes** | `LOT-EDITOR-01` à `LOT-EDITOR-14` | `LevelEditor` : socle, canevas isométrique, pièces, outils, entités et zones, format v4, mode sans fenêtre, contrôle du contenu, tampons, monde, essai dans le jeu |
| **Standards** | `LOT-77`, `LOT-78` | les cinq familles d'exigences du RPG, la numérotation des lots |

## Lire ces fiches aujourd'hui

- Les **identifiants sont d'époque** et ne changent pas : `LOT-19`, `LOT-EDITOR-03`. Tout lot né
  depuis la refonte porte trois chiffres (`LOT-100` et suivants).
- Un **prérequis** n'est écrit dans l'en-tête que s'il a lui-même une fiche ; les lots de l'ancienne
  feuille de route qui n'ont jamais démarré (`LOT-16`, `LOT-27`…) restent cités dans le corps, et
  se retrouvent dans les [archives](../../../vision/archives/README.md). La
  [table de correspondance](../../../vision/correspondance-ancienne-roadmap.md) dit quel lot à
  venir reprend chacun d'eux.
- Plusieurs fiches portent une **vérification manuelle en attente** (souris, clavier, manette) :
  elle est notée dans leur rubrique « Vérification » et n'a pas empêché la livraison.
- Les fichiers d'accompagnement d'un lot (ateliers, captures, maquettes de référence, retouches)
  sont sous [`annexes/`](annexes/README.md), un dossier par lot.
