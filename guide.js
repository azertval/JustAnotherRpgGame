var guide =
[
    [ "Comment lire ce guide", "guide.html#autotoc_md172", null ],
    [ "Architecture en deux couches", "guide.html#autotoc_md173", null ],
    [ "Plan du guide", "guide.html#autotoc_md174", null ],
    [ "Boucle de jeu et pas de temps fixe", "guide-boucle.html", [
      [ "Qu'est-ce qu'une boucle de jeu ?", "guide-boucle.html#autotoc_md37", null ],
      [ "Le piège du framerate variable", "guide-boucle.html#autotoc_md38", null ],
      [ "Le principe du pas de temps fixe", "guide-boucle.html#autotoc_md39", null ],
      [ "L'accumulateur : <a class=\"el\" href=\"classcore_1_1FixedTimestep.html\" title=\"core::FixedTimestep\">core::FixedTimestep</a>", "guide-boucle.html#autotoc_md40", [
        [ "Exemple chiffré", "guide-boucle.html#autotoc_md41", null ],
        [ "La « spirale de la mort »", "guide-boucle.html#autotoc_md42", null ],
        [ "<a class=\"el\" href=\"classcore_1_1FixedTimestep.html#ab7f7f880eef11df91ab7bc647e19bf56\" title=\"core::FixedTimestep::interpolationAlpha\">interpolationAlpha</a>", "guide-boucle.html#autotoc_md43", null ],
        [ "Les frames sans pas de simulation et les entrées", "guide-boucle.html#autotoc_md44", null ]
      ] ],
      [ "Conséquence pratique pour tout le code de simulation", "guide-boucle.html#autotoc_md45", null ],
      [ "Voir aussi", "guide-boucle.html#autotoc_md46", null ]
    ] ],
    [ "ECS : entités, composants, systèmes", "guide-ecs.html", [
      [ "Le problème que l'ECS résout", "guide-ecs.html#autotoc_md76", null ],
      [ "L'entité : <a class=\"el\" href=\"structcore_1_1Entity.html\" title=\"core::Entity\">core::Entity</a>", "guide-ecs.html#autotoc_md77", null ],
      [ "Le <a class=\"el\" href=\"classcore_1_1World.html\" title=\"core::World\">World</a>", "guide-ecs.html#autotoc_md78", null ],
      [ "Le stockage : sparse set (core::ComponentPool&lt;T&gt;)", "guide-ecs.html#autotoc_md79", [
        [ "Ajout et suppression : <em>swap-and-pop</em>", "guide-ecs.html#autotoc_md80", null ],
        [ "Exemple pas à pas", "guide-ecs.html#autotoc_md81", null ]
      ] ],
      [ "Les vues : core::View&lt;Components...&gt;", "guide-ecs.html#autotoc_md82", null ],
      [ "Les systèmes et l'ordre d'exécution", "guide-ecs.html#autotoc_md83", null ],
      [ "Voir aussi", "guide-ecs.html#autotoc_md84", null ]
    ] ],
    [ "Mathématiques du moteur", "guide-maths.html", [
      [ "<a class=\"el\" href=\"structcore_1_1Vector2.html\" title=\"core::Vector2\">Vector2</a> : un point ou une direction dans le monde", "guide-maths.html#autotoc_md131", [
        [ "<a class=\"el\" href=\"structcore_1_1Vector2.html#a5f3517fe219407f73a3ed5542091c31c\" title=\"core::Vector2::lengthSquared\">lengthSquared</a> : éviter la racine carrée", "guide-maths.html#autotoc_md132", null ],
        [ "Égalité approchée", "guide-maths.html#autotoc_md133", null ]
      ] ],
      [ "<a class=\"el\" href=\"structcore_1_1Rect.html\" title=\"core::Rect\">Rect</a> : le rectangle aligné aux axes", "guide-maths.html#autotoc_md134", null ],
      [ "Conventions d'unités et de repère", "guide-maths.html#autotoc_md135", null ],
      [ "Comparaison flottante : pourquoi l'égalité stricte est dangereuse", "guide-maths.html#autotoc_md136", null ],
      [ "Voir aussi", "guide-maths.html#autotoc_md137", null ]
    ] ],
    [ "Niveaux : modèle, couches, entités, chargement", "guide-niveaux.html", [
      [ "Le modèle en mémoire", "guide-niveaux.html#autotoc_md138", [
        [ "Deux systèmes de coordonnées à ne pas confondre", "guide-niveaux.html#autotoc_md139", null ],
        [ "<a class=\"el\" href=\"namespacecore.html#ace99a1be913e6294e42e9ebb145eb875\" title=\"core::TileType\">core::TileType</a> : le vocabulaire des cases", "guide-niveaux.html#autotoc_md140", null ],
        [ "<a class=\"el\" href=\"classcore_1_1TileMap.html\" title=\"core::TileMap\">core::TileMap</a> : la grille", "guide-niveaux.html#autotoc_md141", null ],
        [ "<a class=\"el\" href=\"classcore_1_1Level.html\" title=\"core::Level\">core::Level</a> : la carte assemblée", "guide-niveaux.html#autotoc_md142", null ],
        [ "Couches : ce qu'on voit n'est pas ce qui bloque", "guide-niveaux.html#autotoc_md143", null ],
        [ "Entités et propriétés libres", "guide-niveaux.html#autotoc_md144", null ]
      ] ],
      [ "Chargement JSON", "guide-niveaux.html#autotoc_md145", [
        [ "Exemple concret", "guide-niveaux.html#autotoc_md146", null ],
        [ "Validation", "guide-niveaux.html#autotoc_md147", null ]
      ] ],
      [ "Qui lit la carte", "guide-niveaux.html#autotoc_md148", null ],
      [ "Voir aussi", "guide-niveaux.html#autotoc_md149", null ]
    ] ],
    [ "Entrées et actions logiques", "guide-entrees.html", [
      [ "Le principe : ne jamais coder « en dur » une touche dans le gameplay", "guide-entrees.html#autotoc_md99", null ],
      [ "L'intention : <a class=\"el\" href=\"structcore_1_1ExplorationIntent.html\" title=\"core::ExplorationIntent\">core::ExplorationIntent</a>", "guide-entrees.html#autotoc_md100", null ],
      [ "Échantillonner plutôt que réagir : <a class=\"el\" href=\"classhmi_1_1InputState.html\" title=\"hmi::InputState\">hmi::InputState</a>", "guide-entrees.html#autotoc_md101", [
        [ "Détecter les fronts, pas seulement l'état", "guide-entrees.html#autotoc_md102", null ],
        [ "Le cycle d'un relevé", "guide-entrees.html#autotoc_md103", null ]
      ] ],
      [ "La manette : une seconde source, fusionnée en lecture (EX-CTRL-002)", "guide-entrees.html#autotoc_md104", null ],
      [ "Le menu d'options", "guide-entrees.html#autotoc_md105", null ],
      [ "Les raccourcis de l'éditeur : <a class=\"el\" href=\"classhmi_1_1EditorKeyBindings.html\" title=\"hmi::EditorKeyBindings\">EditorKeyBindings</a>", "guide-entrees.html#autotoc_md106", null ],
      [ "La langue de l'interface : <a class=\"el\" href=\"classhmi_1_1Localization.html\" title=\"hmi::Localization\">hmi::Localization</a>", "guide-entrees.html#autotoc_md107", null ],
      [ "Voir aussi", "guide-entrees.html#autotoc_md108", null ]
    ] ],
    [ "Rendu 2D : de la scène à l'écran", "guide-rendu.html", [
      [ "Vocabulaire de base : GPU, swap chain, back buffer", "guide-rendu.html#autotoc_md150", null ],
      [ "QRhi : une couche d'accès au GPU, pas un changement de cible", "guide-rendu.html#autotoc_md151", null ],
      [ "Les surfaces de dessin : un élément composé avec l'interface", "guide-rendu.html#autotoc_md152", null ],
      [ "Unités monde et pixels : <a class=\"el\" href=\"classhmi_1_1Camera2D.html\" title=\"hmi::Camera2D\">hmi::Camera2D</a>", "guide-rendu.html#autotoc_md153", [
        [ "Cadrer une scène : <span class=\"tt\">fitZoom</span>, <span class=\"tt\">worldCamera</span>, <span class=\"tt\">arenaCamera</span>", "guide-rendu.html#autotoc_md154", null ]
      ] ],
      [ "Le pipeline de dessin de sprites : <a class=\"el\" href=\"classhmi_1_1SpriteBatch.html\" title=\"hmi::SpriteBatch\">hmi::SpriteBatch</a>", "guide-rendu.html#autotoc_md155", [
        [ "Pourquoi « batcher » plutôt que dessiner un sprite à la fois", "guide-rendu.html#autotoc_md156", null ],
        [ "<a class=\"el\" href=\"structhmi_1_1SpriteQuad.html\" title=\"hmi::SpriteQuad\">SpriteQuad</a> : un rectangle texturé", "guide-rendu.html#autotoc_md157", null ],
        [ "Sommets, shaders, et échantillonnage <em>nearest</em>", "guide-rendu.html#autotoc_md158", null ],
        [ "<a class=\"el\" href=\"structhmi_1_1LineQuad.html\" title=\"hmi::LineQuad\">LineQuad</a> : un segment orienté", "guide-rendu.html#autotoc_md159", null ]
      ] ],
      [ "Les textures : atlas procédural, fichiers et replis", "guide-rendu.html#autotoc_md160", [
        [ "<a class=\"el\" href=\"classhmi_1_1TextureAtlas.html\" title=\"hmi::TextureAtlas\">hmi::TextureAtlas</a> : l'atlas des couleurs plates", "guide-rendu.html#autotoc_md161", null ],
        [ "Les textures depuis fichiers", "guide-rendu.html#autotoc_md162", null ],
        [ "Ce qui manque se voit", "guide-rendu.html#autotoc_md163", null ],
        [ "L'animation : des clips en données", "guide-rendu.html#autotoc_md164", null ]
      ] ],
      [ "Composer, puis soumettre", "guide-rendu.html#autotoc_md165", [
        [ "Les calques : un ordonnancement unique", "guide-rendu.html#autotoc_md166", null ],
        [ "La profondeur : trier par le pied (<span class=\"tt\">LOT-07</span>)", "guide-rendu.html#autotoc_md167", null ],
        [ "Ne dessiner que ce qui se voit : le culling", "guide-rendu.html#autotoc_md168", null ],
        [ "Lecture seule", "guide-rendu.html#autotoc_md169", null ]
      ] ],
      [ "Assembler la frame complète", "guide-rendu.html#autotoc_md170", null ],
      [ "Voir aussi", "guide-rendu.html#autotoc_md171", null ]
    ] ],
    [ "Journalisation et assertions", "guide-journalisation.html", [
      [ "Pourquoi journaliser dans un jeu vidéo", "guide-journalisation.html#autotoc_md119", null ],
      [ "Les niveaux de gravité : <a class=\"el\" href=\"namespacecore.html#aa9b5a444ee11933c91d5e3235aa5b5e3\" title=\"core::LogLevel\">core::LogLevel</a>", "guide-journalisation.html#autotoc_md120", null ],
      [ "<a class=\"el\" href=\"classcore_1_1Logger.html\" title=\"core::Logger\">core::Logger</a> : filtrer puis diffuser", "guide-journalisation.html#autotoc_md121", null ],
      [ "Les sinks : où finissent les messages", "guide-journalisation.html#autotoc_md122", null ],
      [ "Les macros de journalisation, par catégorie", "guide-journalisation.html#autotoc_md123", [
        [ "Chaque module a sa propre catégorie", "guide-journalisation.html#autotoc_md124", null ],
        [ "Une règle de performance à respecter", "guide-journalisation.html#autotoc_md125", null ]
      ] ],
      [ "Le format d'une ligne : <a class=\"el\" href=\"namespacecore.html#aafa85d91ee91c84b123fff01f147615e\" title=\"core::formatLogLine\">core::formatLogLine</a>", "guide-journalisation.html#autotoc_md126", null ],
      [ "Configurer le niveau minimal au lancement", "guide-journalisation.html#autotoc_md127", [
        [ "Bootstrap réel : sinks différents en développement et en Release", "guide-journalisation.html#autotoc_md128", null ]
      ] ],
      [ "Assertions : <a class=\"el\" href=\"Assert_8h.html#a2811f3d423b22f1a90179b4ed095f054\" title=\"JADG_ASSERT\">JADG_ASSERT</a>, un outil différent", "guide-journalisation.html#autotoc_md129", null ],
      [ "Voir aussi", "guide-journalisation.html#autotoc_md130", null ]
    ] ],
    [ "Éditeur de niveaux", "guide-editeur.html", [
      [ "Le problème : éditer une carte sans (re)coder le moteur", "guide-editeur.html#autotoc_md85", null ],
      [ "<a class=\"el\" href=\"classcore_1_1LevelDraft.html\" title=\"core::LevelDraft\">core::LevelDraft</a> : une carte qu'on peut défaire", "guide-editeur.html#autotoc_md86", null ],
      [ "<a class=\"el\" href=\"classcore_1_1LevelWriter.html\" title=\"core::LevelWriter\">core::LevelWriter</a> : l'inverse du chargement", "guide-editeur.html#autotoc_md87", null ],
      [ "Peindre, c'est convertir un pixel en case", "guide-editeur.html#autotoc_md88", [
        [ "La palette et les outils : des panneaux Qt séparés du canevas", "guide-editeur.html#autotoc_md89", null ],
        [ "Quatre outils, une même grille : <a class=\"el\" href=\"namespacehmi.html#a02048ad8ad69a87a10e8307ac2bd68dd\" title=\"hmi::EditorTool\">EditorTool</a>", "guide-editeur.html#autotoc_md90", null ],
        [ "Peindre par lot sans dupliquer la logique de peinture : <a class=\"el\" href=\"classcore_1_1LevelDraft.html#a7fb667e044067f5e9bf16f747d3f5f8e\" title=\"core::LevelDraft::paintRegion\">LevelDraft::paintRegion</a>", "guide-editeur.html#autotoc_md91", null ]
      ] ],
      [ "Annuler/refaire : pourquoi des instantanés complets", "guide-editeur.html#autotoc_md92", null ],
      [ "Maquetter, jouer, puis habiller", "guide-editeur.html#guide-editeur-maquette", null ],
      [ "Essai immédiat : jouer sans quitter l'éditeur", "guide-editeur.html#autotoc_md93", null ],
      [ "Enregistrer : valider avant d'écrire, jamais l'inverse", "guide-editeur.html#autotoc_md94", null ],
      [ "Garde-fous contre la perte de travail", "guide-editeur.html#autotoc_md95", null ],
      [ "Cadrer une carte plus grande que la fenêtre", "guide-editeur.html#autotoc_md96", null ],
      [ "Gérer ses fichiers de niveaux", "guide-editeur.html#autotoc_md97", null ],
      [ "Voir aussi", "guide-editeur.html#autotoc_md98", null ]
    ] ],
    [ "Écrans, navigation et boucle de jeu", "guide-ecrans.html", [
      [ "La machine à états : <span class=\"tt\">hmi::ScreenFlow</span>", "guide-ecrans.html#autotoc_md69", null ],
      [ "Le routeur et la pile d'écrans", "guide-ecrans.html#autotoc_md70", null ],
      [ "La vue de jeu et la session qui lui survit", "guide-ecrans.html#autotoc_md71", null ],
      [ "Pause", "guide-ecrans.html#autotoc_md72", null ],
      [ "Ce que le <span class=\"tt\">LOT-67</span> a retiré", "guide-ecrans.html#autotoc_md73", null ],
      [ "Où ça s'insère dans la boucle", "guide-ecrans.html#autotoc_md74", null ],
      [ "Voir aussi", "guide-ecrans.html#autotoc_md75", null ]
    ] ],
    [ "IHM Qt — deux applications, deux technologies", "guide-ihm-qt.html", [
      [ "Pourquoi deux binaires", "guide-ihm-qt.html#autotoc_md109", null ],
      [ "Les trois couches du jeu", "guide-ihm-qt.html#autotoc_md110", null ],
      [ "Les trois modules QML", "guide-ihm-qt.html#autotoc_md111", [
        [ "Éditer un écran sans rien reconstruire", "guide-ihm-qt.html#autotoc_md112", null ],
        [ "Ouvrir les écrans pour les dessiner", "guide-ihm-qt.html#autotoc_md113", null ]
      ] ],
      [ "La surface de rendu", "guide-ihm-qt.html#autotoc_md114", null ],
      [ "La navigation", "guide-ihm-qt.html#autotoc_md115", null ],
      [ "Les réglages, et ce qu'ils atteignent", "guide-ihm-qt.html#autotoc_md116", null ],
      [ "Vérifier une interface sans la regarder", "guide-ihm-qt.html#autotoc_md117", null ],
      [ "Voir aussi", "guide-ihm-qt.html#autotoc_md118", null ]
    ] ],
    [ "Concevoir les écrans dans Qt Design Studio", "guide-conception-qds.html", [
      [ "En une phrase", "guide-conception-qds.html#autotoc_md47", null ],
      [ "Ce que le projet vous montre, et ce qu'il vous cache", "guide-conception-qds.html#autotoc_md48", null ],
      [ "La règle des deux fichiers", "guide-conception-qds.html#autotoc_md49", null ],
      [ "Les jetons : le seul endroit où s'écrit une couleur", "guide-conception-qds.html#autotoc_md50", [
        [ "Le facteur d'agrandissement", "guide-conception-qds.html#autotoc_md51", null ]
      ] ],
      [ "Les données d'exemple", "guide-conception-qds.html#autotoc_md52", null ],
      [ "Les écrans dessinés mais pas encore alimentés", "guide-conception-qds.html#autotoc_md53", null ],
      [ "Les contrôles Qt prennent la couleur des jetons", "guide-conception-qds.html#autotoc_md54", null ],
      [ "La bibliothèque de composants", "guide-conception-qds.html#autotoc_md55", null ],
      [ "Les modules que vous pouvez importer", "guide-conception-qds.html#autotoc_md56", null ],
      [ "Les ornements se tracent, ils ne se collent pas", "guide-conception-qds.html#autotoc_md57", null ],
      [ "Les textes", "guide-conception-qds.html#autotoc_md58", null ],
      [ "Ce qui demande encore un développeur", "guide-conception-qds.html#autotoc_md59", null ],
      [ "Voir aussi", "guide-conception-qds.html#autotoc_md60", null ]
    ] ],
    [ "Système de design et architecture de l'information", "guide-design-ihm.html", [
      [ "L'éditeur : un outil, pas un produit", "guide-design-ihm.html#autotoc_md61", null ],
      [ "Architecture de l'information : ce qui informe reste, ce qui commande est unique", "guide-design-ihm.html#autotoc_md62", [
        [ "Une barre d'état structurée", "guide-design-ihm.html#autotoc_md63", null ],
        [ "Des panneaux groupés, et qui suivent l'outil", "guide-design-ihm.html#autotoc_md64", null ],
        [ "Un état, un contrôle", "guide-design-ihm.html#autotoc_md65", null ]
      ] ],
      [ "Deux identités, deux règles d'échelle (LOT-66)", "guide-design-ihm.html#autotoc_md66", [
        [ "Pourquoi le facteur reste entier après la sortie du pixel art", "guide-design-ihm.html#autotoc_md67", null ]
      ] ],
      [ "Voir aussi", "guide-design-ihm.html#autotoc_md68", null ]
    ] ],
    [ "Audio", "guide-audio.html", [
      [ "La règle d'or, une fois de plus", "guide-audio.html#autotoc_md32", null ],
      [ "Le socle : <span class=\"tt\">hmi::AudioEngine</span>", "guide-audio.html#autotoc_md33", null ],
      [ "Le volume : de l'écran des options au moteur", "guide-audio.html#autotoc_md34", null ],
      [ "Provisionnement : Qt Multimedia", "guide-audio.html#autotoc_md35", null ],
      [ "Voir aussi", "guide-audio.html#autotoc_md36", null ]
    ] ]
];