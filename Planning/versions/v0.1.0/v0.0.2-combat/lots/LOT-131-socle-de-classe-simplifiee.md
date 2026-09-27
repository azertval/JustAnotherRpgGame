+++
id = "LOT-131"
titre = "Le socle de classe simplifiée"
version = "0.0.2"
filiere = "moteur"
statut = "en-cours"
taille = "L"
resume = "Une classe est une donnée : une table de progression, des capacités à effets nommés, des ressources qui se dépensent et se récupèrent."
prerequis = ["LOT-130"]
reprend = ["LOT-47 (socle de classe)", "LOT-25 (capacités de classe, en partie)"]
livrables = [
  "Le schéma et le chargement d'une classe simplifiée : dé de vie, maîtrises, table par niveau.",
  "Les **capacités** comme effets branchés sur les crochets du combat (`LOT-20`, `LOT-21`) : modifier un jet, une CA, des dégâts, un déplacement.",
  "L'**incantation simplifiée** : sorts fixés par la table, **deux lancers par jour et par sort**, pas d'emplacements.",
  "La **résistance globale** aux dégâts, et le déplacement **sans attaque d'opportunité**.",
]
criteres = [
  "Une classe de test à trois capacités se charge et agit en combat sans une ligne de C++ qui la nomme.",
  "Le journal de combat nomme chaque capacité qui a joué.",
  "Un sort épuisé ne se propose plus ; un repos long le rend.",
]
sources = [
  "Player's Guide to Tanares, p. 192 (règles communes), 196 et 200 (incantation simplifiée)",
]
+++

## Périmètre

Le niveau **1 à 5** : c'est ce que l'Empire central demande. La table complète jusqu'au niveau 20
est saisie en données, mais seules les capacités des niveaux 1 à 5 sont implémentées et testées
ici ; le reste appartient à la `0.3.0`.

## Décisions de réalisation

Livré le 27 septembre 2026 (PR à ouvrir). Le socle ne nomme aucune classe ni aucune capacité : il
charge des **effets nommés** et les branche ; la classe d'essai de la racine de données
(`Source/Test/Fixtures/GameData/Rpg/classes/lutteur-d-essai.json`, quatre capacités, trois sorts)
est la seule à agir en combat dans ce lot, et seuls ses tests la nomment.

1. **Une capacité est un catalogue** (`Rpg/capacities/`, `capacity.schema.json`, famille
   `capacities` du contrôle de données) : une liste d'effets d'un des **sept genres** que le moteur
   sait brancher — `attack-bonus`, `armor-class-bonus`, `unarmored-armor-class` (base, modificateurs,
   bouclier permis), `damage-resistance` (tous les types ou une liste), `speed-bonus` (en mètres,
   l'unité des espèces), `no-opportunity-attacks`, `extra-damage` (dés, une fois par tour). Un
   genre inconnu ou un champ que son genre exige et qui manque **refuse la capacité entière** : jouée
   à moitié, elle tromperait plus qu'absente et nommée dans les erreurs. `replaces` retire la
   capacité qu'une autre remplace (*Hit the Mark Improvement*) : les effets ne s'additionnent pas.
2. **La fiche porte ce que sa classe lui donne** (`CharacterSheet::capacities`, copies ;
   `knownSpells` ; `weaponProficiencies`, `armorProficiencies`), posé par `applyClassFeatures` au
   chargement et à chaque montée de niveau, **au niveau atteint** : la table se lit jusqu'au niveau
   courant. Tout ce qui lit une fiche — classe d'armure, vitesse, budget de déplacement, profil de
   combat, jets — applique les effets sans connaître la classe (`EX-RPG-023`). Une capacité ou un
   sort que la table nomme et qu'aucun catalogue ne porte n'est pas une erreur mais un
   **avertissement** (`LoadedCharacterSheet::warnings`, `EX-CNT-031`) : les quatre classes livrées
   nomment aujourd'hui des identifiants fusionnés (`sneak-attack-simplified-scoundrel-s-agility`)
   que les lots `LOT-132` à `LOT-135` remplaceront par des capacités du catalogue.
3. **Les crochets du combat.** Le bonus au jet entre par `AttackRollStage::BeforeRoll` au nom de la
   capacité (« + 2 (Coup precis) »). **Aucun crochet n'existait entre la touche et les dés** : une
   étape `Hit` est ajoutée, jamais appelée sur un raté, où une capacité pousse des `bonusDamage`
   que la résolution lance avec ceux de l'arme — du **type de l'arme**, critique compris, nommés
   au journal (« 1d6 : 4 = 4 tranchant (Coup precis) »). « Une fois par tour » se compte dans la
   mémoire du tour (`ScopedCounters`, portée `Turn`, clé = identifiant de la capacité), qu'une
   attaque d'opportunité pendant le tour d'un autre consomme aussi, comme le Manuel le veut pour
   l'attaque sournoise. Une résistance nomme sa source (`DamageAffinity::source`) : la trace écrit
   « resistance (tranchant ; Peau de fer) ». La session lit l'immunité aux attaques d'opportunité
   là où elle lisait le désengagement, et écrit « sans attaque d'opportunite X (Pas de danseur) »
   quand le pas en aurait provoqué une ; au montage, « capacites X : A, B » dit ce que chacun
   apporte, parce que la CA, les résistances et la vitesse sont dans le profil et ne feraient
   sinon aucune ligne.
4. **La classe d'armure se recalcule depuis ses sources** (`EX-CBT-030`) : sans armure,
   `armorClassFor` prend la **meilleure** formule entre la règle générale (10 + Dex) et celle d'une
   capacité, jamais la somme ; le bouclier s'ajoute si la formule le permet ; sous une armure, la
   formule sans armure ne joue pas ; un bonus fixe s'ajoute à toute forme. La fiche d'essai passe
   de 12 à 15 (10 + Dex 2 + Con 3), 17 avec bouclier, 13 en cuir.
5. **L'incantation simplifiée** (`EX-RPG-025`) : la classe déclare `spellcasting` (caractéristique,
   `castsPerDay`) et sa table `cantrips` / `spells` par niveau ; la fiche tient un compte **par
   sort** (`KnownSpell`, `perDay` 0 = à volonté), `longRest` le remet au complet avec les points de
   vie, et monter de niveau **garde** les lancers dépensés — ce n'est pas un repos. Le seul
   mécanisme de sort de ce lot est le **sort à jet d'attaque** (`attackRoll`, `rangeMeters` ajoutés
   au schéma ; `spellAttackFor` : caractéristique d'incantation + maîtrise, dés du sort marqués
   `Spell` et `Magical`, pas de modificateur aux dégâts). Dans l'arène, `castSpell` refuse un sort
   épuisé (`Exhausted`) **avant** toute dépense ; la barre d'actions le propose grisé « Sort : nom
   (0) » pour que le joueur voie ce qu'un repos rendra. Les sorts connus sans mécanisme joué sont
   dits au journal, pas tus (`EX-RPG-051`).
6. **Les maîtrises d'armes viennent de la donnée** : la classe (`weaponProficiencies`, une
   catégorie `simple`/`martial` ou l'identifiant d'une arme) **et** l'espèce (`weaponProficiencies`
   du nain, héritées par le nain des collines) ; `isProficientWith` les lit, une fiche **sans classe**
   maîtrise tout. Le test des fiches préfabriquées du `LOT-130` ne suppose plus la maîtrise : il la
   lit, et les +5, +1, +3 et +5 des pages tiennent — dont le marteau de guerre du prêtre par le nain.
7. **Les quatre classes livrées** reçoivent leurs maîtrises d'armes et d'armures, leurs
   compétences au choix (`skillChoices`) et, pour le Mage et le Priest, l'incantation simplifiée
   (Intelligence et Sagesse, deux lancers par jour), lues aux pages 192 à 206. Leurs tables de
   capacités et de sorts restent aux lots `LOT-132` à `LOT-135` : `Rpg/capacities/` et `Rpg/spells/`
   sont créés avec un `README.md` chacun, vides de données.

## Ce qui n'est pas ici

- Les sorts à **jet de sauvegarde**, de **soin** ou de **condition** (*sacred flame*, *cure
  wounds*, *bless*, *magic missile* sans jet) : leurs mécanismes viennent avec `LOT-133`,
  `LOT-134` et `LOT-137`. La condition « cible adjacente à un allié » de l'attaque sournoise :
  `LOT-135`, sur l'effet `extra-damage`. *Extra Attack* : `LOT-132`.
- Le **report des lancers** de la session sur la fiche entre deux combats, et un geste de repos
  dans le jeu : le héros est rechargé de sa fiche à chaque rencontre, et la partie n'a pas encore
  de journée. La mécanique est dans `Core` (`spendSpellUse`, `longRest`) ; le jeu qui la tient est
  au groupe (`LOT-138`, `LOT-139`).
- Une capture d'écran de la barre d'actions avec un sort : aucune classe livrée n'en a encore ;
  elle vient avec le Mage (`LOT-133`).

## Exigences

- `EX-RPG-024` — une capacité de classe est une liste d'effets nommés ; le journal nomme chaque
  capacité qui a joué.
- `EX-RPG-025` — l'incantation simplifiée : un compte de lancers par sort et par jour, rendu par
  le repos long ; un sort épuisé ne se propose plus.
