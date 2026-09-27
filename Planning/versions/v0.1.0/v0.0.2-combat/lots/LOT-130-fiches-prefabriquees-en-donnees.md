+++
id = "LOT-130"
titre = "Les quatre fiches préfabriquées, en données"
version = "0.0.2"
filiere = "regles"
statut = "livre"
taille = "M"
resume = "Brawler, Mage, Priest et Scoundrel de niveau 1 existent comme fiches du jeu, valeur pour valeur, avec les espèces et les historiques qu'elles demandent."
prerequis = ["LOT-122"]
reprend = ["LOT-36 (classes provisoires)"]
livrables = [
  "`Rpg/characters/` : les quatre fiches (p. 195, 199, 203, 207).",
  "Ce qui leur manque dans les catalogues : les espèces **Half-Orc**, **Autumn Elf**, **Hill Dwarf** ; les historiques **Dragon Hunter**, **Cartographer**, **Community Leader**, **Undercover**.",
  "Un test par fiche qui recalcule chaque valeur dérivée (CA, PV, initiative, sauvegardes, compétences, attaques) et la compare à la page.",
  "Le registre des **coquilles du livre** et de la valeur retenue pour chacune.",
]
criteres = [
  "Les quatre tests passent ; chaque écart entre la règle et la fiche imprimée est une décision écrite.",
  "`check_rpg_data.py` est vert.",
]
sources = ["Player's Guide to Tanares, p. 195, 199, 203, 207"]
+++

## Ce que les fiches disent — et ne disent pas

Les quatre fiches sont de **niveau 1**. Elles portent caractéristiques, compétences, sauvegardes,
PV, CA, attaques et maîtrises ; elles **ne listent ni capacités, ni sorts, ni équipement** : les
cadres « Power Symbols » et « Heroic Mark » sont vides. Le reste se déduit des règles de la classe
— voir [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).

## Coquilles à trancher

- Scoundrel : vitesse imprimée 30 ft, la règle donne **40** (*Scoundrel's Agility*) ; Perception
  passive imprimée 13, le calcul donne **14**.
- Priest : la hachette est imprimée « Piercing » ; le type normal est **tranchant**.
- Mage : *fire bolt* manque aux attaques imprimées alors que la classe le donne.
- Brawler : le texte cite une capacité *Resilient* absente de la table — la table fait foi.

Proposé : **la règle prime sur la fiche imprimée**, et chaque écart est écrit dans le registre.

## Décisions de réalisation

Livré le 27 septembre 2026 (**PR #146**). **La règle prime sur la fiche imprimée**, et chaque écart est écrit
ci-dessous ; là où le moteur ne sait pas encore jouer la règle, le test dit la valeur du moteur
d'aujourd'hui et nomme le lot qui la changera.

1. **Les catalogues avaient déjà les sept entrées.** Les trois espèces (`demi-orc`,
   `elfe-d-automne`, `nain-des-collines`) et les quatre historiques (`dragon-hunter`,
   `cartographer`, `community-leader`, `undercover`) sont ceux du `LOT-36`. Deux ont été retouchés :
   le nain des collines reçoit sa *Ténacité naine* en **nombre** (`hitPointsPerLevel: 1`,
   `species.schema.json`), comme les augmentations de caractéristique sont une table — le moteur ne
   lit pas la prose des traits, et le prêtre affichait 11 PV pour les 12 du livre ; le texte du trait
   *Sauvagerie* du demi-orc, qui embarquait une demi-page de l'OCR sur les gnomes, est ramené à sa
   phrase.
2. **Une sous-espèce hérite de son parent.** `parentSpecies` était une étiquette : le nain des
   collines n'avait ni les +2 de Constitution ni les langues du nain, l'elfe d'automne ni les +2 de
   Dextérité ni la Perception de l'elfe — aucune fiche ne les avait encore jouées. Au chargement
   (`loadCharacterOptions`), la sous-espèce reçoit désormais les augmentations, les points de vie
   par niveau, les langues, les traits et les mécanismes requis de son parent ; le fichier ne porte
   que ce qu'elle ajoute, comme le livre l'écrit.
3. **Trois fichiers de plus dans `Rpg/characters/`** — `heros-mage.json`, `heros-priest.json`,
   `heros-scoundrel.json` — à côté du `heros-brawler.json` du `LOT-112`, qui est déjà la page 195.
   Le livre ne les nomme pas : **Faelar Trace-Carte**, **Helga Pierre-Sûre** et **Nessa Double-Vie**
   sont des noms proposés, qui se changent sans rien casser. Les quatre sont **provisoires** avec
   le critère de leur classe (le garde-fou `LesClassesProvisoiresNeSontReferenceesParRien`
   l'exige), et la démo continue de charger le seul Brawler.
4. **Les six valeurs de base se déduisent de la page** : la fiche porte les valeurs *avant*
   augmentation d'espèce (`baseAbilities`), et l'espèce les augmente au chargement. Les trois
   premières fiches sont la série standard 15 14 13 12 10 8 ; le Scoundrel est un achat de 27 points.
5. **Le +1 au choix de l'elfe d'automne** — « *vous pouvez aussi augmenter une autre
   caractéristique de votre choix de 1, sauf Dextérité et Intelligence* » (p. 18) — est un choix de
   la **fiche**, pas de l'espèce : nouveau champ `speciesAbilityChoice` du `character.schema.json`,
   appliqué après la table de l'espèce sous le même plafond (`buildCharacterSheet`, dernier
   paramètre). L'espèce garde son `mecanismesRequis` : le moteur applique un choix fait, il ne sait
   pas encore l'**offrir**.
6. **Les maîtrises de compétence que l'espèce accorde** (Intimidation du demi-orc, Perception de
   l'elfe) restent écrites dans `skillProficiencies` de la fiche, comme le `LOT-112` l'avait fait :
   le moteur ne lit pas les traits. Les compétences de l'historique, elles, viennent de son
   catalogue et ne sont pas répétées.
7. **Le test recalcule tout ce que la page imprime** (`test_premade_characters.cpp`, un cas par
   fiche) : caractéristiques finales, PV, CA (`derivedStatsFor`, armure et bouclier compris),
   initiative, vitesse, Perception passive, les six sauvegardes, les **dix-huit** compétences
   (maîtrisées ou non — une valeur fausse sur une compétence non maîtrisée trahit une
   caractéristique fausse), et chaque attaque d'arme avec son bonus, ses dés, son type et ses
   portées en cases. Les armes sont tenues pour maîtrisées : les classes simplifiées ne déclarent
   pas encore leurs maîtrises d'armes (`LOT-131`).

### Registre des coquilles du livre et des écarts

| # | Fiche | La page imprime | La règle donne | Retenu | Où c'est joué |
|---|---|---|---|---|---|
| 1 | Brawler p. 195 | CA **14** | 10 + Dex 1 + Con 3, *Tough as Nails* | **14** | *Tough as Nails*, jouée depuis le `LOT-132` ; le test lit 14 |
| 2 | Brawler p. 195 | le texte cite *Resilient* | la table ne l'a pas | **la table fait foi** | rien à jouer |
| 3 | Mage p. 199 | Con **14** | +1 au choix hors Dex/Int, non appliqué | **Con 15**, le +1 va en Constitution : aucun modificateur ne change, tout le reste de la page reste juste | `speciesAbilityChoice`, joué |
| 4 | Mage p. 199 | quarterstaff seul | la classe donne *fire bolt* (+5, 1d10 feu, 120 ft) | **fire bolt** | sorts : `LOT-133` |
| 5 | Mage p. 199 | 1d6−1 / 1d8−1 | polyvalent à deux mains | **les deux** | le moteur ne joue pas la propriété *polyvalent* ; le test vérifie 1d6−1 |
| 6 | Priest p. 203 | handaxe « Piercing » | la table des armes : **tranchant** | **tranchant** | catalogue, joué |
| 7 | Priest p. 203 | Common, Dwarvish | Community Leader accorde **deux langues** au choix | **aucune choisie** — le jeu n'en a pas besoin, le commun suffit aux dialogues | à choisir le jour d'une création |
| 8 | Priest p. 203 | Sacred Flame (60 ft, 1d8 radiant, sauvegarde de Dex) | cantrip de la classe | **sacred flame** | sorts : `LOT-134` |
| 9 | Scoundrel p. 207 | vitesse **30 ft** | *Scoundrel's Agility* : +10 ft dès le N1 | **40 ft** | capacité de classe : `LOT-135` ; le moteur donne 30 ft d'ici là, le test le dit |
| 10 | Scoundrel p. 207 | Perception passive **13** | 10 + Perception +4 | **14** | joué |
| 11 | Toutes | alignement, personnalité, idéaux, liens, défauts | — | **non repris** : aucun mécanisme ne les lit (`EX-REG-041`) | — |

## Ce qui n'est pas ici

- Les capacités de niveau 1 (*Tough as Nails*, *Scoundrel's Agility*, *Sneak Attack Simplified*,
  les cantrips) et les sorts : c'est l'objet des lots `LOT-131` à `LOT-135`. Le jour où ils sont
  livrés, les deux écarts n° 1 et n° 9 se ferment en changeant la valeur attendue du test.
- L'équipement de départ complet des classes (sacs, outils de voleur, kit de déguisement) : les
  fiches ne portent que les armes et armures que leurs attaques et leur CA supposent.
- Une fiche jouée par la démo autre que le Brawler : le groupe de quatre est le `LOT-138`.

## Exigences

- `EX-CNT-031` — l'augmentation au choix reste un mécanisme requis déclaré par l'espèce.
- `EX-CNT-032` — les quatre fiches portent leur statut provisoire et son critère de retrait.
