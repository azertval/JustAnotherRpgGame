+++
id = "LOT-129"
titre = "Les étages et les toits de la scène"
version = "0.0.1"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Le décor se bâtit en niveaux modulaires — un étage de mur posé sur un autre, une toiture au sommet — et ce qui masque le héros s'efface quand il passe derrière."
prerequis = ["LOT-103", "LOT-105"]
livrables = [
  "Le rendu d'une couche de décor à son `floor` : ses pièces sont élevées d'une hauteur d'étage que déclare le manifeste de la scène.",
  "L'effacement : une pièce d'étage qui masque le héros devient translucide, et redevient opaque quand il en sort.",
  "Dans l'éditeur, une couche d'étage se peint comme les autres, et le canevas montre ou cache les étages.",
  "`Regions/central-empire/capital/Common/Scene/` : la toiture de tuiles bourgogne de la Capitale — pans dans les deux sens, faîtage, rive, angles rentrants et sortants.",
  "Une carte d'essai dans `Source/Test/Fixtures/` : un îlot de murs du kit sur deux étages, coiffé de sa toiture, angles compris.",
]
criteres = [
  "Un mur posé sur la couche `floor = 1` prolonge le mur de la couche `floor = 0` sans décalage manuel par pièce : la hauteur vient du manifeste.",
  "Une toiture se pose au-dessus du dernier étage, quel qu'en soit le nombre, et ses angles raccordent ceux des murs.",
  "Le héros qui passe derrière un étage ou un toit reste visible : un test de rendu le montre à travers.",
  "La collision ne lit que la grille racine : ni un étage ni un toit ne bloquent une case.",
  "Une carte sans couche d'étage se rend comme avant, octet pour octet sur les références PNG.",
]
+++

## Pourquoi

C'est un **entrant du [LOT-108](LOT-108-assets-hd-arenarea.md)** : en ouvrant la commande
d'Arenarea, le besoin de **toits** est apparu, et le moteur ne sait pas en poser. Une pièce se pose
sur une case au sol ; un toit se pose **au-dessus** d'un mur. Sans étage, un bâtiment n'existe que
comme pièce unique, dessinée d'un bloc avec son toit : cela suffit pour une boutique, pas pour un
îlot de manoirs de longueur quelconque. La production du LOT-108 est suspendue jusqu'à ce lot.

Les trois zones de la démo en ont besoin : les manoirs d'Arenarea, la halle et les maisons de
Martpart ([LOT-110](LOT-110-assets-hd-martpart.md)), et le Colisée, dont l'enceinte monte **trois
ordres d'arcades superposés** ([LOT-106](LOT-106-assets-hd-arena-of-fate.md)) — trois étages du
même mur, et non trois pièces hautes.

Le format le permet déjà : le `LOT-EDITOR-12` a **réservé** la hauteur (`"floor"` par couche,
`"elevation"` par case et par entité) ; elle survit aux allers-retours et sort en avertissement du
`--check`. Ce lot fait servir `floor`.

## Périmètre

- **Dedans** : l'élévation d'une couche de décor par son étage, l'ordre de tracé entre étages,
  l'effacement devant le héros, l'éditeur (peindre, montrer, cacher un étage), l'avertissement du
  `--check` levé pour `floor` ; la toiture commune de la Capitale, sans laquelle rien ne se vérifie.
- **Pas dedans** : un personnage qui **monte** à l'étage (terrasses, remparts, coursives
  praticables) — l'`elevation` par case reste réservée ; le Colisée garde ses niveaux jouables en
  cartes distinctes ([LOT-107](LOT-107-carte-arena-of-fate.md), « deux cartes, pas deux étages »).
  Les étages **propres** à une zone (arcades du Colisée, étage de manoir, halle de Martpart) restent
  aux lots de zone.

## Conception

- La hauteur d'un étage est une **donnée du lieu** : le manifeste de la scène la déclare, en pixels
  d'art, comme il déclare déjà `"tile"`. Elle se mesure sur les murs du kit de la Capitale
  ([LOT-105](LOT-105-kit-commun-de-la-capitale.md)).
- Une pièce d'étage se trie avec les autres par la clé de profondeur du composeur, sa rangée étant
  celle de sa case au sol : un toit passe devant ce qui est derrière l'îlot, derrière ce qui est
  devant.
- La toiture suit la méthode et les critères de la V4 du LOT-105 : deux sens sans miroir, vrais
  angles rentrants et sortants, tuiles en phase d'une pièce à l'autre.

## Risques et questions ouvertes

- L'effacement : toute la pièce, ou un disque autour du héros ? À trancher sur l'essai.
- La hauteur d'étage est-elle la même pour le Colisée et pour une maison ? Si non, elle se déclare
  par scène, ce que le manifeste permet.

## Réalisation — 23 septembre 2026

Branche `lot-129-etages-et-toits`.

### Le moteur et l'éditeur — faits

- **Format** (`EX-LVL-025`, qui remplace la réserve de `floor` dans `EX-LVL-024`) : une couche de
  décor à l'étage 1 à 4 est un étage ; ailleurs, `floor` est gardé, ignoré et signalé par `--check`.
- **Hauteur d'étage** : `"storey"` au manifeste du lieu, en pixels d'art, lu comme `tile` avec les
  traits de chaque pièce. **224** pour le kit de la Capitale et celui de l'Empire : la hauteur de
  leurs murs au-dessus du sommet de leur case.
- **Composition** : l'instantané porte les couches d'étage ; une pièce d'étage s'élève de n
  hauteurs d'étage, se trie avec sa case au-dessus du rez et de la figurine (un rang de profondeur
  par étage), et porte son étage (`ComposedQuad::storey`).
- **Effacement** : une pièce d'étage dessinée après le héros et qui le recouvre prend l'opacité
  0,35 — **la pièce entière** (question ouverte tranchée : le disque est écarté, une pièce entière
  se lit mieux et ne coûte rien). Le rez, lui, ne s'efface pas.
- **Collision** : un étage n'y contribue pas ; mettre une couche à l'étage redéduit toute la
  carte.
- **Éditeur** : `LevelDraft::setLayerFloor`, par l'historique ; le panneau des couches règle l'étage
  (« Floor ») ; le pinceau à pièces peint la couche d'étage active ; chaque étage suit la visibilité
  et l'opacité de sa couche ; un préfabriqué garde l'étage de ses couches.
- **Tests** : `test_world_storeys.cpp` (instantané, élévation, tri, effacement), deux tests de
  `test_level_draft_pieces.cpp` (collision, bornes), `test_storey_editing.cpp` (pinceau, visibilité,
  préfabriqués), et `test_storey_render.cpp` : un bâtiment de murs du kit sur deux niveaux, coiffé
  d'un toit, rendu ; le héros derrière se voit sur 37 500 pixels, un PNJ au même endroit sur 2 779.
  Les cartes sans étage se rendent comme avant : 963 tests CTest verts, images de référence
  comprises.

- **Tri** : une pièce d'étage se trie au plus tôt au pied de ce qui la porte. Un mur de deux cases
  se trie au pied de sa seconde ; le toit posé sur sa première passait avant lui, et le mur en
  recouvrait l'égout. Trouvé en rendant la toiture, corrigé dans la composition, testé.

### La toiture — matières produites et installées

Tranché par l'auteur le 23 septembre : toits **à deux pans, pignons de pierre**, « style romain »,
**profondeur libre**. La commande (`Tools/AssetsHD/Regions/central-empire/capital/Common/Toitures/
commande.md`) demande au générateur **quatre matières** peintes à plat — tuiles, faîtage, rive
d'égout, corniche de pignon — et reprend la pierre des murs de la V4 pour le pignon.
`scripts/build_capital_roofs.py` les projette en **112 pièces** d'une case (deux sens, profondeur 2
à 5, quatre positions le long du faîtage) ; la méthode « surface et élévation » est écrite dans la
consigne. Avec des matières provisoires, `scripts/validate_capital_roofs.py` rend par le moteur deux
îlots de murs du kit sur deux étages, coiffés : pièces jointives, pignons, égouts, ordre de tracé
validés.

Les quatre matières ont été produites avec imagegen intégré, puis installées en 112 pièces. Sources, prompts et version provisoire conservés dans `Toitures/`. La référence panoramique de la Capitale fournie par l’auteur sert à l’ambiance architecturale ; la facture suit Arenarea et la V4.

Contrôles : six tests Python réussis, 112 images installées identiques pixel par pixel à la scène testée, ancres identiques, 34 entrées du kit antérieur préservées, budget Capitale de 7,9 Mio. Cartes `Source/Test/Fixtures/Storeys/roofs.json` (deux bâtiments à deux niveaux) et `roofs-all.json` (les 112 modules en 16 assemblages), rendues à deux échelles. Contrôle moteur sans erreur ; avertissement de catalogue de traduction absent dans la racine isolée de test. Il s’agit d’une validation de chargement et de rendu, pas d’un essai de gameplay.

La livraison initiale couvre les toits rectangulaires à deux pans. L’extension demandée ensuite par l’auteur ajoute les intersections ci-dessous ; la validation artistique finale reste à l’auteur.

### Extension des plans — L, T et X

Le catalogue comporte désormais 598 modules : les 112 droites préservées, 216 L, 216 T et 54 croisements. Chaque raccord occupe un carré de D × D cases, D allant de 2 à 5 ; il relie des ailes de même largeur et hauteur. Les L et T sont disponibles dans quatre orientations. Leur combinaison avec les tronçons droits permet des U, H et plans à plusieurs ailes. Les diagonales et raccords de largeurs différentes ne sont pas couverts.

Les matériaux peints sont réutilisés. Les pans situés à l’intérieur de l’union des volumes sont supprimés : ils ne peuvent plus ressortir sous le pignon d’une autre aile. Les PNG antérieurs des L et les scripts sont sauvegardés dans `Toitures/BeforeJunctions/`.

20 tests Python ciblés passent. Les cartes `roofs-l` et `roofs-junctions` couvrent toutes les variantes ; `roofs-buildings` présente les L et T sur deux niveaux de façades de contrôle dans une racine moteur autonome `Toitures/EngineJunctions/`. Mode d’emploi : `Toitures/JONCTIONS.md`.

### L'import dans le projet : une arborescence

598 toits et le kit : 632 fichiers à plat dans `capital/Common/Scene/`. Choix de l'auteur, le kit se
range **par famille**, les toits **par sorte puis par largeur** — `floors/`, `walls/`,
`balustrades/`, `plants/`, `props/`, `roofs/{straight,l,t,x}/d2…d5/` ; le kit impérial en `walls/`,
`props/`, `columns/`. Un seul manifeste par `Scene/`, qui cite chaque pièce par son chemin : les
clés, donc les cartes, ne changent pas. Le moteur prend le chemin dans le manifeste
(`PlaceAppearance::pieceFile`), lit l'ancre d'une image rangée dans le manifeste du lieu, la palette
de l'éditeur se groupe par dossier ; l'installateur range d'après une règle `folders` du
descripteur, que le standard d'arborescence décrit. Les 636 images installées sont identiques,
pixels et ancres, à celles d'avant le rangement.

## Livraison

Livré le 23 septembre 2026, **PR #121**, sur décision de l'auteur. 968 tests CTest et 158 tests des
scripts verts. L'éditeur ne lit pas encore les lieux rangés sous `Regions/` : il y peindra les toits
au `LOT-124`. Les scripts de fabrication et de validation des toitures, à usage unique, ont été
retirés du dépôt à la livraison.

## Exigences

Ce que ce lot réalise, ou réalisera, s'écrit dans les spécifications :

- `EX-LVL-025` — l'étage, couche de décor élevée.
- `EX-EDIT-098` — l'étage réglé, peint, montré ou caché dans l'éditeur.
