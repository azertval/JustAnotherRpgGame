# Recette manuelle

Cette page est la **seule du cahier écrite à la main**. Elle recense ce qu'aucun test automatisé
ne remplace : un parcours à la manette, la fluidité d'un travelling, la lisibilité d'un écran, un
son qui sort. La [définition de « livré »](../../Planning/standards/definition-de-livre.md) en fait
un **critère d'acceptation** — un lot qui attend « une vérification à la souris » reste `en-cours`
tant qu'elle n'est pas faite, et sa fiche dit qui l'a faite et quand.

Chaque contrôle porte un identifiant `RM-…`, stable, cité par la fiche du lot qui le demande. Il
dit **quand** on le joue, **ce qu'on prépare**, **ce qu'on fait**, **ce qu'on regarde**, et ce qui
vaut **refus**. Un contrôle sans refus écrit n'en est pas un : on le passerait toujours.

| Quand | Ce que ça veut dire |
|---|---|
| **Version** | Avant de poser le tag d'une version (`Planning/versions/`), sur l'archive publiée, jamais sur le poste de développement. |
| **Lot** | Avant de passer un lot de la filière à `livre`, sur la branche du lot. |
| **Changement** | Dès qu'une PR touche la mécanique visée, en plus des tests. |

## 1. L'archive et le poste

### RM-001 — L'archive se lance sur un poste vierge

*Version.* Le test de fumée (`scripts/release/smoke_test_release.ps1`) prouve déjà que le jeu
extrait de l'archive rend une image et que `--crash-test` écrit un minidump ; il tourne sur le
runner, qui a tout ce qu'un poste peut avoir. Ce contrôle refait le trajet d'un joueur.

- **Préparer** : une machine Windows sans Qt, sans Visual Studio, sans le dépôt. L'archive
  `JustAnotherRpgGame-<version>-windows-x64.zip` de la release, et rien d'autre.
- **Faire** : extraire, double-cliquer sur `JustAnotherRpgGame.exe` ; ouvrir *Options*, changer la
  langue, *Appliquer*, quitter, relancer. Puis lancer `LevelEditor.exe`.
- **Regarder** : le menu principal paraît sans boîte de dialogue système ; la langue choisie est
  celle du second lancement ; `Logs/` contient un journal daté ; l'éditeur ouvre sa fenêtre sur le
  navigateur de cartes.
- **Refus** : une DLL manquante, une fenêtre noire, un réglage oublié entre deux lancements, un
  dossier créé ailleurs qu'à côté de l'exécutable
  ([où vit la partie](../Guide/Manuel/jouer.md#votre-partie-et-où-elle-vit)).

### RM-002 — Le journal de session dit ce qui s'est passé

*Version.* Le journal est ce qu'un joueur joint à un rapport ; il doit se lire sans le code.

- **Faire** : jouer cinq minutes, changer de carte, ouvrir un dialogue, quitter proprement.
- **Regarder** : dans `Logs/`, chaque ligne porte l'heure, le niveau et la catégorie
  ([le format d'une ligne](../Guide/guide-journalisation.md)) ; on y retrouve le chargement de la
  carte, le franchissement du portail, la fin de session ; aucune ligne `error` sans cause visible.
- **Refus** : une erreur sans suite, un avertissement répété à chaque image, un journal vide.

## 2. Le jeu, au clavier et à la manette

Les commandes sont celles du [manuel](../Guide/Manuel/jouer.md) ; le clavier suffit à tout
(`EX-CTRL-001`), la manette pilote les menus, la carte et l'arène (`EX-CTRL-002`).

### RM-010 — Les menus se parcourent sans souris

*Version, et changement sur `Source/Ui/`.*

- **Faire** : depuis le menu principal, tout parcourir aux flèches et à *Entrée* : *Nouvelle
  partie*, *Options* et ses trois onglets, *Crédits*, retour par *Échap*. Refaire à la manette
  (croix, *A*, *B*).
- **Regarder** : l'entrée en surbrillance est toujours visible, une seule à la fois ; *Échap* et
  *B* reviennent d'un cran et jamais de deux ; *Continuer* et *Charger une partie* sont grisées
  sans sauvegarde et le restent au focus.
- **Refus** : un écran d'où l'on ne sort qu'à la souris, un focus perdu après un onglet, une
  entrée grisée qui s'active.

### RM-011 — Les options font ce qu'elles disent

*Changement sur les options.* Le jeu s'interdit de montrer un réglage inopérant.

- **Faire** : onglet *Graphismes*, cocher *plein écran*, *Appliquer* ; revenir, *Par défaut*,
  *Annuler*. Onglet *Audio*, baisser le volume à zéro, *Appliquer*.
- **Regarder** : le plein écran bascule à *Appliquer*, pas avant ; *Annuler* rétablit ce qui était
  affiché à l'ouverture ; *Par défaut* remet chaque contrôle à sa valeur d'origine ; le volume à
  zéro coupe le moteur audio (le journal le dit, même sans son à jouer).
- **Refus** : un réglage appliqué sans *Appliquer*, un *Annuler* qui garde une modification.

### RM-012 — La pause fige tout

*Changement sur la boucle ou les écrans.*

- **Faire** : en exploration, lancer le héros contre un mur et appuyer sur *Échap* en pleine
  marche ; attendre dix secondes ; *Reprendre*.
- **Regarder** : le héros est exactement où il était, orienté comme il l'était ; aucune
  animation de scène ne tourne derrière l'écran assombri ; les trois choix se parcourent aux
  flèches.
- **Refus** : un pas fait pendant la pause, une orientation revenue au défaut.

### RM-013 — La carte à trois niveaux

*Changement sur l'écran « Carte ».*

- **Faire** : ouvrir la carte depuis le bandeau ; descendre du monde à une région, de la région à
  une ville, aux flèches puis à la manette ; remonter par *Échap*.
- **Regarder** : le repère courant est lisible sur les trois fonds ; la fiche de gauche suit le
  repère ; le retour remonte d'un niveau et garde le repère d'où l'on vient.
- **Refus** : un repère que les flèches n'atteignent pas, une descente sans retour.

### RM-014 — Le journal de quêtes

*Lot de la filière quêtes.* L'écran se tire des seuls drapeaux (`EX-EXP-010`).

- **Faire** : jouer la quête de la démo jusqu'à sa deuxième étape ; ouvrir le journal ; changer de
  quête par *Haut* et *Bas* ; refermer par *Échap* ; changer la langue et rouvrir.
- **Regarder** : la quête est là, à l'état *en cours*, l'entrée montrée est celle de la dernière
  étape atteinte, les étapes franchies sont cochées ; le titre et les étapes existent dans les deux
  langues, sans clé nue.
- **Refus** : une étape cochée avant d'être jouée, une clé `quest.…` affichée telle quelle.

## 3. La scène en haute définition

Les tests de rendu comparent des listes de quads et des images de référence ; ils ne voient ni
le scintillement ni la saccade. C'est l'œil qui juge, et il juge sur l'image du GPU, pas sur le
rendu logiciel de l'éditeur.

### RM-020 — Le travelling ne scintille pas

*Lot de la filière assets HD, et changement sur `hmi::SpriteBatch` ou les textures.* Dernier
critère du `LOT-103`, fait par l'auteur avant de le livrer.

- **Préparer** : une carte HD (Arenarea) et une carte maquette (sans texture), à 1280 × 720 puis en
  plein écran.
- **Faire** : traverser la carte lentement en diagonale, puis à la vitesse de marche ; s'arrêter
  contre un mur ; regarder les arêtes des dalles, les bords des murs, les lignes de toiture.
- **Regarder** : aucune arête ne bat ni ne clignote pendant le déplacement ; les joints entre
  dalles restent fermés ; les figurines ne « tremblent » pas d'un pixel à l'arrêt.
- **Refus** : un scintillement sur les arêtes, un joint qui s'ouvre à certains zooms, un moiré sur
  les sols réduits.

### RM-021 — Le zoom et le travelling restent fluides

*Lot de la filière assets HD, et changement sur la caméra.* Dernier critère du `LOT-125`.

- **Préparer** : l'éditeur avec **trois onglets HD ouverts** (les deux quartiers de la Capitale et
  l'Arena of Fate), puis le jeu sur la même carte.
- **Faire** : zoomer de bout en bout à la molette, en continu ; faire un travelling d'un bord à
  l'autre au clavier ; répéter dans chaque onglet.
- **Regarder** : le compteur de diagnostic des options reste au-dessus de 60 images par seconde
  (`EX-NFR-001`) ; le zoom ne « saute » pas de palier ; le canevas suit la molette sans retard
  perceptible.
- **Refus** : une saccade à un niveau de zoom, un retard entre la molette et l'image, une image
  par seconde qui tombe quand un onglet de plus est ouvert.

### RM-022 — L'étage qui masque le héros devient translucide

*Lot de la filière cartes.* `EX-LVL-025` ; le seuil de transparence est un jugement.

- **Faire** : sur une carte à étage (fixtures `Storeys/` ou un quartier de la Capitale), passer
  sous un balcon puis derrière une façade à deux étages ; s'arrêter dessous.
- **Regarder** : le héros reste visible en tout point de son trajet ; l'étage redevient opaque dès
  qu'il ne le masque plus ; la toiture ne clignote pas au passage d'une case à l'autre.
- **Refus** : un héros invisible sous un étage, un étage qui reste translucide une fois dégagé, une
  transparence qui bat à la frontière d'une case.

### RM-023 — Une carte maquette se lit sans texture

*Lot de la filière cartes.* La maquette est ce que l'on joue avant d'habiller (`LOT-128`).

- **Faire** : ouvrir une carte dont le lieu n'a aucune pièce dessinable ; la parcourir.
- **Regarder** : chaque type de case a **une** couleur, distincte de ses voisines ; les murs sont
  extrudés ; les jetons sont vert (héros), jaune (PNJ à dialogue ou conditionné), rouge
  (rencontre) ; un portail est une flèche ; une zone est un contour pointillé.
- **Refus** : deux types confondus, une case sans couleur, un jeton sans lettre lisible.

## 4. L'éditeur, geste par geste

La définition de « livré » de la filière éditeur est explicite : **le geste est fait à la main par
l'auteur**. Les scénarios `--apply` rejouent la logique ; ce contrôle rejoue la main.

### RM-030 — Le parcours du manuel, du sol à la publication

*Lot de la filière éditeur.* Suivre [Utiliser l'éditeur](../Guide/Manuel/utiliser-l-editeur.md),
étape par étape, sans en sauter une.

- **Faire** : créer une carte avec son lieu, poser le sol, dresser le relief, placer l'entrée,
  poser un PNJ, un portail et sa paire retour, une zone ; essayer par *P* ; enregistrer ;
  contrôler ; relancer par *F5*.
- **Regarder** : chaque panneau suit l'outil actif ; la barre d'état dit la case sous le curseur ;
  l'essai s'ouvre sur la carte au point choisi ; le contrôle ne signale que ce qu'on a
  volontairement laissé.
- **Refus** : une étape du manuel qui ne se fait plus comme le manuel le dit — c'est alors le
  manuel, ou l'éditeur, qui doit changer avant la livraison.

### RM-031 — Un outil nouveau, à la souris et au clavier

*Lot qui ajoute un outil ou une propriété.*

- **Faire** : l'outil, à la souris ; puis son raccourci ; puis le même geste sur une case hors de
  la carte, sur une couche cachée, sur une couche verrouillée ; annuler, refaire.
- **Regarder** : le geste hors carte ne fait rien et ne dit rien de plus qu'un message d'état ; la
  couche cachée ne se peint pas ; *annuler* rend exactement l'état d'avant, en une fois.
- **Refus** : un geste qui écrit sur une couche cachée, une annulation en deux temps.

### RM-032 — L'état de partie de l'essai

*Lot de la filière quêtes ou éditeur.* *Map › World state…* règle les drapeaux dont partent
l'essai immédiat et le jeu.

- **Faire** : poser un PNJ conditionné par un drapeau à valeurs ; changer la valeur dans *World
  state…* ; regarder le canevas ; essayer par *P*.
- **Regarder** : le canevas grise ce qui est absent sous cet état ; l'essai s'ouvre avec le même
  état ; changer la valeur fait paraître ou disparaître le PNJ sans recharger.
- **Refus** : un canevas et un essai qui ne montrent pas la même carte.

## 5. Le contenu

### RM-040 — Une carte livrée se regarde

*Lot de la filière cartes.* `--check` dit qu'elle est jouable ; ce contrôle dit qu'elle est
belle et lisible. Le rendu de la carte est joint à la PR (`LevelEditor --render`).

- **Regarder** : les raccords de murs sont fermés, les toitures ont leurs raccords en L, T et
  croix, aucun sol ne laisse voir la couleur de repli ; les portails se devinent sans la surcouche
  de débug ; l'image de l'onglet « Carte » existe et cadre le quartier.
- **Refus** : une pièce de repli visible en jeu, un raccord ouvert, une carte dont on ne trouve pas
  la sortie sans surcouche.

### RM-041 — Une quête se joue jusqu'à chaque issue

*Lot de la filière quêtes.* Chaque issue est déjà jouée en test, sans fenêtre, à graine fixée ;
ici, on la joue avec la fenêtre.

- **Faire** : jouer la quête jusqu'à sa réussite ; recommencer une partie et la jouer jusqu'à son
  échec ; à chaque embranchement, ouvrir le journal.
- **Regarder** : chaque dialogue affiche du texte dans les deux langues ; ce que le dialogue pose,
  la carte le montre (un PNJ qui paraît, une porte qui se condamne) ; l'écran de fin correspond à
  l'issue.
- **Refus** : un embranchement que le journal ne reflète pas, un texte en clé.

### RM-042 — Un PNJ nouveau, sous tous ses angles

*Lot de la filière PNJ.*

- **Faire** : ouvrir `--screen=AssetGallery` ; retrouver la figurine, son portrait et son jeton ;
  puis l'approcher sur sa carte par les quatre côtés et lui parler.
- **Regarder** : la galerie montre toutes ses poses dans leur emprise (`EX-CNT-042`) ; sa ligne de
  sol est celle des autres figurines ; il regarde le héros ; son nom paraît dans les deux langues.
- **Refus** : une pose absente de la galerie, un PNJ qui flotte ou s'enfonce d'un pixel par
  rapport à ses voisins.

## 6. Consigner

Un contrôle fait s'écrit à deux endroits, et jamais seulement dans la tête de celui qui l'a fait :

- dans la **fiche du lot**, parmi ses décisions de réalisation : l'identifiant `RM-…`, la date, la
  machine, et ce qui a été vu ;
- dans le **`CHANGELOG.md`**, quand c'est le dernier critère qui manquait : « l'auteur a fait les
  contrôles à la main » — comme au `LOT-103` et au `LOT-125`.

Un refus n'attend pas la prochaine version : il devient une correction sur la branche du lot, ou une
question ouverte écrite dans la planification si la correction dépasse le lot.
