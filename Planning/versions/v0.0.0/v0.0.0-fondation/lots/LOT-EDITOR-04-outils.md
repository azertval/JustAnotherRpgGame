+++
id = "LOT-EDITOR-04"
titre = "Les outils du peintre"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "M"
resume = "On trace vite : ligne, seau, gomme, pipette et miroir, mesure, notes d'auteur et essai à la case survolée, chaque geste valant un seul pas d'annulation."
prerequis = ["LOT-EDITOR-03"]
livrables = [
  "`core::LevelDraft::beginGesture`, `endGesture`, `inGesture` et `core::GestureScope` ; le graphe du monde ignore les annexes.",
  "`Editor/Logic` : `PaintTools` (trait, rectangle, ligne, seau, pipette, miroir, mesure) et `EditorTool` à dix outils.",
  "`EditorSidecar` : l'annexe `<carte>.editor.json` et ses notes d'auteur ; les opérations de fichier emportent l'annexe.",
  "`Editor/Ui` : une touche par outil, menu Tools, miroir (`M`), `Alt` + clic, aperçu de la ligne et de la mesure, pastilles des notes, axe du miroir, essai depuis la case (`Shift+P`).",
  "Tests : `test_paint_tools.cpp` (7), `test_editor_sidecar.cpp` (3), `test_level_draft_editing.cpp` (+1), `test_editor_status.cpp` (+1) ; `test_level_schema.py` ignore les annexes.",
  "`EX-EDIT-066` à `EX-EDIT-069`.",
]
criteres = [
  "Tracer une maison de Martpart (sol, murs, porte, seuil) prend moins de dix gestes, sans ouvrir la couche collision ni un formulaire.",
  "Chaque outil est une fonction pure, avec un test ; un geste = un pas.",
  "Notes d'auteur dans `<carte>.editor.json`.",
]
+++

## Pourquoi

Tracer vite : seau, ligne, pipette, gomme et miroir, plus la mesure en cases et en pieds, les notes
d'auteur et l'essai lancé à la case survolée. Chaque outil est une fonction pure, avec un test ; un
geste est un pas d'annulation.

Feuille de route : [éditeur](../../../../vision/archives/feuille-de-route-editeur.md). Le lot alimente `LOT-EDITOR-13` (sans fenêtre)
et `LOT-EDITOR-08` (tampons).

## Ce que le dépôt contenait à l'ouverture (19 septembre 2026)

- **Quatre outils** : pinceau, rectangle, sélection, entité, sans touche propre ; la gomme était un
  bouton de la palette (`LOT-EDITOR-03`).
- **Un glisser du pinceau empilait un pas par case** : `applyBrush` pousse un pas à chaque appel,
  et le canevas l'appelait à chaque case survolée. Défaire un trait de vingt cases demandait vingt
  `Ctrl+Z`.
- **Le fichier annexe `<carte>.editor.json`** était prévu par la feuille de route et déjà écarté
  par `LevelEditor --check` ; le navigateur de cartes et le graphe du monde l'auraient pris pour
  une carte.

## Périmètre

### Livraison

| Partie | Contenu |
|---|---|
| `Core` | `LevelDraft::beginGesture`, `endGesture`, `inGesture`, `core::GestureScope` ; le graphe du monde ignore les annexes |
| `Editor/Logic` | `PaintTools` (trait, rectangle, ligne, seau, pipette, miroir, mesure), `EditorSidecar` (annexe et notes) ; `EditorTool` à dix outils ; barre d'état : miroir, mesure, note survolée ; les opérations de fichier emportent l'annexe |
| `Editor/Ui` | outils et touches, menu Tools, miroir, `Alt` + clic, aperçu de la ligne et de la mesure, pastilles des notes, axe du miroir, essai depuis la case ; la palette montre le pinceau pris |
| Tests | `test_paint_tools.cpp` (7), `test_editor_sidecar.cpp` (3), `test_level_draft_editing.cpp` (+1), `test_editor_status.cpp` (+1) ; `test_level_schema.py` ignore les annexes |

### Ce qui reste hors du lot, nommément

- **Refléter selon une ligne de la grille** : aucune planche n'a la jumelle d'un mur dans cette
  direction. Le miroir des tampons est au `LOT-EDITOR-08`.
- **Le seau de pièces larges** : le rectangle les pave.
- **Remapper les touches des outils** : elles sont écrites en dur, hors de `EditorKeyBindings` ; à
  revoir si une touche gêne.
- **Rejouer ces gestes sans fenêtre** (`--apply`) : `LOT-EDITOR-13`, qui appellera ces fonctions
  telles quelles.

## Conception

- **Un geste du brouillon.** `core::LevelDraft` gagne `beginGesture` / `endGesture` (et
  `core::GestureScope`) : entre les deux, seule la première mutation qui change la carte empile un
  pas, un geste sans effet n'en empile aucun, la révision change à chaque mutation comme avant.
  Du clic au relâchement, un trait du pinceau ou de la gomme est un geste ; la ligne, le seau, le
  rectangle et leur reflet aussi.
- **Les outils ne réécrivent pas le pinceau.** Chaque outil calcule ses cases (`lineCells`,
  `floodRegion`) puis passe par `hmi::applyBrush` du `LOT-EDITOR-03`, case par case : une pièce va
  sur sa couche, la collision suit, une couche verrouillée refuse, sans règle nouvelle.
- **La ligne** suit Bresenham à huit voisins ; ses cases après la première prolongent le trait,
  si bien qu'une pièce large ne s'y décale pas.
- **Le seau** remplit les cases reliées par un côté qui portent **le même contenu** (type et pièce)
  sur la couche que vise le pinceau ; une case couverte par une pièce large n'est égale qu'aux
  autres cases de cette pièce. Il ne verse que des pièces d'une case : une pièce large déborderait
  de la région, le rectangle la pave.
- **La pipette** prend ce qu'on voit : la collision active, son type ; sinon la couche active,
  puis les autres couches visuelles de l'avant vers l'arrière — la pièce qui couvre la case (son
  ancre pour une pièce large), à défaut son type. Elle rend active la couche lue et la palette
  montre le pinceau pris. L'outil Pipette (`I`) rend la main à l'outil du peintre d'avant ;
  `Alt` + clic fait de même depuis n'importe quel outil, sans en changer.
- **La gomme devient un outil** (`E`) : elle gomme au clic et au glisser, quel que soit le pinceau
  armé, qui reste armé pour la suite. Le bouton de la palette part. `Suppr` gomme la sélection,
  sur la couche active, en un pas.
- **Le miroir est l'axe vertical de l'écran iso.** En cases, c'est la diagonale
  `colonne − ligne = k` : (c, r) a pour reflet (r + k, c − k), une emprise c × r devient r × c.
  C'est le seul miroir que les planches connaissent : `mirrorOf` du manifeste lie `wall-left` à
  `wall-right` et `front-left` (1 × 2) à `front-right` (2 × 1). Le reflet pose la **jumelle** ; une
  pièce sans jumelle se pose telle quelle. `M` l'active, par la case survolée (le centre de la carte
  à défaut) ; l'axe se dessine en vert d'eau. Un geste qui **chevauche son reflet** — une case sur
  l'axe, une emprise ou un rectangle qui le traverse — ne se reflète pas : le reflet écraserait ce
  qu'on vient de poser. Un miroir selon une ligne de la grille aurait été plus simple, mais aucune
  planche ne sait refléter un mur dans cette direction.
- **La mesure** (`D`) tire un trait entre deux cases : l'étendue en cases, bornes comprises, et la
  distance, une diagonale comptant une case comme au combat (`core::ReachableArea`), 5 pieds la
  case. Elle reste affichée jusqu'au prochain trait ou au changement d'outil.
- **Les notes d'auteur** (`N`) s'épinglent à une case, une par case, et vivent dans
  `<carte>.editor.json` à côté de la carte. Elles ne sont pas la carte : ni annulation, ni
  « modifié » ; elles s'écrivent dès qu'on les change, et une annexe vide retire son fichier.
  L'annexe s'écrit de façon canonique et garde les clés qu'elle ne connaît pas (les régions
  verrouillées viendront). Renommer, dupliquer ou supprimer une carte emporte ses notes. Le
  navigateur de cartes et le graphe du monde ignorent `*.editor.json`, comme `--check`.
- **L'essai à la case survolée** (`Shift+P`) joue une copie du brouillon dont l'entrée est posée
  sur la case ; le brouillon ne bouge pas. Une case qui arrête le pas est refusée.
- **Une touche par outil** : `B` pinceau, `R` rectangle, `L` ligne, `G` seau (l'usage des logiciels
  de dessin), `E` gomme, `I` pipette, `S` sélection, `O` entité (objets), `D` mesure (distance),
  `N` note ; `M` miroir. Les touches ne sont pas remappables : elles sont des raccourcis de
  `QAction`, et un champ de texte les garde pour lui.
- **Choisir dans la palette, c'est vouloir peindre** : depuis la gomme, la pipette, la mesure, la
  note, la sélection ou l'entité, un choix de la palette rend la main à l'outil du peintre d'avant.

## Vérification

- **Tracer une maison de Martpart (sol, murs, porte, seuil) prend moins de dix gestes, sans ouvrir
  la couche collision ni un formulaire.** ✔ `PaintToolsTest.UneMaisonDeMartpartEnMoinsDeDixGestes` :
  sur un terrain vide, le décor actif et le miroir par l'angle, cinq gestes — un rectangle de rue,
  une ligne de `wall-right`, l'angle, une porte, un seuil — posent deux façades, deux portes et deux
  seuils ; cinq pas d'annulation, la collision égale la déduction, aucune case forcée.
- **Chaque outil est une fonction pure, avec un test ; un geste = un pas.** ✔ ligne
  (`LaLignePoseUneCaseParPas`), trait (`UnTraitEstUnSeulPas`), seau
  (`LeSeauRemplitUneRegionEnUnPas`, `LeSeauEtLesPiecesLarges`), pipette
  (`LaPipettePrendCeQuOnVoit`), miroir (`LeMiroirPoseLaJumelle`), mesure
  (`LaMesureEnCasesEtEnPieds`), geste du brouillon (`EditionDeCarteTest.UnGesteSeDefaitEnUnPas`).
- **Notes d'auteur dans `<carte>.editor.json`.** ✔ `EditorSidecarTest` : nom de l'annexe, écriture
  canonique relue à l'identique, clé inconnue gardée, annexe ignorée par le navigateur.

**Vérification au clavier et à la souris, faite** au [LOT-127](../../../v0.1.0/v0.0.1-demo/lots/LOT-127-recette-de-l-editeur-a-la-main.md), le 24 septembre 2026 : sur Martpart, `M` au-dessus d'un angle de rue,
`L` et une ligne de mur (le reflet paraît) ; `Alt` + clic sur une façade (la palette la montre) ;
`G` sur la place avec `street-3`, puis un seul `Ctrl+Z` ; `D` entre deux lanternes ; `N` sur une
case, puis rouvrir la carte (la pastille revient) ; `Shift+P` sur une rue. Les touches et les clics
postés à la fenêtre n'atteignent pas Qt : même `F9`, qui marchait au `LOT-EDITOR-02`, n'a rien
fait dans la capture. Cette vérification ne se fait pas sans la main de l'auteur.

## Bilan

**Livré le 19 septembre 2026** (ouvert le même jour), sur la branche `lot-editor-04-outils`.
Vérification automatisée : construction `/W4 /WX` sans avertissement, tests unitaires verts (826,
dont deux ignorés comme avant), acceptation sur Martpart ; capture de la fenêtre (barre d'outils à
dix outils et miroir, menu Tools). **Vérifié au clavier et à la souris** au [LOT-127](../../../v0.1.0/v0.0.1-demo/lots/LOT-127-recette-de-l-editeur-a-la-main.md) : les touches et
les clics postés n'atteignent pas Qt (voir « Vérification »).

Exigences : `EX-EDIT-066` à `EX-EDIT-069` (nouvelles) ; `EX-EDIT-014` révisée.
