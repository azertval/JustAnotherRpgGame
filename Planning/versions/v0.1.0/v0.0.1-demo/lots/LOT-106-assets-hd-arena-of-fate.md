+++
id = "LOT-106"
titre = "Assets HD — Arena of Fate (donjon d'Arenarea)"
version = "0.0.1"
filiere = "assets"
statut = "a-faire"
taille = "L"
resume = "Les pièces propres à Arena of Fate, produites au standard et installées dans `capital/arenarea/arena-of-fate/Scene/`."
prerequis = ["LOT-105", "LOT-108", "LOT-129"]
livrables = [
  "`Regions/central-empire/capital/arenarea/arena-of-fate/Scene/` : pièces, `manifest.json`, `appearance.json`.",
  "La commande de la zone (`Tools/AssetsHD/`) : les dix familles passées en revue, ce qui vient du kit, ce qui est propre.",
  "La page de galerie de la zone.",
]
criteres = [
  "Toutes les pièces de l'inventaire ci-dessous paraissent dans la galerie, à l'échelle du standard.",
  "Les 14 divinités reconnues ont chacune leur statue, et les 4 factions reçues chacune leur tribune et son drapeau : la galerie les montre nommées, et aucune ne manque.",
  "Aucune pièce du lot ne représente un Ungod ni le Culte de l'Aile d'Ombre : ces pièces sont au LOT-157.",
  "Aucune pièce ne double une pièce du kit commun.",
]
sources = [
  "Tanares Sourcebook, p. 91-92, 99 : seule arène où l'on conteste un décret impérial",
  "Tanares Sourcebook, ch. 4 (p. 68-86) : les 18 divinités — 14 en statue sur la coursive, les 4 Ungods aux catacombes (LOT-157)",
  "Tanares Sourcebook, ch. 2 (p. 38-49) : les 5 factions — 4 tribunes d'honneur, le Culte n'est pas reçu",
  "Le Colisée de Rome : référence d'architecture (DA de l'auteur, 21 sept. 2026)",
]
+++

## Le lieu

L'amphithéâtre ovale en bord de baie, **à la manière du Colisée de Rome** : sable en contrebas,
podium de marbre, trois niveaux d'arcades superposées, attique à pilastres, gradins rouges, feux
sur le pourtour. La direction artistique complète est dans
[le référentiel de la Capitale](../../../../referentiels/central-empire/capitale.md#larena-of-fate--architecture-et-iconographie-da-de-lauteur-21-sept-2026).
Elle est la **commande** de ce lot ; ce qui suit n'en est que le découpage en pièces.

## Ce que ce lot habille : deux niveaux sur trois

Le monument en compte trois (D-18) :

| Niveau | Ce qu'il est | Ce lot ? |
|---|---|---|
| **0 — l'arène** | sable, podium, coursive, gradins | **oui** |
| **−1 — sous les tribunes** | vestiaires des gladiateurs, **prison des condamnés** | **oui** |
| **−2 — les catacombes** | les Ungods enchaînés, le donjon cultiste | **non** — `LOT-157` |

Au niveau 0, du sable vers l'extérieur, la scène se lit en **trois anneaux**, et l'inventaire suit
ce découpage :

1. **Le podium** — mur droit de deux cases, plaqué de marbre, balustrade au sommet ;
2. **La coursive des dieux** — **14 socles à statue** face au sable, un brasero entre deux socles ;
3. **Les gradins** — **quatre secteurs de peuple**, chacun percé d'une **tribune d'honneur** de
   faction, drapeau tendu devant ; derrière eux, l'enceinte à arcades et l'attique.

## Inventaire des pièces propres

**01 Sols** — sable (dalle de fond répétable en 3 variantes, règle du §4 du standard), pavé de la
coursive et du niveau −1, bordures sable / pavé / marbre.

**02 Façades** — enceinte extérieure : travée d'arcade toscane (rez), ionienne (1er), corinthienne
(2e), panneau d'attique à pilastre et corbeau de mât, angle rentrant, angle sortant.

**03 Colonnes** — les trois ordres en pièce isolée, pour les loges et le seuil.

**04 Accès** — **porte du triomphe** et **porte des morts** (grand axe), herse, grille de vestiaire,
bouche de vomitoire.

**05 Balustrades** — couronnement du podium (droite, angle, pilier) ; garde-corps de loge.

**06 Pièces maîtresses** — **14 statues de divinités** sur socle gravé, en marbre clair, plus un
socle particulier pour Fumetsu Tenshinkin, qui n'est pas un vrai dieu. **4 tribunes d'honneur** :
la loge impériale (la plus grande) et trois loges ouvertes. Braseros du pourtour. **Aucune pièce
d'Ungod ni du Culte** : elles sont au `LOT-157`, avec les catacombes.

**07 Végétal** — néant : rien ne pousse dans une arène (la famille est passée en revue et écartée).

**08 Mobilier** — niveau −1. **Vestiaires des gladiateurs** : râtelier d'armes, banc, fontaine,
seau, torche murale. **Prison des condamnés** : grille de cellule, paillasse, chaîne murale, banc
de garde, brasero de corps de garde. Les deux jeux ne se ressemblent pas : l'un est un vestiaire,
l'autre une geôle.

**09 Bâtiments** — **gradins du peuple en HD**, quatre secteurs, chacun dans la couleur de sa faction ;
les bandes de foule existantes sont en pixel art et se refont. Les deux gardiens de l'entrée.

**10 Seuils** — parvis de la porte du triomphe, escalier du niveau −1, arche de vomitoire, et
l'**escalier des catacombes** au fond de la prison : la pièce est produite ici, **condamnée et
close** ; ce qu'il y a derrière est au `LOT-157`.

**Les drapeaux** — quatre bannières de faction (Empire tanaréen, Forces alliées, Arcanum, Forces
de Darkall), au repos et animées au vent ; le Culte n'est pas reçu, donc pas de cinquième. Héraldique **inventée**, décrite au
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
**Les catacombes en sont dehors** — pièces, Ungods enchaînés, iconographie du Culte : tout cela est
au `LOT-157`. Ce lot s'arrête à la porte condamnée au fond de la prison.
