+++
id = "LOT-145"
titre = "Les mannequins de remplacement"
version = "0.0.2"
filiere = "pnj"
statut = "en-cours"
taille = "M"
resume = "Toute entité sans figurine se voit et s'anime quand même : un mannequin par silhouette tient la place de l'asset, en exploration comme en combat, jusqu'à ce que l'atelier livre le vrai."
prerequis = ["LOT-112"]
livrables = [
  "`Common/Characters/Placeholders/<silhouette>/` : un mannequin par silhouette — `humanoid`, `quadruped`, `flying` —, six animations (repos, marche, attaque, sort, touché, mort), huit images, quatre orientations, portrait et jeton.",
  "La règle de repli du moteur : un PNJ sans `figure`, une figurine dont la bande de repos manque, une créature du bestiaire sans figurine se dessinent par le mannequin de leur silhouette (`hmi::FigureResolver`).",
  "La propriété `silhouette` sur une entité `npc` et sur une fiche de créature (`Rpg/creatures/*.json`), qui choisit le mannequin ; sans elle, `humanoid`.",
  "Le mannequin humanoïde SE-v1 de l'atelier (`Tools/AssetHd/NPC/ManequinNpc/SE-v1/planches`), installé dès le `LOT-118` avec sa seule orientation, servie aux quatre.",
]
criteres = [
  "Une carte dont aucun PNJ n'a de figurine montre un mannequin à chaque PNJ, qui respire au repos et marche, attaque, encaisse et tombe en combat.",
  "Chaque silhouette du bestiaire de la démo a son mannequin dans les quatre orientations, et `check_hd_assets.py` les accepte.",
  "Une figurine livrée remplace le mannequin sans toucher ni à la carte ni au code : seul le manifeste `Characters/` change.",
]
+++

## Rescopé le 25 septembre 2026 : à la `0.0.2`

À la recette de la `0.0.1` ([LOT-122](../../v0.0.1-demo/lots/LOT-122-recette-et-version-0-0-1.md)),
le lot est **en cours** : le mannequin humanoïde SE-v1 est installé dans sa seule orientation, la
règle de repli, la propriété `silhouette` et le contrôle des assets sont livrés avec le `LOT-118` ;
les trois orientations manquantes de l'humanoïde et les mannequins quadrupède et volant restent à
produire. La démo n'en dépend pas — elle se joue avec le mannequin tel qu'il est —, et la version
ne l'attend donc pas : le lot passe à la **`0.0.2`** ([D-26](../../../../vision/decisions.md)), où
le combat de groupe et le bestiaire des rencontres d'arène sont les premiers à en avoir besoin. Sa
fiche garde son numéro et son contenu ; seul le champ `version` change.

## Pourquoi

Les PNJ des zones (`LOT-113`, `LOT-114`, `LOT-115`, reportés à la `0.0.3` par la décision
[D-25](../../../../vision/decisions.md)) et les monstres arrivent après le moteur qui les joue ; dans la
démo, ce sont les mannequins qui tiennent la place des cinq PNJ de la quête, sur les cartes de
principe du [LOT-146](../../v0.0.1-demo/lots/LOT-146-cartes-de-principe-de-la-demo.md). Sans mannequin, une carte peuplée reste un semis de jetons jusqu'à ce que l'atelier ait
tout dessiné, et le combat sur la carte (`LOT-118`) ne peut montrer ni marche, ni coup, ni chute
tant que l'adversaire n'a pas d'asset. Le mannequin **découple** les deux chantiers : le moteur
s'éprouve sur des silhouettes neutres, l'atelier livre au rythme des lots de PNJ, et le jour où une
figurine arrive, elle prend la place sans que rien d'autre ne bouge.

Il précède donc la création des PNJ : c'est ce que le lot des PNJ de chaque zone attend de lui.

## Périmètre

**Dedans** : trois mannequins et la règle de repli ; les chaînes de production de ces mannequins
sont celles des figurines (`install_hd_asset.py`, mode figurine), sans exception.

**Pas dedans** : les figurines elles-mêmes (lots de PNJ), l'habillage des jetons de maquette
(`LOT-128`, qui restent pour ce qui n'est pas un personnage : portails, coffres, déclencheurs), et
tout mannequin de **taille** : un mannequin se met à l'échelle de sa créature par le `scale` de son
installation, il ne se redessine pas par taille.

## Conception

### Les silhouettes

Le bestiaire livré compte 94 créatures : 37 ne font que marcher, 18 volent, 15 nagent aussi, 14
grimpent ; 29 sont Grandes, 8 Très grandes, 21 Très petites. Trois silhouettes couvrent ce que la
démo et la `0.0.2` mettront sur une carte :

| Silhouette | Ce qu'elle remplace | Emprise | Repère |
|---|---|---|---|
| `humanoid` | citadins, gardes, gladiateurs, gobelinoïdes, morts-vivants debout | 1 case | le mannequin SE-v1 de l'atelier : tête ivoire, torse turquoise, membres colorés pour lire les permutations |
| `quadruped` | loups, sangliers, lions, chevaux, ours | 1 case (2 × 2 à l'échelle Grande) | même code de couleurs, quatre pattes, la tête à l'avant de la case |
| `flying` | chauves-souris, corbeaux, diablotins, tout ce qui a `speed.fly` sans marcher | 1 case, **au-dessus** de la ligne de sol | un mannequin ailé porté par une ombre au sol : la ligne de sol du manifeste est celle de l'ombre |

D'autres viendront **si une créature de la version en cours n'entre dans aucune** : une
silhouette serpentine (serpents, vers) et une silhouette amorphe (gelées, vases) sont les
candidates les plus probables de la `0.0.5` ; elles ne se commandent pas avant.

La **taille** n'est pas une silhouette : une créature Grande occupe déjà 2 × 2 cases sur la grille
(`core::CombatState`), et son mannequin se met à l'échelle par `scale` (1,6 pour une Grande, 2,2
pour une Très grande, 0,6 pour une Très petite), dans une cellule large. L'ordre de grandeur se
règle sur le premier loup et le premier ours, pas dans ce lot.

### La règle de repli

`hmi::FigureResolver` (`Source/HMI/Game/`) répond à la question *quelle bande dessiner pour ce
personnage ?* en trois temps :

1. la figurine nommée (`figure` de l'entité, ou le slug de la créature) se cherche par
   `core::figureDirectory` sous les `Characters/` du lieu et de ses niveaux communs ; si sa bande
   de repos existe (`idle-se.png`, à défaut `idle.png`), c'est elle ;
2. sinon, le mannequin de sa silhouette (`silhouette` de l'entité ou de la créature, `humanoid`
   par défaut), s'il est installé ;
3. sinon, le mannequin humanoïde ; et s'il manque lui aussi, le marqueur d'asset
   (`core::assetMarker`), comme aujourd'hui.

Un PNJ dessiné par un mannequin **n'a plus de jeton** : le jeton d'un personnage tenait lieu de
figurine, et il y en a une. Les jetons des portails, des coffres et des déclencheurs restent.

Le résolveur ne relit pas le disque à chaque image : il retient sa réponse par figurine, et
l'oublie quand la carte change.

### La donnée

- `npc.silhouette` (entité de carte, facultatif) : `humanoid`, `quadruped`, `flying`.
- `creature.silhouette` (fiche du bestiaire, facultatif) : mêmes valeurs. L'extraction du bestiaire
  ne la devine pas ; elle se pose à la main sur les créatures de la version en cours.

### Ce que le LOT-118 a déjà posé

Le mannequin humanoïde SE-v1 est installé sous `Common/Characters/Placeholders/humanoid/`, sans
orientation (`idle.png`, `walk.png`…) : une figurine sans bande orientée se dessine de la même
bande dans les quatre sens, et c'est ce que fait le moteur. La règle de repli, la propriété
`silhouette` et le contrôle des assets sont livrés avec lui.

### L'humanoïde aux quatre orientations (27 septembre 2026)

L'atelier a décliné la chorégraphie SE en SW, NE et NW (`Tools/AssetHd/NPC/ManequinNpc/<DIR>-v1/`,
48 poses par direction, une génération par pose) ; l'auteur a déclaré les quatre directions
**finales** le 27 septembre. Les 24 bandes (`<clip>-<se|sw|ne|nw>.png`) sont installées par
`install_hd_asset.py`, aux cadences de la production (repos 0,125 s, marche 0,0625 s, attaque et
sort 0,1 s, touché 0,1 s, mort 0,12 s), les six bandes sans suffixe retirées ; le kit `Common@3`
est publié. `check_hd_assets.py` passe ; la galerie montre les 24 bandes.

`check_figure_walk.py` **refuse** encore les quatre marches, sans que ce soit un défaut du dessin
avéré : trois cellules de marche (SE, SW, NE) ont été allongées à 264 px par l'installateur pour
un pied qui descend sous la ligne de sol, et l'outil n'accepte que 192 × 256 ; la NW a deux images
(1 et 7) à moins de 8 px du bord de sa cellule ; et sa revue manuelle (appuis, alternance, essai en
jeu) reste à remplir par l'auteur. Rien n'a été régénéré.

**Les portraits d'attente.** Le groupe du `LOT-138` montre ses quatre membres au HUD, dans l'écran
Groupe et dans le dialogue ; seul le Brawler avait un visage. Les portraits du Mage, du Priest
(la version Dorsi, choix de l'auteur) et du Scoundrel, produits par l'atelier du `LOT-136`, sont
installés comme **portraits d'attente** : une figurine sans bande, `"strips": []`, dont le nom va
dans la liste `portraits` du manifeste `Characters/` et non dans `npcs`. Le moteur les dessine par
le mannequin ; leurs bandes, quand elles viendront, les feront passer dans `npcs`. L'installateur,
le contrôle des assets et la galerie connaissent cette liste.

Restent : la revue de marche de l'auteur (`Planning/quality/`), les mannequins quadrupède et volant.

## Risques et questions ouvertes

- Le mannequin SE-v1 n'est **pas validé en mouvement** (`Tools/AssetHd/NPC/ManequinNpc/SE-v1/review.md`) :
  il tient sa place, il ne fait pas référence. Sa marche se juge par
  `scripts/assetsGeneration/preview_figure_walk.py` comme toute autre.
- Un mannequin trop lisible finirait dans une capture ou une démo : il porte ses couleurs de
  chantier exprès, pour qu'on ne le confonde jamais avec un asset.
