+++
id = "LOT-102"
titre = "Table rase : assets et cartes"
version = "0.0.1"
filiere = "standard"
statut = "livre"
taille = "L"
resume = "Plus un seul asset pixel art ni une seule carte de l'ancien style dans le dépôt, la nouvelle arborescence en place, le jeu et la CI debout sur une base vide."
prerequis = ["LOT-100", "LOT-123"]
reprend = ["LOT-88 (retrait de l'héritage, même méthode)"]
livrables = [
  "Suppression de `Assets/Scene/`, `Assets/Coliseum/`, `Assets/Npc/`, `Assets/Monsters/` (images), de `Levels/coliseum.json`, `Levels/capital/` et `World/arena/`.",
  "La [nouvelle arborescence](../../../../standards/arborescence-assets.md) créée, avec ses manifestes vides et un README par niveau.",
  "La ressource `JadgUiColiseum` (`Source/Ui/CMakeLists.txt`) retirée — plus aucun QML ne la lisait — et `World/cities/capital.json` supprimé plutôt qu'adapté : le format exige qu'un quartier de départ ait sa carte. `check_rpg_data.py` n'avait rien à changer ; les cartes peintes et le HUD ne sont pas touchés.",
  "Les tests qui nommaient un contenu livré : quatre supprimés, les onze autres rebranchés sur la racine d'essai (`Source/Test/Fixtures/GameData/`, élargie d'un kit d'arène et de figurines).",
  "`Documentation/CahierTest/` et les captures de référence QML régénérés.",
  "L'atelier pixel art retiré : `extract_texture_sheet.py`, `extract_coliseum_atlas.py`, les dispositions du LOT-92, le contrat de `LOT-CREATION-ASSETS`, et la fabrique de figurines `asset_factory.py`.",
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

**Reste** : `Assets/UI/` et `Assets/Fonts/` (le HUD et l'interface à la charte v2 sont au standard),
`Assets/Entities/families.json`, l'atlas (`World/regions/`, `World/locations/`), les dialogues, les
règles (`Rpg/`), les textes.

**Restent aussi, par décision de l'auteur (D-15)** : les seize cartes peintes de `Assets/Maps/`
et `Maps/world-maps.json` (monde, régions, villes), et tout le HUD — ils sont **déjà au standard**.
Conséquence heureuse : `check_map_assets.py`, `test_world_maps.cpp`, l'écran « Carte » et ses
captures de référence n'ont pas à bouger, et la garde CMake de `JadgUiMaps` devient inutile. Seul
`World/cities/capital.json` change : ses quartiers pointent sur des cartes jouables supprimées.

## Conception

L'ordre qui garde la CI verte à chaque commit :

1. poser les données de test et y rebrancher les tests (rien n'est encore supprimé) ;
2. adapter CMake et les scripts pour qu'une base vide soit un état **admis** ;
3. supprimer ; régénérer le cahier de tests et les captures ;
4. créer la nouvelle arborescence et y rebrancher les chemins du moteur
   (`WorldPlay`, `ArenaSceneRenderer`, `AssetGallery`, `EntityReferences`, `MapFormat`) :
   recherche d'une clé dans la zone, puis la ville, la région, le monde.

## L'éditeur

Le critère « l'éditeur s'ouvre sur une carte vierge ; `LevelEditor --check` passe » est faux
aujourd'hui : `--check` rend une erreur sans carte, et dix-huit tests de l'éditeur lisent les cartes
livrées. Le [LOT-123](LOT-123-editeur-sur-une-base-vide.md) passe avant, et la nouvelle arborescence
côté éditeur est au [LOT-124](LOT-124-editeur-et-arborescence-par-niveaux.md).

## Risques

- Le point 4 touche le moteur : s'il grossit, il se détache en lot à part, avant le LOT-104.
- Les mesures de performance nocturnes perdent deux séries (`bench_levels`, `bench_canvas`) jusqu'à
  la première carte HD.

## Ce que la réalisation a tranché

**D-102-1 — Le plan de la Capitale est supprimé, pas adapté.** La fiche le disait « adapté à une
base sans carte jouable » ; le format ne le permet pas — `core::loadCityPlan` exige que le quartier
de départ ait sa carte, et les douze quartiers de `capital.json` nommaient des cartes supprimées.
Rendre le format tolérant aurait affaibli une règle vraie pour un état provisoire. Le jeu dit déjà
sans planter qu'il n'a rien à ouvrir (`EX-NFR-040`), et le [LOT-121](LOT-121-plan-de-la-capitale.md)
redonne le plan sur les cartes 2D HD.

**D-102-2 — Deux noms de contenu quittent le moteur ici, la cascade reste au LOT-103.** Le point 4
de la conception prévoyait de rebrancher les chemins du moteur sur la nouvelle arborescence. Sans
carte ni pièce HD, une cascade de résolution n'aurait rien à résoudre et rien à prouver : elle part
au [LOT-103](LOT-103-rendu-hd.md), avec le reste du rendu. Deux hypothèses de contenu, en revanche,
**empêchaient** de faire tourner une arène d'essai et sont levées : le dossier de pièces d'une arène
vient de son manifeste (champ `scene`), et le décor derrière une zone de combat vient de la carte
que l'arène nomme — non plus de `../Scene/coliseum/` et `arena-of-the-future.json` écrits en dur.

**D-102-3 — La racine d'essai devient celle du jeu.** `Source/Test/Fixtures/EditorData` prend le nom
de `GameData` (`JADG_TEST_DATA_DIR`) : née pour l'éditeur au [LOT-123](LOT-123-editeur-sur-une-base-vide.md),
elle sert maintenant aux deux. Ce qu'un test y prouve n'est jamais un contenu, toujours un
mécanisme ; son README le dit, et dit aussi que son art n'a rien à prouver.

**D-102-4 — La fabrique de figurines part avec l'atelier.** `asset_factory.py` n'était pas dans la
liste de la fiche, mais ses prompts prescrivaient du « 16-bit pixel art » et elle écrivait dans
`Assets/Npc/`. La méthode reste dans les epics des `LOT-91`/`92`/`93`, que le
[LOT-104](LOT-104-chaine-de-production-hd.md) reprend nommément.

**Les exigences n'avaient rien à recevoir.** `EX-VIS-008`, `EX-VIS-009` et `EX-REN-013` ont été
réécrites au [LOT-101](LOT-101-standard-2d-hd.md), chacune avec sa note d'écart désignant le
`LOT-103` pour la mise en œuvre dans le moteur. Ce lot n'y touche donc pas : ce qui reste du pixel
art dans le dépôt est onze commentaires qui décrivent le code **tel qu'il est encore**.

---

Livré le 21 septembre 2026 par la [PR #106](https://github.com/azertval/JustAnotherRpgGame/pull/106).
