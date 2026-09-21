# LOT-87 — Cahier des assets de la charte v2

> Tâche **T2.4** du [LOT-87](../../lots/LOT-87-charte-v2.md). Source : `assets-brief.json`, son contrat
> `assets-brief.schema.json`, son garde-fou `scripts/check_assets_brief.py`.
> Exigences servies : [`EX-IHM-075`](../../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-075) (ornements produits, décrits par ce cahier),
> [`EX-IHM-076`](../../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-076) (provenance « produite »), [`EX-IHM-053`](../../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-053)
> (icônes, refondue par cette tâche).

## À quoi sert ce cahier

Aucun modèle Claude ne dessine un cadre doré ni un fond peint. Ce cahier est ce qui part vers le
générateur d'images ou l'illustrateur : **la liste exhaustive des pièces** que les écrans v2
consomment, et pour chacune ce qu'il faut pour la produire sans revenir poser une question —
taille, marges, fond, états, description, prompt, et la zone de maquette qui la montre. Il est aussi
ce que le T2.6 lira pour **réceptionner** les images : une image qui ne répond à aucune entrée est
refusée, une entrée sans image reste « non livrée ».

Le cahier vit en deux jumeaux. `assets-brief.json` est la **source** : le script le valide et en
assemble les prompts. Cette page en est la **lecture** : les tables plus bas sont engendrées depuis
le JSON (`--write`), et la CI échoue si elles ne le suivent plus. On corrige le JSON, jamais les
tables.

## Lire une entrée

| Champ | Ce qu'il dit |
|---|---|
| **Clé** | `ui/<famille>/<pièce>`, à la syntaxe des clés d'assets du `LOT-39` (`$defs/assetKey` de `common.schema.json`). Le préfixe `ui` la sépare des familles d'entité (`beast`, `item`…). Une pièce à états ou à membres engendre une clé par variante : `ui/button/menu/hover`, `ui/icon/skill/stealth`. |
| **Fichier** | La clé, préfixe ôté, sous `Source/Elements/Assets/UI/` : `ui/button/menu/hover` → `button/menu/hover.png`. Le T2.6 installe là. |
| **Production** | Taille exacte de l'image livrée, en pixels, **à la définition de conception 1920 × 1080**. « Opaque » quand le fond n'est pas transparent. |
| **Tenue** | `9-patch g / h / d / b` : marges gauche, haut, droite, bas ; seuls les bords et le centre s'étirent ou se répètent (`BorderImage`), jamais les coins. Marges haute et basse nulles : la pièce ne s'étire qu'en largeur. `fixe` : la pièce garde ses proportions, multipliée par `uiScale` ; « affichée » donne les tailles auxquelles les écrans la réduisent. `tuile` : répétée. `plein écran` : couvre la fenêtre. |
| **Variantes** | États (`normal`, `hover`, `pressed`, `disabled`…) ou membres d'un jeu d'icônes. Chacune a sa phrase de prompt. |
| **Maquettes** | Numéro de maquette et zone `(x0, y0, x1, y1)` en pixels **de la maquette elle-même** (qui mesure de 1492 à 1672 px de large, pas 1920). On y relève une forme, on n'y prélève aucun pixel. |
| **Écrans** | Les tâches des phases 3 et 4 qui posent la pièce. Chaque pièce en nomme au moins une ; chaque écran en consomme au moins une. |
| **Repli** | Les jetons de l'aplat qui tient la place tant que la pièce n'est pas livrée (`EX-IHM-075`, `EX-NFR-040`) : aucun écran ne dépend d'une image pour s'afficher. |

## Les prompts

Un prompt n'est **pas recopié** dans le JSON : il est **assemblé** au moment de l'envoi, pour que la
palette qu'il impose soit toujours celle de `Tokens.qml`.

```
python scripts/check_assets_brief.py --prompt ui/button/menu/hover   # une variante
python scripts/check_assets_brief.py --prompt ui/icon/skill          # toutes celles d'une pièce
python scripts/check_assets_brief.py --prompt all                    # le cahier entier
```

L'assemblage met bout à bout, dans cet ordre : le **style** commun ; la **matière** de la pièce
(`dark`, `parchment`, `scene`, `neutral`), dont chaque `{jeton}` est remplacé par sa valeur lue dans
`Tokens.qml` ; l'**interdit des lettres**, qui nomme les familles `titleFamily` et `bodyFamily` ; le
**prompt propre** à la pièce ; la phrase de sa **variante** ; la **toile** (taille exacte, fond) ; et
la **règle d'étirement** (9-patch avec ses marges, tranche horizontale, pièce fixe, tuile, plein écran).
La palette et les polices sont donc dans chaque prompt, comme le plan l'exige, sans qu'une seule
valeur soit écrite deux fois. Changer un jeton change le prompt suivant.

Les prompts sont en **anglais** : c'est la langue que les générateurs d'images suivent le mieux.
Descriptions, libellés et documentation restent en français. Le prompt tel qu'envoyé, avec sa date,
est ce que le manifeste retiendra de chaque image livrée (T2.5).

## Ce que la lecture des maquettes a tranché

Le plan donnait une liste de familles ; la lecture écran par écran l'a complétée et, par endroits,
contredite. Chaque écart ci-dessous est dans le JSON.

- **Les icônes du jeu sont des images, et `EX-IHM-053` est refondue.** Les icônes des maquettes sont
  des émaux et des gravures d'or en relief : un tracé vectoriel en ferait une troisième direction
  graphique, la faute que la charte v2 écarte déjà pour les ornements. Elles sont donc produites
  comme eux (`EX-IHM-075`), avec une seule différence : **à deux fois leur taille d'affichage**,
  128 × 128 pour un affichage de 32 à 64 px. Une icône est la pièce la plus petite de l'écran, donc
  celle qu'un agrandissement au-delà de 1080p abîme le plus vite, et doubler sa définition ne coûte
  presque rien. Les icônes de l'**éditeur** restent vectorielles : l'éditeur est un outil, pas la
  charte du jeu.
- **Six onglets d'options, et non huit.** La maquette 05 en montre six ; le T3.2 en décrit six. S'y
  ajoutent six emblèmes de section (langue, interface, sauvegarde, difficulté, réseau,
  confidentialité).
- **Un remplissage de jauge n'est pas une image.** Le plan prévoyait trois remplissages peints ;
  `EX-IHM-075` veut que la couleur d'un remplissage vienne des jetons. Le cahier livre un **fond de
  jauge** (avec ses embouts) et un **relief en niveaux de gris** (`ui/gauge/fill-sheen`) posé sur un
  aplat coloré par les jetons. Une jauge de vie et une jauge d'or partagent la même image.
- **Aucune lettre, sauf le logotype.** La signature de la fiche devient un **paraphe** sans nom
  (le nom change avec le personnage et se pose en `loreFamily`) ; les points cardinaux des boussoles
  et de la mini-carte sont du texte. Le **logotype** (`ui/plate/logo`) est la seule pièce qui porte
  des lettres : un logotype est un dessin, et le nom du jeu ne se traduit pas.
- **Un 9-patch n'a pas de milieu fixe.** L'étoile au centre du séparateur des crédits est une pièce à
  part (`ui/ornament/divider-star`), posée entre deux séparateurs.
- **Pièces ajoutées** parce qu'un écran les montre et qu'aucune famille du plan ne les portait :
  plaque de citation, bandeau de boussole, barre d'état, ruban de parchemin (HUD), rangée d'école
  de magie, étiquette et ligne de champ de la fiche, case de valeur, pastille de maîtrise, bouton
  rond, pastille de sort, bouton « Par défaut » (plaque `info`), anneau de mini-carte, roue d'action,
  case d'action, pastille de niveau, cadran du jour, rose des vents en couleur ; et les icônes des
  crédits, des propriétés d'un sort, des ressources et des compteurs de l'équipe.
- **Pièces sans maquette.** La liste ouverte d'un menu déroulant et les liserés de rareté ne sont
  montrés nulle part ; chacune déclare la pièce qu'elle prolonge (`derivedFrom`).
- **Le panneau parchemin a deux états** : simple, et relié d'une bande de cuir grenat (maquette 03).

## Produire, dans quel ordre

Le chemin critique du lot passe par ici : **aucun écran de la phase 3 ne commence avant que les
fonds, cadres, plaques, boutons et contrôles soient livrés** (clôture du T2.6). L'ordre de production
est donc celui des tables : fonds, cadres, plaques et onglets, boutons, contrôles d'abord ; médaillons,
emplacements, jauges, ornements et icônes peuvent suivre pendant la phase 3.

Pour chaque pièce :

1. assembler le prompt (`--prompt <clé>`), l'envoyer tel quel ;
2. vérifier à l'œil ce que le script ne peut pas voir : aucune lettre, les ornements tenus dans les
   marges, un milieu uniforme ;
3. nommer le fichier d'après la clé et le remettre au T2.6, qui contrôle dimensions, transparence et
   marges, puis l'installe et l'inscrit au manifeste avec le prompt et la date.

## Vérifier le relevé

Les zones ont été relevées sur les maquettes puis contrôlées une à une en les dessinant dessus :

```
python scripts/check_assets_brief.py --annotate <dossier>
```

Le script écrit dans `<dossier>` chaque maquette avec ses rectangles et le nom de la pièce ; c'est
ainsi qu'une zone qui mordait sur un bord de carte a été corrigée. Sans option, il contrôle le cahier
— schéma, jetons existants, clés uniques, marges qui laissent un milieu, zones dans leur maquette,
pièces dérivées qui nomment une pièce réelle, chaque maquette lue, chaque écran servi, cette page à
jour — et sort en erreur à la première faute.

## Laissé ouvert, et à qui

- **La couleur des remplissages de jauge** (vie, expérience, poids) n'est pas encore un jeton : le
  repli nomme `gemLight` et `goldLight`. Le T3.4 la relève sur la maquette 03 avec
  `measure_mockup_palette.py` s'il faut des rôles dédiés.
- **Le centre répété d'un panneau à grain** peut laisser voir sa trame. Le T2.6 le juge à la
  réception, sur un panneau posé à 1920 × 1080 ; si la trame se voit, la pièce repasse en `stretch`.
- **Les écussons de classe** des membres de l'équipe (maquette 09) ne sont pas au cahier : ils
  dépendent des classes jouables, et le T3.7 les y ajoute avec leurs couleurs.

## Le cahier

<!-- DEBUT DES TABLES ENGENDREES : scripts/check_assets_brief.py --write -->

Le cahier compte **80 pièces**, qui engendrent **214 images** (une par état ou par membre).

### Fonds {#lot-87-cahier-background}

4 pièces, 4 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/background/menu-scene` | Scène du menu et des crédits | 1920 × 1080, opaque | plein écran | — | 06 (0, 0, 1672, 941)<br>07 (0, 0, 1672, 941) | T3.1, T3.3 |
| `ui/background/options-backdrop` | Décor flouté des options | 1920 × 1080, opaque | plein écran | — | 05 (0, 0, 1536, 1024) | T3.2, T3.9 |
| `ui/background/parchment-tile` | Parchemin répétable | 512 × 512, opaque | tuile | — | 03 (1110, 880, 1470, 960)<br>09 (40, 605, 470, 640) | T3.4, T3.5, T3.7, T3.8, T3.9 |
| `ui/background/leather-tile` | Cuir sombre répétable | 512 × 512, opaque | tuile | — | 03 (0, 1000, 1536, 1024)<br>10 (0, 922, 1672, 941) | T3.4, T3.5, T3.7, T3.8 |

- **`ui/background/menu-scene`** — Vallée au couchant, cité fortifiée au centre-droit, compagnie de quatre aventuriers de dos sur un promontoire à droite, étendard grenat à gauche. Le tiers gauche reste calme et sombre : la colonne du menu et le logo s'y posent.
  *Prompt propre :* « Epic painted landscape at golden-hour sunset: a river valley with a walled castle city on a hill centre-right, misty mountains, dramatic clouds; four adventurers seen from behind on a rocky outcrop in the right third (archer, armoured warrior, fur-clad dwarf, hooded mage with a glowing staff), a campfire and lantern at the bottom right. The left third is darker and quiet, with a tall garnet war standard at the far left edge, leaving room for a menu column. »
  *Repli :* `panel`.
- **`ui/background/options-backdrop`** — Paysage de forêt et de citadelle, flou et assombri, sans point focal : il ne doit rien disputer au panneau des réglages. Peut être obtenu en floutant la scène du menu hors du jeu, tant que le résultat est déclaré sous cette clé.
  *Prompt propre :* « Out-of-focus painted landscape of a forested valley and a distant citadel, heavy gaussian-like blur, darkened by about 40 %, low contrast, no focal point, warm dusk light. »
  *Repli :* `panel`.
- **`ui/background/parchment-tile`** — Champ de parchemin vieilli, taches et fibres, sans pli ni motif reconnaissable : il se répète sous les documents du personnage.
  *Prompt propre :* « Seamless aged parchment texture: fibres, faint stains and foxing, very subtle cracks, even lighting, no folds, no burns, no drawings, no focal feature. »
  *Repli :* `surface`.
- **`ui/background/leather-tile`** — Le fond sombre et usé sur lequel est posée la feuille — cuir tanné ou pierre patinée, sans relief marqué.
  *Prompt propre :* « Seamless dark worn leather texture, deep brown-black with grey scuffs, soft grain, even lighting, no stitching, no emboss, no focal feature. »
  *Repli :* `background`.

### Cadres 9-patch {#lot-87-cahier-frame}

6 pièces, 16 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/frame/panel-dark` | Panneau sombre à filets d'or | 512 × 512 | 9-patch 112 / 112 / 112 / 112 | — | 05 (430, 110, 1430, 940)<br>05 (95, 65, 440, 890)<br>07 (380, 170, 1290, 830) | T3.2, T3.3, T3.9 |
| `ui/frame/panel-parchment` | Feuille de parchemin rivetée | 768 × 768 | 9-patch 128 / 128 / 128 / 128 | `plain`, `bound` | 03 (15, 15, 1525, 1000)<br>04 (15, 15, 540, 1010)<br>09 (15, 15, 1655, 925)<br>10 (15, 10, 1655, 925) | T3.4, T3.5, T3.7, T3.8, T3.9 |
| `ui/frame/subpanel-dark` | Sous-panneau sombre | 256 × 256 | 9-patch 48 / 48 / 48 / 48 | — | 05 (460, 160, 918, 345)<br>01 (1398, 278, 1658, 485)<br>01 (20, 678, 400, 835)<br>06 (1355, 30, 1630, 100)<br>08 (1030, 918, 1478, 1015)<br>08 (940, 487, 1122, 535) | T3.1, T3.2, T3.6, T4.1 |
| `ui/frame/subpanel-parchment` | Sous-panneau parchemin | 256 × 256 | 9-patch 40 / 40 / 40 / 40 | `normal`, `empty` | 09 (508, 455, 728, 578)<br>10 (48, 188, 465, 300)<br>03 (548, 140, 1080, 540)<br>08 (15, 578, 230, 928) | T3.4, T3.6, T3.7, T3.8 |
| `ui/frame/item-card` | Cadre de fiche d'objet | 512 × 768 | 9-patch 96 / 160 / 96 / 96 | — | 04 (1090, 20, 1520, 1010) | T3.5, T3.9 |
| `ui/frame/school-row` | Rangée d'école de magie | 512 × 96 | 9-patch 96 / 24 / 48 / 24 | `abjuration`, `conjuration`, `divination`, `enchantment`, `evocation`, `illusion`, `necromancy`, `transmutation`, `thaumaturgy` | 10 (510, 155, 1140, 228)<br>10 (510, 237, 1140, 305) | T3.8 |

- **`ui/frame/panel-dark`** — Grand panneau laqué presque noir, double filet d'or, coins ornés de filigranes qui débordent vers l'extérieur. Porte les réglages, les crédits, la pause et le journal.
  *Prompt propre :* « Large rectangular panel: near-black lacquered centre with a faint grain, framed by a double bright gold filet; each corner carries an ornate gold filigree bracket that slightly overhangs outward. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/frame/panel-parchment`** — La feuille entière d'un document du personnage : bord déchiré et bruni, liseré d'or, rivets de laiton aux coins. Variante reliée : une bande de cuir grenat le long du bord gauche (maquette 03).
  *Prompt propre :* « Full document sheet: aged parchment with a torn, browned deckle edge, a thin antique gold inner border, and a brass rivet at each corner. »
  *États :* `plain` simple (« no binding ») ; `bound` reliée (« a worn garnet leather binding strip runs along the whole left edge, inside the left margin »).
  *Repli :* `surface`, `border`.
- **`ui/frame/subpanel-dark`** — Encart sombre à filet d'or simple, petits fleurons d'angle. Sections des options, quêtes et journal du HUD, encart de profil, barre de navigation, info-bulle de la carte.
  *Prompt propre :* « Small inset panel: near-black raised centre, single thin bright gold filet, a tiny gold fleuron at each corner. »
  *Repli :* `panelRaised`, `panelEdge`.
- **`ui/frame/subpanel-parchment`** — Encart de parchemin plus clair, liseré brun doré fin, coins légèrement ornés. Cartes de bâtiment, entrées d'attaque, colonne d'identité, légende de la carte.
  *Prompt propre :* « Small inset panel on parchment: slightly lighter parchment centre, thin brown-gold border line, discreet ornamental corner ticks. »
  *États :* `normal` normal (« resting state ») ; `empty` vide (« centre washed out and faded, border dashed-looking and dimmer »).
  *Repli :* `surfaceAlt`, `border`.
- **`ui/frame/item-card`** — Colonne de parchemin encadrée d'un liseré d'or, bandeau sombre en tête pour le nom de l'objet, fanion grenat en coin supérieur droit (posé à part : `ui/ornament/crest-pennant`).
  *Prompt propre :* « Tall parchment card with an antique gold frame and brass corner caps; the top 128 px hold a dark horizontal band framed in gold, left empty for a title. »
  *Repli :* `surface`, `border`.
- **`ui/frame/school-row`** — Rangée laquée teintée par école, filet d'or, bout droit en pointe. Le médaillon d'école se pose par-dessus à gauche.
  *Prompt propre :* « Horizontal lacquered strip with a thin gold rim and a slightly pointed right end; a darker circular socket shadow at the left where a medallion will sit. »
  *États :* `abjuration` abjuration (« deep sapphire blue lacquer ») ; `conjuration` invocation (« dark ember red lacquer ») ; `divination` divination (« dark violet lacquer ») ; `enchantment` enchantement (« dark ochre gold lacquer ») ; `evocation` évocation (« dark burnt orange lacquer ») ; `illusion` illusion (« dark teal lacquer ») ; `necromancy` nécromancie (« dark plum lacquer ») ; `transmutation` transmutation (« dark emerald green lacquer ») ; `thaumaturgy` thaumaturgie (« charcoal grey lacquer »).
  *Repli :* `panelRaised`, `panelEdge`.

### Plaques et bandeaux {#lot-87-cahier-plate}

10 pièces, 10 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/plate/title-garnet` | Plaque de titre grenat | 768 × 120 | 9-patch 160 / 0 / 160 / 0 | — | 07 (615, 110, 1055, 210)<br>09 (120, 20, 690, 90)<br>10 (130, 12, 480, 72) | T3.3, T3.7, T3.8 |
| `ui/plate/title-black` | Plaque de titre noire | 640 × 96 | 9-patch 120 / 0 / 120 / 0 | — | 03 (545, 65, 1062, 132)<br>03 (1108, 55, 1432, 110)<br>04 (1120, 100, 1500, 160)<br>01 (190, 35, 445, 78)<br>08 (115, 30, 512, 120) | T3.4, T3.5, T3.6, T4.1 |
| `ui/plate/section-banner` | Bandeau de section | 512 × 56 | 9-patch 64 / 0 / 64 / 0 | — | 09 (495, 110, 730, 146)<br>09 (55, 412, 295, 444)<br>10 (45, 155, 195, 184)<br>03 (640, 550, 830, 585) | T3.4, T3.6, T3.7, T3.8 |
| `ui/plate/section-bar` | Barre de section large | 1024 × 56 | 9-patch 48 / 0 / 96 / 0 | — | 10 (505, 97, 1142, 146)<br>10 (1170, 97, 1598, 142)<br>05 (460, 160, 918, 205) | T3.2, T3.8 |
| `ui/plate/field-tag` | Étiquette de champ | 192 × 40 | 9-patch 16 / 0 / 40 / 0 | — | 03 (575, 155, 718, 192)<br>03 (655, 225, 785, 252) | T3.4 |
| `ui/plate/logo` | Logotype du jeu | 1024 × 640 | fixe, affichée 532 × 332, 450 × 282 | — | 06 (110, 25, 640, 345)<br>07 (95, 5, 530, 285) | T3.1, T3.3 |
| `ui/plate/quote` | Plaque de citation | 512 × 56 | 9-patch 64 / 0 / 64 / 0 | — | 06 (28, 858, 392, 910) | T3.1 |
| `ui/plate/compass-bar` | Bandeau de boussole | 768 × 48 | 9-patch 96 / 0 / 96 / 0 | — | 01 (585, 35, 1082, 78)<br>08 (578, 38, 938, 80) | T3.6, T4.1 |
| `ui/plate/status-bar` | Barre d'état | 512 × 48 | 9-patch 24 / 0 / 64 / 0 | — | 01 (0, 870, 388, 915)<br>08 (22, 965, 395, 1020) | T3.6, T4.1 |
| `ui/plate/parchment-ribbon` | Ruban de parchemin | 512 × 48 | 9-patch 64 / 0 / 64 / 0 | — | 01 (1368, 872, 1662, 910) | T4.1 |

- **`ui/plate/title-garnet`** — Plaque d'émail grenat à relief, bordée d'or, avec une agrafe ornementale de chaque côté. Titre d'écran en `Cinzel`.
  *Prompt propre :* « Horizontal title plate of garnet enamel with a subtle brushed texture, bordered by gold; each end carries an ornate gold clasp with a small spike; the middle section is plain and uniform. »
  *Repli :* `gem`, `panelEdge`.
- **`ui/plate/title-black`** — Plaque d'ébène à filet d'or posée sur le parchemin, bouts en pointe ouvragée.
  *Prompt propre :* « Horizontal title plate of black lacquered wood with a thin gold filet, both ends shaped into ornate gold arrow-like finials; the middle section is plain and uniform. »
  *Repli :* `panel`, `accent`.
- **`ui/plate/section-banner`** — Ruban sombre court, petit losange d'or à gauche, pointe à droite. Intertitres des documents.
  *Prompt propre :* « Short dark ribbon banner with a gold edge line, a small gold diamond stud at the left end and a notched point at the right end; uniform middle. »
  *Repli :* `panel`, `accent`.
- **`ui/plate/section-bar`** — Barre sombre pleine largeur de colonne, filet d'or, pointe à droite ; porte un titre à gauche et une valeur à droite.
  *Prompt propre :* « Wide dark header bar with a bright gold top and bottom filet, a small gold star stud at the left end and a long pointed gold tip at the right end; uniform middle. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/plate/field-tag`** — Petite étiquette sombre en flèche vers la droite, qui nomme un champ de la fiche.
  *Prompt propre :* « Small dark label tag with a thin gold edge, square left end and an arrow-shaped right end; uniform middle. »
  *Repli :* `panel`, `accent`.
- **`ui/plate/logo`** — Le titre « Just Another RPG Game » en capitales dorées en relief sur un étendard grenat déchiré, adossé à un anneau de boussole. Seule pièce du cahier qui porte des lettres : un logotype est un dessin, et le nom du jeu ne se traduit pas.
  *Prompt propre :* « Game logotype: the words 'JUST ANOTHER' above 'RPG GAME' in carved, bevelled gold Roman capitals in the spirit of Cinzel, with a red gem set between the two lines; behind them a torn garnet cloth banner and a large gold compass ring with a four-pointed star. »
  *Repli :* `gem`, `goldLight`.
- **`ui/plate/quote`** — Cartouche sombre étroit, fleuron d'or à chaque bout ; porte une citation en `loreFamily`.
  *Prompt propre :* « Narrow dark cartouche with a thin gold filet and a round gold fleuron at each end; uniform middle. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/plate/compass-bar`** — Réglette sombre à graduations d'or et agrafes aux bouts ; les points cardinaux défilent par-dessus.
  *Prompt propre :* « Horizontal dark bar with fine gold tick marks along the bottom edge, a gold chevron marker at the exact centre top, and ornate gold end clasps; the tick pattern repeats evenly. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/plate/status-bar`** — Barre sombre basse, bout droit en pointe : date, heure et lieu.
  *Prompt propre :* « Low dark bar with a thin gold top filet, flat left end, pointed right end with a small gold stud; uniform middle. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/plate/parchment-ribbon`** — Ruban de parchemin aux bouts roulés : indicateur de mode du HUD.
  *Prompt propre :* « Horizontal parchment ribbon with rolled, slightly curled ends and a thin brown edge; uniform middle. »
  *Repli :* `surface`, `border`.

### Onglets {#lot-87-cahier-tab}

2 pièces, 5 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/tab/ribbon` | Ruban d'onglet | 256 × 56 | 9-patch 48 / 0 / 48 / 0 | `active`, `normal`, `disabled` | 04 (560, 68, 1068, 112)<br>09 (775, 28, 1532, 84)<br>10 (505, 18, 1332, 66) | T3.5, T3.7, T3.8 |
| `ui/tab/segment` | Segment d'onglet | 320 × 56 | 9-patch 32 / 0 / 32 / 0 | `active`, `normal` | 10 (48, 100, 472, 148) | T3.8 |

- **`ui/tab/ribbon`** — Onglet en ruban à bouts biseautés : grenat pour l'actif, sombre sinon.
  *Prompt propre :* « Tab ribbon with bevelled ends and a gold rim; uniform middle. »
  *États :* `active` actif (« garnet enamel face, bright gold rim ») ; `normal` normal (« near-black face, gold rim ») ; `disabled` désactivé (« near-black face, dull bronze rim, flattened »).
  *Repli :* `gem`, `panel`, `panelEdge`.
- **`ui/tab/segment`** — Bouton segmenté à deux positions sur parchemin : grenat actif, parchemin clair inactif.
  *Prompt propre :* « Segmented tab with softly rounded ends and a gold border; uniform middle. »
  *États :* `active` actif (« garnet enamel face ») ; `normal` normal (« light parchment face »).
  *Repli :* `gem`, `surfaceAlt`, `border`.

### Boutons {#lot-87-cahier-button}

9 pièces, 35 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/button/menu` | Bouton de menu | 480 × 72 | 9-patch 96 / 0 / 48 / 0 | `normal`, `hover`, `active`, `disabled` | 06 (150, 355, 540, 428)<br>06 (163, 433, 527, 503)<br>05 (112, 195, 432, 258)<br>05 (115, 265, 420, 327) | T3.1, T3.2 |
| `ui/button/primary` | Bouton primaire | 280 × 56 | 9-patch 48 / 0 / 48 / 0 | `normal`, `hover`, `pressed`, `disabled` | 04 (1138, 752, 1452, 812)<br>10 (743, 860, 915, 897) | T3.5, T3.8, T3.9 |
| `ui/button/secondary` | Bouton secondaire | 280 × 56 | 9-patch 48 / 0 / 48 / 0 | `normal`, `hover`, `pressed`, `disabled` | 04 (1143, 815, 1447, 870)<br>04 (898, 878, 1057, 937)<br>05 (973, 772, 1187, 815) | T3.2, T3.5, T3.9 |
| `ui/button/apply` | Bouton Appliquer | 280 × 56 | 9-patch 48 / 0 / 48 / 0 | `normal`, `hover`, `pressed`, `disabled` | 05 (1218, 866, 1402, 920) | T3.2 |
| `ui/button/cancel` | Bouton Annuler | 280 × 56 | 9-patch 48 / 0 / 48 / 0 | `normal`, `hover`, `pressed`, `disabled` | 05 (1032, 866, 1212, 920) | T3.2 |
| `ui/button/default` | Bouton Par défaut | 280 × 56 | 9-patch 48 / 0 / 48 / 0 | `normal`, `hover`, `pressed`, `disabled` | 05 (466, 866, 672, 920) | T3.2 |
| `ui/button/back` | Bouton Retour | 320 × 72 | 9-patch 96 / 0 / 48 / 0 | `normal`, `hover`, `pressed`, `disabled` | 07 (38, 772, 302, 843) | T3.3, T3.8 |
| `ui/button/round` | Bouton rond | 64 × 64 | fixe, affichée 40 × 40, 64 × 64 | `normal`, `hover`, `pressed`, `disabled` | 08 (1432, 248, 1475, 285)<br>01 (1418, 790, 1485, 860)<br>10 (1108, 180, 1132, 205) | T3.6, T3.8, T4.1 |
| `ui/button/chip` | Pastille de sort | 192 × 40 | 9-patch 24 / 0 / 24 / 0 | `normal`, `selected`, `disabled` | 10 (757, 182, 862, 218)<br>10 (925, 257, 1040, 293) | T3.8 |

- **`ui/button/menu`** — Bouton long à bouts biseautés, douille ronde à gauche pour l'icône. Actif : émail grenat. Sert aussi aux onglets verticaux des options, qui ont la même forme.
  *Prompt propre :* « Long button with bevelled pointed ends and an ornate gold rim; at the left end a round gold socket (about 64 px) left empty for an icon; uniform middle. »
  *États :* `normal` normal (« near-black face, gold rim ») ; `hover` survol (« near-black face, brighter gold rim, faint warm glow ») ; `active` actif (« garnet enamel face with a lit centre, bright gold rim ») ; `disabled` désactivé (« near-black face, dull bronze rim, flattened »).
  *Repli :* `panel`, `gemLight`, `panelEdge`.
- **`ui/button/primary`** — Bouton d'émail grenat à cadre d'or ouvragé.
  *Prompt propre :* « Button of garnet enamel with an ornate gold frame; uniform middle. »
  *États :* `normal` normal (« resting state ») ; `hover` survol (« gold rim brightened, faint warm inner glow ») ; `pressed` enfoncé (« face slightly darker and inset by 2 px, rim unchanged ») ; `disabled` désactivé (« desaturated, gold turned to dull bronze, face flattened »).
  *Repli :* `gem`, `gemLight`, `panelEdge`.
- **`ui/button/secondary`** — Bouton sombre à cadre d'or.
  *Prompt propre :* « Button of near-black lacquer with a gold frame; uniform middle. »
  *États :* `normal` normal (« resting state ») ; `hover` survol (« gold rim brightened, faint warm inner glow ») ; `pressed` enfoncé (« face slightly darker and inset by 2 px, rim unchanged ») ; `disabled` désactivé (« desaturated, gold turned to dull bronze, face flattened »).
  *Repli :* `panel`, `panelEdge`.
- **`ui/button/apply`** — Plaque verte : c'est la plaque qui dit « valider », la coche est crème.
  *Prompt propre :* « Button of dark emerald enamel with a gold frame and pointed ends; uniform middle. »
  *États :* `normal` normal (« resting state ») ; `hover` survol (« gold rim brightened, faint warm inner glow ») ; `pressed` enfoncé (« face slightly darker and inset by 2 px, rim unchanged ») ; `disabled` désactivé (« desaturated, gold turned to dull bronze, face flattened »).
  *Repli :* `success`, `panelEdge`.
- **`ui/button/cancel`** — Plaque rouge sombre.
  *Prompt propre :* « Button of dark blood-red enamel with a gold frame and pointed ends; uniform middle. »
  *États :* `normal` normal (« resting state ») ; `hover` survol (« gold rim brightened, faint warm inner glow ») ; `pressed` enfoncé (« face slightly darker and inset by 2 px, rim unchanged ») ; `disabled` désactivé (« desaturated, gold turned to dull bronze, face flattened »).
  *Repli :* `danger`, `panelEdge`.
- **`ui/button/default`** — Plaque bleu nuit, pour les actions neutres de réinitialisation.
  *Prompt propre :* « Button of midnight-blue enamel with a gold frame and rounded ends; uniform middle. »
  *États :* `normal` normal (« resting state ») ; `hover` survol (« gold rim brightened, faint warm inner glow ») ; `pressed` enfoncé (« face slightly darker and inset by 2 px, rim unchanged ») ; `disabled` désactivé (« desaturated, gold turned to dull bronze, face flattened »).
  *Repli :* `info`, `panelEdge`.
- **`ui/button/back`** — Bouton grenat en flèche vers la gauche, douille pour l'icône de retour.
  *Prompt propre :* « Button of garnet enamel whose left end is an ornate gold arrow point and whose right end is a gold clasp; a small empty gold socket near the left for an icon; uniform middle. »
  *États :* `normal` normal (« resting state ») ; `hover` survol (« gold rim brightened, faint warm inner glow ») ; `pressed` enfoncé (« face slightly darker and inset by 2 px, rim unchanged ») ; `disabled` désactivé (« desaturated, gold turned to dull bronze, face flattened »).
  *Repli :* `gem`, `panelEdge`.
- **`ui/button/round`** — Bouton circulaire sombre cerclé d'or : zoom de la carte, raccourcis du HUD, ajout d'un sort.
  *Prompt propre :* « Circular button: near-black domed face with a gold ring rim, empty centre for an icon. »
  *États :* `normal` normal (« resting state ») ; `hover` survol (« gold rim brightened, faint warm inner glow ») ; `pressed` enfoncé (« face slightly darker and inset by 2 px, rim unchanged ») ; `disabled` désactivé (« desaturated, gold turned to dull bronze, face flattened »).
  *Repli :* `panel`, `panelEdge`.
- **`ui/button/chip`** — Pastille allongée sombre à filet d'or, bouts arrondis : un sort préparé dans sa rangée d'école.
  *Prompt propre :* « Elongated pill with rounded ends, dark translucent face and a thin gold rim; uniform middle. »
  *États :* `normal` normal (« resting state ») ; `selected` sélectionné (« brighter rim, lit face ») ; `disabled` désactivé (« faded, rim dull bronze »).
  *Repli :* `panelRaised`, `panelEdge`.

### Contrôles {#lot-87-cahier-control}

10 pièces, 17 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/control/checkbox` | Case à cocher | 48 × 48 | fixe, affichée 28 × 28 | `checked`, `unchecked`, `disabled` | 05 (478, 421, 505, 449)<br>05 (478, 490, 505, 517)<br>05 (1013, 587, 1040, 615) | T3.2 |
| `ui/control/slider-track` | Rail de curseur | 256 × 24 | 9-patch 24 / 0 / 24 / 0 | — | 05 (622, 578, 840, 600) | T3.2 |
| `ui/control/slider-handle` | Poignée de curseur | 40 × 40 | fixe, affichée 24 × 24 | `normal`, `hover` | 05 (815, 576, 840, 601) | T3.2 |
| `ui/control/combo` | Liste déroulante | 320 × 48 | 9-patch 24 / 0 / 56 / 0 | `normal`, `hover`, `open`, `disabled` | 05 (703, 211, 909, 249)<br>05 (1182, 480, 1378, 516) | T3.2 |
| `ui/control/combo-popup` | Liste ouverte | 320 × 256 | 9-patch 24 / 16 / 24 / 24 | — | prolonge `ui/control/combo` | T3.2 |
| `ui/control/divider-gold` | Séparateur d'or | 512 × 16 | 9-patch 48 / 0 / 48 / 0 | — | 05 (463, 203, 918, 210)<br>07 (412, 278, 797, 288) | T3.2, T3.3, T3.8 |
| `ui/control/focus-fleuron` | Fleuron de focus | 48 × 48 | fixe, affichée 24 × 24, 40 × 40 | — | 05 (462, 157, 504, 201)<br>01 (1408, 330, 1438, 358) | T3.1, T3.2, T3.9 |
| `ui/control/pip` | Pastille de maîtrise | 32 × 32 | fixe | `empty`, `filled` | 03 (1113, 125, 1141, 153)<br>01 (1410, 410, 1435, 436) | T3.4, T4.1 |
| `ui/control/ruled-line` | Ligne de champ | 512 × 32 | 9-patch 16 / 0 / 48 / 0 | — | 03 (716, 318, 1040, 342) | T3.4 |
| `ui/control/value-box` | Case de valeur | 64 × 40 | 9-patch 8 / 8 / 8 / 8 | — | 03 (1432, 128, 1478, 160) | T3.4 |

- **`ui/control/checkbox`** — Case carrée à cadre d'or ; coche d'or.
  *Prompt propre :* « Square checkbox with a gold frame and dark inset. »
  *États :* `checked` cochée (« a bold hand-drawn gold check mark inside ») ; `unchecked` décochée (« empty inset ») ; `disabled` désactivée (« empty inset, frame dull grey-bronze »).
  *Repli :* `panel`, `panelEdge`, `goldLight`.
- **`ui/control/slider-track`** — Rail sombre creusé à bouts d'or ; le remplissage est un aplat `panelEdge` dessiné par-dessus.
  *Prompt propre :* « Thin horizontal groove, dark inset with a gold edge, small gold end caps; uniform middle. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/control/slider-handle`** — Poignée ronde d'or à cabochon.
  *Prompt propre :* « Round gold slider knob with a small domed centre. »
  *États :* `normal` normal (« resting ») ; `hover` survol (« brighter, faint glow »).
  *Repli :* `panelEdge`, `goldLight`.
- **`ui/control/combo`** — Champ sombre à cadre d'or, flèche d'or à droite dans la marge fixe.
  *Prompt propre :* « Dropdown field: dark inset with a thin gold frame and a small gold downward triangle at the right end; uniform middle. »
  *États :* `normal` normal (« resting ») ; `hover` survol (« brighter frame ») ; `open` ouverte (« frame lit, triangle pointing up ») ; `disabled` désactivée (« dull frame, grey triangle »).
  *Repli :* `panel`, `panelEdge`.
- **`ui/control/combo-popup`** — Le panneau des choix d'une liste ouverte. Aucune maquette ne le montre : il prolonge le champ fermé.
  *Prompt propre :* « Dropdown list panel: dark inset with a thin gold frame, square top edge meant to attach under a field. »
  *Repli :* `panelRaised`, `panelEdge`.
- **`ui/control/divider-gold`** — Filet d'or fin qui s'amincit aux bouts.
  *Prompt propre :* « Thin horizontal gold rule that tapers to fine points at both ends; uniform middle. »
  *Repli :* `panelEdge`.
- **`ui/control/focus-fleuron`** — Losange d'or facetté : la marque du focus (`EX-IHM-071`), posée devant l'élément courant.
  *Prompt propre :* « Faceted gold diamond fleuron with a darker inner lozenge, like a jewel setting. »
  *Repli :* `goldLight`, `panelEdge`.
- **`ui/control/pip`** — Cercle creux cerclé de laiton ; plein quand la compétence est maîtrisée ou la quête active.
  *Prompt propre :* « Small round brass-rimmed pip. »
  *États :* `empty` vide (« hollow parchment centre ») ; `filled` pleine (« filled with polished garnet »).
  *Repli :* `border`, `gem`.
- **`ui/control/ruled-line`** — Ligne d'encre sépia sous une valeur, terminée par un petit fleuron à droite.
  *Prompt propre :* « Thin sepia ink rule along the bottom, ending at the right with a tiny ink fleuron; uniform middle. »
  *Repli :* `text`.
- **`ui/control/value-box`** — Petite case de parchemin clair, bord brun, pour un bonus.
  *Prompt propre :* « Small light parchment box with a thin brown border and a faint inner shadow. »
  *Repli :* `surfaceAlt`, `border`.

### Médaillons {#lot-87-cahier-medallion}

9 pièces, 11 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/medallion/ability` | Anneau de caractéristique | 160 × 160 | fixe | — | 03 (118, 118, 272, 248)<br>03 (365, 118, 515, 248)<br>03 (385, 440, 540, 578) | T3.4 |
| `ui/medallion/derived-stat` | Anneau de statistique dérivée | 120 × 120 | fixe | — | 04 (55, 735, 172, 838)<br>04 (295, 735, 410, 838) | T3.5 |
| `ui/medallion/icon-ring` | Douille d'icône | 96 × 96 | fixe, affichée 48 × 48, 72 × 72, 96 × 96 | — | 07 (415, 244, 467, 292)<br>09 (1227, 150, 1295, 217)<br>10 (54, 193, 132, 272)<br>10 (1187, 320, 1236, 370) | T3.3, T3.7, T3.8 |
| `ui/medallion/portrait-round` | Cadre de portrait rond | 260 × 260 | fixe, affichée 96 × 96, 120 × 120, 260 × 260 | `filled`, `empty` | 03 (158, 200, 468, 505)<br>09 (65, 463, 180, 582)<br>09 (205, 645, 315, 752)<br>06 (1356, 28, 1446, 110) | T3.1, T3.4, T3.7 |
| `ui/medallion/portrait-square` | Cadre de portrait carré | 220 × 220 | fixe, affichée 96 × 96, 120 × 120 | `normal`, `active` | 01 (28, 222, 142, 322)<br>01 (1110, 712, 1197, 812) | T3.9, T4.1 |
| `ui/medallion/portrait-hud` | Portrait du personnage actif | 220 × 220 | fixe, affichée 112 × 112, 180 × 180 | — | 01 (18, 12, 205, 212)<br>08 (1118, 18, 1228, 122) | T3.6, T4.1 |
| `ui/medallion/level-pip` | Pastille de niveau | 48 × 48 | fixe, affichée 32 × 32, 48 × 48 | — | 01 (25, 158, 82, 207)<br>09 (68, 545, 102, 578) | T3.7, T4.1 |
| `ui/medallion/minimap-ring` | Anneau de mini-carte | 260 × 260 | fixe, affichée 196 × 196, 260 × 260 | — | 01 (1398, 22, 1658, 268)<br>08 (1268, 132, 1462, 322) | T3.6, T4.1 |
| `ui/medallion/action-wheel` | Roue d'action | 160 × 160 | fixe | — | 01 (425, 758, 562, 892) | T4.1 |

- **`ui/medallion/ability`** — Anneau d'or à pointes ouvragées, centre de parchemin où s'écrivent l'abréviation et la valeur.
  *Prompt propre :* « Round gold medallion ring with ornate spiked filigree around it and small blue gems at the base; empty light parchment disc in the centre. »
  *Repli :* `accent`, `surfaceAlt`.
- **`ui/medallion/derived-stat`** — Anneau plus simple que celui des caractéristiques : CA, initiative, vitesse, perception.
  *Prompt propre :* « Round dark iron ring with a gold inner rim and small studs, empty light parchment disc in the centre. »
  *Repli :* `accent`, `surfaceAlt`.
- **`ui/medallion/icon-ring`** — Disque sombre cerclé d'or qui reçoit une icône. Réduit selon l'écran.
  *Prompt propre :* « Round dark enamel disc with a bevelled gold ring, empty centre for an icon. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/medallion/portrait-round`** — Cadre rond d'or à gemmes, centre transparent : le portrait se pose dessous.
  *Prompt propre :* « Round ornate gold portrait frame with small garnet gems at the cardinal points; the inner disc is fully transparent. »
  *États :* `filled` occupé (« transparent centre ») ; `empty` libre (« centre filled with faded parchment and an engraved empty-bust silhouette »).
  *Repli :* `accent`, `gem`.
- **`ui/medallion/portrait-square`** — Cadre carré sombre à coins d'or, centre transparent : membres du groupe, cible, interlocuteur.
  *Prompt propre :* « Square portrait frame of dark iron with ornate gold corners and a thin gold inner filet; the inner square is fully transparent. »
  *États :* `normal` normal (« resting ») ; `active` actif (« garnet glow along the inner filet »).
  *Repli :* `panel`, `panelEdge`.
- **`ui/medallion/portrait-hud`** — Grand cadre rond d'or et de grenat à pastille de niveau en bas à gauche (posée à part).
  *Prompt propre :* « Large round portrait frame: garnet enamel outer ring with gold filigree spikes, thin gold inner ring; fully transparent centre. »
  *Repli :* `gem`, `panelEdge`.
- **`ui/medallion/level-pip`** — Petit disque sombre cerclé d'or où s'écrit le niveau.
  *Prompt propre :* « Small round dark disc with a gold rim. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/medallion/minimap-ring`** — Anneau d'or à points cardinaux gravés en losange (sans lettre : N, E, S, O sont posés en texte), centre transparent.
  *Prompt propre :* « Round ornate gold ring with four small diamond-shaped cartouches at north, east, south and west (left blank); fully transparent centre. »
  *Repli :* `panelEdge`, `goldLight`.
- **`ui/medallion/action-wheel`** — Grand disque grenat cerclé d'or ouvragé, à gauche de la barre d'actions ; l'icône de l'action courante s'y pose.
  *Prompt propre :* « Large round garnet enamel disc with an ornate gold ring and small spikes; empty centre. »
  *Repli :* `gem`, `gemLight`, `panelEdge`.

### Emplacements {#lot-87-cahier-slot}

4 pièces, 13 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/slot/item` | Case d'objet | 96 × 96 | fixe, affichée 80 × 80, 96 × 96 | `empty`, `hover`, `selected`, `equipped` | 04 (573, 140, 672, 242)<br>04 (573, 668, 672, 762)<br>04 (66, 153, 167, 255) | T3.5, T3.9 |
| `ui/slot/rarity` | Liseré de rareté | 96 × 96 | fixe | `common`, `uncommon`, `rare`, `very-rare`, `legendary` | prolonge `ui/slot/item` | T3.5 |
| `ui/slot/quantity-pip` | Pastille de quantité | 40 × 28 | 9-patch 8 / 0 / 8 / 0 | — | 09 (1318, 843, 1370, 870)<br>04 (825, 212, 862, 238) | T3.5, T3.7 |
| `ui/slot/action` | Case d'action | 80 × 80 | fixe, affichée 64 × 64 | `normal`, `active`, `disabled` | 01 (568, 793, 637, 857)<br>01 (638, 793, 700, 857) | T4.1 |

- **`ui/slot/item`** — Case carrée sombre à cadre d'or biseauté.
  *Prompt propre :* « Square item slot: dark recessed inset with a bevelled antique gold frame. »
  *États :* `empty` vide (« resting state ») ; `hover` survolée (« frame brighter ») ; `selected` sélectionnée (« frame bright gold with a soft inner glow ») ; `equipped` équipée (« frame with a small garnet gem at the top edge »).
  *Repli :* `panel`, `accent`.
- **`ui/slot/rarity`** — Liseré transparent posé sur la case : la rareté d'un objet se lit sans texte. Aucune maquette ne le montre ; la fiche de l'objet nomme « Peu courant ».
  *Prompt propre :* « Thin coloured inner border for a square slot, fully transparent elsewhere, soft glow inward. »
  *États :* `common` commun (« pale grey-silver border ») ; `uncommon` peu courant (« green border ») ; `rare` rare (« blue border ») ; `very-rare` très rare (« violet border ») ; `legendary` légendaire (« orange-gold border »).
  *Repli :* `panelEdge`.
- **`ui/slot/quantity-pip`** — Petite plaque sombre en bas à droite de la case, qui porte un nombre.
  *Prompt propre :* « Tiny dark rounded plaque with a thin gold edge; uniform middle. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/slot/action`** — Case de la barre d'actions, cadre d'or ; son numéro de raccourci est du texte en dessous.
  *Prompt propre :* « Square action slot with a dark inset and a bevelled gold frame. »
  *États :* `normal` normal (« resting ») ; `active` active (« bright gold frame with a warm glow ») ; `disabled` désactivée (« dull bronze frame, darkened »).
  *Repli :* `panel`, `panelEdge`.

### Jauges {#lot-87-cahier-gauge}

2 pièces, 2 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/gauge/track` | Fond de jauge | 512 × 28 | 9-patch 40 / 0 / 40 / 0 | — | 03 (590, 602, 1030, 642)<br>03 (590, 728, 1030, 768)<br>01 (205, 80, 520, 115)<br>04 (638, 808, 1042, 838) | T3.4, T3.5, T3.6, T4.1 |
| `ui/gauge/fill-sheen` | Relief de remplissage | 256 × 20 | 9-patch 4 / 0 / 4 / 0 | — | 03 (618, 612, 915, 634) | T3.4, T3.5, T3.6, T4.1 |

- **`ui/gauge/track`** — Rail creux sombre, embouts d'or en pointe ; le remplissage se pose dedans.
  *Prompt propre :* « Horizontal gauge housing: dark recessed channel with a gold rim and pointed ornate gold end caps; uniform middle. »
  *Repli :* `panel`, `panelEdge`.
- **`ui/gauge/fill-sheen`** — Relief en niveaux de gris seulement (reflet haut, ombre basse), posé sur un aplat coloré par les jetons : la couleur d'un remplissage ne vient jamais d'une image (`EX-IHM-075`).
  *Prompt propre :* « Greyscale-only glossy sheen overlay for a gauge fill: a soft white highlight band along the top, a soft dark band along the bottom, transparent in between; no hue at all. »
  *Repli :* `gemLight`, `goldLight`.

### Ornements {#lot-87-cahier-ornament}

11 pièces, 14 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/ornament/wax-seal` | Sceau de cire | 128 × 128 | fixe, affichée 84 × 84, 120 × 120 | — | 03 (618, 858, 738, 968)<br>04 (48, 858, 132, 940) | T3.4, T3.5 |
| `ui/ornament/crossed-crest` | Écusson croisé | 128 × 128 | fixe, affichée 80 × 80, 112 × 112 | — | 03 (758, 778, 862, 902)<br>04 (412, 848, 502, 938)<br>10 (52, 812, 122, 888) | T3.4, T3.5, T3.8 |
| `ui/ornament/signature-flourish` | Paraphe | 256 × 64 | fixe | — | 03 (845, 905, 1030, 935) | T3.4, T3.5 |
| `ui/ornament/compass-watermark` | Boussole en filigrane | 512 × 512 | fixe, affichée 160 × 160, 240 × 240, 400 × 400 | `parchment`, `dark` | 03 (105, 565, 505, 935)<br>04 (1132, 262, 1292, 412)<br>10 (1478, 712, 1645, 900)<br>05 (135, 625, 375, 865) | T3.2, T3.4, T3.5, T3.8 |
| `ui/ornament/compass-rose` | Rose des vents | 192 × 192 | fixe | — | 08 (226, 698, 372, 852) | T3.6 |
| `ui/ornament/corner` | Coin ornemental | 128 × 128 | fixe, affichée 64 × 64, 96 × 96 | — | 05 (92, 818, 168, 892)<br>07 (1232, 158, 1296, 222) | T3.2, T3.3 |
| `ui/ornament/divider-star` | Étoile de séparateur | 64 × 32 | fixe | — | 07 (812, 783, 852, 818) | T3.3 |
| `ui/ornament/crest-shield` | Écu grenat | 128 × 128 | fixe, affichée 96 × 96, 128 × 128 | — | 05 (104, 68, 182, 172)<br>09 (32, 18, 127, 138)<br>10 (24, 8, 112, 98)<br>08 (20, 20, 110, 125) | T3.2, T3.6, T3.7, T3.8 |
| `ui/ornament/crest-pennant` | Fanion grenat | 128 × 160 | fixe, affichée 96 × 120, 128 × 160 | — | 04 (28, 22, 112, 132)<br>04 (1418, 22, 1505, 132)<br>03 (253, 12, 377, 142) | T3.4, T3.5 |
| `ui/ornament/hourglass` | Sablier | 64 × 64 | fixe, affichée 32 × 32 | — | 10 (1395, 480, 1415, 518) | T3.8 |
| `ui/ornament/day-dial` | Cadran du jour | 64 × 64 | fixe, affichée 32 × 32 | `day`, `dusk`, `night` | 01 (15, 872, 50, 908)<br>08 (38, 972, 78, 1012) | T3.6, T4.1 |

- **`ui/ornament/wax-seal`** — Sceau de cire grenat frappé d'une esperluette dragonne.
  *Prompt propre :* « Red wax seal with irregular edges, stamped with a dragon-shaped ampersand emblem, soft cast shadow within the canvas. »
  *Repli :* `gem`, `gemShadow`.
- **`ui/ornament/crossed-crest`** — Écu de laiton sur deux épées croisées.
  *Prompt propre :* « Small brass heater shield over two crossed swords, engraved compass emblem on the shield. »
  *Repli :* `accent`, `border`.
- **`ui/ornament/signature-flourish`** — Le trait de plume sous une signature, sans le nom : le nom est posé en `loreFamily`, parce qu'il change avec le personnage.
  *Prompt propre :* « A single elegant sepia ink pen flourish, a long underline swash with a loop, no letters. »
  *Repli :* `text`.
- **`ui/ornament/compass-watermark`** — Rose des vents gravée, très pâle, en fond de document ou de colonne.
  *Prompt propre :* « Engraved compass rose with sixteen points and concentric rings, drawn as a very faint low-contrast watermark, transparent background. »
  *États :* `parchment` sur parchemin (« faint sepia ink lines ») ; `dark` sur panneau (« faint dark bronze lines, barely lighter than near-black »).
  *Repli :* `background`, `panelRaised`.
- **`ui/ornament/compass-rose`** — Rose des vents d'or et de bronze sur anneau bleu nuit, lettres cardinales absentes (posées en texte).
  *Prompt propre :* « Ornate compass rose: eight-pointed gold and bronze star over a midnight-blue ring with gold graduations; no letters. »
  *Repli :* `panelEdge`, `info`.
- **`ui/ornament/corner`** — Filigrane d'or d'angle, posé sur les coins d'un panneau quand le 9-patch n'en porte pas assez.
  *Prompt propre :* « Top-left corner ornament of gold filigree scrolls and a small spike; transparent elsewhere. The game mirrors it for the other corners. »
  *Repli :* `panelEdge`, `goldLight`.
- **`ui/ornament/divider-star`** — L'étoile centrale d'un séparateur : un 9-patch n'a pas de milieu fixe, l'étoile se pose donc entre deux séparateurs d'or.
  *Prompt propre :* « Small four-pointed gold star with short tapering gold rays left and right. »
  *Repli :* `panelEdge`, `goldLight`.
- **`ui/ornament/crest-shield`** — Écu d'émail grenat à l'esperluette dragonne d'or : l'emblème posé devant un titre d'écran.
  *Prompt propre :* « Heater shield of garnet enamel with a gold rim and a gold dragon-shaped ampersand emblem. »
  *Repli :* `gem`, `panelEdge`.
- **`ui/ornament/crest-pennant`** — Fanion d'étoffe grenat à pointe, esperluette d'or, suspendu au bord supérieur d'une feuille.
  *Prompt propre :* « Hanging pennant of garnet cloth with a V-cut bottom, gold fringe line and a gold dragon-shaped ampersand emblem. »
  *Repli :* `gem`, `accent`.
- **`ui/ornament/hourglass`** — Sablier de laiton : temps d'incantation, durées.
  *Prompt propre :* « Small brass hourglass with sand, front view. »
  *Repli :* `accent`, `border`.
- **`ui/ornament/day-dial`** — Soleil ou lune d'or : le moment de la journée, à côté de l'heure.
  *Prompt propre :* « Small gold celestial dial icon. »
  *États :* `day` jour (« radiant sun with straight rays ») ; `dusk` crépuscule (« half sun on a horizon line ») ; `night` nuit (« crescent moon with two small stars »).
  *Repli :* `goldLight`, `panelEdge`.

### Icônes {#lot-87-cahier-icon}

13 pièces, 87 images.

| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |
|---|---|---|---|---|---|---|
| `ui/icon/menu` | Entrées du menu | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `continue`, `new-game`, `load-game`, `options`, `credits`, `quit` | 06 (150, 355, 540, 805) | T3.1 |
| `ui/icon/options-tab` | Onglets des options | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `general`, `graphics`, `audio`, `game`, `controls`, `accessibility` | 05 (112, 195, 432, 612) | T3.2 |
| `ui/icon/options-section` | Sections des options | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `language`, `interface`, `save`, `difficulty`, `network`, `privacy` | 05 (460, 150, 1390, 700) | T3.2 |
| `ui/icon/skill` | Compétences | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `acrobatics`, `animal-handling`, `arcana`, `athletics`, `deception`, `history`, `insight`, `intimidation`, `investigation`, `medicine`, `nature`, `perception`, `performance`, `persuasion`, `religion`, `sleight-of-hand`, `stealth`, `survival` | 03 (1110, 120, 1480, 865) | T3.4 |
| `ui/icon/ability` | Caractéristiques | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `strength`, `dexterity`, `constitution`, `intelligence`, `wisdom`, `charisma` | 03 (30, 118, 540, 578) | T3.4 |
| `ui/icon/poi` | Points d'intérêt de la carte | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `city`, `fortress`, `dungeon`, `camp`, `main-quest`, `side-quest`, `point-of-interest`, `port` | 08 (15, 578, 230, 928) | T3.6 |
| `ui/icon/nav` | Navigation | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `map`, `quests`, `inventory`, `company`, `options` | 08 (1030, 918, 1478, 1015)<br>01 (1410, 790, 1660, 860) | T3.6, T4.1 |
| `ui/icon/school` | Écoles de magie | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `abjuration`, `conjuration`, `divination`, `enchantment`, `evocation`, `illusion`, `necromancy`, `transmutation`, `thaumaturgy` | 10 (510, 150, 605, 865) | T3.8 |
| `ui/icon/inventory-category` | Catégories d'inventaire | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `all`, `equipment`, `consumables`, `misc` | 04 (560, 68, 1068, 112) | T3.5 |
| `ui/icon/credits-section` | Sections des crédits | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `development`, `art-direction`, `story`, `audio`, `other` | 07 (400, 230, 1260, 640) | T3.3 |
| `ui/icon/spell-property` | Propriétés d'un sort | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `damage-type`, `range`, `damage`, `casting-time`, `components`, `special-effects` | 10 (1180, 320, 1620, 630) | T3.8 |
| `ui/icon/resource` | Ressources | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `health`, `experience`, `weight`, `gold` | 03 (575, 540, 1035, 770)<br>04 (565, 770, 1060, 940) | T3.4, T3.5, T3.7 |
| `ui/icon/company-stat` | Compteurs de l'équipe | 128 × 128 | fixe, affichée 32 × 32, 48 × 48, 64 × 64 | `career-points`, `team-level`, `prestige`, `fame` | 09 (55, 275, 470, 395) | T3.7 |

- **`ui/icon/menu`** — Icônes d'or des six entrées du menu principal.
  *Prompt propre :* « Engraved gold emblem icon, bold readable silhouette at 32 px, subtle relief, transparent background. »
  *Membres :* `continue` Continuer (« a play triangle » — 06 (183, 365, 245, 415)) ; `new-game` Nouvelle partie (« a quill over a scroll » — 06 (190, 443, 245, 490)) ; `load-game` Charger une partie (« an open folder » — 06 (190, 520, 245, 566)) ; `options` Options (« a cogwheel » — 06 (190, 596, 245, 642)) ; `credits` Crédits (« an open book » — 06 (190, 672, 245, 718)) ; `quit` Quitter (« a door with an exit arrow » — 06 (190, 750, 245, 796)).
  *Repli :* `textOnPanel`.
- **`ui/icon/options-tab`** — Les six onglets de la maquette 05 — et non huit, comme le plan l'annonçait.
  *Prompt propre :* « Engraved gold emblem icon, bold readable silhouette at 32 px, subtle relief, transparent background. »
  *Membres :* `general` Général (« a cogwheel » — 05 (140, 203, 192, 248)) ; `graphics` Graphismes (« a monitor » — 05 (140, 275, 195, 320)) ; `audio` Audio (« a speaker with sound waves » — 05 (140, 345, 195, 392)) ; `game` Jeu (« a gamepad » — 05 (140, 418, 195, 462)) ; `controls` Commandes (« a keyboard » — 05 (140, 488, 195, 532)) ; `accessibility` Accessibilité (« a standing human figure » — 05 (145, 558, 192, 605)).
  *Repli :* `textOnPanel`.
- **`ui/icon/options-section`** — L'emblème logé dans le losange de chaque intertitre des options.
  *Prompt propre :* « Engraved gold emblem icon, bold readable silhouette at 32 px, subtle relief, transparent background. »
  *Membres :* `language` Langue (« a speech bubble » — 05 (463, 157, 504, 201)) ; `interface` Interface (« a pointing gauntlet » — 05 (463, 368, 504, 412)) ; `save` Sauvegarde (« a bookmark ribbon » — 05 (463, 635, 504, 680)) ; `difficulty` Difficulté (« a raised fist » — 05 (943, 155, 985, 200)) ; `network` Réseau (« two crossed banners » — 05 (943, 422, 985, 466)) ; `privacy` Confidentialité (« a padlock » — 05 (943, 653, 985, 698)).
  *Repli :* `goldLight`.
- **`ui/icon/skill`** — Les dix-huit compétences, glyphe clair dans un disque noir, comme sur la maquette 03.
  *Prompt propre :* « Light parchment-coloured glyph inside a solid black disc with a thin brass rim, bold readable silhouette at 32 px, transparent background outside the disc. »
  *Membres :* `acrobatics` Acrobaties (« a tumbling figure » — 03 (1148, 119, 1192, 159)) ; `animal-handling` Dressage (« a paw print » — 03 (1148, 160, 1192, 200)) ; `arcana` Arcanes (« a radiant eye in a sunburst » — 03 (1148, 202, 1192, 242)) ; `athletics` Athlétisme (« a dumbbell » — 03 (1148, 243, 1192, 283)) ; `deception` Tromperie (« a theatre mask » — 03 (1148, 285, 1192, 325)) ; `history` Histoire (« an open tome » — 03 (1148, 326, 1192, 366)) ; `insight` Intuition (« a watchful eye » — 03 (1148, 367, 1192, 407)) ; `intimidation` Intimidation (« a roaring face » — 03 (1148, 409, 1192, 449)) ; `investigation` Investigation (« a magnifying glass » — 03 (1148, 450, 1192, 490)) ; `medicine` Médecine (« a cross » — 03 (1148, 492, 1192, 532)) ; `nature` Nature (« a leaf » — 03 (1148, 533, 1192, 573)) ; `perception` Perception (« an eye with lashes » — 03 (1148, 574, 1192, 614)) ; `performance` Représentation (« a lyre » — 03 (1148, 616, 1192, 656)) ; `persuasion` Persuasion (« two speech bubbles » — 03 (1148, 657, 1192, 697)) ; `religion` Religion (« a radiant sun symbol » — 03 (1148, 699, 1192, 739)) ; `sleight-of-hand` Escamotage (« an open hand » — 03 (1148, 740, 1192, 780)) ; `stealth` Discrétion (« a hooded head » — 03 (1148, 781, 1192, 821)) ; `survival` Survie (« a campfire » — 03 (1148, 823, 1192, 863)).
  *Repli :* `text`.
- **`ui/icon/ability`** — Les six caractéristiques. La maquette 03 les écrit en abréviations dans les médaillons, sans icône : les icônes servent aux listes où la place manque (compétences, jets).
  *Prompt propre :* « Sepia ink engraved emblem, bold silhouette at 32 px, transparent background. »
  *Membres :* `strength` Force (« a flexed arm ») ; `dexterity` Dextérité (« a feather ») ; `constitution` Constitution (« a heart inside a shield ») ; `intelligence` Intelligence (« an open book with a star ») ; `wisdom` Sagesse (« an owl ») ; `charisma` Charisme (« a crown »).
  *Repli :* `text`.
- **`ui/icon/poi`** — Les marqueurs de la légende de la carte, plus le port, visible sur la carte mais absent de la légende.
  *Prompt propre :* « Black silhouette map marker on a small gold-rimmed tag, readable at 24 px, transparent background. »
  *Membres :* `city` Ville / Cité (« a city with towers » — 08 (40, 624, 72, 648)) ; `fortress` Forteresse (« a fortress keep » — 08 (40, 648, 72, 672)) ; `dungeon` Donjon / Ruines (« a ruined tower » — 08 (40, 673, 72, 697)) ; `camp` Camp (« a tent » — 08 (40, 697, 72, 721)) ; `main-quest` Quête principale (« an exclamation mark in a gold diamond » — 08 (40, 721, 72, 745)) ; `side-quest` Quête secondaire (« an exclamation mark in a blue diamond » — 08 (40, 746, 72, 770)) ; `point-of-interest` Point d'intérêt (« a small white pearl » — 08 (40, 770, 72, 794)) ; `port` Port (« an anchor » — 08 (425, 155, 458, 186)).
  *Repli :* `text`.
- **`ui/icon/nav`** — La barre de navigation de la carte, et les raccourcis du HUD.
  *Prompt propre :* « Engraved gold emblem icon, bold readable silhouette at 32 px, subtle relief, transparent background. »
  *Membres :* `map` Carte (« a folded map » — 08 (1058, 938, 1112, 988)) ; `quests` Quêtes (« a quill and scroll » — 08 (1143, 938, 1195, 988)) ; `inventory` Inventaire (« a backpack » — 08 (1225, 938, 1280, 988)) ; `company` Équipe (« three heads » — 08 (1310, 938, 1365, 988)) ; `options` Options (« a cogwheel » — 08 (1395, 938, 1448, 988)).
  *Repli :* `textOnPanel`.
- **`ui/icon/school`** — Neuf médaillons émaillés de la couleur de leur école : les huit écoles du jeu de règles, et la thaumaturgie de la maquette 10.
  *Prompt propre :* « Round enamel medallion with a bevelled rim in the school's colour and a glowing emblem in the centre, transparent background. »
  *Membres :* `abjuration` Abjuration (« a blue shield » — 10 (518, 150, 600, 230)) ; `conjuration` Invocation (« a red flame pyramid » — 10 (518, 230, 600, 310)) ; `divination` Divination (« a violet eye » — 10 (518, 308, 600, 388)) ; `enchantment` Enchantement (« a golden mask shield » — 10 (518, 388, 600, 468)) ; `evocation` Évocation (« an orange flame » — 10 (518, 468, 600, 548)) ; `illusion` Illusion (« a teal swirl of mirrors » — 10 (518, 548, 600, 628)) ; `necromancy` Nécromancie (« a violet skull » — 10 (518, 625, 600, 705)) ; `transmutation` Transmutation (« a green alchemical mask » — 10 (518, 705, 600, 785)) ; `thaumaturgy` Thaumaturgie (« a white radiant star » — 10 (518, 785, 600, 865)).
  *Repli :* `panelEdge`.
- **`ui/icon/inventory-category`** — Les quatre filtres de l'inventaire. La maquette 04 les écrit en toutes lettres : les icônes servent à la largeur 1280 × 720, où les libellés ne tiennent plus.
  *Prompt propre :* « Engraved gold emblem icon, bold readable silhouette at 32 px, subtle relief, transparent background. »
  *Membres :* `all` Tous (« four small squares ») ; `equipment` Équipement (« a breastplate ») ; `consumables` Consommables (« a potion flask ») ; `misc` Divers (« a pouch »).
  *Repli :* `textOnPanel`.
- **`ui/icon/credits-section`** — Les cinq sections des crédits.
  *Prompt propre :* « Engraved gold emblem icon, bold readable silhouette at 32 px, subtle relief, transparent background. »
  *Membres :* `development` Développement (« a group of people » — 07 (415, 244, 467, 292)) ; `art-direction` Direction artistique (« a painter's palette » — 07 (415, 475, 467, 527)) ; `story` Scénario & Univers (« a quill » — 07 (840, 242, 892, 294)) ; `audio` Audio (« a cogwheel with a note » — 07 (840, 398, 892, 450)) ; `other` Autres contributions (« a star » — 07 (840, 580, 892, 632)).
  *Repli :* `goldLight`.
- **`ui/icon/spell-property`** — Les rubriques du détail d'un sort.
  *Prompt propre :* « Engraved gold emblem icon, bold readable silhouette at 32 px, subtle relief, transparent background. »
  *Membres :* `damage-type` Type de dégâts (« a flame » — 10 (1188, 321, 1236, 369)) ; `range` Portée (« a bow and arrow » — 10 (1188, 371, 1236, 419)) ; `damage` Dégâts (« a starburst impact » — 10 (1188, 423, 1236, 471)) ; `casting-time` Temps d'incantation (« an hourglass » — 10 (1188, 474, 1236, 522)) ; `components` Composants (« an open grimoire » — 10 (1188, 525, 1236, 573)) ; `special-effects` Effets spéciaux (« a spiral » — 10 (1188, 580, 1236, 628)).
  *Repli :* `goldLight`.
- **`ui/icon/resource`** — Ce que mesurent les jauges et les compteurs.
  *Prompt propre :* « Painted emblem icon with gold and garnet accents, bold silhouette at 32 px, transparent background. »
  *Membres :* `health` Points de vie (« a garnet heart in a gold frame » — 03 (580, 545, 640, 600)) ; `experience` Expérience (« a laurel wreath around a small star » — 03 (580, 670, 640, 725)) ; `weight` Poids (« a tied leather sack » — 04 (568, 775, 632, 838)) ; `gold` Pièces d'or (« a stack of gold coins » — 04 (575, 872, 640, 937)).
  *Repli :* `gem`, `goldLight`.
- **`ui/icon/company-stat`** — Les quatre compteurs de la compagnie.
  *Prompt propre :* « Engraved gold emblem icon, bold readable silhouette at 32 px, subtle relief, transparent background. »
  *Membres :* `career-points` Points de carrière (« an eight-pointed star » — 09 (64, 284, 113, 338)) ; `team-level` Niveau d'équipe (« a shield » — 09 (190, 284, 238, 338)) ; `prestige` Prestige (« a crown » — 09 (300, 284, 348, 338)) ; `fame` Renommée (« a laurel wreath » — 09 (393, 284, 440, 338)).
  *Repli :* `goldLight`.

### Ce que le cahier ne fait pas produire {#lot-87-cahier-exclusions}

| Élément | Maquettes | Raison |
|---|---|---|
| Carte du monde | 08 | plus d'écran de carte du monde (LOT-94) ; celle du LOT-42 se génère dans le style du jeu, jamais extraite du corpus |
| Portraits | 03, 01, 09 | les 172 jetons ronds du corpus (LOT-38) servent de portraits de démonstration |
| Icônes d'objets, d'armes et d'armures | 04 | assets d'entité du LOT-39 (familles item, weapon, armor), servis par un marqueur tant que l'illustration n'est pas livrée : ils ne sont pas de l'interface |
| Scène et grille tactique du HUD | 01, 02 | le viewport est rendu en pixel art par le moteur ; la maquette fixe le cadre, pas la scène (T4.1) |
| Illustrations des bâtiments et de la base | 09 | des données de la compagnie (LOT-45, LOT-83), servies par PendingData jusque-là ; elles entreront au cahier avec leur lot |
| Planche 02 (pack d'assets HUD) | 02 | planche de présentation d'un pack SVG/QML/CSS écarté (T5.1) ; ses pièces sont reprises depuis les maquettes d'écran, qui les montrent en situation |

<!-- FIN DES TABLES ENGENDREES -->
