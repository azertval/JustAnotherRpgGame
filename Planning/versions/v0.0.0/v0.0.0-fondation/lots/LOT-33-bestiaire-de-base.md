+++
id = "LOT-33"
titre = "Bestiaire de base"
version = "0.0.0"
filiere = "donnees"
statut = "livre"
taille = "L"
resume = "Les 94 bêtes du corpus sont des données chargées par le moteur : tout l'aval du combat tourne sur des créatures réelles."
prerequis = ["LOT-32"]
livrables = [
  "94 fichiers sous `Source/Elements/Rpg/creatures/` : 136 traits, 135 actions dont 115 à dégâts typés.",
  "`scripts/sourcebook/bestiaire.py` et la sous-commande `python scripts/sourcebook bestiaire`.",
  "`Extracteur.lignes()` (`scripts/sourcebook/extraction.py`) : la lecture typographique du corpus.",
  "`core::loadBestiary` (`Source/Core/Rpg/Bestiary.{h,cpp}`) et l'énumération `core::CreatureSize`.",
  "Le champ `description` ajouté à `creature.schema.json`.",
  "Neuf tests, dont `test_bestiary.cpp` et ses douze profils recopiés à la main du PDF.",
]
criteres = [
  "Les 94 profils chargent, sans une erreur, et leurs identifiants sont uniques.",
  "Douze profils sont rejoués — classe d'armure, points de vie, vitesse, facteur de puissance, dégâts de l'attaque principale — contre des valeurs recopiées à la main du PDF.",
  "Une extraction incomplète arrête la génération plutôt que de livrer un catalogue amputé : sommaire confronté au corps, gouttière re-vérifiée, six caractéristiques exigées, sept étiquettes de profil exigées.",
  "Toute valeur d'énumération inconnue du moteur est signalée au chargement, jamais ignorée.",
  "`core::CreatureSize` coïncide avec `common.schema.json` — vérifié par test.",
  "Les 94 créatures valident contre `creature.schema.json` en CI.",
  "`ctest` : 974/974 (968 avant le lot, plus 6 ; trois cas ajoutés aux tests d'énumérations).",
]
sources = [
  "Animaux.pdf, p. 2 (sommaire des 94 créatures) et ses trente pages de blocs",
  "Basic Rules, p. 38 (table des langues)",
]
+++

## Pourquoi

Les **94 bêtes** d'`Animaux.pdf` vers `Source/Elements/Rpg/creatures/`. C'est le seul gisement du
corpus qui ne demande **aucun jugement humain** — gabarit régulier, français, texte natif — et
c'est pour cela qu'il était seul dans ce lot.

Livrer les 94 d'abord fait tourner tout l'aval — attaques, IA, rencontres — sur des données réelles
pendant que le reste arrive. Loup, ours, araignée géante et sanglier peuplent le donjon du
[LOT-27](@ref lot-27) sans attendre.

## Périmètre

### Ce que le lot livre

**94 fichiers de données** sous `Source/Elements/Rpg/creatures/` — 136 traits, 135 actions, dont
115 portent des dégâts typés ; 8 langues citées, 21 paragraphes d'ambiance.

**`scripts/sourcebook/bestiaire.py`** et la sous-commande `python scripts/sourcebook bestiaire`,
qui les régénèrent depuis le corpus.

**`Extracteur.lignes()`** (`scripts/sourcebook/extraction.py`) : la lecture **typographique** du
corpus — police, corps, graisse — sur laquelle repose toute l'extraction de ce lot.

**`core::loadBestiary`** (`Source/Core/Rpg/Bestiary.{h,cpp}`), et l'énumération fermée
`core::CreatureSize`, sixième valeur comprise.

**Neuf tests** : six sur le bestiaire, trois cas ajoutés aux tests d'énumérations.

**Un champ `description`** au `creature.schema.json`, et sa raison — voir plus bas.

### Ce que le lot ne fait pas

**Il ne livre pas les 82 créatures de Tanares.** Mêmes gabarits, mais en anglais : c'est le
`LOT-46`, et l'étape de traduction n'avait rien à faire ici.

**Il ne type qu'une clause de dégâts par action.** Une morsure qui inflige « 7 (1d10 + 2) dégâts
perforants + 5 (1d10) dégâts de poison » en porte deux ; `damage` ne rend que la première, et le
texte reste ce qui fait foi jusqu'à ce que le [LOT-21](@ref lot-21) sache composer plusieurs
clauses. Le champ n'est pas *faux*, il est **partiel**, et c'est écrit là où on le lit.

**Il ne porte aucune illustration.** Le champ `asset` existe au schéma, aucune créature ne le
remplit : les clés d'assets sont le `LOT-39`.

**Il n'implémente aucun mécanisme de combat.** Les créatures sont des données ; les jouer est le
`LOT-21`.

## Conception

### L'extraction se fait sur la typographie, pas sur des expressions régulières

Un bloc de statistiques n'a **ni balise ni ponctuation** qui sépare le nom d'un trait de sa
description : seule la graisse le fait — « **Vue aiguisée**. L'aigle a un avantage… ». Découper au
premier point donne « Attaque au corps à corps avec une arme : +4 au toucher, allonge 1,50 m »
comme nom d'action, et coupe « Toile d'araignée (Recharge 5-6) » en son milieu.

Le mode texte ne peut donc pas suffire — et il fait pire : **il perd des espaces**. Les titres de
traits en sortent collés (`Vueaiguisée`, `Sens dela toile`, `Tactiquedegroupe`), faute qu'aucun
contrôle ne rattrape et qu'aucune relecture de la donnée produite ne signale, puisque la donnée
produite *est* la faute. Le fragment de police, lui, porte le texte tel que le document l'écrit.

`Extracteur.lignes()` rend donc chaque ligne avec ses fragments, et tout le module en découle :

| Élément du bloc | Ce qui le désigne |
|---|---|
| nom de créature | police `DnDMr.Eaves`, corps 14,5 — **94 occurrences**, pas une de plus |
| en-tête `ACTIONS` | petites capitales `ScalySans`, corps 9 |
| étiquette de profil | premier fragment **gras** parmi les douze étiquettes du livre |
| nom de trait ou d'action | premier fragment **gras** hors de ces douze |
| paragraphe d'ambiance | police `CenturySchoolbook` — une **autre** police que la règle |
| encadré « Variante » | police `Calibri` — cinq encadrés qui ne sont pas des créatures |
| fin de paragraphe | interligne : 11,2 pt dans un paragraphe, 15,2 pt et plus entre deux |

Aucun de ces sept discriminants n'est une intuition : chacun a été mesuré sur les trente pages du
document, et les deux qui sont des seuils — le corps du titre, l'interligne — tombent dans des
vides nets. Le document ne porte **aucun** interligne entre 11,3 et 15,2 pt.

### Deux contrôles qui arrêtent la génération

**Le sommaire est le point d'attestation.** La page 2 porte 94 entrées, le corps 94 titres, et les
deux listes doivent coïncider **nom pour nom, dans l'ordre**. C'est le seul contrôle qui détecte un
bloc sauté par une coupe de colonne trop étroite — panne qui ne laisse aucune autre trace, parce
qu'un catalogue de 93 créatures se charge, se valide et se joue exactement comme un de 94. Le
sommaire donne en prime la **casse d'affichage** — « Aigle géant » — que les petites capitales du
titre ont perdue.

**La gouttière est mesurée, puis re-vérifiée à chaque exécution.** Les pages sont à deux colonnes ;
lire une page entière entrelace les deux et attribue une action à la créature voisine. Le blanc
central occupe `[287, 309]` sur les trente pages, et la génération le recontrôle page par page : le
jour où une édition le déplace, la coupe **échoue** au lieu de produire des blocs mélangés.

### Ce que le schéma ne peut pas dire, et qui n'est pas perdu pour autant

Trois formulations du livre ne rentrent pas dans un champ typé. Aucune n'est jetée, aucune n'est
**élargie en silence** :

**« Résistance aux dégâts contondants, perforants et tranchants provenant d'attaques non
magiques ».** Mettre `bludgeoning` dans `damageResistances` rendrait le diablotin résistant à une
masse d'armes ordinaire, ce que le livre ne dit pas. Le point-virgule du livre sépare
l'inconditionnel du conditionnel : seule la première partie entre dans le champ typé, la clause
qualifiée reste dans un trait, et la créature **déclare** le mécanisme `resistance-conditionnelle`
(`EX-CNT-030`). Le moteur listera au chargement ce qu'il ne sait pas honorer (`EX-CNT-031`) plutôt
que de le jouer de travers.

**« Comprend le commun mais ne peut pas le parler ».** `languages` porte les langues du catalogue
citées ; la nuance reste dans un trait, avec le mécanisme `langue-comprise-non-parlee`. Sept
créatures sur les huit qui parlent sont dans ce cas.

**« L'aérien »**, que l'aigle géant comprend, n'est **pas** au catalogue des seize langues du
`LOT-43`, et la table des *Basic Rules* p. 38 ne le porte pas davantage. Le rapprocher du
primordial serait un élargissement muet — défendable, invérifiable, et invisible une fois écrit.
La génération le **signale** à chaque exécution et ne l'écrit pas.

**Le paragraphe d'ambiance a désormais un champ.** Vingt et un blocs se terminent par un
paragraphe qui ne porte aucune règle et qui est pourtant ce que le bestiaire affichera — « Les
araignées-loups géantes chassent en terrain découvert ou se cachent dans des terriers ». Il va dans
`description`, ajouté au schéma par ce lot : **le premier remplissage d'une famille est le moment
où le contrat rencontre la réalité**, et laisser tomber ce paragraphe obligerait à réextraire le
document le jour où l'écran existe.

### Cinq défauts du lexique que les 94 noms ont mis au jour

Les identifiants sont **anglais** — convention des catalogues (`acrobatics`, `dwarvish`) — et
viennent du lexique du `LOT-30`, seule table d'autorité du projet. Les 94 noms français s'y
retrouvent, à cinq conditions :

1. **Le champ français porte des variantes séparées par `/`** — « Bec de hache / Autrache ». Une
   comparaison sur la chaîne entière en rate deux. Le même défaut se propage aux **catalogues
   livrés** : `skills/deception.json` s'appelle « Tromperie / Supercherie », et le diablotin, qui a
   « Tromperie +4 », désignait une compétence introuvable.
2. **« Zombi » y est écrit « Zombi Objets magiques D&D 5 »** — un titre de section happé par
   l'extraction du glossaire.
3. **« Tigre à dents de sabre » y est écrit « Tigre à dents de sabe »** — une coquille.
4. **L'entrée « Tigre » a son côté anglais resté en français.** L'identifiant en sortait
   « tigre » au milieu de quatre-vingt-treize identifiants anglais. Celui-là **ne se détecte pas
   mécaniquement** : « Quasit = Quasit » et « Pseudodragon = Pseudodragon » sont identiques des
   deux côtés et parfaitement justes. C'est une relecture des 94 identifiants qui l'a trouvé, et
   c'est la raison pour laquelle cette relecture a eu lieu.
5. **La langue des elfes s'y nomme « elfe »**, quand la table des *Basic Rules* p. 38 et les blocs
   de créature écrivent « **elfique** ». Le `LOT-43` avait déjà buté dessus et l'a tranché dans
   `options.ALIAS_DU_LIVRE` ; cette table est **réutilisée** plutôt que recopiée. Deux copies d'une
   correspondance divergent, et celle-ci ne se serait manifestée que par une chouette géante muette
   en elfique — ce qu'aucun contrôle ne signale.

Les défauts 2, 3 et 4 sont déclarés dans une table d'alias du module plutôt que corrigés dans le
lexique : **le lexique est généré**, et une correction à la main y serait effacée à la prochaine
exécution. L'alias, lui, porte sa raison et se relit.

### Le test lit le livre, pas la génération

L'acceptation du lot demande que dix créatures soient rejouées contre des valeurs **recopiées à la
main du PDF**. La forme n'est pas négociable : un test qui comparerait la sortie de la génération à
elle-même passerait quelle que soit la faute d'extraction — c'est exactement la panne qu'il doit
exclure.

`test_bestiary.cpp` porte donc **douze** profils — deux de plus que demandé, pour couvrir un
facteur de puissance fractionnaire (1/4, 1/2), un mort-vivant, un des plus gros (mammouth, 126 PV)
et un des plus petits (aigle, 3 PV) — chacun avec sa **page imprimée**, de sorte que toute ligne se
revérifie en ouvrant le document.

Le catalogue lu est celui **livré**, jamais une fixture : une copie cesse de prouver quoi que ce
soit le jour où la génération change.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔
2. ✔
3. ✔
4. ✔
5. ✔
6. ✔
7. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 974/974, `clang-format`, les six lints, cahier de test et Doxygen verts.

Alimente [LOT-13](@ref lot-13), [LOT-21](@ref lot-21), [LOT-23](@ref lot-23), [LOT-27](@ref lot-27), `LOT-46`.
