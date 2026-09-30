+++
id = "LOT-1009"
titre = "Les quatre héros"
version = "0.0.2.5"
filiere = "pnj"
statut = "a-faire"
taille = "M"
resume = "Brawler, Mage, Priest et Scoundrel sont des personnages assemblés dans l'atelier : un corps, une texture, des pièces — la première série produite par la chaîne."
prerequis = ["LOT-1008"]
livrables = [
  "Les quatre fiches de personnage sous `Common/Characters/Heroes/`, assemblées dans l'atelier : corps, texture, pièces, teintes.",
  "Leurs quatre **textures de corps**, commandées au générateur d'images d'après la consigne du standard (LOT-1001).",
  "La **bibliothèque de pièces** qu'ils demandent : cheveux et barbes, pièces d'armure, la robe du mage, les armes de leurs fiches préfabriquées.",
  "Le temps passé par héros, écrit en fin de fiche : c'est le coût d'un personnage, que les lots de PNJ de la `0.0.3` reprendront.",
  "Le kit `Common` republié et verrouillé.",
]
criteres = [
  "Chacun des quatre héros se reconnaît à côté de son portrait (jugement de l'auteur).",
  "Les quatre jouent les six animations sans pièce qui traverse le corps ; la robe du mage suit la marche.",
  "L'arme en main est celle de l'inventaire : en changer change la pièce affichée.",
  "Aucun des quatre n'a demandé de retouche à la main d'un maillage ni d'une animation : seules la texture et la fiche sont propres au héros.",
  "La liste `portraits` n'existe plus dans le manifeste `Characters/` ; `check_hd_assets.py` refuse un héros sans fiche.",
]
+++

## Pourquoi

Le LOT-1000 a prouvé la chaîne sur un personnage, à la main. Celui-ci la fait tourner sur une série,
avec l'outil : c'est la première mesure honnête de ce que coûte un personnage, et la bibliothèque
de pièces qu'il laisse sert à tous ceux qui suivent.

## Périmètre

Dedans : les quatre héros de la `0.0.2`, leurs pièces, leurs textures.

Dehors, nommément : leurs **portraits** et **jetons**, qui restent peints
([D-30](../../../../vision/decisions.md)) ; les PNJ de la quête, qui restent des mannequins jusqu'à
leur lot de zone ; toute règle de classe.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Le **portrait d'attente** : un héros sans figurine, porté par la liste `portraits` | `Source/Elements/Assets/Common/Characters/manifest.json` ; sa prise en charge dans `check_hd_assets.py`, `install_hd_asset.py` et `hmi::AssetGallery` | les quatre ont leur fiche ; le concept (`LOT-145`) n'a plus de cas |
| Le brawler et le scoundrel **de la preuve** | atelier local `Tools/Assets3D/Characters/` ; leurs fiches d'essai si elles ont été installées | remplacés par les héros assemblés ; les données d'essai du moteur (`Source/Test/Fixtures/`) restent, elles ne sont pas des héros |
| Le repli des héros sur le mannequin | rien à retirer dans le code — la règle sert aux PNJ sans fiche — mais plus aucun héros ne doit y tomber | critère de `check_hd_assets.py` |
| Les sources 2D des quatre classes | atelier local : `Tools/AssetHd/NPC/Classes/LOT-136-v1/` | à archiver par l'auteur, si ce n'est fait au LOT-1006 |

## Conception

- **La robe du mage** est le cas dur annoncé au LOT-1000 : une pièce liée aux os des jambes. Si elle
  ne tient pas, le mage porte une tunique courte et la fiche le dit.
- **Les armes** viennent des fiches préfabriquées (*Player's Guide*, p. 195, 199, 203, 207) ; une
  arme est une pièce de l'emplacement « main », choisie par l'inventaire.
- Les corps : le brawler (demi-orc) sur le corps musclé, le priest (nain des collines) sur un corps
  mis à l'échelle — le cas « petit » éprouvé au LOT-1006.

## Risques et questions ouvertes

- Si un héros ne se reconnaît pas, la décision D-31 se rouvre pour lui : une forme sculptée par
  personnage, liée au même squelette.
