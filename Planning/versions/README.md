# Versions

Une version par dossier. Le catalogue est [`versions.toml`](versions.toml) ; il donne l'ordre, la
nature et le périmètre de chacune. Une version **détaillée** a son dossier, avec :

- `README.md` — ce que la version a de particulier (facultatif) ;
- `lots/` — une fiche par lot, au [gabarit](../standards/gabarit-lot.md) ;
- `maquettes/` — plans de principe, maquettes d'écran, schémas de quête.

| Dossier | Versions |
|---|---|
| `v0.1.0/` | le référentiel **Empire central** : `0.0.1` (démo), `0.0.2` (combat), `0.0.3` à `0.0.9` (zones), `0.1.0` (recette) |
| `v0.2.0/` | la compagnie et les dernières fonctionnalités |
| `v0.3.0/` | classes et espèces |

Les versions **prévisionnelles** (`0.4.0` à `1.0.0`) n'ont pas de dossier : une ligne du catalogue
leur suffit tant qu'elles sont loin. Les numéros de lots vont par centaine : `LOT-1xx` pour le
référentiel `0.1.0`, `LOT-2xx` pour la `0.2.0`, `LOT-3xx` pour la `0.3.0`.

À lire d'abord : [la quête de la démo](v0.1.0/v0.0.1-demo/quete-demo.md).
