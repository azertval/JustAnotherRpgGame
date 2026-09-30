# Bilan de la version 0.0.2 — système de combat

Écrit à la recette ([LOT-142](lots/LOT-142-recette-et-version-0-0-2.md)), le 30 septembre 2026,
comme la [cinquième règle de la trajectoire](../../../vision/trajectoire.md) le demande : ce qui a
coûté plus que prévu, et ce que la version suivante doit en retenir. Les chiffres viennent des
fiches de lot, du `CHANGELOG.md`, de l'historique Git et de la simulation de la recette.

## En une page

| | |
|---|---|
| Ouverte | 27 septembre 2026 (`LOT-130`, PR #146), au lendemain du tag `v0.0.1` |
| Livrée | 30 septembre 2026, tag `v0.0.2` — **quatre jours** |
| Lots livrés | **16**, du `LOT-130` au `LOT-145`, dont 1 S, 11 M, 4 L |
| Décisions prises en route | D-28 à D-37 (dix), dont sept pour la `0.0.2.5` décidée le dernier jour |
| Critères de sortie | 3 sur 3 (voir [ci-dessous](#les-critères-de-sortie)) |
| Ce qui reste dû | la recette par un joueur extérieur ; Parade et Téméraire des PNJ du livre |

La version a tenu son objet : **les quatre classes de base, fidèles à leurs fiches, et le combat
de groupe**, joués sur les trois cartes de la démo. Elle se ferme sur une **série de six combats**
d'arène et sur un **équilibrage mesuré** — le fait central de ce bilan, puisqu'il a changé une
règle du livre.

## Les critères de sortie

| Critère | Tenu par |
|---|---|
| Chacune des quatre fiches préfabriquées se recrée dans le jeu, valeur pour valeur | `LOT-130` (fiches en données), `LOT-141` : `test_character_sheet_model.cpp` compare les quatre pages du *Player's Guide* ; la retouche de *Tough as Nails* ne change aucune valeur de fiche (PV, CA) |
| Un combat à quatre contre quatre se joue de bout en bout dans l'Arena of Fate, au clavier comme à la manette | `LOT-139` (quatre contre six), `LOT-140` (interface sans souris) ; les tests système jouent le combat de la démo par les gestes du tour ; la série de l'arène en ajoute cinq |
| Chaque capacité de classe des fiches a un effet en combat et une ligne dans le journal | `LOT-131` à `LOT-136`, un test par capacité et par sort ; `LOT-142` : l'IA les joue aussi (`EX-CBT-052`) |

## L'équilibrage de la recette

La simulation joue chaque rencontre de la série, les deux camps par l'IA, sur cent graines, par
le groupe entier et par les quatre **trios** sans une classe
([table complète](annexes/LOT-142-recette/simulation-100-graines.md)) :

| Composition | Bandits (1) | Gladiateurs (2) | Morts (3) | Vétéran (4) | Capitaine (5) | Champion (5) | Série |
|---|---:|---:|---:|---:|---:|---:|---:|
| groupe | 70 % | 72 % | 81 % | 68 % | 73 % | 63 % | 71,2 % |
| sans Brawler | 18 % | 7 % | 9 % | 4 % | 10 % | 18 % | 11,0 % |
| sans Priest | 22 % | 16 % | 2 % | 1 % | 14 % | 5 % | 10,0 % |
| sans Scoundrel | 26 % | 5 % | 40 % | 30 % | 20 % | 27 % | 24,7 % |
| sans Mage | 25 % | 19 % | 7 % | 2 % | 0 % | 0 % | 8,8 % |

Écart entre trios : **15,8 points** (critère : moins de vingt). Il en coûtait trois choses :

1. **L'IA devait jouer les classes.** Simulé à l'arme seule (`LOT-139`), le groupe mesurait des
   classes qui n'étaient pas les siennes. L'IA joue désormais sorts, soins, bénédiction, action
   bonus et attaques supplémentaires (`EX-CBT-052`) : les bandits de la démo passent de 53 à 85 %
   de victoires, puis à 70 % avec la retouche du Brawler.
2. **Le Brawler était indispensable.** Sa résistance à tous les dégâts dès le niveau 1 double ses
   points de vie : le trio sans lui restait 30 à 35 points sous le meilleur, quelles que soient
   les rencontres — trois configurations essayées. L'auteur a rendu la résistance **graduée**
   (D-36) : la simulation a changé une règle du livre, et c'est écrit.
3. **Les bêtes ne menacent pas un groupe de niveau 5.** Mammouths, singes géants et tigres à
   dents de sabre perdaient à coup sûr : sans attaques multiples, le budget du *Guide* surestime
   les grosses créatures. Cinq PNJ du *Manuel des Monstres* et leurs **attaques multiples**
   (`multiattack`) ont fait les niveaux 4 et 5.

## Ce qui a coûté plus que prévu

- **Le `LOT-142` lui-même.** Taillé M comme une recette, il a porté un lot de moteur (l'IA des
  sorts), un lot de règles (la résistance graduée, les attaques multiples), un lot de contenu
  (cinq rencontres, cinq PNJ, un dialogue de 35 nœuds) et un d'interface (le choix du meneur). La
  fiche ne disait ni comment la série se jouait, ni ce qu'était une « composition » : quatre
  choix ont été posés à l'auteur le jour même.
- **Les lots L** : `LOT-131` (le socle de classe), `LOT-136` (les assets des quatre classes),
  `LOT-139` (le combat de groupe), `LOT-144` (le mode Quêtes de l'éditeur, ajouté en cours de
  version).
- **La mesure en Debug.** Un combat à dix se joue en trois secondes en Debug, 0,07 en Release :
  la mesure complète se lance en Release, sous une variable d'environnement, et la CI ne joue
  qu'un test de garde.

## Ce qui a marché, et se garde

- **L'équilibrage par simulation, à la table.** Une table de trente cases, cent graines chacune,
  en quatre minutes : elle a tranché une question de règle que l'intuition aurait laissée ouverte.
- **Les rencontres en données, les formations posées par `--apply`, le contrôle `--check`** : il
  a refusé huit adversaires posés sur les coins de l'ovale, que la simulation déplaçait sans le
  dire.
- **Les fiches valeur pour valeur** (`LOT-130`, `LOT-141`) : la retouche d'une capacité n'a
  touché aucune valeur de fiche.

## Ce que la version suivante doit en retenir

1. **Une recette de version est un lot de contenu.** La `0.0.2.5` (`LOT-1010`) doit dire dans sa
   fiche ce qu'elle mesure, et comment, avant de s'ouvrir.
2. **La recette a toujours besoin d'un joueur extérieur.** Pas plus qu'à la `0.0.1`, il n'y en a
   eu ; les tests système et la simulation tiennent lieu de joueur, pas de regard neuf.
3. **Le budget du *Guide* ne suffit pas à juger une rencontre** : il ignore les attaques multiples
   et les sorts de zone. La simulation juge ; le budget ne sert qu'à dégrossir.
4. **Parade, Téméraire** et les réactions des PNJ ne sont pas jouées : le jour où un PNJ du livre
   compte sur elles, il faudra un lot.

## Ce que la version laisse aux suivantes

| À | Quoi | Où c'est écrit |
|---|---|---|
| `0.0.2.5` | Le passage à la 3D : maillages, squelette, huit corps, jour / nuit | D-29 à D-35 ; `LOT-1000` à `LOT-1010` |
| `0.0.3` | Les lieux de la démo au standard final ; les PNJ en figurines | D-25, D-35 |
| plus tard | Les réactions des PNJ (Parade), Téméraire ; la recette par un joueur extérieur | ce bilan |
