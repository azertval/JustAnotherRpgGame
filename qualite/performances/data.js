window.BENCHMARK_DATA = {
  "lastUpdate": 1790390366844,
  "repoUrl": "https://github.com/azertval/JustAnotherRpgGame",
  "entries": {
    "Combat et niveaux (Release, windows-2022)": [
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "0efad242f23214e99fc0bc4bc2c6cc944153d58b",
          "message": "CI — Refonte phase 4 : Qt, données, vitrine (#56)\n\n* ci: refonte phase 4 — Qt, données, vitrine\n\nLa filière contenu et l'interface reçoivent les mêmes garde-fous que le C++.\n\n- QmlTests (Qt Quick Test) : chaque .ui.qml de Jadg.Ui se construit sans\n  avertissement, comportement des briques OrnateButton/OrnateCheck, et les\n  15 écrans comparés à leur capture de référence (rendu logiciel, 960x540).\n- check_translations.py : catalogue .ts achevé et cohérent ; build-ninja\n  relance lupdate (-no-obsolete) pour prouver qu'il est à jour du code.\n  Deux chaînes manquantes traduites, trois entrées mortes retirées.\n- Minidump sur plantage (HMI/Platform/CrashDump) pour le jeu et l'éditeur ;\n  --crash-test éprouvé par le test de fumée des archives, qui n'embarquent\n  plus Logs/ ni Crashes/.\n- pyproject.toml + uv.lock (jsonschema, pytest), scripts/tests sous pytest\n  à la place des étapes --auto-test, rapport JUnit dans test-report.\n- PSScriptAnalyzer (check_powershell.py) ; deux défauts de setup_dev.ps1.\n- Site qualité sur gh-pages (/qualite/) : couverture de main par domaine,\n  performances de la nuit ; coverage.ps1 partagé avec ci.yml.\n- Renovate pour les GIT_TAG de FetchContent, Dependabot pour uv.lock.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* fix(crash-dump): écrire le minidump depuis un thread dédié\n\nEn CI, sous le générateur Visual Studio en Debug, MiniDumpWriteDump\néchouait quand le thread fautif se décrivait lui-même depuis son filtre SEH\n(CrashDumpTest.EcritUnMinidumpAvecLeContexteDUneException) ; Ninja et Release\npassaient. L'écriture a désormais lieu sur un thread que l'appelant attend,\ncomme le recommande la documentation de dbghelp, et GetLastError en donne la\nraison en cas d'échec.\n\n--crash-test lève EXCEPTION_ACCESS_VIOLATION au lieu de déréférencer un\npointeur nul : même code de sortie et même chemin par le filtre, sans le\ncomportement indéfini que cppcheck signalait (Code scanning).\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* fix(crash-dump): dump réduit si le dump riche échoue\n\nSur le runner, MiniDumpWriteDump avec contexte d'exception échoue de façon\nintermittente en 0x800706F8 (ERROR_INVALID_USER_BUFFER), sous Ninja comme\nsous Visual Studio, jamais sur le poste (300 répétitions). Cause la plus\nprobable : le balayage de la mémoire référencée par les piles lit des pages\nqui changent avant l'écriture. Second essai sans ce balayage, qui garde piles\net contexte ; le test exige désormais un flux d'exception dans le dump.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* fix(crash-dump): replis sur un contexte sans état étendu\n\nLe dump réduit échouait lui aussi sur le runner (0x800706F8) : ce n'est pas\nle balayage mémoire mais le contexte d'exception d'origine que dbghelp refuse,\nsur une partie des runners seulement (poste : i7-8700 sans AVX-512).\nHypothèse retenue : l'état étendu du processeur (CONTEXT_XSTATE).\n\nTrois essais : dump riche avec le contexte d'origine, dump réduit avec une\ncopie limitée au CONTEXT de base, dump réduit avec ClientPointers. Chaque repli\néprouvé seul sur le poste : dump valide avec flux d'exception.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-15T20:53:45Z",
          "url": "https://github.com/azertval/JustAnotherDnDGame/commit/0efad242f23214e99fc0bc4bc2c6cc944153d58b"
        },
        "date": 1789526332740,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 158352.86830356842,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 160435.26785714287 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 216280.85937500428,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 217285.15625 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 56996.785728634626,
            "unit": "ns/iter",
            "extra": "iterations: 24889\ncpu: 57128.65121137852 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 161408.54511970468,
            "unit": "ns/iter",
            "extra": "iterations: 8145\ncpu: 161141.80478821363 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1107256.718750005,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1098632.8125 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 907769.5914266576,
            "unit": "ns/iter",
            "extra": "iterations: 1493\ncpu: 900033.4896182184 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "d7792a077071db467f10679376a39fbaa20e295f",
          "message": "Analyse statique — corriger les 896 alertes clang-tidy de Code scanning (#63)\n\n* clang-tidy : corrections mécaniques des alertes Code scanning\n\nFixits clang-tidy appliqués sur les 100 fichiers signalés : suffixes littéraux en majuscules,\nparenthèses des expressions mixtes, initialiseurs désignés, auto, std::ranges, emplace,\nparamètres passés par référence constante, et autres réécritures sans effet de comportement.\n\nFileLogSink : std::endl remplacé par '\\n' suivi d'un flush explicite, pour garder l'écriture\nimmédiate sur disque que les tests exigent.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* clang-tidy : corrections manuelles des alertes Code scanning restantes\n\n- Complexité cognitive : fonctions découpées en étapes nommées (main, LevelWriter::buildJson,\n  CharacterSheet, Dialogue, EnemyAi, MainWindow::buildUi, GameViewport::tick…).\n- Défauts réels : cast élargissant après la multiplication (AssetMarker, LineOfSight), arrondi\n  par std::lround (CharacterSheetValues), compteurs flottants de PixelCanvas, garde nullptr dans\n  MainWindow::raisePanel, cas Key::E manquant dans KeyName.\n- Champs d'agrégats explicites, std::array, unions DirectXMath lues par l'API, qobject_cast,\n  constantes inutilisées supprimées, nommage aligné sur .clang-tidy.\n- JADG_ASSERT n'enveloppe plus la condition dans une négation.\n- 9 NOLINT justifiés : drapeaux MINIDUMP_TYPE, balayage des codes de touche, méthodes lues par\n  QML, reinterpret_cast imposé par QImage.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* CHANGELOG : alertes clang-tidy corrigées\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* MechanismController : documenter les paramètres de setDoorOpen (Doxygen)\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-17T00:18:16Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/d7792a077071db467f10679376a39fbaa20e295f"
        },
        "date": 1789612715208,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 130823.49337082541,
            "unit": "ns/iter",
            "extra": "iterations: 9956\ncpu: 130260.64684612294 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 177600.65622069975,
            "unit": "ns/iter",
            "extra": "iterations: 7467\ncpu: 177865.94348466586 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 50729.56688272962,
            "unit": "ns/iter",
            "extra": "iterations: 27152\ncpu: 50640.83677077195 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 147044.3350743485,
            "unit": "ns/iter",
            "extra": "iterations: 9956\ncpu: 147524.10606669346 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 909435.8576052382,
            "unit": "ns/iter",
            "extra": "iterations: 1545\ncpu: 920307.4433656958 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 619398.320537539,
            "unit": "ns/iter",
            "extra": "iterations: 2084\ncpu: 622300.8637236084 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "7821d11b506195e3fa8fca90c7bf7aa9514270c8",
          "message": "Analyse statique — corriger les 113 alertes restantes de Code scanning (#64)\n\n* Analyse statique — corriger les 113 alertes restantes de Code scanning\n\nL'analyse clang-tidy de main (d7792a077) relevait 112 alertes, et CppCheck une, dans les\nfichiers arrivés avec l'éditeur (LOT-11) et la galerie des assets : ils n'étaient pas dans le\npérimètre de la PR #63.\n\n- Correctifs mécaniques : suffixes littéraux, parenthèses, initialiseurs désignés complets,\n  std::ranges, std::cmp_*.\n- Complexité cognitive : validateMapEntities, EntityPanel::rebuildForm,\n  WorldGraphView::paintNodes et AssetGalleryRenderer::compose découpées en fonctions.\n- Transtypages : qobject_cast dans le rendu de la galerie (comme l'arène), dynamic_cast pour\n  l'infobulle du graphe ; tableau C de couleur d'effacement remplacé par std::array.\n- Analyseur : le nom d'un point d'arrivée n'est plus déplacé dans la boucle ; LevelWriter\n  n'emploie plus std::visit, que l'analyseur ne suivait pas.\n- CppCheck : le catalogue des dialogues est tenu dans une variable avant d'être parcouru.\n- Deux NOLINT justifiés en ligne : reinterpret_cast de lecture d'en-tête PNG, et _form créé\n  après setupUi.\n\nclang-tidy en local : 0 diagnostic sur les 13 fichiers ; build et 1289 tests verts.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* Changelog — les 113 alertes restantes de Code scanning\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-17T07:05:54Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/7821d11b506195e3fa8fca90c7bf7aa9514270c8"
        },
        "date": 1789662656973,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 167720.66298341966,
            "unit": "ns/iter",
            "extra": "iterations: 8145\ncpu: 168815.22406384285 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 227812.90808637897,
            "unit": "ns/iter",
            "extra": "iterations: 5973\ncpu: 227586.63987945757 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 65373.469387753314,
            "unit": "ns/iter",
            "extra": "iterations: 21854\ncpu: 65777.4320490528 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 189187.81304405027,
            "unit": "ns/iter",
            "extra": "iterations: 7467\ncpu: 188328.64604258738 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1185325.9821428708,
            "unit": "ns/iter",
            "extra": "iterations: 1120\ncpu: 1185825.892857143 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 771829.0625000179,
            "unit": "ns/iter",
            "extra": "iterations: 1600\ncpu: 771484.375 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "e32d883e10d80454ca5de7f37787c4fec4790890",
          "message": "LOT-09 — Le Colisée se parcourt : exploration dans le jeu et première carte (#67)\n\n* LOT-09 — Ouverture : le lot quitte la feuille de route pour son dossier\n\nLe Colisée en version finale est la première carte du jeu ; le lot porte\nl'exploration dans le jeu Qt Quick, le graphe de cartes jouable, le sable\ncomme zone de combat déclarée et le retrait de tout le contenu provisoire.\n\n- epic : l'état du dépôt à l'ouverture (WorldGraph existe déjà, la plomberie\n  QRhi du jeu est posée mais ne dessine rien, GameSession ne compile que dans\n  l'éditeur, les 36 textures du LOT-92 sont installées), les deux décisions\n  d'ouverture (carte posée par script puis retouchée dans l'éditeur ; une seule\n  PR en fin de lot) et les six phases ;\n- feuille de route : la section 11 du LOT-09 part pour cette page, les deux\n  tableaux de la section 6 sont régénérés et le diagramme perd les arêtes\n  entrantes du lot, comme pour tout lot sorti de la page.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 phase 1 — Le graphe du monde se joue : traverser un portail, arriver a un point nomme\n\n`core::WorldGraph` lisait le dossier des niveaux et disait ce que valait chaque\nportail ; il se joue desormais.\n\n- `core::WorldTravel` (Source/Core/World/) : la carte courante, l'entree par un\n  point d'arrivee NOMME, et la traversee d'un portail. Le chargement est injecte\n  (`MapLoader`, `directoryLoader` pour le jeu) : les tests decrivent leurs cartes\n  en memoire, et Core ne connait toujours qu'un dossier qu'on lui nomme.\n- Une carte deja visitee n'est PAS rechargee : revenir du sable doit rendre le\n  lieu tel qu'on l'a laisse. Le compteur de lectures du dossier de test le prouve.\n- `validateWorldGraph` traduit chaque statut de portail en defaut situe et cite\n  (EX-NFR-040) ; `validateWorldMap` releve ce que le graphe ne peut pas voir, le\n  point d'arrivee en double, que la deduplication du graphe efface.\n- Le portail peut exiger un drapeau de monde (`requiresFlag`, propriete de la\n  famille `portal`, traduite dans les deux catalogues) : lu ici, pose au LOT-16.\n- Six tests unitaires, dont le parcours de cinq cartes de fixture aller et\n  retour ; 1277 tests verts (1 ignore, la capture d'arene).\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 phase 2 (1/2) — La session d'exploration et la composition d'un lieu\n\nDecision de l'auteur (17 septembre 2026) : la feuille de route demandait de\ncompiler `hmi::GameSession` dans le jeu, ce qui y aurait amene un SECOND moteur\nde rendu (atlas de tuiles carrees, HUD bitmap) alors que le meme paragraphe\ndemande le composeur de l'arene, donc l'iso du LOT-92. Le lieu et l'arene se\ndessinent par le meme code ; ils se simulent par deux sessions.\n\n- `core::ExplorationSession` : le heros marche sur la carte (position continue\n  en cases, collision sur la grille racine, glissement le long d'un mur pris en\n  biais), franchit le portail de la case ou il ARRIVE, et parle a ce qu'il\n  regarde (`findInteractionTarget` + `dialogueTriggerFor`). Gelee, elle ne bouge\n  plus : c'est l'etat de la carte pendant un dialogue ou un combat.\n- `hmi::PlaceAppearance` : la table d'un lieu, type de tuile -> pieces de la\n  planche de l'atelier. Le sol vient du type (variante decidee par la case,\n  jamais par un tirage ni par l'ordre de parcours) ; le relief nomme sa piece a\n  la case (`core::TileTextureOverride`). Aucun changement de format de niveau\n  (decision de l'auteur).\n- `hmi::WorldSceneComposer` : le jumeau du composeur de l'arene pour une carte.\n  Meme projection, memes planches, meme tri par profondeur ; sol sur Tile,\n  relief sur Object, figurines sur Player, par instantane en valeurs.\n- `hmi::ScenePieces.h` : la geometrie des planches de l'atelier (losange 68 x 42,\n  images 48 x 64, echelle 1,25) vit desormais une seule fois ; les constantes\n  `ARENA_*` la designent.\n- 7 tests de plus, 1284 verts (1 ignore, la capture d'arene).\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 phase 2 (2/2) — Le lieu se dessine dans le jeu : WorldViewport et la camera qui suit\n\n- `hmi::WorldSceneRenderer` : le rendu QRhi d'un lieu, jumeau d'`ArenaSceneRenderer`\n  (memes trois temps, meme ordre de liberation). Une difference, et une seule :\n  les textures d'un lieu ne sont pas connues d'avance -- la carte change au\n  passage d'un portail --, donc elles se chargent a la demande, et un chemin\n  absent n'est tente qu'une fois.\n- `worldCamera` : la camera SUIT le heros et ne sort pas de la carte\n  (EX-LVL-006) ; l'echelle est un nombre de losanges en largeur, pas le cadrage\n  entier de l'arene -- le Colisee ne tient pas dans un ecran.\n- `hmi::WorldModel` : la vue-modele de l'exploration. Elle fait tourner le pas\n  fixe (16 ms), publie la case continue du heros, et emet ce que l'ecran doit\n  jouer (carte entree, dialogue, rencontre, portail verrouille ou casse). La\n  surface de rendu n'obtient qu'un instantane en valeurs.\n- `hmi::WorldViewportItem` (QML `WorldViewport`) : la surface, et le cadrage\n  publie (`tileWidth`, `originX`, `cellAt`, `pointAt`) -- jamais un recalcul en\n  QML.\n- `GameView.qml` quitte `pending` : il tient la session, la dessine, et se joue\n  aux fleches ou a ZQSD, `E` pour parler. Le deplacement est un ETAT de touches,\n  pas une suite de pas.\n- Un PNJ nomme sa figurine de l'atelier (`figure`, LOT-91) ; doublures de\n  conception et allowlist de couches mises a jour (check_ui_layers,\n  check_qml_designer_compat verts).\n\nVerifie a l'ecran : `--screen=GameView` ouvre la vue, cree ses ressources QRhi\nD3D11, et dit que `coliseum.json` manque -- c'est la carte de la phase 3.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 phase 3 — La carte du Colisee, version finale : le sable au centre, un lieu autour\n\nLe Colisee cesse d'etre une grille nue. 40 x 34 cases, 634 franchissables : le\nsable du LOT-50 (20 x 14) au centre, l'enceinte et ses quatre portes, deux\ncouloirs sous les gradins, deux vestiaires, les tribunes du nord et du sud, la\nloge imperiale, le grand escalier, le hall et la porte.\n\n- `Source/Elements/Levels/coliseum.json`, pose par un script d'atelier versionne\n  dans le lot (`atelier/carte_colisee.py`, `--check` compare le commite a ce que\n  le script produit) puis retouchable dans l'editeur : le fichier est un niveau\n  ordinaire, sans marque d'origine.\n- `Assets/Scene/coliseum/appearance.json` : la table du lieu. Le SOL vient du\n  type de tuile de la couche sol -- le type est une fente de matiere, et son nom\n  est le vocabulaire du LOT-08 que le LOT-88 retirera ; le RELIEF nomme sa piece\n  a la case. 31 des 36 pieces de l'atelier sont posees.\n- Cinq PNJ : le heraut sur le sable, le portier au hall, la parieuse aux\n  tribunes, le medecin au vestiaire, le vieux gladiateur sur les gradins --\n  chacun sa figurine de l'atelier (LOT-91) et son dialogue, quatre dialogues\n  neufs et leurs 28 cles de traduction dans les deux catalogues.\n- Six tests de CONTENU (test_coliseum_map.cpp) : la carte se charge, le graphe\n  livre ne porte aucun defaut, chaque dialogue et chaque figurine nommes\n  existent, chaque piece assignee existe sur la planche, les six lieux\n  s'atteignent depuis la porte, et les huit points d'entree sont dans le sable.\n- L'ecran : `worldCamera` agrandit d'un facteur ENTIER (le pixel art se brouille\n  a l'echelle 0,62) et borne le suivi a la scene ; le vide autour du lieu est la\n  nuit, pas du parchemin.\n\nVerifie a l'ecran : `--screen=GameView --window-size=1920x1080 --screenshot=...`\nmontre le hall, les gradins, les bancs, les bannieres, les arches, le sable et\ntrois figurines. 1290 tests verts (1 ignore).\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 phase 3 — Fichiers de donnees en LF, comme tout le depot\n\nLe script d'atelier ecrivait la carte avec le saut de ligne du poste : un fichier\nde donnees commite en CRLF pollue chaque diff. La carte et les quatre dialogues\nsont remis en LF.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 phase 4 — Le sable est la zone de combat declaree, et l'arene n'y joue que dessus\n\nLe Colisee est un lieu : on marche dans le hall, les couloirs, les vestiaires et\nles tribunes, et l'on ne s'y bat pas. Prendre la carte entiere pour grille\ntactique aurait donne un affrontement de 1 360 cases dont mille de gradins.\n\n- `core::CombatZone` (EX-LVL-018) : le rectangle NOMME ou l'on se bat, declare\n  sur la carte comme une entite que l'editeur pose (`combatZone`, nom, largeur,\n  hauteur). `cropLevelToZone` rend la carte reduite a la zone -- grille, couches\n  visibles, entites et assignations de texture translatees, cadrage remis a\n  `wholeLevel` --, et les cases du dehors sont donc INCONNUES de la session :\n  une creature ne peut pas marcher du sable jusqu'aux tribunes.\n- `validateCombatZones` refuse au CHARGEMENT une zone degeneree, debordante ou\n  entierement pleine (EX-NFR-040) ; `validateWorldMap` les releve avec les\n  autres defauts de carte.\n- La carte du Colisee declare sa zone « sable » (20 x 14 en (10, 10)).\n- Le catalogue des arenes : `arena-of-the-future` designe desormais\n  `coliseum.json`, sa zone `sable` et son LIEU (le quartier d'Arenarea de la\n  Capitale, nouveau champ `location` du schema, a cote de la region) ;\n  `ArenaModel` reduit la carte a la zone avant de monter la session.\n- `arena-of-the-future.json` part : la piste nue du LOT-50 n'a plus d'objet. Le\n  banc d'essai, le test des catalogues de l'editeur et le README du dossier des\n  niveaux lisent la carte du Colisee.\n- Trois tests de zone, 1293 tests verts (1 ignore) ; toute la batterie de lints\n  verte.\n\nVerifie a l'ecran : `--screen=Arena` joue sur le sable du Colisee, 20 x 14.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 phase 5 — Parler au PNJ vise, aller sur le sable, et revenir au meme endroit\n\n- L'ecran de dialogue n'a plus d'identifiant ecrit en dur : la carte nomme le\n  dialogue du PNJ vise, le routeur le transporte (`ScreenRouter.openDialogue`,\n  propriete `dialogueId`), l'ecran le joue.\n- Un dialogue peut ENVOYER SE BATTRE : nouvelle action `startCombat` (Core,\n  schema, journal du runner) que le heraut du Colisee porte desormais -- et il\n  perd sa marque « provisoire » : ce n'est plus un dialogue de demonstration\n  mais un PNJ de la premiere carte. Le dialogue ne sait pas ce qu'est un combat,\n  il le DEMANDE (`core::DialogueListener::startCombat`), et l'ecran ouvre le\n  Colisee.\n- La table des ecrans : le Colisee s'ouvre desormais depuis la CARTE, et\n  `CloseArena` revient d'ou l'on vient (`arenaReturnTo`) -- au menu si l'on est\n  venu du menu, sur la carte si le heraut y a envoye.\n- **La session d'exploration est un singleton** : c'est LA PARTIE, pas un objet\n  de l'ecran. La pile d'ecrans ne garde qu'un ecran vivant ; une session\n  possedee par l'ecran mourrait a l'ouverture du dialogue ou du sable, et l'on\n  reviendrait sur une carte neuve, heros a la porte -- ce que le lot interdit.\n- La carte gele quand un ecran la recouvre et reprend quand elle retrouve le\n  focus ; un fondu leve la carte a l'entree, au passage d'un portail et au\n  retour du sable.\n- « Nouvelle partie » ouvre la vue de jeu sur le Colisee, et non plus l'arene :\n  le sable ne se joue plus depuis le menu, c'est le heraut qui y envoie.\n- 1294 tests verts (1 ignore), dont l'aller-retour carte <-> sable de la table\n  des ecrans ; doublures de conception et lints a jour.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 phase 6 — Les retraits, et la capture de reference du Colisee\n\n- Le niveau nu de l'arene etait parti a la phase 4 ; partent ici les six fonds de\n  test (`Assets/Backgrounds/test_*.png`) et `generate_test_backgrounds.py` -- ils\n  servaient a verifier le recadrage de fond a l'oeil dans l'editeur, et la\n  premiere carte du jeu est une scene isometrique qui ne designe aucun fond (le\n  calcul reste couvert par ses tests, qui n'ont jamais lu d'image).\n- La rencontre de demonstration du LOT-18 (`nuee-de-rats.json`) devient LE\n  CONTENU du lieu : `colisee-fauves.json`, les fauves du Colisee, sur les betes\n  du LOT-33. Elle n'est plus marquee provisoire, et les quatre tests qui la\n  nommaient la suivent.\n- La capture de reference : `test_world_scene_renderer.cpp`, jumeau du test de\n  l'arene -- cycle de vie des ressources sur un vrai QRhi hors ecran, chargement\n  a la demande des pieces de la carte (et une seule fois), le Colisee LIVRE\n  dessine sans une seule piece tombee sur le damier, et le cadrage qui suit le\n  heros a un agrandissement entier.\n- Trois retraits demandes par la feuille de route n'ont PAS ete faits, et l'epic\n  dit pourquoi : le personnage de demonstration (aucune creation de personnage\n  n'existe encore dans le jeu ; le retirer viderait la fiche, l'inventaire,\n  l'arene et l'interlocuteur des dialogues), la planche source du LOT-50 (c'est\n  la SOURCE des pieces que l'arene dessine, comme les planches de l'atelier) et\n  `GameViewportItem` (le HUD de combat le pose encore).\n- Documentation : l'epic porte le bilan livre/ecarte et le journal des six\n  phases ; la feuille de route et `lots.md` marquent le lot livre ; le CHANGELOG\n  a son entree ; le cahier de test est regenere (1 311 cas).\n\n1 317/1 317 tests verts (ctest, unitaire + integration + systeme).\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 — Documenter le parametre manquant de standingPieceQuad (Doxygen WARN_AS_ERROR)\n\nLe controle de documentation de la CI traite les avertissements comme des\nerreurs : le premier parametre du poseur de piece n'etait pas documente.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-09 — Traduire le message de carte illisible (catalogue Qt)\n\nLa CI l'a attrape : le message que la vue de jeu affiche quand la carte ne\ns'ouvre pas passe par tr(), donc par le catalogue Qt, qui exige une traduction\nACHEVEE. Catalogue remis a jour du code (lupdate) : la chaine de la vue de jeu\nqui a disparu s'en va, la nouvelle est traduite.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-17T21:47:57Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/e32d883e10d80454ca5de7f37787c4fec4790890"
        },
        "date": 1789699173874,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 157095.9040178518,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 156947.54464285713 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 211820.87499999724,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 209960.9375 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57509.62296959313,
            "unit": "ns/iter",
            "extra": "iterations: 23579\ncpu: 56989.27011323635 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 166593.49294045253,
            "unit": "ns/iter",
            "extra": "iterations: 8145\ncpu: 166896.86924493554 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1135662.187499964,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1135253.90625 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 1688721.1042944589,
            "unit": "ns/iter",
            "extra": "iterations: 815\ncpu: 1687116.5644171778 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "e89301c25f5864a0b9a979a588c2f95ee8cfaef6",
          "message": "LOT-EDITOR-02 — Le canevas montre le lieu, en iso comme dans le jeu (#76)\n\n* refactor(editeur): LOT-EDITOR-02 — la composition sort de HmiLib, le manifeste des pièces descend dans Core\n\nPhase 1 du lot (constats A8 et A9 de la feuille de route de l'éditeur).\n\n- SceneComposition : ComposedScene, PlaceAppearance et WorldSceneComposer forment\n  une bibliothèque sans GPU ni Qt, que HmiLib lie en PUBLIC ; le jeu ne change pas.\n- WorldSceneSource : la composition lit une core::Level ou un brouillon d'éditeur\n  par les mêmes accesseurs ; npcFigures sort de WorldPlay pour être partagé.\n- core::ScenePieceManifest lit Assets/Scene/<lieu>/manifest.json (classe, emprise,\n  ancre, taille, miroir) ; la galerie des assets le lit par là.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* feat(editeur): LOT-EDITOR-02 — le canevas montre le lieu, en iso comme dans le jeu\n\nPhases 2 et 3 du lot (décisions D1, D2, D11 de la feuille de route de l'éditeur).\n\n- EditorViewport devient une QGraphicsView dont l'élément unique peint, par\n  QPainter, la liste de primitives que compose le jeu, bornée au visible ;\n  quadrillage en losanges, case survolée, aperçu des outils, marqueurs, terrain\n  de rencontre et masque de collision par-dessus. Vue à plat en bascule (F9).\n- ScenePainter : remplissage texturé échantillonné au centre des pixels, comme le\n  GPU ; ScenePainterTest compare Martpart et le Colisée au rendu QRhi du jeu\n  (0,06 % des pixels au pire, tolérance 0,5 %).\n- Pointage par le losange, hauteur en paramètre (CanvasPicking) ; instantané du\n  canevas identique à celui du jeu (CanvasScene), testés.\n- Calques grisés et verrouillés, reliefs en transparence (F8), mini-carte,\n  pièces de la case survolée dans la barre d'état.\n- L'essai immédiat est peint de même : l'éditeur ne lie plus ni SpriteBatch ni\n  QRhi ni Qt6::GuiPrivate. Mesure ComposeMartpart : 0,5 ms.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(editeur): LOT-EDITOR-02 — dossier du lot, exigences, guides et README\n\n- Documentation/Editeur/LOT-EDITOR-02-canevas/epic.md : décisions, livraison,\n  acceptation, ce qui reste hors du lot ; le lot quitte la feuille de route.\n- editeur-niveaux.md : EX-EDIT-059 (le canevas montre le lieu comme le jeu),\n  EX-EDIT-060 (pointage par le losange), EX-EDIT-061 (calques, mini-carte).\n- EX-REN-050, guides éditeur, rendu, IHM Qt et boucle : le canevas est une\n  QGraphicsView peinte par QPainter ; cahier de test régénéré.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(editeur): LOT-EDITOR-02 — entrée du changelog\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* fix(editeur): LOT-EDITOR-02 — CI : deux erreurs clang-tidy et trois paramètres non documentés\n\n- CanvasPicking.cpp : la borne de `cellIndex` divise en flottant (bugprone-integer-division).\n- SceneImages.cpp : le pas de ligne se calcule en `qsizetype` (implicit-widening).\n- CanvasPicking.h, ScenePainter.h : paramètres Doxygen complétés (WARN_AS_ERROR).\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-18T21:45:17Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/e89301c25f5864a0b9a979a588c2f95ee8cfaef6"
        },
        "date": 1789785426293,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 155707.55580357,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 155203.6830357143 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 212740.00000000015,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 212402.34375 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57417.955071027,
            "unit": "ns/iter",
            "extra": "iterations: 24216\ncpu: 58071.1100099108 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 162747.968750002,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 162179.1294642857 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1135160.6250000088,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1123046.875 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 1667925.5689424297,
            "unit": "ns/iter",
            "extra": "iterations: 747\ncpu: 1673360.107095047 ns\nthreads: 1"
          },
          {
            "name": "ComposeMartpart",
            "value": 408.2537144515445,
            "unit": "us/iter",
            "extra": "iterations: 3446\ncpu: 403.5475914103308 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "Valentin Eloy",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "Valentin Eloy",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "id": "a7a1bd50822a2d7332d00c7cc8bfd9ba32e0c4f7",
          "message": "fix(hmi): skip unreadable memory when writing a minidump\n\nThe Nightly shuffle job failed on CrashDumpTest.EcritUnMinidumpAvecLeContexteDUneException\nwith ERROR_PARTIAL_COPY on every repeat (run 35416051895): dbghelp cancels the whole dump\nwhen a memory read fails, typically the stack of a thread exiting while it is being read.\nA MiniDumpWriteDump callback now answers ReadMemoryFailureCallback with S_OK, so the\nunreadable region is skipped and the dump is written.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-19T07:48:26Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/a7a1bd50822a2d7332d00c7cc8bfd9ba32e0c4f7"
        },
        "date": 1789804285969,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 130155.43750000802,
            "unit": "ns/iter",
            "extra": "iterations: 11200\ncpu: 129743.30357142857 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 176519.54573358322,
            "unit": "ns/iter",
            "extra": "iterations: 8145\ncpu: 176488.64333947207 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 50919.592857143565,
            "unit": "ns/iter",
            "extra": "iterations: 28000\ncpu: 50781.25 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 143435.70312499919,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 142996.6517857143 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 896577.3535220468,
            "unit": "ns/iter",
            "extra": "iterations: 1519\ncpu: 894914.4173798552 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 1260119.377509984,
            "unit": "ns/iter",
            "extra": "iterations: 996\ncpu: 1270707.8313253012 ns\nthreads: 1"
          },
          {
            "name": "ComposeMartpart",
            "value": 321.9763065385605,
            "unit": "us/iter",
            "extra": "iterations: 4267\ncpu: 322.2404499648465 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "376c541da2dab816d4b4e9bdc1cb9d3093d9b374",
          "message": "fix(hmi): un minidump saute la mémoire illisible au lieu d'échouer (#78)\n\n* fix(hmi): skip unreadable memory when writing a minidump\n\nThe Nightly shuffle job failed on CrashDumpTest.EcritUnMinidumpAvecLeContexteDUneException\nwith ERROR_PARTIAL_COPY on every repeat (run 35416051895): dbghelp cancels the whole dump\nwhen a memory read fails, typically the stack of a thread exiting while it is being read.\nA MiniDumpWriteDump callback now answers ReadMemoryFailureCallback with S_OK, so the\nunreadable region is skipped and the dump is written.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(changelog): record the minidump partial-copy fix\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-19T08:31:59Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/376c541da2dab816d4b4e9bdc1cb9d3093d9b374"
        },
        "date": 1789806898794,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 157455.3236607125,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 156947.54464285713 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 214656.70312499796,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 214843.75 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57278.662867524996,
            "unit": "ns/iter",
            "extra": "iterations: 24216\ncpu: 57425.87545424513 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 161546.8080357095,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 162179.1294642857 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1140405.9374999774,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1135253.90625 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 1690147.3895582648,
            "unit": "ns/iter",
            "extra": "iterations: 747\ncpu: 1673360.107095047 ns\nthreads: 1"
          },
          {
            "name": "ComposeMartpart",
            "value": 407.82182240278246,
            "unit": "us/iter",
            "extra": "iterations: 3446\ncpu: 403.5475914103308 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "7139a1d68fec97bc79bd7860ee3c21fe1e2c2201",
          "message": "fix(ci): le job links de la Nightly ignore les renvois vers le corpus (#80)\n\nLes préparations régionales de la Fabrique d'assets (#77) renvoient aux PDF\nde Documentation/SourceBook/, que le .gitignore garde hors du dépôt : 86 liens\nintrouvables sur le runner, lychee sort en code 2 depuis la nuit du 19 septembre.\nCes cibles sont exclues ; les documents qui les portent restent vérifiés.\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-19T10:11:14Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/7139a1d68fec97bc79bd7860ee3c21fe1e2c2201"
        },
        "date": 1789812850692,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 155998.95089285655,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 155203.6830357143 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 211793.03124999825,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 209960.9375 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57703.03518334913,
            "unit": "ns/iter",
            "extra": "iterations: 24216\ncpu: 58071.1100099108 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 168018.0133928566,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 169154.57589285713 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1133640.7031250051,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1135253.90625 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 2411888.4250474563,
            "unit": "ns/iter",
            "extra": "iterations: 527\ncpu: 2431214.4212523717 ns\nthreads: 1"
          },
          {
            "name": "ComposeMartpart",
            "value": 462.54536324070654,
            "unit": "us/iter",
            "extra": "iterations: 2987\ncpu: 465.5590893873452 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "46a8b09a56ea4dccddd73856b3b0620471d3d7c8",
          "message": "feat(arena): integrate Arena of Brave coliseum interface (#88)",
          "timestamp": "2026-09-19T22:35:13Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/46a8b09a56ea4dccddd73856b3b0620471d3d7c8"
        },
        "date": 1789871925110,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 156224.73214285748,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 156947.54464285713 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 211536.18750000547,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 209960.9375 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57044.248463175376,
            "unit": "ns/iter",
            "extra": "iterations: 24889\ncpu: 56500.86383542931 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 161360.59151785937,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 160435.26785714287 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1105237.4218750051,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1110839.84375 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 2635781.9915254163,
            "unit": "ns/iter",
            "extra": "iterations: 472\ncpu: 2615201.2711864407 ns\nthreads: 1"
          },
          {
            "name": "ComposeMartpart",
            "value": 469.45196428570824,
            "unit": "us/iter",
            "extra": "iterations: 2800\ncpu: 468.75 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "f6fbbbb2c7e08d3d2f5cfe822245597b804822fb",
          "message": "fix(ci): restore scripts/ and stop ignoring it (#93)\n\n* fix(ci): restore scripts/ and stop ignoring it\n\nCo-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>\n\n* fix(docs): drop Doxygen inputs and links to the removed Tools/AssetFactory\n\nCo-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Sonnet 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-20T13:14:53Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/f6fbbbb2c7e08d3d2f5cfe822245597b804822fb"
        },
        "date": 1789910268716,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 155501.77455357125,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 155203.6830357143 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 213549.53569355668,
            "unit": "ns/iter",
            "extra": "iterations: 6892\ncpu: 215376.52350551364 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57070.61754188496,
            "unit": "ns/iter",
            "extra": "iterations: 24889\ncpu: 57128.65121137852 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 160636.72991070937,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 160435.26785714287 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1163730.9821428538,
            "unit": "ns/iter",
            "extra": "iterations: 1120\ncpu: 1157924.107142857 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 2623663.5593220815,
            "unit": "ns/iter",
            "extra": "iterations: 472\ncpu: 2615201.2711864407 ns\nthreads: 1"
          },
          {
            "name": "ComposeMartpart",
            "value": 468.6737529293563,
            "unit": "us/iter",
            "extra": "iterations: 2987\ncpu: 470.79009039169733 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "ab39ebb86bc43f1af39501d7fa9b413446775d10",
          "message": "LOT-101 — Le standard 2D HD devient normatif (#95)\n\n* docs(standard): le standard 2D HD devient normatif (LOT-101)\n\nLa maquette de validation monte huit cases sur huit d'Arenarea a l'echelle du\nstandard, depuis la planche de reference, et la cadre aux deux definitions ou le\njeu se joue. Elle est reproductible (scripts/build_hd_mockup.py --check) et\ndevient la reference de non-regression du rendu HD du LOT-103.\n\nTrois exigences sont reecrites : EX-VIS-008 (la scene peinte, losange de\n256 x 159, figurine de 170 px en cellule 192 x 256, alpha continu, echelle de\nl'art donnee par le lieu), EX-VIS-009 (la frontiere scene / interface, qui\nsepare desormais deux echelles et non deux factures) et EX-REN-013 (zoom libre,\nune case valant la hauteur de la fenetre divisee par 10,8, soit la meme etendue\nde monde a toute definition). La consigne du generateur est ecrite en trois\nblocs.\n\nCe que la maquette a impose au standard : la famille des sols livre d'abord une\ndalle de fond repetable en trois variantes, une planche d'animation porte 8 px\nde marge entre ses images, et une zone qui remplit la vue fait au moins vingt\ncases sur dix-sept.\n\nLe nombre d'images par animation reste la seule valeur ouverte : il se tranche\nsur l'essai de marche commande dans la fiche du lot.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(standard): la consigne sait commander une planche d'animation (LOT-101)\n\nLe bloc B demandait \"ONE piece, centred, alone... no cropping\" : envoye tel\nquel, il contredisait l'essai de marche, qui demande une bande de six ou huit\nimages. Le cadrage a donc sa variante planche d'animation -- meme figure, meme\nlumiere, seule la pose change, 8 px de marge du standard -- et une animation se\ncommande en une seule fois, parce que demander les images une par une garantit\nque le personnage derive.\n\nLe bloc C de l'essai s'aligne : il ne redit plus la marge et nomme les quatre\nposes cles du cycle, pour que six et huit soient compares sur la meme marche.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(standard): la maquette est approuvee, le sol passe au LOT-108 (LOT-101)\n\nL'auteur approuve la maquette aux deux definitions, dans ce qu'elle juge : la\ncomposition et l'emprise se lisent a 100 px de case, et la mollesse du trait a\n2160p est celle de la planche agrandie, pas celle du standard.\n\nSa reserve ne restait portee que par une regle du paragraphe 4 : repete sur tout\nl'ecran, le panneau borde dessine un treillis. Elle devient un risque nomme et un\ncritere du LOT-108, qui produit les sols d'Arenarea -- douze cases sur douze sans\nmotif regulier, parce que le moire ne se voit pas sur quatre.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(standard): la cadence des figurines part au LOT-112 (LOT-101)\n\nLe LOT-101 devait trancher ici le nombre d'images par animation, sur un essai de\nmarche en six et en huit images. Decision de l'auteur : ce lot fige le standard\nde la scene -- geometrie, facture, palette, familles de pieces -- et la maquette\nle valide sur ce terrain. Une cadence ne se juge pas sur une place vide : elle se\njuge sur la premiere figurine, a cote de son ancre et de son sol, et c'est le\nLOT-112 qui la produit en fixant le gabarit de toutes les autres.\n\nL'essai part donc entier au LOT-112 : le bloc C pret a envoyer, les trois\nquestions dans l'ordre, un livrable et un critere. Le paragraphe 5 du standard\nnomme ce lot au lieu de dire \"a trancher\", et le detail des pieces reste ce\nqu'il etait -- l'affaire des lots d'assets dedies, ce standard disant le format\net non le dessin.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(planning): LOT-101 livre (PR #95)\n\nLes trois criteres sont tenus : l'auteur a approuve la maquette aux deux\ndefinitions, le standard de la scene ne laisse aucune valeur ouverte, et les\ntrois lints sont verts. La filiere des assets est debloquee : le LOT-102 peut\ncommencer.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-20T20:43:14Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/ab39ebb86bc43f1af39501d7fa9b413446775d10"
        },
        "date": 1789958572195,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 97286.35156249776,
            "unit": "ns/iter",
            "extra": "iterations: 12800\ncpu: 97656.25 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 137314.19000000073,
            "unit": "ns/iter",
            "extra": "iterations: 10000\ncpu: 139062.5 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 36236.20382543831,
            "unit": "ns/iter",
            "extra": "iterations: 47158\ncpu: 36446.626235209296 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 98337.91953162433,
            "unit": "ns/iter",
            "extra": "iterations: 14689\ncpu: 102117.23058070664 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 673417.4184261063,
            "unit": "ns/iter",
            "extra": "iterations: 2084\ncpu: 532329.6545105566 ns\nthreads: 1"
          },
          {
            "name": "LoadShippedLevel",
            "value": 1475412.248995962,
            "unit": "ns/iter",
            "extra": "iterations: 996\ncpu: 1443273.092369478 ns\nthreads: 1"
          },
          {
            "name": "ComposeMartpart",
            "value": 269.8241874637339,
            "unit": "us/iter",
            "extra": "iterations: 6892\ncpu: 283.3901625072548 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "2f46f3125f78187d31e996c6d3eea38bc2ba9688",
          "message": "docs(site) : une seule charte pour les trois parties du site publié (#107)\n\nLe site de gh-pages avait trois rendus pour trois générateurs : le bleu et\nblanc de Doxygen à la racine, l'ivoire de la planification sous planning/, et\nune troisième palette écrite à la main dans la page qualité.\n\nSite/ tient désormais ce qu'ils ont en commun :\n\n- tokens.css : la palette, les fontes et le thème sombre, écrits une fois ;\n- topbar.css : la barre d'en-tête, mêmes libellés et même ordre partout, avec\n  la partie courante marquée ;\n- theme.css : l'habillage des pages écrites ici (planification, qualité) ;\n- reference.css et header.html : la référence de code. La première rebranche\n  les ~140 variables CSS de Doxygen sur les jetons, le second porte la barre à\n  la place des onglets de Doxygen (DISABLE_INDEX).\n\nDeux points ont demandé d'y regarder de près. HTML_COLORSTYLE passe à\nAUTO_LIGHT : en LIGHT, Doxygen résout ses variables à la génération et écrit\nses couleurs en dur, ne laissant rien à rebrancher. Et son bloc sombre\nredéclare ces variables sous html:not(.dark-mode), plus spécifique qu'un html\nnu : reference.css reprend le même sélecteur, sans quoi le branchement serait\nappliqué en clair et entièrement recouvert en sombre.\n\nLa barre de Doxygen est posée dans #top, dont navtree.js mesure la hauteur\npour placer l'arbre et le contenu ; elle n'est donc pas sticky, à la\ndifférence des deux autres.\n\nAucun contenu ne change, et plus aucune couleur ne s'écrit ailleurs que dans\nSite/tokens.css.\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-21T19:53:19Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/2f46f3125f78187d31e996c6d3eea38bc2ba9688"
        },
        "date": 1790044753791,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 101491.02343749661,
            "unit": "ns/iter",
            "extra": "iterations: 12800\ncpu: 101318.359375 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 138679.12499999793,
            "unit": "ns/iter",
            "extra": "iterations: 11200\ncpu: 139508.92857142858 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 37784.709821428056,
            "unit": "ns/iter",
            "extra": "iterations: 35840\ncpu: 37928.989955357145 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 104695.57999999779,
            "unit": "ns/iter",
            "extra": "iterations: 10000\ncpu: 115625 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 698954.0923399705,
            "unit": "ns/iter",
            "extra": "iterations: 1906\ncpu: 655823.7145855194 ns\nthreads: 1"
          },
          {
            "name": "LoadTestLevel",
            "value": 1523289.9999999744,
            "unit": "ns/iter",
            "extra": "iterations: 1000\ncpu: 1671875 ns\nthreads: 1"
          },
          {
            "name": "ComposeTestMap",
            "value": 302.83955606145105,
            "unit": "us/iter",
            "extra": "iterations: 5271\ncpu: 263.82564978182506 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "711ae9f5112b1e351cdce9eae0c9f62181e0560f",
          "message": "LOT-128 — Les cartes maquettes : dessiner et jouer sans texture (#108)\n\n* docs(planning) : LOT-128 planifié, six décisions et six phases\n\nL'audit M1-M8 de la fiche est recoupé dans le code. Quatre points qui ne se\ntranchaient pas depuis la fiche sont arbitrés, deux autres apparaissent en\nl'écrivant :\n\n- D1, la primitive est un quad à quatre sommets libres : un losange iso au\n  rapport 0,62 n'est ni un rectangle aligné ni un segment ;\n- D2, le jeton est une image engendrée en code pur, lettre comprise — il\n  n'existe aucun rendu de texte en scène côté jeu depuis le LOT-88 ;\n- D3, la couleur du jeton se déduit de ce que le format dit déjà : « npc\n  hostile » n'a aucune existence dans MapEntity ;\n- D4, « --render --plan » se lit comme la maquette du planning, il ne s'y\n  superpose pas : douze types génériques ne diront jamais gradins ni podium ;\n- D5, la palette vit dans le code — une maquette se dessine sans aucun asset ;\n- D6, un bloc fait une case de haut, quel que soit le type.\n\nLe repli se déclenche sur « cette case n'a nommé aucune pièce », pas sur « la\ncarte n'a pas de lieu » : M1 et M6 se referment du même geste.\n\nTaille M -> L. Le lot passe en cours.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-128 — La primitive de maquette : un quad à quatre sommets libres\n\nPhase 1 sur six. Rien ne change à l'écran : aucun appelant encore.\n\nUn losange isométrique au rapport 0,62 n'est ni un SpriteQuad (rectangle\naligné sur les axes) ni un LineQuad, et ce n'est même pas un carré tourné,\npuisque sa hauteur et sa largeur ne sont pas dans le même rapport que ses\ncôtés. Les faces d'un bloc extrudé sont, elles, des parallélogrammes.\nhmi::PolyQuad couvre les deux.\n\nCe n'est pas un cas nouveau pour le GPU : un quad, ce sont déjà quatre\nsommets, et draw(LineQuad) en produisait déjà à des positions libres. Le\nformat de sommet, le tampon d'indices et le pipeline ne bougent pas. La\ntexture liée est l'aplat blanc, de sorte que le culling, le regroupement par\ntexture et le tri restent ceux de toutes les autres primitives — ce que les\ntests vérifient.\n\nLes deux soumissions apprennent la primitive au même endroit qu'avant, par un\nswitch exhaustif : submitComposedScene côté GPU, paintComposedScene côté\nQPainter. La parité de ces deux chemins reste tenue par test_scene_painter.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-128 — La palette de maquette, et le sol qu'elle peint\n\nPhase 2 sur six. Referme M1 et M6.\n\nLe repli ne se déclenche pas sur « la carte n'a pas de lieu » mais sur « cette\ncase n'a nommé aucune pièce ». Les deux manques se referment alors du même\ngeste : la carte sans lieu, où aucune case n'en nomme, et le type que la table\ndu lieu ne couvre pas — l'eau du Colisée, jusqu'ici invisible. Un seul chemin à\nécrire, un seul à tester.\n\nL'instantané gagne le type de chaque case, tiré de la même grille que son sol :\nce qui se dessine en maquette est ce que la couche de sol dit, jamais une\nautre. La composition émet alors un losange de couleur, trié à la profondeur de\nsa case comme une pièce, lié à un aplat blanc de 1 x 1 que les deux rendus\nfournissent désormais dans leur table de textures.\n\nLa palette vit dans le code (décision D5) : une maquette doit se dessiner quand\naucun fichier d'asset n'est présent, et une palette chargée depuis le disque\nréintroduirait la dépendance que le lot supprime.\n\nTrois chemins de peinture de types disparaissent, remplacés par celui de la\ncomposition : paintIsoTypeColors du canevas, le repli de « --render », et les\ncouleurs de l'atlas procédural dans la vue à plat. La vignette de la palette et\nla mini-carte suivent : le canevas iso, le canevas à plat, la vignette et la\nmini-carte montrent enfin UNE couleur par type, et non quatre qui se\nressemblent.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-128 — Les blocs : un mur se lit comme un mur\n\nPhase 3 sur six. Referme M3.\n\nUn type qui bloque — wall, solid, cliff — ne se dessine plus en losange plat\nmais en bloc de trois faces, haut d'une case (décision D6). Les trois faces\nprennent trois éclairements : sans cet écart, trois quads de la même teinte ne\nse distinguent pas et le bloc redevient la tache plate qu'on voulait quitter.\n\nLe bloc va sur le calque du décor, trié au pied de sa case exactement comme une\npièce de relief. C'est ce qui le fait masquer ce qui est derrière lui,\nfigurines comprises : posé sur le calque des tuiles, il passerait sous le héros\nquel que soit leur ordre, et le mur cesserait d'être un mur.\n\nL'eau profonde bloque le pas mais n'est pas de la matière : elle reste un\nlosange plat, plus sombre que l'eau vive, et l'on voit par-dessus.\n\nVérifié à l'œil sur une carte d'essai sans lieu, rendue par « --render » :\nenceinte de murs, herbe, chemins de sable et de terre, mare avec son eau\nprofonde, un bloc de pierre et une falaise — tout se distingue, et les murs\nmasquent ce qu'ils doivent masquer.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-128 — Les jetons : qui est où, et par où l'on sort\n\nPhase 4 sur six. Referme M4.\n\nLe jeton entier — disque, cerne et lettre — est peint en code pur, comme\ncore::assetMarker peint le marqueur d'un asset manquant, puis téléversé comme\nn'importe quelle texture (décision D2). Il n'existe aucun rendu de texte en\nscène côté jeu, et en introduire un pour trente-six caractères aurait coûté\nplus que la table de glyphes 5 × 7 qui tient ici.\n\nIl s'adresse par un chemin, « Token/<nature>/<lettre>.png », comme une pièce de\nlieu : les deux rendus n'ont rien de neuf à apprendre, ils voient un chemin de\nplus et savent qu'un chemin de jeton se peint au lieu de se charger. Le jeu et\nl'éditeur montrent donc la même image, au pixel près, sans qu'on ait à le\nvouloir.\n\nLa couleur se déduit de ce que le format dit déjà (décision D3) : aucune\npropriété n'est ajoutée. « npc hostile » n'existe pas dans MapEntity — le jaune\nse règle sur « ce PNJ porte un dialogue », et le LOT-116 le rebranchera sur la\nquête. Un PNJ qui a déjà sa figurine n'a pas de jeton par-dessus.\n\nTrois décisions sont venues du rendu, pas du plan, et sont écrites dans la\nfiche : le jeton vit sur le calque de l'interface, sans quoi un mur d'en face\nle coupe en deux (D7) ; sa lettre vient du type à défaut d'étiquette, jamais de\nl'identifiant, qui vaut « E » pour tout le monde (D8) ; et il est peint ou\nabsent, jamais un damier (D9).\n\nVérifié à l'œil : M la mère, N le PNJ muet, C le coffre, G la porte, W les\nloups, A le portail avec sa flèche, la zone de combat cernée de rouge et la\nronde tracée en blanc.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-128 — Le jeu, les essais, et la preuve qu'une carte nue se voit\n\nPhase 5 sur six. Referme M2, M6 et M8, et tient EX-EXP-005.\n\nL'essai immédiat (P) et l'essai complet (F5) passaient déjà par\ncomposeWorldScene : ils héritent de la maquette sans une ligne, ce qui était\nl'intérêt de la mettre là. Le jeu aussi. Ne restait que le message : une carte\nsans lieu n'est plus un manque à signaler mais l'état de départ normal d'une\ncarte, et le journal le dit maintenant en information, pas en avertissement.\n\n« --check » signale un type de tuile que la table du lieu ne couvre pas — l'eau\net l'eau profonde aujourd'hui. Un avertissement, pas une erreur : la case se\nvoit désormais, elle n'est simplement pas encore habillée.\n\nLa preuve tient en un test. Une carte bâtie en mémoire, sans lieu, sans pièce\net sans aucun fichier d'image, est rendue hors écran deux fois : par le rendu\nQRhi du jeu, puis par le peintre QPainter de l'éditeur. Le test vérifie que les\nseules textures demandées sont celles des jetons, que plus de la moitié de la\nsurface est peinte — rien n'est resté vide — et que moins de 0,5 % des pixels\ndiffèrent entre les deux rendus. EX-EXP-005 et la parité des deux chemins\ntiennent donc par la même assertion, sur le cas qui les mettait le plus en\ndanger.\n\nLa capture jointe au test montre ce que le GPU du jeu dessine : l'enceinte en\nblocs, la mare, les quatre jetons et la flèche du portail.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* LOT-128 — L'atelier : maquetter, jouer, puis habiller\n\nPhase 6 sur six. Referme M5 et M7.\n\n« New map » crée désormais ses couches dans tous les cas, lieu ou pas : une\ncarte neuve sans couche était un cul-de-sac silencieux, puisque\n« Change sheet… » refuse ensuite de l'habiller. Un modèle Blockout — une\nenceinte et son ouverture — s'ajoute aux trois existants, et l'annexe connaît\nl'état « blockout », entre « generated » et « retouched ».\n\nLe modèle a mis au jour un trou : les modèles livrés peignent leurs murs sur la\ncouche DÉCOR, que la maquette ne regardait pas. Un mur s'y extrude maintenant\ncomme sur le sol, sans quoi une carte maquettée à la manière des modèles aurait\nété vide.\n\n« --render --plan » rend la carte au vocabulaire des plans de principe du\nplanning : blocs couchés à plat — un plan se lit, il ne se joue pas, et\nl'extrusion y cacherait justement ce qu'on vient y voir —, pastilles, et une\nlégende des types et des natures de jeton employés. Ses libellés s'écrivent\navec la table de glyphes des jetons : « --render » tourne sans QApplication\n(LOT-EDITOR-13), donc sans aucune police, et QPainter::drawText y échoue.\n\nLe guide d'usage ouvre le chapitre « maquetter, jouer, puis habiller » : ce que\nla maquette montre, comment l'essayer, et ce que « Change sheet… » change — ou\nplutôt ne change pas.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(changelog) : les cartes maquettes du LOT-128\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(planning) : LOT-128 livré, et la recette qui reste à l'auteur\n\nLe cahier de test suit les vingt-deux cas ajoutés par le lot.\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n* docs(planning) : LOT-128, le numéro de sa PR\n\nCo-Authored-By: Claude Opus 5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5 <noreply@anthropic.com>",
          "timestamp": "2026-09-22T06:54:26Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/711ae9f5112b1e351cdce9eae0c9f62181e0560f"
        },
        "date": 1790060345410,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 156815.93749999732,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 156947.54464285713 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 217657.1532211364,
            "unit": "ns/iter",
            "extra": "iterations: 6892\ncpu: 217643.64480557168 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57173.06085139341,
            "unit": "ns/iter",
            "extra": "iterations: 22974\ncpu: 57129.79890310786 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 163074.9888392862,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 163922.99107142858 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1180887.9687499995,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1184082.03125 ns\nthreads: 1"
          },
          {
            "name": "LoadTestLevel",
            "value": 2671517.5847457102,
            "unit": "ns/iter",
            "extra": "iterations: 472\ncpu: 2681408.8983050846 ns\nthreads: 1"
          },
          {
            "name": "ComposeTestMap",
            "value": 550.9304539975998,
            "unit": "us/iter",
            "extra": "iterations: 2489\ncpu: 552.4306950582563 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "33de88bc169b6ab01d6850112ca0b76f4ad9538d",
          "message": "docs(planning) : LOT-103 livré (#114)\n\nLe contrôle visuel du travelling, dernier critère du lot, est fait par\nl'auteur : la maquette ne scintille pas. La fiche passe à `livre` et ne\ndit plus que la PR #112 est un brouillon.\n\nCo-authored-by: Claude Opus 5.5 <noreply@anthropic.com>",
          "timestamp": "2026-09-22T20:57:36Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/33de88bc169b6ab01d6850112ca0b76f4ad9538d"
        },
        "date": 1790131170070,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 158083.30357143877,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 158691.40625 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 214497.5937500071,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 214843.75 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57099.40535979689,
            "unit": "ns/iter",
            "extra": "iterations: 24889\ncpu: 57128.65121137852 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 162915.23634131948,
            "unit": "ns/iter",
            "extra": "iterations: 8145\ncpu: 164978.51442602824 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1112723.1249999702,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1123046.875 ns\nthreads: 1"
          },
          {
            "name": "LoadTestLevel",
            "value": 2687333.5341367535,
            "unit": "ns/iter",
            "extra": "iterations: 498\ncpu: 2698293.172690763 ns\nthreads: 1"
          },
          {
            "name": "ComposeTestMap",
            "value": 538.6701171875341,
            "unit": "us/iter",
            "extra": "iterations: 2560\ncpu: 537.109375 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "776e4bf13307d8387f16e15c4be859c0927ee362",
          "message": "Merge pull request #121 from azertval/lot-129-etages-et-toits\n\nLOT-129 — Les étages et les toits de la scène",
          "timestamp": "2026-09-23T22:40:43Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/776e4bf13307d8387f16e15c4be859c0927ee362"
        },
        "date": 1790217684342,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 157996.32812500012,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 156947.54464285713 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 214351.31250001406,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 212402.34375 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57714.76296663616,
            "unit": "ns/iter",
            "extra": "iterations: 24216\ncpu: 57425.87545424513 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 162271.7522321412,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 162179.1294642857 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1146633.593750046,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1147460.9375 ns\nthreads: 1"
          },
          {
            "name": "LoadTestLevel",
            "value": 2652019.70338987,
            "unit": "ns/iter",
            "extra": "iterations: 472\ncpu: 2648305.084745763 ns\nthreads: 1"
          },
          {
            "name": "ComposeTestMap",
            "value": 616.1646930779003,
            "unit": "us/iter",
            "extra": "iterations: 2297\ncpu: 612.2115803221593 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "a138dbe4b2b2a3e20e4f44d2afdb7ce3d714f870",
          "message": "Merge pull request #129 from azertval/lot-109-carte-arenarea\n\nLOT-109 — Arenarea, le quartier entier ; affichage d'un lieu refait ; LOT-127 livré",
          "timestamp": "2026-09-24T21:44:19Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/a138dbe4b2b2a3e20e4f44d2afdb7ce3d714f870"
        },
        "date": 1790304029236,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 157625.12276785553,
            "unit": "ns/iter",
            "extra": "iterations: 8960\ncpu: 156947.54464285713 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 211652.04687499718,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 209960.9375 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 56935.550875453264,
            "unit": "ns/iter",
            "extra": "iterations: 24216\ncpu: 56780.640898579455 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 161791.3321055766,
            "unit": "ns/iter",
            "extra": "iterations: 8145\ncpu: 161141.80478821363 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1100266.4062500498,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1098632.8125 ns\nthreads: 1"
          },
          {
            "name": "LoadTestLevel",
            "value": 2608559.322033807,
            "unit": "ns/iter",
            "extra": "iterations: 472\ncpu: 2615201.2711864407 ns\nthreads: 1"
          },
          {
            "name": "ComposeTestMap",
            "value": 635.1201339285747,
            "unit": "us/iter",
            "extra": "iterations: 2240\ncpu: 634.765625 us\nthreads: 1"
          },
          {
            "name": "ArenareaSnapshot",
            "value": 4.175658132530168,
            "unit": "ms/iter",
            "extra": "iterations: 332\ncpu: 4.235692771084337 ms\nthreads: 1"
          },
          {
            "name": "ArenareaComposeWholeMap",
            "value": 8.382339644970148,
            "unit": "ms/iter",
            "extra": "iterations: 169\ncpu: 8.321005917159763 ms\nthreads: 1"
          },
          {
            "name": "ArenareaTexturePaths",
            "value": 3.6628002564101827,
            "unit": "ms/iter",
            "extra": "iterations: 390\ncpu: 3.6458333333333335 ms\nthreads: 1"
          },
          {
            "name": "ArenareaBuildStaticScene",
            "value": 8.35307831325311,
            "unit": "ms/iter",
            "extra": "iterations: 166\ncpu: 8.377259036144578 ms\nthreads: 1"
          },
          {
            "name": "ArenareaFrame1080p",
            "value": 60.16173935753304,
            "unit": "us/iter",
            "extra": "iterations: 22974\ncpu: 59.850265517541565 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "f20d7a39f7f722f896325ec286689c9b7f725132",
          "message": "Merge pull request #141 from azertval/claude/recettage-v0-0-1-lot-122-ixgfl2\n\nLOT-122 — Recette et version 0.0.1 : la démo basique",
          "timestamp": "2026-09-25T20:05:24Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/f20d7a39f7f722f896325ec286689c9b7f725132"
        },
        "date": 1790367021285,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 134072.74005624218,
            "unit": "ns/iter",
            "extra": "iterations: 9956\ncpu: 133399.4576134994 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 189586.79523234905,
            "unit": "ns/iter",
            "extra": "iterations: 7467\ncpu: 188328.64604258738 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 18308.499171322135,
            "unit": "ns/iter",
            "extra": "iterations: 81455\ncpu: 18415.075808728747 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 68324.39410664169,
            "unit": "ns/iter",
            "extra": "iterations: 20837\ncpu: 68237.99011373999 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 933396.3791968813,
            "unit": "ns/iter",
            "extra": "iterations: 1519\ncpu: 936059.9078341014 ns\nthreads: 1"
          },
          {
            "name": "LoadTestLevel",
            "value": 2080457.8571431062,
            "unit": "ns/iter",
            "extra": "iterations: 560\ncpu: 2064732.142857143 ns\nthreads: 1"
          },
          {
            "name": "ComposeTestMap",
            "value": 530.9528652751103,
            "unit": "us/iter",
            "extra": "iterations: 2635\ncpu: 527.7514231499051 us\nthreads: 1"
          },
          {
            "name": "ArenareaSnapshot",
            "value": 0.07609597506288199,
            "unit": "ms/iter",
            "extra": "iterations: 18286\ncpu: 0.07604861642786831 ms\nthreads: 1"
          },
          {
            "name": "ArenareaComposeWholeMap",
            "value": 0.10710469999999077,
            "unit": "ms/iter",
            "extra": "iterations: 10000\ncpu: 0.1078125 ms\nthreads: 1"
          },
          {
            "name": "ArenareaTexturePaths",
            "value": 0.054787894531243,
            "unit": "ms/iter",
            "extra": "iterations: 25600\ncpu: 0.054931640625 ms\nthreads: 1"
          },
          {
            "name": "ArenareaBuildStaticScene",
            "value": 0.10701273000001947,
            "unit": "ms/iter",
            "extra": "iterations: 10000\ncpu: 0.1078125 ms\nthreads: 1"
          },
          {
            "name": "ArenareaFrame1080p",
            "value": 2.5127569642856025,
            "unit": "us/iter",
            "extra": "iterations: 560000\ncpu: 2.5390625 us\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "917b73a208e9bacf09a65e68e9e8098160a0272c",
          "message": "Merge pull request #142 from azertval/fix/code-scanning-alertes\n\nCode scanning — Les alertes clang-tidy de main corrigées",
          "timestamp": "2026-09-25T21:54:53Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/917b73a208e9bacf09a65e68e9e8098160a0272c"
        },
        "date": 1790390358207,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "ReachableAreaDashing",
            "value": 165438.03560466514,
            "unit": "ns/iter",
            "extra": "iterations: 8145\ncpu: 164978.51442602824 ns\nthreads: 1"
          },
          {
            "name": "FindPathAcrossGrid",
            "value": 215761.14062500373,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 214843.75 ns\nthreads: 1"
          },
          {
            "name": "LineOfSightAcrossGrid",
            "value": 57517.34803435748,
            "unit": "ns/iter",
            "extra": "iterations: 24216\ncpu: 57425.87545424513 ns\nthreads: 1"
          },
          {
            "name": "CoverFromWithInterposed",
            "value": 166802.2099447531,
            "unit": "ns/iter",
            "extra": "iterations: 8145\ncpu: 166896.86924493554 ns\nthreads: 1"
          },
          {
            "name": "PlanTurnFourVersusFour",
            "value": 1115249.7656250037,
            "unit": "ns/iter",
            "extra": "iterations: 1280\ncpu: 1110839.84375 ns\nthreads: 1"
          },
          {
            "name": "LoadTestLevel",
            "value": 2633034.3220338915,
            "unit": "ns/iter",
            "extra": "iterations: 472\ncpu: 2648305.084745763 ns\nthreads: 1"
          },
          {
            "name": "ComposeTestMap",
            "value": 686.2684675835103,
            "unit": "us/iter",
            "extra": "iterations: 2036\ncpu: 683.0181728880157 us\nthreads: 1"
          },
          {
            "name": "ArenareaSnapshot",
            "value": 0.093693490926137,
            "unit": "ms/iter",
            "extra": "iterations: 14933\ncpu: 0.09312428848858234 ms\nthreads: 1"
          },
          {
            "name": "ArenareaComposeWholeMap",
            "value": 0.13641028525512136,
            "unit": "ms/iter",
            "extra": "iterations: 9956\ncpu: 0.13653826838087585 ms\nthreads: 1"
          },
          {
            "name": "ArenareaTexturePaths",
            "value": 0.06940739049302772,
            "unit": "ms/iter",
            "extra": "iterations: 20364\ncpu: 0.06982297191121586 ms\nthreads: 1"
          },
          {
            "name": "ArenareaBuildStaticScene",
            "value": 0.13671827038971363,
            "unit": "ms/iter",
            "extra": "iterations: 9956\ncpu: 0.13653826838087585 ms\nthreads: 1"
          },
          {
            "name": "ArenareaFrame1080p",
            "value": 3.5761087366678335,
            "unit": "us/iter",
            "extra": "iterations: 389565\ncpu: 3.569686701834097 us\nthreads: 1"
          }
        ]
      }
    ],
    "Peinture du canevas de l'editeur (Release, windows-2022)": [
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "776e4bf13307d8387f16e15c4be859c0927ee362",
          "message": "Merge pull request #121 from azertval/lot-129-etages-et-toits\n\nLOT-129 — Les étages et les toits de la scène",
          "timestamp": "2026-09-23T22:40:43Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/776e4bf13307d8387f16e15c4be859c0927ee362"
        },
        "date": 1790217691009,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "PaintHdMockup1080p",
            "value": 14.080715000000055,
            "unit": "ms/iter",
            "extra": "iterations: 100\ncpu: 13.59375 ms\nthreads: 1"
          },
          {
            "name": "PaintHdMockupZoomedOut",
            "value": 5.220152272727319,
            "unit": "ms/iter",
            "extra": "iterations: 264\ncpu: 5.208333333333333 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "a138dbe4b2b2a3e20e4f44d2afdb7ce3d714f870",
          "message": "Merge pull request #129 from azertval/lot-109-carte-arenarea\n\nLOT-109 — Arenarea, le quartier entier ; affichage d'un lieu refait ; LOT-127 livré",
          "timestamp": "2026-09-24T21:44:19Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/a138dbe4b2b2a3e20e4f44d2afdb7ce3d714f870"
        },
        "date": 1790304035081,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "PaintHdMockup1080p",
            "value": 13.125178124999692,
            "unit": "ms/iter",
            "extra": "iterations: 128\ncpu: 12.8173828125 ms\nthreads: 1"
          },
          {
            "name": "PaintHdMockupZoomedOut",
            "value": 5.146497794117643,
            "unit": "ms/iter",
            "extra": "iterations: 272\ncpu: 5.112591911764706 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "f20d7a39f7f722f896325ec286689c9b7f725132",
          "message": "Merge pull request #141 from azertval/claude/recettage-v0-0-1-lot-122-ixgfl2\n\nLOT-122 — Recette et version 0.0.1 : la démo basique",
          "timestamp": "2026-09-25T20:05:24Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/f20d7a39f7f722f896325ec286689c9b7f725132"
        },
        "date": 1790367027464,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "PaintHdMockup1080p",
            "value": 10.168085156250228,
            "unit": "ms/iter",
            "extra": "iterations: 128\ncpu: 9.8876953125 ms\nthreads: 1"
          },
          {
            "name": "PaintHdMockupZoomedOut",
            "value": 5.15824821428542,
            "unit": "ms/iter",
            "extra": "iterations: 280\ncpu: 5.133928571428571 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "azertval",
            "username": "azertval",
            "email": "valentin.eloy@gmail.com"
          },
          "committer": {
            "name": "GitHub",
            "username": "web-flow",
            "email": "noreply@github.com"
          },
          "id": "917b73a208e9bacf09a65e68e9e8098160a0272c",
          "message": "Merge pull request #142 from azertval/fix/code-scanning-alertes\n\nCode scanning — Les alertes clang-tidy de main corrigées",
          "timestamp": "2026-09-25T21:54:53Z",
          "url": "https://github.com/azertval/JustAnotherRpgGame/commit/917b73a208e9bacf09a65e68e9e8098160a0272c"
        },
        "date": 1790390364861,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "PaintHdMockup1080p",
            "value": 13.257540178571503,
            "unit": "ms/iter",
            "extra": "iterations: 112\ncpu: 12.834821428571429 ms\nthreads: 1"
          },
          {
            "name": "PaintHdMockupZoomedOut",
            "value": 5.173259926470768,
            "unit": "ms/iter",
            "extra": "iterations: 272\ncpu: 5.170036764705882 ms\nthreads: 1"
          }
        ]
      }
    ]
  }
}