# Les décisions

Une ligne par décision de **planification**. Les décisions de réalisation vivent dans la fiche du
lot qui les a prises.

## Tranché

| # | Date | Décision | Pourquoi |
|---|---|---|---|
| D-01 | 20 sept. 2026 | Le jeu quitte le **pixel art** pour la **2D HD**, sur le modèle de la planche de référence d'Arenarea | décision de l'auteur |
| D-02 | 20 sept. 2026 | **Table rase** : tous les assets de scène et toutes les cartes actuels sont supprimés ; pas de cohabitation des deux styles | décision de l'auteur — repartir sur un standard imposé |
| D-03 | 20 sept. 2026 | Le référentiel `0.1.0` est **l'Empire central seul** : ses zones, ses PNJ nommés, quatre classes, ses peuples et animaux, ses monstres | l'ancien référentiel (le monde entier) était titanesque |
| D-04 | 20 sept. 2026 | La `0.0.1` est une **démo basique** : trois quartiers, une quête, un combat seul contre un | décision de l'auteur |
| D-05 | 20 sept. 2026 | La `0.0.2` est le **système de combat** : quatre classes d'après les fiches préfabriquées, combat de groupe, interface | décision de l'auteur |
| D-06 | 20 sept. 2026 | Une sous-version `0.0.x` **par zone**, puis une version `0.x.0` **par région** ; `0.2.0` la compagnie, `0.3.0` classes et espèces, `0.16.0` autres régions, `0.17.0` plan pénombral, `1.0.0` monde complet | décision de l'auteur |
| D-07 | 20 sept. 2026 | La planification vit dans **`Planning/`**, découpé en sous-dossiers, avec un site relié à la documentation | décision de l'auteur |
| D-08 | 20 sept. 2026 | L'ancienne feuille de route est **figée**, pas supprimée ; les lots repris reçoivent de **nouveaux numéros à partir de `LOT-100`** | une centaine de renvois `@ref lot-NN` de la documentation pointent sur ses ancres ; renuméroter évite toute collision avec l'histoire |
| D-09 | 20 sept. 2026 | Les lots sont des **fichiers à en-tête TOML** ; les états « prêt », « prochain », « en attente » se **calculent** | une page de 2 600 lignes dérivait, et son lint devait deviner sa structure |
| D-10 | 20 sept. 2026 | Le **moteur est révisé** pour la HD (LOT-103) — contrairement à l'hypothèse de départ | l'[audit](../standards/audit-passage-hd.md) : échelle de l'art, hauteur de figurine, filtrage et zoom sont compilés |
| D-11 | 20 sept. 2026 | La suppression des assets est un **lot** (LOT-102), pas un geste : les tests sont d'abord rebranchés sur des données de test | une trentaine de tests, quatre scripts et trois règles CMake lisent les fichiers supprimés |
| D-12 | 20 sept. 2026 | Les régions s'ouvrent **par proximité de l'Empire** | on n'ouvre une région que si l'on peut y marcher |
| D-13 | 20 sept. 2026 | Le niveau est **plafonné à 5** jusqu'à la `0.3.0` | c'est ce que les quatre classes de base couvrent dans l'Empire |
| D-14 | 20 sept. 2026 | Les quatre classes de base **restent** après la `0.3.0` | ce sont les classes simplifiées du livre, pas un brouillon des vraies |

## À trancher par l'auteur

| # | Question | Proposition | Se tranche au |
|---|---|---|---|
| Q-01 | Les seize **cartes peintes** de `Assets/Maps/` partent-elles avec le reste ? | Garder le monde et les treize régions (ce n'est pas du pixel art) ; refaire les plans de ville | LOT-102 |
| Q-02 | Les valeurs du **standard 2D HD** : losange de 256 × 159, figurine de 170 px, six ou huit images par animation | Celles du [standard](../standards/style-2d-hd.md), éprouvées sur une maquette | LOT-101 |
| Q-03 | L'**interface** (charte v2) reste-t-elle telle quelle à côté d'une scène HD peinte ? | Oui : elle n'est pas en pixel art ; revoir seulement `EX-VIS-009`, qui opposait les deux identités | LOT-101 |
| Q-04 | « **Arena of Fate** » est une zone à part, alors que le livre la place **dans** Arenarea | La garder à part : c'est une carte d'intérieur, avec ses vestiaires et son sable | LOT-107 |
| Q-05 | Le **maître d'arène**, la mère, l'enfant, le garde : noms et apparences | À nommer au lot de la quête ; le livre n'en dit rien | LOT-120 |
| Q-06 | Qui fait le **jet de dialogue** dans un groupe : le meneur ou le plus doué ? | Le meneur | LOT-138 |
| Q-07 | **Figurine composée** (corps, tenue, arme) ou une figurine par combinaison d'espèce et de classe ? | À décider tôt : 21 espèces × 20 classes ne se dessinent pas une à une | LOT-303, à instruire dès la `0.0.2` |
| Q-08 | **Stockage des assets** au-delà d'un gibioctet : dépôt, Git LFS, ou dépôt à part ? | Mesurer à la `0.0.3`, décider alors | LOT-156 |
| Q-09 | L'ancienne feuille de route et son lint : jusqu'à quand ? | Jusqu'à la `0.0.1`, puis retrait | LOT-122 |
| Q-10 | Les ~17 **villages sans texte** de la carte du monde : contenu inventé, ou décor ? | Décor dans la `0.1.0`, contenu au besoin | `0.0.7` à `0.0.9` |
| Q-11 | Le **Madwalker** sans ses formes pénombrales, ou reporté à la `0.17.0` ? | Livré sans, complété ensuite | LOT-312 |
| Q-12 | L'**éditeur** : ses quatre lots restants (`LOT-EDITOR-08` à `11`) avant ou pendant les zones ? | `LOT-EDITOR-09` (lieu à plusieurs cartes) avant Phantom Fortress ; les autres au besoin | `0.0.5` |
