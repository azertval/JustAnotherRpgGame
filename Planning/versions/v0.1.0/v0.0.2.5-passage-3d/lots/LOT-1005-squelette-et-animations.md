+++
id = "LOT-1005"
titre = "Squelette et animations"
version = "0.0.2.5"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Le moteur anime un modèle par son squelette : une figurine est un maillage lié au squelette commun, qui joue les animations communes."
prerequis = ["LOT-1003"]
livrables = [
  "La **déformation par os** dans la passe de maillages : matrices d'os, poids par sommet, lecture du squelette et des clips d'un `.glb`.",
  "Les **clips d'animation** : repos, marche, attaque, sort, touché, mort, avec durée, boucle et **image clé** (l'instant de l'impact) déclarés en données.",
  "La **fiche de personnage** lue par le moteur : son modèle (`.glb`) et son squelette. Les modèles sont sans arme ([D-42](../../../../vision/decisions.md)) : le moteur n'accroche aucune pièce à un os.",
  "`hmi::FigureResolver` : une figurine est une fiche de personnage **ou**, en attendant le LOT-1006, des bandes ; la règle de repli par `silhouette` désigne un squelette.",
  "L'orientation libre d'un modèle : il fait face à sa direction de marche ou à sa cible, sans table de quatre orientations.",
  "Données d'essai : un modèle tiré du brawler de la preuve (LOT-1000), réduit pour tenir sous le plafond de 5 Mio des fichiers suivis, sous `Source/Test/Fixtures/` ; tests sans GPU (pose d'un os à un instant) et rendu hors écran.",
  "Le **budget d'un modèle** — triangles, définition de la texture, os et influences par sommet — mesuré sur `bench_world_frame` et écrit au [standard 3D](../../../../standards/style-3d.md) ([D-40](../../../../vision/decisions.md)).",
]
criteres = [
  "Sur la carte d'essai, le modèle d'essai marche, attaque, encaisse et tombe ; ses pieds ne glissent pas à 2 cases par seconde.",
  "Les signaux de combat (`CombatCues`) partent à l'image clé du clip : le journal, les dégâts et l'animation restent synchrones, comme avec les bandes.",
  "Le jeu livré, dont tous les personnages sont encore des bandes, se rend au pixel près comme avant le lot.",
  "Huit modèles animés à l'écran ne font pas régresser `bench_world_frame`.",
  "Le standard 3D porte un budget chiffré, tiré de cette mesure ; sa ligne ouverte « LOT-1005 » est close.",
]
+++

## Pourquoi

Le but de la version est là : six animations réglées **une fois** sur un squelette, rejouées par
tous les personnages. Le moteur n'a aujourd'hui qu'une forme d'animation — la bande d'images
(`hmi::AnimationCatalog`, `.anim.json`), une par animation **et** par orientation.

## Périmètre

Dedans : le squelette, les clips, la fiche, l'accroche des pièces, sur données d'essai.

Dehors, nommément :

- les vrais corps et les vraies animations ([LOT-1006](LOT-1006-corps-de-reference.md)) ;
- les **effets** (`Common/Fx/`) : ils restent des bandes, et `AnimationCatalog` reste pour eux ;
- le tissu simulé, la cinématique inverse, le mélange d'animations au-delà d'un fondu : pas dans
  cette version.

## À supprimer

Rien d'installé : à la livraison, le jeu affiche encore ses bandes. C'est la seule cohabitation de
la version, elle dure **un lot**, et son retrait est écrit.

| Quoi | Où | Retiré par |
|---|---|---|
| Les bandes de figurine et leur lecture (cellule, ligne de sol, quatre orientations) | `Common/Characters/`, `SceneTextureTraits`, `ScenePieces.h`, `WorldSceneComposer` | le [LOT-1006](LOT-1006-corps-de-reference.md), dans sa PR |
| La branche « bandes » de `hmi::FigureResolver` | `Source/HMI/Game/FigureResolver.{h,cpp}` | le LOT-1006 |
| Le choix d'une bande `-se`, `-sw`, `-ne`, `-nw` d'après la direction | le rendu des figurines | le LOT-1006 : un modèle s'oriente librement |

## Conception

- **Un squelette par silhouette.** Ce lot ne connaît que `humanoid` ; `quadruped` et `flying`
  viendront avec leurs créatures, sans code nouveau.
- **Un maillage par personnage, un squelette** ([D-38](../../../../vision/decisions.md)) : le
  moteur lit le modèle que la fiche cite ; vêtements, armure et coiffure sont dans ce maillage.
- **Aucune pièce accrochée** (D-42) : les modèles sont sans arme. L'accroche d'une arme à un os
  de main n'entre dans ce lot que si l'auteur la décide. Une robe ou une cape longue est déformée
  avec le personnage.
- **L'image clé** remplace l'indice d'image des bandes : un clip déclare l'instant de l'impact, et
  `CombatCues` s'y accroche.

## Tranché à l'ouverture (2 octobre 2026)

- **Trois fichiers disent un personnage en modèle.** Sa **fiche** (`character.json`, dans son
  dossier) nomme son modèle et la silhouette de son squelette ; son **modèle** (`.glb`) porte le
  maillage, la liaison de chaque sommet, les os et les courbes des clips ; la **description du
  squelette** (`Common/Characters/Skeletons/<silhouette>/skeleton.json`) déclare les os et, par
  clip, la durée, la boucle et l'image clé. Les courbes restent dans le `.glb` de chaque personnage
  parce qu'elles sont posées sur ses propres articulations (un enfant n'a pas la foulée d'un
  adulte) ; ce qui est commun — et que le combat lit — est dans `skeleton.json`.
- **Le chargeur reste dans `Core`** (`core::readMeshFile`) et lit en plus `skins[0]`, `JOINTS_0`,
  `WEIGHTS_0` et les animations (translation et rotation, `LINEAR` ou `STEP`). La pose se calcule
  sans GPU (`core::poseSkeleton`) : un test lit la position d'un os à un instant.
- **Un second pipeline, pas un second chemin.** Un maillage lié a un second tampon de sommets (os
  et poids) et se dessine par `mesh_skinned.vert` ; un maillage fixe garde le pipeline du
  `LOT-1003`, donc son image. Le bloc d'os a **64** matrices.
- **Le cap est un angle** (`WorldFigureSnapshot::heading`) : la direction du pas ou de la cible,
  dans le plan de la grille. La diagonale (`FigureFacing`) reste pour les bandes, et donne son cap
  à un modèle qui n'en reçoit pas.
- **L'image clé arrive au combat par combattant** (`hmi::CombatCueTrack::setTimings`) : le geste
  dure son clip, le touché de la cible part à `key`. Une figurine en bandes garde 0,64 s et
  l'impact à mi-geste : le jeu livré ne change pas.
- **Arme → clip.** Le moteur ne connaît que deux gestes d'arme, l'attaque et le tir (`ranged`) ; les
  noms `dagger`, `onehand`, `twohand` du manifeste des bandes ne sont lus par aucun code. Un modèle
  joue `attack` pour toute arme de contact, et `attack` aussi pour un tir tant que son squelette
  n'a pas de clip `ranged` — c'est le cas du squelette humanoïde, qui en a six.
- **Les données d'essai ne viennent pas du brawler de la preuve.** Ses fichiers liés et la chaîne
  qui les produisait (`Tools/Assets3D/Meshy-test/`) n'étaient plus dans l'atelier à l'ouverture du
  lot. Le modèle d'essai est le **mannequin humanoïde** reçu le 2 octobre, réduit à 4 000 triangles
  (`Source/Test/Fixtures/Characters`, 0,6 Mio), et un **pantin** de trois os écrit octet par octet
  (`Source/Test/Fixtures/Meshes`). La chaîne de liaison a donc été réécrite dans ce lot, où la
  fixture en dépend : `scripts/assetsGeneration/rig_character.py` (liaison aux 53 os, six clips,
  `skeleton.json`), `scripts/checks/check_character_model.py` (les contrôles du standard, §9) et
  `scripts/assetsGeneration/render_character_review.py` (les planches de revue). Le
  [LOT-1006](LOT-1006-corps-de-reference.md) s'en sert pour les vrais personnages et soumet les
  clips à l'auteur.

## Mesures (poste de l'auteur, 2 octobre 2026)

| Mesure | Avant | Avec huit modèles animés |
|---|---:|---:|
| `Benchmarks`, `ArenareaFrame1080p` → `ArenareaFrame1080pEightModels` (composition, sans GPU) | 3,7 µs | 46 µs : la pose de huit squelettes de 53 os |
| `CanvasBenchmarks`, `WorldFrameStrips1080p` → `WorldFrameEightModels1080p`, mannequin d'essai (4 000 triangles) | 5,8 ms | 6,0 ms |
| le même, huit modèles de l'atelier (100 000 triangles, texture 2048 px, `JADG_FIGURE_MODELS`) | 5,9 ms | 6,3 ms, 237 Mio de mémoire graphique |
| `CanvasReadbackFloor` (effacer et relire une image vide) | 5,6 ms | — |

Les trois dernières lignes comprennent la relecture de l'image, que le jeu ne paie pas. Le kit
d'Arenarea n'était pas installé sur le poste au moment de la mesure : la scène autour des modèles
est la carte sans ses images. Le budget qu'en tire le standard est au
[§3](../../../../standards/style-3d.md#3-le-poids-dun-modèle).

Contrôles de la chaîne sur le mannequin d'essai (`check_character_model.py`) : pénétration du sol
0,44 mm, glissement du pied posé inférieur à 0,53 px d'art.

## Ce que le lot laisse

- Aucun personnage **livré** n'est encore un modèle : le kit `Common` n'a ni fiche ni
  `skeleton.json`. Le [LOT-1006](LOT-1006-corps-de-reference.md) les installe et retire les bandes.
- La **galerie** de débug et les contrôles d'assets (`check_hd_assets.py`) ne connaissent pas encore
  les modèles de personnage : au LOT-1006, avec les fichiers.
- La cible `fuzz_mesh` s'amorce désormais aussi sur le pantin lié ; elle n'a pas été relancée sur
  le poste dans ce lot — la nuit de la CI le fait.

## Risques et questions ouvertes

- Le nombre d'os par sommet et par squelette fixe la taille des tampons du shader. La preuve a
  mesuré 53 os et quatre influences par sommet ; **ce lot fixe le budget**, le standard ne l'a pas
  borné (D-40).
- Les modèles de la preuve pèsent 15 Mio et 103 000 triangles : s'ils font régresser
  `bench_world_frame`, le budget se resserre ici et le LOT-1006 produit en conséquence.
- Les attaques par arme du mannequin (`LOT-136`) associent aujourd'hui une bande à une arme :
  l'association devient arme → clip, à vérifier arme par arme. Le clip joué suit l'arme de
  l'inventaire ; le modèle, lui, a les mains vides (D-42).
