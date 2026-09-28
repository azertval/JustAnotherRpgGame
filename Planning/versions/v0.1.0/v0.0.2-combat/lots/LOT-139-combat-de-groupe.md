+++
id = "LOT-139"
titre = "Le combat de groupe"
version = "0.0.2"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Quatre contre plusieurs : initiative mêlée, chaque personnage joué à son tour, alliés et ennemis qui se gênent et s'entraident."
prerequis = ["LOT-137", "LOT-138", "LOT-132", "LOT-133", "LOT-134", "LOT-135"]
reprend = ["LOT-29", "LOT-41 (rencontres, en partie)"]
livrables = [
  "L'entrée en combat du groupe : placement des quatre sur la zone de combat.",
  "Le tour de chaque personnage joué ; la fin de tour ; l'attente.",
  "Les effets d'**allié** : attaque sournoise, *bless*, soins, tenaille.",
  "Les rencontres **à plusieurs adversaires**, décrites en données, et leur budget de difficulté.",
  "La démo : le maître d'arène lâche **six bandits** sur le groupe, à gauche du sable contre la droite.",
]
sources = [
  "Guide du Maître, « La difficulté d'une rencontre de combat », p. 82-83 ; « Les points d'expérience en fonction de l'indice de dangerosité », p. 274",
  "Manuel des Monstres, « Bandit », p. 343",
  "Manuel des Joueurs, « Tomber à 0 point de vie », PDF p. 199 (le stabilisé qui revient à 1 PV)",
]
criteres = [
  "Un combat à quatre contre quatre se joue de bout en bout dans l'Arena of Fate, au clavier comme à la manette.",
  "Le rejeu à graine fixée donne le même combat.",
  "L'IA répartit ses attaques : elle ne s'acharne pas sur un personnage à terre quand un autre menace.",
]
+++

## Pourquoi

Le moteur de combat sait déjà jouer plusieurs combattants par camp : le tour par tour du `LOT-20`
et l'IA du `LOT-23` n'ont jamais supposé un duel. Ce qui manque, c'est **le joueur à quatre** :
l'entrée en combat, la sélection, les effets entre alliés.

## Ce que le `LOT-138` lui laisse

- Le groupe existe en exploration, mais **seul le meneur** combat sur la carte : l'entrée en
  combat des quatre, sur la zone, est ici.
- Ce que le combat laisse **aux fiches** : les points de vie perdus, les lancers de sorts
  dépensés (`LOT-131`), un membre mort (`Dead`, `LOT-137`) qui ne suit plus le groupe. Aujourd'hui
  chaque rencontre relit les fiches pré-tirées, pleines.

## Décisions de réalisation

Livré le 27 septembre 2026 (exigences `EX-CBT-051`, `EX-CBT-060` à `EX-CBT-063`), **PR #156**.

1. **Le groupe entre là où il marche.** `core::prepareMapEncounter` prend les cases du groupe,
   meneur d'abord (`MapEncounterSetup::partyCells`) : chaque suiveur garde la case où l'exploration
   l'a laissé — dans les pas du meneur — ou prend la case libre de la zone la plus proche, et une
   note le dit. Pas de formation inventée ni d'entités `arenaEntry` sur la carte : le joueur voit
   son groupe se figer où il était. Le montage à un seul héros reste une surcharge.
2. **Chacun joue à son tour, par les mêmes gestes.** Le moteur n'avait jamais supposé un duel :
   `hmi::CombatModel` s'adresse au combattant actif quand il est au joueur (sans profil d'IA), et
   l'IA ne joue que les ennemis. `hmi::EncounterModel` monte les quatre (`partySources`,
   `mountBout`), lit chaque fiche **une fois** (`Catalogs::heroes`), et publie `partyMembers`
   (nom, points de vie, portrait, actif, à terre, mort) et `activeMember` ; `heroName` reste le
   meneur, que la caméra suit. Le HUD met en avant le membre dont c'est le tour, la case du groupe
   du cadre marque l'actif ; `LOT-140` fera le reste de l'interface. La fuite est celle de tous
   (`EX-CBT-012`) : chaque membre se retire à son tour, `F`.
3. **Le combat laisse aux fiches ce qu'il en reste.** `core::PartyLedger` (un `MemberRecord` par
   membre : points de vie, lancers restants par sort) est tenu par la partie (`hmi::WorldModel`,
   vidé à `endGame` et à « Nouvelle partie »), appliqué au montage (`core::applyRecord`, et aux
   `ArenaSpell::uses`) et écrit à la sortie (`settleParty`). Debout : ce qui reste ; **à terre à la
   victoire : 1 PV** — le *Manuel* rend 1 PV au stabilisé après 1d4 heures, la victoire vaut ce
   repos ; **mort** : `WorldModel::buryMember`, le membre quitte le groupe, ne suit plus, et s'il
   menait, le suivant mène. Le dernier membre ne s'enterre pas. Une **défaite** ne laisse rien : la
   partie s'y termine (`LOT-119`). L'écran Groupe et le HUD lisent le registre
   (`PartyModel::hitPointsOf`). Le repos long, qui l'effacera, vient avec l'auberge.
4. **Les effets d'allié étaient déjà là** : l'attaque sournoise (`allyAdjacentToTarget`,
   `LOT-135`), *bénédiction* sur les trois alliés les plus proches (`LOT-134`), les soins et
   *épargner les mourants* (`LOT-137`) lisent le camp, pas un héros. Ce lot **active la prise en
   tenaille** sur la carte (`ArenaBout::flanking`, règle optionnelle du *Guide du Maître* que le
   `LOT-23` jouait à l'Arène du Futur) : à quatre, la place de chacun compte, et l'IA la cherche
   autant que le joueur.
5. **L'IA répartit ses coups** (`EX-CBT-051`) : un ennemi à terre n'est une cible que si **aucun
   ennemi debout ne menace l'acteur au contact** (`core::planTurn`, `menacesImmediates`) ; sinon
   `finishDowned` ne compte pas. Décision nommée, le *Guide du Maître* ne disant rien d'achever
   (`LOT-137`, D10). Le test du `LOT-137` est réécrit : Aldric à trois cases, le gobelin achève
   Bran ; Aldric au contact, il frappe Aldric.
6. **Le budget d'une rencontre** est une règle en donnée, `Rpg/rules/encounter-difficulty.json`
   (schéma `encounter-difficulty.schema.json`, famille des règles de `check_rpg_data.py`) : les
   seuils de PX par niveau (p. 82), les multiplicateurs par nombre de monstres et par taille de
   groupe (p. 83), les PX par indice de dangerosité (p. 274). `core::rateEncounter` rejoue les cinq
   étapes ; l'exemple du livre (275 / 550 / 825 / 1 400 ; 1 000 PX modifiés = difficile) est un
   test. Rien ne l'affiche encore : l'éditeur (`LOT-143`).
7. **La démo : quatre bandits.** Deux créatures originales, écrites d'après le bandit du *Manuel
   des Monstres* (p. 343, ID 1/8, 25 PX) : `bandit` (cimeterre) et `bandit-archer` (arbalète
   légère, dague au contact — le moteur prend la première attaque qui porte, d'où deux fiches).
   **Une rencontre paraît à son marqueur** : `EncounterModel::begin` prend pour déclencheur
   l'entité `encounter` de la carte qui porte la rencontre, s'il y en a une — c'est autour d'elle
   que la formation s'écrit et que l'éditeur la contrôle (`LOT-146` la posait déjà « là où la
   rencontre le fait paraître ») —, quel que soit ce qui engage le combat, dialogue ou pas ; à
   défaut, la case de la dernière interaction, comme avant. Le marqueur du sable est en (25, 10),
   côté droit ; `arene-bandits` pose trois bandits au cimeterre une case à sa gauche, trois
   arbalétriers une case à sa droite ; le groupe parle au maître depuis la gauche et y reste. Six fois 25 PX, multiplié par deux : **300 PX,
   une rencontre difficile** pour quatre niveaux 1 (seuils 100 / 200 / 300 / 400) — quatre
   bandits (moyenne) se gagnaient 98 fois sur cent par l'IA, cinq 83 fois : trop peu de risque
   pour une démo dont la mort est une fin. La rencontre
   `arene-combattant` est retirée ; le combattant de l'arène reste au bestiaire. Les textes du
   maître d'arène parlent au pluriel. Les gestes de l'annexe du `LOT-146` nomment la nouvelle
   rencontre, sans redéplacer le marqueur.
8. **L'équilibrage se remesure** (`test_quete_des_pommes.cpp`, les quatre joués par l'IA
   agressive, tenaille en jeu — donc **sans sort ni soin**, à l'arme seule) : le groupe gagne
   **une fois sur deux** (53 sur cent aux graines 1 à 100, 88 sur deux cents graines tirées de
   la graine maîtresse 120), bornes 45-60 et 85-115 écrites dans le test. Le
   joueur, qui lance les sorts, soigne et cherche la tenaille, fait mieux. Le critère du `LOT-120`
   — le héros seul l'emportait deux fois sur trois — est remplacé par celui-ci. Un combat à dix
   dure deux secondes en Debug : le test à mille graines passe à **deux cents**, sinon la CI y
   passait une demi-heure ; la tenaille n'y est pour rien (mesuré sans : 232 s pour cent). La
   borne de terminaison des tests d'intégration passe de 300 à 900 s : deux cents combats
   prennent sept minutes sur un poste de la CI.

Tests : `test_map_encounter.cpp` (les cases du groupe, sans partage, le débordement rapproché, le
groupe vide refusé), `test_encounter_difficulty.cpp` (les règles, l'exemple du livre, les bandits),
`test_party_ledger.cpp` (application bornée, oubli), `test_death_and_dying.cpp` (l'IA qui
répartit), `test_encounter_model.cpp` (sept combattants ; le rejeu à graine fixée donne le même
journal avec plus d'un membre joué ; le registre relu, montré par l'écran de groupe, écrit à la
sortie ; le mort enterré, le suivant mène), `test_party_model.cpp` (la fuite de tous),
`test_quete_des_pommes.cpp` et `test_demo_de_bout_en_bout.cpp` (les bandits ; le joueur scripté
marche vers sa cible quand il ne la porte pas).

**Reste à la main de l'auteur** : jouer le quatre contre quatre sur le sable, au clavier et à la
manette — passer d'un membre à l'autre au fil de l'initiative, voir la case du groupe du cadre
suivre, fuir membre par membre sur une rencontre fuyable.

## Ce qui n'est pas ici

- La piste d'initiative aux jetons, le panneau du personnage actif avec ses sorts et lancers, la
  prévisualisation d'une capacité, l'action « attendre » : le `LOT-140`.
- Le budget affiché à côté de l'entité `encounter`, et le verdict d'une zone qui compte le groupe :
  le `LOT-143`.
- Le repos long qui rend les points de vie et les lancers (`core::longRest`) : avec l'auberge.
- Les points d'expérience gagnés à la victoire, la montée de niveau : le `LOT-141`.
- L'attaque à distance du Scoundrel (`ranged: arc-court` de sa fiche) : `heroContestant` ne prend
  que l'arme en main directrice, comme avant ce lot.
