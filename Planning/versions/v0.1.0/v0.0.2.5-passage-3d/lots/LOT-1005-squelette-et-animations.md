+++
id = "LOT-1005"
titre = "Squelette et animations"
version = "0.0.2.5"
filiere = "moteur"
statut = "a-faire"
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

## Risques et questions ouvertes

- Le nombre d'os par sommet et par squelette fixe la taille des tampons du shader. La preuve a
  mesuré 53 os et quatre influences par sommet ; **ce lot fixe le budget**, le standard ne l'a pas
  borné (D-40).
- Les modèles de la preuve pèsent 15 Mio et 103 000 triangles : s'ils font régresser
  `bench_world_frame`, le budget se resserre ici et le LOT-1006 produit en conséquence.
- Les attaques par arme du mannequin (`LOT-136`) associent aujourd'hui une bande à une arme :
  l'association devient arme → clip, à vérifier arme par arme. Le clip joué suit l'arme de
  l'inventaire ; le modèle, lui, a les mains vides (D-42).
