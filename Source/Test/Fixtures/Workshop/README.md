# Les données d'essai de l'atelier des assets

Les données d'essai du `LOT-1008` : de quoi installer un personnage par l'atelier, sans fenêtre,
et comparer ce qu'il écrit à des fichiers attendus (`test_character_workshop.cpp`).

| Fichier | Contenu |
|---|---|
| `garde.character.json` | la **fiche d'atelier** du garde du bourg : son modèle et son squelette (ceux de [`Fixtures/Characters`](../Characters/README.md)), son portrait, son jeton, et les fichiers de l'aller-retour par Blender |
| `portrait.png`, `token.png` | un portrait de 512 × 512 et un jeton de 128 × 128 : des aplats, aux tailles du standard |
| `petit.png` | une image de 64 × 64 : ce que l'atelier **refuse** comme portrait — il ne retaille rien |
| `base/` | la racine de données de départ : le manifeste du commun, et celui du bourg, encore vide — le garde n'y est pas |
| `attendu/` | ce que l'installation écrit de lisible : les deux manifestes et la fiche `character.json` du garde |

Les fichiers de `attendu/` ne se retouchent pas à la main : ils sortent de la commande, rejouée sur
une copie de `base/` —

    cp -r Source/Test/Fixtures/Workshop/base /tmp/atelier
    LevelEditor --data /tmp/atelier --apply Source/Test/Fixtures/Workshop/garde.character.json

puis recopiés (`manifest.json` des deux niveaux, `garde/character.json`). Le modèle, le portrait
et le jeton installés sont comparés à leurs sources, octet pour octet.
