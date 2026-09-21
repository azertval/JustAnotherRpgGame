+++
id = "LOT-106"
titre = "Assets HD — Arena of Fate (donjon d'Arenarea)"
version = "0.0.1"
filiere = "assets"
statut = "a-faire"
taille = "L"
resume = "Les pièces propres à Arena of Fate, produites au standard et installées dans `capital/arenarea/arena-of-fate/Scene/`."
prerequis = ["LOT-105", "LOT-108"]
livrables = [
  "`Regions/central-empire/capital/arenarea/arena-of-fate/Scene/` : pièces, `manifest.json`, `appearance.json`.",
  "La commande de la zone (`Tools/AssetsHD/`) : les dix familles passées en revue, ce qui vient du kit, ce qui est propre.",
  "La page de galerie de la zone.",
]
criteres = [
  "Toutes les pièces de l'inventaire ci-dessous paraissent dans la galerie, à l'échelle du standard.",
  "Les 18 divinités ont chacune leur statue, et les 5 factions chacune leur tribune et son drapeau : la galerie les montre nommées, et aucune ne manque.",
  "Aucune pièce ne double une pièce du kit commun.",
  "La zone pèse moins de 40 Mio.",
]
sources = [
  "Tanares Sourcebook, p. 91-92, 99 : seule arène où l'on conteste un décret impérial",
  "Tanares Sourcebook, ch. 4 (p. 68-86) : les 18 divinités, une statue chacune",
  "Tanares Sourcebook, ch. 2 (p. 38-49) : les 5 factions, une tribune d'honneur chacune",
  "Le Colisée de Rome : référence d'architecture (DA de l'auteur, 21 sept. 2026)",
]
+++

## Le lieu

L'amphithéâtre ovale en bord de baie, **à la manière du Colisée de Rome** : sable en contrebas,
podium de marbre, trois niveaux d'arcades superposées, attique à pilastres, gradins rouges, feux
sur le pourtour, vestiaires en hypogée. La direction artistique complète — l'enceinte, les
**18 statues de divinités**, les **5 tribunes d'honneur** et leurs drapeaux, les cinq tribunes de
peuple — est dans [le référentiel de la Capitale](../../../../referentiels/central-empire/capitale.md#larena-of-fate--architecture-et-iconographie-da-de-lauteur-21-sept-2026).
Elle est la **commande** de ce lot ; ce qui suit n'en est que le découpage en pièces.

## Les trois anneaux

Du sable vers l'extérieur, la scène se lit en trois anneaux, et l'inventaire suit ce découpage :

1. **Le podium** — mur droit de deux cases, plaqué de marbre, balustrade au sommet ;
2. **La coursive des dieux** — 18 socles à statue face au sable, un brasero entre deux socles ;
3. **Les gradins** — cinq secteurs de peuple, chacun percé d'une **tribune d'honneur** de faction,
   drapeau tendu devant ; derrière eux, l'enceinte à arcades et l'attique.

## Inventaire des pièces propres

**01 Sols** — sable (dalle de fond répétable en 3 variantes, règle du §4 du standard), pavé de la
coursive et de l'hypogée, bordures sable / pavé / marbre.

**02 Façades** — enceinte extérieure : travée d'arcade toscane (rez), ionienne (1er), corinthienne
(2e), panneau d'attique à pilastre et corbeau de mât, angle rentrant, angle sortant.

**03 Colonnes** — les trois ordres en pièce isolée, pour les loges et le seuil.

**04 Accès** — **porte du triomphe** et **porte des morts** (grand axe), herse, grille de vestiaire,
bouche de vomitoire.

**05 Balustrades** — couronnement du podium (droite, angle, pilier) ; garde-corps de loge.

**06 Pièces maîtresses** — **18 statues de divinités** sur socle gravé, en trois familles de
facture : marbre clair pour les quatorze divinités reconnues, **basalte noir à socle cerclé de
chaînes** pour les quatre Ungods (C'thraxis, Droggath, Krynnethoth, Z'ulvath), et un socle
particulier pour Fumetsu Tenshinkin, qui n'est pas un vrai dieu. **5 tribunes d'honneur** :
la loge impériale (la plus grande), trois loges ouvertes, la loge du Culte **close et grillagée**.
Braseros du pourtour.

**07 Végétal** — néant : rien ne pousse dans une arène (la famille est passée en revue et écartée).

**08 Mobilier** — vestiaires A et B : râtelier d'armes, banc, paillasse, seau, torche murale.

**09 Bâtiments** — **gradins du peuple en HD**, cinq secteurs, chacun dans la couleur de sa faction ;
les bandes de foule existantes sont en pixel art et se refont. Les deux gardiens de l'entrée.

**10 Seuils** — parvis de la porte du triomphe, escalier d'hypogée, arche de vomitoire.

**Les drapeaux** — cinq bannières de faction (Empire tanaréen, Forces alliées, Arcanum, Forces de
Darkall, Culte de l'Aile d'Ombre), au repos et animées au vent. Héraldique **inventée**, décrite au
référentiel : le livre donne les emblèmes en image, jamais en texte, et **rien n'est décalqué**.
La bannière au lion de l'Empire vient du kit commun (LOT-105), en grand format.

Le détail du quartier — texte du livre et lieux nommés sur le plan — est dans
[le référentiel de la Capitale](../../../../referentiels/central-empire/capitale.md).

## Une sous-zone, pas un quartier

L'Arena of Fate est un **donjon d'Arenarea** (décision D-16) : un lieu clos, à plusieurs salles —
vestiaire A, vestiaire B, couloir, sable —, où l'on entre **depuis le quartier**, par la porte de
l'arène au bout du parvis. Elle n'a pas d'entrée sur le plan de la Capitale : l'onglet « Carte »
la montre **dans** Arenarea. Ses assets et sa carte se rangent sous `capital/arenarea/arena-of-fate/`,
et elle puise d'abord dans le kit d'Arenarea, puis dans celui de la Capitale.

## Périmètre

Les **pièces de scène** seulement. Les figurines sont au lot des PNJ, la carte au lot de la carte.
