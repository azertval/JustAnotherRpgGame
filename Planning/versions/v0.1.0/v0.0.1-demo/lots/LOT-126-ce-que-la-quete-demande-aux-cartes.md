+++
id = "LOT-126"
titre = "Ce que la quête demande aux cartes"
version = "0.0.1"
filiere = "editeur"
statut = "a-faire"
taille = "M"
resume = "Tout ce que « Des pommes pour l'arène » pose sur une carte se pose, se voit et se contrôle dans l'éditeur : un PNJ présent selon un drapeau, une porte close, un escalier condamné, un déclencheur."
prerequis = ["LOT-116"]
livrables = [
  "La **condition de présence** du LOT-116 déclarée dans `EntityKinds` pour toute famille : drapeau, opérateur, **valeur** ; l'inspecteur propose les valeurs que la quête déclare.",
  "Le catalogue des drapeaux lit les **quêtes**, plus seulement les dialogues ; `--check` refuse une valeur qu'aucune quête ne déclare, et suit les `carte#id` que les quêtes citent (source `EntityRefs`, prête depuis le `LOT-EDITOR-05`).",
  "Une famille **`prop`** : une pièce du lieu posée comme entité, avec sa condition de présence et la collision de son emprise — les portes de l'arène, closes sous `condamne`.",
  "`portal.sealed` : un portail **condamné**, sans arrivée, que le contrôle accepte et que le graphe des cartes montre en pointillé.",
  "`zone.trigger` : ce qu'une zone déclenche à l'entrée — un dialogue, un drapeau posé, un transfert vers `carte` + arrivée.",
  "Le canevas **sous un état de partie** : un sélecteur de valeurs de drapeaux grise ce qui est absent ; l'essai (`P`, `F5`) part de cet état.",
]
criteres = [
  "Sur une carte de test, le garde et l'enfant paraissent sous `acceptee` et disparaissent sous `enfant-libere`, au canevas comme dans l'essai, sans recharger.",
  "`--check` passe sur une carte qui porte un escalier condamné, et échoue sur la même carte si `sealed` est retiré.",
  "`--check` atteint le vestiaire A par le **transfert** de la zone du parvis : une case que seul un déclencheur dessert n'est pas « inatteignable ».",
  "Aucune ligne de code par famille dans `Source/Editor` : le test bloquant du contrat d'extension reste vert.",
]
+++

## Pourquoi

La [quête de la démo](../quete-demo.md) introduit quatre choses qu'aucune carte n'a jamais portées,
et que l'éditeur ne sait pas écrire ([audit](../../../../standards/audit-editeur.md), §3) : un PNJ
conditionné, un **décor** conditionné (une pièce vit dans une couche, sans condition), un portail
volontairement sans arrivée, un déclencheur. Sans ce lot, elles s'écriraient à la main dans le
JSON — le retour des scripts que le `LOT-EDITOR-06` a retirés.

## Périmètre

Le **côté carte** du mécanisme. Le LOT-116 livre les drapeaux, les quêtes et la condition dans
`core` ; ce lot les déclare au contrat, les fait contrôler et les montre. La part « jeu » de `prop`
et de `zone.trigger` (les composer, les jouer) est dedans : ce sont des familles d'entité, pas des
fonctions d'éditeur.

**Pas dedans** : écrire les dialogues et les quêtes — des données, hors de l'éditeur par décision.

## Risques

- `prop` recoupe les couches de décor. La règle : ce qui **change** en cours de partie est une
  entité, tout le reste une pièce de couche. À écrire dans le guide d'usage.
