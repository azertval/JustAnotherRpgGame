+++
id = "LOT-316"
titre = "Les mannequins de remplacement"
version = "0.0.1"
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

## Pourquoi

Les PNJ de la démo (`LOT-113`, `LOT-114`, `LOT-115`) et les monstres arrivent après le moteur qui
les joue. Sans mannequin, une carte peuplée reste un semis de jetons jusqu'à ce que l'atelier ait
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
`silhouette` et le contrôle des assets sont livrés avec lui. Restent à produire : les trois
orientations manquantes de l'humanoïde, et les mannequins quadrupède et volant.

## Risques et questions ouvertes

- Le mannequin SE-v1 n'est **pas validé en mouvement** (`Tools/AssetHd/NPC/ManequinNpc/SE-v1/review.md`) :
  il tient sa place, il ne fait pas référence. Sa marche se juge par
  `scripts/assetsGeneration/check_figure_walk.py` comme toute autre.
- Un mannequin trop lisible finirait dans une capture ou une démo : il porte ses couleurs de
  chantier exprès, pour qu'on ne le confonde jamais avec un asset.
