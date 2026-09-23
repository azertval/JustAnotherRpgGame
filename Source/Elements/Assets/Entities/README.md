# Illustrations d'entité

Les images des **entités** du jeu — créatures, objets, armes, armures, espèces, historiques,
classes. Le `LOT-08` a fixé le vocabulaire des tuiles de terrain ; celui-ci fixe celui des entités
(`LOT-39`).

## Une clé, jamais un chemin

Une donnée désigne son illustration par une **clé d'asset** — `beast/wolf` — et jamais par un
chemin de fichier (`EX-CNT-040`). Un chemin dans une donnée de règle lierait le catalogue à
l'arborescence du disque, et tout déplacement de dossier casserait des créatures.

La clé se **déduit** de la famille et de l'identifiant : une créature `wolf.json` attend
`beast/wolf` sans rien écrire. Le champ `asset` d'une donnée sert à **déroger** — deux entrées qui
partagent une illustration —, pas à répéter ce qui se déduit.

## Où vivent les images

    Assets/Entities/<famille>/<identifiant>.png

`beast/wolf` se sert donc dans `Entities/beast/wolf.png`. `families.json` déclare les familles, les
dimensions attendues et les dossiers de catalogue qui les alimentent — les dimensions vivent là et
non dans le C++ (`EX-VIS-007`).

## Ce qui se passe tant qu'une image manque

Rien de grave, et c'est le point : la clé reçoit un **marqueur généré** (`core::assetMarker`),
déterministe — la même clé donne toujours le même marqueur, barré de deux diagonales pour qu'on ne
le prenne jamais pour une illustration définitive. Le jeu tourne donc **complet** avant qu'aucune
illustration n'existe (`EX-CNT-041`), et la production graphique devient un remplacement progressif
plutôt qu'un préalable bloquant.

`scripts/checks/check_asset_keys.py` **échoue** sur une clé orpheline, et se contente de **lister** les
clés encore servies par un marqueur : c'est un état d'avancement, pas un défaut.
