# Le modèle d'essai animé

Les données d'essai du `LOT-1005` : un **vrai** modèle de personnage, lié au squelette humanoïde
commun, pour que le moteur anime autre chose qu'un pantin de trois os.

| Fichier | Contenu |
|---|---|
| `Assets/Common/Characters/Mannequins/humanoid/humanoid.glb` | le mannequin humanoïde de l'atelier, réduit à 4 000 triangles et à une texture de 256 px (0,6 Mio), lié aux 53 os, avec ses six clips |
| `Assets/Common/Characters/Skeletons/humanoid/skeleton.json` | la description du squelette : les 53 os, et par clip sa durée, sa boucle et son image clé |

**Rien ne s'y retouche à la main.** Les deux fichiers sortent de la chaîne de liaison, depuis le
mannequin reçu de Meshy et sa fiche de liaison, qui sont dans l'atelier local (`Tools/`, jamais
livré) :

    py -3.13 scripts/assetsGeneration/rig_character.py \
        Tools/Assets3D/Standard/Personnages/mannequin/Meshy_AI_Chromatic_Sentinel_1002134639_texture.glb \
        Source/Test/Fixtures/Characters/Assets/Common/Characters/Mannequins/humanoid/humanoid.glb \
        --sheet Tools/Assets3D/Personnages/mannequin/liaison.json \
        --skeleton Source/Test/Fixtures/Characters/Assets/Common/Characters/Skeletons/humanoid/skeleton.json \
        --triangles 4000 --texture 256 --no-report

La fiche du `LOT-1005` demandait un modèle tiré du brawler de la preuve (`LOT-1000`) ; ses fichiers
n'étaient plus dans l'atelier le 2 octobre 2026, et le mannequin, dont chaque membre porte sa
teinte, laisse lire un défaut de déformation à la couleur.

Qui les lit : `test_skeleton.cpp` (les 53 os, les six clips, le contact au sol),
`test_figure_model_render.cpp` (le mannequin marche sur la carte d'essai, hors écran), et les
mesures `ArenareaFrame1080pEightModels` (`Benchmarks`) et `WorldFrame…Models1080p`
(`CanvasBenchmarks`). Le pantin de trois os, dont une pose se calcule de tête, est dans
[`Fixtures/Meshes`](../Meshes/README.md).
