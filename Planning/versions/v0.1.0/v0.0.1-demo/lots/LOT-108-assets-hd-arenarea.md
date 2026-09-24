+++
id = "LOT-108"
titre = "Assets HD — Arenarea"
version = "0.0.1"
filiere = "assets"
statut = "livre"
taille = "XL"
resume = "Le quartier extérieur complet d'Arenarea — avenue, manoirs, jardins, lieux nommés, hippodrome, Natural Pool, Arena Gate et Arching Bridge —, et les compléments du commun de la Capitale qu'il exige."
prerequis = ["LOT-105", "LOT-129"]
livrables = [
  "`Regions/central-empire/capital/arenarea/Scene/` : les pièces propres (postes A), `manifest.json`, `appearance.json`.",
  "`Regions/central-empire/capital/Common/Scene/` : les compléments du commun (postes C), ajoutés sans retoucher les pièces validées.",
  "La commande de la zone (`Tools/AssetsHD/`), alignée sur les identifiants de la [checklist validée](../../../../standards/checklist-lot108-arenarea.md).",
  "La page de galerie de la zone, la carte d'assemblage exhaustive et la scène de quartier rendues par le moteur.",
]
criteres = [
  "Chaque poste S, C, M, J, U, G, D, B, P, H, E et F de la checklist est livré ou explicitement retiré par l'auteur ; ses pièces paraissent dans la galerie, à l'échelle du standard.",
  "Aucune pièce ne double une pièce du kit commun ; aucune pièce validée du LOT-105 n'est retouchée.",
  "Le sol de la zone est posé sur douze cases sur douze sans qu'aucun motif régulier n'apparaisse.",
  "Les conditions V01 à V10 de la checklist sont tenues ; la scène de quartier se charge et se rend à 1080p et 2160p.",
]
sources = [
  "Tanares Sourcebook, p. 96-99 ; plan VTT (référence seule) : Herofate Ave, Gauntlet St, Golden Chalice Casino, Dusk of Justice, Inlet's Bazaar, Cloaked Brewer Pub, Hippodrome, Natural Pool, Arena Gate, Arching Bridge",
]
+++

## Le lieu

Le quartier des nobles : manoirs à colonnes de marbre, jardins de façade, fontaines de pierre, Herofate Avenue.

## Le périmètre : la checklist validée

L'inventaire est la [checklist du LOT-108](../../../../standards/checklist-lot108-arenarea.md) :
102 postes d'assets et d'assemblages, et dix conditions de fabrication (V01-V10). L'auteur l'a
validée le **24 septembre 2026**, avec deux décisions :

- **Les compléments du commun (postes C, et ceux de J) relèvent de ce lot.** Ils s'installent dans
  le commun de la Capitale, à côté des pièces du `LOT-105`, qu'ils complètent sans les retoucher ;
  les postes A restent propres à Arenarea.
- **L'hippodrome (H01-H09) et l'Arching Bridge (E05) se produisent dès maintenant**, bien que la
  carte de démo du [LOT-109](LOT-109-carte-arenarea.md) ne les montre pas : le kit sert le quartier
  entier, la démo n'en parcourt qu'une part.

Il n'y a pas de budget de poids ([D-23](../../../../vision/decisions.md)). Les postes R (porte du
triomphe, façade du Colisée) se coordonnent avec le [LOT-106](LOT-106-assets-hd-arena-of-fate.md) :
une seule source, une seule clé.

Le détail du quartier — texte du livre et lieux nommés sur le plan — est dans
[le référentiel de la Capitale](../../../../referentiels/central-empire/capitale.md).

## Le risque du sol

La maquette du [LOT-101](LOT-101-standard-2d-hd.md) a montré le poste qui coûte : répétée sur tout
l'écran, la dalle bordée de la planche de référence dessine un **treillis** sombre qui n'existe dans
aucune ville. Le sol se produit donc en premier et se juge en premier — d'abord une dalle de fond
sans bordure en trois variantes au moins (règle du [§4 du standard](../../../../standards/style-2d-hd.md)),
les panneaux bordés seulement ensuite, pour border une place ou tracer une allée. Le critère
ci-dessus se vérifie à l'œil, sur douze cases sur douze : c'est là que le moiré se voit, pas sur
quatre.

## Les toits

Le [LOT-129](LOT-129-etages-et-toits.md), livré, a posé les étages et la toiture de la Capitale :
les manoirs et l'hippodrome s'en coiffent. La suspension du 23 septembre est levée.

## Hors du lot

Les figurines, les animaux, les intérieurs (casino, pub, manoirs), la carte (`LOT-109`) et le
gameplay en hauteur : escaliers, tribunes et terrasses sont des pièces visuelles (V05).

## Livré le 24 septembre 2026

La production coordonnée ([production-lot108.md](../../../../standards/production-lot108.md)) a
installé **1 491 pièces** : 1 029 dans le commun de la Capitale, 462 dans Arenarea. Chaque poste
de la checklist a ses pièces, sauf M12 et F06, qui sont des assemblages : ils sont livrés en
**préfabriqués de l'éditeur** (`Source/Elements/Editor/Prefabs/`) — trois manoirs témoins au
niveau de la Capitale, deux parvis témoins à celui d'Arenarea.

Vérifié sur l'arbre installé, indépendamment des rapports de l'atelier :

- `check_hd_assets.py` et `check_binary_files.py` conformes ; les 705 entrées communes antérieures
  identiques, aucun PNG déjà suivi modifié (V01) ;
- les pièces rangées par famille sous `Scene/` (règle 3 de l'arborescence) — l'installation les
  avait posées dans un dossier plat `lot108/` ; les clés n'ont pas changé ;
- build et 997 tests CTest verts, dont `AssetGalleryTest.ToutAssetLivreEstDansLaGalerie`
  (2 214 images de la Capitale) ; 139 tests Python ;
- dix cartes d'assemblage de l'atelier, ramenées au lieu `central-empire/capital/arenarea` et
  résolues par niveaux sur l'arbre installé : `--check` sans erreur, rendu identique à celui de
  l'atelier à quelques variantes de sol près.

Revue artistique de l'auteur (galerie de l'atelier et
rendus moteur). Relevés soumis à la première revue : le dallage de marbre dessine un quadrillage serré et
régulier sur une grande place (critère du sol) ; les gradins de l'hippodrome se lisent comme des
blocs pleins ; l'amorce de l'Arching Bridge montre un raccord en biais, et la place touche l'eau
sans quai dans l'assemblage témoin.

**Reprise du 24 septembre** (`Production108/Review108/`) : marbre S01 repeint, tribunes à gradins
de bois, approche du pont à marches et parapets, quai continu ; 87 PNG d'Arenarea modifiés, le
commun intact, aucune clé ajoutée ni retirée. Revérifié sur l'arbre installé : contrôles conformes,
997 tests verts, onze cartes témoins sans erreur. Relevés restants pour la revue :

- **S01** : `floor-marble-05` et `-06` sont des copies exactes de `-01` et `-02` — quatre fonds
  distincts au lieu de six ; le contraste de la matière (écart-type de luminance ≈ 5) est deux à
  trois fois plus faible que celui des sols validés de la V4 (12 à 15) : plus de treillis, mais un
  sol presque uni à l'échelle 1, loin du « relief lisible » des critères ;
- **C10/B04** : les « bouts gauches » des auvents de bazar et de l'auvent rayé replié sont le module
  courant, sans fermeture ; seul le bout droit ferme.

**Deuxième reprise** (`Production108/Review108B/`) : six fonds de marbre distincts (écart moyen
minimal 5,5), contraste de 12,5 à 15,4, au niveau de la V4 ; ourlet gauche en volume sur les huit
bouts d'auvent. Revérifié : 73 PNG modifiés, contrôles conformes, 997 tests verts, douze cartes
témoins sans erreur. L'auteur a validé le lot en l'état le 24 septembre 2026 (pose décalée du marbre comprise) ; la
checklist est cochée.

## En fin de lot : les assets sortent de l'historique Git

Dernière tâche du lot, demandée par l'auteur le 24 septembre 2026 : les images des kits livrés
(`Regions/`, `Common/`, les cartes `Maps/` et le HUD `UI/`) sont publiées en archives immuables,
Git ne garde que leurs manifestes et un verrou, l'installation les télécharge d'elle-même, puis le
suivi des images archivées est retiré (sans réécrire l'historique). Le détail — format, travail
T1-T6, précautions et critères — est dans
[l'annexe dédiée](../../../../standards/assets-hors-git-lot108.md).

**Fait** : sept kits publiés en `@1` (191 Mio) et verrouillés dans
`Source/Elements/Assets/kits.lock.json` ; `scripts/fetch_assets.py` les installe (poste, build,
CI) ; les images de `Common/`, `Regions/`, `Maps/` et `UI/` ne sont plus suivies. Les images du
LOT-108 ne sont jamais entrées dans l'historique.
