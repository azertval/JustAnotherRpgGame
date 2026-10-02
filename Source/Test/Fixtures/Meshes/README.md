# La carte d'essai en maillages

Les données d'essai du `LOT-1003` : de quoi faire dessiner des **volumes** au moteur avant que le
kit de la Capitale ne soit modelé (`LOT-1004`). Une racine de données à la forme de
`Source/Elements` — `Assets/`, `Levels/` —, que les tests lisent et que l'éditeur ouvre :

    LevelEditor --data Source/Test/Fixtures/Meshes --map=ilot
    LevelEditor --render --data Source/Test/Fixtures/Meshes --output ilot.png ilot

**Rien ne s'y retouche à la main.** Tout est écrit par
`scripts/assetsGeneration/build_mesh_fixture.py`, sans bibliothèque ni compression, pour que deux
postes produisent les mêmes octets ; `--check` vérifie que les fichiers sont à jour.

| Fichier | Contenu |
|---|---|
| `Assets/Scene/ilot/floor.glb` | une dalle d'une case, à plat : 2 triangles |
| `Assets/Scene/ilot/wall.glb` | un bloc de mur d'une case, haut d'un étage (2,366 m) : 10 triangles |
| `Assets/Scene/ilot/roof.glb` | un toit à quatre pans sur trois cases de côté : 4 triangles |
| `Assets/Scene/ilot/paving.png` | la dalle de sol **en image**, pour qu'une même carte mêle les deux formes |
| `Assets/Scene/ilot/manifest.json` | le lieu : trois clés citent un maillage (`"mesh"`), une une image (`"file"`) |
| `Assets/Npc/temoin/` | la figurine témoin, 1,80 m, d'une teinte que rien d'autre ne porte |
| `Assets/Npc/pantin/` | le **pantin** (`LOT-1005`) : un modèle de 1,80 m, trois blocs verts liés à trois os (`Root`, `spine_01`, `head`), six clips de deux à quatre clés, et sa fiche `character.json` |
| `Assets/Common/Characters/Skeletons/pantin/skeleton.json` | la description du squelette du pantin : os, durée, boucle et image clé de chaque clip |
| `Levels/ilot.json` | dix cases sur huit : une cour dallée de maillages, un anneau de huit murs, son toit à l'étage, deux PNJ |

Les maillages sont au [standard 3D](../../../../Planning/standards/style-3d.md) : le mètre, la
hauteur vers +Y, l'origine au sol sous le centre de l'emprise, un maillage, une primitive, une
matière, sa texture incorporée ; **+X suit les colonnes** de la grille, **+Z ses lignes**. Le losange
de l'art du lieu est petit (64 × 40) : c'est une fixture, aucune de ces pièces n'entre dans le jeu.

Qui les lit : `test_mesh_file.cpp` (le chargeur), `test_mesh_composition.cpp` (la composition, sans
GPU), `test_mesh_render.cpp` (la passe et le tampon de profondeur, hors écran) et la cible de
fuzzing `fuzz_mesh`, qui s'amorce sur les trois `.glb`.
