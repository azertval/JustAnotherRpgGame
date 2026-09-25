# Source/Elements/

**Assets et éléments statiques** du jeu.

## Périmètre
- `Levels/` — cartes (`.json`), copiées à côté de l'exécutable au build.
- `Localization/` — catalogues de traduction (`.lang`, `.ts`), copiés à côté de l'exécutable au build.
- `Assets/` — images et polices (scènes, PNJ, cartes peintes, illustrations, polices), copiées à
  côté de l'exécutable au build (`LOT-39`) ; voir `Assets/README.md`.
- `Rpg/` — catalogues du jeu de rôle (espèces, classes, historiques, compétences, créatures,
  équipement, règles et leurs schémas), copiés à côté de l'exécutable au build.
- `World/` — atlas du monde (régions, villes, lieux), plans de ville, dialogues et quêtes, copiés
  au build.
- `Editor/` — ce que l'éditeur de niveaux produit et réutilise : les préfabriqués d'un lieu
  (`Prefabs/`) et les modèles de carte (`Templates/`).
- `Maps/` — `world-maps.json`, positions relevées sur les cartes peintes (`LOT-94`), copié au build.
- `Credits/` — `credits.json`, les crédits affichés par le jeu.

Ces éléments sont consommés par `../Core/` (données de carte et de jeu) et `../HMI/` (rendu, UI Qt).

> Note : les images des kits d'assets ne vivent pas dans Git. Elles sont publiées à part et
> récupérées par `scripts/fetch_assets.py` d'après `Assets/kits.lock.json` ; le dépôt ne garde que
> les manifestes (voir `Assets/README.md`).
