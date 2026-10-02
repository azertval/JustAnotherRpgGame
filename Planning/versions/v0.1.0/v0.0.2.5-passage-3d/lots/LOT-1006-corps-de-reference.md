+++
id = "LOT-1006"
titre = "Le squelette commun et le mannequin"
version = "0.0.2.5"
filiere = "pnj"
statut = "a-faire"
taille = "L"
resume = "Un squelette, six animations soignées, un mannequin et le premier personnage produit au standard : le socle commun de tous les personnages, installé dans le jeu — et plus une seule bande de figurine."
prerequis = ["LOT-1005"]
livrables = [
  "`Common/Characters/Skeletons/humanoid/` : le squelette de 53 os et les **six animations** (repos, marche, attaque, sort, touché, mort), avec leurs images clés — les quatre poses simples de la preuve reprises, le sort posé.",
  "Le **mannequin** : un maillage neutre lié au squelette, qui tient la place de tout humanoïde sans modèle (`hmi::FigureResolver`, propriété `silhouette`).",
  "Le **premier personnage au standard** : généré depuis une vue de face en pose neutre, mains vides ([D-39](../../../../vision/decisions.md)), lié par sa seule fiche de liaison.",
  "Le [standard des personnages](../../../../standards/personnages-3d.md) corrigé d'après ce personnage : sa section « non mesuré » (l'image de référence) et ses constats ouverts deviennent des mesures, ou sont réécrits.",
  "Les scripts Blender de l'atelier local qui lient un maillage au squelette et posent les animations, et leurs commandes de reconstruction.",
  "`check_hd_assets.py`, `install_hd_asset.py` et la galerie de débug connaissent squelettes et modèles de personnage ; les contrôles de l'export du standard (structure, poids, contact, glissement) sont dans `scripts/`.",
  "Le kit `Common` republié et verrouillé.",
]
criteres = [
  "Les six animations, jouées par le mannequin et par le personnage au standard sur une carte d'essai, sont approuvées par l'auteur : appuis alternés à la marche, pas de glissement à 2 cases par seconde, pas de maillage qui se traverse.",
  "Le personnage généré depuis la vue de face a le buste droit et un visage lisible à 100 px par case (jugement de l'auteur) ; il n'a demandé que son image de référence et sa fiche de liaison, sans retouche d'un maillage, d'un poids ni d'une animation.",
  "Dans le jeu, tout personnage s'affiche par son modèle ou par le mannequin, marche et combat ; le combat à quatre contre quatre de la `0.0.2` se joue de bout en bout.",
  "`git ls-files \"Source/Elements/Assets/**/Characters/**/*.anim.json\"` ne rend rien, et aucun kit publié ne contient de bande de figurine.",
  "`hmi::FigureResolver` ne connaît plus qu'une forme de figurine, le modèle ; aucun test ne charge de bande de figurine.",
]
+++

## Pourquoi

C'est le lot où la production de personnages devient stable : après lui, un personnage de plus
coûte une image de référence, une génération et une fiche de liaison — les animations sont déjà là.
C'est aussi celui qui **retire l'ancien** — toutes les bandes de figurine, d'un coup, pour qu'aucune
ne traîne.

> **Réécrit au `LOT-1001`** (1er octobre 2026). La fiche commandait **huit corps** communs
> (D-31) ; l'auteur les a refusés à la preuve et a tranché pour **un maillage par personnage**
> ([D-38](../../../../vision/decisions.md)). Ce qui reste commun, et que ce lot livre, est le
> squelette, les animations et le mannequin. Le nom du fichier garde l'ancien titre : des fiches
> livrées le citent.

## Périmètre

Dedans : le squelette humanoïde, les six animations, le mannequin, un personnage produit selon le
standard pour en éprouver la règle non mesurée — l'image de référence —, le retrait des bandes.

Dehors, nommément :

- les quatre héros ([LOT-1009](LOT-1009-les-quatre-heros.md)) : entre ce lot et celui-là, le brawler
  s'affiche par son modèle de la preuve, les trois autres par le mannequin, et tous gardent leur
  **portrait** ;
- l'outil qui écrit les fiches ([LOT-1008](LOT-1008-atelier-des-assets-3d.md)) ;
- les silhouettes `quadruped` et `flying` : elles viendront avec leurs créatures.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Les **132 bandes de figurine** et leurs `.anim.json` : mannequin 2D (40), brawler (20), mage, priest, scoundrel (24 chacun) | `Source/Elements/Assets/Common/Characters/Placeholders/humanoid/`, `Heroes/*/` | les modèles les remplacent ; portraits et jetons **restent** |
| La lecture d'une bande de figurine : cellule (`192 × 256`, `384 × 256`), ligne de sol, `frameHeight`, suffixes `-se` / `-sw` / `-ne` / `-nw` | `Source/HMI/Graphics/SceneTextureTraits.{h,cpp}`, `ScenePieces.h`, `WorldSceneComposer`, `Source/HMI/Game/FigureResolver` | plus rien ne s'affiche par elle |
| Les règles de bandes de figurine dans les outils | `scripts/checks/check_hd_assets.py`, `scripts/assetsGeneration/install_hd_asset.py`, `render_character_strips.py` (le rendu en bandes du LOT-1000) et leurs tests | le rendu en bandes n'était qu'un moyen de juger et un repli ; le repli n'a plus lieu d'être une fois les modèles dans le jeu |
| Les bandes de figurine des données d'essai | `Source/Test/Fixtures/GameData/Assets/{Arena,Common,Monsters,Npc}/`, `Fixtures/LevelTree/Assets/` (celles de figurine parmi leurs 42 `.anim.json`, et leurs images) | remplacées par un modèle d'essai ; les tests gardent leur objet |
| L'affichage des bandes de figurine dans la galerie | `Source/HMI/Graphics/AssetGallery.{h,cpp}` | la galerie montre des modèles |
| L'exigence des bandes de figurine | `EX-REN-012`, dans `Documentation/Specification/rendu-technique.md` → `exigences-retirees.md` | le moteur anime le modèle lui-même (`EX-VIS-008`) |
| Les chapitres « figurines » du guide et du cahier de test | `Documentation/Guide/`, `Documentation/CahierTest/` (régénéré) | documentation d'un format retiré |
| La suite 2D du [LOT-145](../../v0.0.2-combat/lots/LOT-145-mannequins-de-remplacement.md) : mannequins quadrupède et volant en bandes | sa fiche | ils ne sont pas produits et ne le seront pas en 2D (décision D-34) |
| Les corps communs refusés et les essais de reconstruction locale | atelier local : `Tools/Assets3D/Characters/`, `Tools/Assets3D/Versions/`, `Tools/Assets3D/TripoSR-test/` | à archiver par l'auteur : aucun n'est une source du jeu |
| Les planches et consignes de figurines 2D | atelier local : `Tools/AssetHd/NPC/ManequinNpc/`, `Tools/AssetHd/NPC/Classes/LOT-136-v1/` | à archiver par l'auteur — **après** la génération des héros, dont elles sont les références |

Ce qui **reste**, à dessein : les 24 bandes d'effets de `Common/Fx/`, `hmi::AnimationCatalog` qui
les lit, et les portraits et jetons peints.

## Conception

- **Un squelette, des articulations par personnage.** Le squelette est commun ; sa position dans
  chaque maillage est dans la fiche de liaison. Un personnage petit ou grand n'a pas d'autre
  squelette : à éprouver ici sur un cas petit, dont la foulée doit tout de même couvrir une case en
  une demi-seconde.
- **La pose neutre est la pose de liaison.** C'est ce que la règle de l'image de référence apporte
  à ce lot : des bras écartés du corps se pèsent sans les masques d'équipement que la preuve a dû
  régler à la main.
- **Le modèle d'un personnage est une donnée de sa fiche**, pas de ses règles : rien de ce lot ne
  touche à `Core`.

## Risques et questions ouvertes

- **La pose neutre n'a jamais été générée.** Si Meshy rend un personnage moins fidèle depuis une
  vue de face que depuis sa figurine, la décision D-39 se rouvre devant l'auteur, images à l'appui.
- **Des clips posés mains vides** : l'attaque est un geste sans lame. Si l'auteur décide plus
  tard d'accrocher une arme à une main (D-42), les clips d'attaque se relisent arme par arme.
- Entre ce lot et le LOT-1009, trois héros sont des mannequins : c'est une régression visuelle
  assumée, bornée à deux lots, et la version ne se livre pas dans cet état.
