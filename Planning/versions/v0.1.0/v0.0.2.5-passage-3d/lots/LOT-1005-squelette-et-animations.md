+++
id = "LOT-1005"
titre = "Squelette et animations"
version = "0.0.2.5"
filiere = "moteur"
statut = "a-faire"
taille = "L"
resume = "Le moteur anime un modèle par son squelette : une figurine peut être un corps, une texture et des pièces accrochées à des os, jouant les animations communes."
prerequis = ["LOT-1003"]
livrables = [
  "La **déformation par os** dans la passe de maillages : matrices d'os, poids par sommet, lecture du squelette et des clips d'un `.glb`.",
  "Les **clips d'animation** : repos, marche, attaque, sort, touché, mort, avec durée, boucle et **image clé** (l'instant de l'impact) déclarés en données.",
  "La **fiche de personnage** lue par le moteur : corps, texture, pièces (os, maillage, teinte) ; une pièce rigide suit son os.",
  "`hmi::FigureResolver` : une figurine est une fiche de personnage **ou**, en attendant le LOT-1006, des bandes ; la règle de repli par `silhouette` désigne un squelette.",
  "L'orientation libre d'un modèle : il fait face à sa direction de marche ou à sa cible, sans table de quatre orientations.",
  "Données d'essai : le corps et le brawler de la preuve (LOT-1000) sous `Source/Test/Fixtures/` ; tests sans GPU (pose d'un os à un instant, accroche d'une pièce) et rendu hors écran.",
]
criteres = [
  "Sur la carte d'essai, le modèle d'essai marche, attaque, encaisse et tombe ; ses pieds ne glissent pas à 2 cases par seconde.",
  "Les signaux de combat (`CombatCues`) partent à l'image clé du clip : le journal, les dégâts et l'animation restent synchrones, comme avec les bandes.",
  "Changer l'arme équipée du modèle d'essai change la pièce en main, sans nouvel asset.",
  "Le jeu livré, dont tous les personnages sont encore des bandes, se rend au pixel près comme avant le lot.",
  "Huit modèles animés à l'écran ne font pas régresser `bench_world_frame`.",
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
- **Huit corps, un squelette** ([D-31](../../../../vision/decisions.md)) : le moteur ne distingue
  pas les corps, il lit celui que la fiche cite.
- **Les pièces sont rigides** : chacune suit un os. Une robe ou une cape longue est une pièce liée
  à plusieurs os ; c'est un maillage déformé comme un corps, pas un cas à part.
- **L'image clé** remplace l'indice d'image des bandes : un clip déclare l'instant de l'impact, et
  `CombatCues` s'y accroche.

## Risques et questions ouvertes

- Le nombre d'os par sommet et par squelette fixe la taille des tampons du shader : le standard
  (LOT-1001) le borne.
- Les attaques par arme du mannequin (`LOT-136`) associent aujourd'hui une bande à une arme :
  l'association devient arme → clip, à vérifier arme par arme.
