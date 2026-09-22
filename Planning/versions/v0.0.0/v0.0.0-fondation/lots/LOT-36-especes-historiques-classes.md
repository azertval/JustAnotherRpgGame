+++
id = "LOT-36"
titre = "Espèces, historiques et classes provisoires"
version = "0.0.0"
filiere = "donnees"
statut = "livre"
taille = "L"
resume = "De quoi construire un personnage jouable au plus tôt : 22 espèces, 13 historiques et 4 classes provisoires, extraits de trois livres et chargés par le moteur."
prerequis = [
  "LOT-32",
  "LOT-43",
]
livrables = [
  "**39 fichiers de données** sous `Source/Elements/Rpg/` : `species/` (22), `backgrounds/` (13), `classes/` (4, avec leurs 80 lignes de progression).",
  "`scripts/sourcebook/personnage.py` et la sous-commande `python scripts/sourcebook personnage`.",
  "Trois modules partagés d'extraction : `mise_en_page.py`, `catalogues.py`, et la déduplication des lignes surimprimées dans `extraction.py`.",
  "`core::loadCharacterOptions` (`Source/Core/Rpg/CharacterOptions.{h,cpp}`) et neuf tests.",
]
criteres = [
  "**Trois classes chargent et donnent les bons modificateurs** — les quatre le font : dé de vie, caractéristique principale et les **deux** jets de sauvegarde, comparés à la table de la page 57 recopiée à la main.",
  "**La progression du niveau 1 au niveau 5 ne fait intervenir aucune valeur codée en C++** : le bonus de maîtrise est lu dans la table livrée, et le test vérifie en prime que les vingt niveaux se suivent sans trou. La formule générale donnerait le même résultat, et c'est précisément le piège : l'écrire ferait cesser de lire la donnée.",
  "**Toute espèce exigeant un mécanisme absent est listée au chargement**, jamais jouée en silence (`EX-CNT-031`).",
  "**Les quatre classes provisoires portent leur marque**, et un test vérifie qu'aucune donnée définitive ne les référence.",
  "Sept espèces des **trois** livres sont rejouées contre des valeurs recopiées à la main.",
  "Toute compétence citée par un historique existe au catalogue du `LOT-43` — vérifié par test.",
  "`ctest` : **983/983** (974 avant le lot, plus 9).",
]
sources = [
  "Basic Rules, p. 12-22 et p. 41-46",
  "Manuel des Joueurs, p. 12 et p. 41-44",
  "Tanares Player's Guide, p. 4, p. 57, p. 192-207 et p. 260-273",
]
+++

## Pourquoi

De quoi construire un personnage jouable au plus tôt : **22 espèces**, **13 historiques** et les
**4 classes simplifiées** qui serviront de socle au premier modèle de combat. Le
[LOT-13](LOT-13-fiche-de-personnage.md) suppose ces trois catalogues sans dire d'où ils viennent ; ils viennent d'ici.

## Périmètre

**39 fichiers de données** sous `Source/Elements/Rpg/` — `species/` (22), `backgrounds/` (13),
`classes/` (4, avec leurs 80 lignes de progression).

**`scripts/sourcebook/personnage.py`** et la sous-commande `python scripts/sourcebook personnage`.

**Trois modules partagés** extraits de ce que le `LOT-33` avait écrit pour lui seul :
`mise_en_page.py` (la grille à deux colonnes, mesurée une fois), `catalogues.py` (relire les
catalogues déjà livrés) et la déduplication des lignes surimprimées dans `extraction.py`. Le
bestiaire est reposé dessus, et produit une sortie **identique à l'octet près**.

**`core::loadCharacterOptions`** (`Source/Core/Rpg/CharacterOptions.{h,cpp}`) et **neuf tests**.

## Conception

### Trois documents, deux langues — et c'est là qu'est la difficulté

| Catalogue | Source | Pourquoi celle-là |
|---|---|---|
| 4 races + 6 sous-races | *Basic Rules* p. 12-22 | texte natif propre, blocs `TRAITS` réguliers |
| 5 races | *Manuel des Joueurs* | **seule** source française de leurs mécaniques |
| 4 espèces + 4 sous-espèces | *Player's Guide* | propres au monde de Tanares |
| 6 historiques | *Basic Rules* p. 41-46 | texte natif propre |
| 7 historiques | *Player's Guide* p. 260-273 | propres à Tanares |
| 4 classes simplifiées | *Player's Guide* p. 192-207 | socle **provisoire** du premier combat |

### Le *Manuel des Joueurs* est un scan, et sa graisse ment

La méthode du [LOT-33](LOT-33-bestiaire-de-base.md) — la graisse porte la structure — **ne s'applique pas** au
*Manuel*. Son OCR attribue les polices au hasard : sur le bloc du tieffelin, « Vitesse. » ne porte
aucune graisse, « Âge. » en porte sur deux fragments non contigus, et les titres eux-mêmes sont
mutilés — `TaiJJe` pour « Taille », `Vision dans Je noir`, `tliaumaturgie`, `d'wie`. Un détecteur
de titres n'y a rien à détecter.

Deux parades, et aucune n'est une relecture :

1. **Les mécaniques se lisent par leur phrase, pas par leur titre.** « votre valeur de Charisme
   augmente de 2 » se trouve quel que soit l'état du mot qui l'introduit. Les quatre champs qui
   comptent — augmentation, taille, vitesse, langues — ont chacun une formulation stable.
2. **Les noms de traits viennent du lexique** (`LOT-30`), qui porte les capacités raciales sous ses
   catégories `capacité (nain)`, `capacité (tieffelin)`… C'est exactement ce pour quoi il a été
   construit, et c'est la parade que le [LOT-43](LOT-43-options-de-personnage.md) employait déjà pour les dons.

**Et les augmentations sont recoupées.** Elles figurent deux fois dans le *Manuel* : dans le bloc de
la race, et dans la table « Augmentations raciales » de la page 12, dont l'ordonnée rattache chaque
race à sa caractéristique. Les deux doivent coïncider, sinon la génération s'arrête.

**Le recoupement a servi dès la première exécution.** L'OCR a **entièrement effacé** la ligne
d'augmentation du demi-elfe — et son « Âge. » avec —, si bien que son bloc commence au milieu d'une
phrase. La table de la page 12, elle, porte « Demi-elfe (+2) » sous Charisme, et c'est elle qui
restitue la valeur. La règle est celle du `LOT-43`, et elle distingue deux cas que rien ne distingue
à l'œil : une valeur **présente des deux côtés et différente** arrête la génération ; une valeur
**présente d'un seul côté** est une ligne escamotée, rapportée et non fatale.

#### Quatre corruptions d'OCR déclarées, une par une

`!'ore` pour « l'orc » — le `l` bas de casse ressort en point d'exclamation, le `c` en `e` ;
`commwi` pour « commun » ; `(+l)` pour `(+1)` sur **toute** la table des augmentations. Sans les
deux premières, le demi-orc sort ne parlant que le commun et le tieffelin sans le commun du tout.
Chacune est écrite dans une table avec ce que le scan a fait : une substitution non déclarée serait
indiscernable d'une règle du jeu.

**Le *Manuel* appelle le drakéide « Sangdragon ».** Le lexique fait autorité et dit « drakéide » ;
la graphie du livre est déclarée comme alias, exactement comme le `LOT-43` l'avait fait pour les
douze dons que les deux sources françaises traduisent différemment.

#### Sa gouttière bouge d'une page à l'autre

Les documents aidedd ont une gouttière fixe — `[287, 309]` — que le `LOT-33` avait mesurée. Le
*Manuel* n'en a pas : `[288, 309]` page 41, `[272, 296]` page 42, rien du tout page 44. Un blanc
figé y couperait tantôt dans une colonne, tantôt dans l'autre, et le texte des deux se mêlerait au
milieu d'une phrase. `mise_en_page.colonnes_de_page()` **mesure** donc le blanc qui contient le
milieu de la page, et rend une seule colonne quand il n'y en a pas.

### Le *Player's Guide* dessine ses titres deux fois

Ses titres sont composés en double, à la coordonnée exacte, à la police exacte, au texte exact — un
titre contourné, dont le remplissage et le trait forment deux passes de dessin. La superposition est
invisible à l'écran et **double tout ce qui se compte** : les douze espèces du chapitre 1 s'y
relèvent vingt-quatre fois. `Extracteur.lignes()` écarte désormais une ligne rendue deux fois au
même endroit ; le critère est volontairement strict — même ordonnée, même abscisse, même police,
même texte —, aucun document ne posant deux fois la même chaîne au même point pour deux raisons
différentes.

### Ce que le corpus dit, et que la feuille de route disait autrement

**Douze espèces de Tanares, pas treize.** Le sommaire du *Player's Guide* (p. 4) en porte douze :
nain, elfe, halfelin, humain, drakéide, gnome, orc, tieffelin, cirrus, gloomfolk, soulborn,
taii'maku — plus les quatre sous-espèces elfiques, comptées à part. Le chiffre est relevé, pas
estimé.

**Tanares ne porte aucune mécanique pour les huit espèces classiques.** Ses chapitres leur donnent
de l'histoire, du peuplement, des variantes optionnelles — jamais un bloc de traits. « La version de
Tanares fait foi sur le fond » se lit donc comme le corpus le permet : le **fond narratif** vient de
Tanares, la **mécanique** des livres français. L'**orc** est le seul nom que ceux-là n'ont pas — ils
ont le demi-orc — et il reste hors du catalogue faute de mécanique.

**Le livre annonce six historiques et en porte sept.** Son paragraphe d'ouverture énumère
cartographe, meneur, survivant pénombral, chasseur de dragons, occultiste et agent infiltré, et
oublie l'*imperial servant* que son propre sommaire liste p. 266. C'est le sommaire qui fait foi :
il indexe ce que le livre contient, la prose décrit ce que l'auteur avait en tête.

### Ce que le schéma ne peut pas dire

**Une augmentation « au choix du joueur ».** Presque toutes les espèces de Tanares accordent une
augmentation fixe *et* une au choix — « one other ability score of your choice increases by 1 » —,
et le cirrus n'accorde **que** du choix. Le schéma porte une table, qui ne sait pas dire « au
choix » ; en choisir une figerait la règle, et toutes les espèces de Tanares augmenteraient la même.
La partie fixe entre dans la table, le mécanisme `augmentation-de-caracteristique-au-choix` est
déclaré (`EX-CNT-030`), et `core::CharacterOptions::requiredMechanisms()` le liste au chargement
(`EX-CNT-031`).

**Une espèce sans taille ni vitesse.** Le *soulborn* est le cas d'espèce au sens propre : le livre
écrit « Your size is equivalent to your birth parents » et « Your base walking speed matches that of
your birth parents ». Ce n'est pas une extraction incomplète, c'est la règle — et
`species.schema.json` exige une taille et une vitesse, ce qu'il avait toutes les raisons de faire.
Lui en inventer une le rendrait jouable et faux. **L'espèce est écartée du catalogue, et la
génération le dit à chaque exécution** : la décision revient à un lot qui saura modéliser
l'héritage.

### Les quatre classes sont provisoires, et le déclarent

Brawler, mage, priest et scoundrel font tourner attaques, dégâts et tours sans exiger d'abord le
système complet de ressources de classe. Elles seront retirées au profit des seize classes complètes
du `LOT-47` et des `LOT-51` à `LOT-65`, et portent donc `status.provisoire` avec son critère de
retrait **écrit d'avance** (`EX-CNT-032`), à un seul endroit : quatre copies d'un critère divergent,
et celle qu'on lirait ne serait pas celle qui vaut.

Un test donne son sens au critère : il balaie **tout** `Source/Elements/Rpg/` et vérifie qu'aucune
donnée définitive ne cite `brawler`, `mage`, `priest` ni `scoundrel`. Le jour du retrait, supprimer
ces quatre fichiers ne cassera rien.

**Deux colonnes de la table des classes ne sont pas extraites**, et c'est délibéré. La *description*
et les *maîtrises d'armes et d'armures* courent sur trois lignes chacune et s'entrelacent avec leurs
voisines ; on en tire « A crafty rogue specialized in coordinat- » et « Light and medium armor,
shields, Constitution simple and martial weapons » — une phrase coupée au milieu d'un mot, et un
mélange de deux colonnes qu'aucun schéma ne refuserait. Les maîtrises relèvent de toute façon du
`LOT-34`.

### Un seuil de corps se pose sous la valeur, jamais dessus

Le corps rendu par un PDF est un **flottant** : 16 s'y lit parfois 15,999998, et 9,5 se lit 9,4599.
Un seuil posé à l'égalité laissait passer le titre de chapitre « RACES » et ratait les quatre races
— sans erreur, sans message, avec un catalogue à une entrée. Les seuils sont donc posés **sous** la
valeur mesurée, et la tolérance de comparaison (0,25 pt) reste sous le plus petit écart qui sépare
deux niveaux de titre du corpus : le demi-point entre le 9,5 d'un titre de capacité et le 9,0 de son
texte.

## Ce que le lot ne fait pas

**Il ne livre ni équipement ni maîtrises d'outils.** Les deux catalogues les référencent par
identifiant d'objet, et les objets sont le `LOT-34`. Forger ces identifiants maintenant créerait des
références pendantes que rien ne contrôlerait avant que le `LOT-34` ne les renomme.

**Il ne livre pas les seize classes complètes**, ni leurs 31 tables de progression : ce sont le
`LOT-47`, les `LOT-51` à `LOT-65` et le `LOT-84`.

**Il n'implémente aucune capacité.** Traits d'espèce et capacités d'historique sont du texte nommé ;
les jouer viendra avec les mécanismes qui les portent.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 983/983, `clang-format`, les six lints, cahier de test et Doxygen verts ; tous les critères d'acceptation sont cochés dans l'epic d'origine. Alimente [LOT-13](LOT-13-fiche-de-personnage.md), [LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27).

Exigences couvertes : [`EX-CNT-030`](../../../../../Documentation/Specification/contenu.md#EX-CNT-030), [`EX-CNT-031`](../../../../../Documentation/Specification/contenu.md#EX-CNT-031), [`EX-RPG-010`](../../../../../Documentation/Specification/rpg.md#EX-RPG-010), [`EX-RPG-011`](../../../../../Documentation/Specification/rpg.md#EX-RPG-011).
