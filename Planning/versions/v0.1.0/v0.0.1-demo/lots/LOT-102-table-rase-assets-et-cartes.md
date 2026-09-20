+++
id = "LOT-102"
titre = "Table rase : assets et cartes"
version = "0.0.1"
filiere = "standard"
statut = "a-faire"
taille = "L"
resume = "Plus un seul asset pixel art ni une seule carte de l'ancien style dans le dépôt, la nouvelle arborescence en place, le jeu et la CI debout sur une base vide."
prerequis = ["LOT-100"]
reprend = ["LOT-88 (retrait de l'héritage, même méthode)"]
livrables = [
  "Suppression de `Assets/Scene/`, `Assets/Coliseum/`, `Assets/Npc/`, `Assets/Monsters/` (images), de `Levels/coliseum.json`, `Levels/capital/` et `World/arena/`.",
  "La [nouvelle arborescence](../../../../standards/arborescence-assets.md) créée, avec ses manifestes vides et un README par niveau.",
  "Les gardes CMake (`Source/Ui/CMakeLists.txt`), `check_map_assets.py`, `check_rpg_data.py` et `World/cities/capital.json` adaptés à une base sans carte.",
  "Les tests qui nommaient un contenu livré : cinq supprimés, les autres rebranchés sur des **données de test** (`Source/Test/Fixtures/`).",
  "`Documentation/CahierTest.md` et les captures de référence QML régénérés.",
  "L'atelier pixel art retiré : `extract_texture_sheet.py`, `extract_coliseum_atlas.py`, les dispositions du LOT-92, le contrat de `LOT-CREATION-ASSETS`.",
]
criteres = [
  "La CI entière est verte sur une base sans aucun asset de scène ni aucune carte.",
  "Le jeu démarre, affiche son menu et ses écrans ; l'éditeur s'ouvre sur une carte vierge ; `LevelEditor --check` passe.",
  "`git grep -i \"pixel art\"` ne trouve plus que de l'histoire (dossiers de lots livrés, CHANGELOG).",
  "Aucun test n'a été supprimé sans que son mécanisme soit couvert ailleurs — la liste est dans la PR.",
]
+++

## Pourquoi

On repart sur des bases propres : deux styles ne cohabitent pas, même provisoirement. Mais
l'[audit](../../../../standards/audit-passage-hd.md) montre que ce n'est pas un `git rm` : trois règles CMake,
quatre scripts de contrôle et une trentaine de tests lisent les fichiers supprimés. C'est un lot,
avec sa PR et sa recette.

## Périmètre

**Part** : tout l'art de scène, toutes les figurines, toutes les cartes jouables, les arènes de
`World/arena/`, l'atelier pixel art et ses scripts.

**Reste** : `Assets/UI/` et `Assets/Fonts/` (l'interface à la charte v2 n'est pas du pixel art),
`Assets/Entities/families.json`, l'atlas (`World/regions/`, `World/locations/`), les dialogues, les
règles (`Rpg/`), les textes.

**À trancher par l'auteur** : les seize cartes peintes de `Assets/Maps/`. Proposé : garder le
monde et les treize régions, qui ne sont pas du pixel art et servent toutes les versions ; refaire
les plans de ville avec leurs quartiers (lots de cartes).

## Conception

L'ordre qui garde la CI verte à chaque commit :

1. poser les données de test et y rebrancher les tests (rien n'est encore supprimé) ;
2. adapter CMake et les scripts pour qu'une base vide soit un état **admis** ;
3. supprimer ; régénérer le cahier de tests et les captures ;
4. créer la nouvelle arborescence et y rebrancher les chemins du moteur
   (`WorldPlay`, `ArenaSceneRenderer`, `AssetGallery`, `EntityReferences`, `MapFormat`) :
   recherche d'une clé dans la zone, puis la ville, la région, le monde.

## Risques

- Le point 4 touche le moteur : s'il grossit, il se détache en lot à part, avant le LOT-104.
- Les mesures de performance nocturnes perdent deux séries (`bench_levels`, `bench_canvas`) jusqu'à
  la première carte HD.
