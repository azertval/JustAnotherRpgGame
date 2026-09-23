pragma Singleton
import QtQuick

/*!
    Jetons de design des ecrans du jeu -- la SOURCE UNIQUE de l'apparence (LOT-86, LOT-87).

    Ce fichier est ecrit a la main et appartient a la conception. Il n'est engendre par rien, et
    rien ne le recopie : c'est ce qui permet d'en changer une valeur sans compiler une ligne de
    C++. Le chassis d'edition a son propre jeu, en C++ chez l'editeur -- deux langages, deux
    binaires, aucun chemin entre eux : l'etancheite des deux portees est structurelle.

    Les couleurs sont nommees par ROLE, jamais par teinte. Un jeton `accent` survit a un changement
    de couleur ; un jeton qui s'appellerait `or` deviendrait un mensonge le jour ou l'accent passe
    au bleu.
*/
QtObject {
    id: tokens

    // --- Facteur d'agrandissement de la charte v2 (LOT-87) --------------------------------------
    //
    // REEL : le rapport entre la fenetre et la definition de conception, 1920 x 1080. La charte v2
    // n'a plus de filet d'un pixel a proteger -- ses cadres sont des images 9-patch produites a
    // 1080p, que Qt echantillonne a toute taille --, donc plus de raison de s'arreter aux entiers.
    //
    // L'application le lie a la taille de la fenetre (`Main.qml`). La valeur par defaut 1 est
    // celle de la CONCEPTION : Qt Design Studio dessine a 1080p, et y lit les tailles exactes de
    // l'echelle typographique ci-dessous.
    property real uiScale: 1

    // --- Facteur d'agrandissement ENTIER : l'ancien viewport ----------------------------------
    //
    // ENTIER, borne a [1, 3]. Il servait au viewport de la scene en pixel art ; depuis le LOT-103,
    // la scene se cadre elle-meme, a un facteur libre tire de la definition (une case = hauteur de
    // la fenetre / 10,8, EX-REN-013), et plus rien ne se regle sur lui. Il ne reste affiche que par
    // le panneau de diagnostic.
    //
    // L'application ecrit cette valeur au demarrage, calculee depuis la hauteur de la fenetre. La
    // valeur par defaut 2 est celle de la CONCEPTION : c'est ce que Qt Design Studio affiche, et
    // c'est le facteur d'une fenetre 720p.
    property int scale: 2

    // --- Couleurs : parchemin de Tanares (LOT-66, LOT-76, EX-IHM-070) --------------------------
    //
    // Chaque teinte est RELEVEE sur les feuilles de personnage de la source -- histogramme
    // quantifie des pages rendues -- et non choisie a vue. Une couleur inventee ressemble a la
    // source sans en venir, et rien ne le dit jamais.
    readonly property color background: "#d0c0a0"   // parchemin vieilli
    readonly property color surface: "#e0d0b0"      // champ de la feuille
    readonly property color surfaceAlt: "#f0e0d0"   // encadre clair
    readonly property color border: "#907030"       // brun dore
    readonly property color text: "#302000"         // encre sepia
    readonly property color textMuted: "#705020"    // encre delavee
    readonly property color accent: "#c0a060"       // or des filets
    readonly property color accentHover: "#c0b080"

    // Seul role qui ne vienne pas des feuilles : une feuille de personnage n'a pas d'etat
    // d'erreur a montrer. Rouge de garance assombri, tenant le contraste sur le parchemin --
    // signale ici comme non atteste plutot que passe sous silence.
    readonly property color error: "#8a2f20"

    // Cadre : un trait exterieur d'encre, un filet ornemental dore en retrait, et l'ombre portee.
    // Ce ne sont pas des biseaux -- la lumiere ne vient pas d'en haut a gauche, il n'y a pas de
    // relief a simuler. Ce qui doit rester lisible, c'est l'ECART entre le trait et le filet :
    // deux traits de meme valeur ne composent pas un encadrement, mais une bordure epaisse.
    readonly property color frameEdge: "#302000"
    readonly property color frameOrnament: "#907030"
    readonly property color frameShadow: "#705020"

    // Grenat des cabochons et de la plaque du bandeau de titre (LOT-76), releve sur deux angles
    // opposes du meme cabochon -- qui donnent la meme valeur.
    readonly property color gem: "#701010"
    readonly property color gemShadow: "#400000"

    // --- Couleurs : charte v2, relevees sur les maquettes (LOT-87, EX-IHM-070) -----------------
    //
    // Les roles du parchemin, de l'or et du grenat ci-dessus sont GARDES : ils viennent du corpus,
    // et les maquettes ne les contredisent pas (parchemin mesure #e4d4ac contre #e0d0b0). Ceux-ci
    // sont NOUVEAUX, et chacun est releve par `scripts/measure_mockup_palette.py` sur une zone
    // nommee d'une maquette -- la table complete est dans l'epic du LOT-87. `--check` echoue si
    // l'une de ces valeurs s'ecarte du releve.

    // Panneaux sombres : menu, options, credits, carte, HUD.
    readonly property color panel: "#0c0c0c"          // fond du panneau des credits (07)
    readonly property color panelRaised: "#141414"    // panneau pose sur la scene, quetes du HUD (01)
    readonly property color panelEdge: "#e4a43c"      // filet d'or qui borde un panneau sombre (05)
    readonly property color goldLight: "#fcd444"      // or eclaire des ornements (05)
    readonly property color gemLight: "#8c0404"       // face eclairee du grenat, entree active (06)

    // Texte pose sur un panneau sombre. `text` et `textMuted` restent l'encre du parchemin.
    readonly property color textOnPanel: "#fcfcfc"       // libelle d'entree du menu (06)
    readonly property color textOnPanelMuted: "#74747c"  // libelle desactive (05)

    // Camp d'un combattant, en texte sur un panneau sombre : journal de combat, ordre d'initiative
    // (LOT-87, T4.1). Le camp se lit AUSSI a sa marque (losange ou pastille) : jamais a sa seule
    // teinte, qu'un joueur qui distingue mal le bleu du rouge ne lirait pas.
    readonly property color textAlly: "#74e4fc"   // tour du joueur, journal du HUD (01)
    readonly property color textEnemy: "#ac443c"  // tour de l'ennemi, journal du HUD (01)

    // Semantiques : la MATIERE des plaques d'action, face eclairee. Le texte pose dessus est
    // `textOnPanel` ; aucune de ces trois teintes n'est lisible en texte sur `panel`.
    readonly property color success: "#0c2c0c"   // plaque d'Appliquer (05)
    readonly property color danger: "#540c0c"    // plaque d'Annuler (05)
    readonly property color info: "#0c141c"      // plaque de Par defaut (05)

    // --- Typographie ---------------------------------------------------------------------------
    //
    // Les familles sont nommees, pas chargees ici : l'application enregistre les TTF au demarrage
    // et Qt Design Studio les prend dans `FontFiles` du .qmlproject. Les deux voient donc les
    // memes noms. Changer de police se fait ICI, sans toucher au C++.
    //
    // Charte v2 (LOT-87) : `Cinzel` en titres, `IM Fell English` en corps. Deposees et enregistrees
    // au T2.3 (Source/Elements/Assets/Fonts/, registerIdentityFonts()), a cote de la charte v1 --
    // qui reste en service tant que la phase 3 n'a pas transcrit les quatorze ecrans existants.
    readonly property string bodyFamily: "IM Fell English"

    // Titres, plaques et bandeaux : capitales romaines, trop solennelles pour du corps de texte.
    readonly property string titleFamily: "Cinzel"

    // Citations et textes d'ambiance, en italique (`font.italic: true`) : la meme famille que le
    // corps, dont l'italique est un fichier a part. Un role distinct quand meme, pour qu'une
    // citation puisse changer de voix sans que le corps la suive.
    readonly property string loreFamily: "IM Fell English"

    // Signatures a la plume (fiche de personnage) : une anglaise calligraphiee, jamais pour un
    // texte a lire -- seulement pour un nom que le personnage aurait signe lui-meme.
    readonly property string signatureFamily: "Pinyon Script"

    // --- Echelle typographique de la charte v2 (LOT-87) ----------------------------------------
    //
    // En pixels A 1080p, multipliee ici par `uiScale` : a la conception (uiScale = 1) un
    // formulaire lit exactement 58, 36, 24, 18 et 14 ; en jeu, la meme ecriture suit la fenetre.
    // Arrondie, parce qu'une taille de glyphe fractionnaire rend un texte flou sans rien gagner.
    // Prefixe `font` : les noms courts sont encore pris par la charte v1, jusqu'au T5.2.
    readonly property int fontDisplay: Math.round(58 * uiScale)       // titre du jeu, logo
    readonly property int fontScreenTitle: Math.round(36 * uiScale)   // plaque de titre d'ecran
    readonly property int fontSectionTitle: Math.round(24 * uiScale)  // bandeau de section
    readonly property int fontBody: Math.round(18 * uiScale)          // libelles et corps
    readonly property int fontCaption: Math.round(14 * uiScale)       // legendes, aides, version

    // --- Espacements et trait de la charte v2 (LOT-87, T2.7) -----------------------------------
    //
    // Meme regle que l'echelle typographique : ecrits a 1080p, multiplies ici par `uiScale`, et
    // jamais par l'entier.
    readonly property real gapSmall: 8 * uiScale        // entre un libelle et sa valeur
    readonly property real gapMedium: 16 * uiScale      // entre deux controles d'une section
    readonly property real gapLarge: 32 * uiScale       // entre deux sections, retrait d'un panneau

    // Le trait des aplats de repli -- ce que les briques dessinent tant que l'image produite de
    // leur piece n'est pas livree. Jamais sous un pixel : un filet de 0,7 px disparait.
    readonly property real strokeWidth: Math.max(1, Math.round(2 * uiScale))
}
