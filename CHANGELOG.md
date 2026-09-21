# Changelog

Toutes les évolutions notables du projet sont consignées ici.
Format inspiré de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/) ;
le projet suit le [versionnage sémantique](https://semver.org/lang/fr/).

## [Non publié]

- **Tampons et préfabriqués dans l'éditeur (LOT-EDITOR-08).** Une sélection se copie désormais
  **entière** : les types de chaque couche, les pièces qui y sont ancrées (prises entières, le
  rectangle s'agrandissant jusqu'à leur emprise), les entités et les cases de collision forcées.
  Coller pose le tampon au curseur en **un pas d'annulation**, chaque entité recevant un
  identifiant neuf ; `Ctrl+Maj+V` pose son reflet, pièces jumelles comprises. Un tampon
  s'enregistre comme **préfabriqué** du lieu (`Editor/Prefabs/<lieu>/`), que la palette montre dans
  un onglet avec une vignette générée de son propre contenu, et que `LevelEditor --list-prefabs` et
  `--save-prefab` servent sans fenêtre. Une carte neuve part enfin d'un **modèle** —
  intérieur, rue, arène —, qui donne ses couches, sa taille et son entrée sans nommer aucune pièce.
  `--check` nomme tout fichier de la bibliothèque qu'il ne sait pas relire.

- **Un minidump n'échoue plus sur la pile d'un autre thread.** Le test
  `CrashDumpTest.EcritUnMinidumpAvecLeContexteDUneException` échouait par intermittence sur les
  runners de CI (`ERROR_PARTIAL_COPY`), jamais sur le poste. Le rappel posé en `#78` ne pouvait pas
  l'éviter : la documentation de `MINIDUMP_CALLBACK_TYPE` dit qu'un échec de lecture *dans une pile*
  est tenu pour irrécupérable et n'appelle aucun rappel. `writeMiniDump` tente donc, en dernier
  recours, un dump restreint au seul thread du plantage : les piles des autres threads ne sont plus
  lues, donc plus une cause d'échec. Cette dernière tentative est de plus réessayée : sur les
  runners, la même version donne une exécution verte et une rouge, l'échec tenant à un état que
  `dbghelp` lit au mauvais moment. Le relevé des erreurs de chaque tentative
  (`hmi::lastMiniDumpAttemptErrors`) est affiché par le test, pour qu'une prochaine panne se lise
  dans le journal de la CI — il a déjà servi : `ERROR_INVALID_USER_BUFFER` sous OpenCppCoverage,
  là où `ctest` passait.

- **Le standard 2D HD devient normatif (LOT-101).** Le style qui remplace le pixel art est chiffré
  et éprouvé : une maquette de huit cases sur huit d'Arenarea, montée par `scripts/build_hd_mockup.py`
  depuis la planche de référence et cadrée à 1080p et à 2160p, sert désormais de référence de
  non-régression au rendu HD. Trois exigences sont réécrites — `EX-VIS-008` (la scène peinte,
  losange de 256 × 159, figurine de 170 px, alpha continu, échelle de l'art donnée par le lieu),
  `EX-VIS-009` (la frontière scène / interface, qui sépare désormais deux échelles et non deux
  factures) et `EX-REN-013` (zoom libre, une case valant la hauteur de la fenêtre divisée par 10,8,
  soit la même étendue de monde à toute définition). La consigne du générateur est réécrite en trois
  blocs (`Planning/standards/consigne-2d-hd.md`), et sait commander une planche d'animation. Le
  standard de la scène est complet ; le nombre d'images par animation se fixe au `LOT-112`, sur la
  première figurine, parce qu'une cadence se juge à côté d'une ancre et d'un sol, pas sur une place
  vide.

- **Planification par versions (`Planning/`, LOT-100).** Le jeu quitte le pixel art pour la 2D HD,
  le référentiel `0.1.0` devient l'Empire central seul et la `0.0.1` une démo basique : trois
  quartiers de la Capitale et une quête. Le nouveau dossier `Planning/` porte la trajectoire
  jusqu'à la `1.0.0`, 101 fiches de lots pour les versions `0.0.1` à `0.3.0` (livrables, critères,
  maquettes), les référentiels de contenu tirés du corpus, le standard 2D HD, l'arborescence des
  assets et l'audit du passage à la HD. Un lint (`Planning/outils/lint_planning.py`) le garde en
  CI, et un site engendré depuis le dossier est publié sous `/planning/`. L'ancienne feuille de
  route est figée.

- **`scripts/` de nouveau versionné.** Le dossier avait été supprimé (#92) alors que la CI, les
  hooks pre-commit et les workflows de release en dépendent ; il est restauré et retiré du
  `.gitignore`. `Tools/AssetFactory` et `editor-captures` restent supprimés.

- **Alertes Code scanning (clang-tidy) corrigées.** Une centaine d’alertes de l’analyse de
  `main` : champs de structures initialisés par défaut, fonctions trop denses découpées
  (`BattleGrid`, `deriveCollision`, `MapRefactor`, `MainWindow`, `EditorViewport`…), méthodes
  rendues statiques, `const_cast` confiné à un seul point, tableaux C remplacés par `std::array`.
  Aucun changement de comportement.

- **Nouvelle interface du Colisée : Arena of Brave.** Le parvis, la piste et les deux
  vestiaires remplacent l’ancienne carte en exploration et servent de décor au combat.
  Le kit de 366 pièces conserve ses ancrages, sa profondeur et sa projection dans le jeu
  et l’éditeur ; les portails relient Arenarea, la piste et les deux camps.
  Guide : [Arena of Brave](Documentation/arena-of-brave-map.md).

- **Renommer et remplacer (LOT-EDITOR-14).**
  - **Un renommage suit tout ce qui cite** : renommer une carte — dossier compris,
    `coliseum` → `capital/coliseum` — récrit les portails, les variantes, les quartiers et portes
    gardées des villes, la clé de son nom dans chaque catalogue, et déplace ses notes ; un point
    d'arrivée ou un identifiant d'entité, de même. Rien n'est écrit si le renommage est impossible,
    et la fenêtre montre d'abord ce qui sera récrit.
  - **« Qui cite ceci ? »** pour une carte, une entité, un point d'arrivée ou une pièce ; un
    double-clic mène à la citation.
  - **Remplacer une pièce** par une autre, sur la carte ouverte (un `Ctrl+Z` le défait) ou sur
    toutes les cartes qui la posent.
  - **Changer une carte de planche sans la repeindre** : chaque pièce va à son homonyme, une table
    de correspondance donne les autres, la collision se redéduit. La planche d'Arenarea se
    commande à l'atelier des textures ; la carte y passera d'une commande.
  - Les mêmes opérations sans fenêtre : `--who-cites`, `--rename-map`, `--rename-arrival`,
    `--rename-id`, `--replace-piece`, `--change-scene`.
- **Contrôle du contenu (LOT-EDITOR-07).**
  - **`--check` dit si une carte se joue**, sur toutes les cartes, et la CI échoue sinon :
    références des entités et drapeaux de monde, rencontres qui ne tiennent pas, PNJ, coffre,
    portail ou zone hors d'atteinte depuis l'entrée et les points d'arrivée ; il avertit d'un
    portail sans retour ou d'un point d'arrivée que rien ne nomme. Une variante se contrôle sur les
    cases de sa base.
  - **Panneau « Problems »** : les constats de toutes les cartes, au lancement et à chaque
    enregistrement ; un double-clic ouvre la carte, sélectionne l'entité et cerne la case.
  - **Le nom d'une carte est une clé de traduction** (`map.coliseum.name`), que le jeu traduit :
    « The Coliseum » en anglais. Créer, renommer ou dupliquer une carte complète les catalogues.
- **Les cartes quittent leurs scripts (LOT-EDITOR-06)** — premier jalon de l'éditeur.
  - **L'éditeur fait foi** : le Colisée, Martpart et Arenarea s'ouvrent et s'enregistrent dans
    l'éditeur sans changer d'un octet ; les scripts qui les posaient restent comme trace et
    refusent d'écrire dans les cartes du jeu.
  - **Une retouche par carte**, rejouée par `--apply` : la porte sud et la loge du Colisée ne se
    chevauchent plus (`--check` sans avertissement), le marché de Martpart prend trois devantures
    et un grand étal, un angle de mur égaré quitte le parvis d'Arenarea.
  - **La fenêtre édite enfin le dépôt** : elle ouvre `Source/Elements`, et non la copie que la
    construction refait (et écrasait) à côté de l'exécutable ; le titre montre le dossier ouvert.
  - **Une nouvelle carte choisit son lieu** : nom, taille et planche ; elle naît avec son sol et
    son relief, et passe le contrôle telle quelle.
  - **Guide d'usage** : faire une carte de bout en bout, dans l'éditeur.
- **L'éditeur sans fenêtre (LOT-EDITOR-13).**
  - **`LevelEditor --apply gestes.json`** rejoue sur une carte ce que ferait la main — outil, appui,
    glisser, pièce ou couche armée — par les fonctions mêmes que le canevas appelle ; un geste
    refusé est nommé, et la carte n'est pas touchée. Les onze outils s'y rejouent.
  - **`LevelEditor --render`** rend une carte en PNG, en isométrie, sans ouvrir de fenêtre ; la CI
    publie le rendu de chaque carte qu'une PR ajoute ou change (artefact `map-renders`).
  - **Un scénario par outil**, comparé à un fichier attendu, tient lieu de test d'IHM ; une rue de
    Martpart gommée puis retracée par `--apply` redonne la carte livrée octet pour octet.
- **Entités et zones sur le canevas (LOT-EDITOR-05).**
  - **Les zones se tirent à la souris** : une zone de combat, un îlot ou une zone de règles se
    tire entre deux coins et se redimensionne par huit poignées ; pendant qu'on tire la zone de
    combat du Colisée, son verdict tactique suit (cases libres, entrées d'arène dedans et dehors).
  - **L'outil Forme** (`Z`) peint une zone de règles case par case et trace un trajet de PNJ point
    par point ; la famille « trajet » entre dans la table, prête pour les rondes.
  - **Les entités se voient** : figurine à la place du marqueur, étiquette (la carte cible d'un
    portail, le nom d'une zone), formation d'une rencontre par ses figurines.
  - **Sélection multiple et déplacement en groupe** (`Maj` + clic), liste filtrable avec
    identifiants ; l'inspecteur borne les entiers et propose figurines, drapeaux, lieux, objets et
    `carte#id`.
  - **Un test bloque toute famille d'entité lue par le jeu et inconnue de l'éditeur** : il a
    trouvé la zone de règles, que l'éditeur ne savait pas poser.

- **Les outils du peintre (LOT-EDITOR-04).**
  - **Ligne, seau, gomme et pipette**, chacun à sa touche ; `Alt` + clic prend la pièce qu'on
    voit depuis n'importe quel outil, et la palette la montre.
  - **Un geste se défait d'un coup** : un trait de vingt cases, une ligne ou un seau, c'est un seul
    `Ctrl+Z` (il en fallait un par case).
  - **Le miroir** (`M`) reflète chaque geste de l'autre côté d'un axe vertical de l'écran et pose la
    jumelle des pièces : une ligne de `wall-right` trace aussi la façade en `wall-left`. Une maison
    de Martpart se trace en cinq gestes.
  - **La mesure** dit cases et pieds ; **les notes d'auteur** s'épinglent aux cases, dans
    `<carte>.editor.json` ; **l'essai** part de la case survolée (`Shift+P`).

- **L'éditeur pose les pièces du lieu (LOT-EDITOR-03).**
  - **La palette est la planche du lieu** : vignettes groupées par classe, recherche, et à part les
    pièces que la carte cite sans que la planche les ait, en damier. Les types de tuile restent dans
    leur onglet, repli d'une carte sans lieu.
  - **Une pièce se pose en un geste** sur sa couche, avec le type de sa case et la collision de son
    emprise ; un étal 2 × 1 occupe ses deux cases, et la gomme le retire entier.
  - **La collision suit chaque geste**, sur ses seules cases. Peindre la collision force la case,
    la gomme la libère, et un masque magenta montre les cases forcées.
  - **Le Colisée n'a plus de case forcée** : on ne sort plus par la porte dans le vide qui entoure
    l'amphithéâtre, et les deux piliers du couloir ouest arrêtent le pas comme ceux de l'est.

- **Le format de carte v4, gardé en CI (LOT-EDITOR-12).**
  - **Chaque case de couche nomme sa pièce** ; la pièce ne vit plus sur la grille de collision. Les
    trois cartes (Colisée, Martpart, Arenarea) ont été migrées et se jouent à l'identique.
  - **La collision se déduit des pièces** (type tactique déclaré par le manifeste du lieu), hors
    des cases que l'auteur force à la main ; une pièce large occupe et trie toute son emprise.
  - **Identifiants d'entité** jamais réemployés, **zones peintes**, **variantes** de carte, et une
    place réservée à la hauteur.
  - `LevelEditor --migrate` convertit une carte ; `LevelEditor --check` contrôle toutes les cartes
    en CI. Schéma publié : `Documentation/Editeur/level.schema.json`.

- **Un minidump s'écrit même quand une zone mémoire est illisible.** Si une zone ne peut pas
  être lue pendant le dump (la pile d'un thread qui se termine, par exemple), elle est sautée au
  lieu de faire échouer tout le fichier (`ERROR_PARTIAL_COPY`). Vu sur la Nightly du 19 septembre.

- **Fabrique d’assets, premier essai PNJ (LOT-CREATION-ASSETS, hors roadmap).**
  Préparation A+B+C, mémoire par asset, réception, reprises ciblées, contrôles bloquants,
  comparaisons et journaux en ligne de commande, avec génération dans le chat local.
  39 tours sur les cinq PNJ du pilote ; aucun remplacement des assets livrés : échelle,
  palette et animation restent insuffisantes pour valider le pilote. Bilan dans
  `Tools/AssetFactory/bilan-poc.md`.

- **L'éditeur montre le lieu tel qu'on le jouera (LOT-EDITOR-02).**
  - **Vue iso par défaut** : le canevas peint les pièces des planches comme le jeu, même liste,
    même ordre ; son image égale celle du jeu à 0,06 % des pixels près. `F9` bascule vers la vue à
    plat, qui lit types et collision.
  - **Pointage juste** : on désigne la case par son losange, y compris derrière un mur haut ; la
    barre d'état donne la case survolée et ses pièces.
  - **Calques** : une couche peut être grisée ou verrouillée ; `F8` passe les reliefs en
    transparence ; une mini-carte (« Overview ») montre toute la carte et ramène la vue d'un clic.
  - **L'essai immédiat** est peint de la même façon : l'éditeur ne passe plus par le GPU.

- **L'éditeur de cartes devient un module à part, et ne perd plus de travail (LOT-EDITOR-01).**
  - **Un module** : le code quitte `Source/HMI` pour `Source/Editor` (`Logic/`, bibliothèque
    `EditorLogic` ; `Ui/`, l'exécutable `LevelEditor`) ; le jeu n'en dépend plus.
  - **Un outil interne** : style Fusion de Qt (clair ou sombre selon le système), textes en
    anglais, fenêtre et panneaux construits en code. Retirés : la charte de l'éditeur (jetons,
    feuille de style, icônes tracées, menu Thème, police Inter), ses formulaires `.ui` et ses
    189 clés de traduction.
  - **Reprise après plantage** : un brouillon modifié est sauvegardé deux secondes après le dernier
    geste, et proposé à la reprise au démarrage suivant (`%LOCALAPPDATA%\JustAnotherRpgGame\Editor`).
  - **Une carte changée sur disque n'est plus écrasée** : l'éditeur la relit si rien n'est modifié,
    sinon il demande de relire ou de garder, et met l'autre version de côté.
  - **« Modified » suit le contenu** : défaire jusqu'à l'état enregistré rend une carte non
    modifiée ; fermer avec des modifications demande quoi en faire ; l'historique d'annulation est
    plafonné à 200 pas.
  - **Corrigé** : déplacer la fenêtre vers un autre écran pouvait remplacer le brouillon en cours
    par le Colisée relu sur disque.

- **Les premiers monstres : le lion, le loup et le soldat Ironhand (LOT-93).** L'atelier des
  monstres dessine une créature depuis le texte seul de son bloc, au style des PNJ, et les trois
  dont la version `0.0.1` a besoin sont livrés, animés, dans la galerie des assets (famille
  « Monstres »).
  - **Deux gabarits** : Moyen (une case, 48 × 64) et Grand (2 × 2 cases, 96 × 96) ; le lion est le
    premier Grand.
  - **Pas de sort, pas de `cast`** : une créature qui n'en lance pas n'en livre pas, et
    `check_asset_keys.py` l'accepte ; il refuse une figurine hors catalogue, une bande mal
    dimensionnée ou un `cast` livré sans être déclaré.
  - **Les sentinelles Ironhand** des portes de Martpart et d'Arenarea portent la figurine du
    soldat : une carte nomme une figurine de l'atelier des monstres par son dossier
    (`"figure": "Monsters/ironhand-soldier"`), et le marqueur qui les dessinait s'efface.
  - Le lion et les loups ne paraissent pas encore au Colisée : c'est le `LOT-27`.

- **L'éditeur de cartes a sa feuille de route (`Documentation/Editeur/feuille-de-route.md`).**
  Quatorze lots `LOT-EDITOR`, une piste à part de celle du jeu : édition en iso avec les pièces du
  lieu, format de carte version 4, pilotage sans fenêtre, fin des cartes écrites par script. Aucun
  code ne change encore.

- **Correctif : l'éditeur plantait au démarrage.** Depuis le passage des panneaux de l'éditeur
  en `.ui` (#71), la palette lisait son arbre avant de construire sa mise en page : un pointeur
  nul, et `LevelEditor` s'arrêtait sur une violation d'accès avant d'ouvrir sa fenêtre.

- **Les écrans du jeu portent leurs images peintes : les 213 pièces de la charte v2 (LOT-87).**
  Cadres, plaques, boutons, onglets, contrôles, médaillons, emplacements, jauges, ornements et
  icônes remplacent les aplats de repli sur les dix-neuf écrans, sans qu'un formulaire change.
  - **Provenance** : chaque image garde le prompt réellement envoyé au générateur ; planches et
    inventaire de la livraison dans la documentation du lot.
  - **Corrigés** : la mini-carte du HUD n'affiche plus de silhouette de portrait, et un titre de
    plaque se réduit plutôt que de déborder de ses ornements.
  - **Fiche de personnage** : les six caractéristiques et les statistiques de la compagnie portent
    leur icône ; abréviations et modificateurs passent sous le médaillon, hors des ornements qui
    les masquaient. La signature s'écrit à la plume (*Pinyon Script*, SIL OFL 1.1), au-dessus du
    paraphe.
  - **Pause et compagnie** : les entrées de la pause sont centrées dans leur panneau ; le fond des
    portraits suit l'ouverture de chaque cadre et ne déborde plus de l'anneau.

- **Retrait de l'héritage : le dépôt ne garde plus rien du jeu d'origine (LOT-88).**
  - **L'essai de l'éditeur joue le jeu** : **P** lance l'exploration du jeu sur le brouillon —
    marcher, interagir, franchir un portail vers une autre carte —, et **Échap** revient à
    l'édition. `hmi::WorldPlay` est la mise en scène partagée par le jeu et l'éditeur.
  - **Retirés** : l'ancien runtime que l'essai faisait tourner (physique de plateforme,
    mécanismes, tuile de sortie, caméra de suivi et zones de caméra, plans et parallaxe, ombres,
    skins et raccords, bruitages), et l'habillage de l'éditeur qui les servait — panneaux Liens,
    Textures, Plans, atelier pixel art (le `LOT-69` est absorbé), bibliothèque d'assets, espaces
    de travail, remappage.
  - **Le format de carte ne porte plus que le RPG** : douze types de tuiles, une entrée exigée ;
    les trois cartes livrées perdent leur sortie et leur cadrage.
  - **Assets et scripts** de l'ancien jeu supprimés ; l'atlas de tuiles est procédural.
  - **Polices pixel** (`Pixelify Sans`, `Press Start 2P`) supprimées, crédits compris ; les tags
    du jeu d'origine (`v0.0.1` à `v0.1.3`) sont retirés du dépôt.
  - **Spécifications** : 120 exigences retirées, ancres conservées ; `ia.md` et `decors.md`
    partent pour `exigences-retirees.md` ; `Documentation/Heritage/` et la notation `LOT-H-NN`
    sont supprimées. Une exigence retirée ne peut plus être citée par le code
    (`lint_exigences.py`). Guides, manuel et README décrivent le jeu tel qu'il est.

- **Martpart et Arenarea se parcourent : le graphe des quartiers de la Capitale (LOT-96).**
  « Nouvelle partie » ouvre le jeu à la **porte de l'Est de Martpart**, le quartier du marché ;
  on passe à **Arenarea** par l'avenue et l'on revient au point d'arrivée nommé.
  - **Deux quartiers tracés depuis le plan de la ville** peint par l'auteur : un script d'atelier
    pose chaque porte sur le bord que coupe la direction du quartier voisin, puis les rues, la
    place, les îlots et les ruelles ; les cartes se retouchent dans l'éditeur. Martpart porte sa
    planche (`LOT-92`) ; Arenarea l'emprunte, faute de planche propre.
  - **La ville a son graphe** (`World/cities/capital.json`) : ses douze quartiers, chacun avec sa
    carte ou la porte gardée qui le ferme, et la porte de départ. `check_rpg_data.py` relie chaque
    quartier à sa fiche d'atlas, à son point du plan et à sa carte.
  - **Dix portes gardées** par une sentinelle de l'Armée Ironhand, qui refuse le passage. Sa
    figurine n'existe pas encore : une figurine sans image se dessine désormais par son
    **marqueur** (`LOT-39`) plutôt que par le damier.
  - **L'écran « Carte » descend au quartier, puis à l'îlot** : le plan montre le quartier du héros
    et ceux qu'il a parcourus. Le quartier agrandit le plan de la ville (*provisoire*, en attendant
    sa carte peinte) ; l'îlot est la carte du quartier telle que le jeu la dessine, rendue hors
    écran.
  - Les cartes d'un sous-dossier (`capital/martpart`) entrent au graphe du monde et au
    navigateur de l'éditeur sous leur chemin relatif ; `--map=<carte>[@<arrivée>]` ouvre une carte
    dans un build de développement.

- **Analyse statique : les 27 alertes restantes de Code scanning corrigées.** Les fichiers
  arrivés avec `LOT-09`/`LOT-94` (exploration, zones de combat, carte du monde) et une alerte
  plus ancienne d'`ArenaModel` étaient hors du périmètre des corrections précédentes.
  Initialiseurs désignés complets, tableau C remplacé par `std::array`, parenthèses explicites
  sur des calculs mélangeant les opérateurs, `return {...}` pour les constructions déjà typées,
  concaténation de chaîne sans copies intermédiaires, un usage après déplacement détecté par
  l'analyseur corrigé, deux branches identiques fusionnées, un paramètre pris par référence, et
  `ArenaModel::loadCatalogs` découpée pour réduire sa complexité cognitive.
- **Le Colisée se parcourt : l'exploration dans le jeu, et la première carte (LOT-09).**
  « Nouvelle partie » ouvre le **Colisée en version finale** à sa porte : 40 × 34 cases, dont le
  sable du `LOT-50` (20 × 14) au centre comme **zone de combat déclarée**. On parcourt au clavier
  le hall, les couloirs sous les gradins, les deux vestiaires, les tribunes et la loge impériale ;
  on parle aux PNJ ; le héraut envoie sur le sable, et l'on en revient sur la carte, au même
  endroit.
  - **Un seul moteur de rendu** : le lieu se dessine comme l'arène — même pipeline QRhi, même
    projection isométrique, mêmes planches de l'atelier des textures (`LOT-92`). `WorldViewport`
    est le jumeau d'`ArenaViewport` ; la caméra **suit** le héros, à un agrandissement entier, et
    ne sort pas de la carte. *Décision de l'auteur* : plutôt que de compiler dans le jeu la session
    de l'éditeur (qui aurait amené un second rendu, en tuiles carrées), une **session
    d'exploration** neuve vit dans `Core`, jumelle de la session d'arène.
  - **Le graphe du monde se joue** : traversée d'un portail par **point d'arrivée nommé**, jamais
    par des coordonnées ; une carte visitée n'est pas rechargée (le lieu reste ce qu'on a quitté) ;
    un portail peut exiger un drapeau de monde. Un portail orphelin, une zone de combat qui déborde
    ou qu'aucune case ne laisse libre sont refusés **au chargement**, avec un code exploitable.
  - **Ce qu'une case porte** (*décision de l'auteur*) : le sol vient du type de tuile de la couche
    « sol », traduit par la table du lieu (`Assets/Scene/<lieu>/appearance.json`) ; le relief nomme
    sa pièce **à la case**, par l'assignation de texture que l'éditeur pose déjà. Aucun changement
    du format de niveau.
  - **Parler à qui l'on regarde** : l'écran de dialogue n'a plus d'identifiant écrit en dur — la
    carte nomme le dialogue du PNJ visé. Un dialogue peut envoyer se battre (nouvelle action
    `startCombat`), et le héraut du Colisée perd sa marque « provisoire ».
  - **Le contenu provisoire s'en va** : le niveau nu de l'arène (`arena-of-the-future.json`), les
    six fonds de test et leur script, la rencontre de démonstration — remplacée par les fauves du
    Colisée. Restent le personnage de démonstration et la planche source du `LOT-50`, pour les
    raisons écrites dans l'epic du lot.
  - **Vérifié** : 1 297 tests, dont le parcours de cinq cartes aller-retour, la carte livrée
    (chaque dialogue, chaque figurine et chaque pièce qu'elle nomme existent ; ses six lieux
    s'atteignent depuis la porte) et le rendu hors écran du Colisée sur un vrai `QRhi`.

- **Les images du corpus quittent le dépôt, et l'écran « Carte » revient sur les cartes de l'auteur
  (LOT-94).** Les deux cartes du monde extraites du corpus étaient les seules images du corpus
  commises : elles partent, avec l'écran qui les affichait et la commande
  `sourcebook illustrations` qui les extrayait. L'écran « Carte » revient aussitôt, refondu, sur
  **seize cartes peintes par l'auteur**, sans lettrage : le monde, les treize régions de l'atlas,
  les plans de la Capitale impériale et de Fisherman's Wharf.
  - **Trois niveaux** : la vue d'ensemble, une région, le plan d'une ville. Les mêmes commandes aux
    trois, au clavier, à la manette et à la souris — flèches pour le repère voisin, Tab ou LB/RB
    pour le lieu suivant de la liste, Entrée, clic ou A pour ouvrir, Retour arrière, Échap, clic
    droit ou B pour remonter, +/−, molette ou X/Y pour agrandir, glisser pour déplacer la carte.
    On ne s'y déplace pas : la carte sert à s'orienter. Le bouton « Carte » du cadre de jeu est
    rallumé, et les écrans du RPG sont de nouveau neuf. `--map-region=<id>` et `--map-city=<id>`
    ouvrent un niveau directement.
  - **Les lieux placés** : 61 lieux de l'atlas sur les cartes de région, les douze quartiers de la
    Capitale et les douze sites numérotés de Fisherman's Wharf sur leur plan, 103 noms de
    géographie ; 19 entrées de l'atlas qui ne sont pas des lieux sont écartées nommément. Les noms
    sont posés par le jeu, jamais peints. Un lieu que le livre ne situe pas, ou qui sort du cadre
    peint, reste dans la liste de sa région sans repère : le jeu n'invente pas de position — les
    îles de la Tempête n'en ont ainsi aucun. Ctrl+clic journalise la fraction sous le pointeur,
    pour retoucher le relevé.
  - **Données et outil** : `Source/Elements/Assets/Maps/` (seize JPEG, 15,9 Mo, et leur
    manifeste de provenance `author`), `Source/Elements/Maps/world-maps.json` (les positions, à
    part de l'atlas) ; `scripts/check_map_assets.py` les recoupe en intégration continue avec
    l'atlas.
  - **Le fond du menu principal est produit** (`ui/background/menu-scene`) ; les captures de
    référence du menu et des crédits sont régénérées.
  - **Spécification** : une illustration d'interface est produite, jamais extraite
    (`EX-IHM-076`, refondue — les cartes de l'auteur ont leur propre manifeste) ;
    `check_ui_assets.py` refuse toute provenance autre que `produced` ; l'écran à trois niveaux
    (`EX-IHM-106`) ; aucun nom peint, des positions tenues à part de l'atlas et jamais inventées
    (`EX-IHM-107`).
  - **Feuille de route** : le `LOT-94` est livré et absorbe le `LOT-95` (le plan de la Capitale),
    retiré par fusion ; `capital.json`, les niveaux quartier et îlot du plan et la position du
    héros passent au `LOT-96` ; le `LOT-42` bâtit sur l'écran livré et n'a plus de carte du monde à
    produire.

- **Analyse statique : les 113 alertes restantes de Code scanning corrigées.** L'analyse de main
  en relevait encore dans l'éditeur et la galerie des assets, arrivés hors du périmètre de la
  correction précédente. Réécritures mécaniques, quatre fonctions trop complexes découpées,
  transtypages vérifiés (`qobject_cast`, `dynamic_cast`), un `std::visit` que l'analyseur ne
  suivait pas remplacé, et une boucle sur un temporaire rendue explicite. Deux `NOLINT` justifiés.
- **Analyse statique : les 896 alertes clang-tidy de Code scanning corrigées.** Sur les cent
  fichiers signalés, les réécritures mécaniques sont appliquées, les fonctions trop complexes
  découpées en étapes nommées, et quelques défauts réels corrigés : conversions élargissantes
  après une multiplication, arrondi manuel, compteurs de boucle flottants, déréférencement
  possible d'un dock nul dans l'éditeur. Neuf `NOLINT` restent, chacun justifié en ligne.
- **Galerie des assets, un outil de débug.** `--screen=AssetGallery` (ou le sélecteur d'écrans en
  build de développement) montre tous les assets livrés d'un coup : PNJ, héros et gladiateurs du
  Colisée, pièces de la planche, textures des scènes, skins animés. Chaque forme occupe un bloc —
  son emprise plus une case de marge — et joue son animation ; l'inspecteur donne taille, images,
  durée, boucle, emprise et ancre, et avance image par image. On s'y déplace au clic maintenu, on
  zoome à la molette, une minicarte situe la vue. Seuls les blocs à l'écran sont dessinés, ceux
  d'un anneau autour gardent leur texture, les autres la libèrent après deux secondes. Ce n'est
  pas un écran du jeu, et il n'appartient à aucun lot de la feuille de route.
  - **Spécification** : tout asset livré doit y paraître (`EX-CNT-042`). Un test parcourt toutes
    les images de `Assets/` et échoue sur celle que la galerie ne montre pas, hors exclusions
    nommées (planches sources, atlas, interface, polices) ; portraits, joueur, objets, fonds et
    skins fixes y entrent donc aussi.

- **Atelier des textures : la scène a son style (LOT-92).** Une maquette approuvée par l'auteur
  fixe le style de la scène — pixel art isométrique, soir, lanternes, pierre et bois sombres —
  et une méthode commande ensuite une planche de textures **par lieu**, depuis sa fiche d'atlas.
  - **La Capitale dans l'atlas** : la Capitale et ses douze quartiers (Martpart, Arenarea…)
    entrent dans l'atlas, qui passe à 107 lieux ; les filigranes de commande sortent des fiches.
  - **L'arène se dessine avec l'atelier** : sable, dalles, murs, arches, torches et bannières du
    Colisée remplacent le décor de la planche du `LOT-50`, orientés et posés par leur ancre.
  - **Martpart** a sa planche, commandée sans rien rédiger pour ce quartier : 22 textures de rue,
    de murs, d'étals et de marchandises.
  - **Outil** : `scripts/extract_texture_sheet.py` prépare l'envoi au générateur, découpe les
    planches reçues (pièces lues dans l'ordre, orientation contrôlée, miroirs, palette commune),
    installe et revérifie (`--check`).
  - **Spécification** : la scène en pixel art et l'interface à la charte v2, avec leur frontière
    (`EX-VIS-008`, `EX-VIS-009`).

- **Feuille de route : le *vertical slice* se joue dans la Capitale.** Les quatre lots du chemin
  critique — `LOT-09`, `LOT-16`, `LOT-17`, `LOT-27` — étaient rédigés sans lieu, sans PNJ, sans
  quête, et ne se vérifiaient qu'en test. Ils sont réécrits autour de la **Capitale du Central
  Empire** (Sourcebook, pages 94 à 103 ; plan `VTT/Map - Capital.jpg`), où se trouve déjà le
  Colisée : l'exploration dans le jeu et le graphe des douze quartiers (`LOT-09`), la quête
  « Les enfants de Martpart » tirée du livre (`LOT-16`), la reprise de partie (`LOT-17`), la
  ville habillée — cartes, figurines, textures — et le combat sur la carte (`LOT-27`). La
  région de départ passe des Freelands à la Capitale (§8) ; un plan d'intégration (§6) dit ce
  qu'on voit dans le jeu à la fin de chaque lot. Le `LOT-09` devient le prochain lot calculé.
- **L'éditeur produit les cartes du RPG (LOT-11).** `LevelEditor` reste l'outil d'auteur — décision
  tranchée, `EX-EDIT-030` refondue — et sait désormais tout ce qu'il faut pour écrire une carte sans
  toucher au JSON.
  - **Trois couches** (panneau « Couches ») : la collision, le sol et le décor, chacun peint à part,
    montré ou masqué, estompé ; la collision se superpose en masque coloré. Ajouter le premier sol
    reprend l'image de la carte, et une couche d'image refuse les tuiles qui portent une règle.
  - **Entités** (outil « Entité », panneau « Entités ») : coffre, panneau, PNJ, rencontre, portail,
    point d'arrivée, entrée d'arène — posés, déplacés, retirés, renseignés dans un formulaire tiré
    de la table des familles. Les dialogues, rencontres, cartes et points d'arrivée se choisissent
    dans les catalogues livrés, et une référence cassée est signalée à sa case.
  - **Portails** : une carte cible et un point d'arrivée nommé, jamais des coordonnées ; l'onglet
    « Graphe » du navigateur de cartes montre le monde qu'ils relient, portails cassés compris.
  - **Terrain tactique** : une rencontre posée dans un couloir, ou dont la formation tombe dans un
    mur, est signalée, et sa zone se voit sur la carte.
  - **Essai immédiat** : les entités posées s'y voient, avec leur marqueur généré, et répondent à la
    touche d'interaction.
  - **Correctif** : la fenêtre de l'éditeur construisait tous ses panneaux deux fois — la barre
    d'outils montrait chaque outil en double.
- **Atelier des PNJ : la preuve de concept (LOT-91).** Cinq figurines animées au style du jeu,
  produites par une méthode reproductible, entrent dans le dépôt — premières des 160 fiches du
  *Character Compendium*.
  - **Assets** (`Source/Elements/Assets/Npc/`) : Anariel, Lizz, Xorius, Nakral et Jade, six bandes
    chacun (`idle`, `walk`, `hit` en 48 × 64 ; `death`, `attack`, `cast` en 96 × 64), leurs
    `.anim.json` et un portrait pixel art, listés par `manifest.json`.
  - **Au Colisée**, quatre héros prennent l'apparence d'un PNJ (champ `replaces` du manifeste) ;
    chaque bande se dessine à sa largeur réelle, centrée au même pied. Sans manifeste, rien ne
    change.
  - **La méthode** (`Documentation/Lot/LOT-91-atelier-pnj/`) : l'epic, le journal du PoC et son
    verdict, et l'atelier versionné (prompts, ancres, références par PNJ, scripts).
- **Fan game non commercial : licence, crédits et nom.** Le projet dit enfin ce qu'il est — un jeu
  gratuit, non officiel, inspiré de *Dungeons & Dragons* et de *Tanares* — et sa licence cesse de
  le contredire.
  - **Licence non commerciale.** La GPL v3 autorisait la vente du jeu ; le code passe sous
    **PolyForm Noncommercial 1.0.0** (`LICENSE`, ~690 en-têtes SPDX) et les contenus originaux sous
    **CC BY-NC-SA 4.0** (`LICENSE-CONTENT`). Le code reste lisible, mais n'est plus *open source* au
    sens de l'OSI. Qt, en lien dynamique sous LGPLv3, n'en est pas affecté.
  - **Univers et ayants droit nommés** (`README.md`, `THIRD-PARTY-NOTICES.md`, écran *Crédits*) :
    Wizards of the Coast, Black Book Éditions, Dragori Games ; non-affiliation ; attribution
    CC BY 4.0 du SRD 5.1. `THIRD-PARTY-NOTICES.md` gagne aussi les polices Cinzel et IM Fell
    English, qui y manquaient.
  - **`JustAnotherDnDGame` devient `JustAnotherRpgGame`** : cible CMake, exécutable, archives de
    release, dossier de réglages Qt (les options et la progression locales repartent de zéro),
    documentation. Le titre affiché et le logotype disent « Just Another RPG Game ». La famille
    d'exigences `EX-DND` devient `EX-REG` (`regles-d20.md`).
  - **Crédits défilants** : la section *Univers et inspirations* rejoint l'écran, dont les colonnes
    défilent (souris, molette, flèches) quand elles dépassent le panneau (`OrnateScrollBar`). Un nom
    de `credits.json` peut désormais être un libellé traduit, pour une mention qui n'est pas un nom
    propre.
- **Nightly ne tourne plus sur les PR.** Une PR qui touchait `Source/Fuzz/`, `Source/Benchmark/` ou le
  workflow lançait toute la nuit (fuzzing, clang-tidy complet, Qt suivant…) ; il ne part plus qu'à
  2 h 17 UTC, ou à la demande (*Run workflow*).

- **CI : Qt, données, vitrine** (refonte de la chaîne d'outillage, phase 4). La filière contenu et
  l'interface ont les mêmes garde-fous que le C++.
  - **Tests Qt Quick** (`QmlTests`) : les 47 fichiers `.ui.qml` de `Jadg.Ui` se construisent sans un
    avertissement, les boutons et cases de la charte v2 déduisent le bon état, et les **15 écrans
    sont comparés à leur capture de référence** (rendu logiciel, identique sur le poste et le
    runner). Une couleur de jeton changée fait échouer 12 écrans sur 15.
  - **Traductions vérifiées** (`scripts/check_translations.py`) : aucune traduction inachevée ou
    disparue, marqueurs `%1` identiques ; `build-ninja` relance `lupdate` pour prouver que le
    catalogue est à jour du code. Deux chaînes qu'il manquait déjà (l'aide souris de l'arène, une
    étiquette de la galerie de l'atelier) traduites, trois entrées mortes retirées.
  - **Minidump sur plantage** : le jeu et l'éditeur écrivent `Crashes/<application>_<version>_<date>.dmp`,
    lisible avec le zip de symboles de la release ; le test de fumée des archives le prouve
    (`--crash-test`). Les archives n'embarquent plus `Logs/` ni `Crashes/` du poste qui les construit.
  - **Scripts Python** : dépendances figées par `uv.lock` (jsonschema quitte `ci.yml`), **pytest**
    (`scripts/tests`) qui rend nommés et comptés les auto-tests et chaque fixture RPG, rapport dans
    la PR avec ceux des builds.
  - **Scripts PowerShell** lus par **PSScriptAnalyzer** (compatibilité Windows PowerShell 5.1
    comprise) ; deux défauts de `setup_dev.ps1` corrigés.
  - **Site qualité** sur gh-pages (`/qualite/`) : couverture de `main` par domaine, rapport détaillé,
    dernières mesures de performance et leurs courbes, à côté de la Doxygen.
  - **Renovate** pour les `GIT_TAG` de FetchContent, **Dependabot** pour `uv.lock`.

- **CI : nuit et profondeur** (refonte de la chaîne d'outillage, phase 3). Les défauts lents à
  trouver se cherchent la nuit, sans allonger une PR.
  - **`nightly.yml`**, non bloquant : clang-tidy sur tout `Source/` (tendance par famille), tests
    en ordre aléatoire répété à graine affichée, MSVC `/analyze` et cppcheck en SARIF, build contre
    la version de Qt suivante, liens internes de la documentation (lychee, hors ligne).
  - **Fuzzing des lecteurs de données** (`Source/Fuzz`, `-DBUILD_FUZZERS=ON`, libFuzzer de MSVC) :
    l'enveloppe JSON, les niveaux, les dialogues, l'habillage et les traductions, sous
    AddressSanitizer, avec un corpus qui grandit d'une nuit à l'autre.
  - **Mesures de performance** (`Source/Benchmark`, Google Benchmark) : déplacement, ligne de vue,
    abri, tour d'IA, chargement de niveau ; historique dans la branche `benchmarks`, alerte au-delà
    de 150 %.
  - **CodeQL** (`codeql.yml`) sur chaque PR : C++ sans build, Python et workflows.
  - **Archives lancées avant publication** (`scripts/smoke_test_release.ps1`) et **attestation de
    provenance** de chaque fichier publié (`gh attestation verify`).
  - Six liens relatifs cassés de la documentation réparés, trouvés par le nouveau contrôle.
  - **Premier défaut trouvé par le fuzzing** : un nombre JSON hors de portée (`1e400`) faisait lever
    `readJsonObject` au lieu de rendre un échec décrit — n'importe quel catalogue ainsi écrit
    arrêtait le jeu. Corrigé, avec son test.

- **CI : parité du poste** (refonte de la chaîne d'outillage, phase 2). La CI confirme ce que le
  poste a déjà vérifié, elle ne le découvre plus.
  - **Hooks avant chaque commit** (`.pre-commit-config.yaml`) : clang-format, ruff, actionlint,
    zizmor, gitleaks, conflits de fusion et de casse, YAML, JSON (invalide, clé en double, BOM),
    garde-fou binaires, et format du message de commit. Rejoués sur tout le dépôt par le job
    `pre-commit` de la CI.
  - **`scripts/setup_dev.ps1`** compare les outils du poste aux versions de `ci.yml`, qu'il lit
    sans en recopier aucune, et installe ce qui diverge avec `-Install`.
  - **`scripts/check.py`** rejoue en une commande les contrôles du job `lint-exigences`, lus dans
    `ci.yml`, puis les hooks.
  - **Cache de compilation sccache** sur les presets Ninja (jobs Ninja, ASan et clang-tidy de la CI,
    et poste où `sccache` est dans le PATH) : un build refait à cache plein passe de 487 s à 247 s.
    **Inactif avec un MSVC en français** : sccache y réécrit la sortie `/showIncludes`, et Ninja
    perdrait des dépendances d'en-têtes ; CMake le détecte et s'en passe.
  - **Garde-fou binaires** : aucun fichier au-delà de 5 Mio, et un binaire doit appartenir à une
    famille déclarée `binary` dans `.gitattributes`.
  - **Versions croisées vérifiées** (`scripts/check_tool_pins.py`) : clang-format dans `ci.yml` et
    dans les hooks, Doxygen dans `ci.yml` et `docs.yml`.
  - **`.clangd`** pour les diagnostics clang-tidy dans l'éditeur ; **`build.ps1 -Label`** pour ne
    lancer qu'un étage de tests.
  - Écartés, avec leur raison dans `.pre-commit-config.yaml` : `qmlformat` (Qt 6.11 dé-indente les
    blocs de documentation QML), `ruff format` et le reformatage des JSON écrits à la main.

- **CI : voir dans la PR** (refonte de la chaîne d'outillage, phase 1). Savoir ce qui a cassé sans
  ouvrir un log.
  - **Résultats des tests** des builds Debug, Release et Ninja publiés en commentaire de PR et en
    check run : échecs avec leur message, tests ajoutés ou retirés.
  - **Couverture des lignes ajoutées** commentée par Codecov, à titre informatif : le cliquet
    bloquant reste celui de `ci.yml`.
  - **Annotations sur la ligne** : avertissements MSVC, assertions GoogleTest en échec, écarts
    `clang-format`, et tous les diagnostics `clang-tidy` — bloquants ou non — convertis en SARIF
    (`scripts/clang_tidy_sarif.py`) et visibles dans *Code scanning*.
  - **Un résumé en tête de chaque job** : nombre de tests et les plus lents, couverture, taille des
    exécutables, diagnostics `clang-tidy` par famille, verdict de chaque contrôle du référentiel.
    Ces contrôles s'exécutent désormais tous, même après un premier échec.
  - **CHANGELOG vérifié en PR** (`changelog.yml`, `scripts/check_changelog.py`) : une ligne ajoutée
    à cette section, ou le label `no-changelog`.

- **CI : plus vite, plus propre** (refonte de la chaîne d'outillage, phase 0).
  - **Compilation parallèle sous Visual Studio** (`/MP`) : MSBuild compilait les fichiers d'un
    projet un par un, et les jobs `vs` prenaient deux fois le temps du job Ninja.
  - **Un run obsolète est annulé** au push suivant sur la même PR ; chaque job a un **délai
    maximal** au lieu des six heures par défaut ; le jeton est **en lecture seule** sauf dans les
    jobs qui publient, et aucun checkout ne le conserve.
  - **L'installation de Qt est écrite une fois** (`.github/actions/setup-qt`) au lieu de six.
  - **Tous les workflows se relancent à la main** (`workflow_dispatch`).
  - **Actions épinglées par SHA**, mises à jour par **Dependabot** une fois par semaine.
  - **Release** : une version n'est publiée que depuis un tag **sur `main`** dont les tests passent
    en Debug et en Release ; `debug-latest` n'est plus supprimée puis recréée, elle est mise à jour
    sur place ; chaque release porte ses **symboles `.pdb`** (Release compris, jusqu'ici sans) dans
    une archive à part, et un fichier **`SHA256SUMS`**.

- **Colisée : deux bugs vus en jouant.**
  - **L'IA ne fuit plus le combat** (`LOT-23`). Trois causes dans la comparaison des candidats
    (`core::planTurn`) : la clé anti-suicide comptait les tireurs sur toute leur portée, donc toute
    case atteignable dépassait le seuil dès deux archers ou armes de jet en face, et la case de
    contact perdait contre n'importe quelle case lointaine — elle ne compte plus que les attaques
    **de contact** ; la menace d'un round entier, une marche d'escalier au bord de la zone où
    l'ennemi peut frapper, pesait plus que quelques cases d'approche, et un prudent restait hors de
    portée ou reculait — **avancer** quand on ne peut pas attaquer est désormais une **clé**, avant
    le score ; enfin, à score égal, le premier candidat examiné l'emportait, c'est-à-dire la case
    de plus petit indice : le repli après attaque filait au coin haut-gauche, et un tireur allait y
    tirer — à score égal, toute famille de candidats préfère désormais la case qui demande le moins
    de déplacement. Trois tests, dont un dans une salle aux dimensions de l'arène.
  - **Le chemin prévisualisé suit la droite** (`LOT-19`). Le départage « prédécesseur d'indice le
    plus petit » faisait monter le chemin pour le redescendre, d'où un tracé en triangle vers une
    case en haut à droite. L'exploration retient tous les prédécesseurs au meilleur coût, et la
    remontée choisit le plus proche de la droite départ→arrivée, à égalité le plus petit indice ;
    `findPath` et `ReachableArea::pathTo` restent d'accord sur deux cents cartes.

- **IHM de combat** (`LOT-24`, `EX-IHM-003`, `EX-CBT-020`). Le combat du Colisée se lit avant de se
  jouer, et se joue sans souris.
  - **Un curseur de ciblage** au clavier (flèches, Tab, Entrée, 1 à 9, Espace) et **à la manette**
    (croix, X, A, LB / RB, Y) : le jeu Qt Quick lit enfin la manette (`hmi::GamepadNavigator`), la
    croix se répète quand on la tient.
  - **Une prévisualisation qui est le jet** (`core::previewAttack`, `core::previewMove`) : le chemin
    tracé jusqu'au curseur et ce qu'il restera de déplacement, qui frappera en chemin ; ou l'attaque,
    son jet requis et sa chance de toucher, la CA abri compris, chaque source d'avantage et de
    désavantage. Un test compare la prévisualisation au jet jeté ensuite.
  - **La barre d'actions** : les attaques, esquiver, se désengager, se précipiter, et la réaction —
    le joueur choisit de **laisser passer** les attaques d'opportunité de son combattant.
  - **Les PV des ennemis ne s'affichent plus** : « ensanglanté » sous la moitié, comme le *Guide du
    Maître* le laisse voir, et comme l'IA le lit.
  - Le curseur a son propre calque et son propre signal : le déplacer ne reconstruit plus la scène.
  - L'écran du Colisée traduit en anglais, vues-modèles comprises ; un exécutable de test pour les
    vues-modèles (`RuntimeTests`).
  - Hors du lot, nommément : le combat sur la carte d'exploration et le HUD qui s'y superpose, que
    le jeu Qt Quick n'a pas encore (repris au `LOT-27`), les gabarits de zone (avec les sorts), le
    journal traduit (il vient du `Core`).

- **IA tactique ennemie** (`LOT-23`, `EX-CBT-050`). Les ennemis du Colisée jouent seuls, par les
  règles du *Guide du Maître* (chapitre 8, « Le combat ») et les mêmes actions que le joueur.
  - **Ce que la table sait, et rien de plus** : l'état **ensanglanté** d'un adversaire sous la
    moitié de ses points de vie (`core::isBloodied`), jamais ses PV ; la chance de toucher tirée du
    **jet requis**, CA moins bonus d'attaque (`core::requiredRoll`, `core::hitChance`) ; les dégâts
    moyens, et au critique les dés ajoutés (`core::expectedDamage`). Tout en entiers : deux
    exécutions donnent le même tour, et le rejeu le même journal.
  - **Un tour décidé case par case** (`core::planTurn`) : chaque case atteignable, chaque attaque,
    chaque cible en vue, pesée contre la menace du prochain round et les attaques d'opportunité du
    chemin ; sinon s'avancer par le chemin, se précipiter, esquiver ou se désengager ; un prudent
    recule après avoir frappé. La décision s'écrit au journal avant d'être jouée (`core::playTurn`).
  - **Ni suicide, ni blocage** : une case à portée de plus d'ennemis que le profil n'en tolère perd
    contre toute case plus sûre, quel que soit son score ; une IA qui peut attaquer attaque, sinon
    elle avance. Trente combats générés, joués par l'IA des deux côtés, atteignent tous leur issue.
  - **Cinq profils en données** (`Source/Elements/Rpg/rules/behaviors.json`) — agressif, prudent,
    soutien, archer, meute — et leurs règles d'attribution : le loup, qui porte *Tactique de
    groupe*, chasse en meute ; le singe, dont le rocher frappe plus fort que le poing, tire.
  - **La prise en tenaille**, règle optionnelle du Guide (`core::isFlanked`) : avantage au corps à
    corps pour deux alliés de part et d'autre d'un ennemi, la ligne des centres tranche. L'Arène du
    Futur la joue (`flanking` dans sa donnée).
  - **Dans l'arène** : l'action *se précipiter* (`core::ArenaSession::dash`), une politique qui
    décide des attaques d'opportunité au lieu de toutes les prendre, et une case à cocher pour
    commander soi-même les ennemis.
  - Hors du lot, nommément : les lanceurs de sorts (`LOT-25`, `LOT-35`), les actions de repaire et
    les traits de groupe comme mécanismes (`LOT-46`), l'intention de l'IA montrée autrement qu'au
    journal (`LOT-24`).

- **Portée, ligne de vue et zones d'effet** (`LOT-22`, `EX-CBT-021`, `EX-CBT-022`). Le terrain compte :
  un mur cache, un muret abrite, une boule de feu s'arrête contre une paroi.
  - **Une ligne de vue symétrique par construction** (`core::hasLineOfSight`) : des segments entre
    points de grille, testés en arithmétique entière exacte contre les cases qu'ils touchent — rien
    n'avance case par case, et A voit B si et seulement si B voit A, vérifié sur des grilles
    générées. Le coin commun de deux murs arrête le regard comme il arrête le pas ; l'eau profonde
    et la falaise ne l'arrêtent pas.
  - **L'abri du Manuel** (`core::coverFrom`, `core::coverBetween`) : partiel (+2), important (+5),
    total (on ne vise pas). Les murs abritent selon les lignes qu'ils coupent, une créature ou un
    muret partiellement, une herse de façon importante — et les abris **ne s'additionnent pas**.
    L'abri se pose sur le jet une fois et une seule (`core::AttackRoll::applyCover`), et le journal
    l'écrit : « [abri partiel : CA 15 -> 17] ».
  - **Viser demande la portée, puis la vue** (`core::checkTarget`) ; le tir au contact n'est
    désavantagé que par un ennemi qui voit le tireur ; l'esquive et l'attaque d'opportunité
    demandent de voir.
  - **Les cinq zones d'effet** (`core::AreaOfEffect` : cône, cube, cylindre, ligne, sphère) : une case
    est dans la zone si la forme en couvre au moins la moitié, calculée exactement ; les cases
    qu'aucune ligne droite ne relie à l'origine en sortent (`core::affectedCells`).
  - **Les armes déclarent leurs propriétés et leurs portées** : `properties`, `versatileDamage`,
    `rangeNormal`, `rangeLong` sont tirés de la table des *Basic Rules* à l'extraction, et les
    actions à distance du bestiaire portent leurs portées. La hallebarde frappe à deux cases, la
    dague se lance (`core::thrownAttackFor`), l'arc du squelette tire à 16/64 cases, et les armes de
    finesse le sont vraiment.
  - **Dans l'arène**, un pilier cache ou abrite ; l'écran choisit la première attaque qui peut viser
    la cible, et dit quand elle est hors de vue.
  - Hors du lot, nommément : les sorts et leurs sauvegardes (avec les classes), la lumière et les
    sens, l'affichage des portées et des gabarits (`LOT-24`).

- **Attaques, dégâts et états** (`LOT-21`, `EX-CBT-030`, `EX-CBT-031`, `EX-CBT-032`). Une attaque se
  résout selon le Manuel des Joueurs, chapitre 9, et ses dégâts traversent un pipeline.
  - **Le jet d'attaque est un objet** (`core::AttackRoll`) que trois points d'insertion lisent et
    amendent avant que l'issue ne soit figée : ajouter un avantage ou un désavantage nommé, relancer
    ou substituer un d20, ajouter un modificateur après avoir vu le total. Un 20 naturel touche
    quelle que soit la CA et fait un critique ; un 1 naturel rate toujours.
  - **Le critique double les dés, jamais le modificateur** (`core::rollDamage`) ; des dégâts ne sont
    jamais négatifs ; une salve se lance une fois pour toutes ses cibles.
  - **Un pipeline à étapes nommées** (`core::DamagePipeline`) : source, conversion, résistances,
    réserves, points de vie — chaque étape est un point d'insertion, et la dernière est un seul
    appel à `CombatState::applyDamage`. Résistance **puis** vulnérabilité, après tous les autres
    modificateurs, une seule fois chacune (l'exemple du Manuel est rejoué par test) ; des affinités
    contournables par la source (« non magique ») ; des **points de vie temporaires** qui se perdent
    d'abord, ne se cumulent pas et ne se soignent pas ; des **structures** qui ont des PV et des
    résistances.
  - **Des profils d'attaque** tirés du bestiaire (`core::attacksFor` : allonge en cases, attaque à
    distance, refus nommé d'une action sans type de dégâts) et de l'arme de la fiche
    (`core::weaponAttackFor` : Force ou Dextérité, finesse, maîtrise, coup à mains nues).
  - **Un journal qui dit tout** (`core::AttackOutcome::describe`) : « d20 = 12 + 3 (Force) + 2
    (maitrise) = 17 contre CA 15 : touche ; degats 1d8+3 : 5 + 3 = 8 tranchant ; resistance
    (tranchant) 8 -> 4 ; PV 30 -> 26 ».
  - **Ce que l'agonie lira** : deux crochets de plus, `DamageTaken` (annoncé même à 0 PV) et
    `CombatantDowned`, avec l'excédent au-delà de 0 et le drapeau critique.
  - **Dans l'arène**, le coup d'essai du `LOT-50` laisse la place à l'action *attaquer*, et
    s'ajoutent *esquiver*, *se désengager* et l'**attaque d'opportunité** à la sortie de l'allonge.
    Le personnage de démonstration frappe avec son épée longue, contre la CA de son armure.
  - Hors du lot, nommément : l'inconscience, les jets contre la mort et la mort instantanée
    (`LOT-72`), la portée, la ligne de vue et l'abri (`LOT-22`), les sorts (avec les classes).

- **PNJ et dialogues** (`LOT-15`, `EX-VIS-003`, `EX-RPG-042`). On parle à un PNJ par un arbre
  scripté : répliques, réponses, conditions sur drapeau, actions, jets de compétence.
  - **Un graphe en JSON, sans aucun texte** (`core::DialogueGraph`, `Source/Elements/World/dialogues/`).
    Chaque réplique et chaque réponse ont une clé de traduction fabriquée depuis les identifiants ;
    un test vérifie que toutes existent en français et en anglais.
  - **Refusé au chargement, pas découvert en jeu** : cible inconnue, choix vide, réponses toutes
    conditionnelles, cycle qui ne passe par aucune réponse à donner, nœud orphelin, impasse,
    difficulté écrite en nombre. Toutes les fautes d'un fichier d'un coup, chacune nommant son
    nœud ; les compétences, degrés, objets et langues nommés sont confrontés aux catalogues.
  - **`core::DialogueRunner`, pur** : il enchaîne conditions, actions et jets jusqu'à la réplique
    suivante, réévalue la condition d'une réponse au moment du geste, pose les drapeaux, donne les
    objets, démarre une quête par `quest/<id>/started`, et jette ses d20 contre le degré lu dans
    `rules/difficulty.json`. Une conversation se joue nœud par nœud sans fenêtre, et se rejoue à
    l'identique à graine égale.
  - **Refusé faute de langue commune** : un PNJ déclare ses langues, la fiche porte les siennes —
    celles de l'espèce et celles qu'elle choisit (`core::CharacterSheet::languages`).
  - **`hmi::DialogueMode`**, troisième mode de jeu : le monde est gelé pendant la conversation.
  - **L'écran est branché** : `hmi::DialogueModel` remplace `PendingData` ; on clique une réponse
    (ou `1` à `9`), une réponse qui mène à un jet l'annonce, le jet se restitue sur la réplique
    suivante, et la conversation finie referme l'écran. Dialogue de démonstration provisoire : le
    héraut du Colisée, quatorze nœuds, une Persuasion de difficulté moyenne.
  - **Autour** : l'échelle des degrés de difficulté a enfin un lecteur
    (`core::loadDifficultyScale`), la table des interactifs connaît les PNJ (`npc`, qui nomme son
    dialogue), `LedgerList` sait rendre ses lignes cliquables.
  - Hors du lot, nommément : ouvrir la conversation depuis la carte (l'interaction ne tourne pas
    encore dans la session de jeu, `LOT-27`), les quêtes et leur journal (`LOT-16`), la
    persistance (`LOT-17`), les portraits.

- **Le Colisée : bac à sable de combat** (`LOT-50`). Un lieu pour éprouver le combat, encore et
  encore, sans monter une partie — et qui est une zone du jeu final : les Arènes de Tanares.
  - **La première carte** de `Source/Elements/Levels/`, vide depuis le `LOT-01` : l'Arène du
    Futur, une enceinte de murs et de gradins, du sable, deux portes, douze points d'entrée écrits
    comme des entités `arenaEntry` de la carte — six par camp, rangés par rang.
  - **`core::ArenaSession` tient le combat** : le premier objet du jeu à tenir un
    `core::CombatState`. Il monte la composition sur la carte en nommant chaque refus, jette
    l'initiative à graine fixée, joue — déplacement, coup, fin de tour, retrait — et **rejoue** :
    une seule suite aléatoire pour l'initiative, les jets et les dégâts, et un journal ; même
    graine, même journal, vérifié par test.
  - **Personne n'y meurt** : le rituel de Marque Héroïque relève tout le monde à l'issue, sauf dans
    une arène létale, qui est l'exception écrite dans la donnée. La **troisième économie
    d'action** (`heroicAction`) est déclarée à chaque combattant marqué, par le crochet du `LOT-20`.
  - **Un coup d'essai, provisoire et dit comme tel** : déclaration, action, d20 contre la classe
    d'armure, dés, `applyDamage`. Le kit vient de la première action qui frappe d'une créature, ou
    du coup à mains nues du SRD pour un personnage. Le `LOT-21` remplace la façon, garde le lieu.
  - **La donnée** : trois arènes du Sourcebook dans `Source/Elements/World/arena/` (`lethal`,
    `heroicMark`, `map`), les huit Marques Héroïques en règle, et leurs deux schémas ;
    `check_rpg_data.py` connaît les deux familles.
  - **L'écran** : « Nouvelle partie » ouvre l'arène ; un écran de développeur en QML, sans charte, qui
    compose deux camps depuis le bestiaire et le personnage de démonstration, choisit une Marque et
    une graine, lance, joue case par case, rejoue. `hmi::ScreenId::Arena` dans la table de
    navigation, `hmi::ArenaModel` dans `Jadg.Runtime`, sa doublure pour l'atelier.
  - Hors du lot, nommément : l'ouverture de l'arène depuis le monde (`LOT-42`), le dessin du combat
    sur la carte et la manette (`LOT-24`), les capacités qui dépenseront la *Heroic Action*.

- **Initiative et tour par tour** (`LOT-20`, `EX-CBT-010`, `EX-CBT-011`, `EX-CBT-012`). Le combat a
  son horloge : qui joue avant qui, ce qu'un tour permet, et comment un combat finit.
  - **L'initiative est jetée une fois, et départagée par une règle écrite.** Total, modificateur,
    Dextérité, alliés avant ennemis, identifiant — l'ordre de la donnée, jamais celui de la mémoire.
    La relance d'un d20 que le Manuel laisse au MD est écartée : elle ferait dépendre l'ordre du
    nombre d'égalités survenues avant. Cent vingt ordres d'insertion donnent la même suite.
  - **Le tour ne finit que sur demande** (`endTurn`) : épuiser ses ressources ne termine rien. Le
    curseur du round est une **place**, pas un indice — un renfort ou un fuyard ne fait sauter aucun
    tour, et un renfort rangé après la place en cours joue ce round-ci.
  - **L'économie d'action est une liste** (`core::ActionEconomy`) : la troisième économie des
    Marques Héroïques se déclare, une réaction s'octroie. Chaque ressource revient au début du tour
    de son porteur, jamais à la fin du tour courant ; le déplacement se fractionne.
  - **Les trois fins** — victoire, défaite, fuite —, évaluées après chaque changement. Une salve qui
    abat les deux camps est une défaite, quel que soit l'ordre des cibles ; un ennemi en fuite
    compte pour la victoire ; une rencontre dont on ne fuit pas refuse la sortie d'un allié.
  - **Neuf crochets nommés** (`core::CombatHook`), des repères d'initiative fixe qui perdent les
    égalités (repaire à 20, renforts à 0), un acteur flottant, des compteurs par tour, round,
    rencontre et jour, et une mémoire d'immunité par couple (créature, source). Ce qu'un abonné
    change se règle en sortant de l'appel, jamais au milieu d'une annonce.
  - **La grille est remplie** : `core::mountEncounter` pose le groupe et la rencontre en nommant
    chaque refus ; `moverFor` dit qui l'on traverse (un allié, un ennemi à deux tailles d'écart).
  - Un combat à cinq se joue sans fenêtre jusqu'à sa fin et se rejoue à l'identique ; un autre en
    monte quatre alliés.
  - **Corrigé en chemin** (`LOT-19`) : traverser la case d'une autre créature coûte double, comme
    le dit le Manuel. Le défaut était latent tant que personne ne traversait personne.

- **Grille tactique et déplacement** (`LOT-19`, `EX-CBT-020`, `EX-REG-051`). Le combat a sa grille :
  qui se tient où, jusqu'où l'on va ce tour-ci, et par où.
  - **La règle vient du Manuel, et elle a contredit la feuille de route.** « Jouer sur un
    quadrillage » : une case coûte 1 **même en diagonale**, 2 en terrain difficile — à condition
    de pouvoir les payer —, et l'on ne coupe pas le coin d'un mur. `core::GridDistanceField`, qu'on
    devait réemployer, est un parcours à quatre voisins et à coût uniforme : il ne sait rien de
    cela. Il reste l'outil de la récompense de progression ; le déplacement est un Dijkstra à huit
    voisins (`core::ReachableArea`) et un A* (`core::findPath`).
  - **Même entrée, même chemin — et le même dans les deux algorithmes.** À coût égal, le
    prédécesseur d'indice de case le plus petit l'emporte : une règle posée sur le graphe, pas sur
    l'ordre d'exploration. L'A* ne s'arrête donc pas à la première sortie de la destination ; le
    test qui compare les deux algorithmes sur deux cents cartes aléatoires à graine fixe échoue
    quand on l'y arrête.
  - **Deux créatures ne partagent jamais une case**, par **emprise** : une créature de taille G
    tient 2 × 2, et ne passe pas là où un humain passe. `place` et `moveTo` refusent avec leur
    raison plutôt que de corriger. On traverse qui la requête autorise (`canPassThrough`), jamais
    personne par défaut, et l'on ne s'arrête sur aucun.
  - **L'altitude est un attribut, jamais une géométrie.** Un volant survole l'eau profonde et la
    falaise et ignore la boue ; il ne traverse pas les murs, que la grille de collision ne sait
    pas distinguer d'un muret.
  - **Le budget tronque** : 9 m font 6 cases, 10 m aussi — un segment de 1,50 m entamé n'en est
    pas un.
  - **Le crochet des zones** : une couche à propriétés déclare une zone (`zonesAt`) ; seul
    `difficultTerrain: true` — le booléen, pas un `1` — est interprété ici. Le terrain difficile
    naît aussi en combat, et un objet de grille a des points de vie.
  - Huit cas de test du cahier s'affichaient sans criticité ni étapes : leurs balises `\tcat`
    avaient été écrites avec une tabulation. Corrigé.

- **Charte v2 et intégration des maquettes** (`LOT-87`, en cours). Les dix maquettes du pack UI
  deviennent la charte visuelle ; les cadres, plaques et fonds seront produits à part, à 1080p.
  - **Phase 0 — socle vert.** Branches mortes archivées, débris retirés, maquettes déplacées dans
    l'epic, lot inscrit.
  - **Phase 1 — le module de conception.** Qt Design Studio ouvre chaque formulaire *et* chaque
    jumeau en mode conception, et le jeu démarre — ce que 125 commits d'une branche abandonnée
    n'avaient pas obtenu. Trois modules QML, chacun déclaré **dans le répertoire de ses fichiers**,
    sans un seul alias de ressource : `Jadg.Ui` (`Source/Ui`, QML pur, `designersupported`),
    `Jadg.Runtime` (`Source/HMI/Runtime`, les types C++, bibliothèque statique) et `Jadg.App`
    (`Source/App/Game/Qml`, la fenêtre, la pile d'écrans et les jumeaux de câblage, module de
    l'exécutable). Le diagnostic qui fonde ce découpage : l'atelier n'était pas grisé ; les jumeaux
    nommant un type C++ restaient irrésolus, et un formulaire nommait lui-même un type C++. Sa
    bibliothèque de composants, elle, liste chaque dossier « (vide) » avant comme après —
    `designersupported` posé, deux dispositions essayées — et reste à instruire. Des **doublures** QML des types C++
    (`Source/Ui/Mocks/`) rendent les jumeaux ouvrables dans l'atelier. Le garde-fou
    `check_qml_designer_compat.py` vérifie désormais un **contrat** (imports, motifs, doublures
    complètes, jumeaux appareillés, fichiers listés) et non une structure CMake. La surface de rendu
    C++ quitte `GameViewForm.ui.qml` pour son jumeau. Le pin de Qt et la découverte de Qt vivent
    dans `Source/CMakeLists.txt`, visibles des trois répertoires qui en dépendent.
  - **Phase 2 — la charte v2 écrite, et ses jetons (T2.1, T2.2).** L'epic porte la charte : ce qui
    est gardé (le parchemin, l'or et le grenat du corpus), ce qui change (panneaux sombres pour les
    écrans posés sur une scène, `Cinzel` et `IM Fell English`, ornements en images 9-patch
    produites, facteur d'agrandissement réel), ce qui est écarté et pourquoi (polices pixel, facteur
    entier hors du viewport, tracé des ornements, découpage des maquettes). Les dix rôles nouveaux
    sont **relevés** sur les maquettes par `scripts/measure_mockup_palette.py`, qui nomme maquette
    et zone pour chacun et vérifie `Tokens.qml` (`--check`). La mesure a corrigé l'œil : les
    sémantiques `success`, `danger` et `info` sont des matières de plaque, illisibles en texte.
    `EX-IHM-070`, `EX-IHM-075` et `EX-IHM-076` refondues, `EX-IHM-081` précisée. `Tokens.qml` gagne
    ces rôles, `uiScale` (lié à la fenêtre par `Main.qml`), `loreFamily` et une échelle `font*` de
    cinq tailles à 1080p ; les grandeurs v1 restent, marquées obsolètes. Jusqu'au dépôt des polices
    (T2.3), les écrans v1 s'affichent dans la famille de repli.
  - **Phase 2 — les briques de la charte v2 (T2.7).** Treize contrôles `.ui.qml` que la phase 3
    transcrira dans les écrans : `PanelFrame`, `TitlePlate`, `SectionBanner`, `OrnateButton`,
    `OrnateTab`, `OrnateCheck`, `OrnateSlider`, `OrnateCombo`, `StatMedallion`, `PortraitFrame`,
    `ItemSlot`, `Gauge`, `GoldDivider`. Les contrôles interactifs sont des contrôles Qt restylés ;
    les états sont des propriétés, et `forcedState` les impose. Chaque brique nomme une **clé du
    cahier** et pose l'image livrée — à la taille de conception, réduite d'un bloc par `uiScale` —
    ou, tant qu'elle manque, l'aplat de jetons que le cahier prévoit. `Theme/Artwork.qml` dit quelles
    pièces sont livrées ; `receive_ui_assets.py` l'écrit, `check_ui_assets.py` le vérifie. La galerie
    (`DesignStudio/Main.ui.qml`) pose chaque brique dans ses états, dans l'atelier comme dans le jeu
    (`--screen=Gallery`).

- **Refonte de l'IHM sur Qt Quick, avec la conception séparée du code** (`LOT-86`).
  L'objectif n'est pas technique : **un artiste doit pouvoir modifier les interfaces sans ouvrir un
  fichier source**, en travaillant directement dans Qt Design Studio.
  - **Deux applications, deux technologies d'IHM.** `JustAnotherRpgGame` est le jeu, en Qt Quick,
    sur `QGuiApplication` ; `LevelEditor` est l'éditeur de niveaux, inchangé, en Qt Widgets. Ils
    partagent `Core`, le rendu, les entrées, l'audio et l'amorçage — jamais une technologie
    d'interface. Le jeu **ne lie pas `Qt6::Widgets`**, et c'est la garantie qui porte tout le lot :
    un widget ne peut pas y réapparaître par inadvertance, l'édition de liens échouerait. Les tenir
    dans une seule application obligeait à choisir une technologie pour deux besoins opposés — un
    outil d'auteur à docks détachables d'un côté, une image agrandie d'un facteur entier de l'autre.
    C'est de là que venaient les 2 472 lignes de `MainWindow.cpp`.
  - **La couche de maquettes HTML disparaît.** `.design-mockups/` portait des planches dessinées à
    la main, transcrites ensuite en C++ par un développeur, et un lint vérifiait que les deux copies
    de la palette n'avaient pas divergé — trois représentations du même écran, deux transcriptions
    manuelles, un garde-fou pour rattraper les erreurs. La maquette et l'écran sont désormais **le
    même fichier**. Ce que les planches décidaient est reporté dans l'epic du lot, y compris le fait
    que leur texte était **périmé** : elles annonçaient encore la direction « Ambre nuit » du
    `LOT-68`, alors que les `LOT-66` et `LOT-76` avaient remplacé l'identité par le parchemin de
    Tanares. Le contrôle qui les reliait comparait les couleurs, pas les mots.
  - **Les jetons d'identité vivent en QML, écrits à la main, et nulle part ailleurs.** Engendrer
    `Tokens.qml` depuis le C++ aurait remis la conception derrière un générateur et un contrôle de
    fraîcheur : la surcouche qu'on supprime ailleurs. Le raisonnement qui l'évite est simple —
    après la refonte, **plus aucun C++ n'a besoin des couleurs d'identité**, leurs seuls
    consommateurs étant les écrans, qui deviennent du QML. `DesignTokens` perd donc sa portée
    identité et ne garde que celle de l'éditeur. L'étanchéité des deux portées, jusqu'ici garantie
    par un test, devient **structurelle** : deux langages, deux binaires, aucun chemin entre eux.
  - **Éditer un écran sans rien reconstruire.** `qt_add_qml_module` embarque les `.qml` dans la
    ressource ; on écrit donc un second `qmldir` dont les chemins désignent les **sources**, et
    `JADG_QML_FROM_SOURCE=1` le place en tête des chemins d'import. Ce `qmldir` est **engendré
    depuis la même liste** que la ressource : ajouter un écran ne crée pas un second endroit à
    synchroniser. Vérifié de bout en bout — deux couleurs changées dans `Tokens.qml`, relance, le
    changement est à l'écran, sans qu'aucun compilateur ait été lancé.
  - **Une tranche verticale complète** : vue-modèle C++ → formulaire `.ui.qml` → écran affichant les
    vraies données du personnage de démonstration. Le **formatage** n'est pas refait : le signe d'un
    modificateur, le « 25 / 30 » des points de vie, le point qui marque une maîtrise restent dans
    `hmi::characterSheetValues`, fonction pure et testée — deux endroits qui savent écrire un
    modificateur finiraient par ne plus l'écrire pareil. Les libellés sont des **données** : les
    compétences viennent du catalogue de règles, les caractéristiques du lexique, qui garantit une
    seule traduction par terme.
  - **Le garde-fou est le cœur du lot, pas la bascule QML.** Une refonte qui ne produit que du QML
    redérive. `scripts/check_ui_layers.py` vérifie six règles (`EX-IHM-100` à `EX-IHM-105`) et
    **verrouille** en outre `EX-ARCH-001`/`EX-NFR-010` — `Core` sans un seul en-tête Qt, vrai depuis
    le `LOT-01`, qu'un seul `QString` suffirait à rendre faux — sans les redéclarer. Les six règles
    ont été vérifiées **en mordant** : une violation injectée dans chacune, le contrôle rouge à
    chaque fois. Un lint qui passe sur du code propre mais ne se déclenche jamais ne vaut rien, et
    il s'auto-vérifie contre la vacuité — la panne du `LOT-78`, où un contrôle vert ne lisait rien.
  - **Six pièges silencieux, tous consignés là où ils se reproduiraient** : le `qmldir` engendré qui
    ne déclare pas un singleton malgré son `pragma` (chaque import en construirait une instance
    neuve, et le facteur d'agrandissement ne serait vu par aucun écran) ; le module embarqué sous un
    préfixe où l'engine ne regarde pas ; `windeployqt` sans `--qmldir`, qui produit un jeu se
    lançant sans interface ; le fichier d'enregistrement des types qui inclut les en-têtes par nom
    de base dans un `__has_include` échouant sans bruit ; `qmlcachegen` dont le C++ engendré
    déclenche `C4702` depuis les en-têtes de Qt ; et un tableau JavaScript qui n'expose que
    `modelData` là où un modèle expose ses rôles — un écran validé sur des données d'exemple se
    serait affiché vide une fois branché aux vraies.
  - `--screenshot=<chemin>` capture la fenêtre **par Qt lui-même** : les API de capture de Windows
    rendent une image noire d'une fenêtre Qt Quick, dessinée par le GPU. La vérification visuelle
    des écrans devient reproductible au lieu de dépendre d'un œil devant l'écran au bon moment.
  - `scripts/build.ps1` accepte `-Target` : le contrôle QML se lance localement comme en CI, sans
    contourner l'environnement MSVC que ce script existe pour établir.
  - **Les treize écrans existent, et l'ancienne couche est retirée du châssis d'édition**
    (−3 879 lignes). `MainWindow` redevient ce que son nom dit : le viewport est de nouveau le
    widget central, là où il partageait une pile avec cinq écrans du jeu. La pile disparaissant,
    disparaît aussi l'enveloppe défilante que chaque écran traversait — elle existait parce qu'une
    pile propage le minimum de **toutes** ses pages, y compris masquées, et qu'un écran dense
    fixait à lui seul le plancher de la fenêtre. Sans pile d'écrans, le mécanisme du défaut n'existe
    plus. L'éditeur s'ouvre désormais **directement** sur son espace de travail.
  - **La navigation est réelle.** `hmi::ScreenRouter` ne décide rien : toute la règle vit dans la
    table de transitions pure et testée, et une transition non déclarée est **refusée**, jamais
    silencieusement acceptée. Il publie un **état**, jamais un chemin de fichier — la conception
    peut réorganiser `Screens/` sans qu'une ligne de C++ ne s'en aperçoive.
  - **Les options sont branchées : chaque réglage atteint le moteur** (`EX-IHM-083`). Plein écran
    par liaison sur la fenêtre, volume vers `hmi::AudioEngine`, langue par échange de `QTranslator`
    suivi de `QQmlEngine::retranslate()`, compteur de diagnostic vers un recouvrement qui affiche la
    cadence — et synchronisation verticale sur le format de surface, donc **au prochain lancement**,
    ce que l'écran **dit** au lieu de le taire. `hmi::OptionsModel` se borne à persister et à
    prévenir ; c'est l'application qui branche. Le faire dans la vue-modèle lui aurait fait
    connaître le moteur audio et la fenêtre, c'est-à-dire la frontière même que ce lot établit. Les
    clés de configuration historiques sont **reprises telles quelles** : les renommer aurait
    réinitialisé en silence les préférences de qui jouait avant la refonte.
  - **Les contrôles Qt prennent la couleur des jetons, et il a fallu imposer le style pour cela.**
    Sous Windows, Qt choisit « FluentWinUI3 », qui peint avec les couleurs du système et ignore
    largement la palette : interrupteurs et curseur de volume ressortaient en **bleu** au milieu du
    parchemin, et aucune retouche de `Tokens.qml` n'y pouvait rien. Le jeu impose « Basic », dont
    tout le rendu vient de la palette, que `Main.qml` dérive des jetons. Sans cela, la seule issue
    aurait été d'écrire une couleur dans chaque écran — exactement ce que `Tokens.qml` existe pour
    empêcher.
  - **Un sélecteur d'écrans, pour pouvoir tout vérifier avant qu'un niveau n'existe.** Deux boutons
    font défiler les quatorze écrans (`Logic/ScreenProbe.qml`) : sans eux, les sept écrans dessinés
    mais pas encore alimentés ne sont atteignables par aucun chemin de jeu. Ce n'est pas une
    fonctionnalité, et le code le garantit — il se lie à `ScreenRouter.developerBuild`, faux dans un
    binaire livré. Il **rend la main au routeur** dès que le jeu navigue de lui-même : épinglé, il
    aurait empêché `Échap` de fermer quoi que ce soit et fait paraître la navigation cassée par
    l'outil censé permettre de la vérifier.
  - **`EX-IHM-075` n'est pas retirée, contrairement à ce que le cadrage avait conclu.** Les trois
    raisons qu'elle invoque — une image ne s'étire pas honnêtement, fige ses couleurs hors des
    jetons, et ne suit pas le facteur entier — restent vraies en QML, et **Qt Quick Shapes** les
    honore toutes en restant éditable dans Qt Design Studio. Ce qui change n'est pas « tracé par du
    code » mais « tracé par du **C++** ». La géométrie pure relevée sur le corpus est conservée dans
    `Presentation` comme source du portage, plutôt que jetée puis redessinée de mémoire.
  - **Les ornements sont tracés, en Qt Quick Shapes.** Cadre à cabochons, bandeau à ailes, fleuron
    de focus : portés depuis les géométries relevées sur `Character_Sheets_Tanares.pdf`. Un détail
    manquait au premier essai et se voyait — la pierre **déborde** du carré d'angle d'un facteur
    deux, sans quoi elle fait l'épaisseur de l'encadrement et son octogone se confond avec le filet ;
    elle reste ancrée **au coin** et jamais centrée, faute de quoi elle sortirait du panneau et se
    ferait rogner.
  - **Le jeu est traduisible, et le français est sa langue source.** Les 101 chaînes des écrans QML
    n'étaient portées par aucun catalogue. 89 traductions anglaises sont **reprises** du catalogue
    maison par `scripts/seed_translations.py`, qui ne devine rien : une source sans correspondance
    exacte reste à traduire et il la signale. Il a d'ailleurs trouvé une vraie ambiguïté du corpus —
    « Bourse » traduit **Purse** (l'argent) et **Pouch** (l'emplacement) — et a refusé de choisir.
    Les libellés à clé **calculée** (caractéristiques, emplacements) restent au lexique, dont
    `rpg.glossary.csv` garantit une traduction unique par terme de règle.
  - **La surface de rendu du jeu est un `QQuickRhiItem`** : QRhi rend en **Direct3D 11 dans une
    fenêtre Qt Quick**, et le QML se compose par-dessus — la garantie que le portage sur
    `QRhiWidget` cherchait côté éditeur, obtenue sans un seul widget. Elle n'affiche encore aucune
    scène : `Source/Elements/Levels/` est vide par construction, et bâtir une session autour d'un
    niveau inexistant aurait produit du code que rien ne peut vérifier.
  - **Documentation refondue** : `interface-ihm.md` §11, la traçabilité d'`architecture.md`, et cinq
    guides — dont un nouveau, **`guide-conception-qds`**, qui ne s'adresse pas au développeur mais à
    qui dessine les écrans : ce qui se modifie sans jamais ouvrir un fichier source, ce qui demande
    encore un développeur, et pourquoi la frontière est là.
  - **Quatre tests retirés, aucune garantie perdue.** Celui des tailles de police dans les `.ui` est
    remplacé par un lint qui couvre **tous** les écrans et non deux. Les trois tests d'étanchéité
    des portées tombent parce que l'étanchéité est devenue **structurelle** : l'identité vit en QML,
    dans un autre binaire, et aucun chemin ne relie plus les deux. C'est le meilleur sort qu'on
    puisse réserver à un test — que ce qu'il surveillait devienne impossible.
  - **Les sept écrans sans données sont dessinés, et leur travail est mis à l'abri.** Journal,
    carte, dialogue, marchand, tableau de la Guilde, ATH de combat et feuille d'équipe existent
    comme formulaires `.ui.qml`, fidèles aux blocs que la table décrivait et aux libellés de
    `fr.lang`, mot pour mot. Chacun de leurs **41 champs** porte une **clé d'attribution** nommée
    qui aboutit à l'ancre `hmi::PendingData`. Le jour où un lot fonctionnel livre sa donnée, il
    remplace `PendingData` par sa vraie vue-modèle dans le fichier de **câblage** : le formulaire
    ne bouge pas. `python scripts/list_pending_bindings.py` en donne l'inventaire — **dérivé du
    QML**, donc toujours exact, là où une liste écrite à côté aurait cessé d'être vraie au premier
    écran branché.
  - **Des tirets cadratins, jamais de fausses données.** Un écran rempli de valeurs plausibles se
    prend pour un écran fini : il passe les relectures, on l'oublie, et un jour quelqu'un s'étonne
    que le marchand vende toujours les mêmes trois objets. Le pied de l'écran l'avoue en outre —
    « Écran dessiné, données à brancher ». Les **valeurs d'exemple**, elles, vivent dans les
    formulaires : Qt Design Studio les affiche, la conception juge sa mise en page dessus, et le
    jeu ne les voit jamais.
  - **Le châssis fait défiler ce qui ne tient pas**, sur le chemin commun et non écran par écran.
    C'est la leçon payée trois fois du côté des widgets, où le même débordement fut corrigé deux
    fois écran par écran avant qu'on ne comprenne qu'une règle à réappliquer se reperd au premier
    écran ajouté.
  - Quatre défauts trouvés par `qmllint`, dont deux propres à Qt Design Studio et donc invisibles
    autrement : `screen` redéfinissait une propriété de `Window` (l'écran **physique**), et deux
    identifiants trop génériques dans des `.ui.qml` que le designer ne sait pas garantir. Plus un
    délégué qui lisait la portée de son fichier par un mécanisme que QML ne garantit plus.

- **Inventaire et équipement** (`LOT-14`). Porter, équiper et consommer des objets, avec un effet
  **mesurable** sur la fiche.
  - **Aucune statistique n'est stockée, et c'est tout le lot.** `core::Inventory` ne porte ni classe
    d'armure, ni poids total, ni encombrement : toutes sont des **fonctions** de ce qu'il contient,
    recalculées à chaque lecture. Le défaut classique — appliquer un bonus à la volée (`ca += 2`) et
    le retrancher au retrait — fait **dériver** la CA après trois équipements et deux retraits dans
    le désordre, sans que rien ne le signale. Une valeur qu'on ne stocke pas ne peut pas dériver ;
    un test joue quand même six ordres différents pour que la propriété le reste.
  - **La règle est relevée sur le livre**, pas devinée : `rules/encumbrance.json` porte la capacité
    de charge (7,5 kg par point de Force), les deux seuils d'encombrement et les pénalités de
    vitesse, chacun avec la **phrase du corpus** qui l'atteste (page 68). Tout est en **grammes** —
    le livre écrit des kilogrammes, les catalogues donnent des grammes, et mêler les deux dans une
    somme donnerait un sac de cinq cents kilos pour une poignée de fléchettes.
  - **C'est la catégorie qui décide, pas l'emplacement** : un bouclier rangé au torse compte comme
    un bouclier — il *ajoute* à la CA au lieu de la remplacer. Les confondre donnerait un personnage
    en bouclier seul avec une CA de 2.
  - **La classe d'armure de la fiche vient de l'équipement porté** : elle est calculée à la
    construction, sans rien savoir de l'armure endossée depuis. Le personnage de démonstration passe
    de 11 à 15 dès qu'il porte son cuir clouté et son bouclier — le critère du lot, visible à
    l'écran.
  - **La finesse attend sa donnée** : la branche existe, mais aucune arme ne déclare encore la
    propriété autrement qu'en toutes lettres dans son texte français, et lire une règle dans de la
    prose est ce que ce projet évite. `Weapon` lit désormais le tableau `properties` que le schéma
    prévoyait déjà.

- **Fiche de personnage : maquette et interface** (`LOT-38`). Le `LOT-68` avait livré neuf écrans
  vides ; celui-ci en **remplit un**, relevé sur les cinq feuilles Tanares **vierges** du corpus et
  alimenté par un personnage réel.
  - **Un champ affiché, ou écrit comme non alimenté** — et cette liste n'est pas un document à
    côté, elle est **dans la table** : chaque ligne de l'ossature porte un identifiant de valeur,
    ou une chaîne vide qui dit « ce champ existe, rien ne l'alimente encore ». Il reste au tiret
    cadratin, jamais à zéro : un « 0 » se lirait comme un état du jeu et mentirait. La colonne des
    identifiants vides **est** le périmètre restant — agonie (`LOT-72`), inventaire (`LOT-14`),
    dons (`LOT-47`), sorts (`LOT-35`), Guilde (`LOT-45`).
  - **La cinquième planche est une feuille d'ÉQUIPE, pas une fiche** : blason, quartier général,
    mécénat. Elle a donc son écran — le **neuvième** — et c'est la preuve de ce que le `LOT-68`
    affirmait : il s'ajoute par une entrée de table et ses clés de traduction, sans qu'aucun des
    huit autres, ni la feuille de style, ni le châssis, n'aient été touchés (`EX-IHM-090`).
  - **Les valeurs sont calculées par la règle, pas recopiées** : modificateurs signés, maîtrises
    marquées, points de vie lus contre leur maximum, Perception passive dérivée. Le tout dans une
    fonction **pure**, vérifiable sans ouvrir de fenêtre — et un test tient le seul contrat qui
    relie les deux côtés, l'identifiant de valeur.
  - **Le personnage affiché est une donnée**, `Rpg/characters/demonstration-brenna.json`, avec son
    schéma et sa validation en CI. Elle ne porte que des **choix** — espèce, classe, historique,
    caractéristiques de base, niveau : le reste est dérivé par le moteur, et le niveau s'atteint
    par gain d'expérience, le chemin qu'une partie empruntera. Déclarée **provisoire** avec son
    critère de retrait : elle disparaît quand une partie fournira un personnage réel (`LOT-29`,
    `LOT-17`).
  - **Le lexique tient maintenant les compétences et les caractéristiques.** Les dix-huit
    compétences et les six caractéristiques s'affichent, donc s'écrivent dans le catalogue de
    traduction — et ce sont des termes de règle, « Escamotage » et non « Tour de main ». Deux
    espaces de noms de plus sous `check_glossary.py`, qui passe de 0 à **24 clés de règle
    contrôlées**.
  - **La planche gravée a été essayée, puis écartée** — et la raison est écrite plutôt que perdue.
    La feuille du corpus a été vectorisée, son lettrage anglais retiré du tracé (2 565 sous-chemins
    sur 21 906) et les intitulés traduits reposés aux mêmes rectangles : cela fonctionnait. Ce qui
    l'a arrêtée est en deux temps. **Qt ne sait pas rendre ce tracé** : `QSvgHandler` rejette tout
    `<path>` de plus de 32 768 éléments — sans le dire, `isValid()` reste vrai et le rendu est vide
    — et celui-ci en demande 540 094. Le **découper** ne marche pas davantage : un remplissage se
    calcule sur l'ensemble des contours d'un même chemin, et le contour du cadre de page fait à lui
    seul 24 535 points en enveloppant toute la feuille — trois découpes essayées, trois images
    fausses. Restait le masque d'encre en PNG, qui marchait ; mais **un fond monolithique n'est pas
    un asset** : on ne peut ni déplacer un cartouche, ni réutiliser un anneau sans rejouer toute la
    chaîne. La feuille reviendra en assets **unitaires**.
  - **L'écran est donc l'ossature du `LOT-68`, remplie** : identité, progression, six
    caractéristiques avec leur modificateur, combat, six jets de sauvegarde, dix-huit compétences.
    La table des écrans reste la seule description de la fiche — le jour des assets unitaires, ce
    sont les widgets qui changeront, pas ce qu'ils affichent.

- **Le châssis des écrans du RPG** (`LOT-68`). Huit écrans manquaient au jeu, et aucun n'existait
  même en ébauche : fiche de personnage, inventaire et équipement, journal de quêtes, carte du
  monde, dialogue, marchand, tableau de la Guilde, ATH de combat. Ce lot ne les **remplit** pas —
  c'est le travail des `LOT-38`, `LOT-42`, `LOT-45` et `LOT-24` — il livre ce qu'ils ont en commun
  et qu'aucun ne doit réinventer.
  - **L'ossature est une table, et c'est tout le lot.** Le critère de la feuille de route disait :
    *ajouter un neuvième écran ne demande de toucher à aucun des huit*. Il ne se tient pas avec huit
    fichiers d'interface, fussent-ils bien écrits — le premier pied de page à corriger le serait
    huit fois. `hmi::rpgScreens()` décrit donc chaque écran en **données pures** (blocs, genres,
    libellés, `EX-IHM-090`), et le châssis Qt ne connaît **aucun** écran par son nom. Même règle
    pour la feuille de style, qui habille par **rôle** et jamais par nom d'objet.
  - **Les champs annoncés sont relevés sur les modèles déjà livrés** — `core::CharacterSheet`,
    `core::Ability`, `core::Equipment`. Une ossature qui annonce des champs que le modèle ne porte
    pas promet ce que le jeu ne pourra pas tenir. Les valeurs, elles, sont des **tirets** : ce lot
    livre le cadre, et une valeur d'exemple se lirait comme un état du jeu (`EX-IHM-072`).
  - **La règle de superposition appartient à l'écran, pas à l'appelant** (`EX-IHM-091`). La carte du
    monde et l'ATH de combat se consultent **en marchant** — on ouvre une carte pour savoir où l'on
    va sans s'arrêter ; les six autres suspendent la simulation. Décidée au point d'appel, cette
    règle se contredirait d'un appel à l'autre sans que rien ne le signale.
  - **« Nouvelle partie » ouvre le châssis, et c'est un échafaudage assumé.** Cette entrée n'a
    aucune carte à charger — `demo-deplacement.json` n'existe pas, le `LOT-67` l'avait écrit — et
    huit écrans qu'aucun chemin n'atteint ne se relisent ni ne se valident. La ligne à rendre à son
    usage le jour où le `LOT-27` livrera une carte est **une seule**, et elle le dit. En attendant,
    l'écran de jeu et celui de pause ne sont plus atteignables depuis le menu ; la table de
    transitions déclare et teste déjà l'ouverture d'un écran du RPG depuis l'un et l'autre.
  - **Trois défauts d'agencement, trouvés en ouvrant l'application** et invisibles dans le code : le
    pied d'actions passait sous la ligne de flottaison (une seconde zone défilante borne désormais
    le contenu seul, le pied reste posé au bas du cadre) ; le bandeau de titre débordait de la
    fenêtre à la taille des titres d'écran, sortant la colonne de droite du cadre sans qu'aucune
    erreur ne soit levée ; et les rappels de touches imposaient leur largeur — une aide ne décide
    pas de la largeur d'une fenêtre.

- **Menus et vocabulaire d'un RPG** (`LOT-67`). Le jeu décrivait un autre jeu : « Choisir un
  niveau » au menu, « Recommencer le niveau » en pause, et un avertissement de sortie qui parlait
  de « la progression du **tableau** en cours ». Ce lot retire la **notion de niveau discret** —
  des écrans, du code, du vocabulaire et des exigences — et remplace le décor du menu principal par
  la **carte du monde de Tanares**.
  - **Le menu principal perd deux entrées, parce qu'elles ne menaient plus nulle part.**
    « Continuer » reposait sur une progression au tableau, « Choisir un niveau » sur une séquence :
    les deux sont retirées. Les griser aurait coûté plus de confiance qu'elles n'apportaient
    d'information (`EX-IHM-072`). « Continuer » revient avec la sauvegarde du `LOT-17`, les entrées
    RPG de la pause avec les écrans du `LOT-68`.
  - **Il perd aussi son titre** : le fond *est* la carte du monde, et un bandeau posé dessus
    répétait en lettres ce que l'image dit déjà. Les autres écrans gardent le leur — sans image à
    eux, on ne saurait pas où l'on est.
  - **La carte du monde trace la frontière que le `LOT-76` avait ouverte.** Ce lot-là concluait que
    l'habillage se **trace** (`EX-IHM-075`) ; celui-ci livre une image, et c'est la même frontière
    prise de l'autre côté (`EX-IHM-076`) : un ornement se trace parce qu'il doit se redimensionner
    et suivre les jetons, une carte peinte ne le peut pas. Mêmes garde-fous — région déclarée,
    manifeste recoupé en CI avec les fichiers et le code, repli si l'image manque.
  - **Du JPEG, seul du dépôt, et c'est délibéré** : le PNG de cette carte pèse 4,4 Mo, son JPEG
    0,7, pour une différence que personne ne voit sous un voile. Le poids du dépôt est un sujet du
    corpus depuis le début (`EX-CNT-023`). Le filigrane d'achat est **recadré**, jamais effacé :
    l'effacer demanderait de repeindre ce qu'il recouvre.
  - **~1 300 lignes retirées** : deux écrans (sélection de niveau, fin de niveau), deux modèles
    (`Progression`, `LevelSequence`), un bilan de partie (`LevelRunStats`), deux maquettes. C'étaient
    les mises en œuvre des exigences retirées ; les garder aurait laissé du code que plus aucune
    exigence ne justifie.
  - **Seize exigences traitées : douze retirées, trois refondues, une conservée.** `EX-GP-040`,
    `EX-IHM-003` et `EX-IHM-004` avaient un objet **au-delà** du niveau discret — le jeu a toujours
    des états, un ATH et un écran de pause — et les retirer aurait laissé leur mise en œuvre
    orpheline. Les douze autres sont **retirées, pas supprimées** : leurs ancres restent, avec le
    motif du retrait.
  - **« niveau » devient « carte », partout**, y compris côté éditeur où la famille de clés
    `level.*` devient `map.*` : l'éditeur n'édite pas des niveaux, il édite les cartes du monde.
    Les deux catalogues restent synchrones, 376 clés de chaque côté. L'événement `LevelCompleted`
    devient `ExitReached`, et `SequenceCompleted` disparaît avec son bruitage.
  - **Ce que le lot ne rend pas jouable, et le dit** : « Nouvelle partie » ouvre
    `demo-deplacement.json`, qui **n'existe pas** — le `LOT-01` a purgé les niveaux du jeu de
    plateforme et aucun lot n'en a livré depuis. Le constat est antérieur à ce lot (la « Nouvelle
    partie » d'avant chargeait une séquence tout aussi absente) mais il devient visible.

- **Habillage d'interface extrait des livres** (`LOT-76`). Les écrans du jeu portent enfin ce qui
  fait reconnaître une page de Tanares en une seconde : la **pierre sertie** à l'angle des panneaux
  et le **bandeau de titre à ailes**. Tous deux **tracés**, donc nets à tout facteur
  d'agrandissement et pilotés par les jetons de la charte.
  - **Le découpage d'images a été construit, puis abandonné.** Vingt et une planches PNG à 300 ppp,
    leur manifeste et leur lint d'intégrité existaient ; c'est l'écran qui a tranché. Un cabochon de
    108 pixels sur un panneau haut de 340 mangeait le tiers de sa hauteur, et le bandeau demandait
    l'impossible à un découpage en tranches — une plaque qui s'allonge avec le titre, des ailes qui
    n'en font rien. À ce point-là, on ne redimensionnait plus une image, on la redessinait mal.
  - **Ce que le corpus donne reste entier : la mesure, pas la matière.** Le grenat des cabochons
    (`#701010`) est la dominante quantifiée des pixels rouges d'un cabochon, mesurée **séparément
    sur deux angles opposés** de la planche — même méthode que la palette du `LOT-66`, même
    vérification croisée. Le grenat profond des plaques (`#400000`) vient du bandeau du livre.
  - **`error` n'est plus le seul rôle inventé.** Le `LOT-66` le signalait comme non attesté, « une
    feuille de personnage n'ayant pas d'état d'erreur à montrer ». C'était vrai d'un état d'erreur
    et faux du rouge : il est dans les gemmes. Deux jetons neufs, `gem` et `gemShadow`, relevés.
  - **L'invariant du bandeau : l'envergure des ailes suit la hauteur, jamais la largeur.** Un titre
    long allonge la plaque et rien d'autre — ce qu'une image étirée ne sait pas faire. Quand la
    largeur manque, ce sont les ailes qui cèdent, puis disparaissent ; jamais la plaque, qui porte
    le titre.
  - **La variante accentuée garde ses angles nus.** Son filet passe à la couleur d'accent pour
    signaler un écran superposé ; une pierre par-dessus rendrait ce signal illisible.
  - **Les six titres d'écran deviennent des bandeaux.** Le texte tient **entre** les ailes par les
    marges de contenu, et non par un décalage au moment de peindre — sinon l'élision décide sur la
    mauvaise largeur, et le mot coupé n'apparaît que sur le titre le plus long. Sa couleur passe à
    l'or pâle : l'or des filets tenait sur du parchemin, il disparaît sur le grenat.
  - Exigence ajoutée : `EX-IHM-075` — l'habillage ornemental se **trace**, il ne se livre pas en
    image. `EX-IHM-070` imposait de relever les **couleurs** ; rien n'était écrit des **formes**.

- **Charte visuelle : sortir de l'identité pixel art** (`LOT-66`). L'interface héritée du jeu de
  plateforme laisse place à l'identité du **parchemin de Tanares** — parchemin, encre sépia, filets
  et cabochons dorés, titrage à empattements.
  - **La palette est relevée, pas choisie.** Chaque teinte vient de l'histogramme quantifié des
    pages rendues de `Character_Sheets_Tanares.pdf`. C'est la règle du corpus transposée à la
    couleur : une couleur inventée ressemble à la source sans en venir, et rien ne le dit jamais.
    Un seul rôle n'est pas attesté — `error`, une feuille de personnage n'ayant pas d'état d'erreur
    à montrer — et il est **signalé comme tel dans le code** plutôt que glissé dans la liste.
  - **Les rôles de cadre changent de nom, et c'est le cœur du lot.** `outline`, `bevelLight`,
    `bevelDark` nommaient un **biseau** : une lumière venue d'en haut à gauche. Le parchemin n'a pas
    de relief à simuler. Garder ces noms en peignant un encadrement plat aurait produit du code
    juste dont les noms décrivent autre chose. Ils deviennent `frameEdge`, `frameOrnament`,
    `frameShadow`.
  - **Ce qui fait un encadrement, c'est la réserve.** Un trait d'encre, une **réserve de
    parchemin**, un filet doré : sans la réserve du milieu, les deux traits se touchent et
    l'ensemble devient une bordure épaisse de deux tons — sans qu'aucune erreur ne soit levée,
    toutes les bandes étant toujours là. Un test relève donc le rôle **visible** à mi-hauteur et
    exige d'y trouver du parchemin.
  - **Le facteur d'agrandissement reste entier, pour une autre raison.** Le filtrage au plus proche
    voisin ne le justifie plus ; les longueurs de la feuille de style, elles, sont des entiers de
    pixels, et à 1,5× le trait et le filet s'arrondissent à la même épaisseur — la réserve
    disparaît. Une échelle fractionnaire ne serait pas *floue*, elle serait **fausse**.
  - **641 lignes de widgets pixel art supprimées** de `Source/HMI/Interface/`. Quatre des cinq
    modules disparaissent ; `PixelArtScale` est **renommé** `IdentityScale` parce qu'`EX-IHM-081`,
    que le lot ne touche pas, est écrite en fonction de ce facteur — le supprimer laisserait une
    exigence sans mise en œuvre.
  - **Le focus n'est jamais perdu de vue.** `EX-IHM-071` et `EX-IHM-072` sont tenues sans
    changement. Le curseur en escalier devient un **fleuron** anticrénelé, tracé **une seule fois**
    et appelé des deux côtés : deux tracés séparés dériveraient l'un de l'autre, et le joueur
    croirait à deux états différents.
  - **Onze exigences refondues**, à commencer par la racine `EX-ARCH-022` — dont dix tenaient
    d'elle leur justification.
  - `ctest` reste à **1009** cas, tous verts.

- **Équipement : armes, armures et matériel** (`LOT-34`). Les tables des *Basic Rules* vers
  `Source/Elements/Rpg/` — **37 armes**, **13 armures** et **125 objets** (matériel, outils,
  montures et véhicules).
  - **C'est le lot où le §4 se paie.** « Un tableau ne s'extrait pas en flux de texte » est une
    règle du projet depuis le `LOT-30` ; nulle part sa conséquence n'est aussi silencieuse qu'ici :
    une valeur de prix décalée d'une ligne ne casse rien, ne lève aucune alerte, et déséquilibre
    l'économie sans que personne ne comprenne pourquoi.
  - **Le groupe d'une arme n'est pas dans sa rangée.** Ce sont les intertitres du livre — « Armes
    courantes de corps à corps », « Armes de guerre à distance » — et eux seuls qui disent qu'une
    arme est courante ou de guerre. Une extraction qui ne lirait que les rangées produirait
    trente-sept armes sans catégorie, et `category` est requis au schéma sans que rien ne dise
    qu'il est **juste**. Le défaut s'est manifesté à la première exécution : la bande d'ordonnées
    commençait après le premier intertitre, et dix armes sortaient sans catégorie — c'est le
    contrôle de cardinal, 27 au lieu de 37, qui l'a dit.
  - **Les trois formes de la colonne CA disent trois règles.** `11 + Mod.Dex` sans plafond,
    `14 + Mod.Dex (max +2)` plafonné, `18` sans Dextérité du tout. Les réduire à leur premier
    nombre appliquerait la Dextérité au harnois — ce qui rend le personnage **plus** résistant,
    jamais moins, ne provoque aucune erreur et passe pour de l'équilibrage. Le test choisit une
    Dextérité de +4 précisément pour que l'écart se voie.
  - **Trois cas que le livre écrit et qu'un schéma refusait.** Le **filet** n'inflige aucun dégât —
    il entrave — et `damage` est devenu facultatif plutôt que de lui inventer des dés ; la
    **fronde** n'a pas de poids, et le tiret du livre vaut *absent*, jamais zéro ; le **bouclier**
    n'est pas une armure, il ajoute au lieu de remplacer, et le traiter comme telle donnerait une
    CA de 2 à un personnage en bouclier seul.
  - **La table du matériel est composée en deux sous-tables côte à côte** : une rangée y porte six
    cellules, donc deux objets. Les lire d'un bloc donnerait un objet pesant
    « 500 g Billes de fronde (20) ».
  - **Deux unités converties une seule fois.** Le livre mêle kilogrammes et grammes dans la même
    table, et compte en pièces d'or, d'argent et de cuivre ; les catalogues ne connaissent que les
    grammes et les pièces de cuivre.
  - `ctest` passe de 1001 à **1008** cas, tous verts.

- **Entités de carte et interaction** (`LOT-10`). Les cartes se peuplent d'entités qui ne sont
  **pas des tuiles** — coffres, panneaux, et demain PNJ et portails — et le joueur peut interagir
  avec elles.
  - **Le piège du lot est un coffre ouvert deux fois.** Le critère est facile à énoncer et facile à
    rater : *y compris après un aller-retour de carte*. C'est cette moitié de phrase qui décide de
    la conception — quand le joueur revient, l'entité du coffre est **recréée depuis le fichier de
    niveau**, qui ne sait rien de ce qui s'est passé. Un booléen porté par l'entité disparaîtrait
    avec elle, et le coffre redonnerait son butin à chaque passage : un défaut qui ne casse rien,
    ne lève aucune alerte, et se confond avec de la générosité de conception. L'état vit donc dans
    `core::WorldFlags`, à côté des entités. **Le test détruit le monde et le reconstruit** pour le
    vérifier.
  - **La clé de drapeau est fabriquée, jamais écrite à la main** : `<carte>/<type>@<colonne>,<ligne>`.
    Deux coffres d'une carte se distinguent par leur case, et le nom de carte empêche que vider un
    coffre au village en vide un autre au donjon. Corollaire assumé : déplacer un coffre dans
    l'éditeur le remet à neuf pour une partie en cours — l'inverse demanderait un identifiant
    stable que le `LOT-11` devrait générer et maintenir unique.
  - **Un coffre vidé n'est plus une cible du tout**, et pas seulement une cible qui ne fait rien :
    continuer à l'afficher promettrait au joueur quelque chose qui n'arrivera pas.
  - **La case visée suit la direction dominante, jamais une diagonale.** Un personnage qui regarde
    à 30° vise la case de droite : viser en diagonale rendrait la cible imprévisible à la manette
    analogique, alors que le joueur doit savoir ce qu'il désigne **avant** d'appuyer. Une
    orientation nulle ne vise rien.
  - **L'interaction ne traverse pas un mur**, et **à plusieurs candidats le choix est
    déterministe** — le plus proche du centre de la case visée, puis le plus petit indice. Sans
    départage, deux objets sur la même case donneraient tantôt l'un tantôt l'autre selon l'ordre de
    parcours de l'ECS, qui n'est pas stable.
  - **Un type d'entité inconnu produit tout de même une entité**, sans composant interactif : la
    refuser ferait disparaître un objet de la carte sans que son auteur comprenne pourquoi
    (`EX-NFR-040`). Les familles connues sont une table, non un `if` par cas — le `LOT-15` et le
    `LOT-09` en ajouteront sans retoucher la fonction.
  - `ctest` passe de 992 à **1001** cas, tous verts.

- **La fiche de personnage** (`LOT-13`). Toute créature — héros, PNJ, ennemi — a désormais une
  fiche complète : caractéristiques, points de vie, classe d'armure, niveau, bonus de maîtrise,
  jets de sauvegarde, compétences, vitesse. Et elle **monte de niveau**.
  - **« Aucune valeur de règle dans le C++ » se vérifie sur le diff, pas sur l'intention.** C'est le
    critère le plus facile à croire tenu : une valeur de règle a l'air d'une constante
    d'implémentation, et rien ne les distingue une fois écrites. Ce lot en a trouvé **trois**.
  - **La table d'expérience est une donnée** : vingt seuils et vingt bonus de maîtrise, extraits de
    la table des *Basic Rules* p. 11 et lus **par coordonnée** — ses trois colonnes n'ont ni filet
    ni séparateur. Elle tiendrait en trois lignes de C++, et c'est ce qui la rend dangereuse :
    équilibrer la progression demanderait alors une recompilation à chaque essai. Deux contrôles
    arrêtent la génération — les vingt niveaux présents **et dans l'ordre**, les seuils
    **strictement croissants** : deux seuils inversés rendent une montée infranchissable, ou
    franchissable deux fois.
  - **Deux constantes extraites avec la phrase qui les atteste.** La classe d'armure sans armure et
    le plafond d'une caractéristique — 10 et 20 — sont cherchés dans leur phrase du livre, et la
    phrase est écrite dans la donnée produite. C'est ce qui distingue une constante extraite d'une
    constante tapée de mémoire : la seconde a l'air de la première.
  - **Le `20` que le `LOT-36` avait codé en dur est parti.** `abilityScoreWith()` bornait une
    augmentation d'espèce à une constante ; le plafond est désormais un paramètre lu dans la
    donnée, et le test du `LOT-36` a été repris pour le lire **au même endroit que le moteur** —
    sinon il vérifierait sa propre copie de la règle.
  - **La fiche est un objet autonome, jamais un singleton joueur.** Le test construit **quatre**
    fiches, en blesse une, en fait monter une autre de deux niveaux, et vérifie que les deux
    dernières n'ont pas bougé : si `CharacterSheet` devenait un singleton, ce cas tomberait le
    premier. Le bonus de maîtrise n'y est d'ailleurs pas stocké — il se lit dans la table au niveau
    courant, sans quoi une montée de niveau laisserait un personnage avec le bonus de l'ancien.
  - **Le composant ECS ne porte pas la fiche, il la désigne.** Une fiche n'appartient pas à une
    entité : un personnage garde la sienne quand il change de carte et que son entité est détruite
    puis recréée. `INDICE_ABSENT` distingue une entité **sans** fiche d'une entité liée à la
    première du registre — les confondre ferait attaquer un tonneau avec les caractéristiques du
    héros.
  - **Trois décisions de règle, écrites là où on les lit.** Les points de vie sont
    **déterministes** (le livre laisse le choix ; des PV tirés au dé rendraient une partie
    irrejouable) ; monter de niveau **n'est pas un soin**, les PV courants montent du gain et non
    jusqu'au maximum ; et **perdre de l'expérience n'est pas une règle de ce jeu**, un gain négatif
    est ignoré plutôt que d'aboutir à une descente de niveau silencieuse.
  - **Le lot n'a pas écrit de `ClassDefinition`** : le `LOT-36` l'avait déjà livrée sous le nom de
    `PlayableClass`. Un second type pour la même chose aurait créé deux vérités sur ce qu'est une
    classe.
  - `ctest` passe de 983 à **992** cas, tous verts.

- **Espèces, historiques et classes provisoires** (`LOT-36`). De quoi construire un personnage
  jouable au plus tôt : **22 espèces**, **13 historiques** et les **4 classes simplifiées** qui
  serviront de socle au premier modèle de combat — 39 fichiers tirés de **trois documents et deux
  langues**, et c'est ce mélange qui fait la difficulté du lot.
  - **Le *Manuel des Joueurs* est un scan, et sa graisse ment.** La méthode du `LOT-33` — la
    graisse porte la structure — n'y tient pas : « Vitesse. » ne porte aucune graisse, « Âge. » en
    porte sur deux fragments non contigus, et les titres sont mutilés (`TaiJJe`,
    `Vision dans Je noir`, `tliaumaturgie`). Deux parades : les mécaniques se lisent par **leur
    phrase** et non par leur titre, et les noms de traits viennent du **lexique**, qui les porte
    proprement — la parade que le `LOT-43` employait déjà pour les dons.
  - **Le recoupement a servi dès la première exécution.** Les augmentations de caractéristique
    figurent deux fois dans le *Manuel* : dans le bloc de la race et dans la table de la page 12.
    L'OCR a **entièrement effacé** la ligne d'augmentation du demi-elfe — son bloc commence au
    milieu d'une phrase — et c'est la table qui la restitue. Une valeur présente des deux côtés et
    différente **arrête** la génération ; une valeur présente d'un seul côté est une ligne
    escamotée, rapportée et non fatale.
  - **Quatre corruptions d'OCR déclarées une par une** : `!'ore` pour « l'orc » (le `l` ressort en
    point d'exclamation, le `c` en `e`), `commwi` pour « commun », `(+l)` pour `(+1)` sur toute la
    table. Sans les deux premières, le demi-orc ne parle que le commun et le tieffelin pas du tout.
    Une substitution non déclarée serait indiscernable d'une règle du jeu.
  - **La gouttière du *Manuel* bouge d'une page à l'autre** — `[288, 309]` p. 41, `[272, 296]`
    p. 42, rien du tout p. 44. Un blanc figé y couperait tantôt dans une colonne, tantôt dans
    l'autre. La coupe est désormais **mesurée** page par page.
  - **Le *Player's Guide* dessine ses titres deux fois**, à la coordonnée exacte : un titre
    contourné, dont le remplissage et le trait forment deux passes. Invisible à l'écran, et cela
    double tout ce qui se compte — les douze espèces du chapitre 1 s'y relèvent vingt-quatre fois.
  - **Ce que le schéma ne peut pas dire n'est ni jeté ni inventé.** Les espèces de Tanares laissent
    une augmentation **au choix du joueur** : la partie fixe entre dans la table, le mécanisme est
    déclaré (`EX-CNT-030`) et listé au chargement (`EX-CNT-031`). Le **soulborn**, lui, hérite sa
    taille et sa vitesse des parents du personnage et n'en a donc aucune ; le schéma les exige, et
    l'espèce est **écartée en le disant** plutôt que dotée de valeurs inventées.
  - **Le corpus dit douze espèces de Tanares, pas treize**, et son chapitre des historiques en
    annonce six pour en porter sept — le sommaire fait foi, il indexe ce que le livre contient.
    Tanares ne porte d'ailleurs **aucune mécanique** pour les huit espèces classiques : le fond
    narratif vient de lui, la mécanique des livres français.
  - **Les quatre classes sont provisoires et le déclarent**, avec un critère de retrait écrit
    d'avance et à un seul endroit. Un test balaie tout `Source/Elements/Rpg/` et vérifie qu'aucune
    donnée définitive ne les référence : le jour du retrait, supprimer ces fichiers ne cassera rien.
  - **La progression du niveau 1 au niveau 5 vient de la donnée.** La formule générale donnerait le
    même résultat, et c'est le piège : l'écrire en C++ ferait cesser de lire la table, et la
    première classe dont la progression sort de l'ordinaire passerait inaperçue.
  - **Un seuil de corps se pose sous la valeur mesurée, jamais dessus.** Le corps rendu par un PDF
    est un flottant : 16 s'y lit 15,999998. Un seuil à l'égalité laissait passer le titre de
    chapitre et ratait les quatre races — sans erreur, sans message, avec un catalogue à une entrée.
  - Trois modules partagés sortent de ce que le `LOT-33` avait écrit pour lui seul — la grille à
    deux colonnes, la relecture des catalogues livrés, la déduplication des lignes surimprimées ;
    le bestiaire est reposé dessus et produit une sortie **identique à l'octet près**.
  - `ctest` passe de 974 à **983** cas, tous verts.

- **Le bestiaire de base : les 94 bêtes du SRD** (`LOT-33`). `Animaux.pdf` vers
  `Source/Elements/Rpg/creatures/` — 94 fichiers, 136 traits, 135 actions dont 115 portent des
  dégâts typés. Le premier catalogue rempli du projet, et le premier que le moteur charge.
  - **L'extraction se fait sur la typographie, pas sur des expressions régulières.** Un bloc de
    statistiques n'a ni balise ni ponctuation qui sépare le nom d'un trait de sa description :
    seule la graisse le fait — « **Vue aiguisée**. L'aigle a un avantage… ». Découper au premier
    point donne « Attaque au corps à corps avec une arme : +4 au toucher, allonge 1,50 m » comme
    nom d'action. `Extracteur.lignes()` rend désormais police, corps et graisse, et sept
    discriminants **mesurés** — pas devinés — découpent le document : le corps du titre, la police
    du paragraphe d'ambiance, celle des encadrés « Variante », l'interligne. Le document ne porte
    aucun interligne entre 11,3 et 15,2 pt ; le seuil tombe dans ce vide.
  - **Le mode texte perd des espaces, et la faute est indétectable en aval.** Les titres de traits
    en sortent collés — `Vueaiguisée`, `Sens dela toile`, `Tactiquedegroupe`. Aucun contrôle ne la
    rattrape et aucune relecture de la donnée produite ne la signale, puisque la donnée produite
    *est* la faute. Le fragment de police, lui, porte le texte tel que le document l'écrit.
  - **Le sommaire est le point d'attestation.** 94 entrées page 2, 94 titres dans le corps, et les
    deux listes doivent coïncider nom pour nom : c'est le seul contrôle qui détecte un bloc sauté,
    panne qui ne laisse aucune autre trace — un catalogue de 93 créatures se charge, se valide et
    se joue exactement comme un de 94. La gouttière des deux colonnes, elle, est re-vérifiée page
    par page : si elle bouge, la coupe **échoue** au lieu de mélanger deux créatures.
  - **Ce que le schéma ne peut pas dire n'est ni jeté, ni élargi en silence.** « Résistance aux
    dégâts contondants provenant d'attaques **non magiques** » : mettre `bludgeoning` dans
    `damageResistances` rendrait le diablotin résistant à une masse d'armes ordinaire. La clause
    qualifiée reste dans un trait, et la créature **déclare** le mécanisme
    `resistance-conditionnelle` (`EX-CNT-030`). Même traitement pour « comprend le commun mais ne
    peut pas le parler ». Et « l'**aérien** », que l'aigle géant comprend, n'est ni au catalogue
    des seize langues ni à la table des *Basic Rules* p. 38 : le rapprocher du primordial serait un
    élargissement muet, la génération le **signale** et ne l'écrit pas.
  - **Cinq défauts du lexique mis au jour par les 94 noms.** Les variantes séparées par `/`
    (« Bec de hache / Autrache ») — défaut qui se propage aux catalogues livrés, où
    « Tromperie / Supercherie » faisait désigner au diablotin une compétence introuvable ; « Zombi
    Objets magiques D&D 5 », un titre de section happé ; « Tigre à dents de **sabe** », une
    coquille ; l'entrée « Tigre » dont le côté **anglais** est resté en français, qui sortait un
    identifiant `tigre` au milieu de quatre-vingt-treize identifiants anglais ; et la langue des
    elfes nommée « elfe » quand le livre écrit « **elfique** ». Ce dernier était déjà tranché par
    le `LOT-43` : sa table d'alias est **réutilisée**, pas recopiée. Le quatrième ne se détecte pas
    mécaniquement — « Quasit = Quasit » est identique des deux côtés et juste — et c'est une
    relecture des 94 identifiants qui l'a trouvé.
  - **Le test lit le livre, pas la génération.** Douze profils sont rejoués contre des valeurs
    recopiées à la main du PDF, chacun avec sa page imprimée. Un test qui comparerait la sortie de
    la génération à elle-même passerait quelle que soit la faute d'extraction.
  - **Un champ `description` au schéma de créature** : 21 blocs se terminent par un paragraphe
    d'ambiance qui ne porte aucune règle et qui est pourtant ce que le bestiaire affichera. Le
    premier remplissage d'une famille est le moment où le contrat rencontre la réalité.
  - `core::CreatureSize` rejoint les énumérations fermées, confrontée au **schéma** et non au
    lexique : celui-ci n'en porte que cinq, « Moyenne » ayant échappé à l'extraction du glossaire.
  - `ctest` passe de 968 à **974** cas, tous verts.

- **Le cœur chiffré : dés, caractéristiques, jet de d20** (`LOT-12`). Du calcul pur dans `Core`,
  sans fenêtre ni GPU : la notation `NdF±M`, les six caractéristiques, le d20 avec avantage et
  désavantage, et les six degrés de difficulté en **donnée**.
  - **Le défaut que les tests ont trouvé et que la relecture n'aurait pas vu.** `nextInt` rejette
    la queue de l'intervalle pour éviter le biais modulo ; la formulation naturelle — rejeter
    au-delà du dernier multiple complet — donne un seuil de 2³² quand l'étendue divise 2³², qui ne
    tient pas dans un `uint32` et retombe à **0** : boucle infinie. Le défaut ne se voit que sur
    les **puissances de deux** — un d6 et un d20 passent, un d8 gèle. C'est le test de
    rejouabilité, écrit sur 3d8, qui l'a attrapé. La version retenue rejette la queue basse, vide
    dans ce cas.
  - **Deux arrondis qui ne se voient pas.** `(score - 10) / 2` tronque vers zéro en C++ : un score
    de 7 donnerait `-1` au lieu de `-2`, et le personnage serait moins pénalisé qu'il ne doit
    l'être sur chacun de ses jets, pendant toute la partie. Et avantage plus désavantage
    **s'annulent** (`EX-REG-002`), y compris à deux contre un — la règle annule, elle ne compte
    pas. Les deux ont leur cas de test explicite.
  - **Un 20 naturel n'est pas un total de 20** : `isNaturalTwenty()` regarde le dé retenu. Les
    confondre rendrait critique un jet sur deux à haut niveau, et passerait pour de l'équilibrage.
  - **La restitution est une exigence, pas un journal** (`EX-REG-003`) : les **deux** dés sont
    conservés en cas d'avantage, et chaque modificateur porte son origine —
    `d20 (avantage : 7, 14) = 14 + 3 (Dexterite) + 2 (maitrise) = 19 >= 15 : reussite`.
  - **Les degrés de difficulté sont une donnée** (`EX-REG-021`), extraits de la table « Tâche / DD »
    des *Basic Rules* : six paliers de 5 à 30. Le test lui-même lit le seuil dans le fichier plutôt
    que d'écrire `15`.
  - **Une case vaut 1,5 m**, figé dans une constante nommée. La conversion mètres ↔ cases existe
    forcément quelque part ; le seul choix ouvert était *à un endroit, ou à trente*.
  - `ctest` passe de 954 à **968** cas, tous verts.

- **Les options de personnage : dons, multiclassage, compétences, langues** (`LOT-43`). Quatre
  catalogues oubliés du premier découpage, que la fiche de personnage suppose sans jamais dire d'où
  ils viennent : **18 compétences**, **16 langues**, **42 dons** et la règle du multiclassage — 77
  fichiers de données, quatre schémas, et le mécanisme C++ qui cumule les emplacements de sorts.
  - **Le multiclassage vient du *Manuel des Joueurs***, seule source complète : les *Basic Rules*
    n'en portent ni les prérequis ni les maîtrises et renvoient au chapitre 6. Les douze prérequis
    de caractéristique et les douze lignes de maîtrises en sortent proprement.
  - **Sa table d'emplacements est recoupée**, et il le fallait : l'OCR y efface les cellules valant
    `1`, onze lignes sur vingt amputées. La donnée est prise sur la progression du magicien des
    *Basic Rules* — identique et en texte natif — puis confrontée cellule à cellule au *Manuel* :
    **26 cellules rétablies**, annoncées à chaque génération. Une divergence qui ne serait pas un
    `1` manquant **arrête** la génération.
  - **Les deux sources françaises ne traduisent pas les mêmes dons pareil** : « Adepte des
    éléments » contre « Adepte élémentaire », « Ritualiste » contre « Magie rituelle » — douze dons
    sur quarante-deux. Le lexique fait autorité, la graphie du livre est conservée en `variantes`.
    Les noms viennent d'ailleurs du lexique et non du livre, dont les titres sont des scans
    mutilés : `DouÉ`, `E(PLOR.) __ TEUR DE DONJONS_`.
  - **« Sorcier » n'est pas *sorcerer***. Le *Manuel* appelle ainsi la classe que le lexique nomme
    « occultiste » (*warlock*), alors que l'**ensorceleur** est deux lignes plus haut dans la même
    table. Un rapprochement par ressemblance aurait interverti leurs prérequis en silence ; l'alias
    est déclaré, avec la raison.
  - **Le meilleur test du multiclassage est celui du livre** : *« ce rôdeur 4/magicien 3 […] quatre
    emplacements de niveau 1, trois de niveau 2 et deux de niveau 3 »*. Le test le reproduit et lit
    la table **livrée** — c'est le livre qui vérifie l'implémentation. Deux pièges sont couverts :
    l'arrondi se fait **par classe** (paladin 3/rôdeur 3 donne 2, pas 3) et la **magie de pacte est
    exclue** de la somme (`EX-RPG-052`).
  - **Les 42 dons sont livrés `narratif` et `provisoire`**, critère de retrait écrit d'avance
    (`EX-CNT-032`) : aucun mécanisme de don n'existe encore, et un don qui se présenterait comme
    jouable coûterait plus cher à diagnostiquer qu'un don déclaré non joué.
  - **Toute langue citée par une créature ou une espèce doit exister au catalogue** — nouveau
    contrôle en CI, vérifié par injection d'une créature parlant le « draconien ».
  - `ctest` passe de 948 à **954** cas, tous verts.

- **L'OCR ne corrompt pas seulement les nombres, il en supprime** (constat de préparation du
  `LOT-43`). Sur la table du multiclassage du `Manuel-Des-Joueurs`, l'extraction **efface toute
  cellule valant `1`** : onze lignes sur vingt amputées, et un magicien de niveau 20 y perd ses
  emplacements de niveau 8 et 9. Une valeur fausse finit par se voir ; une valeur absente ressemble
  à une case vide légitime, et cette table en contient de vraies — aucune relecture ne pouvait
  l'attraper. Le défaut a été révélé par **recoupement** avec la table du magicien, identique et en
  texte natif propre dans les *Basic Rules*. La §4 de la feuille de route porte désormais ce
  quatrième niveau de bruit, et la règle qui en découle : *une table numérique tirée d'un scan se
  recoupe contre une seconde source ou contre un invariant.*

- **Les schémas de données RPG** (`LOT-32`). Le contrat **avant** les données : onze schémas JSON,
  un validateur en CI et trois énumérations C++, livrés alors qu'aucune donnée n'existe encore.
  C'est l'ordre qui compte — un contrat écrit après coup se contente de décrire ce qui a déjà été
  produit, défauts compris.
  - **Onze schémas** sous `Source/Elements/Rpg/schema/` : les dix familles annoncées — créature,
    objet, arme, armure, sort, espèce, classe, historique, état, type de dégâts — plus
    `common.schema.json`, qui porte ce que toutes réutilisent. Le champ `source` y est obligatoire
    (`EX-CNT-001`, `EX-CNT-002`), et `additionalProperties: false` partout : sans lui,
    `weigthGrams` au lieu de `weightGrams` passe sans un mot et l'arme pèse zéro.
  - **Un triangle, trois artefacts, deux contrôles.** Le moteur (`core::DamageType`), le contrat
    (`common.schema.json`) et la table de traduction (`rpg.glossary.csv`) nomment les mêmes choses.
    `test_rpg_enums.cpp` compare le C++ au schéma **livré** ; `check_rpg_data.py` compare le schéma
    au lexique. La troisième arête n'est **pas** contrôlée, par transitivité : un troisième contrôle
    serait bruyant, et le jour où deux des trois échouent ensemble on ne saurait plus lequel dit
    vrai. Vérifié par injection — ajouter `"sonic"` au seul schéma fait échouer les deux, chacun
    avec son message.
  - **Les trois énumérations fermées** : `core::DamageType` (13), `core::Condition` (15),
    `core::MagicSchool` (8), avec `switch` exhaustif sans `default`, sur le patron de
    `core::tileTypeName`. Leurs cardinaux sont écrits dans un test : le test de coïncidence
    resterait vert si les deux côtés perdaient la même valeur, celui-ci non.
  - **Le validateur s'auto-teste**, faute de données à valider : trois fixtures valides à accepter,
    **dix invalides à refuser**, chacune nommée d'après son défaut — provenance absente, type de
    dégâts en français, `ld8` au lieu de `1d8`, caractéristique manquante, champ mal orthographié,
    donnée provisoire sans critère de retrait, JSON tronqué… Chacune produit **exactement une**
    violation, située au fichier et à la ligne (`EX-CNT-010`).
  - **Deux unités internes uniques** : prix en pièces de **cuivre**, poids en **grammes**, entiers.
    Le corpus mélange « 500 g » et « 2 kg », « 2 pa » et « 25 po » ; convertir à l'entrée évite les
    arrondis là où l'encombrement se calcule par somme.
  - **Le formalisme des dés est contraint par expression régulière** — c'est la parade au risque
    résiduel que le `LOT-30` avait laissé ouvert : un `1d8` devenu `ld8` à l'OCR est invisible à la
    relecture et fatal à l'exécution.
  - `ctest` passe de 943 à **948** cas, tous verts.

- **La chaîne d'extraction du corpus, et le lexique bilingue** (`LOT-30`). Premier lot de la
  filière contenu : l'outillage qui tirera des huit PDF de `Documentation/SourceBook/` les 176
  créatures, l'équipement, les sorts, les espèces et les dix régions du jeu — puis sa première
  sortie, qui l'éprouve.
  - **`scripts/sourcebook/`, sur PyMuPDF** : manifeste des huit documents (empreinte SHA-256,
    pagination, provenance), texte, tableaux **par coordonnée**, images **par rendu clippé**, cache
    indexé par empreinte, et une ligne de commande — `info`, `verifier`, `texte`, `tableau`,
    `image`, `regions`, `stats`, `glossaire`.
  - **La double page est confirmée** : la page PDF 50 des deux livres Tanares porte les pages
    imprimées 100 et 101. Les six autres décalages ont été relevés un par un, et deux ne valent pas
    zéro. Une empreinte qui ne correspond plus **arrête** l'extraction (`EX-CNT-020`) : poursuivre
    produirait des données décalées sans qu'aucun message ne le dise.
  - **Le tableau par coordonnée tient sur un scan.** La table de progression du barbare du
    `Manuel-Des-Joueurs`, en OCR, donne bien « Attaque supplémentaire » au **niveau 5**, là où un
    rendu en flux décale toute la colonne d'un cran (`EX-CNT-021`). C'était l'affirmation la plus
    risquée de l'analyse du corpus.
  - **Le lexique : 2 084 entrées**, et non « environ 1 200 » comme l'annonçait la feuille de route
    — une estimation à l'œil, fausse de 74 %. La page est corrigée.
  - **Le glossaire porte de vrais homonymes**, ce qui a changé la clé d'unicité : `light` vaut
    « légère » comme propriété d'arme et « Lumière » comme sort, `bane` « Fléau / Imprécation »
    comme sort et « Baine » comme divinité. Dédupliquer sur l'anglais seul en écrasait un des deux
    en silence — et faisait traduire un sort par un adjectif d'arme. La clé est le couple
    **(anglais, catégorie)**.
  - **Le complément des *Basic Rules* est attesté, pas saisi.** Les huit écoles de magie, la
    propriété `special` et la quinzième condition (`exhaustion`) manquaient ou n'étaient pas
    catégorisées. Chaque ajout déclare la page où il est attesté, et la construction échoue si le
    terme ne s'y trouve pas — un complément tapé de mémoire est une donnée inventée qui a
    l'apparence d'une donnée extraite. Au passage, le lexique fige un faux ami : l'école
    `conjuration` se dit **« invocation »**.
  - **`scripts/check_glossary.py`, en CI, s'auto-teste avant de se prononcer.** Aucune clé de règle
    n'existe encore : le contrôle serait vert par vacuité, et personne ne saurait s'il fonctionne —
    la panne exacte du `LOT-78`. Six catalogues fictifs le mettent à l'épreuve à chaque appel. La
    comparaison des traductions ignore la casse mais **pas les accents** : une table d'autorité
    française qui accepte « etourdi » pour « étourdi » n'impose plus rien.
  - **Le corpus n'était pas exclu du dépôt**, contrairement à ce que la feuille de route affirmait
    depuis son écriture : la règle `.gitignore` vivait comme modification locale non commitée sur un
    seul poste, et un `git add -A` sur un clone neuf embarquait les 280 Mo. Elle est commitée, avec
    le cache d'extraction et les worktrees d'agent (1,6 Go), et `check_glossary.py` vérifie
    désormais l'exclusion (`EX-CNT-023`).

- **La feuille de route dit enfin par quoi commencer.** Elle portait un « ordre d'exécution
  recommandé » en cinq lignes de *quand* flous — « démarrables maintenant », « avec `LOT-09` »,
  « avant `LOT-13` » — dont aucune ne désignait un premier lot. La question « et maintenant ? » se
  répondait donc par une relecture de 2 500 lignes, et deux relectures ne donnaient pas forcément
  la même réponse.
  - **Une règle, à la place d'un avis** : *à chaque pas, parmi les lots dont tous les prérequis
    sont faits, celui qui en débloque le plus* — à égalité, le plus petit numéro. « Débloque » se
    compte : le `LOT-30` débloque quarante-neuf des soixante-neuf lots restants, le `LOT-49` aucun.
    « Outillage et contrats ; le plus tôt est le mieux » cesse ainsi d'être un avis éditorial pour
    devenir ce que le graphe dit, chiffre à l'appui. Un premier essai avait pris le **plus petit
    numéro** comme critère : déterministe, mais bête — il plaçait deux lots de moteur devant la
    chaîne qui doit leur fournir leurs catalogues.
  - **Un tableau d'avancement en tête de page**, calculé et non écrit : les soixante-neuf lots
    restants dans l'ordre, avec ce que chacun débloque et son statut — `prochain`, `prêt` (tous
    ses prérequis livrés), `en attente`. `scripts/lint_lots.py` gagne une **règle 13** qui le
    recalcule et le refuse s'il diverge d'une ligne. Le tableau ne figure qu'à un endroit : le
    recopier en section 6 aurait recréé les deux documents divergents que cette page combat.
  - **Le regroupement d'intention est conservé** en section 6, sous son propre titre. Il ne donne
    pas l'ordre — le tableau d'en-tête le donne — mais la *raison* de chaque placement, et c'est
    la seule chose qu'un calcul ne produira jamais : un graphe dit qu'un lot en attend un autre,
    il ne dit pas pourquoi on a voulu ce lien.
  - **Le lint ne voyait pas la moitié du graphe.** Les vingt et un lots absorbés (`LOT-09` à
    `LOT-29`) écrivent leurs prérequis dans un bloc de citation, `> Prérequis : …`, quand les lots
    de la filière les écrivent en italique ; le lint ne lisait que la seconde forme. Ni le contrôle
    d'acyclicité ni celui des prérequis existants ne les avait donc jamais examinés.
  - **Ce que le calcul a révélé** : le `LOT-09`, plus petit numéro restant, n'attend que des lots
    livrés à la lecture de sa seule ligne « Prérequis » — mais le `LOT-37` déclare l'alimenter, le
    graphe de cartes attendant l'atlas des régions sans quoi il relierait des nœuds inventés. Il
    tombe au rang 33. Un prérequis compte quel que soit le côté où il est déclaré, et c'est aussi
    ce qui repousse le `LOT-27` au rang 20 : il ne déclare rien, cinq lots de contenu déclarent
    l'alimenter.
  - **Le lien `LOT-38` / `LOT-39` était déclaré à l'envers**, et le calcul l'a rendu visible : la
    plomberie des clés d'assets se disait prérequis de la maquette de fiche, au motif qu'elle lui
    fournirait ses panneaux de parchemin — mais ces panneaux étaient partis au `LOT-76` lors du
    même audit, et la ligne « Prérequis » n'avait pas suivi. L'ordre réel est `LOT-38` puis
    `LOT-39` : on dessine la maquette, *puis* on nomme les clés de ce qu'elle affiche. La section 6
    l'écrivait déjà en toutes lettres ; c'est la ligne « Prérequis », celle que le graphe lit, qui
    disait le contraire.
  - **Conséquence, et elle n'est pas anodine** : le chemin critique jusqu'au *vertical slice* passe
    de cinq à **huit lots**, et la fiche de personnage y entre — `LOT-30` → `LOT-32` → `LOT-43` →
    `LOT-36` → `LOT-13` → `LOT-38` → `LOT-39` → `LOT-27`. Aucune version de cette page ne disait
    que la fiche et sa maquette étaient sur le chemin critique. Le chemin est long parce qu'il est
    réel : le `LOT-39` produit le marqueur généré sans lequel le slice n'a rien à afficher.
  - **`lint_lots.py --regenerer`** réécrit les deux tableaux calculés. Les règles 10 et 13 savaient
    refuser un tableau qui a dérivé ; sans cette option, corriger une ligne « Prérequis » obligeait
    à recopier jusqu'à cinquante lignes à la main — le geste même qui réintroduit l'erreur qu'on
    vient de corriger.
  - **Pas de date pour la `0.1.0`, et la raison est écrite.** La cadence observée — onze lots en
    deux jours — placerait la version dans deux semaines si on l'extrapolait ; les lots livrés sont
    des lots de socle, ceux qui restent portent des catalogues de plusieurs centaines d'entrées.
    L'extrapolation est fausse, et la page le dit plutôt que de laisser le lecteur la faire.

- **Une seule routine de lecture JSON, et des tests paramétrés** (`LOT-79`). Le dépôt comptait
  **six** réimplémentations de `loadFromFile` — `SkinCatalog`, `AnimationCatalog`, `SoundCatalog`,
  `PixelPalette`, `LevelLoader`, `LevelSequenceLoader` — répétant la même séquence (`accept()` puis
  `parse()`, racine objet, version absente valant 1, version supérieure refusée), dont quatre
  redéfinissaient les **mêmes cinq catégories d'échec** sous quatre noms. La filière contenu
  s'apprêtait à en ajouter quinze.
  - `core::JsonDocument` porte l'enveloppe commune une fois pour toutes, et ne lève jamais
    (`EX-NFR-040`). Les six lecteurs y passent ; chacun garde son énumération publique et traduit
    depuis la catégorie partagée par un `switch` **exhaustif et sans `default`**, si bien
    qu'ajouter une catégorie d'un côté fait échouer la compilation.
  - **Un échec dit désormais où.** `nlohmann` ne rapporte qu'un décalage en octets, que les six
    lecteurs jetaient : le message était « JSON malformé », devant un catalogue de mille lignes.
    Il s'annonce maintenant `sounds.json:12:5 : …` (`EX-CNT-010`). C'est la seule différence qui se
    voit à l'usage, et c'est celle qui compte.
  - **Premiers tests paramétrés du dépôt.** `Source/Test/` ne comptait aucun `TEST_P` ni aucun
    parcours de dossier de fixtures. `Source/Test/Fixtures/Json/` en porte six — valide, tronqué,
    virgule en trop, racine tableau, version future, version non entière — et un test vérifie
    qu'aucune fixture du dossier n'est **orpheline** : un fichier qu'aucun test ne lit ne protège
    de rien. C'est la capacité qui compte plus que ces six cas : un bestiaire de 176 créatures se
    teste en balayant un dossier, pas en écrivant 176 `TEST`.
  - `nlohmann_json` devient une dépendance **PUBLIC** de `Core` : `JsonDocument.h` expose l'arbre
    parsé, donc la bibliothèque fait partie de l'interface et non de l'implémentation. L'éviter
    aurait demandé une façade typée par-dessus `nlohmann`, soit un second modèle d'arbre à
    maintenir pour ne rien gagner d'observable.
  - `ctest` passe de 927 à **943** cas, tous verts.

- **Les numéros de lots ne sont plus ambigus** (`LOT-78`). Deux numérotations de lots se
  recouvraient dans les spécifications ; les renvois à l'ancienne ont été distingués des lots de ce
  programme, et `scripts/lint_lots.py` gagne une **règle 12** : tout `LOT-NN` d'une spécification
  doit désigner un lot existant de ce programme. (Le `LOT-88` a depuis retiré l'ancienne
  numérotation.)

- **La moitié RPG de la spécification** (`LOT-77`). Cinq familles d'exigences étaient **fantômes** —
  `EX-CNT`, `EX-REG`, `EX-RPG`, `EX-CBT`, `EX-INV` — citées par une vingtaine de lots sans qu'aucun
  document ne les porte. Cinq documents les portent désormais, pour **69 exigences** :
  `regles-d20.md` (le jet d20, la maîtrise, le temps et le repos, les conditions), `rpg.md` (la
  fiche comme agrégat dérivé, classes et ressources, progression, sorts), `combat.md` (tour, espace,
  attaque, agonie), `inventaire.md` (équipement, encombrement, monnaie) et `contenu.md` (provenance,
  contrats, extraction, ce qu'une donnée promet).
  - **La CI était rouge à dessein** depuis la réparation du lint (`FAMILY_REF_RE` captait enfin les
    références de famille entière, comme `EX-CNT-*`). Ce lot est ce qui la remet au vert, et c'était la
    seule façon légitime de le faire — pas une entrée ajoutée à la liste des exceptions.
    `lint_exigences.py` compte **339 exigences déclarées et 339 référencées**.
  - **Trois généricités décident de la faisabilité du programme**, et sont écrites comme telles :
    une ressource de classe est *une quantité, une cadence, ce qu'elle alimente* (`EX-RPG-021`) —
    rage, ki et second souffle sont la même structure, faute de quoi chacun des quinze lots de
    classes modifierait le code du repos ; ajouter une classe ne touche **aucun** fichier C++
    existant hors sa mécanique propre (`EX-RPG-023`) ; **plusieurs systèmes d'emplacements de sorts
    coexistent** (`EX-RPG-052`), parce que la magie de pacte en fournit un second récupéré au repos
    court, et que coder « les emplacements » au singulier obligerait à tout reprendre.
  - **Le catalogue sera complet avant le moteur**, et cela devait être écrit : une donnée déclare
    les mécanismes qu'elle exige (`EX-CNT-030`) et le moteur **refuse en le disant** ce qu'il ne
    sait pas honorer (`EX-CNT-031`). Une classe dont la ressource propre n'existe pas se signale au
    chargement plutôt que de se jouer en silence comme une classe ordinaire amputée.
  - La rubrique **« Exigences couvertes »** est posée sur les 25 lots qui les implémentent. Elle
    manquait partout, faute de familles à citer.

- **Audit de la feuille de route** (`0.1.0`). `roadmap-0.1.0.md` a été confrontée au dépôt et à
  elle-même : trois affirmations sur le dépôt étaient fausses, six comptes internes incohérents, et
  trois travaux annoncés n'avaient aucun porteur.
  - **Le *vertical slice* n'était plus un jalon précoce** : le `LOT-69` déclarait le Colisée en
    prérequis, ce qui plaçait le socle de classe et les seize classes **devant** le `LOT-27`, à
    rebours de l'argument de la page elle-même. Le `LOT-69` est réduit à la suppression de l'atelier
    pixel art ; l'édition dans la scène devient la cible du `LOT-11`, qui n'est pas commencé.
  - **Quatre fusions** (numéros retirés, jamais réattribués) : le lexique rejoint la chaîne
    d'extraction (`LOT-31` → `LOT-30`), le repos rejoint l'horloge (`LOT-71` → `LOT-70`), l'agonie
    rejoint les conditions (`LOT-73` → `LOT-72`), et le `LOT-48` — « volume long, sans jalon » — est
    dissous dans chaque lot de catalogue : la page écrivait qu'« un lot sans date de fin est un lot
    qu'on ne finit pas », puis en gardait un.
  - **Cinq scissions**, sur une règle unique — le code d'un côté, la donnée de l'autre :
    `LOT-37`/`LOT-80`, `LOT-40`/`LOT-81`, `LOT-41`/`LOT-82`, `LOT-45`/`LOT-83`, `LOT-47`/`LOT-84`.
  - **La collision de numéros est bien plus large qu'estimé** : 208 renvois `LOT-NN` ambigus dans
    **douze** fichiers de spécification, et non « six specs ». D'où le `LOT-78`.
  - `scripts/lint_lots.py` **refuse en CI** ce que l'audit a dû trouver à la main : cycle de
    prérequis, lien déclaré d'un seul côté, lot absent du tableau d'ordre, compte annoncé faux,
    exigence revendiquée par deux lots, tableau récapitulatif périmé, arête de diagramme que rien ne
    déclare.

- **Vocabulaire de terrain du RPG** (`LOT-08`). Neuf types de tuile là où le `LOT-01` avait laissé
  le strict minimum hérité : `Grass`, `Dirt`, `Sand`, `Water`, `DeepWater` (sols), `Wall`, `Cliff`
  (obstacles), `Bridge`, `Stairs` (passages). Il **ajoute** sans rien remplacer — le vocabulaire de
  puzzle du socle sert tel quel au RPG.
  - **La chaîne complète pour chacun**, dans le même lot : nom de format, catégorie de palette,
    libellés `fr` **et** `en`, couleur de repli dans l'atlas procédural, franchissabilité, test
    d'aller-retour. C'est la leçon la plus chère de l'héritage, où ajouter un type touchait
    « exactement la même chaîne de huit fichiers » et où l'un d'eux se faisait toujours oublier.
  - Trois garde-fous nouveaux la tiennent : chaque type a une couleur de repli **non noire et
    distincte** (le jeu affiche une carte sans aucun fichier d'image), chaque libellé de palette
    est **traduit dans les deux catalogues**, et la borne de l'énumération reste dérivée du dernier
    type.
  - **L'eau profonde arrête, l'eau peu profonde non** : c'est la seule distinction qui rende une
    rive jouable. `core::isSolid` est le seul endroit d'où la règle de nage la sortira le jour venu
    — plutôt que des tests « sauf si c'est de l'eau » parsemés dans le code.
  - Aucune **silhouette** n'est déclarée : le mécanisme conservé par le `LOT-01` découpe la matière
    qui n'occupe pas toute la case, ce qui décrivait des pentes et des arrondis. Le terrain d'une
    vue de dessus est carré par nature ; lui inventer des découpes serait du travail contre le
    genre.

- **Tri par profondeur : le monde en vue de dessus devient crédible** (`LOT-07`). Le personnage
  passe **devant** ce qui est au-dessus de lui à l'écran, **derrière** ce qui est en dessous.
  - **Ce que l'epic n'avait pas vu** : alimenter le `sortOrder` existant ne suffisait pas.
    `ComposedScene::sort()` triait par (calque, **texture**, `sortOrder`) — la texture *avant* le
    tri fin — et un personnage n'a jamais la texture d'un arbre : le regroupement écrasait l'ordre
    de profondeur. Pire, `Object` et `Player` étant deux calques distincts, le personnage passait
    **toujours** devant un objet, où qu'il soit.
  - Correctif : `Object` et `Player` forment une **bande de profondeur** commune, à l'intérieur de
    laquelle la profondeur tranche avant la texture (`EX-REN-018`). Le surcoût — des passes de
    dessin supplémentaires — est assumé : aucun ordre de calque ne peut rendre justes à la fois
    « derrière l'arbre du bas » et « devant l'arbre du haut ».
  - La profondeur se lit au **pied** du sprite, pas à son coin haut, et se quantifie au pixel
    (16 sous-divisions par unité) : deux sprites que l'écran ne peut pas départager ne doivent pas
    permuter au gré des arrondis flottants. Le tri restant stable, rien ne scintille.
  - Les tuiles d'une couche de **décor** (`LOT-04`) rejoignent la bande ; le sol reste sous tout le
    monde. `core::buildLevelScene` annonce désormais le **rôle** de la couche d'origine de chaque
    tuile — ce qu'elle *est*, pas son rang dans une liste que l'auteur peut réordonner.
  - **Caméra isotrope** : zone morte carrée (1,5 sur les deux axes, contre 1,5 × 1,0) et
    anticipation **vectorielle**, qui suit la marche sur les deux axes. Anticiper seulement à
    gauche et à droite était un reste du jeu de plateforme.

- **Le jeu est de nouveau jouable : déplacement top-down en 8 directions** (`LOT-06`). Referme la
  parenthèse ouverte par le `LOT-01`, où la physique de plateforme avait été retirée sans
  remplaçant. `core::TopDownMovementSystem` enchaîne intention → vitesse → balayage continu →
  position, **sans gravité** : aucun axe n'est privilégié, `x` et `y` sont traités exactement de la
  même façon.
  - **La diagonale n'est pas plus rapide** (`EX-EXP-001`) : l'intention est normalisée avant d'être
    mise à l'échelle. Sans cela, aller en biais donnerait `√2 ≈ 1,41` fois la vitesse cardinale —
    le défaut le plus courant du genre, et le plus visible en jeu. Vérifié par un test, pas par une
    relecture.
  - Le balayage continu déjà en place (`core::sweepAabb`) fait le reste : aucune traversée de mur à
    vitesse absurde, glissement le long d'un obstacle pris en biais, et vitesse de l'axe bloqué
    remise à zéro — pousser contre un mur n'accumule aucun élan (`EX-EXP-002`, `EX-EXP-003`).
  - `core::Actor` remplace `core::Player` : **deux champs au lieu de trente**. Contact au sol,
    coyote time, jump buffering, dash, wall jump et combos décrivaient un personnage de plateforme
    et étaient **inertes** depuis le `LOT-01`. Restent l'orientation — un **vecteur**, parce qu'on
    regarde dans huit directions et que le sprite (`LOT-08`), l'interaction (`LOT-10`) et l'attaque
    (`LOT-21`) en dépendront — et la masse, seuil des plaques de pression.
  - Conséquences assumées de cette disparition : l'animation choisit son clip d'après la **norme**
    de la vitesse (marcher vers le haut est une marche), le HUD perd ses compteurs de sauts et de
    dashs, et la détection d'événements perd ses transitions de personnage (saut, atterrissage,
    glissade murale) — sans producteur, faute d'un état qui puisse les justifier.

- **Modes de jeu : l'ordre des passes sort de la session** (`LOT-05`). `hmi::GameSession` mêlait
  deux rôles — **orchestrateur** du pas fixe (monde ECS, caméra, événements, HUD, `FixedTimestep`,
  interpolation) et **mode de jeu** (l'ordre des passes lui-même). Tant qu'il n'y avait qu'un genre,
  la confusion ne coûtait rien ; le RPG a besoin d'au moins trois ordres — exploration, dialogue
  (`LOT-15`), combat (`LOT-18`) — qui se seraient entassés en `if` dans une fonction déjà longue.
  - `hmi::IGameModePasses` nomme les onze passes d'un pas fixe ; `hmi::IGameMode` les enchaîne.
    `GameSession` **implémente** les passes (héritage privé : elles sont offertes au mode, pas à
    l'appelant) et son `update()` ne fait plus que déléguer — aucun ordre codé en dur, aucun
    `if (mode == …)`, la sélection est polymorphe (`EX-ARCH-002`).
  - `hmi::ExplorationMode` est le premier mode, extrait **à comportement constant** du corps de
    `GameSession::update`. Aucune fonctionnalité ajoutée : mélanger un refactoring et une nouveauté
    ici aurait rendu indécidable lequel des deux avait cassé quoi.
  - L'interface des passes ne parle que de `core::` — pas de Qt, pas de GPU. C'est ce qui rend un
    mode **testable sans fenêtre**, quand `GameSession` exige un atlas, un lot de sprites et une
    police : le mode se vérifie contre des passes qui **enregistrent** la séquence des appels.
    `passOrder()` en fait une documentation exécutable, comparée à la séquence réelle par un test
    — un ordre modifié sans mettre la liste à jour échoue au lieu de mentir aux diagnostics.

- **Format de carte `version: 3` : couches, entités, propriétés libres** (`LOT-04`). Une carte
  n'est plus une grille plate unique mais **N couches typées** — sol, décor — superposées à la
  grille de collision, plus une **liste d'entités** (PNJ, coffres, panneaux, portails,
  déclencheurs). C'est la seule évolution de format structurante du programme, et elle précède
  toute production de carte : une carte dessinée sur le format plat serait à refaire.
  - **La collision reste le tableau racine `tiles`**, celui qui porte déjà l'entrée, la sortie et
    les mécanismes ; `layers` ne décrit que les couches **visibles**, et une couche `collision`
    déclarée est refusée avec un message qui renvoie à la racine. Deux grilles à tenir d'accord se
    désynchronisent, et c'est celle qu'on ne voit pas qui gagne (`EX-LVL-016`).
  - **Migration ascendante** : une carte `version: 2` se charge sans y toucher, sa grille promue en
    couche unique `legacy` — décor **et** collision, comme avant. Réécrite, elle ressort **sans**
    tableau `layers` : l'éditeur ne convertit pas un fichier dans le dos de son auteur.
  - **Propriétés libres et tolérance aux champs inconnus** (`EX-LVL-018`) — le point à ne pas rater
    du lot. Toute clé qu'une couche ou une entité porte sans que le chargeur la connaisse est
    conservée et **réémise**. Sans cela, les besoins du combat découverts en phase D (terrain
    difficile, couverture, hauteur) imposeraient un `version: 4` en plein milieu du programme, avec
    migration de tout le contenu déjà produit.
  - Le **brouillon d'édition** transporte couches, entités et propriétés sans encore savoir les
    modifier (`LOT-11`), redimensionne toutes les couches avec la carte, et écrit la grille éditée
    dans la couche de collision (`EX-EDIT-011`).
  - `LevelWriter::buildJson` prend désormais l'agrégat `core::LevelData` du `LOT-03` : les couches
    et les entités auraient porté sa liste positionnelle à onze paramètres, dont trois `vector`
    voisins interchangeables sans erreur de compilation.
  - `buildLevelScene` boucle sur les couches visibles et **ignore la collision** : un masque n'est
    pas une image. Le rang de la couche ordonne les sprites, le décor par-dessus le sol.

- **Agrégat `LevelData`** (`LOT-03`). Le constructeur de `core::Level` prenait ses composantes en
  **paramètres positionnels** — jusqu'à 19 avant le `LOT-01`, 11 après lui. Deux
  `std::optional<std::string>` voisins (`background`, `skinSet`) s'intervertissaient sans que le
  compilateur bronche, et le RPG s'apprête à rajouter des champs (couches de tuiles, entités,
  connexions de carte, zones de rencontre). La dette, actée dans l'en-tête lui-même depuis le
  `LOT-69` d'origine, est soldée alors que la liste est au plus court.
  - `core::LevelData` s'écrit avec les *designated initializers* de C++20 : chaque site de
    construction est lisible sans commentaire.
  - **`tileMap` n'a volontairement pas de défaut** : `core::TileMap` n'étant pas constructible par
    défaut, l'omettre est une **erreur de compilation**, jamais une grille vide silencieuse. Tous
    les autres champs ont un défaut utile — un site minimal tient en deux lignes.
  - Le constructeur positionnel est retiré d'emblée, sans l'étape `[[deprecated]]` prévue : dix
    sites d'appel seulement, la béquille coûtait plus qu'elle ne rapportait.

- **Remise à nu du moteur** (`LOT-01`). Le dépôt part d'un moteur 2D complet — ECS, boucle à pas
  fixe, chargeur de niveaux, rendu, éditeur, IHM Qt — dont tout le gameplay propre à la vue de côté
  est retiré, pour devenir un **RPG en vue de dessus** : exploration temps réel, rencontres en
  combat tactique au tour par tour régi par un système d20 maison. Au total **63 000 lignes** et
  376 fichiers retirés ; le personnage ne se déplace plus jusqu'au `LOT-06`.
