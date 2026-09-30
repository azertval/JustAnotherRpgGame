+++
id = "LOT-1006"
titre = "Les corps de référence"
version = "0.0.2.5"
filiere = "pnj"
statut = "a-faire"
taille = "L"
resume = "Huit corps humanoïdes, un squelette, six animations : le socle commun de tous les personnages, installé dans le jeu — et plus une seule bande de figurine."
prerequis = ["LOT-1005"]
livrables = [
  "`Common/Characters/Bodies/` : les **huit corps** — homme, femme × maigre, normal, musclé, gros —, liés au même squelette, au contrat du standard (LOT-1001).",
  "`Common/Characters/Skeletons/humanoid/` : le squelette et les **six animations** (repos, marche, attaque, sort, touché, mort), avec leurs images clés.",
  "Le **mannequin** : une fiche de personnage neutre par corps, qui tient la place de tout humanoïde sans fiche (`hmi::FigureResolver`, propriété `silhouette`).",
  "Les scripts Blender de l'atelier local qui construisent corps, squelette et animations, et leurs commandes de reconstruction.",
  "`check_hd_assets.py`, `install_hd_asset.py` et la galerie de débug connaissent corps, squelettes et fiches de personnage.",
  "Le kit `Common` republié et verrouillé.",
]
criteres = [
  "Les six animations, jouées par chacun des huit corps sur une carte d'essai, sont approuvées par l'auteur : appuis alternés à la marche, pas de glissement à 2 cases par seconde, pas de maillage qui se traverse.",
  "Une même texture de corps et une même pièce d'équipement se posent sur les huit corps sans retouche.",
  "Dans le jeu, tout personnage — les quatre héros compris — s'affiche par le mannequin de son corps, marche et combat ; le combat à quatre contre quatre de la `0.0.2` se joue de bout en bout.",
  "`git ls-files \"Source/Elements/Assets/**/Characters/**/*.anim.json\"` ne rend rien, et aucun kit publié ne contient de bande de figurine.",
  "`hmi::FigureResolver` ne connaît plus qu'une forme de figurine, la fiche de personnage ; aucun test ne charge de bande de figurine.",
]
+++

## Pourquoi

C'est le lot où la production de personnages devient stable : après lui, un personnage de plus ne
coûte qu'une texture et un choix de pièces. C'est aussi celui qui **retire l'ancien** — toutes les
bandes de figurine, d'un coup, pour qu'aucune ne traîne.

## Périmètre

Dedans : les huit corps, le squelette humanoïde, les six animations, le mannequin, le retrait des
bandes.

Dehors, nommément :

- les quatre héros en propre ([LOT-1009](LOT-1009-les-quatre-heros.md)) : entre ce lot et celui-là,
  ils s'affichent par le mannequin de leur corps et gardent leur **portrait** ;
- la bibliothèque de pièces d'équipement, qui se constitue à l'atelier
  ([LOT-1008](LOT-1008-atelier-des-assets-3d.md)) et avec les héros ;
- les silhouettes `quadruped` et `flying` : elles viendront avec leurs créatures.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Les **132 bandes de figurine** et leurs `.anim.json` : mannequin 2D (40), brawler (20), mage, priest, scoundrel (24 chacun) | `Source/Elements/Assets/Common/Characters/Placeholders/humanoid/`, `Heroes/*/` | les corps les remplacent ; portraits et jetons **restent** |
| La lecture d'une bande de figurine : cellule (`192 × 256`, `384 × 256`), ligne de sol, `frameHeight`, suffixes `-se` / `-sw` / `-ne` / `-nw` | `Source/HMI/Graphics/SceneTextureTraits.{h,cpp}`, `ScenePieces.h`, `WorldSceneComposer`, `Source/HMI/Game/FigureResolver` | plus rien ne s'affiche par elle |
| Les règles de bandes de figurine dans les outils | `scripts/checks/check_hd_assets.py`, `scripts/assetsGeneration/install_hd_asset.py`, `render_character_strips.py` (le rendu en bandes du LOT-1000) et leurs tests | le rendu en bandes n'était qu'un moyen de juger et un repli ; le repli n'a plus lieu d'être une fois les corps dans le jeu |
| Les bandes de figurine des données d'essai | `Source/Test/Fixtures/GameData/Assets/{Arena,Common,Monsters,Npc}/`, `Fixtures/LevelTree/Assets/` (celles de figurine parmi leurs 42 `.anim.json`, et leurs images) | remplacées par un corps d'essai ; les tests gardent leur objet |
| L'affichage des bandes de figurine dans la galerie | `Source/HMI/Graphics/AssetGallery.{h,cpp}` | la galerie montre des modèles |
| Les chapitres « figurines » du guide et du cahier de test | `Documentation/Guide/`, `Documentation/CahierTest/` (régénéré) | documentation d'un format retiré |
| La suite 2D du [LOT-145](../../v0.0.2-combat/lots/LOT-145-mannequins-de-remplacement.md) : mannequins quadrupède et volant en bandes | sa fiche | ils ne sont pas produits et ne le seront pas en 2D (décision D-34) |
| Les planches et consignes de figurines 2D | atelier local : `Tools/AssetHd/NPC/ManequinNpc/`, `Tools/AssetHd/NPC/Classes/LOT-136-v1/` | à archiver par l'auteur |

Ce qui **reste**, à dessein : les 24 bandes d'effets de `Common/Fx/`, `hmi::AnimationCatalog` qui
les lit, et les portraits et jetons peints.

## Conception

- **Huit corps, un contrat.** Même squelette, et — proposition du standard — même topologie et même
  dépliage : c'est ce qui fait qu'une texture et une pièce valent pour les huit. Le deuxième
  critère le vérifie.
- **Le corps d'un personnage est une donnée de sa fiche**, pas de ses règles : ni le sexe ni la
  corpulence ne touchent à `Core`.
- **Les tailles** (gnome, demi-orc) se font par mise à l'échelle du squelette, à éprouver ici sur un
  cas petit et un cas grand.

## Risques et questions ouvertes

- Six animations × huit corps : une animation juste sur le corps normal peut traverser le corps
  gros (bras dans le ventre). Le premier critère les passe toutes en revue.
- Entre ce lot et le LOT-1009 les héros sont des mannequins : c'est une régression visuelle
  assumée, bornée à deux lots, et la version ne se livre pas dans cet état.
