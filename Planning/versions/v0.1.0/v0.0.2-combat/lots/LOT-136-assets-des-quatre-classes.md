+++
id = "LOT-136"
titre = "Assets des quatre classes"
version = "0.0.2"
filiere = "assets"
statut = "livre"
taille = "L"
resume = "Les quatre héros ont leur figurine HD, leur portrait, leurs effets de sort."
prerequis = ["LOT-130"]
livrables = [
  "`Common/Characters/Heroes/{brawler,mage,priest,scoundrel}/` : figurine animée (repos, marche, attaque, sort, touché, mort), portrait, jeton.",
  "`Common/Fx/` : *fire bolt*, *magic missile*, *sacred flame*, *cure wounds*, *bless*, *fireball*, l'impact et le raté.",
]
criteres = [
  "Les quatre héros se distinguent **à la silhouette**, à la taille du jeu.",
  "Chaque sort des niveaux 1 à 5 a un effet visible ; aucun n'est un simple texte.",
]
sources = [
  "Player's Guide to Tanares, p. 195, 199, 203, 207 : les quatre illustrations — **référence de costume seulement**",
]
+++

## Les quatre silhouettes

| Classe | Fiche du livre | Silhouette |
|---|---|---|
| Brawler | Half-Orc, sans armure, grande hache | massive, torse nu, arme à deux mains |
| Mage | Autumn Elf, bâton | élancée, robe, bâton |
| Priest | Hill Dwarf, écailles, bouclier, marteau | trapue, bouclier, symbole sacré |
| Scoundrel | Human, cuir, rapière, arc court | fine, capuche, deux lames |

Le héros de la démo **est** le Brawler : le [LOT-112](../../v0.0.1-demo/lots/LOT-112-heros-de-la-demo.md)
livrait `Heroes/brawler/` (sans l'animation de sort, qu'un Brawler n'a pas). Ce lot le **reprend**
avec les trois autres, dans l'atelier commun (décision de l'auteur, 28 sept. 2026).

## Décisions de réalisation

Livré le 28 septembre 2026 après l'essai en jeu de l'auteur, branche
`lot-136-assets-des-quatre-classes`, **PR #158**.

- **Les images de l'atelier sont prises telles quelles.** Source :
  `Tools/AssetHd/NPC/Classes/LOT-136-v1/` (hors Git), une pose HD par image, habillée sur le
  mannequin du [LOT-145](LOT-145-mannequins-de-remplacement.md) ; `pose-jobs.json` donne la
  dernière version de chaque pose. Sur décision de l'auteur, les 648 poses encore « à revoir » de
  l'atelier sont retenues avec les 88 qu'il avait approuvées ; les remarques de
  `delivery-summary.json` (tête de hache du Brawler en NE, rapière du Scoundrel en NE et SW…) restent
  à juger en jeu.
- **Le Brawler du LOT-112 est remplacé** par celui de l'atelier (même portrait) : ses vingt bandes
  (repos, marche, attaque, touché, mort, quatre orientations) sont celles du LOT-136.
- **Bandes.** `Tools/AssetsHD/Common/Characters/Heroes/build_lot136_strips.py` pose les huit poses de
  chaque action côte à côte (planche 2560 × 320, comme le mannequin) et écrit le descripteur
  `install.json` ; `install_hd_asset.py` installe. Mage et Priest : six actions ; Scoundrel : cinq
  plus le **tir à l'arc** ; Brawler : cinq. 92 bandes au total.
- **Taille par classe.** L'installateur ramène le repos à 170 px ; les quatre héros y avaient la
  même hauteur, ce qui contredit le critère de silhouette. Chaque bande porte donc une `scale` : le
  standard multiplié par la taille de la classe — Brawler × 1,10, Mage × 1,03, Scoundrel × 0,97,
  Priest × 0,85.
- **Nouvelle animation `ranged`** (tir) : `hmi::figure_clips::RANGED`, déclarée dans le manifeste
  `Characters/`. Une attaque dont l'arme a une portée la joue ; une figurine sans bande de tir (le
  mannequin d'une autre silhouette) joue son `attack` (`figure_clips::fallbackOf`).
- **Le mannequin humanoïde prend les attaques par arme** de l'atelier
  (`Tools/AssetHd/NPC/ManequinNpc/WeaponAttacks/<DIR>-v1/planches`, demande de l'auteur) : l'arc
  devient sa bande `ranged` — les bandits à l'arbalète tirent —, la dague, l'arme à une main et
  l'arme à deux mains entrent au kit sous `dagger`, `onehand` et `twohand`, quatre orientations,
  huit images. Le moteur ne choisit pas encore la bande par l'arme tenue : ces trois-là ne se
  voient que dans la galerie.
- **L'écran joue enfin l'incantation.** `core::ArenaSession::setActionObserver` annonce le début et
  la fin de chaque attaque et de chaque sort (`ArenaActionNotice` : acteur, cible, identifiant du
  sort, tir, raté). `EncounterModel` en tire le geste (`attack`, `ranged` ou `cast`) — jusqu'ici
  aucun sort ne jouait la bande `cast` —, et l'attaque annoncée par le combat n'ajoute pas un second
  geste ; les attaques d'opportunité passent toujours par `AttackDeclared`.
- **Effets.** 20 planches du générateur découpées par `Tools/AssetsHD/Common/Fx/build_fx.py` le long
  des couloirs transparents (les images n'y sont pas sur une grille régulière), en cellules de
  256 px de haut, sol à 252 : les 17 sorts du Mage et du Priest jusqu'au niveau 5, l'impact, le raté,
  la flèche. Les projectiles (trait de feu, projectile magique, rayon ardent, flèche) ont leur
  miroir `-left`. Le moteur les dessine comme des figurines sans orientation (`Common/Fx/<effet>`) :
  `CombatCueKind::Effect`, `CombatCueTrack::effects()`. Un projectile vole du lanceur à la cible
  pendant la moitié du geste ; le reste paraît à la cible à l'impact : l'effet du sort, l'impact de
  chaque coup encaissé, le raté d'un jet manqué. La galerie des assets joue les effets par leur
  `.anim.json`.
- **Essai en jeu** : validé par l'auteur le 28 septembre 2026.
