# Crédits graphiques

Le premier essai de la Fabrique d’assets (19 septembre 2026) est conservé séparément dans
`Tools/AssetFactory` (hors dépôt). Ses candidats ne sont pas
validés ni intégrés ici ; la provenance des PNJ livrés reste celle de LOT-91.

Aucune image de `Source/Elements/Assets/` ne provient d'un pack tiers : toutes sont propres au
projet. Leur provenance, famille par famille :

| Dossier | Provenance | Trace |
|---|---|---|
| `Maps/` | cartes **peintes par l'auteur**, sans lettrage (`LOT-94`) | `"provenance": "author"` dans `Maps/manifest.json` |
| `Coliseum/` | découpées par `scripts/extract_coliseum_atlas.py` dans une planche de production sortie d'un générateur d'images (`LOT-50`) | `Coliseum/README.md`, `Coliseum/manifest.json` |
| `Scene/<lieu>/` | planches de l'atelier des textures, générateur d'images d'OpenAI, envois à la main, découpées par `scripts/extract_texture_sheet.py` (`LOT-92`) | `Scene/<lieu>/manifest.json` |
| `Npc/` | bandes et portraits de l'atelier des PNJ, même générateur (`LOT-91`) | `Npc/README.md`, `Npc/manifest.json` |
| `UI/` | illustrations de l'interface, chacune avec sa provenance | `UI/illustrations.json` |

Aucune image n'est tirée des livres du corpus source : `UI/illustrations.json` refuse cette
provenance (voir `THIRD-PARTY-NOTICES.md`, à la racine du dépôt).

Les polices tierces de `Fonts/` sont sous SIL Open Font License 1.1 ; chaque `*-LICENSE.txt`
accompagne sa famille (voir `Fonts/README.md`).
