window.BENCHMARK_DATA = {
  "lastUpdate": 1789871928776,
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
      }
    ]
  }
}