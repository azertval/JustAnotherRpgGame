+++
id = "LOT-43"
titre = "Options de personnage : dons, multiclassage, compétences, langues"
version = "0.0.0"
filiere = "donnees"
statut = "livre"
taille = "M"
resume = "La fiche de personnage dispose des quatre catalogues qu'elle supposait : 18 compétences, 16 langues, 42 dons et la règle du multiclassage."
prerequis = [
  "LOT-32",
]
livrables = [
  "**77 fichiers de données** sous `Source/Elements/Rpg/` : `skills/` (18), `languages/` (16), `feats/` (42), `rules/multiclassing.json`, et quatre schémas.",
  "`scripts/sourcebook/options.py` et la sous-commande `python scripts/sourcebook options`.",
  "`core::multiclassCasterLevel` (`Source/Core/Rpg/Multiclassing.{h,cpp}`) et six tests.",
  "Un contrôle de référence croisée dans `scripts/check_rpg_data.py` : toute langue citée par une créature ou une espèce existe au catalogue.",
]
criteres = [
  "Un personnage multiclassé calcule ses emplacements conformément à la table, **vérifié sur trois combinaisons** — rôdeur 4/magicien 3 (l'exemple du livre), paladin 3/rôdeur 3, sorcier 5/magicien 3 — plus guerrier 5/roublard 5 et deux cas limites.",
  "Toute langue référencée par une créature ou une espèce existe au catalogue — contrôle en CI, **vérifié par injection** d'une créature parlant le « draconien ».",
  "Les 18 compétences, 16 langues et 42 dons sont extraits et validés contre leurs schémas.",
  "Une extraction incomplète **arrête** la génération plutôt que de livrer un catalogue amputé : chaque catalogue vérifie son cardinal, et le multiclassage vérifie son recoupement.",
  "`ctest` : **954/954** (948 avant le lot, plus 6).",
]
sources = [
  "Basic Rules, p. 31, p. 38 et p. 64",
  "Manuel des Joueurs, p. 165-167",
]
+++

## Pourquoi

Quatre catalogues oubliés du premier découpage, tous présents dans le corpus et tous exigés par la
fiche de personnage : **18 compétences**, **16 langues**, **42 dons**, et la règle du
**multiclassage**. Le [LOT-13](@ref lot-13) les suppose sans jamais dire d'où ils viennent ; ils
viennent d'ici.

## Périmètre

**77 fichiers de données** sous `Source/Elements/Rpg/` — `skills/` (18), `languages/` (16),
`feats/` (42), `rules/multiclassing.json` (1) — et **quatre schémas** qui les décrivent.

**`scripts/sourcebook/options.py`** et la sous-commande `python scripts/sourcebook options`, qui
les régénèrent depuis le corpus.

**`core::multiclassCasterLevel`** (`Source/Core/Rpg/Multiclassing.{h,cpp}`), et six tests.

**Un contrôle de référence croisée** dans `scripts/check_rpg_data.py` : toute langue citée par une
créature ou une espèce doit exister au catalogue.

## Conception

### Le choix de la source est la moitié du travail

Sur ce lot, la question n'était pas *comment extraire* mais *d'où*. Chaque catalogue a une réponse
différente, et chacune est écrite dans le module :

| Catalogue | Source | Pourquoi |
|---|---|---|
| compétences | *Basic Rules* p. 64 | texte natif propre, la liste tient en cinq lignes |
| langues | *Basic Rules* p. 38 | texte natif propre, table à trois colonnes |
| dons | **lexique** (`LOT-30`) pour les noms | les titres du livre sont des scans mutilés |
| multiclassage | *Manuel des Joueurs* p. 165-167 | **seule** source complète |

**Le multiclassage vient du *Manuel des Joueurs*, et c'était la bonne exigence.** Les *Basic Rules*
n'en portent ni les prérequis ni les maîtrises : elles renvoient explicitement au chapitre 6 du
*Manuel*. Les douze prérequis de caractéristique et les douze lignes de maîtrises en sortent
proprement, à un artefact près — `1 ntelligence` pour `Intelligence`, visible et corrigé.

### Trois défauts du corpus que ce lot a mis au jour

**1. L'OCR efface les cellules valant `1`.** Sur la table d'emplacements du *Manuel des Joueurs*,
**onze lignes sur vingt** sont amputées d'une à quatre cellules — un lanceur de niveau 20 y perd
ses emplacements de niveau 8 et 9. Ce n'est pas de la corruption mais de la **disparition** : une
case effacée ressemble à une case vide légitime, et cette table en contient de vraies.

La parade est un **recoupement**, pas une relecture : la table du multiclassage est identique à la
progression du **magicien**, qui figure en texte natif propre dans les *Basic Rules* p. 31. La
donnée est prise sur cette dernière, puis confrontée cellule à cellule à celle du *Manuel* —
**26 cellules** rétablies, et la génération l'annonce à chaque exécution. Une divergence qui ne
serait *pas* un `1` manquant **arrête la génération** : ce serait que les deux tables ne décrivent
pas la même chose, et cela se tranche à la main.

**2. Les deux sources françaises ne traduisent pas les mêmes dons pareil.** Le *Manuel des Joueurs*
écrit « Adepte des éléments », « Spécialiste des boucliers », « Ritualiste » ; le lexique (aidedd)
écrit « Adepte élémentaire », « Maître des boucliers », « Magie rituelle ». Douze dons sur
quarante-deux divergent. Les deux graphies sont défendables ; ce qui ne l'est pas, c'est d'en avoir
deux dans le même jeu. Le lexique fait autorité, et le nom du livre est conservé en `variantes`
pour que la correspondance reste traçable.

**3. « Sorcier » n'est pas *sorcerer*.** Le *Manuel des Joueurs* appelle « Sorcier » la classe que
le lexique nomme « occultiste » — le *warlock*. « Sorcier » ressemble à *sorcerer*, qui est
l'**ensorceleur**, une autre classe présente dans la même table deux lignes plus haut. Un
rapprochement par ressemblance aurait interverti leurs prérequis en silence. L'alias est
**déclaré**, avec la raison, dans `ALIAS_DU_LIVRE`.

### Le multiclassage, la règle qu'il est le plus facile d'implémenter de travers

`EX-RPG-041` le dit, et deux pièges le confirment :

**L'arrondi se fait par classe, jamais sur le total.** Un paladin 3/rôdeur 3 donne `1 + 1 = 2`, et
non `⌊6/2⌋ = 3`. Sommer d'abord et diviser ensuite donne un emplacement de trop, à un niveau où
c'en est un tiers de plus.

**La magie de pacte est exclue de la somme** (`EX-RPG-052`). Les emplacements du sorcier
(occultiste) sont peu nombreux, toujours au niveau maximal, et récupérés au repos **court**. Les
additionner double la puissance du personnage sans que rien ne le signale.

**Le meilleur test disponible est celui du livre.** Le *Manuel* travaille lui-même un exemple :
*« ce rôdeur 4/magicien 3, vous êtes considéré comme un personnage de niveau 5 […] : vous avez donc
quatre emplacements de niveau 1, trois de niveau 2 et deux de niveau 3 »*. Le test le reproduit
tel quel, et lit les emplacements dans la table **livrée**. C'est le livre qui vérifie
l'implémentation, et non l'implémentation qui vérifie sa propre arithmétique.

La correspondance classe → progression (`full`, `half`, `third`, `pact`, `none`) est une **donnée**,
attestée sur la phrase de la page 166 : la génération vérifie que chaque classe qu'elle déclare y
est bien citée. Une table de fractions saisie de mémoire est indétectablement fausse — un lanceur
multiclassé mal calculé **reste jouable, simplement faux**.

### Les dons sont livrés provisoires, et le disent

Aucun mécanisme de don n'existe encore dans le moteur. Les quarante-deux dons portent donc
`narratif: true` (`EX-RPG-040`, `EX-RPG-051`) et `status.provisoire` avec son **critère de retrait
écrit d'avance** (`EX-CNT-032`) : *le `LOT-13` a livré la fiche et les mécanismes que ce don exige,
et l'effet est décrit en `mecanismesRequis`*.

Un don qui se présenterait comme jouable sans l'être coûterait bien plus cher à diagnostiquer qu'un
don déclaré non joué. La CI les énumère, groupés par motif — quarante-deux lignes identiques ne se
lisent pas.

## Ce que le lot ne fait pas

**Il ne décrit pas l'effet des dons.** Nom, provenance, variantes : c'est tout, et c'est déclaré.

**Il n'extrait pas les prérequis de dons.** Le champ existe au schéma, aucun ne le porte : les
lignes « Prérequis » du livre sont dans le corps du texte, sous des titres mutilés, et les extraire
demanderait le découpage par don que ce lot n'a pas fait.

**Il ne charge rien en C++ hors du multiclassage.** Les catalogues sont des données validées en CI ;
le moteur n'en lit que ce dont il a besoin, par `core::JsonDocument` ([LOT-79](@ref lot-79)).

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 954/954, `clang-format`, les six lints, cahier de test et Doxygen verts ; tous les critères d'acceptation sont cochés dans l'epic d'origine. Alimente [LOT-13](@ref lot-13), `LOT-36`.

Exigences couvertes : [`EX-REG-012`](@ref EX-REG-012), [`EX-RPG-040`](@ref EX-RPG-040), [`EX-RPG-041`](@ref EX-RPG-041), [`EX-RPG-042`](@ref EX-RPG-042).
