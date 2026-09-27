+++
id = "LOT-137"
titre = "États, agonie et mort"
version = "0.0.2"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "Un personnage à zéro point de vie tombe, fait ses jets de sauvegarde contre la mort, se relève si on le soigne."
prerequis = ["LOT-131"]
reprend = ["LOT-72", "LOT-35 (états, en partie)"]
livrables = [
  "Les états dont les quatre classes ont besoin : inconscient, à terre, béni, invisible, concentré.",
  "Agonie, jets contre la mort, stabilisation, mort ; *spare the dying*, *cure wounds*, *revivify*.",
  "La défaite du **groupe** : tous à terre.",
]
criteres = [
  "Un allié à terre se relève par *cure wounds* et rejoue à son tour.",
  "Trois échecs tuent ; un 20 naturel relève.",
  "L'IA achève ou épargne selon son profil (`LOT-23`).",
]
sources = ["Manuel des Joueurs, ch. 9 (PDF p. 189-210)"]
+++

## Périmètre

Les états **nécessaires aux quatre classes jusqu'au niveau 5**, pas le catalogue du *Manuel* : le
reste vient avec les classes complètes de la `0.3.0`.

## Décisions de réalisation

Livré le 27 septembre 2026 (**PR #152**). Les règles sont celles du *Manuel des Joueurs*, « Tomber à 0 point
de vie » (PDF p. 199), « Soins » (p. 199) et l'annexe A, « Inconscient » et « À terre »
(p. 293-294).

1. **Trois statuts, plus la mort.** `core::CombatantStatus` gagne `Dead` : le mort reste sur la
   grille — son corps, que *revigorer* vise —, ne joue plus, ne se soigne plus. Ce qui arrive à
   0 PV est un champ du profil, `core::AtZeroHitPoints` : un personnage agonise (le défaut), un
   monstre du bestiaire meurt (« Les monstres et la mort »). Un profil écrit à la main agonise.
2. **La machine note, la session jette.** `core::CombatState` tient le compteur
   (`core::DeathSaves`), la mort instantanée (excédent au moins égal au maximum), l'échec d'un
   blessé à terre (deux sur un critique), la stabilisation, `revive`, et annonce
   `CombatHook::DeathSaveDue` à la place d'un mourant ; la session d'arène jette le d20 avec sa
   suite aléatoire et l'écrit. La machine n'a toujours pas de dés : un test lui donne ses d20.
3. **Un 20 relève et fait jouer.** Le jet se fait « au début du tour » : relevé par un 20, le
   personnage joue ce tour-ci ; un 1 compte deux échecs même quand la *bénédiction* porterait le
   total à 10 — c'est le dé qui tranche.
4. **À terre.** Tomber met à terre ; un soin relève mais laisse à terre jusqu'au début du tour, où
   le moteur **relève d'office** pour la moitié du déplacement (« Se relever ») : rester couché ne
   sert à rien ici. D'ici là, on l'attaque avec avantage au contact et désavantage au-delà, et il
   frappe avec désavantage (une attaque d'opportunité).
5. **Inconscient.** Avantage contre lui ; un coup qui le touche **au contact** est critique
   (étape `Hit`, `AttackRoll::criticalSource`, écrit au journal) ; il rate d'office ses
   sauvegardes de Force et de Dextérité, et une sphère le prend — un mourant dans la boule de feu
   note un échec. Un sort qui blesse ne le vise pas directement : l'achever se fait à l'arme.
6. **Concentré.** Des dégâts subis par un lanceur debout qui tient un sort de concentration
   demandent une sauvegarde de Constitution, DD 10 ou la moitié des dégâts, *bénédiction*
   comprise ; un échec met fin à ses effets. *Vol*, *invisibilité* et *bénédiction* ne déclarent
   plus `jet-de-concentration`. Un lanceur qui tombe perd sa concentration sans jet, comme avant.
7. **Béni, invisible** étaient joués (`LOT-133`, `LOT-134`) ; ils entrent, avec les quatre
   nouveaux et *en vol*, dans `ArenaSession::conditionsOf` (`core::CombatCondition`), que
   l'inspecteur de l'écran de combat affiche. Pas de catalogue d'états en donnée : les huit du
   moteur suffisent jusqu'au niveau 5.
8. **Les sorts.** *Épargner les mourants* est le mécanisme `stabilizes` ; *revigorer*,
   `revives` (`hitPoints` 1, `withinRounds` 10 : « moins d'une minute »). Le diamant de 300 po
   n'est pas décompté — le combat ne tient pas l'inventaire. *Soin des blessures* relevait déjà
   (`LOT-134`) ; il ne vise plus un mort. *Restauration inférieure* reste déclarée
   (`etats-en-combat`) : aucune des quatre classes n'inflige aveuglé, assourdi, paralysé ou
   empoisonné avant le niveau 5, et le catalogue des états vient avec la `0.3.0`.
9. **La défaite du groupe** ne change pas de règle : « debout » compte seul, et tous à terre — ou
   morts — c'est perdu (`LOT-20`). Un combat **sans mort** (`setLethal(false)`, recopié de
   `ArenaBout::lethal` : la Marque Héroïque) ne tue ni ne fait jeter personne ; la carte est
   létale.
10. **L'IA** (`LOT-23`) : le poids `finishDowned` d'un profil (`behaviors.json`, schéma) vaut
    pour un ennemi à terre ce que `damageDealt` vaut pour un ennemi debout ; 0 l'épargne. Le
    *Guide du Maître* (p. 247-255) ne dit rien d'achever : c'est une décision nommée — agressif
    75, meute 100, prudent, soutien et archer 0. Un ennemi à terre n'est jamais une menace, et
    l'espérance ne compte pas le critique au contact (sous-estimée, jamais surestimée).

Écran : un mort se couche comme un combattant à terre (`down`, plus `dead`), l'inspecteur écrit
« mort » et les états de la session ; `jadg_en.ts` traduit « mort ».

Tests : `test_death_and_dying.cpp` — trois échecs tuent, un 20 relève et fait jouer, un 1 compte
double, la stabilisation, les dégâts à terre et la mort instantanée, le monstre qui meurt, la
Marque qui protège, *revive* ; dans l'arène : le soin qui relève et fait rejouer, le jet écrit,
*épargner les mourants*, *revigorer* et sa minute, le critique au contact, la concentration
rompue ; l'IA qui achève ou épargne. `test_class_priest.cpp` : le grimoire de niveau 5 compte les
deux sorts.

## Ce qui n'est pas ici

- Assommer au lieu de tuer (« Assommer une créature »), les premiers soins (Sagesse (Médecine)
  DD 10), le retour à 1 PV d'un stabilisé après 1d4 heures : hors combat, ou sans demande des
  quatre classes.
- Le catalogue des états du *Manuel* (aveuglé, empoisonné, entravé…) et ce qui les lève : la
  `0.3.0`.
- Choisir de rester à terre, ramper : l'interface de combat de groupe (`LOT-140`).
- Ce que la mort d'un héros fait **après** le combat (fiche, partie) : le groupe de quatre
  (`LOT-138`) et le combat de groupe (`LOT-139`).

## Exigences

- `EX-CBT-040` — à 0 PV un personnage tombe inconscient et jette contre la mort ; trois succès le
  stabilisent, trois échecs le tuent.
- `EX-CBT-041` — un soin remet le compteur à zéro et rend conscient.
- `EX-RPG-050` — *épargner les mourants* et *revigorer* sont des combinaisons déclarées de
  mécanismes ; `EX-RPG-051` — *restauration inférieure* déclare ce qu'elle attend.
