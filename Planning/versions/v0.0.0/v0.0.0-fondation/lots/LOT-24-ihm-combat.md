+++
id = "LOT-24"
titre = "IHM de combat"
version = "0.0.0"
filiere = "interface"
statut = "livre"
taille = "L"
resume = "Le combat du Colisée est lisible et se joue entièrement au clavier ou entièrement à la manette, avec des prévisualisations qui sont le jet réel."
prerequis = ["LOT-21", "LOT-23"]
livrables = [
  "`Source/Core/Combat/CombatPreview.{h,cpp}` : `core::previewAttack`, `core::firstValidAttack`, `core::previewMove`.",
  "Dans l'arène : `circumstancesAgainst`, `previewOpportunities`, `setTakesOpportunities`, et le prédicat unique `provokes`.",
  "`hmi::GamepadNavigator` et `hmi::ButtonRepeat` : la manette dans le jeu Qt Quick.",
  "`hmi::ArenaModel` : curseur de ciblage, actions du tour, confirmation, chemin, prévisualisation.",
  "À l'écran : le calque `ArenaMark`, la barre d'actions numérotée, le panneau de prévisualisation, l'aide des commandes.",
  "Cinquante-huit entrées de traduction ; `JadgRuntime` dans `qt_add_translations`.",
  "L'exécutable de test `RuntimeTests`, premier test d'une vue-modèle Qt Quick.",
]
criteres = [
  "Un combat complet se joue entièrement au clavier et entièrement à la manette.",
  "Chaque jet affiché est traçable au journal.",
  "`check_ui_layers.py` vert.",
  "Traduction fr/en complète.",
  "Vérification IHM manuelle par l'utilisateur.",
  "`ctest` : 1145/1145 (1139 avant ; deux tests de prévisualisation, un de répétition, trois de vue-modèle).",
]
+++

## Pourquoi

Rendre le combat **lisible et jouable** : ordre d'initiative, portées surlignées, curseur de
ciblage, prévisualisations, journal — et le jouer **entièrement au clavier et entièrement à la
manette**.

## Périmètre

### Ce que ce lot livre

- **La prévisualisation** (`Source/Core/Combat/CombatPreview.{h,cpp}`) : l'attaque telle qu'elle
  serait jetée (`core::previewAttack` — ciblage, CA abri compris, jet requis, chance de toucher,
  sources d'avantage et de désavantage, espérance de dégâts), la première attaque qui porte
  (`core::firstValidAttack`), et le déplacement tel qu'il serait joué (`core::previewMove` — chemin,
  déplacement restant, qui frappera en chemin).
- **Dans l'arène** : les circonstances de la session exposées (`circumstancesAgainst`), la
  prévisualisation des attaques d'opportunité (`previewOpportunities`), et le choix du joueur de
  **laisser passer** ses opportunités (`setTakesOpportunities`). La règle d'une attaque
  d'opportunité est désormais **un seul prédicat** (`provokes`), que le déplacement et sa
  prévisualisation partagent.
- **La manette dans le jeu** (`hmi::GamepadNavigator`, `hmi::ButtonRepeat`) : un objet QML qui sonde
  XInput tant que l'écran l'écoute et émet des appuis nommés, la croix se répétant quand on la tient.
- **La vue-modèle du Colisée** (`hmi::ArenaModel`) : le curseur de ciblage (`moveCursor`,
  `centerCursor`, `cycleTarget`), les actions du tour (`turnActions`, `selectAction`,
  `cycleAction`), la confirmation (`confirm`), le chemin jusqu'au curseur (`pathCells`) et la
  prévisualisation en lignes lisibles (`preview`).
- **À l'écran** : le curseur et le chemin dans leur propre calque (`ArenaMark`), la barre d'actions
  numérotée, le panneau de prévisualisation, l'aide des commandes clavier et manette, et l'état de la
  manette.
- **La traduction** : les textes de l'écran du Colisée, que le catalogue anglais n'avait jamais
  extraits depuis le `LOT-50`, et les `tr()` des vues-modèles, désormais extraits (`JadgRuntime` dans
  `qt_add_translations`) — cinquante-huit entrées traduites, aucune en suspens.
- Le ménage de `hmi::GameHud` : la fonction morte et les commentaires qui parlaient encore de
  budgets.

### Ce qui reste hors du lot, nommément

- **Le combat sur la carte d'exploration gelée**, et le HUD de combat (`CombatHudForm`) qui s'y
  superpose : ils attendent que le jeu Qt Quick ait une exploration (`LOT-09`, `LOT-27`). La
  vue-modèle et la prévisualisation de ce lot sont celles qu'ils liront.
- **Les gabarits de zone** à prévisualiser (`core::affectedCells`) : aucun sort ne les emploie
  encore (`LOT-25`, `LOT-35`).
- **Les animations d'attaque** par `AnimationCatalog` : l'arène anime ses figurines par
  `AnimatedSprite` depuis le `LOT-50`, et `AnimationCatalog` n'est pas lu par le QML.
- **Le journal traduit** : ses lignes viennent du `Core`, en français, sans Qt ; les traduire passe
  par les clés du catalogue `.lang`, pas par ce lot.
- **Le remappage** des commandes du combat : les touches et les boutons sont ceux du tableau.

## Conception

### Ce que la feuille de route supposait, et ce que le dépôt contenait

L'état des lieux fait au démarrage a trouvé quatre écarts entre la section de ce lot et le code. Ils
décident de la forme de la livraison, et il vaut mieux les écrire que les laisser redécouvrir.

- **Aucun combat n'existe hors du Colisée dans le jeu Qt Quick.** `hmi::CombatMode` n'est instancié
  que par ses tests ; `GameSession` et `GameViewport`, qui tiennent l'exploration, ne sont compilés
  que dans l'éditeur ; la surface de rendu du jeu (`GameViewportItem`) ne dessine qu'un fond. Il n'y
  a donc **pas de carte d'exploration à geler** sous un combat : le brancher est l'affaire du lot
  qui rendra l'exploration au jeu (`LOT-09`, `LOT-27`).
- **L'écran `CombatHud` n'est ouvert par rien**, et n'a aucune session à lire. Écrire sa vue-modèle
  aurait produit un second écran de combat sans combat.
- **Le jeu Qt Quick ne lisait pas la manette.** Il liait `xinput`, mais le sondage
  (`hmi::GamepadPoller`) n'était compilé que dans l'éditeur.
- **`hmi::GameHud` n'avait plus de budgets de sauts et de dashs** (retirés au `LOT-06`) : seuls une
  fonction morte et deux commentaires en parlaient encore. **`EditorOverlay`** n'est pas un calque
  réemployable mais une couche de rendu de l'éditeur, et la grille du jeu est du QML.

La section du lot le disait elle-même : « l'arène tient la machine dans `core::ArenaSession`, et
c'est ce que cet écran lira ». Le lot livre donc l'IHM de combat **sur l'écran du Colisée**, le
seul combat jouable ; la même vue-modèle et la même prévisualisation serviront le combat sur la
carte quand elle existera.

### Les commandes

| Geste | Clavier | Manette |
|---|---|---|
| Déplacer le curseur | flèches | croix ou stick gauche (répétés tenus) |
| Confirmer : attaquer, se déplacer, l'action choisie ; lancer ; rejouer | Entrée | A |
| Cible suivante, précédente (de la plus proche à la plus lointaine) | Tab, Maj+Tab | X |
| Action suivante, précédente | Page suivante, Page précédente, 1 à 9 | RB, LB |
| Recentrer sur le combattant actif ; nouvelle composition une fois fini | Retour arrière | B |
| Fin du tour | Espace | Y |

Les flèches suivent les **axes de la grille**, pas l'écran : sur la vue isométrique, « haut » monte
vers la droite. Un curseur qui monterait droit à l'écran avancerait en diagonale sur la grille et
ne visiterait qu'une case sur deux.

## Décisions de réalisation

- **Une prévisualisation qui ne ment pas** passe par les fonctions du jet réel, et un test compare
  la prévisualisation au jet que la session jette ensuite — même CA, même abri, même posture, mêmes
  sources — sur une tenaille, puis sur un tir à longue portée, à travers un allié, contre une cible
  qui esquive.
- **Les PV d'un ennemi ne s'affichent plus** : « ensanglanté » sous la moitié, « à terre », et une
  jauge à moitié ou pleine. C'est ce que le *Guide du Maître* laisse voir (`LOT-23`), et l'IA ne lit
  pas davantage : l'écran ne donne pas au joueur ce qu'il refuse à l'adversaire. Les alliés gardent
  leurs PV exacts.
- **Décliner une attaque d'opportunité se choisit avant**, comme on tient une réaction prête, et
  non au milieu du déplacement de l'ennemi : suspendre le tour d'une IA pour poser la question
  aurait fait d'un tour une suite de fenêtres. Le choix est la dernière action de la barre, et
  survit au rejeu.
- **Le curseur a son propre signal.** Chaque geste émettait `changed`, et `changed` reconstruisait
  les 280 cases de la scène : un pas de curseur coûtait un rendu complet, et la croix tenue aurait
  saccadé. Le curseur, le chemin, les actions et la prévisualisation notifient `cursorChanged`, et
  la scène les dessine dans un calque au-dessus du décor. Mesuré en Debug sur la capture de
  l'écran : 10 s pour la composition seule, 18 s pour un combat lancé avec **un** pas de curseur
  avant le correctif, 12,8 s avec **quarante** pas après.
- **Le clic reste** : il pose le curseur et agit, avec l'attaque choisie si elle peut viser la
  cible, sinon la première qui le peut.
- **Les boutons Esquiver, Se désengager et Se précipiter** ont rejoint la barre d'actions : un seul
  endroit pour ce qu'on fait de son action, au clavier, à la manette et à la souris.
- **La composition** (le roster, les camps, la graine) reste un écran de mise en place, à la souris
  ou au clavier par les contrôles Qt (Tab, Espace) ; la manette lance, rejoue et revient à la
  composition.
- **Les vues-modèles ont un exécutable de test**, `RuntimeTests`, dans le `bin` du jeu : elles
  lisent leurs catalogues à côté de l'exécutable, comme le jeu. C'est le premier test d'une
  vue-modèle Qt Quick du dépôt.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔ en automatique pour les gestes (`ArenaModelTest.UnCombatSeJoueParLesSeulsGestes` : un combat du Colisée mené à son issue par cible suivante, curseur, confirmer et fin du tour, l'ennemi joué par l'IA) ; la traduction des touches et des boutons en gestes est dans le jumeau, **à vérifier à la main**.
2. ✔ Le statut d'une attaque est l'entrée même du journal, et le test le vérifie à chaque attaque ; la prévisualisation est le jet (`CombatPreviewTest.LaPrevisualisationEstLeJet`).
3. ✔
4. ✔ pour l'écran : cinquante-huit entrées ajoutées, aucune en suspens ; le journal, venu du `Core`, reste en français (voir plus haut).
5. ⏳ À faire : lancer le jeu, ouvrir le Colisée, composer le personnage contre un loup et un singe, puis jouer un combat au clavier seul, et un autre à la manette seule.
6. ✔

## Bilan

Statut : **livré, en attente de la vérification IHM manuelle**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1145/1145 en Debug, `check_ui_layers`, `check_qml_designer_compat`, les lints et le contrôle des données RPG verts ; captures de l'écran en combat relues.

Alimente [LOT-27](@ref lot-27).

Exigences couvertes : `EX-IHM-003` (l'affichage dit ce dont le joueur a besoin pour décider), `EX-CBT-020` (les cases atteignables et le chemin **montrés avant** que le joueur ne s'engage). Aucune exigence ajoutée.
