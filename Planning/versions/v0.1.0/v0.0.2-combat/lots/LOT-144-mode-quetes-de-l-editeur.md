+++
id = "LOT-144"
titre = "Éditeur — le mode Quêtes"
version = "0.0.2"
filiere = "editeur"
statut = "a-faire"
taille = "L"
resume = "Une quête s'écrit dans l'éditeur, à côté des cartes qu'elle traverse : ses drapeaux, ses étapes, ses textes de journal, et l'état de partie de chaque étape en un clic."
prerequis = ["LOT-120", "LOT-126"]
livrables = [
  "Un **mode Quêtes** dans la fenêtre de l'éditeur (un onglet à côté des cartes) : la liste des quêtes de `World/quests`, en créer, renommer, supprimer.",
  "L'écriture d'une quête sans JSON : ses **drapeaux déclarés** (identifiant, valeurs, initiale), ses **étapes** (identifiant, conditions `isSet` / `equals` / `notEquals` choisies parmi les drapeaux et valeurs connus, effets `setFlag` / `clearFlag`, issue), dans l'ordre du récit.",
  "Les **textes du journal** (`quest.<quête>.title`, `quest.<quête>.<étape>`) saisis dans chaque catalogue de langue, comme les noms de carte (`LOT-EDITOR-07`).",
  "Le fichier écrit est **canonique** et relu par `core::readQuest` avant d'être enregistré : une quête que le jeu refuserait ne s'écrit pas ; l'erreur nomme l'étape, comme `--check` nomme la ligne.",
  "**Qui s'en sert** : pour chaque drapeau et chaque valeur, les entités des cartes (présence, déclencheurs, portails), les dialogues et les autres quêtes qui le lisent ou le posent ; un clic ouvre la carte sur l'entité (`carte#id`).",
  "Une étape peut nommer **où elle se joue** (`at`, une entité `carte#id`) : le mode y mène, et `--check` suit la citation (source `EntityRefs`) — la part du `LOT-126` qu'aucun format ne portait encore.",
  "**Jouer l'étape** : choisir une étape règle l'état de partie du canevas (`LOT-126`) sur des valeurs qui l'atteignent ; l'essai (`P`, `F5`) part de là.",
  "Renommer un drapeau ou une valeur déclarée **propose** de renommer ses usages dans les cartes, les dialogues et les quêtes, par un plan puis une écriture (`LOT-EDITOR-14`).",
]
criteres = [
  "La quête « Des pommes pour l'arène » (`LOT-120`) se réécrit entièrement dans le mode, sans ouvrir son fichier : le fichier produit est identique octet pour octet à celui livré.",
  "Sous « acceptee » choisie comme étape jouée, le garde et l'enfant paraissent au canevas d'Arenarea sans autre geste.",
  "Renommer la valeur `condamne` met à jour la quête, le dialogue du garde et la condition de présence des portes de l'arène ; `--check` reste vert.",
  "Une étape sans condition, ou qui compare une valeur non déclarée, ne s'enregistre pas.",
  "Vérifié à la main, comme tout lot de l'éditeur ([définition de « livré »](../../../../standards/definition-de-livre.md)) : créer une quête de trois étapes, la jouer, la supprimer.",
]
+++

## Pourquoi

Jusqu'au `LOT-126`, l'éditeur écrit **les cartes** d'une quête — ses PNJ conditionnés, ses portes,
ses déclencheurs — et laisse **la quête** au JSON, décision prise à l'audit de l'éditeur. L'auteur
l'a revue le 24 septembre 2026 ([D-24](../../../../vision/decisions.md)) : une quête ne se relit
pas sans les cartes qu'elle traverse, et ses valeurs de drapeau se tapent à la main à trois
endroits (quête, dialogue, carte) où une faute de frappe ne se voit qu'au `--check`. La
`0.0.3` rapporte « Les enfants de Martpart » ([LOT-155](../../v0.0.3-capitale-intra-muros/lots/LOT-155-quete-les-enfants-de-martpart.md)),
puis chaque version apporte ses quêtes : elles s'écriront là où l'on voit leurs cartes.

## Périmètre

La **quête** telle que le `LOT-116` la définit — drapeaux déclarés, étapes lues dans les drapeaux,
effets, issues — et ses textes de journal. Le format ne gagne qu'un champ facultatif, `at`, sur
l'étape.

**Pas dedans** :

- les **dialogues** : leur graphe (`LOT-15`) reste écrit en données ; le mode Quêtes montre ceux qui
  lisent ou posent un drapeau, il ne les édite pas ;
- un **graphe** de quête : une étape se lit dans le monde (décision 2 du `LOT-116`) ; le mode montre
  les étapes dans l'ordre du récit et, pour chacune, les valeurs qui l'atteignent ;
- les récompenses, l'expérience : elles n'existent pas encore dans le format.

## Conception

- Le mode vit dans `Source/Editor` comme le reste : logique pure dans `Editor/Logic` (lecture,
  écriture canonique, usages, renommage), widgets en code dans `Editor/Ui`. Le chargeur et la
  validation restent ceux de `core` : le mode n'a pas sa propre idée d'une quête valide.
- L'écriture canonique est la condition du critère « octet pour octet » : elle se fixe **avant** que
  la quête de la démo soit livrée, ou la quête de la démo s'écrit sous cette forme (`LOT-120`).
- Les usages se calculent comme `--who-cites` (`LOT-EDITOR-14`) : une table, des fonctions pures,
  un test bloquant.

## Risques et questions ouvertes

- Renommer une valeur touche des fichiers de trois sortes : le plan doit être relu avant d'écrire,
  comme `--rename-map`.
- Le champ `at` : faut-il un **lieu** (une carte) plutôt qu'une entité, pour une étape qui se joue
  partout ? À trancher en réalisant ; l'entité suffit à la quête de la démo.
