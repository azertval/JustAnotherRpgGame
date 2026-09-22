# Vision & périmètre

> Statut : **en cours**. Le moteur qui porte ces objectifs est livré (version `0.0.0`,
> « Fondation du moteur ») ; le premier jalon jouable est la **démo** `0.0.1` — deux quartiers de la
> Capitale, un donjon, une quête. La [planification](../../Planning/README.md) dit quel lot porte
> quoi, et dans quel ordre.

## Concept

**RPG 2D en vue isométrique**, à monde de cartes connectées. Le joueur dirige un personnage qui
explore en **temps réel** (déplacement libre 8 directions, interaction avec les PNJ, les coffres
et les portails) et affronte les rencontres en **combat tactique au tour par tour** sur la grille
de la carte, régi par un système **d20**.

- **Genre** : action-RPG d'exploration + combat tactique.
- **Perspective** : 2D **isométrique**, décor en tuiles multi-couches (sol / décor / collision).
  La carte se **pense** en grille orthogonale — une case, des voisins, un parcours — et se
  **dessine** en losanges (§Identités visuelles) : c'est la même carte, vue autrement.
- **Session type** : progression continue dans un monde persistant, sauvegardée.
- **Public** : joueurs appréciant l'exploration et la réflexion tactique.
- **Plateforme** : Windows (bureau), rendu Qt QRhi (Direct3D 11).

### Décisions de cadrage

Quatre décisions structurantes, actées avant le `LOT-01` et non réouvertes sans arbitrage
explicite :

1. **Règles d20 maison**, structurellement compatibles avec le SRD (six caractéristiques,
   modificateur `(score-10)/2`, jet d20 contre difficulté ou classe d'armure, avantage/désavantage)
   mais **sans en dépendre** : classes et sorts propres, définis en JSON. On garde la familiarité
   sans la charge d'implémentation ni l'obligation d'attribution.
2. **Combat sur la carte d'exploration**, jamais en arène séparée. La grille de combat est
   **dérivée de la couche collision** du niveau. Conséquence de level design à tenir dès la
   première carte : *toute carte doit être un terrain tactique valide* — largeur suffisante,
   obstacles cohérents.
3. **Un héros au départ, quatre à terme.** Rien ne doit supposer l'unicité du personnage : la
   fiche est un objet autonome, l'ordre d'initiative est multi-alliés, la sauvegarde stocke une
   *liste*. Le passage au groupe est un lot d'ajout, jamais une refonte.
4. **Échelle : 1 case = 1,5 m** (5 ft), avec `PIXELS_PER_UNIT = 16` inchangé. Fixe portées,
   vitesses et gabarits d'effet.

## Identités visuelles

Le jeu a **deux identités**, une par couche de l'image, et aucune ne déborde sur l'autre. Elles ne
se distinguent plus par leur facture — depuis le `LOT-101`, les deux sont **peintes** — mais par
leur **rôle** : la scène est du monde, et son échelle est celle du lieu ; l'interface renseigne le
joueur, et son échelle est celle de la fenêtre. La frontière entre les deux est écrite, pas laissée
à l'œil.

- **EX-VIS-008** — La **scène** — sols, murs, objets du monde et figurines —
  doit être **peinte en isométrie haute définition** : losange de sol de **256 × 159 pixels d'art**
  (rapport 0,62, celui d'`core::IsoProjection`), figurine humanoïde de **170 px** dans une cellule
  de **192 × 256** dont la ligne de sol est à `y = 252` (cellule large **384 × 256** pour l'attaque
  et le sort), grande créature en **384 × 384**. L'échelle de l'art est une **donnée du lieu** — le
  champ `"tile"` de son manifeste de pièces — et non une constante du rendu : un lieu peut être
  livré plus fin ou plus grossier sans toucher au code. Les images sont en sRGB, à **alpha continu**
  prémultiplié au chargement, sans palette imposée par image, et échantillonnées en **bilinéaire
  avec mipmaps** (`EX-ARCH-022`). Le style est **écrit** et non laissé au générateur : le standard
  2D HD (`Planning/standards/style-2d-hd.md`) et la consigne du générateur
  (`Planning/standards/consigne-2d-hd.md`), tirés de la **planche de référence approuvée par
  l'auteur** (`Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png`), fixent le trait (un
  contour sombre et fin, bronze foncé, jamais noir pur), la lumière (clé douce en haut à gauche,
  ombre propre peinte, **aucune ombre portée** dans la pièce), la projection orthographique, la
  palette du lieu et les **dix familles de pièces** dont une zone fait l'inventaire. Une pièce
  **tient seule** : fond transparent, pas de sol sous un mur, pas de décor autour d'un meuble.
  Les figurines de l'atelier des PNJ (`LOT-91`) sont de la scène, à l'échelle de son sol.
  > **Ce que l'exigence ne dit pas encore.** Le nombre d'images par animation reste ouvert : il se
  > tranche sur l'essai de marche du `LOT-101`, et aucune figurine ne se produit en série avant.
- **EX-VIS-009** — L'**interface** — écrans, panneaux, HUD, et tout ce qui
  **renseigne le joueur par-dessus la scène** (curseur, chemin, portées, texte ancré) — doit porter
  la **charte v2** (`EX-IHM-070`) : images produites à 1080p et échantillonnées à tout facteur,
  polices vectorielles embarquées (`EX-REN-032`). Aucun élément de l'interface n'est de l'art de
  scène et aucun élément du monde ne porte la charte : pas de filet d'or ni de `Cinzel` dans la
  scène, pas de pièce isométrique ni de palette de lieu dans les écrans. Les deux couches se
  **mesurent** différemment, et c'est là que la frontière se vérifie : une pièce de scène se met à
  l'échelle du **lieu** (`EX-VIS-008`), une image d'interface à celle de la **fenêtre**. Le seul
  point de contact est le **viewport** de la scène, qu'un écran de l'interface encadre sans le
  peindre.

## Boucle de gameplay

1. Le joueur explore une carte : déplacement libre, interaction avec le décor et les PNJ.
2. Il progresse dans le monde par des portails entre cartes, guidé par les dialogues et les quêtes.
3. Une rencontre se déclenche (contact, zone, dialogue) : le monde se fige, la grille tactique se
   monte sur la carte courante.
4. Le combat se joue au tour par tour, dans l'ordre d'initiative, chaque action résolue au d20.
5. Victoire : retour à l'exploration, l'ennemi retiré de la carte durablement, butin et expérience
   acquis. Défaite : reprise à la dernière sauvegarde.

## Objectifs du moteur

- **EX-VIS-001** — Le jeu doit proposer un personnage jouable se déplaçant
  librement en 8 directions sur une carte en tuiles, dessinée en isométrie.
- **EX-VIS-002** — Le jeu doit relier plusieurs cartes par des portails, avec
  retour possible au point de départ.
- **EX-VIS-003** — Le jeu doit permettre de dialoguer avec un PNJ, dialogue à
  choix et conditions.
- **EX-VIS-004** — Le jeu doit résoudre un combat tactique complet au tour par
  tour : initiative, déplacement à portée, attaque au d20 contre une classe d'armure, fin de
  rencontre.
- **EX-VIS-005** — Toute résolution chiffrée doit être **déterministe à graine
  fixée** : un combat rejoué produit exactement les mêmes jets (`EX-NFR-002`).

## Objectifs produit (au-delà du moteur)

- **EX-VIS-006** — Le projet doit fournir un **éditeur de cartes** permettant à
  des membres non-développeurs de créer du contenu sans coder : couches, entités, portails
  (`LOT-11`).
- **EX-VIS-007** — Toute règle chiffrée (classes, sorts, objets, ennemis) doit
  être **définie en données** (JSON), jamais codée en dur dans le C++ : c'est ce qui rend
  l'équilibrage possible sans recompiler.

## Hors périmètre

- Multijoueur, réseau.
- Groupe de plusieurs personnages jouables (prévu, mais **après** la démo `0.0.1` — cf. décision 3).
- Génération procédurale de cartes ou de donjons.
- Édition collaborative en temps réel dans l'éditeur.
- Portabilité hors Windows.

## Traçabilité

Ces objectifs sont détaillés dans [`gameplay.md`](gameplay.md), [`controles.md`](controles.md),
[`rendu-technique.md`](rendu-technique.md), [`niveaux.md`](niveaux.md),
[`exigences-non-fonctionnelles.md`](exigences-non-fonctionnelles.md),
[`editeur-niveaux.md`](editeur-niveaux.md), [`architecture.md`](architecture.md) et, pour le
RPG, [`exploration.md`](exploration.md), [`regles-d20.md`](regles-d20.md), [`rpg.md`](rpg.md),
[`combat.md`](combat.md), [`inventaire.md`](inventaire.md) et [`contenu.md`](contenu.md). Chaque
[fiche de lot](../../Planning/README.md) référence les exigences `EX-…` qu'elle couvre, et le site
en tire la liste inverse : pour une exigence, les lots, le code et les tests qui la citent.
