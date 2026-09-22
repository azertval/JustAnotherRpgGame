+++
id = "LOT-34"
titre = "Équipement, monnaie, objets magiques"
version = "0.0.0"
filiere = "donnees"
statut = "livre"
taille = "M"
resume = "Armes, armures et matériel des Basic Rules sont des données justes au prix, au poids et à la classe d'armure près."
prerequis = ["LOT-32"]
livrables = [
  "175 fichiers de données : `weapons/` (37), `armors/` (13), `items/` (125).",
  "`scripts/sourcebook/equipement.py` et la sous-commande `python scripts/sourcebook equipement`.",
  "`core::loadEquipment` et `core::armorClassFor` (`Source/Core/Rpg/Equipment.{h,cpp}`).",
  "`damage` et `damageType` rendus facultatifs au schéma des armes, avec leur raison (le filet).",
  "Sept tests, dont la CA des 13 armures contre la table du livre.",
]
criteres = [
  "La CA calculée par le moteur pour chacune des 13 armures égale la colonne CA de la table, vérifié par un test dont les valeurs sont recopiées à la main du livre.",
  "Le poids total d'un inventaire de départ correspond au calcul manuel — cotte de mailles, épée longue, bouclier et deux javelines : 27,5 + 1,5 + 3 + 2 × 1 = 34 kg.",
  "Le bouclier ajoute et ne remplace pas, seul comme avec une armure.",
  "Les 37 armes portent la catégorie et la portée de leur intertitre.",
  "Les 125 objets de matériel, d'outillage et de monture valident contre leur schéma.",
  "`ctest` : 1008/1008 (1001 avant le lot, plus 7).",
]
sources = [
  "Basic Rules, tables d'équipement (armes, armures, matériel, montures) ; p. 57-58 pour les marchandises et services, non extraits",
]
+++

## Pourquoi

Les tables d'équipement des *Basic Rules* vers `Source/Elements/Rpg/` : **37 armes**, **13
armures** et **125 objets** — matériel d'aventurier, outils, montures et véhicules — avec
leurs dégâts, leur classe d'armure, leur poids, leur prix et leurs propriétés.

## Périmètre

### Ce que le lot livre

**175 fichiers de données** — `weapons/` (37), `armors/` (13), `items/` (125 : 96 de matériel,
16 outils, 13 montures et véhicules).

**`scripts/sourcebook/equipement.py`** et la sous-commande `python scripts/sourcebook equipement`.

**`core::loadEquipment` et `core::armorClassFor`** (`Source/Core/Rpg/Equipment.{h,cpp}`), et **sept
tests**.

### Ce que le lot ne fait pas

**Il ne livre pas les objets magiques.** Le chapitre 10 du *Sourcebook* est en anglais et demande
une étape de traduction ; il alimente le [LOT-26](../../../../vision/archives/feuille-de-route-jeu.md#lot-26), qui ne l'attend pas pour exister.

**Il ne livre ni les marchandises ni les services.** Leurs deux tables, pages 57 et 58, sont noyées
dans du texte courant et ne se relèvent pas comme des tables : elles restent à extraire, et rien de
ce que le lot doit prouver n'en dépend.

**Quatre selles portent le nom que le livre leur donne dans sa sous-rubrique** — « D'équitation »,
« De bât », « Exotique », « Militaire » — parce que le mot « Selle » est un intertitre de la table
des montures et non une cellule de leur rangée. C'est le nom du livre, pas une faute d'extraction ;
le rendre lisible demanderait de propager l'intertitre comme le fait la table des armes, ce que
cette table-là ne justifie pas pour quatre lignes.

**Il ne gère pas l'inventaire.** `totalWeightGrams` calcule un poids ; porter, équiper et encombrer
sont le [LOT-14](LOT-14-inventaire-et-equipement.md).

## Conception

### C'est le lot où le §4 se paie

« Un tableau ne s'extrait pas en flux de texte » (`EX-CNT-021`) est une règle que le corpus impose
depuis le `LOT-30`. Nulle part sa conséquence n'est aussi silencieuse qu'ici : **une valeur de prix
décalée d'une ligne ne casse rien, ne lève aucune alerte, et déséquilibre l'économie sans que
personne ne comprenne pourquoi**. Les deux tables sont donc lues par coordonnée — les rangées par
ordonnée, les cellules par abscisse.

Ces tables sont **pleine largeur**, contrairement au reste du livre : elles traversent la gouttière
que le `LOT-33` avait mesurée, et les découper en colonnes couperait chaque rangée en deux au milieu
d'un prix. Leur région est donc déclarée — page et bande d'ordonnées — et le module contrôle le
nombre de rangées qu'il en tire.

### Le groupe d'une arme n'est pas dans sa rangée

Le livre range ses armes sous quatre intertitres — « Armes courantes de corps à corps », « Armes de
guerre à distance »… — et ses armures sous quatre autres. **Ce sont ces intertitres, et eux seuls,
qui disent qu'une arme est courante ou de guerre** ; la rangée ne porte rien de tel.

Une extraction qui ne lirait que les rangées produirait trente-sept armes sans catégorie — et
`category` est requis au schéma, mais **rien ne dit qu'il est juste**. Les intertitres sont reconnus
à leur rangée à **une seule cellule** : une table à cinq colonnes n'en produit pas d'autre.

Le défaut s'est manifesté à la première exécution, et c'est le contrôle de cardinal qui l'a dit : la
bande d'ordonnées commençait **après** le premier intertitre, si bien que les dix premières armes
sortaient sans catégorie et étaient silencieusement écartées. 27 armes au lieu de 37 — sans le
contrôle, un catalogue amputé de dix armes se serait chargé et validé sans un mot.

### Trois cas que le livre écrit et qu'un schéma refusait

**Le filet n'inflige aucun dégât.** Sa rangée a une cellule de moins que les autres, et le schéma du
`LOT-32` exigeait `damage` et `damageType`. Lui inventer des dés en ferait une arme qui touche pour
rien, ce qui est faux : le filet **entrave**. Les deux champs sont devenus facultatifs, avec la
raison écrite dans le schéma — et l'alignement des colonnes se fait sur la première cellule qui
*ressemble* à des dégâts, jamais sur un indice fixe : un décalage mettrait le poids dans la colonne
des dégâts, ce que rien ne refuserait.

**La fronde n'a pas de poids.** Sa colonne porte un tiret. Le tiret vaut **absent**, jamais zéro —
une case que le livre n'a pas remplie n'est pas une valeur nulle. Le cas est rapporté à chaque
génération plutôt que masqué.

**Le bouclier n'est pas une armure.** Il est dans la même table, et le livre l'y met ; mais il
**ajoute** à la classe d'armure au lieu de la remplacer. Le traiter comme une armure donnerait une
CA de 2 à un personnage en bouclier seul. La table porte donc treize rangées — douze armures et le
bouclier — et c'est le chiffre que la feuille de route annonce.

### Les trois formes de la colonne CA disent trois règles

C'est la faute qui ne se voit pas :

| Ce que le livre écrit | Ce que cela veut dire |
|---|---|
| `11 + Mod.Dex` | la Dextérité s'ajoute **sans plafond** (armures légères) |
| `14 + Mod.Dex (max +2)` | elle s'ajoute **plafonnée à 2** (armures intermédiaires) |
| `18` | elle ne s'ajoute **pas du tout** (armures lourdes) |

Réduire les trois à leur premier nombre appliquerait la Dextérité au harnois — ce qui rend le
personnage **plus** résistant, jamais moins, ne provoque aucune erreur, et passe pour de
l'équilibrage.

Le test choisit une Dextérité de 18 (modificateur +4) précisément pour révéler la faute : avec +1,
un harnois à 19 au lieu de 18 se lit mal ; avec +4, il donne 22, et le plafond des intermédiaires se
distingue nettement du sans-plafond des légères.

### Deux unités, converties une seule fois

Le livre écrit des prix en pièces (po, pa, pc) et des poids en kilogrammes **et** en grammes dans la
même table — « 6,5 kg », « 500 g ». Les schémas veulent des **pièces de cuivre** et des **grammes**.
La conversion se fait à l'extraction : mélanger deux unités dans un total donnerait un sac de cinq
cents kilos pour une poignée de fléchettes, et une masse d'armes moins chère qu'une dague.

### La table du matériel est composée en deux sous-tables côte à côte

Une rangée y porte **six** cellules, donc deux objets. Les lire d'un bloc donnerait un objet nommé
« Acide (fiole) » coûtant « 25 po » et pesant « 500 g Billes de fronde (20) » — une valeur qui n'est
pas un poids, et qu'aucun schéma ne refuserait puisque le champ serait simplement absent. La forme
est déclarée table par table, et le découpage se fait par triplets.

Une ligne dont le prix n'est pas lisible **n'est pas un objet** : c'est un intertitre — « Barde
×4 ×2 » — ou une ligne de continuation. L'écrire produirait un objet gratuit portant un nom de
section.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔
2. ✔
3. ✔
4. ✔
5. ✔
6. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1008/1008, `clang-format`, les six lints, cahier de test et Doxygen verts.

Alimente [LOT-14](LOT-14-inventaire-et-equipement.md), [LOT-26](../../../../vision/archives/feuille-de-route-jeu.md#lot-26), [LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27).

Exigences couvertes : [`EX-INV-001`](../../../../../Documentation/Specification/inventaire.md#EX-INV-001), [`EX-INV-010`](../../../../../Documentation/Specification/inventaire.md#EX-INV-010), [`EX-INV-011`](../../../../../Documentation/Specification/inventaire.md#EX-INV-011), [`EX-INV-041`](../../../../../Documentation/Specification/inventaire.md#EX-INV-041).
