+++
id = "LOT-21"
titre = "Attaques, dégâts et états"
version = "0.0.0"
filiere = "regles"
statut = "livre"
taille = "L"
resume = "Une attaque se résout au d20 contre la classe d'armure et ses dégâts typés traversent un pipeline à étapes, selon le chapitre 9 du Manuel des Joueurs."
prerequis = ["LOT-13", "LOT-14", "LOT-20"]
livrables = [
  "`Source/Core/Combat/Damage.{h,cpp}` : `core::DamageClause`, `core::rollDamage`, `core::DamageTraits`, `core::HitPointReserve`, `core::DamagePipeline`.",
  "`Source/Core/Combat/Attack.{h,cpp}` : `core::AttackProfile`, `core::attacksFor`, `core::weaponAttackFor`, `core::AttackRoll`, `core::AttackHooks`, `core::rollAttack`, `core::resolveAttack`, `core::AttackOutcome::describe`.",
  "`core::CombatantProfile::armorClass` et `damageTraits`, `core::CombatState::grantReserve`, les crochets `DamageTaken` et `CombatantDowned`.",
  "Dans l'arène : `core::ArenaSession::attack`, `dodge`, `disengage`, et les attaques d'opportunité de `move` ; le coup d'essai du LOT-50 retiré.",
  "`core::weaponAttackAbility` : la règle Force, Dextérité, finesse écrite une seule fois.",
  "Sept tests de dégâts, six d'attaque, un d'arène.",
]
criteres = [
  "Un 20 naturel double les dés et non le modificateur, vérifié par test.",
  "Un 1 naturel rate, même avec un total supérieur à la CA.",
  "PV bornés à 0.",
  "Inconscience et jets de sauvegarde contre la mort testés aux bornes.",
  "Résistances, vulnérabilités et immunités appliquées dans le bon ordre.",
  "Chaque jet produit une entrée de journal complète et lisible.",
  "`ctest` : 1120/1120 (1107 avant ; sept tests de dégâts, six d'attaque, un d'arène pour l'opportunité ; le test des kits du coup d'essai est retiré, celui du coup est réécrit).",
]
+++

## Pourquoi

Résoudre une attaque au d20 contre la classe d'armure, et appliquer ses dégâts — selon le **Manuel
des Joueurs, chapitre 9** (« Effectuer une attaque », « Dégâts et guérison »), qui fait foi pour
chaque règle de ce lot. Les sorts n'en sont pas : ils arriveront de manière exhaustive avec les
classes.

## Périmètre

- **Les dégâts** (`Source/Core/Combat/Damage.{h,cpp}`) : clauses typées et drapeaux de source
  (`core::DamageClause`, `core::DamageFlag`), le lancer qui double les dés d'un critique
  (`core::rollDamage`), les affinités d'une cible et ce qui les contourne (`core::DamageTraits`),
  les **réserves** de points de vie (`core::HitPointReserve`), et le **pipeline à étapes nommées**
  (`core::DamagePipeline`) qui se termine dans `core::CombatState::applyDamage` — pour un combattant
  comme pour une structure de la grille.
- **L'attaque** (`Source/Core/Combat/Attack.{h,cpp}`) : les profils (`core::AttackProfile`) tirés
  du bestiaire (`core::attacksFor`) ou de l'arme d'une fiche (`core::weaponAttackFor`), la distance
  entre emprises (`core::gridDistance`), l'allonge (`core::inReach`), les circonstances que la
  grille sait dire (`core::attackCircumstances`), le **jet amendable** (`core::AttackRoll`,
  `core::AttackHooks`, `core::rollAttack`), la résolution dans le combat (`core::resolveAttack`) et
  son **entrée de journal** (`core::AttackOutcome::describe`).
- **Dans le combat** : la classe d'armure et les affinités sur le profil
  (`core::CombatantProfile::armorClass`, `damageTraits`), les réserves sur le combattant
  (`core::CombatState::grantReserve`), et deux crochets de plus — `DamageTaken` et
  `CombatantDowned` — qui portent les PV avant et après, l'excédent et le critique.
- **Dans l'arène** : le coup d'essai du [LOT-50](LOT-50-colisee.md) est **retiré** ;
  `core::ArenaSession::attack` joue l'action *attaquer*, `dodge` et `disengage` les actions
  *esquiver* et *se désengager*, et `move` déclenche les **attaques d'opportunité**. L'écran gagne
  deux boutons ; le personnage de démonstration frappe avec son épée longue, contre la CA que son
  équipement lui donne.
- **Autour** : la règle « Force au corps à corps, Dextérité à distance, la meilleure des deux en
  finesse » n'est plus écrite qu'une fois (`core::weaponAttackAbility`, que la fiche et l'attaque
  lisent toutes deux).

Il ne livre **pas** l'agonie : ni inconscience, ni jets contre la mort, ni mort instantanée, ni coup
qui assomme. La feuille de route les avait fusionnés dans le `LOT-72` à l'audit, et le critère
d'acceptation du lot qui les réclamait encore est une trace de l'état antérieur ; ce lot s'arrête
aux PV bornés à 0 et **rapporte** ce dont l'agonie aura besoin (ci-dessous). Il ne livre pas non
plus la portée à distance, la ligne de vue ni l'abri (`LOT-22`).

### Ce qui reste hors du lot, nommément

- **L'agonie et la mort**, l'état inconscient, le coup qui assomme, la surprise : `LOT-72`.
- **La portée, la ligne de vue, l'abri**, voir sans être vu : `LOT-22`.
- **Combat à deux armes** (l'attaque bonus sans modificateur de caractéristique), **empoigner** et
  **bousculer** (des oppositions), **aider**, **se tenir prêt**, **se précipiter**, **se cacher** : aucun
  n'a encore d'usage sans les classes, les conditions ou l'IHM de combat qui les proposera.
- **Le combat monté et le combat sous-marin** : sans monture ni carte immergée, rien à éprouver.
- **Les sorts**, et les attaques qu'ils demandent : avec les classes (`LOT-25`, `LOT-35`).

## Conception

### Les règles du Manuel, et où chacune vit

| Règle (chapitre 9) | Où | Vérifiée par |
|---|---|---|
| d20 + modificateurs ≥ CA : touché | `core::rollAttack` | `AttackTest.ChaqueJetProduitUneEntreeDeJournalComplete` |
| Force au contact, Dextérité à distance, finesse au choix ; maîtrise si l'arme est maîtrisée ; même modificateur aux dégâts | `core::weaponAttackFor` | `AttackTest.LesProfilsSeTirentDuBestiaireEtDeLaFiche` |
| Mains nues : 1 + Force, contondant, toujours maîtrisé | `core::weaponAttackFor(sheet, nullptr, …)` | idem |
| Un 20 touche « peu importe les modificateurs ou la CA » ; un 1 rate | `core::rollAttack` | `AttackTest.UnVingtNaturelToucheEtDoubleLesDes`, `UnUnNaturelRateMemeAuDessusDeLaCA` |
| Critique : tous les dés deux fois, modificateurs une fois | `core::rollDamage` | `DamageTest.LeCritiqueDoubleLesDesPasLeModificateur` |
| Des dégâts jamais négatifs | `core::rollDamage` | idem |
| Des dégâts sur plusieurs cibles se lancent une fois | `core::DamagePipeline::apply` (une salve, un jet) | `DamageTest.UneSalveSeLanceUneFoisEtSAppliqueDUnCoup` |
| Résistance **puis** vulnérabilité, après tous les autres modificateurs ; plusieurs résistances comptent une fois | `DamagePipeline`, étape `Resistances` | `DamageTest.LesResistancesSAppliquentDansLOrdreDuManuel` (l'exemple du livre : 25 − 5, puis la moitié, 10) |
| PV entre 0 et le maximum | `core::CombatState` | `DamageTest.LesPointsDeVieSontBornesAZeroEtLExcedentEstRapporte` |
| Points de vie temporaires : perdus d'abord, non cumulables, pas soignés, absorbent à 0 PV sans relever | `core::CombatState::grantReserve`, étape `Reserves` | `DamageTest.LesPointsDeVieTemporairesAbsorbentDAbordEtNeSeCumulentPas` |
| Tir au contact d'un ennemi, au-delà de la portée normale : désavantage | `core::attackCircumstances` | `AttackTest.LaGrilleDitLaPorteeEtLesCirconstances` |
| Attaque d'opportunité à la sortie de l'allonge, par la réaction ; se désengager l'évite | `core::ArenaSession::move` | `ArenaTest.LOpportuniteLeDesengagementEtLEsquive` |
| Esquiver : désavantage aux attaques contre soi jusqu'au début de son tour | `core::ArenaSession::dodge` | idem |

### Un jet qui est un objet

Le §4bis de la feuille de route demandait que le résultat d'un jet soit un objet que des crochets
lisent et amendent **avant** que l'issue ne soit figée. `core::AttackRoll` est cet objet, et
`core::AttackHooks` lui offre trois instants, dans l'ordre :

- `BeforeRoll` — ajouter une source d'avantage ou de désavantage (nommée : c'est ce que le journal
  écrit), un bonus, changer la CA visée (l'abri du `LOT-22`), baisser le seuil critique (le
  Champion) ;
- `DiceRolled` — lire les d20 **bruts**, en relancer un (`reroll`), en substituer un résultat
  stocké (`substitute`) ;
- `BeforeOutcome` — ajouter un modificateur après avoir vu le total (`addModifier`).

L'issue n'est figée qu'après : un 1 rate, un résultat au moins égal au seuil critique touche et
est critique, sinon le total se compare à la CA. Chaque amendement s'inscrit dans le jet. Le même
point d'insertion rend les tests écrivables : un « 20 naturel » y est un greffon qui substitue le
dé, pas une graine cherchée à la main.

### Un pipeline, pas une soustraction

Les dégâts traversent cinq étapes nommées (`core::DamageStage`), chacune point d'insertion :
**source** (drapeaux), **conversion** (type), **résistances** (immunité, puis résistance, puis
vulnérabilité — les greffons s'exécutent *avant* le moteur, ce qui place une réduction fixe là où
le Manuel la veut), **réserves** (la plus récente absorbe d'abord), **points de vie**. Chaque
changement de montant s'inscrit dans une trace (`core::DamageWork::adjust`), que le journal restitue
: « résistance (contondant) 20 -> 10 ».

- **Une salve, un appel.** Le pipeline accumule les pertes de toutes ses cibles et les applique en
  **un** `CombatState::applyDamage(span)` : la règle du [LOT-20](LOT-20-initiative-tour-par-tour.md) — une zone qui abat les
  deux camps est une défaite quel que soit l'ordre des cibles — tient donc aussi pour des dégâts
  résistés, absorbés ou convertis.
- **Contournables.** Une affinité nomme les drapeaux qui la contournent : « résistance aux dégâts
  contondants non magiques » s'écrit `{Bludgeoning, Resistance, Magical}`. Le bestiaire du SRD ne
  porte que des listes nues, qui deviennent des affinités sans contournement ; la condition reste
  dans le texte tant que la donnée ne l'écrit pas.
- **Les structures ont des PV.** `core::GridObject` porte ses affinités, et
  `DamagePipeline::applyToStructure` les fait traverser les mêmes étapes, réserves exceptées.
- **Aucun type par défaut** (`EX-CBT-032`). Une action de bestiaire à bonus d'attaque dont les dégâts
  ne seraient pas typés est **refusée** par `core::attacksFor`, qui le nomme ; un test charge les 94
  créatures livrées et n'en trouve aucune.

### Ce que l'agonie trouvera en place

`core::CombatEvent` porte, pour `DamageTaken` et `CombatantDowned`, les PV avant et après, le
maximum, l'**excédent** au-delà de 0 et le drapeau **critique**. C'est exactement ce que le chapitre
9 demande au `LOT-72` : la mort instantanée quand l'excédent atteint le maximum (l'exemple du clerc
à 6 PV sur 12 qui en subit 18 est rejoué par test), un échec de jet contre la mort pour des dégâts
subis à 0 PV — `DamageTaken` est annoncé même à 0 —, deux pour un critique. `core::crossedBelow`
dit qu'un seuil a été **franchi** (*Battle Fury* sous 50 %), une fois.

## Décisions de réalisation

- **La CA est un nombre du profil, calculé à sa construction.** `EX-CBT-030` interdit une CA
  accumulée ; le profil la prend de ce qui la produit — `core::derivedStatsFor` pour une fiche (le
  personnage de démonstration passe de sa CA sans armure à celle de son armure de cuir clouté et de
  son bouclier), le bloc pour une créature — et ce qui la change en combat passe par le jet, jamais
  par ce nombre.
- **Les maîtrises d'armes sont supposées.** Les classes provisoires du `LOT-36` ne les déclarent pas :
  `weaponAttackFor` prend un booléen, que l'arène passe à vrai jusqu'au socle de classe (`LOT-47`).
- **Une allonge de bestiaire en mètres devient des cases** arrondies, au moins une : 3 m font 2 cases,
  et une allonge de 0 m (une nuée) frappe au contact, deux créatures ne partageant jamais une case.
  Une action **sans** allonge est une attaque à distance.
- **Sans portée connue, une attaque à distance ne vise qu'au contact** — avec le désavantage du tir
  au contact. Aucune arme ni aucun bloc ne structure encore ses portées ; lire « portée 24/96 m »
  dans la prose est ce que le projet refuse, et c'est le `LOT-22` qui les structurera.
- **Points de vie temporaires : le moteur garde le plus grand.** Le Manuel laisse le joueur choisir
  entre garder ou remplacer ; tant qu'aucune réserve n'a de durée, garder le plus grand est le seul
  choix raisonnable.
- **Une cible à terre n'est pas attaquable.** `declareAttack` le refuse depuis le `LOT-20` ;
  l'avantage contre un inconscient et le critique au contact sont des effets d'état, au `LOT-72`.
- **L'arène prend toutes les attaques d'opportunité.** Il n'y a ni interface pour décliner
  (`LOT-24`) ni comportement pour choisir (`LOT-23`). Le déplacement s'arrête à la dernière case où
  l'on peut se tenir avant la sortie — jamais sur un allié qu'on traverse —, les attaques se jouent
  par identifiant croissant, et le déplacement reprend si le combattant tient debout.
- **Le journal écrit l'attaque à sa place.** Sa ligne se réserve après la déclaration et avant les
  dés, et se remplit une fois l'attaque résolue : la chute, l'issue et la Marque Héroïque qu'elle
  déclenche s'écrivent après elle, dans l'ordre où c'est arrivé.
- **Le bestiaire ne type que la première clause** d'une action. Le moteur sait porter plusieurs
  clauses et le test du critique le montre ; c'est la donnée qui n'en fournit qu'une (le serpent
  venimeux géant perd ainsi son poison), et l'extraire est l'affaire du contrôle de cohérence
  (`LOT-49`) ou des créatures de Tanares (`LOT-46`).

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔ Sur deux cents graines, 1d6+3 critique tient entre 5 et 15 ; la même règle se vérifie dans une attaque résolue.
2. ✔ 31 contre CA 5 : raté.
3. ✔ Aux trois bornes — un PV de moins que le total, le total, au-delà — avec la chute annoncée une fois et l'excédent rapporté.
4. ✘ Au `LOT-72`, où la feuille de route les a fusionnés : ce critère est antérieur à la fusion. Ce que l'agonie lira est livré et testé.
5. ✔ L'exemple du Manuel, résistant et vulnérable, deux résistances, immunité, contournement, source qui ignore les résistances.
6. ✔ Comparée mot pour mot, touchée et ratée.
7. ✔

**Deux défauts réintroduits à la main** — doubler le modificateur d'un critique, appliquer la vulnérabilité avant la résistance — font échouer, le premier deux tests, le second le test d'ordre.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1120/1120 en Debug et en Release, `clang-format`, les lints, la Doxygen et le cahier de test verts ; deux défauts réintroduits à la main font chacun échouer leurs tests ; écran de l'arène capturé.

Alimente [LOT-22](LOT-22-portee-ligne-de-vue.md), [LOT-23](LOT-23-ia-tactique.md), [LOT-24](LOT-24-ihm-combat.md), `LOT-25`, [LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27), `LOT-47` et `LOT-72`.

Exigences couvertes : `EX-CBT-030` (l'attaque contre une CA recalculée depuis ses sources), `EX-CBT-031` (le critique double les dés), `EX-CBT-032` (dégâts typés, jamais de type par défaut), et `EX-REG-003` pour chaque attaque. Aucune exigence ajoutée.
