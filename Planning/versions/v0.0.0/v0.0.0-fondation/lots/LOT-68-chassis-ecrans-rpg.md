+++
id = "LOT-68"
titre = "Le châssis des écrans du RPG"
version = "0.0.0"
filiere = "interface"
statut = "livre"
taille = "M"
resume = "Les huit écrans du RPG existent, se naviguent et se valident avant d'être remplis, et un neuvième s'ajoute par une entrée de table."
prerequis = [
  "LOT-66",
  "LOT-76",
  "LOT-67",
]
livrables = [
  "`hmi::rpgScreens()` : la table de données pures qui décrit l'ossature des huit écrans (blocs de sept genres).",
  "`hmi::RpgScreenFrame` : le châssis commun — cadre, navigation, parcours de focus, pied d'actions toujours visible.",
  "La règle de superposition portée par chaque écran (`EX-IHM-091`) et appliquée par `MainWindow`.",
  "L'habillage par rôle (`rpgRole`) dans `theme-identity.qss`, et les clés de l'ossature dans les deux catalogues.",
  "« Nouvelle partie » ouvre le châssis sur la fiche de personnage (`MainWindow::newGame`).",
]
criteres = [
  "**Les huit écrans existent** et s'ouvrent, même vides.",
  "**Un écran s'ouvre et se ferme depuis n'importe quel autre sans repasser par le menu principal** — le cycle est fermé, dans les deux sens, à la souris comme aux gâchettes de la manette.",
  "**Ajouter un neuvième écran ne demande de toucher à aucun des huit** : une entrée dans `hmi::rpgScreens()`, ses clés dans les deux catalogues, rien d'autre — ni code d'interface, ni feuille de style.",
  "**Chaque clé de l'ossature existe dans les deux catalogues**, vérifié par test : rien d'autre qu'une table ne les rattache au code.",
  "**Le pied d'actions reste visible** quel que soit le contenu, et aucun écran ne contraint la taille de la fenêtre (`EX-IHM-080`).",
  "`ctest` : **1014/1014** (1007 avant, plus les sept cas de ce lot).",
]
+++

## Pourquoi

Huit écrans manquaient au jeu, et aucun n'existait même en ébauche : fiche de personnage,
inventaire et équipement, journal de quêtes, carte du monde, dialogue, marchand, tableau de la
Guilde, affichage tête haute de combat.

Ce lot ne les **remplit** pas — c'est le travail du `LOT-38` pour la fiche, du `LOT-42` pour la
carte, du `LOT-45` pour la guilde, du [LOT-24](LOT-24-ihm-combat.md) pour le combat. Il livre ce qu'ils ont
en commun et qu'aucun ne doit réinventer : le **cadre**, la **navigation**, le **parcours de
focus**, la **règle de superposition**, et l'**ossature** de chacun.

## Conception

### Ce que le joueur voit

« Nouvelle partie » ouvre la fiche de personnage. Le pied de page porte trois entrées permanentes —
**Fermer**, **Écran précédent**, **Écran suivant** — et les rappels de touches correspondants :
`LB/RB` change d'écran, `A` valide, `ÉCHAP` revient. Huit clics sur « Écran suivant » font le tour
et ramènent à la fiche.

Les huit écrans sont **vides**, et le montrent : chaque valeur affichée est un tiret cadratin. Ce
n'est pas un oubli, c'est le périmètre — une valeur d'exemple posée dans une fiche se lirait comme
un état du jeu et mentirait à la première lecture (`EX-IHM-072`).

| Écran | Ce que l'ossature annonce |
|---|---|
| Fiche de personnage | Identité, progression, six caractéristiques ; combat, six jets de sauvegarde, compétences |
| Inventaire et équipement | Huit emplacements d'équipement, charge et bourse ; sac de trente cases |
| Journal de quêtes | Liste de quêtes ; détail et objectifs |
| Carte du monde | Régions et lieu courant ; cadre de la carte |
| Dialogue | Portrait et attitude de l'interlocuteur ; réplique et quatre réponses |
| Marchand | Marchandises et bourse ; votre sac |
| Tableau de la Guilde | Contrats ; contrat sélectionné, commanditaire, rang, récompense |
| Combat | Ordre d'initiative, cible, barre d'actions |

Ces champs ne sont pas inventés : ils sont **relevés sur les modèles déjà livrés** —
`core::CharacterSheet` ([LOT-13](LOT-13-fiche-de-personnage.md)), `core::Ability` ([LOT-12](LOT-12-des-caracteristiques-jets.md)),
`core::Equipment` ([LOT-34](LOT-34-equipement.md)). Une ossature qui annonce des champs que le modèle ne
porte pas promet ce que le jeu ne pourra pas tenir.

### L'ossature est une table, et c'est tout le lot

Le critère d'acceptation de la feuille de route disait : *ajouter un neuvième écran ne demande de
toucher à aucun des huit*. Il ne se tient pas avec huit fichiers d'interface, fussent-ils bien
écrits — le premier pied de page à corriger le serait huit fois, et neuf au lot suivant.

L'ossature vit donc dans `hmi::rpgScreens()` : une **table de données pures**, sans Qt, testable
seule. Elle nomme pour chaque écran son identifiant d'objet, sa clé de titre, sa règle de
superposition, et la liste de ses blocs — chacun d'un des **sept genres** que le châssis sait
peindre (champs, grille, liste, prose, portrait, piste, barre d'actions). `hmi::RpgScreenFrame` ne
connaît **aucun** écran par son nom : il peint ce que la table décrit.

C'est une entorse assumée à la convention « la mise en page hors code », qui veut un `.ui` par
écran. La mise en page reste pourtant déclarative : elle a seulement changé de format, de XML à
table C++, parce que ce que ces huit écrans partagent pèse plus lourd que ce qui les distingue.

**Même règle pour la feuille de style** : les blocs sont habillés par **rôle** (propriété dynamique
`rpgRole`), jamais par nom d'objet. Tenir dans `theme-identity.qss` la liste des blocs décrits par
la table les ferait diverger au premier écran ajouté — la leçon des sections de l'écran de crédits
([LOT-66](LOT-66-charte-visuelle.md)), et elle pèse plus lourd ici.

### La règle de superposition, portée par l'écran et non par l'appelant

`EX-IHM-091` : chaque écran déclare s'il **suspend** la simulation ou s'il se consulte **en
marchant**. La carte et l'ATH de combat ne suspendent pas — on ouvre une carte pour savoir où l'on
va sans s'arrêter, et l'ATH de combat *est* le jeu pendant un combat. Les six autres suspendent.

La règle appartient à la description de l'écran parce qu'un même écran s'ouvrira depuis la pause,
depuis le jeu et depuis une touche : décidée au point d'appel, elle se contredirait d'un appel à
l'autre sans que rien ne le signale. `MainWindow` l'applique, il ne la redécide pas — y compris au
**passage** d'un écran à l'autre, où elle peut changer.

### « Nouvelle partie » : un échafaudage, et il est écrit

Cette entrée devrait ouvrir une carte. Elle n'en a aucune : le [LOT-01](LOT-01-fork-purge.md) a purgé les
niveaux du jeu de plateforme, `demo-deplacement.json` n'existe pas, et le [LOT-67](LOT-67-menus-vocabulaire-rpg.md)
avait écrit ce constat plutôt que de le laisser découvrir. Elle chargeait donc un fichier absent.

Elle ouvre désormais le **châssis**, sur la fiche de personnage. Ce n'est pas un pis-aller : huit
écrans qu'aucun chemin n'atteint ne se relisent pas, ne se naviguent pas et ne se valident pas — et
c'est justement pour les valider que ce lot existe. La ligne à remplacer le jour où il y aura une
carte à charger est **une seule**, dans `MainWindow::newGame`, et elle le dit.

Conséquence à assumer : `ScreenId::Game` et l'écran de **pause** ne sont plus atteignables depuis
le menu tant que cette ligne n'est pas rendue à son usage. Ils ne sont ni retirés ni modifiés, et la
table de transitions déclare déjà — et teste — l'ouverture d'un écran du RPG **depuis le jeu** et
**depuis la pause**, avec le retour vers l'écran d'origine. Les entrées RPG de l'écran de pause que
le `LOT-67` annonçait s'y brancheront sans rien réécrire d'autre.

### Trois défauts d'agencement, trouvés en regardant l'écran

Aucun n'était visible dans le code, et aucun ne l'aurait été sans ouvrir l'application :

- **Le pied d'actions passait sous la ligne de flottaison.** La pile d'écrans enveloppe déjà chaque
  page dans une zone défilante (`EX-IHM-080`), ce qui suffit à ne pas contraindre la fenêtre — mais
  pas à garder « Écran suivant » à l'écran. Une **seconde** zone défilante, intérieure au châssis,
  borne la hauteur du contenu seul : le pied reste posé au bas du cadre, quoi qu'il porte.
- **Le bandeau de titre débordait de la fenêtre.** À la taille des titres d'écran, « Inventaire et
  équipement » et ses deux ailes réclamaient à eux seuls plus de 1280 pixels — et comme un écran ne
  contraint pas la fenêtre, c'est la colonne de droite qui sortait du cadre, sans qu'aucune erreur
  ne soit levée. Ces titres prennent la taille des titres de **section** ; les autres écrans du jeu
  tiennent en un ou deux mots, ceux-ci en comptent jusqu'à trois.
- **Les rappels de touches imposaient leur largeur.** Ils l'ont perdue : une aide ne décide pas de
  la largeur d'une fenêtre, elle s'efface quand la place manque.

## Ce que le lot ne fait pas

**Il ne remplit aucun écran** — c'est le périmètre, et les quatre lots consommateurs sont nommés
ci-dessus.

**Il ne peint pas la scène derrière les écrans qui se consultent en marchant.** Ils sont, comme
Options et Crédits, des **pages** de la pile d'écrans. Le recouvrement leur reviendra quand il y
aura une scène à laisser voir derrière eux : aujourd'hui, il se peindrait sur du vide.

**Il ne livre pas de planche de maquette.** La matière de conception de la fiche appartient au
`LOT-38`, qui porte explicitement « maquette et interface » ; les sept autres écrans suivront leur
lot. Ce qui est livré ici est le châssis, et il se regarde dans l'application.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1014/1014, `clang-format`, les sept lints, cahier de test et Doxygen verts. Vérification manuelle : les huit écrans ouverts et parcourus dans l'application. Tous les critères d'acceptation sont cochés dans l'epic d'origine.

Exigences **ajoutées** : [`EX-IHM-090`](../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-090) (châssis commun, ossature en données), [`EX-IHM-091`](../../../../../Documentation/Specification/interface-ihm.md#EX-IHM-091) (règle de superposition).
