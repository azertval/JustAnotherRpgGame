+++
id = "LOT-147"
titre = "Zone — Arenarea, repris au standard du jeu final"
version = "0.0.3"
filiere = "cartes"
statut = "a-faire"
taille = "L"
resume = "Les assets et la carte d'Arenarea livrés par les LOT-108 et LOT-109 sont refaits au standard que le jeu final exige ; la carte de principe de la démo leur cède la place."
prerequis = ["LOT-151"]
livrables = [
  "`Regions/central-empire/capital/arenarea/Scene/` repris : chaque poste de la [checklist du LOT-108](../../../../standards/checklist-lot108-arenarea.md) relu pièce par pièce, refait là où il est en deçà du standard, gardé là où il le tient ; le kit republié et verrouillé (`@2`).",
  "Les compléments du commun de la Capitale (postes C) relus de même, sans retoucher les pièces validées du `LOT-105`.",
  "`Levels/central-empire/capital/arenarea.json` : la carte du quartier entier, reprise sur les pièces refaites, dessinée **dans l'éditeur** ; elle remplace la carte de principe du `LOT-146` sous le même identifiant, et garde ses arrivées et ses zones nommées.",
  "L'image de la zone pour l'onglet « Carte », peinte par l'auteur, sous `capital/arenarea/Map/`.",
  "Le relevé de ce qui n'était pas au standard dans les deux livraisons, versé au [standard](../../../../standards/style-2d-hd.md) ou à la [consigne de production](../../../../standards/consigne-2d-hd.md) : ce qu'une zone ne doit plus livrer.",
]
criteres = [
  "L'auteur valide la zone sur la galerie **et** sur le rendu du moteur, à 1080p et 2160p, comme au standard du jeu final — pas « en l'état ».",
  "Le sol de la zone est posé sur douze cases sur douze sans motif régulier ; chaque pièce tient la palette de l'Empire et la lumière du standard.",
  "`LevelEditor --check` passe ; la carte tient 60 images par seconde à 1080p sur le poste de référence.",
  "La quête « Des pommes pour l'arène » se rejoue de bout en bout sur la carte reprise, sans changer une ligne de la quête.",
]
sources = [
  "Tanares Sourcebook, p. 91-92, 96-99 ; plan VTT (référence seule)",
]
+++

## Pourquoi

Les [LOT-108](../../v0.0.1-demo/lots/LOT-108-assets-hd-arenarea.md) et
[LOT-109](../../v0.0.1-demo/lots/LOT-109-carte-arenarea.md) ont été livrés le 24 septembre 2026
et validés en l'état. Relus le lendemain, ils sont **loin du standard de qualité** que l'auteur veut
pour le jeu final ([D-25](../../../../vision/decisions.md)) : ils sont à refaire. La démo n'en
dépend plus — elle se joue sur des cartes de principe (`LOT-146`) — et la reprise trouve sa place
ici, avec les six autres quartiers, où une zone se produit pour de bon.

Ce lot est aussi l'occasion d'apprendre : ce qui, dans ces deux livraisons, n'était pas au
standard doit s'écrire dans le standard lui-même, pour qu'aucune zone de la `0.0.3` ne le refasse.

## Périmètre

**Dedans** : les pièces de scène d'Arenarea et les compléments communs qu'il a versés, la carte
jouable, l'image de l'onglet. La checklist du `LOT-108` reste l'inventaire ; ce lot ne l'étend pas.

**Pas dedans** : les PNJ d'Arenarea (`LOT-114`), l'Arena of Fate (`LOT-106`, `LOT-107`) — dont la
façade se coordonne avec ce lot —, les intérieurs, les animaux.

## Conception

Ce qui est repris se décide **sur pièce**, à la relecture : une pièce qui tient le standard se
garde, une pièce qui ne le tient pas se refait, jamais « on refait tout » par principe. La
production suit la [consigne](../../../../standards/consigne-2d-hd.md) et le
[gabarit de commande](../../../../standards/gabarit-commande-zone.md), comme toute zone. La carte
reprend le tracé du `LOT-109` — le quartier entier, ses lieux nommés, ses portails condamnés vers
les cartes qui n'existent pas encore — sur les pièces refaites.

## Risques et questions ouvertes

- **La taille.** Le `LOT-108` a installé 1 491 pièces. Si la relecture en refait la majorité, le
  lot est un `XL` et se redécoupe (assets, puis carte), comme le gabarit de la version le prévoit.
- **Ce que « standard du jeu final » veut dire** n'est écrit nulle part avec cette exigence : le
  premier geste du lot est de le dire, sur les pièces livrées, avant d'en refaire une.
