+++
id = "LOT-32"
titre = "Schémas de données RPG"
version = "0.0.0"
filiere = "donnees"
statut = "livre"
taille = "M"
resume = "Le contrat des données RPG existe avant les données : onze schémas, un validateur en CI et trois énumérations fermées tenues d'accord."
prerequis = ["LOT-30", "LOT-79"]
livrables = [
  "Onze schémas sous `Source/Elements/Rpg/schema/`, dont `common.schema.json`.",
  "`scripts/checks/check_rpg_data.py`, exécuté en CI, auto-testé sur `scripts/fixtures/rpg/` (trois fixtures valides, dix invalides).",
  "`core::DamageType` (13 valeurs), `core::Condition` (15), `core::MagicSchool` (8), et leur correspondance nom ↔ valeur par `switch` exhaustif.",
  "`Source/Test/Unit/Core/Rpg/test_rpg_enums.cpp` : cinq tests, dont celui qui relie le C++ aux schémas.",
]
criteres = [
  "Une donnée invalide fait échouer la CI avec le fichier et la ligne fautifs (`EX-CNT-010`) — vérifié sur dix fixtures, une violation chacune.",
  "Ajouter une valeur d'énumération d'un seul côté fait échouer un test (`EX-CNT-011`) — vérifié par injection, sur les deux contrôles.",
  "Le champ `source` est obligatoire au schéma et son absence est refusée (`EX-CNT-001`, `EX-CNT-002`).",
  "Une donnée provisoire est énumérée par la CI et doit écrire d'avance son critère de retrait (`EX-CNT-032`).",
  "Aucun catalogue RPG ne porte sa propre routine de lecture — il n'en existe aucun, et le [LOT-79](LOT-79-socle-chargement-donnees.md) est la seule voie ouverte.",
  "`ctest` : 948/948 (943 avant le lot, plus 5).",
]
+++

## Pourquoi

Le contrat **avant** les données. Onze schémas JSON, un validateur en intégration continue, et les
trois énumérations fermées du RPG en C++ — le tout livré alors qu'il n'existe **aucune donnée** à
valider. C'est l'ordre qui compte : les catalogues arrivent du `LOT-33` au `LOT-84`, et un contrat
écrit après coup se contente de décrire ce qui a déjà été produit, y compris ses défauts.

## Périmètre

### Ce que le lot livre

**Onze schémas** sous `Source/Elements/Rpg/schema/` : les dix familles annoncées — créature, objet,
arme, armure, sort, espèce, classe, historique, état, type de dégâts — plus `common.schema.json`,
qui porte ce que toutes réutilisent. Une définition écrite deux fois diverge ; c'est la leçon des
six lecteurs JSON du [LOT-79](LOT-79-socle-chargement-donnees.md), transposée aux contrats.

**`scripts/checks/check_rpg_data.py`**, exécuté en CI, qui valide tout `Source/Elements/Rpg/**/*.json`.

**Trois énumérations C++** — `core::DamageType` (13 valeurs), `core::Condition` (15),
`core::MagicSchool` (8) — et leur correspondance nom ↔ valeur par `switch` **exhaustif et sans
`default`**, sur le patron exact de `core::tileTypeName`.

**`Source/Test/Unit/Core/Rpg/test_rpg_enums.cpp`**, cinq tests, dont celui qui relie le C++ aux
schémas.

### Ce que le lot ne fait pas

**Il ne charge rien.** Aucun lecteur C++ de catalogue n'est écrit ici : quand ils viendront, ils
passeront par `core::JsonDocument` ([LOT-79](LOT-79-socle-chargement-donnees.md)) et non par une septième réimplémentation
de `loadFromFile`. Ce lot écrit des contrats.

**Il ne valide pas les schémas à l'exécution.** La validation par schéma est faite en CI, en
Python. Embarquer un validateur JSON Schema dans le moteur coûterait une dépendance de plus pour
recontrôler ce qui est déjà contrôlé avant d'entrer dans le dépôt.

**Il ne livre aucune donnée.** Pas même les treize types de dégâts en catalogue : leur nom affiché
et leur description sont du contenu, et ils arriveront avec le lot qui les emploie.

## Conception

### Le triangle, et pourquoi chaque arête n'est contrôlée qu'une fois

Trois artefacts nomment les mêmes choses : le **moteur** (`core::DamageType`), le **contrat**
(`common.schema.json`) et la **table de traduction** (`rpg.glossary.csv`, `LOT-30`). Trois listes
qui doivent rester identiques, et qui n'ont aucune raison de le rester toutes seules.

| Arête | Contrôlée par | Comment |
|---|---|---|
| moteur ↔ contrat | `test_rpg_enums.cpp` (`ctest`) | le test **lit le schéma livré** et compare aux noms produits par le C++ |
| contrat ↔ lexique | `scripts/checks/check_rpg_data.py` (CI) | les valeurs du schéma sont exactement les termes anglais du lexique sous la catégorie correspondante |
| moteur ↔ lexique | *aucun* | par transitivité des deux précédents |

La troisième arête est **délibérément non contrôlée**. Un troisième contrôle serait redondant,
donc bruyant, et le jour où deux des trois échouent ensemble on ne saurait plus lequel dit vrai.

**Vérifié par injection** : ajouter `"sonic"` à la seule énumération du schéma fait échouer les
deux contrôles, chacun avec son message — `core::DamageType et common.schema.json/$defs/damageType
divergent` côté `ctest`, `sonic ne figure pas au lexique sous « type de dégâts »` côté CI.

### Ce que la réalisation a tranché

**Les énumérations sont en C++, les catalogues n'y sont pas.** `core::Condition` porte les quinze
états ; il ne porte **aucun** de leurs effets, qui vivent dans `Source/Elements/Rpg/conditions/`.
Une énumération qui porterait les effets les figerait dans le code, ce qu'`EX-VIS-007` interdit.
Le catalogue existe donc *malgré* l'énumération, et pas à sa place.

**Les cardinaux sont écrits dans un test.** 13, 15, 8. Ces nombres viennent des règles du jeu, pas
du corpus. Le test de coïncidence resterait vert si les deux côtés perdaient la même valeur — lors
d'une résolution de conflit, par exemple ; celui-ci ne le resterait pas.

**Un schéma décompose, il ne calcule pas.** La classe d'armure d'une armure est donnée en base,
contribution de Dextérité et plafond, jamais en formule textuelle : une formule dans une donnée est
du code déguisé, que le moteur devrait interpréter.

**Deux unités internes uniques.** Les prix sont en **pièces de cuivre**, les poids en **grammes**,
entiers tous les deux. Le corpus mélange « 500 g » et « 2 kg », « 2 pa » et « 25 po » ; convertir à
l'entrée évite des arrondis là où l'encombrement se calcule par somme, et l'affichage reste une
affaire de présentation.

**Le formalisme des dés est contraint par expression régulière.** `1d8`, `2d6+3`, `4`. C'est la
parade au risque résiduel que le [LOT-30](LOT-30-chaine-extraction-corpus.md) a laissé ouvert : un `1d8` devenu `ld8` à
l'OCR est invisible à la relecture et fatal à l'exécution.

**`additionalProperties: false` partout.** Sans lui, `weigthGrams` au lieu de `weightGrams` passe
sans un mot, et l'arme pèse zéro.

**Les messages étaient dupliqués, et c'est corrigé.** Le socle commun est appliqué par `allOf`
*et* repris dans le `required` de chaque famille ; un champ obligatoire manquant se signalait donc
deux fois. Deux lignes identiques pour une seule faute font douter qu'il n'y en ait qu'une.

### Le validateur s'auto-teste, faute de données à valider

Aucun catalogue n'existe encore. `check_rpg_data.py` serait donc **vert par vacuité** — la panne du
[LOT-78](LOT-78-numeros-herites.md), déjà rencontrée au [LOT-30](LOT-30-chaine-extraction-corpus.md). Il s'éprouve sur
`scripts/fixtures/rpg/` avant de se prononcer : trois fixtures valides qu'il doit accepter, et
**dix fixtures invalides qu'il doit refuser**, chacune nommée d'après le défaut qu'elle déclenche —
provenance absente, provenance inconnue, type de dégâts en français, dés mal formés, caractéristique
manquante, champ mal orthographié, identifiant hors énumération, donnée provisoire sans critère de
retrait, propriété d'arme inconnue, JSON tronqué.

Chacune produit **exactement une** violation, située : `weapons/des-mal-formes.json:6 : damage :
'ld8' does not match …`. C'est ce que demande `EX-CNT-010` — « donnée invalide » sur un catalogue
de mille entrées ne se corrige pas, une ligne si.

`jsonschema` ne rapporte qu'un chemin logique (`actions/0/damageType`), jamais une position. La
ligne est retrouvée en cherchant les clés du chemin dans le texte brut, sans revenir en arrière :
exact sur une donnée mise en forme normalement, pessimiste de quelques lignes sur un fichier écrit
d'un bloc. Très au-dessus, dans les deux cas, de ce qu'il remplace.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔
2. ✔
3. ✔
4. ✔
5. ✔
6. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 948/948, `clang-format`, les six lints, cahier de test et Doxygen verts.

Prérequis de `LOT-33` à `LOT-37`, `LOT-43` et `LOT-84`.

Exigences couvertes : [`EX-CNT-001`](../../../../../Documentation/Specification/contenu.md#EX-CNT-001), [`EX-CNT-002`](../../../../../Documentation/Specification/contenu.md#EX-CNT-002), [`EX-CNT-010`](../../../../../Documentation/Specification/contenu.md#EX-CNT-010), [`EX-CNT-011`](../../../../../Documentation/Specification/contenu.md#EX-CNT-011), [`EX-CNT-032`](../../../../../Documentation/Specification/contenu.md#EX-CNT-032).
