+++
id = "LOT-14"
titre = "Inventaire et équipement"
version = "0.0.0"
filiere = "regles"
statut = "livre"
taille = "M"
resume = "Porter, équiper et retirer des objets change la fiche d'un montant mesurable, sans qu'aucune statistique ne puisse dériver."
prerequis = ["LOT-13", "LOT-34"]
livrables = [
  "`core::Inventory`, sans aucune statistique stockée, et `core::derivedStatsFor`, qui recalcule classe d'armure, poids et encombrement à chaque lecture.",
  "`rules/encumbrance.json` : capacité de charge, seuils d'encombrement et pénalités de vitesse, chacun avec la phrase du corpus qui l'atteste.",
  "`Weapon` lit le tableau `properties` que le schéma prévoyait.",
  "L'écran d'inventaire du LOT-68 rempli : `hmi::inventoryValues` (seize emplacements, charge, bourse, sac).",
  "La classe d'armure de la fiche tirée de l'équipement porté.",
  "Treize cas de test, dont `LaClasseDArmureNeDerivePasAvecLOrdre` (six ordres).",
]
criteres = [
  "Équiper une armure change la CA du montant attendu ; la retirer restitue exactement la valeur d'origine, quel que soit l'ordre des opérations.",
  "Un test équipe et retire dans plusieurs ordres et vérifie l'absence de dérive.",
  "Capacité de charge respectée, avec un comportement défini au dépassement.",
  "Catalogue d'objets entièrement en données.",
  "`ctest` : 1034/1034 (1021 avant, plus les treize cas de ce lot).",
]
sources = [
  "Corpus, p. 68 (capacité de charge et encombrement, relevés dans `rules/encumbrance.json`)",
]
+++

## Pourquoi

Porter, équiper et consommer des objets, avec un effet **mesurable** sur la fiche.

## Périmètre

**Il n'édite rien.** L'inventaire se lit ; équiper et déséquiper depuis l'écran est un autre sujet.
**Il ne consomme rien** : `removeFromBackpack` existe et est testé, mais aucun geste de jeu ne
l'appelle encore. **Il n'applique pas le désavantage** de l'encombrement lourd : l'état est calculé
et nommé, et c'est le système de jets qui l'appliquera.

## Conception

### Le piège, et la façon dont il est écarté

Le critère d'acceptation le disait : *retirer un équipement doit annuler exactement son effet*.
L'erreur classique est d'appliquer un bonus en l'ajoutant à la volée (`ca += 2`) et de le retrancher
au retrait ; après trois équipements et deux retraits dans le désordre, la classe d'armure a
**dérivé**, et rien ne le signale — le personnage est simplement devenu un peu plus, ou un peu
moins, résistant.

La parade n'est pas un test, c'est une **absence de champ**. `core::Inventory` ne porte aucune
statistique : ni classe d'armure, ni poids total, ni encombrement. Toutes sont des **fonctions** de
ce qu'il contient (`core::derivedStatsFor`), recalculées à chaque lecture. Une valeur qu'on ne
stocke pas ne peut pas dériver ; il n'y a rien à annuler, puisqu'il n'y a rien eu à appliquer.

Le test `LaClasseDArmureNeDerivePasAvecLOrdre` joue quand même **six ordres** différents d'équipement
et de retrait et vérifie qu'on retombe exactement sur la valeur de départ — non pour rattraper le
défaut, mais pour que la propriété reste vraie le jour où quelqu'un ajoutera un champ « pour aller
plus vite ».

### Ce que la règle dit, et où elle est écrite

La capacité de charge est **relevée sur le livre**, pas devinée : `rules/encumbrance.json` porte la
capacité (7,5 kg par point de Force), les deux seuils d'encombrement (2,5 et 5 kg par point) et les
pénalités de vitesse (3 et 6 mètres) — chacun avec la **phrase du corpus** qui l'atteste, page 68.
`EX-VIS-007` interdit qu'un `7500` nu vive dans le C++ : il ne dirait pas ce qu'il représente, et
l'ajuster demanderait une recompilation.

**Tout est en grammes.** Le livre écrit des kilogrammes ; les catalogues du [LOT-34](LOT-34-equipement.md)
donnent déjà des grammes entiers. Mêler les deux unités dans une somme donnerait un sac de cinq
cents kilos pour une poignée de fléchettes, et l'unité unique supprime la question.

Le **comportement au dépassement** est défini ici plutôt que laissé à chaque appelant : au-delà de
la capacité, le personnage ne porte plus — la vitesse tombe à zéro. Entre les deux seuils, elle est
diminuée de ce que le livre dit.

### Ce que le lot n'invente pas

**Aucune règle de compatibilité d'emplacement.** Ranger une armure à l'emplacement de tête est une
question de donnée et d'interface, pas du noyau. Ce que `equip` garantit est plus étroit et plus
utile : un emplacement ne porte jamais deux choses, et ce qui en sort est **rendu à l'appelant**
plutôt que perdu.

**C'est la catégorie qui décide, pas l'emplacement.** Un bouclier rangé au torse compte comme un
bouclier — il *ajoute* à la classe d'armure au lieu de la remplacer —, parce que c'est sa catégorie
qui porte cette règle. Confondre les deux donnerait un personnage en bouclier seul avec une CA de 2.

**La finesse attend sa donnée.** La caractéristique d'attaque suit l'arme : Force au corps à corps,
Dextérité à distance. Une arme de *finesse* laisse le choix, et la branche existe — mais aucune arme
du catalogue ne déclare encore cette propriété autrement qu'en toutes lettres dans son texte
français, et lire une règle dans de la prose est exactement ce que ce projet évite. `Weapon` lit
désormais un tableau `properties` que le schéma prévoit déjà ; il se remplira au
[LOT-49](../../../../vision/archives/feuille-de-route-jeu.md#lot-49).

### L'écran

L'inventaire est l'un des neuf écrans du [LOT-68](LOT-68-chassis-ecrans-rpg.md), et il garde son rendu générique :
seize emplacements, la charge, la bourse, le sac. Ce lot ne change pas le châssis — il **remplit**
son ossature, en câblant chaque champ sur un identifiant que `hmi::inventoryValues` produit.

Deux choses y sont des règles d'**affichage**, et vivent donc dans l'écran et non dans le modèle :
la bourse, que le modèle compte en pièces de cuivre et que l'écran répartit en or, argent et cuivre
comme la feuille du corpus le fait ; et les poids, comptés en grammes et lus en kilogrammes — un sac
de 12 450 g ne se lit pas.

La **classe d'armure de la fiche** vient désormais de l'équipement porté : elle est calculée à la
construction, sans rien savoir de l'armure endossée depuis, et c'est la statistique dérivée qui la
remplace quand on en fournit une. C'est ce qui rend le critère du lot visible à l'écran — le
personnage de démonstration passe de 11 à 15 dès qu'il porte son cuir clouté et son bouclier.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔ (six ordres joués)
2. ✔
3. ✔
4. ✔ (125 objets, schéma et validation en CI)
5. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1034/1034, `clang-format`, les huit lints, cahier de test et Doxygen verts ; vérification manuelle : l'inventaire ouvert, rempli et parcouru dans l'application.

Aucune exigence ajoutée : `EX-INV-020` couvre ce lot.
