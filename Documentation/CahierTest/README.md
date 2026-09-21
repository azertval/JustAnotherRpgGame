# Cahier de test

**900 cas de test**, un par test automatisé du dépôt. Le cahier est **engendré** depuis les blocs `\castest{…}` écrits au-dessus de chaque test par `scripts/generate_cahier_test.py` : il ne s'édite pas — on corrige le commentaire du test, puis on relance le script. La CI refuse un cahier périmé, et refuse un test sans bloc.

## Lire une fiche

Chaque cas porte l'**identifiant GoogleTest** (`Suite.Nom`, retrouvable tel quel dans le code et dans le rapport `ctest`), sa **criticité**, sa **catégorie**, son **emplacement** (`fichier:ligne`), son objet en une phrase, ses **étapes**, et le **résultat attendu** — les assertions réellement vérifiées par le test, traduites en français.

| Criticité | Ce qu'un échec signifie |
|---|---|
| **Bloquant** | Le jeu ne démarre pas, corrompt une donnée ou fausse une règle : rien ne se livre. |
| **Critique** | Une fonction centrale rend un résultat faux ; la version ne sort pas en l'état. |
| **Majeur** | Un comportement attendu manque ou dévie, avec contournement possible. |
| **Mineur** | Un confort, un message, une valeur par défaut. |

## Synthèse par domaine

| Domaine | Type | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|---|
| [Core](core.md) | Tests unitaires | 1 | — | — | 1 | — |
| [Core · Combat](core-combat.md) | Tests unitaires | 112 | 33 | 48 | 30 | 1 |
| [Core · Data](core-data.md) | Tests unitaires | 12 | — | 5 | 5 | 2 |
| [Core · Diagnostics](core-diagnostics.md) | Tests unitaires | 22 | — | — | 19 | 3 |
| [Core · Ecs](core-ecs.md) | Tests unitaires | 35 | — | 5 | 30 | — |
| [Core · Gameplay](core-gameplay.md) | Tests unitaires | 9 | — | 5 | 3 | 1 |
| [Core · Levels](core-levels.md) | Tests unitaires | 117 | — | 24 | 76 | 17 |
| [Core · Math](core-math.md) | Tests unitaires | 26 | 4 | — | 18 | 4 |
| [Core · Resources](core-resources.md) | Tests unitaires | 12 | 2 | 6 | 4 | — |
| [Core · Rpg](core-rpg.md) | Tests unitaires | 81 | 3 | 44 | 33 | 1 |
| [Core · Time](core-time.md) | Tests unitaires | 7 | — | 1 | 6 | — |
| [Core · World](core-world.md) | Tests unitaires | 53 | 1 | 26 | 23 | 3 |
| [Editor](editor.md) | Tests unitaires | 175 | 14 | 43 | 91 | 27 |
| [HMI · Audio](hmi-audio.md) | Tests unitaires | 3 | — | 1 | 1 | 1 |
| [HMI · Game](hmi-game.md) | Tests unitaires | 6 | — | 1 | 3 | 2 |
| [HMI · Graphics](hmi-graphics.md) | Tests unitaires | 138 | 18 | 48 | 65 | 7 |
| [HMI · Input](hmi-input.md) | Tests unitaires | 25 | 1 | 3 | 19 | 2 |
| [HMI · Interface](hmi-interface.md) | Tests unitaires | 29 | 2 | 10 | 16 | 1 |
| [HMI · Localization](hmi-localization.md) | Tests unitaires | 9 | — | — | 9 | — |
| [HMI · Platform](hmi-platform.md) | Tests unitaires | 5 | — | 2 | 3 | — |
| [HMI · Presentation](hmi-presentation.md) | Tests unitaires | 17 | — | 4 | 10 | 3 |
| [HMI · Runtime](hmi-runtime.md) | Tests unitaires | 3 | — | 1 | 2 | — |
| [Tests d'intégration](integration.md) | Tests d'intégration | 2 | — | 1 | 1 | — |
| [Tests système](systeme.md) | Tests système | 1 | — | 1 | — | — |
| **Total** | | **900** | **78** | **279** | **468** | **75** |

## Lancer les tests

```
powershell -File scripts/build.ps1      # compile (préréglage ninja)
ctest --preset ninja                     # exécute tous les cas
ctest --preset ninja -R AttackTest       # une suite
```

Un cas qui échoue se retrouve ici par son identifiant (la recherche du site le trouve), et dans le code par l'emplacement que donne sa fiche.
