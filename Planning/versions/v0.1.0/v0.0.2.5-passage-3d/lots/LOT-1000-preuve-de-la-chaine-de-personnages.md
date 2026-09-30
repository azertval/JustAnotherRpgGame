+++
id = "LOT-1000"
titre = "La preuve de la chaîne de personnages"
version = "0.0.2.5"
filiere = "pnj"
statut = "en-cours"
taille = "M"
resume = "Un personnage fait d'un corps commun, d'une texture peinte et de pièces d'équipement marche correctement et se reconnaît : la porte qui ouvre, ou non, le passage à la 3D."
prerequis = []
livrables = [
  "Dans l'atelier local (`Tools/Assets3D/Characters/`, hors dépôt) : un corps de preuve — homme musclé, tiré de MPFB —, le squelette `game_engine` et les cinq animations que lit le moteur (repos, marche, attaque, touché, mort), construits par script Blender sans fenêtre ; la marche est l'animation jugée, les quatre autres sont des poses clés simples.",
  "Le **brawler** : sa texture de corps, peinte par le générateur d'images dans la pose du corps et projetée par script ; deux ou trois pièces d'équipement accrochées à un os ; l'export `.glb`.",
  "Un **deuxième personnage** (le scoundrel, corps différent) passé par les mêmes scripts, sans les modifier.",
  "`scripts/assetsGeneration/render_character_strips.py` : le modèle animé rendu par Blender au format des bandes actuelles (8 images, 4 orientations, cellule du standard), et ses tests ; `install_hd_asset.py` accepte une bande `placed`, déjà au format du moteur.",
  "Les 20 bandes du brawler ainsi rendues, installées dans `Common/Characters/Heroes/brawler/` ; le kit `Common` republié.",
  "Le **verdict** de l'auteur, écrit en fin de fiche : la version s'ouvre, ou s'abandonne.",
]
criteres = [
  "Le modèle du brawler se reconnaît à côté de son portrait (jugement de l'auteur).",
  "Dans le jeu, le brawler marche dans les quatre diagonales avec des appuis alternés, sans glissement, sans retouche à la main d'aucune image.",
  "Le deuxième personnage passe par la chaîne sans changer une ligne des scripts : seuls sa fiche, sa texture et ses pièces diffèrent.",
  "À 100 px par case, le brawler rendu tient à côté du décor actuel (jugement de l'auteur).",
  "Aucune bande générée du brawler ne reste, ni dans le kit ni dans le dépôt : `check_hd_assets.py` passe.",
]
+++

## Pourquoi

Tout le passage à la 3D repose sur une hypothèse que rien n'a encore prouvé dans ce projet : qu'un
personnage se fabrique à partir d'un **corps commun**, d'une **texture peinte** et de **pièces
d'équipement**, et qu'il reste reconnaissable. Si elle tient, le générateur d'images ne dessine
plus jamais un mouvement — la cause des marches refusées le 24 septembre. Si elle ne tient pas, il
vaut mieux le savoir avant d'avoir touché au moteur.

## Périmètre

Dedans : un corps, un squelette, la **marche** soignée, deux personnages, le rendu en bandes,
l'installation du brawler. Les quatre autres animations du moteur (repos, attaque, touché, mort)
sont posées aussi, simplement : sans elles le brawler n'aurait pas de figurine entière — le moteur
dessine un damier à la place d'une bande absente — et ses bandes générées ne pourraient pas partir
(décision de l'auteur, 30 septembre 2026).

Dehors, nommément :

- le moteur — pas une ligne de C++ ne change ; le modèle se juge **dans le jeu 2D**, par ses bandes
  rendues ;
- les sept autres corps, le sort (`cast`) et la reprise soignée des animations
  ([LOT-1006](LOT-1006-corps-de-reference.md)) ;
- le standard écrit ([LOT-1001](LOT-1001-standard-3d.md)), qui se rédige **d'après** ce que ce lot
  mesure, comme le `LOT-101` l'a fait d'après sa maquette.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Les 20 bandes **générées** du brawler et leurs `.anim.json` | `Source/Elements/Assets/Common/Characters/Heroes/brawler/` | les bandes rendues les remplacent, fichier pour fichier ; deux jeux de bandes pour un héros serait un asset mort |
| L'avis de refus des marches | `Planning/quality/brawler-walk-review.json`, et le dossier `Planning/quality/` s'il reste vide | il porte sur des fichiers qui n'existent plus |
| Les sources 2D du brawler | atelier local, `Tools/AssetHd/NPC/Classes/LOT-136-v1/brawler/` | à archiver hors du poste de travail par l'auteur |

Si la porte **ne s'ouvre pas**, rien n'est supprimé : les bandes rendues ne sont pas installées, le
lot passe `abandonne` avec sa raison, et la version avec lui.

## Conception

- **Blender est l'atelier, pas le générateur.** Il se pilote par script, sans fenêtre (vérifié le
  30 septembre : maillage, squelette, poids automatiques, animation, export `.glb`, Blender 5.2.2).
- **La forme vient du corps**, la même pour tous ; l'identité vient de la texture et des pièces.
  Cheveux et barbe sont des pièces, pas de la texture.
- **La texture** se commande au générateur d'images en deux vues, face et dos, dans la pose de
  liaison du corps ; un script la projette sur le dépliage du corps. Le générateur n'a qu'une image
  fixe à réussir.
- **Le corps vient de MPFB** (MakeHuman pour Blender, extension de `extensions.blender.org` ; code
  GPL, maillages humains CC0), choisi par l'auteur le 30 septembre : sexe, musculature et poids sont
  des curseurs d'**un seul** maillage, donc les huit corps du LOT-1006 partagent topologie, dépliage
  et squelette (`game_engine`, 53 os). Le script applique les formes et retire la géométrie d'aide ;
  le `.glb` n'emporte rien de MPFB.
- **Les poses se disent en cibles**, pas en angles d'os : une cheville à atteindre (deux os résolus
  analytiquement, genou vers l'avant), une main à placer, une direction où pointer l'arme. Les mêmes
  fonctions animent tous les corps du squelette.
- **La marche tient la règle du moteur** : un cycle couvre une case de 1,5 m en 0,5 s. Le pied posé
  recule exactement de cette vitesse (attaque du talon, déroulé, départ sur la plante) ; le bassin
  descend juste assez pour que les deux jambes atteignent leurs chevilles.
- **La caméra du rendu est celle du jeu** : orthographique, 45°, inclinée de asin 0,62 ≈ 38,3°, à
  120,7 px par mètre (une case de 1,5 m fait 256 px, un corps de 1,80 m 170 px — la hauteur du
  standard, sans réglage). Le pied tombe au milieu de la cellule sur la ligne de sol par
  construction ; la bande s'installe **`placed`**, copiée telle quelle, sans le recentrage sur la
  boîte que demandaient les images du générateur (il déplacerait une figurine qui porte une hache).
  Un pied posé devant ou à côté passe sous la ligne de sol à l'écran : la cellule s'allonge par pas
  de 8 px, la règle d'installation existante.
- **Le rendu en bandes** reprend exactement le format lu par le moteur : le jugement se fait donc à
  la vitesse du jeu, sur une carte, à côté du décor. Ce rendu reste un **repli** si la suite de la
  version glisse : il donne déjà la production stable, sans jour / nuit.

## Risques et questions ouvertes

- L'auteur a tranché que le corps commun, la texture et les pièces **suffisent** (D-31). Si le
  brawler ne se reconnaît pas, la décision se rouvre : le repli est un générateur image → maillage
  lié au même squelette par script. Aucun n'a été essayé ; c'est un autre lot, pas un prolongement de
  celui-ci.
- Les vêtements amples (la robe du mage) ne se peignent pas sur des jambes : ils demandent une
  pièce liée aux os des jambes. Ce lot ne le prouve pas — le brawler et le scoundrel n'en portent
  pas ; le [LOT-1009](LOT-1009-les-quatre-heros.md) le rencontrera.
