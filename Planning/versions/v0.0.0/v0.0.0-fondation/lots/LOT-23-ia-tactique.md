+++
id = "LOT-23"
titre = "IA tactique ennemie"
version = "0.0.0"
filiere = "regles"
statut = "livre"
taille = "L"
resume = "Les ennemis jouent leur tour seuls, par des heuristiques pondérées déterministes, avec les mêmes actions et les mêmes informations que le joueur."
prerequis = ["LOT-20", "LOT-21", "LOT-22"]
livrables = [
  "`Source/Core/Combat/EnemyAi.{h,cpp}` : `core::isBloodied`, `core::requiredRoll`, `core::hitChance`, `core::expectedDamage`, `core::BehaviorProfile`, `core::planTurn`, `core::playTurn`, `core::shouldTakeOpportunity`.",
  "`Source/Core/Combat/Flanking.{h,cpp}` : la prise en tenaille, règle optionnelle du Guide du Maître.",
  "Dans l'arène : `core::ArenaSession::dash`, `isDodging`, `behaviorOf`, `setOpportunityPolicy`, `core::Arena::flanking`.",
  "`Source/Elements/Rpg/rules/behaviors.json` et son schéma : cinq profils et leurs règles d'attribution.",
  "À l'écran du Colisée : les ennemis joués par l'IA, chaque décision au journal, un bouton *Se précipiter*.",
  "Deux tests de tenaille, sept d'IA.",
]
criteres = [
  "Un combat IA contre IA se termine toujours, sur un échantillon de configurations générées.",
  "Rejeu à graine fixe strictement reproductible.",
  "L'IA ne finit pas son tour à portée de trois ennemis quand une position sûre existait.",
  "Tests headless intégralement automatisables.",
  "`ctest` : 1139/1139 (1130 avant ; deux tests de tenaille, sept d'IA).",
]
sources = [
  "Guide du Maître, chapitre 8, « Le combat » (PDF p. 247-255)",
]
+++

## Pourquoi

Donner aux ennemis un comportement de combat crédible, **déterministe** et testable sans GPU : des
heuristiques pondérées, pas un réseau de neurones (la décision de la feuille de route tient).

Le **Guide du Maître**, chapitre 8, « Le combat » (PDF p. 247-255), fait foi : c'est la source que
l'utilisateur a désignée pour ce lot. Ces pages ne donnent **aucune tactique** aux monstres. Elles
disent au MD ce qu'il sait d'un combat et comment il le compte ; l'IA est le MD qui joue les
monstres, et elle prend exactement cela. Ce que le Guide ne dit pas — les poids — est une donnée,
nommée ci-dessous.

## Périmètre

- **L'IA** (`Source/Core/Combat/EnemyAi.{h,cpp}`) : ce que la table sait (`core::isBloodied`,
  `core::requiredRoll`, `core::hitChance`, `core::criticalChance`, `core::expectedDamage`), les
  profils (`core::BehaviorProfile`, `core::loadBehaviors`, `core::behaviorFor`), le tour décidé
  (`core::planTurn`, `core::TurnPlan`) et joué (`core::playTurn`), et la décision de prendre une
  attaque d'opportunité (`core::shouldTakeOpportunity`, `core::aiOpportunityPolicy`).
- **La prise en tenaille** (`Source/Core/Combat/Flanking.{h,cpp}`), règle optionnelle du Guide :
  `core::crossesOppositeSides`, `core::isFlankedFrom`, `core::isFlanked`.
- **Dans l'arène** : l'action *se précipiter* (`core::ArenaSession::dash`), qui manquait au joueur
  aussi ; l'état d'esquive lisible (`isDodging`) ; le profil de chaque combattant
  (`core::ArenaContestant::behavior`, `behaviorOf`) ; une politique d'opportunité
  (`setOpportunityPolicy`) — l'arène prenait **toutes** les attaques d'opportunité faute de
  quelqu'un pour décider ; la tenaille jouée si la donnée de l'arène l'active
  (`core::Arena::flanking`, `core::ArenaBout::flanking`).
- **Dans la donnée** : `Source/Elements/Rpg/rules/behaviors.json` et son schéma — cinq profils
  (agressif, prudent, soutien, archer, meute) et leurs règles d'attribution ; le champ `flanking`
  du schéma des arènes, activé pour l'Arène du Futur.
- **À l'écran du Colisée** : les ennemis sont joués par l'IA (une case à cocher le désactive pour
  commander les deux camps, comme au `LOT-50`), leurs tours se jouent seuls entre ceux du joueur, et
  chaque décision s'écrit au journal avant d'être jouée ; un bouton *Se précipiter*.

Il ne livre **pas** les lanceurs de sorts (aucun sort n'existe encore : `LOT-25`, `LOT-35`), les
actions de repaire et les boss (`LOT-46`), la règle *Tactique de groupe* elle-même (seul le profil
la lit ; le mécanisme vient avec les créatures de Tanares, `LOT-46`), ni le choix du vol ou de la
marche créature par créature (`core::profileFor` garde sa règle).

### Ce qui reste hors du lot, nommément

- **Les lanceurs de sorts** et le choix dans une liste de sorts : `LOT-25`, `LOT-35`.
- **Les actions de repaire, les boss, les traits de groupe comme mécanismes** (*Tactique de groupe*
  donnant l'avantage) : `LOT-46`.
- **Le choix du vol** créature par créature.
- **Les règles optionnelles écartées** des mêmes pages : les diagonales à 1,50 puis 3 m, faire face ;
  et les **poursuites**, qui sont une scène d'exploration, pas un combat tactique.
- **Montrer** l'intention de l'IA au joueur autrement qu'au journal : l'IHM de combat, `LOT-24`.
- **Comparer l'IA à la tactique élémentaire** du `LOT-20` (marcher vers le plus proche, frapper) :
  aucun critère ne le demande, et une victoire d'une IA contre une autre dit plus des poids que du
  comportement.

## Conception

### Les règles du Guide, et où chacune vit

| Règle du Guide (chapitre 8) | Où | Vérifiée par |
|---|---|---|
| Les points de vie des monstres se suivent en secret ; sous la moitié, un monstre « semble souffrir » : ensanglanté | `core::isBloodied` ; l'IA ne lit jamais les PV d'un adversaire | `EnemyAiTest.LIaNeLitQueLEtatEnsanglante` |
| « Gérer les foules » : le résultat minimal au d20 est la CA de la cible moins le bonus d'attaque | `core::requiredRoll`, `core::hitChance` | `EnemyAiTest.LeJetRequisEtLEsperanceSuiventLeGuide` |
| Dégâts moyens ; au critique, « lancez les dés de dégâts associés au coup et ajoutez-les aux dégâts moyens » | `core::expectedDamage` | idem |
| Le champ de vision et l'abri sur une grille (méthode déjà prise au `LOT-22`) | `core::planTurn` vise par `hasLineOfSight`, compte l'abri par `coverFrom` | `EnemyAiTest.LArchereChercheLaVue` |
| Prise en tenaille : adjacents, sur des côtés ou des angles opposés, la ligne des centres tranche ; pas contre un ennemi qu'on ne voit pas, pas neutralisé ; une grande créature par l'une de ses cases | `core::crossesOppositeSides`, `core::isFlankedFrom` | `FlankingTest.LaLigneDesCentresTranche` |
| La tenaille donne l'avantage aux jets d'attaque au corps à corps | `core::ArenaSession` (`contextAgainst`) | `FlankingTest.LaTenailleDonneLAvantageDansLArene` |
| Une règle **optionnelle** | `core::Arena::flanking`, faux par défaut | idem |
| « Gérer le temps de réaction » : l'attaque d'opportunité interrompt son déclencheur | `core::ArenaSession::move` (inchangé), la décision dans `core::shouldTakeOpportunity` | `EnemyAiTest.SePrecipiterEtChoisirSesOpportunites` |

### Comment l'IA décide

À son tour, l'IA examine **toutes** les cases où finir son déplacement (`core::ReachableArea`), et
la sienne. Pour chacune :

- **les attaques possibles** — chaque attaque, chaque ennemi debout à portée et en vue — valent
  l'espérance de dégâts contre la CA de la cible et son abri, dans la posture que la case donne
  (tenaille, longue portée, tir au contact, esquive de la cible), pondérée par le profil : plus
  contre une cible ensanglantée, plus par allié déjà à son contact (la meute), plus si elle menace un
  allié ensanglanté (le soutien) ;
- **la menace** est l'espérance des coups que les ennemis peuvent porter sur cette case au prochain
  round — en marchant jusqu'au contact, ou à distance depuis leur portée plus leur vitesse ;
- **les attaques d'opportunité** que le chemin provoque, telles que l'arène les jouera.

Sans attaque possible, elle compare **s'avancer** vers l'ennemi le plus proche par le chemin
(`core::findPath`), **se précipiter** au plus loin de ce chemin, **esquiver** si son profil le veut
et qu'on la menace, ou **se désengager** si le chemin provoque une attaque d'opportunité. Après avoir
frappé, un profil prudent ou archer **recule** vers une case moins menacée, si elle en existe une.

Le meilleur candidat se choisit dans un **ordre lexicographique** : d'abord le moins d'ennemis en
excès sur ce que le profil tolère, puis attaquer plutôt que ne pas attaquer, puis **avancer** vers
l'ennemi plutôt que tenir ou reculer, puis le score, puis le moins de déplacement. À égalité, le
premier examiné reste.

### Les deux défauts à prévenir, et comment

**Le suicide.** Les poids ne suffisent pas : une case qui prend en tenaille un ennemi ensanglanté
peut rapporter plus que trois menaces faibles ne coûtent. Le nombre d'ennemis qui peuvent frapper la
case de fin **sans bouger** est donc une **clé**, pas un poids : au-delà de `toleratedThreats` (au
plus 2, le schéma l'impose), une case perd contre toute case qui en tolère plus. Le test place la
seule case de tenaille au contact de trois héros ; **réintroduit à la main**, un classement au seul
score y envoie les cinq profils.

**Le blocage.** Trois règles le rendent impossible sur une carte connexe : une IA qui **peut**
attaquer attaque ; une IA qui ne le peut pas **avance** le long d'un chemin dont la longueur décroît
— c'est une **clé** de la comparaison, avant le score, depuis la correction ci-dessous ; chaque tour
se **termine** (`core::ArenaSession::endTurn`), qu'il ait servi ou non.

*Corrigé après livraison, en jouant au Colisée.* L'IA fuyait. (1) L'avance n'était qu'un poids
(`approachPerTile`) face à la menace, qui vaut un round entier de dégâts dès qu'une case est à
« vitesse + allonge » d'un héros et rien au-delà : sur la carte 20×14 de l'arène, un prudent
s'arrêtait au bord de cette zone et reculait quand le héros avançait — les tests, joués dans des
salles de 10 à 16 cases où la zone couvre tout, ne le voyaient pas. (2) « À portée de trois
ennemis » comptait les tireurs sur toute leur portée : dès deux arcs ou armes de jet en face, toute
case atteignable dépassait `toleratedThreats`, et comme l'excès passe avant l'attaque, la case de
contact perdait contre n'importe quelle case lointaine. Seules les attaques **de contact** comptent
désormais ; un tireur pèse dans la menace. (3) À score égal, le premier candidat examiné
l'emportait, donc la case de plus petit indice : le repli après attaque filait au coin haut-gauche,
et un tireur allait y tirer. Le dernier critère de la comparaison est désormais le moins de
déplacement. Le test génère trente salles — de 10 à 16
cases sur 8 à 10, des piliers, deux à quatre combattants par camp, tous les profils, un tiers de
tireurs, la tenaille une fois sur deux — et joue chaque combat par l'IA des deux côtés : tous
atteignent leur issue avant la garde de 600 tours.

### Déterministe, en entiers

Pas un flottant dans la décision : une chance de toucher est un nombre de quatre-centièmes (le carré
d'un vingtième, pour l'avantage et le désavantage), des dégâts moyens des demi-points, un poids un
pourcentage ; l'espérance est donc un entier de huit-centièmes de point. Les cases se parcourent par
indice croissant, les cibles par identifiant croissant, une égalité garde le premier candidat. Le
rejeu d'une graine sur trois du test de terminaison redonne **le même journal**, décision comprise.

## Décisions de réalisation

- **Les points de vie restent secrets, l'état ensanglanté se voit.** C'est la lecture littérale du
  Guide, et c'est `EX-CBT-050` : deux cibles à 20/20 et 11/20 ont le même score au point près ; à
  9/20, la seconde est préférée. Un poids qui lirait la fraction de vie a été essayé à la main pour
  vérifier que le test le refuse.
- **La CA de la cible se lit ; ses résistances non.** L'IHM de combat montrera la probabilité de
  toucher au joueur (`LOT-24`), qui suppose la CA ; une résistance ne se connaît qu'après l'avoir vue
  jouer.
- **La menace ne compte pas l'abri.** L'abri de chaque case contre chaque tireur doublait le coût
  d'un tour pour une nuance de deux points de CA ; un tireur qui marche trouve de toute façon sa
  case. L'abri compte, en revanche, dans le choix de la cible et dans le jet réel.
- **« À portée de trois ennemis »** se lit « trois ennemis peuvent frapper la case **au contact,
  sans bouger** » : mesurée avec leur déplacement, ou avec la portée d'un tireur, presque toute case
  d'une arène est à portée de tout le monde, et le critère n'aurait rien interdit — ou tout.
- **La tenaille est optionnelle, et activée au Colisée.** Le Guide la présente comme une règle
  optionnelle ; l'arène est le banc d'essai du combat, et c'est la seule règle de positionnement de
  ces pages. Une arène sans le champ ne la joue pas.
- **La ligne des centres tranche toujours**, et pas seulement « en cas de doute » : c'est la seule
  des deux formulations du Guide qui se calcule sans interprétation. Coins compris, ce qui couvre
  les angles opposés.
- **Se précipiter** est une action du Manuel que l'arène n'offrait pas : l'IA en avait besoin pour
  traverser une grande salle, et `EX-CBT-050` interdit une action réservée aux monstres, d'où le
  bouton.
- **Cinq profils, pas quatre.** La feuille de route nommait agressif, prudent, soutien et archer ;
  la *meute* s'ajoute parce que dix créatures du bestiaire portent *Tactique de groupe*, et que le
  profil est la seule façon de les faire frapper ensemble avant que le trait ne devienne un
  mécanisme (`LOT-46`). Le soutien, sans sorts, protège : il vise ce qui menace un allié
  ensanglanté.
- **L'attribution est une règle de la donnée** : un trait, une créature, ou le goût du tir (une
  attaque à distance à portée connue au moins aussi forte qu'au contact). Le singe, dont le rocher
  frappe plus fort que le poing, est donc archer.
- **Un personnage enrôlé côté ennemi** prend le profil par défaut.
- **Le seuil d'opportunité est un jet requis**, le vocabulaire du Guide : le prudent laisse passer
  un fuyard qu'il ne toucherait qu'à 16 ou plus.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔ Trente salles, compositions et profils générés ; chaque combat a son issue.
2. ✔ Même journal et même nombre de tours au rejeu ; aucune décision ne passe par un flottant.
3. ✔ Pour les cinq profils, sur la case la plus rentable et la seule dangereuse ; le défaut réintroduit fait échouer le test.
4. ✔ Tout est dans `Core`, joué sans fenêtre.
5. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1139/1139 en Debug, les lints et le contrôle des données RPG verts ; deux défauts réintroduits à la main font chacun échouer leur test.

Alimente [LOT-24](@ref lot-24), [LOT-27](@ref lot-27), `LOT-46`.

Exigences couvertes : `EX-CBT-050` (l'IA choisit dans les mêmes actions que le joueur, avec les mêmes informations). Aucune exigence ajoutée.
