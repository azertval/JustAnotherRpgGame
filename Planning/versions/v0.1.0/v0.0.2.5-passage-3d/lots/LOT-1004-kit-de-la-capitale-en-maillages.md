+++
id = "LOT-1004"
titre = "Le kit de la Capitale en maillages"
version = "0.0.2.5"
filiere = "assets"
statut = "a-faire"
taille = "L"
resume = "L'architecture de la Capitale — sols, murs, balustrades, haies, escaliers, toits — devient des volumes : mêmes clés, mêmes cartes, et la combinatoire des pièces d'angle et de jonction disparaît."
prerequis = ["LOT-1003"]
livrables = [
  "Le script du kit (`Tools/AssetsHD/…/capital/Common/`, atelier local) émet des **maillages `.glb`** et un **atlas de matières** au lieu de PNG projetés ; ses commandes de reconstruction sont dans le `README.md` du kit.",
  "`Regions/central-empire/capital/Common/Scene/` et `Regions/central-empire/Common/Scene/` : les maillages installés, le manifeste où chaque clé convertie cite son maillage — **les clés ne changent pas**.",
  "Les **toits** : un maillage paramétré par profondeur et par sorte (droit, L, T, X) au lieu de 686 images.",
  "`check_hd_assets.py` et `install_hd_asset.py` connaissent les maillages ; la galerie de débug les montre (`EX-CNT-042`).",
  "Les kits republiés et verrouillés (`publish_asset_kit.py`).",
  "Le rendu des quatre cartes livrées, avant et après, joint à la PR.",
  "Le [standard 3D](../../../../standards/style-3d.md) complété de ce que ce lot mesure et tranche : la **forme** (maillage ou image) de chaque pièce des familles laissées « à classer », le **budget** d'un maillage de décor, la hauteur d'étage en mètres, et le verdict de l'auteur sur le **contour sombre**, jugé sur le kit rendu avec et sans ([D-40](../../../../vision/decisions.md)) ; le chemin d'un maillage de décor écrit dans le [gabarit de commande](../../../../standards/gabarit-commande-zone.md).",
]
criteres = [
  "Les quatre cartes livrées se chargent sans retouche et `LevelEditor --check` passe : aucune clé de pièce n'a changé.",
  "Côte à côte, à 100 et 200 px par case, l'auteur juge le mur, le sol et le toit en maillage conformes aux critères de qualité validés le 23 septembre (continuité, vrais angles, volume, matière propre).",
  "Un toit en L, en T et en X se monte sur la carte d'essai sans pièce de jonction dédiée.",
  "Plus aucun PNG ne reste dans les dossiers `floors/`, `walls/`, `balustrades/`, `stairs/` et `roofs/` du kit ; `check_hd_assets.py` refuse une pièce en image posée sur une couche d'étage.",
  "`git grep -n \"occlusion\\|FLOOR_SEAM_OVERLAP\" Source/HMI` ne trouve plus le mécanisme d'étage 2D ni le recouvrement des dalles.",
  "Le poids du kit de la Capitale est publié, avant et après.",
  "Le standard 3D n'a plus de ligne ouverte « LOT-1004 » : contour, formes et budget de décor y sont écrits, datés.",
]
+++

## Pourquoi

Murs, balustrades, haies et toits de la Capitale sont **déjà** des volumes : des matières à plat
(`wall-surface.png`, `paving-surface.png`…) projetées par script sur des formes, puis aplaties en
PNG. Ce lot arrête l'aplatissement. La méthode ne change pas, la sortie si — et quatre des critères
de qualité qui ont coûté quatre versions au `LOT-105` deviennent vrais par construction.

## Périmètre

Dedans : l'architecture du kit commun de la Capitale et de l'Empire central.

Dehors, nommément ([D-30](../../../../vision/decisions.md)) :

- le **mobilier** et les **pièces maîtresses** peints (`props/`, `columns/`, et ce que l'inventaire
  ci-dessous classe en image) : tolérés en image jusqu'à la `0.0.3` ;
- les kits d'**Arenarea**, de l'**Arena of Fate** et de **Martpart** : repris avec leur quartier ;
- la lumière : les maillages sortent non éclairés, comme le moteur les dessine au LOT-1003.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Les PNG des familles converties : sols (180), murs (294), balustrades (102), escaliers (78), toits (686) — **1 340 images** — et les haies parmi `plants/` | `Source/Elements/Assets/Regions/central-empire/capital/Common/Scene/` ; les 2 murs de `central-empire/Common/Scene/walls/` | les maillages les remplacent sous les mêmes clés ; le kit se republie sans elles |
| Leurs entrées `width`, `height`, ancre en pixels | `Scene/manifest.json` (862 Kio aujourd'hui) | une clé en maillage ne porte plus de géométrie d'image |
| La hauteur d'étage en pixels d'art (`"storey": 224`) | manifestes des kits | une hauteur en mètres, dans le maillage |
| Le mécanisme d'étage 2D : rang de tri `Storey + n − 1`, `ComposedQuad::storey` et `occlusion`, `coverCells`, effacement à 0,35 de la pièce entière | `Source/HMI/Graphics/ComposedScene.{h,cpp}`, `WorldSceneComposer`, `StaticWorldScene`, `RenderLayer.h` | plus aucune pièce d'étage n'est une image ; l'effacement devant le héros se refait sur la géométrie |
| Le recouvrement des dalles (`FLOOR_SEAM_OVERLAP`) | `WorldSceneComposer` | un sol en maillage n'a pas de bord adouci à recouvrir |
| La maquette 2D HD de non-régression | `Source/Test/Fixtures/HdMockup/`, `test_hd_mockup_render.cpp`, `scripts/assetsGeneration/build_hd_mockup.py`, `scripts/tests/test_build_hd_mockup.py` | elle vérifie un rendu de sols et de façades en image qui n'existe plus |
| Les références d'image des étages en 2D | `test_world_storeys.cpp`, `test_capital_kit_render.cpp`, `Source/Test/Fixtures/Storeys/` — **réétalonnés**, pas supprimés | ils gardent leur objet |
| Les calibres, jonctions et versions antérieures des pièces 2D | atelier local : `Toitures/Calibres`, `BeforeJunctions`, `BeforeMaterials`, `V4/BeforeRelief`, `Versions/before-v4` | à archiver par l'auteur ; les **matières** (`Sources/`) restent, ce sont elles qui servent |

## Conception

**Inventaire à faire en premier.** Le kit compte 1 734 images en treize dossiers. Chaque famille
se classe en l'une des deux sortes, et la fiche garde le tableau :

| Dossier | Images | Sorte attendue |
|---|---|---|
| `floors/`, `walls/`, `balustrades/`, `stairs/`, `roofs/` | 1 340 | projetée par script → **maillage** |
| `plants/` | 91 | haies → maillage ; arbres et massifs → image |
| `access/`, `facades/`, `buildings/`, `bridges/`, `docks/` | 134 | **à classer** : maillage si la pièce sort d'une projection, image tolérée sinon |
| `props/`, `columns/` | 169 | image tolérée jusqu'à la `0.0.3` |

**Un maillage par forme, pas par placement.** Les variantes `u` / `v`, les angles rentrants et
sortants, les débuts et fins de toit sont aujourd'hui des fichiers ; ils deviennent une rotation,
ou une section du même maillage. Les **clés** restent pour que les cartes ne bougent pas : plusieurs
clés peuvent citer le même maillage avec une orientation.

**Les matières** du kit sont des surfaces à plat, sans ombre : elles passent telles quelles en
atlas. L'ombre propre que le script ajoutait se fait désormais par sommet, en attendant la lumière
du [LOT-1007](LOT-1007-eclairage-et-cycle-jour-nuit.md).

## Risques et questions ouvertes

- **La facture.** Le jugement côte à côte est un critère ; s'il échoue sur une famille, elle reste
  en image et la fiche le dit — elle rejoint la dette de la `0.0.3`.
- **Les familles « à classer »** peuvent faire grossir le lot : au-delà de la taille L, elles
  partent au [LOT-151](../../v0.0.3-capitale-intra-muros/lots/LOT-151-kit-commun-intra-muros.md).
- Ne pas repartir des anciens calibrages V3 : la référence reste la V4 après reprise du relief.
- **Le contour sombre**, s'il est gardé, demande une passe de rendu que le LOT-1003 n'a pas
  prévue : elle entre alors dans ce lot, ou dans un lot de moteur à part si elle le fait déborder.
