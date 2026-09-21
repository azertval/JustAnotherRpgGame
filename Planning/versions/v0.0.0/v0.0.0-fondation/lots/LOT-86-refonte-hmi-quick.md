+++
id = "LOT-86"
titre = "Refonte de l'IHM sur Qt Quick"
version = "0.0.0"
filiere = "interface"
statut = "livre"
taille = "XL"
resume = "Les interfaces du jeu deviennent modifiables par un artiste sans qu'il ouvre un fichier source : le jeu passe sur Qt Quick, la maquette et l'écran deviennent le même fichier."
prerequis = [
  "LOT-38",
  "LOT-68",
]
livrables = [
  "Deux exécutables : `JustAnotherRpgGame` (Qt Quick, ne lie pas `Qt6::Widgets`) et `LevelEditor` (Qt Widgets, dans son propre binaire).",
  "Les écrans du jeu en `.ui.qml` sous `Source/Ui/`, ouvrables dans Qt Design Studio par `Source/Ui/JadgUi.qmlproject`.",
  "`Source/Ui/Theme/Tokens.qml` : les jetons à un seul endroit ; les ornements tracés en `Shape` QML.",
  "La surface de rendu du jeu en `QQuickRhiItem` (QRhi, Direct3D 11).",
  "`scripts/check_ui_layers.py` (six règles) et les exigences `EX-IHM-100` à `EX-IHM-105`.",
  "Le sélecteur d'écrans de développement `Logic/ScreenProbe.qml`.",
  "La documentation : `interface-ihm.md` §11, `architecture.md`, les guides IHM et le nouveau `guide-conception-qds`.",
  "Le retrait de `.design-mockups/` et de l'ancienne couche Qt Widgets du jeu.",
]
criteres = [
  "**Test de l'artiste** — ouvrir `Source/Ui/JadgUi.qmlproject` dans Qt Design Studio, y déplacer un bloc d'un écran, changer une couleur dans `Tokens.qml`, remplacer un ornement par un autre SVG ; relancer le jeu avec `JADG_QML_FROM_SOURCE=1` et voir les trois changements, **sans qu'aucun compilateur C++ n'ait été lancé**.",
  "**Test du développeur** — le `git diff` du test précédent ne touche que `Source/Ui/**` et `Source/Elements/**`. Aucun `.cpp`, aucun `.h`, aucun fichier engendré.",
  "Deux exécutables : `JustAnotherRpgGame` (Qt Quick, ne lie pas `Qt6::Widgets`) et `LevelEditor`.",
  "`scripts/check_ui_layers.py` vert, et les quatre garde-fous devenus sans objet retirés.",
  "Build `/W4 /WX` sans avertissement",
  "`ctest` : **1011/1011**",
  "`qmllint` sans un seul avertissement sur les fichiers QML",
  "`clang-format` propre sur l'ensemble du dépôt",
  "Lint d'exigences (352/352), lint des lots, `check_ui_layers` (6 règles), `check_glossary`, `check_qt_version_pin` : verts",
  "Doxygen sans erreur",
  "Les quatorze écrans se chargent sans un seul avertissement QML, capturés et relus",
  "Les deux binaires démarrent",
  "**Les réglages atteignent le moteur** (`EX-IHM-083`) : plein écran, volume, langue et compteur de diagnostic immédiatement, synchronisation verticale au prochain lancement — et l'écran le dit",
  "Les contrôles Qt prennent la couleur des jetons : style « Basic » imposé, palette dérivée de `Tokens.qml`. Sans cela, `FluentWinUI3` les peignait en bleu et aucune retouche des jetons n'y pouvait rien",
  "Un sélecteur d'écrans de développement (`Logic/ScreenProbe.qml`) permet de parcourir les quatorze écrans tant qu'aucun niveau n'existe pour y mener ; il est lié à `ScreenRouter.developerBuild` et rend la main au routeur dès que le jeu navigue",
]
+++

## Pourquoi

**Rendre les interfaces du jeu modifiables par un artiste sans qu'il ouvre un fichier source.**

Pas « ranger le C++ », pas « QML est plus souple » : séparer la conception du code comme le fait
l'industrie, où ce sont deux métiers qui ne se marchent pas dessus. Tout ce que ce lot livre se juge
à ce critère, et à lui seul.

Corollaire tenu partout : **aucune surcouche**. Une couche qui n'existe que pour compenser
l'inadéquation d'une autre est le défaut qu'on corrige, pas la solution.

## Ce qui existait

**Côté code**, la couche Qt Widgets avait cessé d'être tenable :

- `MainWindow.cpp` faisait **2 472 lignes** et mêlait le poste de travail de l'éditeur, le viewport
  de rendu, la pile des écrans de jeu, l'échelle d'identité et la feuille de style.
- Le défaut du plancher de taille — `QStackedWidget::minimumSizeHint` vaut le maximum sur **toutes**
  les pages, y compris masquées — s'était produit **trois fois**. Corrigé deux fois écran par écran,
  puis masqué par un enveloppeur `ScreenPageHost`. Le guide en tirait lui-même la leçon : *« une
  règle qu'il faut se rappeler d'appliquer se reperd au premier écran ajouté »*.
- Le `LOT-85`, abandonné au profit de celui-ci, avait ajouté **1 268 lignes d'outillage** — plugin
  Qt Designer, résolveur de feuille de style, générateur de `.ui`, deux garde-fous — dont l'unique
  fonction était de rendre les `.ui` visualisables dans Qt Designer. Surcouche exemplaire : elle
  compensait le fait que le format `.ui` ne sait pas décrire ces écrans.
- Surface totale : **~23 700 lignes** (14 304 de C++ `Interface/`, 7 158 de `.ui`, 980 de feuilles
  de style, 766 d'outillage, 502 de scripts).

**Côté conception**, il n'y avait pas de séparation mais une transcription. `.design-mockups/`
portait des maquettes HTML/CSS dessinées à la main à 1280 × 720 ; la palette y était **écrite deux
fois**, en CSS et en C++, tenue par un lint dont le commentaire disait : *« Rien ne les reliait. Une
retouche de teinte d'un côté laissait l'autre en arrière sans qu'aucune Pull Request ne le
signale. »* Les libellés étaient recopiés depuis `fr.lang` **mot pour mot**, à la main.

Trois représentations du même écran, deux transcriptions manuelles, un garde-fou pour rattraper les
erreurs. L'artiste dessinait, un développeur transcrivait.

## Conception

### La direction artistique, reprise des maquettes supprimées

`.design-mockups/` disparaît avec ce lot : la maquette et l'écran deviennent le même fichier. Ses
décisions sont reportées ici, seule trace qui subsiste.

#### Attention : le texte des maquettes était périmé

Les planches nommaient la direction retenue « **Ambre nuit** » et la décrivaient comme « votre
bleu-nuit et votre ambre `#ffd133`, rendus en pixel art ». **Ce n'est plus l'identité du jeu.** Ce
texte datait du `LOT-68` ; les `LOT-66` et `LOT-76` ont depuis remplacé la portée identité par le
**parchemin de Tanares**, dont chaque teinte est *relevée* sur les feuilles de personnage de la
source — parchemin vieilli `#d0c0a0`, encre sépia `#302000`, or des filets `#c0a060`, grenat des
cabochons `#701010`. `#ffd133` n'est plus que l'accent du **châssis d'édition**.

C'est une illustration de ce que ce lot corrige : la maquette avait cessé de décrire le jeu, et le
seul contrôle qui les reliait comparait les couleurs — pas les mots. `Source/Ui/Theme/Tokens.qml`
porte désormais ces valeurs, à un seul endroit, et il n'y a plus de second texte à laisser périmer.

*Le prix, assumé et toujours vrai* : le parchemin est chaleureux et lisible, mais peu mémorable.
Personne ne reconnaîtra le jeu à sa capture d'écran.

#### Les deux directions écartées, et pourquoi

Gardées pour ne pas refaire le débat dans six mois.

- **B — Cyan cathodique.** Noir profond, cyan froid, lignes de balayage. La plus caractérisée : on
  sait en une seconde qu'on est devant un jeu et non un logiciel. *Écartée* : les lignes de balayage
  fatiguent sur de longues sessions, et le froid va mal aux écrans de fin de niveau, qui doivent
  être accueillants.
- **C — Néon arcade.** Magenta et cyan sur violet profond, la seule qui donne une couleur signature.
  *Écartée* : deux accents, c'est deux fois plus de règles à tenir — il faudrait décider une bonne
  fois lequel dit « action » et lequel dit « information ».

#### Décisions d'écran à honorer

- **Menu principal** — pas de cadre : les entrées se posent sur une scène à trois plans. La
  lisibilité tient à un dégradé sombre sur le tiers gauche plutôt qu'à un voile plein, pour que le
  décor reste visible.
- **Options** — compteur d'images par seconde en haut à droite, onglet Vidéo. Il remplace l'ancien
  sélecteur de limite d'images/s, grisé et jamais branché : afficher le chiffre est utile, imposer
  un plafond ne l'était pas. C'est un élément de HUD, il vit avec le HUD.
- **Fin de niveau** — le bilan (temps, morts, sauts) est retenu. **À savoir avant de démarrer :
  aucune de ces trois valeurs n'est comptée aujourd'hui.** Il faut les accumuler pendant la partie
  et les faire remonter, et le temps doit être mesuré en **pas de simulation** plutôt qu'en horloge
  murale pour rester comparable d'une machine à l'autre. C'est du travail de session de jeu, pas
  d'habillage.

## Exigences couvertes

Nouvelles, toutes vérifiées par `scripts/check_ui_layers.py` — c'est la condition pour qu'elles
soient des règles et non des intentions :

- [`EX-IHM-100`](@ref EX-IHM-100) — une modification purement visuelle, sans toucher au C++ ;
- [`EX-IHM-101`](@ref EX-IHM-101) — la présentation ne connaît ni Qt Quick ni Qt Widgets ;
- [`EX-IHM-102`](@ref EX-IHM-102) — le jeu ne lie pas `Qt6::Widgets` ;
- [`EX-IHM-103`](@ref EX-IHM-103) — écrans et contrôles en `.ui.qml`, sans code impératif ;
- [`EX-IHM-104`](@ref EX-IHM-104) — imports connus à la fois de Qt et de Qt Design Studio ;
- [`EX-IHM-105`](@ref EX-IHM-105) — aucun littéral d'apparence hors du thème.

Réutilisées et **verrouillées** par le même contrôle, sans être redéclarées :
[`EX-ARCH-001`](@ref EX-ARCH-001) et [`EX-NFR-010`](@ref EX-NFR-010) — `Core` sans un seul en-tête
Qt, vrai depuis le `LOT-01` et qu'un seul `QString` suffirait à rendre faux.

## Où en est le lot

**Fait et vérifié.**

- **Deux applications.** Le jeu en Qt Quick (`QGuiApplication`, ne lie pas `Qt6::Widgets`),
  l'éditeur de niveaux en Qt Widgets dans son propre binaire. L'ancienne couche est retirée du
  châssis d'édition (−3 879 lignes) et l'éditeur s'ouvre directement sur son espace de travail.
- **Les treize écrans** existent en QML. Fiche et inventaire branchés sur de vraies données ; sept
  écrans du RPG et la page Options dessinés avec **47 clés d'attribution** vers l'ancre
  `hmi::PendingData` ; menu, pause et crédits fonctionnels avec une navigation réelle.
- **Les jetons vivent en QML**, écrits à la main. Aucun générateur, aucun JSON intermédiaire :
  après la refonte, plus aucun C++ n'a besoin des couleurs d'identité.
- **Les ornements sont tracés**, portés en `Shape` QML depuis les géométries relevées sur le
  corpus — cadre à cabochons, bandeau à ailes, fleuron de focus.
- **Le jeu est traduisible.** Le français est sa langue source ; 89 des 101 traductions anglaises
  ont été reprises du catalogue maison, le reste écrit à la main.
- **La surface de rendu** du jeu est un `QQuickRhiItem` : QRhi rend en Direct3D 11 dans une fenêtre
  Qt Quick, et le QML se compose par-dessus.
- **Six garde-fous**, chacun vérifié **en mordant** — violation injectée, contrôle rouge, arbre
  rendu propre ensuite.
- **Documentation** : `interface-ihm.md` §11, `architecture.md`, `guide-ihm-qt`, `guide-design-ihm`,
  `guide-ecrans`, `guide-editeur`, `guide-entrees`, et un nouveau `guide-conception-qds` — le mode
  d'emploi de la conception. Doxygen sans erreur.

**Reste, et il faut le dire.**

- **Le viewport n'affiche aucune scène.** `Source/Elements/Levels/` est vide par construction depuis
  le `LOT-01` : la plomberie est établie et vérifiée, la session se branchera quand il y aura une
  carte à jouer. Bâtir une session autour d'un niveau inexistant aurait produit du code que rien ne
  peut vérifier.
- **Les options ne sont pas branchées**, délibérément : `EX-IHM-083` exige qu'un réglage exposé
  atteigne le moteur, et il n'y a pas encore de viewport à régler.
- **L'éditeur garde le catalogue maison** pour ses textes : 188 sites d'appel, dont 14 à clé
  dynamique, pour un outil de développeur où aucun utilisateur ne verrait la différence.
- **Les noms de compétences restent français** en anglais : ils viennent des JSON de règles, qui
  n'ont qu'un champ `name`. C'est une limite de la **donnée**, pas de l'IHM.
- `MenuBackdropGeometry` et `KeyHintText` attendent leur portage en QML ; ils restent compilés et
  testés comme sources de ce portage.
- **Les modules `aqtinstall` de la CI** n'ont pas été touchés : ajouter un nom de module inconnu
  ferait échouer le provisionnement pour tout le monde. Si Qt Quick manque à l'image, rien ne passe
  inaperçu — `find_package` nomme les modules absents et le contrôle d'exécutable échoue franchement.

## Vérification

Dans l'epic d'origine, les quatre premiers critères — test de l'artiste, test du développeur, deux exécutables, `scripts/check_ui_layers.py` vert avec retrait des quatre garde-fous devenus sans objet — ne portent pas de coche ; tous les suivants sont cochés (✔).

## Bilan

Statut : **livré** par la PR #24 ; le [LOT-87](@ref lot-87) s'y adosse. Ce qui reste hors du lot est nommé dans « Où en est le lot ».
