+++
id = "LOT-112"
titre = "Le héros de la démo"
version = "0.0.1"
filiere = "pnj"
statut = "livre"
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

La commande assemblée, le descripteur de l'essai (`previewOnly` : il se mesure, il ne s'installe
pas) et la marche à suivre sont dans `Tools/AssetsHD/Essais/lot-112-cadence/`. Les deux marches se
jugent par `scripts/preview_figure_walk.py`, qui les fait marcher sur la vue 1080 de la maquette,
à la vitesse du jeu.

## Décisions de réalisation

### D1 — Le héros est la fiche du livre, telle quelle

Décision de l'auteur : le héros est la fiche pré-tirée du Brawler (*Player's Guide to Tanares*,
p. 195), sans retouche — demi-orc, Dragon Hunter, For 16 Dex 13 Con 16 Int 10 Sag 12 Cha 8, 15 PV.
Le livre ne le nomme pas : il s'appelle **Grom Tranche-Écaille** (`Rpg/characters/heros-brawler.json`),
un nom proposé qui se change sans rien casser.

Conséquence voulue : la fiche d'origine visait **Persuasion +5** ; celle du livre a **−1**. Le
critère passe de « deux fois sur cinq » à « **une fois sur dix** » à DD 18 (un 19 ou un 20 au dé), et
la [quête](../quete-demo.md) est avertie : son DD, ou sa voie pacifique, se reprend au `LOT-120`.

### D2 — Il remplace Brenna, et reste provisoire avec sa classe

Le héros devient le personnage que chargent la fiche, les dialogues et l'arène
(`hmi::loadDemonstrationState`) ; `demonstration-brenna.json` part, comme son critère de retrait le
prévoyait. Sa classe est l'une des quatre classes simplifiées, déclarées provisoires : la fiche
l'est aussi, avec le même critère (le garde-fou `LesClassesProvisoiresNeSontReferenceesParRien`
l'exige).

**Écart connu** : la CA du livre (14) vient de *Tough as Nails*, sans armure 10 + Dex + Con. Les
capacités de classe arrivent en `0.0.2` ; d'ici là le moteur affiche 11.

### D3 — Quatre orientations peintes, une bande chacune

Décision de l'auteur : quatre orientations **peintes**, pas deux plus un miroir (l'arme changerait de
main). Une bande par animation et par diagonale : `walk-se.png`, `walk-sw.png`, `walk-ne.png`,
`walk-nw.png`, chacune avec son `.anim.json`. Le format de bande du moteur ne change pas.

- Avancer d'une colonne descend vers le **sud-est** de l'écran, d'une ligne vers le **sud-ouest** :
  les quatre flèches sont exactement les quatre diagonales (`hmi::figureFacingFor`).
- Deux flèches enfoncées tombent **entre** deux diagonales : la figurine garde la sienne si elle
  convient, plutôt que de basculer d'une image à l'autre.
- Une figurine est orientée si sa bande `idle-se.png` existe ; `check_hd_assets.py` refuse une
  animation orientée à moitié, et exige le repos et la marche.

### D4 — La ligne de sol est une donnée, les pieds vont au centre du losange

Le moteur posait le **bas de la cellule** un peu au-dessus de la pointe sud (marge de 0,42
losange, héritée des figurines 48 × 64) : les pieds tombaient 8,7 px d'art sous le centre du
losange, là où la maquette du `LOT-101` les met. Les manifestes `Characters/` déclarent désormais
`"ground": 252`, et une figurine qui le déclare a sa ligne de sol **exactement** sur le point de sa
position. Sans ligne de sol, l'ancien placement reste. L'échelle et le sol se lisent dans le premier
manifeste **ancêtre** : un héros est rangé par classe (`Characters/Heroes/brawler/`).

### D5 — La vitesse de marche tombe à 2 cases par seconde

Un cycle de marche (deux pas) couvre une case. À 4 cases par seconde — 6 m/s, une course —, des
pieds qui ne glissent pas demandaient quatre cycles par seconde, soit une image toutes les 31 ms à
huit images. Décision de l'auteur : **2 cases par seconde**, 3 m/s, une marche vive. Le cycle dure
une demi-seconde : **62 ms** par image à huit, **83 ms** à six. La cadence est lue dans la bande
(`frameDuration`), plus dans une constante du code.

### D6 — L'installateur a un mode figurine

`install_hd_asset.py` installe une planche de marche comme il installe une pièce : il la détoure, la
découpe en N images (colonnes vides, sinon parts égales), la met à l'échelle du **cadre debout**
(170 px), pose ses pieds sur la ligne 252 et centre la figurine dans sa cellule — un seul décalage
pour toute la bande, si bien qu'une fente ou un accroupissement dessinés restent. Il produit aussi le
portrait (512²) et le jeton (128², détouré en rond), et inscrit la figurine dans `npcs`.

- La marge de 8 px du standard tient **à gauche et à droite** de chaque cellule : c'est là que la
  voisine est, et que le mipmap bave. En bas, le bord adouci d'un pied posé sur 252 descend à 254.
- Un descripteur `previewOnly` se mesure et s'aperçoit mais ne s'écrit pas : l'essai de cadence ne
  peut pas atterrir dans les assets par mégarde.

### D7 — Huit images par animation

Les deux marches du garde, reçues le 23 septembre, ont tourné sur la maquette à 2 cases par seconde
(`build/lot-112/marche-6.webp`, `marche-8.webp`). Les mesures de l'installateur tranchaient déjà :
à six, l'image 4 décolle (sol à 244 au lieu de 252) et la silhouette dérive de 12 px vers l'avant
avant de revenir d'un coup à la reprise ; à huit, le sol reste entre 248 et 252, le bord de la
silhouette entre 65 et 71, et c'est le même garde d'un bout à l'autre. Verdict de l'auteur :
**huit**, pour toutes les animations. Le §5 du standard le dit, et la consigne reçoit ce que l'essai
a confirmé : la ligne `OUTPUT` d'une planche, la ligne `REFERENCE` des envois suivants, le cadrage
du portrait.

### D8 — Le héros installé, et ce que ses planches ont appris à l'installateur

Les vingt bandes acceptées par l'auteur (`Tools/AssetsHD/Common/Characters/Heroes/brawler/selection-acceptee.json`)
sont installées dans `Common/Characters/Heroes/brawler/` avec le portrait et le jeton : 4,3 Mio.

- **Les pieds de la plupart des images** — la médiane de leurs points bas — se posent sur 252 : ni
  l'image de repos seule (le générateur ne tient pas sa ligne de sol au pixel), ni le point le plus
  bas de la bande (la lame de l'attaque, qui plonge devant les pieds, soulevait tout de 13 px). Ce qui
  passe sous le sol agrandit la cellule vers le bas, par pas de 8 px (272 pour l'attaque vue du
  sud-est) ; le moteur pose la ligne de sol depuis le haut de la cellule et n'en sait rien de plus.
- **L'attaque vue du sud-est est recentrée** (`"recentre": true`, demande de l'auteur) : le
  générateur faisait glisser le corps de 70 px d'une image à l'autre ; chaque image pose désormais son
  bassin au milieu de la cellule, et l'attaque tient sur sa case.
- **La découpe rend chaque morceau entier à son image** ; des corps qui se touchent se partagent en
  croissant depuis le milieu de leur image. Couper aux bornes tranchait les haches.
- **Une pose plus haute que la cellule** (la hache levée) réduit toute la bande juste assez pour
  tenir, et l'installateur le dit : 6 % pour l'attaque vue du sud-est, 2 % du nord-est.
- **La mort passe en cellule large** (384 × 256), comme l'attaque : un corps allongé ne tient pas
  dans 192 px. Le standard le dit.
- **Sept bandes ont été recommandées** : leurs images se chevauchaient, et aucune découpe ne départage
  ce qui est collé. La commande a gagné une ligne `SPACING` ; les reprises se découpent par colonnes
  vides, sans deviner.

## Livraison

Livré le 23 septembre 2026, **PR #116**, sur décision de l'auteur. Le critère 1 est tenu sur la
maquette du `LOT-101` (`build/lot-112/heros-marche.webp`, quatre orientations) et en jeu sur une
carte d'essai ; deux vérifications passent à des lots suivants :

- **la marche sur une vraie carte de la démo**, qui n'existe pas encore — elle se voit dès les cartes
  des `LOT-107` à `LOT-111` ;
- **l'attaque, le touché et la mort**, installés mais pas joués : le monde ne connaît que le repos et
  la marche, et le combat sur la carte est au `LOT-118`.
