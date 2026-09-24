# Cahier de test

**1018 cas de test**, un par test automatisé du dépôt. Le cahier est **engendré** depuis les blocs `\castest{…}` écrits au-dessus de chaque test par `scripts/docs/generate_cahier_test.py` : il ne s'édite pas — on corrige le commentaire du test, puis on relance le script. La CI refuse un cahier périmé, et refuse un test sans bloc. Seule la [recette manuelle](recette-manuelle.md) s'écrit à la main.

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
| [Core · Gameplay](core-gameplay.md) | Tests unitaires | 17 | — | 10 | 6 | 1 |
| [Core · Levels](core-levels.md) | Tests unitaires | 122 | 1 | 24 | 80 | 17 |
| [Core · Math](core-math.md) | Tests unitaires | 26 | 4 | — | 18 | 4 |
| [Core · Resources](core-resources.md) | Tests unitaires | 20 | 5 | 6 | 9 | — |
| [Core · Rpg](core-rpg.md) | Tests unitaires | 82 | 3 | 45 | 33 | 1 |
| [Core · Time](core-time.md) | Tests unitaires | 7 | — | 1 | 6 | — |
| [Core · World](core-world.md) | Tests unitaires | 63 | 1 | 32 | 27 | 3 |
| [Editor](editor.md) | Tests unitaires | 210 | 25 | 46 | 111 | 28 |
| [HMI · Audio](hmi-audio.md) | Tests unitaires | 3 | — | 1 | 1 | 1 |
| [HMI · Game](hmi-game.md) | Tests unitaires | 6 | — | 1 | 3 | 2 |
| [HMI · Graphics](hmi-graphics.md) | Tests unitaires | 186 | 36 | 59 | 83 | 8 |
| [HMI · Input](hmi-input.md) | Tests unitaires | 25 | 1 | 3 | 19 | 2 |
| [HMI · Interface](hmi-interface.md) | Tests unitaires | 29 | 2 | 10 | 16 | 1 |
| [HMI · Localization](hmi-localization.md) | Tests unitaires | 9 | — | — | 9 | — |
| [HMI · Platform](hmi-platform.md) | Tests unitaires | 5 | — | 2 | 3 | — |
| [HMI · Presentation](hmi-presentation.md) | Tests unitaires | 18 | — | 5 | 10 | 3 |
| [HMI · Runtime](hmi-runtime.md) | Tests unitaires | 3 | — | 1 | 2 | — |
| [Tests d'intégration](integration.md) | Tests d'intégration | 4 | — | 3 | 1 | — |
| [Tests système](systeme.md) | Tests système | 1 | — | 1 | — | — |
| **Total** | | **1018** | **111** | **308** | **522** | **77** |

## Trois étages de vérification

Le dépôt vérifie à trois hauteurs, et le cahier range chaque cas à la sienne : la colonne *Type* de la synthèse vient du dossier du test (`Source/Test/Unit`, `Integration`, `Systeme`).

| Étage | Ce qu'il prouve | Ce qu'il ne prouve pas | Où |
|---|---|---|---|
| **Unitaire** | Une fonction ou une classe tient son contrat, seule, sans fenêtre ni GPU (`EX-NFR-010`) : un jet, une grille, un chargeur, une vue-modèle. | Que les pièces s'assemblent. | `Source/Test/Unit/<Core, HMI, Editor>/<domaine>/` |
| **Intégration** | Plusieurs modules jouent ensemble sur des données réelles du dépôt : une carte livrée se charge, se compose et se parcourt. | Que l'exécutable démarre. | `Source/Test/Integration/` |
| **Système** | Un parcours **de bout en bout**, tel qu'un utilisateur le ferait — l'auteur dessine une carte dans l'éditeur, puis le jeu la joue —, sans fenêtre. | Le ressenti : fluidité, lisibilité, plaisir. | `Source/Test/Systeme/` ; l'archive publiée a son test de fumée (`scripts/release/smoke_test_release.ps1`) et la [recette manuelle](recette-manuelle.md) le reste |

Les écrans Qt Quick ont en plus leurs tests de référence (`Source/Test/Qml/`, images comparées pixel à pixel) et les scripts Python les leurs (`scripts/tests/`, pytest) ; ni les uns ni les autres ne sont des cas GoogleTest, et ils sont décrits dans [Build, tests et intégration continue](../Guide/guide-outils.md#les-suites-de-tests).

## Ce que chaque exigence a pour garde

La [matrice de traçabilité](couverture-exigences.md) donne, pour chaque exigence en vigueur des spécifications, les cas de test qui la citent — et laisse visibles celles qu'aucun test ne cite. Chaque fiche porte de même ses exigences, et chaque page de domaine récapitule celles qu'elle vérifie.

## Ce que les tests ne remplacent pas

La [recette manuelle](recette-manuelle.md) est la seule page du cahier écrite à la main : les contrôles qu'un humain fait avant de dire « livré » — fluidité, lisibilité, son, manette — avec, pour chacun, ce qu'on regarde et ce qui doit se voir.

## Lancer les tests

```
powershell -File scripts/build.ps1      # compile (préréglage ninja)
ctest --preset ninja                     # exécute tous les cas
ctest --preset ninja -R AttackTest       # une suite
```

Un cas qui échoue se retrouve ici par son identifiant (la recherche du site le trouve), et dans le code par l'emplacement que donne sa fiche.


## Ajouter un cas

Un test **sans** bloc `\castest{}` fait échouer la CI : le cahier est exhaustif par construction,
sinon il ne vaut rien — un cahier partiel laisse croire que ce qui n'y figure pas n'est pas testé.
Le bloc se met dans le commentaire du test, juste au-dessus de sa déclaration :

```cpp
/**
 * @brief Un 20 naturel touche quelle que soit la CA, et double les des de degats.
 * \castest{<b>Un 20 naturel touche une CA hors d'atteinte, est un critique, et double les des
 * de degats sans doubler le modificateur.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. L'heroine (+5, 1d8+3) attaque un gobelin a la CA 40.<br/>2. Le d20 est force
 * a 20.<br/>
 * \tattendu Touche et critique ; deux d8 lances, modificateur 3.
 * }
 */
TEST(AttackTest, UnVingtNaturelToucheEtDoubleLesDes) {
```

| Champ | Ce qu'il porte |
|---|---|
| `<b>…</b>` | L'**objet** du cas, en une phrase : ce que le test établit, pas ce qu'il fait. |
| `\tcat` | La **catégorie**, telle qu'elle paraîtra sur la fiche. |
| `\tcrit` | La **criticité**, parmi les quatre du tableau ci-dessus. |
| `\tetapes` | Les **étapes**, numérotées, séparées par `<br/>`. |
| `\tattendu` | Le **résultat attendu**, en français. |

Le **résultat attendu** d'une fiche n'est toutefois pas recopié de `\tattendu` quand le test porte
des assertions : le générateur lit les assertions GoogleTest du corps de la fonction et les
traduit. Une fiche dit donc ce que le test **vérifie réellement**, et non ce que son auteur a écrit
qu'il vérifiait — les deux divergent au premier remaniement, et c'est toujours le commentaire qui a
tort. `\tattendu` ne sert que de repli, pour un cas dont aucune assertion ne se laisse traduire.

## Ce que le cahier ne dit pas

Il recense ce qui est **vérifié automatiquement**, et cela seul. Une règle du jeu qu'aucun test ne
couvre n'y laisse aucune trace — l'absence d'une fiche n'est donc pas la preuve qu'un comportement
est libre, seulement qu'il n'est pas gardé. Ce qui **doit** être vrai se lit dans les
[spécifications](../Specification/README.md) ; ce cahier dit ce qui est tenu.
