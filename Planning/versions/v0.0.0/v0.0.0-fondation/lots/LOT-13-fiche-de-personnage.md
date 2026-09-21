+++
id = "LOT-13"
titre = "Fiche de personnage"
version = "0.0.0"
filiere = "regles"
statut = "livre"
taille = "M"
resume = "Toute créature — héros, PNJ, ennemi — reçoit une fiche complète construite depuis une espèce, une classe et un historique, et monte de niveau sans qu'aucune valeur de règle soit écrite en C++."
prerequis = ["LOT-12"]
livrables = [
  "`core::CharacterSheet` (`Source/Core/Rpg/CharacterSheet.{h,cpp}`) : la fiche, sa construction, les modificateurs de sauvegarde et de compétence, le gain d'expérience et la montée de niveau.",
  "`core::SkillCatalog` (`Source/Core/Rpg/Skill.{h,cpp}`) : les dix-huit compétences et la caractéristique de chacune.",
  "`rules/experience.json` (vingt niveaux, seuils et bonus de maîtrise) et `rules/character-creation.json` (CA sans armure, plafond de caractéristique), avec leurs deux schémas.",
  "Le composant ECS `core::RpgActor` (`Source/Core/Ecs/Components/RpgActor.h`), qui désigne la fiche par un indice.",
  "Neuf tests, un par phrase de l'acceptation.",
]
criteres = [
  "**Trois classes définies en JSON se chargent et donnent les bons modificateurs** — brawler (d12), mage (d6) et priest (d8) : points de vie, jets de sauvegarde maîtrisés **et** non maîtrisés. ✔",
  "**Montée de niveau reproductible et testée aux bornes** : le seuil **exact** fait monter, un point de moins ne fait pas monter, le seuil du niveau 5 fait passer directement au niveau 5, et donner le même total en une ou deux fois donne la même fiche. ✔",
  "**Aucune valeur de règle codée en dur** : le test lit les constantes dans la donnée et les compare à la fiche — s'il portait sa propre copie de la règle, il resterait vert tout en la contredisant. ✔",
  "**Quatre fiches indépendantes** : blesser l'une et faire monter l'autre ne touche ni la troisième ni la quatrième. ✔",
  "`ctest` : **992/992** (983 avant le lot, plus 9). ✔",
]
sources = ["Basic Rules, p. 10 (classe d'armure sans armure)", "Basic Rules, p. 11 (table d'expérience, plafond de caractéristique, points de vie)"]
+++

## Pourquoi

Donner à toute créature — héros, PNJ, ennemi — une fiche complète : caractéristiques, points de
vie, classe d'armure, niveau, bonus de maîtrise, jets de sauvegarde, compétences, vitesse. Et la
faire **monter de niveau**.

## Périmètre

### Ce que le lot livre

**`core::CharacterSheet`** (`Source/Core/Rpg/CharacterSheet.{h,cpp}`) : la fiche, sa construction
depuis une espèce, une classe et un historique, les modificateurs de sauvegarde et de compétence,
le gain d'expérience et la montée de niveau.

**`core::SkillCatalog`** (`Source/Core/Rpg/Skill.{h,cpp}`) : les dix-huit compétences du `LOT-43`
et la caractéristique de chacune.

**Deux tables de règles extraites du corpus** — `rules/experience.json` (vingt niveaux, leurs
seuils et leurs bonus de maîtrise) et `rules/character-creation.json` (la classe d'armure sans
armure et le plafond d'une valeur de caractéristique) — avec leurs deux schémas.

**Le composant ECS `core::RpgActor`** (`Source/Core/Ecs/Components/RpgActor.h`).

**Neuf tests**, un par phrase de l'acceptation.

### Ce que le lot ne fait pas

**Il ne calcule pas la classe d'armure d'une armure portée.** Une fiche sans équipement a la CA que
le livre lui donne ; les armures sont le [LOT-34](LOT-34-equipement.md), et l'inventaire le
[LOT-14](LOT-14-inventaire-et-equipement.md).

**Il n'attribue pas les capacités de classe.** La progression porte leurs identifiants, niveau par
niveau ; les jouer viendra avec les mécanismes qui les portent.

**Il n'a pas d'interface.** La fiche est un objet du `Core`, sans fenêtre : sa maquette et son écran
sont le [LOT-38](LOT-38-fiche-de-personnage.md).

## Conception

### Le lot n'a pas écrit de `ClassDefinition` : elle existait déjà

Le périmètre annonçait un `ClassDefinition.{h,cpp}` « piloté par JSON, jamais codé en dur : dé de
vie, maîtrises, progression par niveau ». C'est exactement `core::PlayableClass`, que le
[LOT-36](LOT-36-especes-historiques-classes.md) a livré en chargeant `Source/Elements/Rpg/classes/` — dé de vie,
caractéristique principale, jets de sauvegarde et vingt lignes de progression.

Écrire un second type pour la même chose aurait créé deux vérités sur ce qu'est une classe, et la
divergence se serait vue le jour où l'une des deux aurait appris quelque chose que l'autre ignore.
La fiche **consomme** `PlayableClass` ; le lot n'a rien réécrit.

### « Aucune valeur de règle dans le C++ » se vérifie sur le diff, pas sur l'intention

C'est le critère d'acceptation le plus facile à croire tenu, et le plus facile à trahir : une
valeur de règle a l'air d'une constante d'implémentation, et rien ne les distingue une fois
écrites. Ce lot en a trouvé **trois**, dont une qu'il avait lui-même introduite au lot précédent.

**La table d'expérience** — vingt seuils et vingt bonus de maîtrise — tiendrait en trois lignes de
C++, et c'est ce qui la rend dangereuse. Elle est extraite de la table des *Basic Rules* p. 11, lue
**par coordonnée** (`EX-CNT-021`) : ses trois colonnes n'ont ni filet ni séparateur, et un mode en
flux y mêlerait les seuils aux numéros de niveau. Deux contrôles arrêtent la génération — les vingt
niveaux doivent être présents **et dans l'ordre**, les seuils **strictement croissants**. Deux
seuils inversés rendraient une montée de niveau infranchissable, ou franchissable deux fois.

**La classe d'armure sans armure et le plafond d'une caractéristique** sont deux nombres — 10 et
20 — et ils valaient d'être extraits : un `10` nu dans un calcul de CA ne dit pas ce qu'il
représente. Chacun est **cherché dans la phrase qui l'atteste**, et la phrase est écrite dans la
donnée produite :

> « Sans armure, ni bouclier, la CA de votre personnage est égale à **10** + son modificateur de
> Dextérité. » — *Basic Rules* p. 10
>
> « Vous ne pouvez pas augmenter une valeur de caractéristique au-delà de **20**. » — p. 11

C'est ce qui distingue une constante extraite d'une constante tapée de mémoire : la seconde a
l'air de la première, et rien ne les sépare une fois écrites.

**Le `20` que le `LOT-36` avait codé en dur.** `abilityScoreWith()` bornait une augmentation
d'espèce à un `constexpr int VALEUR_MAXIMALE = 20`. Le plafond est désormais un **paramètre**, lu
dans la donnée — et le test du `LOT-36` a été repris pour le lire au même endroit que le moteur,
au lieu de vérifier sa propre copie de la règle.

**Une fiche se construit, elle ne se déclare pas.** Tous les champs chiffrés de `CharacterSheet`
partent de zéro, et aucun ne porte de valeur « raisonnable » par défaut : un `armorClass = 10` dans
la structure serait la règle du livre écrite en C++, et une fiche à demi construite passerait pour
une fiche jouable.

### `CharacterSheet` est un objet autonome, jamais un singleton joueur

La décision de cadrage est « un héros au départ, quatre à terme » : le passage au groupe
([LOT-29](../../../../vision/archives/feuille-de-route-jeu.md#lot-29)) ne doit **rien** changer à ce type. Aucune fonction du fichier ne prend
« le personnage » implicitement — toutes reçoivent la fiche sur laquelle elles travaillent.

Le test `QuatreFichesIndependantes` est ce qui rend la règle exécutoire : il construit quatre
fiches, en blesse une, en fait monter une autre de deux niveaux, et vérifie que les deux dernières
n'ont pas bougé. Si `CharacterSheet` devenait un singleton, ce cas serait le premier à tomber.

**Le bonus de maîtrise n'est pas dans la fiche.** Il se lit dans la table d'expérience au niveau
courant. Le stocker le figerait au moment de la construction, et une montée de niveau laisserait
un personnage avec le bonus de l'ancien — un défaut qui se joue et ne se voit pas.

### Le composant ECS ne porte pas la fiche, il la désigne

`RpgActor` porte un **indice**, pas une `CharacterSheet`. La raison de taille est secondaire ; la
vraie est qu'**une fiche n'appartient pas à une entité**. Un personnage du groupe garde la sienne
quand il change de carte et que son entité est détruite puis recréée ([LOT-09](LOT-09-colisee-premiere-carte.md)) : une
fiche survit à l'entité qui la représente. Loger la fiche dans le composant lierait la vie de l'une
à celle de l'autre, et le `LOT-29` devrait défaire ce lien.

`INDICE_ABSENT` distingue une entité **sans fiche** — un décor animé, un projectile — d'une entité
dont la fiche serait la première du registre. Les confondre ferait attaquer un tonneau avec les
caractéristiques du héros.

### Trois décisions de règle, écrites là où on les lit

**Les points de vie sont déterministes.** Le livre laisse le choix entre lancer le dé de vie et
prendre « la valeur fixe indiquée dans la description de votre classe, qui se trouve être la valeur
moyenne (arrondie au supérieur) du dé » (p. 11). Le moteur prend la seconde : des points de vie
tirés au dé rendraient une partie irrejouable, ce qu'`EX-NFR-002` interdit. Le niveau 1 reçoit le
**maximum** du dé — c'est la règle, et la raison pour laquelle un mage de niveau 1 n'a pas trois
points de vie.

**Monter de niveau n'est pas un soin.** Les points de vie courants montent du **gain**, pas jusqu'au
maximum : rendre toute sa vie à un personnage blessé ferait de la montée de niveau une potion
gratuite.

**Perdre de l'expérience n'est pas une règle de ce jeu.** Un gain négatif est ignoré. L'accepter en
silence ferait *descendre* un personnage de niveau, et le défaut passerait pour de l'équilibrage.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à
992/992, `clang-format`, les six lints, cahier de test et Doxygen verts. Alimente
[LOT-14](LOT-14-inventaire-et-equipement.md), [LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27), [LOT-38](LOT-38-fiche-de-personnage.md), [LOT-74](../../../../vision/archives/feuille-de-route-jeu.md#lot-74).

Exigences couvertes : [`EX-REG-010`](../../../../../Documentation/Specification/regles-d20.md#EX-REG-010), [`EX-REG-011`](../../../../../Documentation/Specification/regles-d20.md#EX-REG-011),
[`EX-RPG-001`](../../../../../Documentation/Specification/rpg.md#EX-RPG-001).
