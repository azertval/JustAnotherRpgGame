# La maquette du standard 2D HD, en données d'essai

La maquette du `LOT-101` — huit cases sur huit d'Arenarea, à l'échelle du standard — installée
comme une zone livrée : une image par pièce, un manifeste qui déclare le losange de l'art
(`"tile": [256, 159]`), l'emprise et l'ancre de chaque pièce, et la disposition de la scène.

Le test `test_hd_mockup_render.cpp` en fait une carte, la rend par le moteur à 1080p et à 2160p,
et compare les deux images aux références sans texte de
`Planning/versions/v0.1.0/v0.0.1-demo/maquettes/` (`LOT-103`).

**Rien ne s'y retouche à la main.** Tout est écrit par `scripts/build_hd_mockup.py` depuis la
planche de référence (`Tools/AssetsHD/`, non versionnée), du même geste que les maquettes ;
`--fixture` n'écrit que ces données, `--check` vérifie qu'elles sont à jour.

| Fichier | Contenu |
|---|---|
| `scene.json` | la carte : taille, légende des sols, pièces et leur case, point suivi, fond, les deux vues et leur référence |
| `Scene/arenarea-maquette/` | deux dalles de 256 × 159 et six pièces, avec leur `manifest.json` |

Ces pièces sont **agrandies** de la planche (facteur 1,88) : elles jugent le rendu, jamais la
finesse d'un asset — aucune n'entre dans le jeu.
