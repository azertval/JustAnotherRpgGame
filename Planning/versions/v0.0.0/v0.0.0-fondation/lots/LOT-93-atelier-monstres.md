+++
id = "LOT-93"
titre = "Atelier des monstres : les figurines du bestiaire, depuis le texte seul"
version = "0.0.0"
filiere = "assets"
statut = "livre"
taille = "M"
resume = "Une créature sans référence visuelle reçoit sa figurine animée depuis son seul texte, au style des PNJ : le lion, le loup et le soldat Ironhand ouvrent la série."
prerequis = ["LOT-91", "LOT-33", "LOT-92"]
livrables = [
  "`Source/Elements/Assets/Monsters/<slug>/` pour `lion`, `wolf` et `ironhand-soldier` : bandes, `.anim.json`, `portrait.png` ; `Monsters/manifest.json` avec ses deux gabarits.",
  "L'atelier du lot : `atelier/prompts/` (dispositions Moyen et Grand, corps, marche, `cast.txt`, `sans_cast.txt`, `portrait_creature.txt`), `atelier/ancres/planche_grand.png`, `atelier/monstres/index.json` (173 blocs), `atelier/monstres/<slug>/`, `atelier/scripts/` (`prompts.py`, `chatgpt.py`, `integre.py`).",
  "Les scripts communs du LOT-91 étendus : `normalise.py --gabarit grand`, `--vides`, `--garde-finale`, `scinder`, `par_la_grille` ; `compose.py --travail` ; `palette.py` avec dossier des fiches.",
  "`scripts/check_asset_keys.py` valide les figurines, avec ses tests `scripts/tests/test_check_asset_keys.py`.",
  "La galerie des assets lit `Monsters/` (`readFigures`, `AssetGalleryTest.FigurinesDeMonstres`).",
  "Les dix sentinelles de Martpart et d'Arenarea portent la figurine `Monsters/ironhand-soldier` (`hmi::figureStripPath`).",
]
criteres = [
  "Le lion, le loup et le soldat Ironhand ont, dans `Source/Elements/Assets/Monsters/<slug>/`, leurs bandes aux dimensions de leur gabarit, leurs `.anim.json` et `portrait.png`, et une entrée au manifeste — sans `cast`.",
  "Ils se jouent dans la galerie des assets, animation par animation, et passent la revue.",
  "`check_asset_keys.py` rend 0.",
  "Leurs références sont dans `atelier/monstres/<slug>/` (`fiche.json`, `prompt_b.txt`, `portrait.png`, `palette.txt`) ; la planche du lion est versée dans `atelier/ancres/planche_grand.png`.",
  "Une disposition modifiée garde sa version précédente et dit ce qui a changé.",
]
sources = [
  "Tanares Sourcebook, p. 164 (bloc du soldat Ironhand)",
  "Tanares Sourcebook, p. 79-175 (relevé provisoire des 79 blocs de Tanares)",
]
+++

## Pourquoi

L'atelier des PNJ part d'une **image** : le portrait peint du *Character Compendium*, que la chaîne
normalise avant de l'imiter. Le bestiaire n'en a pas. Ce lot dérive l'atelier pour le cas **sans
référence visuelle** : la créature est décrite par le **texte** seul — son bloc, sa taille, ce que le
Sourcebook en dit —, et le style est celui des PNJ, pour que la bête et la figurine qui l'affronte
soient du même monde.

Les premières fiches sont celles dont la version `0.0.1` a besoin, et elles sont **trois**, décidées
par l'auteur le 18 septembre 2026 :

| Créature | Taille | Gabarit | Pourquoi |
|---|---|---|---|
| **Lion** (`lion`) | Grande | 96 × 96 | le fauve de `colisee-fauves.json` |
| **Loup** (`wolf`) | Moyenne | 48 × 64 | les deux loups de la même rencontre |
| **Soldat Ironhand** (`ironhand-soldier`) | Moyenne | 48 × 64 | l'armée qui tient la Capitale ; un humanoïde sans portrait prouve que la chaîne ne se limite pas aux bêtes |

Le contrôle visuel se fait **dans la galerie des assets** (`--screen=AssetGallery`), et seulement
là : c'est l'outil fait pour cela. Les faire jouer au Colisée ou en ville est le travail du
[LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27).

## Les décisions

| Sujet | Décision |
|---|---|
| **Générateur** | Celui du LOT-91 : génération d'images d'OpenAI, envois **à la main** dans l'interface ChatGPT. Claude ne dessine pas : il rédige, assemble, mesure et relit. |
| **Style** | `style.txt` du LOT-91, **gelé**, dont seul le paragraphe « VIEW AND SCALE » est remplacé : une bête ne se voit pas de face et n'a pas la taille d'un héros (`prompts.py`, `VUES`). |
| **Vue** | Humanoïde : de face, légèrement plongeante, comme les PNJ. Bête : de **trois quarts**, légèrement plongeante, **tournée vers la droite**, là où elle frappe — la tête à droite, la queue à gauche, sur toutes les images. |
| **Référence** | Aucune image du corpus, ni rendue ni décalquée. Le **portrait pixel art** de la créature, produit d'abord depuis le texte (étape P), tient la place du portrait peint : il fixe la tête et la palette avant la planche. |
| **Jeu d'animations** | Celui des PNJ : `idle` 6, `walk` 8, `hit` 4, `death` 6, `attack` 8, `cast` 8. Une créature **sans sort n'a pas de `cast`** : ses rangées 7 et 8 sont demandées vides (`sans_cast.txt`), lues vides (`normalise.py --vides 7,8`), et le manifeste ne les déclare pas. |
| **Taille** | La rubrique **SIZE** du bloc B donne la hauteur à l'écran (pas de 2 px) mesurée contre les héros de l'échelle (90 px) ; `fiche.json` porte la même hauteur en pixels d'art (`hauteur`), que `normalise.py --hauteur` impose. |
| **Silhouette** | Une rubrique **SILHOUETTE** dans le bloc B : la lisibilité d'une bête en quelques dizaines de pixels se joue à la forme, pas au détail. |
| **Emplacement** | `Source/Elements/Assets/Monsters/<slug>/` : les bandes, leurs `.anim.json` et `portrait.png` ; `Monsters/manifest.json` porte les gabarits et une entrée par créature. Le slug est l'identifiant du catalogue (`lion`, `wolf`) ; pour un bloc pas encore extrait, celui que le `LOT-46` lui donnera (`ironhand-soldier`), et l'entrée dit le lot attendu (`"awaiting": "LOT-46"`). |
| **Tailles hors gabarit** | *Huge* (3 × 3 cases) et *Gargantuan* attendent un troisième gabarit, écrit à la première qui le demande ; *Tiny* et *Small* prennent le gabarit Moyen. |

### Les deux gabarits

La **planche** a la même grille aux deux gabarits — 1 536 × 2 048, huit rangées de 256 px, les
rangées 5 à 8 en cellules de 384 px —, si bien que `compose.py` n'en dépend pas ; seules les
**bandes** changent, et `normalise.py --gabarit` les découpe.

| Bande | Moyen (1 case) | Grand (2 × 2 cases) |
|---|---|---|
| `idle`, `walk`, `hit` | 48 × 64, pied au centre | 96 × 96, pied au centre |
| `death` | **96** × 64 (couché, deux fois la hauteur) | 96 × 96 (un fauve tombe sur le flanc, pas plus long que debout) |
| `attack`, `cast` | 96 × 64, pied à x = 32 | 192 × 96, pied à x = 64 |
| cellules de planche, rangées 1-4 | 192 (rangée 4 : 256) | 192 |

Tant qu'aucune planche Grande n'est retenue, la référence 1 d'une planche Grande est celle
d'Anariel, pour le **style seul** (la disposition le dit) ; la première retenue — le lion — est
versée dans `atelier/ancres/planche_grand.png` et devient l'ancre des suivantes.

## L'atelier

| Chemin | Contenu |
|---|---|
| `atelier/prompts/` | `disposition_moyen.txt`, `disposition_grand.txt`, `disposition_reprise.txt` (passes par animation), `corps_humanoide.txt` et `corps_bete.txt` (règles d'arme ou de corps), `marche_quadrupede.txt`, `cast.txt` et `sans_cast.txt`, `portrait_creature.txt`. Chaque fichier porte sa version en première ligne. |
| `atelier/ancres/` | `planche_grand.png`, l'ancre des planches Grandes, versée au premier Grand retenu. Les autres ancres sont celles du LOT-91. |
| `atelier/monstres/index.json` | Les **173 blocs relevés** : slug, nom, taille, gabarit, lot d'origine (ci-dessous). |
| `atelier/monstres/<slug>/` | `fiche.json` (créature, gabarit, corps, hauteur, sort), `prompt_b.txt` (le bloc B), puis `portrait.png` et `palette.txt` écrits à l'étape P, et au besoin `marche.txt` (une marche propre). |
| `atelier/scripts/` | `prompts.py` (chemins, fiches, assemblage des prompts), `chatgpt.py` (préparer les envois), `integre.py` (verser dans le jeu). |

Les scripts **communs** restent dans l'atelier des PNJ (`LOT-91-atelier-pnj/atelier/scripts/`) et
ont gagné ce qu'il fallait : `normalise.py --gabarit grand`, `compose.py --travail <dossier>`,
`palette.py <slug> <portrait> <dossier des fiches>`. `normalise_portrait.py` sert tel quel.

Le **dossier de travail**, hors dépôt (variable `MONSTRES_ATELIER`, par défaut
`D:\JustAnotherDnDGame-monstres`), reçoit les envois préparés (`chatgpt\`), les images générées et
leurs normalisations (`<slug>\token\`, `<slug>\tourK\`, `<slug>\final\`).

Les scripts se lancent depuis `atelier/scripts/`, avec `py -3.13` (`numpy`, `Pillow`).

## La méthode, pour une créature

Les créatures d'une série passent **ensemble, étape par étape**, comme les PNJ.

| Étape | Ce qu'on fait | Contrôle pour passer |
|---|---|---|
| **B — Fiche** | Lire le bloc (catalogue, ou page du Sourcebook) et écrire `monstres/<slug>/fiche.json` et `prompt_b.txt` sur le modèle des trois existants : CREATURE, LOOK, SILHOUETTE, SIZE, WEAPON, PALETTE (remplie en P), CAST, ANIMATION SET. | Gabarit Moyen ou Grand ; sinon la créature attend son gabarit. |
| **P — Portrait** | `chatgpt.py portrait <K> <slugs>` ; générer ; lancer les deux lignes de `NORMALISER.txt` (`normalise_portrait.py`, puis `palette.py`, qui copie le portrait dans `monstres/<slug>/` et remplit la ligne PALETTE). | La créature que décrit le texte, lisible ; un fichier d'image valide. |
| **S — Planche** | `chatgpt.py planche <K> <slugs>` ; générer ; lancer `NORMALISER.txt` (hauteur, gabarit et rangées vides y sont déjà). | Code 0, **et** la revue ci-dessous. |
| **M — Marche** *(si besoin)* | Si « walk bouge peu » ou si l'œil la juge figée : `chatgpt.py marche <K> <slugs>` ; `NORMALISER.txt`. | Code 0 ; huit silhouettes distinctes. |
| **X — Passe par animation** *(si besoin)* | Une seule animation fausse : `chatgpt.py <anim> <K> <slugs>` ; `NORMALISER.txt`. | Code 0 ; revue de cette animation. |
| **A — Assemblage** | `py -3.13 ..\..\..\LOT-91-atelier-pnj\atelier\scripts\compose.py <slug> <tour S> [<anim>=<tour> …] --travail <dossier de travail>`. | Code 0. |
| **I — Intégration** | `integre.py <slug>` : bandes, `.anim.json` et `portrait.png` dans `Assets/Monsters/<slug>/`, entrée au manifeste, **case cochée ci-dessous** ; puis `scripts/check_asset_keys.py`, et la créature se regarde dans la galerie. | `check_asset_keys.py` rend 0 ; la revue, animation par animation, dans la galerie. |

Les règles du LOT-91 valent ici : on corrige le prompt, jamais l'image ; une animation fausse se
refait seule ; au plus trois tours par étape.

### La revue (huit « oui »)

1. La créature que décrit le bloc B : LOOK et SILHOUETTE reconnaissables.
2. La tête et les couleurs du portrait retenu.
3. La même créature sur toutes les images (proportions, marques, équipement, **une seule arme**).
4. Le style de la planche d'Anariel : contour, ombrage, densité, pas d'anticrénelage.
5. Une bête tournée vers la droite sur toutes les images ; un humanoïde, l'arme dans la même main.
6. `idle` boucle ; `attack` et `hit` finissent près de la garde ; `death` 5-6 couchés sur la ligne.
7. Rien dans les rangées 7-8 d'une créature sans sort ; les effets restent dans leur cellule.
8. Aucun texte, cadre, ombre au sol, décor.

## La production du 18 septembre 2026

**Un seul tour par étape, par décision de l'auteur** : les portraits et les planches du tour 1 sont
retenus tels quels, sans passe M ni X. Ce que la planche avait de faux se règle **à la lecture**,
jamais en retouchant une image ; `normalise.py` y a gagné trois recours, chacun nommé dans le
rapport de la planche :

| Créature | Ce que la planche avait | Ce que la lecture en fait |
|---|---|---|
| Lion | Le générateur l'a dessiné bien plus grand que demandé : des figures de marche et les deux corps couchés de la mort se touchent ; le second couché dérive hors de sa cellule. | Deux figures collées se coupent à la colonne la moins remplie de leur tiers central (`scinder`) ; un corps couché qui sortirait de la bande garde le centre du précédent. |
| Loup | Rien d'illisible ; la marche bouge peu (rapport marche/repos 1,0, sous le seuil de 4 du LOT-91). | Gardée telle quelle. |
| Soldat Ironhand | La rangée `hit` n'a que trois figures ; la lecture brute en fabriquait une quatrième en détachant le bouclier du corps. | `--garde-finale hit` : les trois images dessinées, puis l'image 1 d'`idle` — la « garde » que la disposition demande pour la quatrième. |

Le dernier recours de lecture, `par_la_grille` (couper près des bornes de cellule quand les figures
se touchent sur la grille), est écrit et n'a pas servi : `scinder` a suffi. La planche du lion est
versée dans `atelier/ancres/planche_grand.png`.

Écarts relevés à la revue, gardés : le soldat porte des filets dorés que sa fiche ne demandait pas,
et une traînée d'épée orangée ; les figurines sont petites dans leur cellule (le loup fait 28 pixels
d'art, le lion 44 dans une cellule de 96), comme la fiche le voulait.

## Les sentinelles portent le soldat

*Décision de l'auteur, 18 septembre 2026* : le soldat Ironhand **est** la « sentinelle » et la
« patrouille Ironhand » des figurines de la Capitale ([LOT-96](LOT-96-quartiers-capitale.md), `LOT-27`). Les dix
sentinelles des portes de Martpart et d'Arenarea, jusque-là dessinées par leur marqueur, portent
donc `"figure": "Monsters/ironhand-soldier"`. Une valeur de `figure` qui contient une barre est un
dossier depuis `Assets/` (`hmi::figureStripPath`) ; sans barre, c'est un PNJ de `Npc/`, comme
avant. Le marqueur d'une figurine de monstre absente a sa clé, `monsters/<slug>`. Le lion et les
loups, eux, attendent le `LOT-27` pour paraître au Colisée.

## Ce qui a été livré à l'ouverture

- L'outillage ci-dessus, éprouvé à blanc le 18 septembre 2026 : deux planches **synthétiques** (un
  lion Grand, un loup Moyen, sans `cast`) sont passées de `chatgpt.py` à `integre.py`, bandes aux
  bonnes dimensions, manifeste écrit, `check_asset_keys.py` vert, la famille « Monstres » dans la
  galerie ; puis retirées.
- `Source/Elements/Assets/Monsters/manifest.json`, avec ses deux gabarits et aucune créature.
- `check_asset_keys.py` valide les figurines, et ses tests (`scripts/tests/test_check_asset_keys.py`)
  prouvent qu'une créature sans sort est acceptée sans `cast`, et qu'un `cast` livré mais non
  déclaré, une animation requise manquante, une bande mal dimensionnée, une créature hors catalogue
  ou un dossier hors manifeste sont refusés.
- La galerie des assets lit `Monsters/` comme `Npc/` (`readFigures`), testée par
  `AssetGalleryTest.FigurinesDeMonstres`.
- Les fiches B des trois créatures, puis leurs portraits et leurs figurines (ci-dessus).

## Les blocs

Cochés par `integre.py` quand la figurine est intégrée. **173 blocs relevés** : les 94 du SRD
(`LOT-33`, au catalogue), et 79 blocs du *Tanares Sourcebook* relevés le 18 septembre 2026 par leur
ligne de taille suivie d'« Armor Class ». Le `LOT-46` en annonce 82 : son extraction fait foi, et
complètera cette liste. Les trois premières sont en tête.

### Les trois de la version 0.0.1

- [x] Lion — Grande — `lion`
- [x] Loup — Moyenne — `wolf`
- [x] Soldat Ironhand — Moyenne — Sourcebook p. 164 — `ironhand-soldier`

### Le bestiaire du SRD (`LOT-33`)

- [ ] Grand singe — Moyenne — `ape`
- [ ] Bec de hache — Grande — `axe-beak`
- [ ] Babouin — Petite — `baboon`
- [ ] Blaireau — Très petite — `badger`
- [ ] Chauve-souris — Très petite — `bat`
- [ ] Ours noir — Moyenne — `black-bear`
- [ ] Faucon de sang — Petite — `blood-hawk`
- [ ] Sanglier — Moyenne — `boar`
- [ ] Ours brun — Grande — `brown-bear`
- [ ] Chameau — Grande — `camel`
- [ ] Chat — Très petite — `cat`
- [ ] Serpent constricteur — Grande — `constrictor-snake`
- [ ] Crabe — Très petite — `crab`
- [ ] Crocodile — Grande — `crocodile`
- [ ] Cerf — Moyenne — `deer`
- [ ] Loup sanguinaire — Grande — `dire-wolf`
- [ ] Cheval de trait — Grande — `draft-horse`
- [ ] Aigle — Petite — `eagle`
- [ ] Éléphant — Très grande — `elephant` — *gabarit à écrire*
- [ ] Élan — Grande — `elk`
- [ ] Serpent volant — Très petite — `flying-snake`
- [ ] Grenouille — Très petite — `frog`
- [ ] Singe géant — Très grande — `giant-ape` — *gabarit à écrire*
- [ ] Blaireau géant — Moyenne — `giant-badger`
- [ ] Chauve-souris géante — Grande — `giant-bat`
- [ ] Sanglier géant — Grande — `giant-boar`
- [ ] Mille-pattes géant — Petite — `giant-centipede`
- [ ] Serpent constricteur géant — Très grande — `giant-constrictor-snake` — *gabarit à écrire*
- [ ] Crabe géant — Moyenne — `giant-crab`
- [ ] Crocodile géant — Très grande — `giant-crocodile` — *gabarit à écrire*
- [ ] Aigle géant — Grande — `giant-eagle`
- [ ] Élan géant — Très grande — `giant-elk` — *gabarit à écrire*
- [ ] Scarabée de feu géant — Petite — `giant-fire-beetle`
- [ ] Grenouille géante — Moyenne — `giant-frog`
- [ ] Chèvre géante — Grande — `giant-goat`
- [ ] Hyène géante — Grande — `giant-hyena`
- [ ] Lézard géant — Grande — `giant-lizard`
- [ ] Pieuvre géante — Grande — `giant-octopus`
- [ ] Chouette géante — Grande — `giant-owl`
- [ ] Serpent venimeux géant — Moyenne — `giant-poisonous-snake`
- [ ] Rat géant — Petite — `giant-rat`
- [ ] Scorpion géant — Grande — `giant-scorpion`
- [ ] Hippocampe géant — Grande — `giant-sea-horse`
- [ ] Requin géant — Très grande — `giant-shark` — *gabarit à écrire*
- [ ] Araignée géante — Grande — `giant-spider`
- [ ] Crapaud géant — Grande — `giant-toad`
- [ ] Vautour géant — Grande — `giant-vulture`
- [ ] Guêpe géante — Moyenne — `giant-wasp`
- [ ] Belette géante — Moyenne — `giant-weasel`
- [ ] Araignée-loup géante — Moyenne — `giant-wolf-spider`
- [ ] Chèvre — Moyenne — `goat`
- [ ] Faucon — Très petite — `hawk`
- [ ] Requin-chasseur — Grande — `hunter-shark`
- [ ] Hyène — Moyenne — `hyena`
- [ ] Diablotin — Très petite — `imp`
- [ ] Chacal — Petite — `jackal`
- [ ] Épaulard — Très grande — `killer-whale` — *gabarit à écrire*
- [ ] Lézard — Très petite — `lizard`
- [ ] Mammouth — Très grande — `mammoth` — *gabarit à écrire*
- [ ] Molosse — Moyenne — `mastiff`
- [ ] Mule — Moyenne — `mule`
- [ ] Pieuvre — Petite — `octopus`
- [ ] Chouette — Très petite — `owl`
- [ ] Panthère — Moyenne — `panther`
- [ ] Serpent venimeux — Très petite — `poisonous-snake`
- [ ] Ours polaire — Grande — `polar-bear`
- [ ] Poney — Moyenne — `pony`
- [ ] Pseudodragon — Très petite — `pseudodragon`
- [ ] Quasit — Très petite — `quasit`
- [ ] Piranha — Très petite — `quipper`
- [ ] Rat — Très petite — `rat`
- [ ] Corbeau — Très petite — `raven`
- [ ] Requin de récif — Moyenne — `reef-shark`
- [ ] Rhinocéros — Grande — `rhinoceros`
- [ ] Cheval de selle — Grande — `riding-horse`
- [ ] Tigre à dents de sabre — Grande — `saber-toothed-tiger`
- [ ] Scorpion — Très petite — `scorpion`
- [ ] Hippocampe — Très petite — `sea-horse`
- [ ] Squelette — Moyenne — `skeleton`
- [ ] Araignée — Très petite — `spider`
- [ ] Esprit follet — Très petite — `sprite`
- [ ] Nuée de chauves-souris — Moyenne — `swarm-of-bats`
- [ ] Nuée d'insectes — Moyenne — `swarm-of-insects`
- [ ] Nuée de serpents venimeux — Moyenne — `swarm-of-poisonous-snakes`
- [ ] Nuée de piranhas — Moyenne — `swarm-of-quippers`
- [ ] Nuée de rats — Moyenne — `swarm-of-rats`
- [ ] Nuée de corbeaux — Moyenne — `swarm-of-ravens`
- [ ] Tigre — Grande — `tiger`
- [ ] Vautour — Moyenne — `vulture`
- [ ] Cheval de guerre — Grande — `warhorse`
- [ ] Belette — Très petite — `weasel`
- [ ] Zombi — Moyenne — `zombie`

### Les blocs de Tanares (`LOT-46`, relevé provisoire)

- [ ] Giant Beetle — Grande — Sourcebook p. 79 — `giant-beetle`
- [ ] Giant Wasp Queen — Grande — Sourcebook p. 79 — `giant-wasp-queen`
- [ ] Kitsune — Moyenne — Sourcebook p. 102 — `kitsune`
- [ ] Etoraax Manifestation — Gigantesque — Sourcebook p. 118 — `etoraax-manifestation` — *gabarit à écrire*
- [ ] Chosen of Ba-Ka — Moyenne — Sourcebook p. 133 — `chosen-of-ba-ka`
- [ ] Akhu Knight — Moyenne — Sourcebook p. 134 — `akhu-knight`
- [ ] Zealot of Ba-Ka — Moyenne — Sourcebook p. 134 — `zealot-of-ba-ka`
- [ ] Nightmare Warrior — Très grande — Sourcebook p. 135 — `nightmare-warrior` — *gabarit à écrire*
- [ ] Nightmare Lord — Très grande — Sourcebook p. 136 — `nightmare-lord` — *gabarit à écrire*
- [ ] Archnightmare — Gigantesque — Sourcebook p. 136 — `archnightmare` — *gabarit à écrire*
- [ ] Scouting Eye — Moyenne — Sourcebook p. 137 — `scouting-eye`
- [ ] Penumbral Guardian — Moyenne — Sourcebook p. 138 — `penumbral-guardian`
- [ ] Watcher Overlord — Moyenne — Sourcebook p. 138 — `watcher-overlord`
- [ ] Purple Witches Sorcerer — Moyenne — Sourcebook p. 140 — `purple-witches-sorcerer`
- [ ] Purple Witch Initiate — Moyenne — Sourcebook p. 140 — `purple-witch-initiate`
- [ ] Purple Witch Mistress — Moyenne — Sourcebook p. 141 — `purple-witch-mistress`
- [ ] Arachne Ambusher — Très grande — Sourcebook p. 142 — `arachne-ambusher` — *gabarit à écrire*
- [ ] Arachne Commander — Très grande — Sourcebook p. 142 — `arachne-commander` — *gabarit à écrire*
- [ ] Arachne Elder Matriarch — Très grande — Sourcebook p. 142 — `arachne-elder-matriarch` — *gabarit à écrire*
- [ ] Collector — Petite — Sourcebook p. 143 — `collector`
- [ ] Ancient Collector — Petite — Sourcebook p. 143 — `ancient-collector`
- [ ] Shadow Wing Initiate — Moyenne — Sourcebook p. 144 — `shadow-wing-initiate`
- [ ] Shadow Mage — Moyenne — Sourcebook p. 145 — `shadow-mage`
- [ ] Prophet of Shadows — Moyenne — Sourcebook p. 145 — `prophet-of-shadows`
- [ ] Master of Pain — Moyenne — Sourcebook p. 145 — `master-of-pain`
- [ ] Bauronite Dragon Wyrmling — Moyenne — Sourcebook p. 146 — `bauronite-dragon-wyrmling`
- [ ] Adult Bauronite Dragon — Très grande — Sourcebook p. 147 — `adult-bauronite-dragon` — *gabarit à écrire*
- [ ] Young Bauronite Dragon — Grande — Sourcebook p. 147 — `young-bauronite-dragon`
- [ ] Ancient Bauronite Dragon — Gigantesque — Sourcebook p. 148 — `ancient-bauronite-dragon` — *gabarit à écrire*
- [ ] Penumbral Dragon Wyrmling — Moyenne — Sourcebook p. 149 — `penumbral-dragon-wyrmling`
- [ ] Young Penumbral Dragon — Grande — Sourcebook p. 149 — `young-penumbral-dragon`
- [ ] Adult Penumbral Dragon — Très grande — Sourcebook p. 149 — `adult-penumbral-dragon` — *gabarit à écrire*
- [ ] Ancient Penumbral Dragon — Gigantesque — Sourcebook p. 150 — `ancient-penumbral-dragon` — *gabarit à écrire*
- [ ] Tameranium Dragon Wyrmling — Moyenne — Sourcebook p. 152 — `tameranium-dragon-wyrmling`
- [ ] Young Tameranium Dragon — Grande — Sourcebook p. 152 — `young-tameranium-dragon`
- [ ] Adult Tameranium Dragon — Très grande — Sourcebook p. 153 — `adult-tameranium-dragon` — *gabarit à écrire*
- [ ] Ancient Tameranium Dragon — Gigantesque — Sourcebook p. 153 — `ancient-tameranium-dragon` — *gabarit à écrire*
- [ ] Ridge Drake — Grande — Sourcebook p. 154 — `ridge-drake`
- [ ] Desert Drake — Grande — Sourcebook p. 154 — `desert-drake`
- [ ] Jungle Drake — Grande — Sourcebook p. 154 — `jungle-drake`
- [ ] Penumbral Anger — Moyenne — Sourcebook p. 155 — `penumbral-anger`
- [ ] Penumbral Envy — Moyenne — Sourcebook p. 156 — `penumbral-envy`
- [ ] Penumbral Disgust — Moyenne — Sourcebook p. 156 — `penumbral-disgust`
- [ ] Penumbral Fear — Moyenne — Sourcebook p. 157 — `penumbral-fear`
- [ ] Penumbral Gluttony — Grande — Sourcebook p. 158 — `penumbral-gluttony`
- [ ] Penumbral Grudge — Grande — Sourcebook p. 158 — `penumbral-grudge`
- [ ] Penumbral Greed — Moyenne — Sourcebook p. 159 — `penumbral-greed`
- [ ] Penumbral Misery — Très petite — Sourcebook p. 160 — `penumbral-misery`
- [ ] Penumbral Lust — Grande — Sourcebook p. 160 — `penumbral-lust`
- [ ] Penumbral Pride — Grande — Sourcebook p. 161 — `penumbral-pride`
- [ ] Penumbral Pain — Moyenne — Sourcebook p. 161 — `penumbral-pain`
- [ ] Penumbral Sadness — Moyenne — Sourcebook p. 162 — `penumbral-sadness`
- [ ] Penumbral Sloth — Très grande — Sourcebook p. 162 — `penumbral-sloth` — *gabarit à écrire*
- [ ] Gloomfolk Hive Hunter — Moyenne — Sourcebook p. 163 — `gloomfolk-hive-hunter`
- [ ] Gloomfolk Hero — Moyenne — Sourcebook p. 163 — `gloomfolk-hero`
- [ ] Ironhand Brute — Moyenne — Sourcebook p. 164 — `ironhand-brute`
- [ ] Ironhand Arbalist — Moyenne — Sourcebook p. 164 — `ironhand-arbalist`
- [ ] Kemet Warrior — Moyenne — Sourcebook p. 165 — `kemet-warrior`
- [ ] Kemet Archer — Moyenne — Sourcebook p. 165 — `kemet-archer`
- [ ] Kemet Berserker — Moyenne — Sourcebook p. 166 — `kemet-berserker`
- [ ] Kemet Assassin — Moyenne — Sourcebook p. 166 — `kemet-assassin`
- [ ] Kemet Necromancer — Moyenne — Sourcebook p. 167 — `kemet-necromancer`
- [ ] Kemet Ice Elementalist — Moyenne — Sourcebook p. 167 — `kemet-ice-elementalist`
- [ ] Kepesh Elite Warrior — Moyenne — Sourcebook p. 168 — `kepesh-elite-warrior`
- [ ] Ba-Ka Faithful Servant — Moyenne — Sourcebook p. 168 — `ba-ka-faithful-servant`
- [ ] High Priest of Ba-Ka — Moyenne — Sourcebook p. 169 — `high-priest-of-ba-ka`
- [ ] Sand Wizard — Moyenne — Sourcebook p. 169 — `sand-wizard`
- [ ] Kikoku Soldier — Grande — Sourcebook p. 170 — `kikoku-soldier`
- [ ] Kikoku Worker — Moyenne — Sourcebook p. 170 — `kikoku-worker`
- [ ] Lindwurm — Très grande — Sourcebook p. 171 — `lindwurm` — *gabarit à écrire*
- [ ] Kikoku Queen — Très grande — Sourcebook p. 171 — `kikoku-queen` — *gabarit à écrire*
- [ ] Penumbral Aberration — Moyenne — Sourcebook p. 172 — `penumbral-aberration`
- [ ] Stone Spirit — Grande — Sourcebook p. 173 — `stone-spirit`
- [ ] Spectral Ninja — Moyenne — Sourcebook p. 173 — `spectral-ninja`
- [ ] Makian Guard — Moyenne — Sourcebook p. 174 — `makian-guard`
- [ ] Makian Thopter — Grande — Sourcebook p. 175 — `makian-thopter`
- [ ] Makian Shaker — Très grande — Sourcebook p. 175 — `makian-shaker` — *gabarit à écrire*
- [ ] Makian Tool — Moyenne — Sourcebook p. 175 — `makian-tool`

## Bilan

Statut : **livré le 18 septembre 2026** (ouvert le même jour). Le lion, le loup et le soldat
Ironhand sont dans `Source/Elements/Assets/Monsters/`, au manifeste, et dans la galerie des assets ;
`check_asset_keys.py` et les 768 tests sont verts. Reste la **relecture à l'écran** dans la galerie
(`--screen=AssetGallery`, famille « Monstres »), animation par animation : la capture automatique
part avant le chargement des images. Le détail de la production est plus haut.

Alimente : [LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27) (le lion et les loups du Colisée, le soldat Ironhand de la ville),
`LOT-46` (les 82 blocs de Tanares).
Exigences : `EX-CNT-040` et `EX-CNT-041` (les clés d'assets et leur état d'avancement, que
`check_asset_keys.py` étend aux figurines), `EX-CNT-042` (tout asset livré paraît dans la galerie).
Aucune exigence ajoutée.
