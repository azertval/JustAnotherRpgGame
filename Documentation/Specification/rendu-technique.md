# Rendu & cible technique

> Statut : **livré** pour le lieu qu'on parcourt, l'arène et le canevas de l'éditeur : scène
> composée sur QRhi (Direct3D 11), tri par profondeur, textures chargées depuis des fichiers,
> texte et traductions par Qt. Dépend de [`vision.md`](vision.md).

## 1. Cible technique
- **EX-REN-001** — Le jeu doit fonctionner sous **Windows 10/11 (x64)**.
- **EX-REN-002** — Le rendu doit s'appuyer sur **Direct3D 11**, atteint **au
  travers de QRhi** (`EX-REN-050`), qui retient ce backend par défaut sous Windows. Direct3D 12 est
  écarté : surdimensionné pour de la 2D.
- **EX-REN-003** — La fenêtre du jeu doit être redimensionnable, avec titre et
  icône ; aucun écran n'en contraint la taille (`EX-IHM-080`).

## 2. Rendu 2D

Tout ce chapitre repose sur une seule convention géométrique : la scène est **peinte en isométrie**,
et le losange de sol a un rapport hauteur/largeur de **0,62**. De là découlent l'échelle de l'art,
la façon dont une figurine se pose sur sa case, et le facteur d'affichage déduit de la fenêtre.

![Maquette de la projection isométrique : le losange de 256 × 159 pixels d'art et les deux formules qui donnent le centre d'une case, la figurine de 170 px posée par le pied dans sa cellule de 192 × 256, et la même étendue de monde cadrée à 1080p comme à 2160p avec l'art toujours réduit, jamais agrandi](maquettes/rendu-technique-projection-iso.svg)

> **Note** — Depuis le `LOT-103`, le code suit cette maquette : l'échelle de l'art se lit dans le
> manifeste du lieu (`"tile": [256, 159]`), la figurine se découpe par sa cellule entière, et le
> zoom est libre. La maquette du `LOT-101`, rendue par le moteur, est comparée à la maquette montée
> à la main par `test_hd_mockup_render.cpp`.

- **EX-REN-010** — Le rendu doit dessiner une carte à partir des **pièces de la
  planche de son lieu** (`EX-VIS-008`) : une pièce de sol par case, une pièce de relief là où la
  carte en pose une, la table d'apparence du lieu décidant laquelle.
- **EX-REN-011** — Le rendu doit dessiner les **figurines** — héros et PNJ — avec
  transparence, posées sur leur case par le pied.
- **EX-REN-012** — Une figurine doit s'animer par **bandes d'images** nommées
  (`idle`, `walk`, `attack`, `hit`, `death`), dont le découpage est décrit par des données
  (`EX-REN-005`). Depuis le `LOT-112`, une bande existe par **diagonale isométrique** — quatre
  orientations peintes, `walk-se`, `walk-sw`, `walk-ne`, `walk-nw` —, une animation compte
  **huit images** dans une cellule de 192 × 256 dont la ligne de sol est déclarée (`ground`), et
  l'attaque comme la mort occupent la cellule large de 384 × 256 (`EX-VIS-008`). L'orientation de
  la session (`EX-EXP-004`) choisit la bande ; la cadence (`frameDuration`) est lue dans la bande,
  jamais dans le code (`EX-EXP-011`).
- **EX-REN-005** — Les **animations** doivent être décrites par des **données**
  (clip nommé, suite d'images, durée par image, bouclé ou joué une fois) et non codées en dur. Un
  asset sans description d'animation est affiché comme une **image fixe**.
- **EX-REN-013** — La **caméra** du lieu doit suivre le héros, bornée à la scène
  (un axe plus étroit que la vue est centré), à un facteur d'affichage **libre** — l'agrandissement
  entier du pixel art n'a plus d'objet (`EX-ARCH-022`) — et **déduit de la définition de la
  fenêtre** : la largeur d'une case à l'écran vaut la hauteur de la fenêtre divisée par **10,8**,
  soit 100 px à 1080p et 200 px à 2160p. Toutes les définitions cadrent donc la **même étendue de
  monde** — 19,2 losanges de large, 17,4 de haut — et un écran plus fin montre le même jeu plus
  finement, jamais plus de jeu. La « fenêtre » est ici la **scène 16:9** qu'`EX-REN-019` y
  inscrit.
- **EX-REN-019** — La scène du jeu est **toujours au format 16:9** : dans une fenêtre d'un autre
  format, le plus grand rectangle 16:9 qui tient y est inscrit, **centré au pixel**, et la fenêtre
  peint le reste en **noir** — des bandes sur les côtés pour une fenêtre plus large, en haut et en
  bas pour une plus haute. Les facteurs d'échelle de l'interface et de la scène se lisent sur ce
  **rectangle**, jamais sur la fenêtre. Laisser un écran se recomposer dans n'importe quel format
  donnerait un jeu conçu à 1920 × 1080 qui n'est jamais vu tel qu'il est dessiné, et une scène posée
  à un demi-pixel flouterait tout ce qu'elle contient.

![Maquette du cadre 16:9 : la scène inscrite et centrée dans trois fenêtres, sans bande à 1280 × 720, avec des bandes en haut et en bas dans une fenêtre plus haute, sur les côtés dans une fenêtre plus large, et les facteurs d'échelle lus sur le rectangle de la scène](maquettes/rendu-technique-cadre-16-9.svg)

Les deux exigences qui suivent décident **qui passe devant qui**. Elles se complètent : le calque
tranche entre familles (le curseur est toujours au-dessus du sol), la profondeur tranche à
l'intérieur de la famille où le monde se dessine.

![Maquette de l'ordre de dessin : la pile de calques du fond vers l'interface, la bande de profondeur qui réunit relief, figurines et étages, un mur, un héros et un étage translucide triés par la hauteur de leur pied à l'écran, et la clé de tri qui multiplie la profondeur par six pour y loger le rang, relief, figurine ou étage un à quatre](maquettes/rendu-technique-ordre-de-tri.svg)

- **EX-REN-014** — Le rendu doit gérer un ordre de dessin par **calques**,
  défini par un **ordonnancement unique et explicite** (`hmi::RenderLayer`) dont aucun calque
  concurrent ne peut s'écarter : sol, objets et figurines, puis interface et aides d'édition.
- **EX-REN-018** — Dans la scène isométrique, l'ordre de dessin des acteurs et du décor
  traversé doit venir de leur **profondeur**, et non de leur calque : une entité passe devant ce qui
  est plus haut qu'elle à l'écran, derrière ce qui est plus bas. La profondeur se lit au **pied** du
  sprite — le bord bas, point de contact avec le sol — et non à son coin haut. Ces calques forment
  une **bande de profondeur** commune, à l'intérieur de laquelle le tri par profondeur passe
  **avant** le regroupement par texture. Le tri doit rester **stable** et quantifié au pixel : à
  profondeur égale, deux sprites gardent un ordre constant d'une image à l'autre. Hors de cette
  bande, l'ordre des calques reste souverain (`EX-REN-014`). Concrétisé en `LOT-07`. Depuis le
  `LOT-129`, la bande compte **six rangs** par profondeur — le relief, la figurine, puis les étages
  un à quatre (`EX-LVL-025`) — et les jetons de maquette n'y sont plus : ils renseignent, donc
  ils sont de l'interface en scène (`EX-REN-023`).

### Le rendu de maquette : une carte sans texture (`LOT-128`)

Une carte se dessine et se **joue** avant d'être habillée (décision D-22). Le rendu doit donc
avoir un mode où **rien** ne vient d'un fichier : ni sol, ni mur, ni figurine.

![Maquette du rendu sans texture : la case sans pièce dessinée en losange plat de la couleur de son type, le mur extrudé en bloc à trois faces d'une case de haut, les jetons ronds à lettre dont la couleur se déduit de ce que le format dit déjà, la flèche d'or du portail barrée s'il est condamné, la zone en pointillé, et la table de la palette qui vit dans le code](maquettes/rendu-technique-maquette-sans-texture.svg)

- **EX-REN-023** — Une case qui **ne nomme aucune pièce** se dessine en **losange plat** de la
  couleur de son type — une couleur par type, distincte de ses voisines —, et un type qui arrête le
  pas s'**extrude** en bloc à trois faces d'une case de haut ; les entités sont des **jetons**
  ronds à lettre dont la couleur se **déduit** du format (vert le joueur, jaune le PNJ qui parle ou
  qu'un drapeau conditionne, rouge l'hostile, gris le muet ; flèche d'or pour un portail, barrée
  s'il est condamné ; pointillé pour une zone). La **palette vit dans le code** : une maquette doit
  se dessiner quand aucun fichier d'asset n'est présent, et une palette chargée du disque
  réintroduirait la dépendance que le mode supprime. Le repli se déclenche sur « cette case n'a
  nommé aucune pièce », **pas** sur « la carte n'a pas de lieu » : la carte sans lieu et la pièce
  que le lieu ne couvre pas — l'eau du Colisée — se referment du même geste. Le jeu et l'éditeur
  montrent la **même image**, jetons compris (`EX-EDIT-059`, `LevelEditor --render --plan`).
- **EX-REN-041** — Le rendu doit **charger ses textures depuis des fichiers
  image** (PNG au minimum), décodés en pixels RGBA puis créés en texture GPU. Le filtrage suit la
  **nature de l'asset** (`EX-ARCH-022`) — refondu au `LOT-66` : figer le plus proche voisin aurait
  rendu crénelée toute illustration peinte.
- **EX-REN-042** — Les **assets graphiques** doivent être **externalisés en
  fichiers** éditables hors code (remplacer le fichier suffit à changer l'apparence), copiés à côté
  de l'exécutable comme les cartes et les traductions. Les images des kits viennent du **verrou**
  et non de Git (`EX-CNT-070`) : « remplacer le fichier » se fait par une publication, jamais par
  un commit.
- **EX-REN-043** — Le rendu doit pouvoir dessiner, en une seule image, des
  primitives provenant de **plusieurs textures distinctes**, selon l'ordonnancement de calques
  unique de `EX-REN-014`.
- **EX-REN-007** — Un asset graphique absent ou illisible n'interrompt jamais le
  rendu : il est remplacé par un **repli visible** (damier, ou marqueur généré pour une figurine) et
  journalisé une fois avec le **nom du fichier** (`EX-NFR-040`).

## 3. Boucle & temps
- **EX-REN-020** — Le jeu doit tourner à **60 images/seconde** cible.
- **EX-REN-021** — La logique doit être mise à jour à **pas de temps fixe**
  (simulation déterministe), le rendu étant découplé.
- **EX-REN-022** — La présentation doit être synchronisée pour éviter le
  *tearing* : elle appartient au **compositeur de Qt** (`EX-REN-050`).

## 4. Interface (HMI)
- **EX-REN-030** — Le jeu doit afficher un **menu principal** (nouvelle partie,
  options, crédits, quitter).
- **EX-REN-031** — Le jeu affiche un écran de **pause** (Échap en exploration),
  qui suspend réellement la simulation. Détaillé côté interface par `EX-IHM-004`.
- **EX-REN-032** — Le jeu doit afficher son **texte** avec des polices
  **vectorielles embarquées** — un titrage à empattements et un corps de lecture, conformes à la
  charte (`LOT-66`, `LOT-87`). Le texte est rendu par Qt.
- **EX-REN-033** — Tout **texte affiché** doit passer par un **catalogue de
  traduction** : le code référence des **clés** stables, résolues vers une chaîne selon la **langue
  active**, chargée depuis un **fichier par langue** (français par défaut). Aucun libellé
  d'interface n'est codé en dur, afin de rendre l'ajout d'une langue trivial. Une clé ou un fichier
  de langue manquant est traité comme une **erreur récupérable** (repli déterministe), cf.
  `EX-NFR-040`.

## 5. Audio
- **EX-REN-047** — La lecture audio vit **entièrement dans `HMI`**
  (`hmi::AudioEngine`, Qt Multimedia) : la simulation reste pure, déterministe et testable **sans
  périphérique audio** (`EX-NFR-010`), et le son n'a **aucun effet** sur elle (`EX-ARCH-012`). Qt
  Multimedia est provisionné selon `EX-BUILD-010`.
- **EX-REN-048** — Le **volume** est réglable depuis les options, prend effet
  immédiatement et est **persisté**. L'absence de périphérique audio est une **erreur
  récupérable** (`EX-NFR-040`) : le jeu reste pleinement jouable en silence.

## 6. Surface de rendu
- **EX-REN-050** — Le rendu doit être présenté dans un **élément composé avec
  l'interface** — `QQuickRhiItem` pour le jeu, une `QGraphicsView` peinte par `QPainter` pour
  l'éditeur depuis le `LOT-EDITOR-02` — et jamais dans une **fenêtre native** embarquée : un
  élément frère d'une fenêtre native ne se dessine pas de façon fiable par-dessus elle.

## Exigences retirées {#ren-retirees}

> Ancres conservées, jamais renumérotées : les lots livrés s'y réfèrent.

- **EX-REN-004** *(retirée au `LOT-88`)* — modèle de présentation DXGI : la
  présentation appartient à Qt (`EX-REN-022`).
- **EX-REN-006** *(retirée au `LOT-88`)* — apparence des mécanismes selon leur
  état : le jeu n'a pas de mécanismes.
- **EX-REN-008** *(retirée au `LOT-88`)* — effets de traînée, de poussière et de
  secousse d'écran.
- **EX-REN-009** *(retirée au `LOT-88`)* — planche du personnage de plateforme ;
  les figurines sont `EX-REN-011`.
- **EX-REN-015** *(retirée au `LOT-88`)* — caméra par salle.
- **EX-REN-016** *(retirée au `LOT-88`)* — trois modes de cadrage choisis par le
  niveau.
- **EX-REN-017** *(retirée au `LOT-88`)* — taille de zone de caméra par niveau.
- **EX-REN-040** *(retirée au `LOT-88`)* — bruitages de saut, de mécanisme et de
  fin de tableau.
- **EX-REN-044** *(retirée au `LOT-88`)* — image de fond d'un niveau.
- **EX-REN-045** *(retirée au `LOT-88`)* — ombres portées des tuiles solides.
- **EX-REN-046** *(retirée au `LOT-88`)* — bascule de rendu Physique/Texture.
- **EX-REN-049** *(retirée au `LOT-88`)* — composition des plans picturaux.

## Traçabilité
Tout ce qui touche fenêtre, rendu, entrées et interface relève de `Source/HMI` ; la logique de
simulation reste dans `Source/Core`. Contraintes de performance :
[`exigences-non-fonctionnelles.md`](exigences-non-fonctionnelles.md).
