+++
id = "LOT-12"
titre = "Dés, caractéristiques, jets"
version = "0.0.0"
filiere = "regles"
statut = "livre"
taille = "M"
resume = "Le cœur chiffré du système existe dans `Core` : dés déterministes, six caractéristiques, modificateurs et jet de d20 avec avantage, désavantage et restitution complète."
prerequis = []
livrables = [
  "`Core/Rpg/Dice.{h,cpp}` — la notation `NdF±M`, son analyse et son évaluation.",
  "`Core/Rpg/Ability.h` et `Ability.cpp` — les six caractéristiques et leur modificateur.",
  "`Core/Rpg/Check.{h,cpp}` — le jet de d20, avantage, désavantage, restitution (`CheckResult`, `rollStance`).",
  "`Core/Rpg/Scale.h` — une case vaut 1,5 m, figé là et nulle part ailleurs.",
  "`DeterministicRandom::nextInt` — tirage entier **sans biais modulo**.",
  "`Source/Elements/Rpg/rules/difficulty.json` — les six degrés de difficulté, en **donnée**.",
  "14 tests.",
]
criteres = [
  "Distribution vérifiée sur **100 000 tirages** à graine fixe : les vingt faces sortent, aucune ne s'écarte de plus de 10 % de la moyenne. ✔",
  "**Rejouabilité stricte** : même graine → même séquence, dé par dé ; une graine différente donne une séquence différente. ✔",
  "Avantage = max de deux d20, désavantage = min ; les deux ensemble **s'annulent**, y compris à 2 contre 1. ✔",
  "1 et 20 naturels détectés et **distingués** d'un total de 1 ou 20. ✔",
  "Aucune dépendance ECS, Qt ou rendu : `Core/Rpg/` compile seul. ✔",
  "Une notation mal formée est refusée — dont `ld8`, la faute d'OCR que le [LOT-30](@ref lot-30) a documentée. ✔",
  "`ctest` : **968/968** (954 avant le lot, plus 14). ✔",
]
sources = ["Basic Rules, p. 64 (table « Tâche / DD »)"]
+++

## Pourquoi

Le cœur chiffré du système : dés déterministes, six caractéristiques, modificateurs, et le jet de
d20 avec avantage et désavantage contre un seuil. Du calcul pur, dans `Core`, testable sans fenêtre
ni GPU.

## Périmètre

### Ce que le lot livre

- **`Core/Rpg/Dice.{h,cpp}`** — la notation `NdF±M`, son analyse et son évaluation.
- **`Core/Rpg/Ability.h` et `Ability.cpp`** — les six caractéristiques et leur modificateur.
- **`Core/Rpg/Check.{h,cpp}`** — le jet de d20, avantage, désavantage, restitution.
- **`Core/Rpg/Scale.h`** — une case vaut 1,5 m, figé là et nulle part ailleurs.
- **`DeterministicRandom::nextInt`** — tirage entier **sans biais modulo**.
- **`Source/Elements/Rpg/rules/difficulty.json`** — les six degrés de difficulté, en **donnée**.
- 14 tests.

### Ce que le lot ne fait pas

**Il ne connaît ni personnage ni créature.** `rollCheck` prend un seuil et des modificateurs ; d'où
ils viennent est l'affaire du [LOT-13](@ref lot-13). `Core/Rpg/` ne dépend ni de l'ECS, ni de Qt,
ni du rendu.

**Il n'applique aucune règle de critique.** `isNaturalTwenty()` le *signale* ; ce qu'un critique
fait aux dégâts appartient au [LOT-21](@ref lot-21).

## Conception

### Le défaut que les tests ont trouvé, et que la relecture n'aurait pas vu

`nextInt` rejette la queue de l'intervalle pour éviter le biais modulo. La formulation naturelle est
de rejeter la queue **haute** — « au-delà du dernier multiple complet de `n` ». Elle est fausse :

```cpp
const std::uint32_t limite = 0 - (0 - etendue) % etendue;   // vaut 0 si etendue divise 2^32
while (tirage >= limite) { … }                              // toujours vrai : boucle infinie
```

Quand `etendue` divise exactement 2³² — c'est-à-dire pour **toute puissance de deux** — le reste
vaut 0, le seuil devrait valoir 2³², il ne tient pas dans un `std::uint32_t` et retombe à 0. La
boucle ne se termine jamais.

**Le défaut ne se voit que sur les puissances de deux.** Un d6, un d20, un d10 passent ; un d4, un
d8, un d16 bloquent. Les premiers tests écrits employaient un d6 et un d20 ; c'est le test de
rejouabilité, écrit sur **3d8**, qui a gelé la suite. Aucune relecture du code n'aurait attrapé
cela : la formule *a l'air* juste, et son cas dégénéré est précisément celui qu'on ne pense pas à
essayer.

La version retenue rejette la queue **basse**, `[0, 2³² mod n[`, qui est vide quand `n` divise
2³² — donc aucun rejet, et aucun débordement à écrire.

### Deux arrondis qui ne se voient pas

**Le modificateur de caractéristique.** `(score - 10) / 2` tronque **vers zéro** en C++ : un score
de 7 donne `-1` au lieu de `-2`. Le personnage serait moins pénalisé qu'il ne doit l'être, sur
chacun de ses jets, pendant toute la partie — et la formule *a l'air* juste. Le cas de test qui
distingue les deux arrondis est explicitement écrit : `abilityModifier(7) == -2`.

**L'annulation de l'avantage.** Deux sources d'avantage contre une de désavantage donnent
**normal**, pas avantage : la règle annule, elle ne compte pas (`EX-REG-002`). C'est
contre-intuitif la première fois, et c'est pourquoi la décision vit dans une seule fonction —
`rollStance` — plutôt que chez chaque appelant, qui aurait à arbitrer sa pile de bonus contre
toutes les autres à chaque capacité ajoutée.

### Un 20 naturel n'est pas un total de 20

`isNaturalTwenty()` regarde le **dé retenu**, jamais le total. Un 20 naturel déclenche le coup
critique ; un total de 20 obtenu avec un 8 et douze points de bonus n'est qu'un total. Les
confondre rendrait critique un jet sur deux à haut niveau, et le défaut passerait pour un
équilibrage.

### La restitution n'est pas un journal, c'est une exigence

`EX-REG-003` demande que tout jet soit reconstituable : le dé, les modificateurs **avec leur
origine**, le seuil et l'issue. `CheckResult` conserve donc les **deux** dés en cas d'avantage —
pas seulement celui qui a été retenu — et chaque modificateur porte sa source.

> `d20 (avantage : 7, 14) = 14 + 3 (Dexterite) + 2 (maitrise) = 19 >= 15 : reussite`

« Vous échouez » n'apprend rien. Un jeu de rôle au dé dont le joueur ne peut pas reconstituer
pourquoi il a échoué se joue comme une machine à sous — et c'est aussi le seul outil de diagnostic
praticable quand une capacité ne s'applique pas.

### Ce qui est figé ailleurs qu'en dur

**Les degrés de difficulté sont une donnée** (`EX-REG-021`), extraits de la table « Tâche / DD »
des *Basic Rules* p. 64 : six paliers, de 5 à 30. Aucun nombre de difficulté n'apparaît
littéralement dans le code — régler l'équilibre du jeu ne doit pas demander de recompiler, et une
valeur nue dans un `if` ne dit pas ce qu'elle représente. Le test lui-même lit le seuil dans le
fichier plutôt que d'écrire `15`.

**L'échelle du monde est une constante nommée.** Une case vaut 1,5 m. Toutes les portées du corpus
sont en mètres — « allonge 1,50 m », « portée 6 m/18 m » — et la grille compte en cases : la
conversion existe forcément quelque part, et le seul choix ouvert était *à un endroit, ou à trente*.
À trente, il suffit qu'un seul emploie 1,52 (les cinq pieds d'origine) pour qu'une portée de six
cases en devienne cinq ailleurs dans le jeu.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à
968/968, `clang-format`, les six lints, cahier de test et Doxygen verts. Aucun prérequis : pur
`Core`, zéro dépendance.

Exigences couvertes : [`EX-REG-002`](@ref EX-REG-002), [`EX-REG-003`](@ref EX-REG-003),
[`EX-REG-021`](@ref EX-REG-021), [`EX-NFR-002`](@ref EX-NFR-002).
