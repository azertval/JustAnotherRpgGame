+++
id = "LOT-142"
titre = "Recette et version 0.0.2"
version = "0.0.2"
filiere = "version"
statut = "livre"
taille = "M"
resume = "Le système de combat est joué, équilibré et tagué."
prerequis = ["LOT-140", "LOT-141", "LOT-136", "LOT-143"]
livrables = [
  "Une **série de six rencontres** d'arène, de difficulté croissante, qui éprouve les quatre classes.",
  "L'équilibrage par simulation : taux de victoire par rencontre et par composition de groupe.",
  "La démo `0.0.1` rejouée avec une classe au choix.",
  "Installeur, notes de version, tag `v0.0.2`, bilan.",
]
criteres = [
  "Les trois critères de sortie de la version sont tenus.",
  "Aucune des quatre classes ne gagne ou ne perd toutes les rencontres : l'écart de taux de victoire entre compositions reste sous vingt points.",
]
+++

## Périmètre

Pas de nouvelle zone : la `0.0.2` se joue sur les trois cartes de la démo.

## Décisions de réalisation

Livré le 30 septembre 2026 (exigences `EX-CBT-052`, `EX-CBT-064`, `EX-CBT-065`), branche
`lot-142-recette-0-0-2`, **PR #165**, la PR de publication de la version. Le [bilan](../bilan.md) dit ce que la
version a coûté et ce que la suivante en retient.

1. **Quatre choix de l'auteur, le 30 septembre.** La série se joue **au maître d'arène**, un
   niveau (jusqu'au 5) et un repos entre deux combats ; la simulation fait jouer aux héros
   **leurs sorts et leurs soins** ; une « composition » est un **trio** sans une classe ; « une
   classe au choix » est le **choix du meneur** à « Nouvelle partie ».
2. **L'IA joue les sorts** (`EX-CBT-052`, `core::planTurn`) : une famille de gestes de plus,
   pesée dans la monnaie des attaques — sorts à jet d'attaque, à sauvegarde, à sphère (un allié
   pris compte double, en moins), soins d'un allié ensanglanté ou à terre, *bénédiction* hors
   concentration, attaques supplémentaires, arme spirituelle en action bonus ; les capacités de
   l'acteur (*Sneak Attack*, *Hit the Mark*) entrent dans l'espérance. Les valeurs des gestes qui
   ne blessent pas (relever, stabiliser, ramener, bénir) sont des décisions nommées. En
   simulation, chaque héros est joué par le profil de son rôle (`aggressive`, `pack`, `support`,
   `archer`).
3. **Le registre garde le niveau, et connaît le repos** (`EX-CBT-064`). `settleParty` repartait
   d'un enregistrement neuf : un niveau donné retombait au combat suivant. Le repos long
   (`core::PartyLedger::rest`, action de dialogue `rest`) rend points de vie et lancers, garde le
   niveau ; le cache des fiches du combat se remplit avant d'appliquer le registre.
4. **La série** : `arene-bandits` (niv. 1, la démo), `arene-gladiateurs` (2), `arene-morts` (3),
   `arene-veteran` (4), `arene-capitaine` (5), `arene-champion` (5). Le maître d'arène reste sur
   le sable après les bandits (`condamne|enfant-libere`) ; son dialogue donne une fois par victoire
   le niveau et le repos (`arene/recompense-N`), puis propose le combat suivant. Cinq marqueurs
   posés par `--apply` ([gestes](../annexes/LOT-142-recette/gestes/arena-of-fate.json)), aucune
   formation sur un obstacle (`--check` vert).
5. **Les adversaires du livre.** Les bêtes du bestiaire ne menacent pas un groupe de niveau 4 ou
   5, qui battait mammouths et singes géants à coup sûr. À la demande de l'auteur, cinq PNJ du
   *Manuel des Monstres* (annexe B) : malfrat, berserker, capitaine bandit, vétéran, gladiateur ;
   et leurs **attaques multiples** (`multiattack`, jouées comme l'*Extra Attack*). Parade et
   Téméraire sont décrits, pas joués.
6. **L'équilibre des classes est retouché** (`EX-CBT-065`). Quelles que soient les rencontres,
   le trio sans Brawler restait 30 à 35 points sous le meilleur : sa résistance à **tous** les
   dégâts dès le niveau 1 double ses points de vie. L'auteur l'a rendue **graduée** — la part des
   PV perdus, la moitié au plus à 0 PV —, écart au livre écrit dans la capacité et au
   [référentiel](../../../../referentiels/regles/classes-simplifiees.md).
7. **La mesure** (cent graines par case, Release,
   [table](../annexes/LOT-142-recette/simulation-100-graines.md)) : le groupe gagne 70, 72, 81,
   68, 73 et 63 % ; les trios 11 % (sans Brawler), 10 % (sans Priest), 25 % (sans Scoundrel) et
   9 % (sans Mage) sur la série — **écart 15,8 points**. Aucun trio ne gagne ni ne perd tout. Au
   budget du *Guide*, les rencontres sont « difficiles », les deux dernières « mortelles ». Le
   test de garde tient chaque rencontre dans sa bande sur trente graines en Release (six en
   Debug, qui ne vérifie que la fin des combats) ; la mesure complète tourne sous
   `JADG_SIMULATION_SEEDS`. Les bandits de la démo passent de 53 % (arme seule) à 70 %.
8. **Le meneur au choix.** « Nouvelle partie » ouvre l'écran Groupe, titré « Choisissez votre
   meneur » (`WorldModel::choosingLeader`) ; une carte imposée (`--map=`, le lanceur) ne le
   demande pas. Le meneur parle et fait les jets : le test système rejoue la démo avec le
   Scoundrel pour meneur jusqu'à la fin par la parole.
9. **La version.** `CMakeLists.txt` porte `0.0.2` ; le CHANGELOG ferme la section
   `## [0.0.2] - 2026-09-30` (PR au label `no-changelog`, comme au `LOT-122`) ; le tag `v0.0.2` se
   pose sur le commit de fusion, `release.yml` teste, empaquette, lance les archives et publie.
   Les deux dernières alertes clang-tidy de Code scanning sont corrigées dans la même PR.

Tests : `test_ai_spells.cpp` (six gestes de l'IA), `test_serie_de_l_arene.cpp` (budget, bandes,
mesure complète), `test_party_ledger.cpp` et `test_encounter_model.cpp` (niveau gardé, repos),
`test_dialogue.cpp` (l'action `rest`), `test_quete_des_pommes.cpp` (équilibrage de la démo, le
maître qui reste), `test_demo_de_bout_en_bout.cpp` (le meneur choisi), `test_party_model.cpp`.

## Ce qui n'est pas ici

- La recette par un **joueur extérieur** : pas plus qu'à la `0.0.1`, elle n'a eu lieu — voir le
  bilan.
- Parade, Téméraire, et les attaques de multiattaque imposées par le bloc (deux cimeterres et une
  dague) : l'IA choisit chaque attaque parmi celles du bloc.
- Un écran dédié au choix du meneur : c'est l'écran Groupe, titré pour l'occasion.
