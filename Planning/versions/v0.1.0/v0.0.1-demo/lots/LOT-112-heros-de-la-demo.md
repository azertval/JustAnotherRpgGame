+++
id = "LOT-112"
titre = "Le héros de la démo"
version = "0.0.1"
filiere = "pnj"
statut = "en-cours"
taille = "M"
resume = "Le personnage que l'on joue : la fiche pré-tirée du Brawler, une figurine HD animée, un portrait."
prerequis = ["LOT-104"]
livrables = [
  "`Common/Characters/Heroes/brawler/` : la figurine du héros (repos, marche, attaque, touché, mort ; quatre orientations), portrait, jeton.",
  "Sa fiche de niveau 1, `Rpg/characters/heros-brawler.json` : la fiche pré-tirée du Brawler (*Player's Guide to Tanares*, p. 195), reprise telle quelle ; elle remplace le personnage de démonstration.",
  "Le gabarit de figurine HD, qui sert ensuite à tous les PNJ.",
  "Le **nombre d'images par animation**, tranché sur un essai de marche en six et en huit images, puis écrit au [§5 du standard](../../../../standards/style-2d-hd.md) (déféré par le LOT-101, D-101-8).",
]
criteres = [
  "Le héros marche sur la maquette du LOT-101 sans glisser ni flotter : ancre et sol justes aux quatre orientations.",
  "Sa fiche redonne les nombres du livre, et sa Persuasion à DD 18 réussit une fois sur dix (Charisme 8, non maîtrisée).",
  "Le §5 du standard ne dit plus « six ou huit » : il dit un nombre, et le héros l'applique.",
]
+++

## Pourquoi

La première figurine HD fixe le **gabarit** de toutes les autres : hauteur, ancre, cadence. Elle
passe avant les PNJ.

## Périmètre

Un seul héros, **préfabriqué** : pas de création de personnage dans la démo. C'est le Brawler
pré-tiré du *Player's Guide* (demi-orc, Dragon Hunter, grande hache), le premier des quatre héros
de la `0.0.2` : le [LOT-136](../../v0.0.2-combat/lots/LOT-136-assets-des-quatre-classes.md) n'a plus
que les trois autres à produire.

## L'essai : six ou huit images

Le [LOT-101](LOT-101-standard-2d-hd.md) a figé le standard de la scène et laissé cette seule valeur
ouverte : une cadence se juge sur une figurine, à côté de son ancre et de son sol, pas sur une place
vide. L'écart de coût est d'**un quart sur chaque PNJ du jeu**, sur chaque animation et chaque
orientation — d'où l'essai sur pièces plutôt que le raisonnement.

La commande prend le bloc A de [la consigne](../../../../standards/consigne-2d-hd.md) inchangé, le
bloc B dans sa variante **planche d'animation**, et ce bloc C envoyé **deux fois** — rien d'autre ne
change que le nombre d'images :

```
PIECE: a walk cycle for a city guard of the Central Empire, seen from the
south-east isometric direction: burgundy tabard over a mail shirt, old gold
trim, a spear held upright in the right hand, no shield.
FAMILY: figure — humanoid, 170 px tall, ground line at the bottom of each frame.
PLACE: Arenarea, the arena quarter of the Capital; accent burgundy.
VARIANTS: {SIX | EIGHT} frames — one full walk cycle that loops, the contact,
down, passing and up poses evenly spread over the strip.
```

Ce qu'on regarde, dans cet ordre :

1. la marche en six images **saccade-t-elle** à la cadence du jeu ? Si oui, la question est close.
2. à huit, le générateur tient-il le **même personnage** d'une image à l'autre ? Deux images de plus,
   c'est deux occasions de plus de dériver, et une dérive se repeint à la main.
3. le quart de coût en plus, sur tout le jeu, l'auteur l'accepte-t-il ?

Le garde de l'essai n'est pas le héros : c'est volontaire. L'essai tranche une **cadence**, et un
personnage secondaire suffit à la montrer ; le héros, lui, se dessine une fois la valeur connue.
