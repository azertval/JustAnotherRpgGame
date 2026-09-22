+++
id = "LOT-39"
titre = "Plomberie des clés d'assets"
version = "0.0.0"
filiere = "donnees"
statut = "livre"
taille = "M"
resume = "Le jeu tourne complet — trois cents entrées affichables — avant qu'une seule illustration ne soit produite : la production graphique devient un remplacement progressif."
prerequis = [
  "LOT-33",
  "LOT-34",
  "LOT-37",
  "LOT-38",
]
livrables = [
  "`core::isValidAssetKey`, qui emploie la même expression que `common.schema.json`.",
  "`core::expectedAssetKeys` : le manifeste dérivé des catalogues (308 clés attendues, sept familles), sans fichier commité.",
  "`Assets/Entities/families.json` : les familles, leurs dimensions et les dossiers de catalogue qui les alimentent.",
  "Le **marqueur généré** déterministe (teinte par hachage FNV-1a, deux diagonales) pour toute clé sans image.",
  "`check_asset_keys.py` : échoue sur une clé orpheline ou une image inattendue, liste sans échouer les clés servies par un marqueur.",
]
criteres = [
  "**Aucune clé d'asset orpheline dans les catalogues.** ✔ (vérifié par test *et* par lint, dans les deux sens — clé sans donnée, image sans clé)",
  "**Toute clé sans image obtient un marqueur généré.** ✔ (les 308 clés attendues, une par une)",
  "**La CI liste les clés sans art définitif sans échouer.**",
  "`ctest` : **1056/1056** (1047 avant, plus les neuf cas de ce lot).",
]
+++

## Pourquoi

Que le jeu tourne **complet** — trois cents entrées affichables — avant qu'une seule illustration
ne soit produite.

## Conception

### Une clé, jamais un chemin

Une donnée désigne son illustration par une **clé** : `beast/wolf`. Jamais
`Assets/Entities/beast/wolf.png` (`EX-CNT-040`). Un chemin dans une donnée de règle lie le
catalogue à l'arborescence du disque : le jour où l'on range autrement, ce sont des créatures qui
cassent, et le lien entre le déplacement de dossier et la créature sans image ne se voit pas.

`core::isValidAssetKey` refuse donc explicitement ce qui ressemble à un chemin — y compris
`beast/wolf/token`, qui est un chemin déguisé en clé à trois segments — et emploie **la même
expression** que `common.schema.json` : la donnée refusée à la validation est exactement celle que
le moteur refuse.

### La clé se déduit ; le champ `asset` sert à déroger

Trois cents entrées auraient sinon porté trois cents lignes recopiées, et la première faute de
frappe aurait donné une créature sans image sans que rien ne l'explique. Une créature `wolf.json`
attend donc `beast/wolf` **sans rien écrire**.

Le champ `asset` d'une donnée existe pour le cas qui ne se déduit pas : deux entrées qui partagent
une illustration. C'est une **dérogation**, pas une répétition — et c'est pourquoi le contrôle
d'unicité ne porte que sur les clés *dérivées* : deux dérogations vers la même image sont
légitimes, deux clés dérivées identiques révèlent deux entrées de même identifiant.

### Le manifeste est dérivé, jamais tenu à la main

`core::expectedAssetKeys` énumère les clés qu'attendent les catalogues, en les parcourant. Il n'y a
**aucun fichier de manifeste commité** : il divergerait du catalogue au premier ajout de créature,
et c'est la copie oubliée qu'on lit six mois plus tard. Le dériver coûte un balayage de dossier et
ne peut pas mentir.

Les **dimensions** attendues, en revanche, sont de la donnée : `Assets/Entities/families.json`
déclare chaque famille, sa taille et les dossiers de catalogue qui l'alimentent. Un `96` nu dans un
calcul de découpe ne dirait pas ce qu'il représente, et l'ajuster demanderait une recompilation
(`EX-VIS-007`). Les familles y sont **lues**, jamais énumérées en C++ : le vocabulaire grandira —
sorts, PNJ, quartiers de guilde — et une énumération fermée obligerait chaque lot suivant à
recompiler pour ajouter une famille.

Sept familles à ce jour, **308 clés attendues** : 94 créatures, 125 objets, 37 armes, 13 armures,
22 espèces, 13 historiques, 4 classes. Compétences, langues et dons n'attendent aucune
illustration, et ne figurent donc pas.

### Le marqueur, et pourquoi il est déterministe

`EX-CNT-041` : toute clé sans image obtient un **marqueur généré**. Sans lui, trois cents entrées
s'afficheraient comme des trous, et la production graphique deviendrait un préalable bloquant —
rien ne pourrait être joué ni relu avant elle. Avec, elle devient un **remplacement progressif** :
une image à la fois, et le jeu ne cesse jamais d'être jouable.

La même clé donne **toujours** le même marqueur. Un marqueur tiré au hasard changerait à chaque
lancement : le loup ne serait plus reconnaissable d'une partie à l'autre, deux captures du même
combat différeraient, et un test ne pourrait rien en dire. La teinte vient donc d'un hachage
**écrit dans le projet** (FNV-1a) plutôt qu'emprunté à `std::hash`, dont la valeur n'est pas
garantie d'une plateforme ou d'une version de bibliothèque à l'autre — le marqueur d'un loup
changerait de teinte au changement de compilateur, une régression invisible en développement et
visible chez le joueur.

Deux diagonales barrent la vignette : un marqueur doit se voir **comme** un marqueur, et ne jamais
passer pour une illustration définitive livrée un peu vite.

Une clé **malformée** ne reçoit pas de marqueur. Lui en donner un la ferait passer pour une entrée
en attente d'illustration, alors qu'elle est une faute de donnée.

### Le lint fait deux choses de nature différente

C'est le cœur du critère d'acceptation, et il est délibéré :

| Cas | Ce que fait `check_asset_keys.py` |
|---|---|
| clé orpheline — dérogation malformée, ou famille inconnue | **échoue** (`EX-CNT-040`) |
| illustration livrée qu'aucune donnée n'attend | **échoue** — elle ne serait jamais affichée |
| clé encore servie par un marqueur | **liste**, et n'échoue pas (`EX-CNT-041`) |

Un lint qui échouerait sur la troisième ligne rendrait le dépôt rouge jusqu'à la dernière
illustration livrée, et plus personne ne lirait sa sortie. C'est un état d'avancement, pas un
défaut.

## Ce que le lot ne fait pas

**Il n'affiche rien.** Le marqueur est produit et vérifié, mais aucun écran ne montre encore de
créature : le premier consommateur sera l'éditeur ([LOT-11](LOT-11-editeur-multicouches.md)) ou le contenu du
[LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27). Le dire plutôt que de laisser croire que le bestiaire est à l'écran :
la plomberie est posée, le robinet n'est pas encore branché.

**Il ne produit aucune illustration.** C'est tout son objet : rendre la production graphique
progressive au lieu de bloquante.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1056/1056, `clang-format`, les neuf lints, cahier de test et Doxygen verts ; tous les critères d'acceptation sont cochés dans l'epic d'origine.

Aucune exigence ajoutée : `EX-CNT-040` et `EX-CNT-041` couvrent ce lot, et c'est leur premier emploi.
