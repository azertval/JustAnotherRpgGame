+++
id = "LOT-1007"
titre = "Éclairage et cycle jour / nuit"
version = "0.0.2.5"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Le monde a une heure : le soleil tourne, les ombres s'allongent, et la nuit les lampadaires et les braseros éclairent la rue."
prerequis = ["LOT-1003"]
livrables = [
  "L'**heure du monde** dans `Source/Core` (`core::WorldClock`, `EX-EXP-015`) : elle avance avec l'exploration, se fige en dialogue et en combat, se lit et se règle par l'option de débug `--hour=` ; une carte peut déclarer une heure fixe (`hour`) ; rien d'autre n'en dépend dans cette version.",
  "Un **shader éclairé** pour les maillages : lumière d'ambiance et soleil directionnel — la lune la nuit —, dont la couleur et la direction suivent l'heure par une table en données (`Assets/Common/Lighting/daylight.json`, `EX-REN-052`, `EX-REN-053`).",
  "Les **ombres portées** du soleil (carte de profondeur, `EX-REN-054`) : portées par les personnages, par les maillages et par la boîte de chaque pièce de décor en image ; reçues par les maillages et par le sol.",
  "Les **lumières de nuit** (`EX-REN-055`) : une pièce déclare la lumière qu'elle émet (`light`, et son éclat `glow`) dans le manifeste de son kit, une entité `light` se pose dans l'éditeur ; elles s'allument au crépuscule, seize au plus par image.",
  "La **teinte de l'heure** appliquée aux images tolérées (mobilier, kits non convertis) ; les effets, les jetons et les marques d'interface gardent leur éclat (`EX-REN-056`).",
  "Des références d'image à quatre heures — aube, midi, crépuscule, nuit — sur la carte d'essai (`LitRenderTest`), et les captures des cartes livrées.",
  "L'**éditeur** (`EX-EDIT-105`, `EX-EDIT-106`) : le canevas éclairé à l'heure choisie (case *Lighting*, curseur d'heure), l'essai qui en part, `--render --hour`, `--hour=`, l'heure fixe dans *Map properties…*, le geste `mapProperties`.",
  "Les **options du jeu** : *Ombres* (désactivées, basses, moyennes, hautes), à côté de l'anticrénelage et de la définition du rendu.",
]
criteres = [
  "Sur la carte d'Arenarea, l'auteur juge un cycle complet accéléré : pas de saut de lumière, une nuit où l'on lit encore la grille et les personnages.",
  "À midi, la lumière n'a pas changé la facture : sur la carte d'essai (`Source/Test/Fixtures/Meshes`), les images sont telles que peintes, pixel pour pixel, et le toit en maillage garde sa teinte — le rapport de son rouge à son bleu ne bouge pas d'un dixième, sa luminance moyenne d'un cinquième.",
  "Un combat lancé de nuit se joue et se lit comme de jour : cases de portée, cibles et zones restent visibles.",
  "`CanvasBenchmarks` tient la cadence avec ombres et huit lumières de nuit à l'écran : `WorldFrameShadowedEightModels1080p` sous 16,7 ms.",
  "Aucune texture de **modèle** ne porte d'ombre propre cuite : la même matière sert à toute heure. Celles du kit de la Capitale, resté en images (D-43), partent avec lui à la `0.0.3`.",
]
+++

## Pourquoi

C'est le second but de la version. Il n'est possible que parce que le décor a désormais des faces
orientées : une image peinte garde sa lumière « du haut à gauche » à toute heure.

> **Amendé le 2 octobre 2026** ([D-43](../../../../vision/decisions.md)) : le
> [LOT-1004](LOT-1004-kit-de-la-capitale-en-maillages.md) est clos sans modification, le kit de la
> Capitale reste en images jusqu'à la `0.0.3`. Ce lot éclaire donc les **personnages** et les
> maillages de la carte d'essai, et **teinte** tout le décor ; le soleil sur l'architecture viendra
> avec le kit repris ([LOT-151](../../v0.0.3-capitale-intra-muros/lots/LOT-151-kit-commun-intra-muros.md)).

> **Amendé le 3 octobre 2026** ([D-45](../../../../vision/decisions.md)) : l'auteur délègue les
> choix de réalisation de ce lot, avec une demande — étudier le besoin réel, « notamment côté
> éditeur pour placer les sources et précalculer les rendus du décor » — et un but, le meilleur
> rendu possible, options du jeu comprises. L'étude est ci-dessous ; ses choix sont datés de ce
> jour et pris par délégation. Deux critères ont été précisés : le deuxième dit ce que « conforme »
> veut dire (une lumière qui modèle une face la change, par définition), le quatrième nomme le
> banc qui mesure réellement le GPU.

## Périmètre

Dedans : l'heure, le soleil, les ombres, les lumières de nuit, la teinte des images, ce qu'il faut
à l'éditeur pour les poser et les voir, les réglages du joueur.

Dehors, nommément :

- ce que l'heure **déclenche** dans le jeu — présence des PNJ, boutiques fermées, rencontres de
  nuit : ce sont des règles, elles viendront avec les zones ;
- la météo ;
- le soleil **sur** les images tolérées : elles prennent la teinte, les lumières de nuit et
  jettent l'ombre de leur boîte, mais leurs faces gardent la lumière qu'on leur a peinte. C'est une
  raison de plus de les modeler à la `0.0.3` ;
- l'**occlusion** des lumières de nuit : la lumière d'une lanterne traverse un mur. Voir
  l'étude ;
- l'affichage de l'heure au joueur : `hmi::WorldModel::hour` la publie, aucun écran ne la montre
  encore — c'est une pièce de HUD, à dessiner.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| ~~L'ombre propre posée par sommet au LOT-1004~~ | — | sans objet : le kit n'a pas été converti (D-43) ; le LOT-151 produit ses maillages sans ombre cuite |
| ~~La règle « lumière du haut à gauche, ombre propre peinte » pour ce qui est modelé~~ | — | sans objet : elle n'est plus dans `style-3d.md`, qui dit déjà « la lumière vient du moteur » ; elle ne vit que dans le standard 2D archivé |
| ~~Les références d'image du LOT-1004 rendues sans lumière~~ | — | sans objet (D-43) |
| Le champ `facing` du banc `bench_canvas_frame.cpp` | `Source/Benchmark` | retiré de `hmi::WorldFigureSnapshot` par le LOT-1005 : le banc ne compilait plus |

Rien d'autre : ce lot ajoute une capacité, il ne remplace pas un mécanisme existant — `Core`
n'avait aucune heure du monde.

## L'étude du besoin

### Ce qu'on éclaire, en vrai

Le relevé du 3 octobre dit de quoi la scène est faite, et c'est lui qui décide de tout :

| | Combien | Forme |
|---|---:|---|
| Pièces de décor installées (Capitale, Arenarea, Arena of Fate, Empire central) | 2 218 | **images**, toutes |
| Personnages | 20 | maillages de 100 000 triangles, normales comprises |
| Maillages de décor | 0 | — hors la carte d'essai (`Fixtures/Meshes`) |
| Pièces qui évoquent une source — lampadaires, lanternes, braseros | 21 | images, dont 7 variantes éteintes |

Un moteur d'éclairage « complet » — lumière indirecte, matières à rugosité, carte de lumière
cuite — éclairerait donc vingt personnages et aucun mur. Le besoin réel est plus étroit, et plus
exigeant sur un point : **faire tenir ensemble** des personnages modelés et un décor peint qui a
déjà sa lumière.

### Quatre besoins, quatre réponses

| Besoin | Réponse retenue | Écartée, et pourquoi |
|---|---|---|
| L'image change avec l'heure, sans saut | une **table de clés** en données, interpolée ; la teinte multiplie les images | une formule de ciel physique : rien à régler à la main, et la lisibilité du combat la nuit est un critère |
| Les personnages ont du volume | ambiance + lumière dirigée par la **normale**, avec une lumière qui « enveloppe » (les faces à l'opposé ne tombent pas au noir) | des matières à rugosité et relief : `reduce_model.py` ne garde que la couleur de base, et une soixantaine de pixels de haut n'en montrerait rien |
| Tout tient au sol | une **carte d'ombres** vue du soleil : les maillages y portent leur vraie silhouette, chaque pièce en image la **boîte** de son emprise | l'ombre « tache » sous les pieds : elle ne tourne ni ne s'allonge ; le décalque incliné de chaque image : mille sept cents pièces de murs et de toits qui se recouvrent |
| La nuit a ses lumières | des sources ponctuelles, seize au plus par image, calculées **par pixel** | un tampon de lumière en passe séparée : il supporte plus de sources, mais demande une cible de plus et sa résolution sous MSAA, pour un besoin qui n'existe pas — la carte la plus éclairée a cinq lampadaires |

### Faut-il précalculer le rendu du décor ?

**Non, pas dans cette version** — et la mesure le dit mieux qu'un principe. Huit modèles de
100 000 triangles sur Arenarea, 1080p, Release, RTX 4060 Ti, médiane de cinq passes, relecture de
l'image comprise (`CanvasBenchmarks`, 3 octobre 2026) :

| Mesure | Durée |
|---|---:|
| `CanvasReadbackFloor` — effacer et relire une image vide | 5,29 ms |
| `WorldFrameEightModels1080p` — sans éclairage | 6,18 ms |
| `WorldFrameLitEightModels1080p` — crépuscule, huit lumières de nuit | 6,24 ms |
| `WorldFrameShadowedEightModels1080p` — le même, carte d'ombres de 2048 texels | 6,47 ms |

L'éclairage entier coûte **0,3 ms** sur 16,7. Précalculer, ce serait économiser ces 0,3 ms au
prix de trois choses :

- **figer l'heure**. Une image de décor cuite vaut pour une lumière ; le cycle en demande une
  infinité, ou une cuisson par heure remarquable et un fondu entre elles ;
- **un produit de plus à tenir à jour**. Une image cuite se périme dès qu'une pièce bouge ; les
  kits sont hors de Git, verrouillés par empreinte (`kits.lock.json`), et chaque carte aurait la
  sienne ;
- **un décor qui part**. Les 2 218 images sont reprises en maillages à la `0.0.3` (D-43) : cuire
  leur lumière aujourd'hui, c'est outiller ce qu'on jette.

Ce qui se précalcule l'est déjà, sans fichier : la carte est **composée une fois**
(`hmi::StaticWorldScene`), ses lumières et ses boîtes d'ombre sont **relevées une fois** dans
l'instantané (`hmi::snapshotWorldScene`), les boîtes sont un seul maillage refait quand la carte
change. À chaque image il ne reste que le soleil, seize lumières et une passe d'ombres.

**Quand la question se reposera.** Trois signes, qu'aucune carte ne montre aujourd'hui :
une place à plus de seize sources visibles ensemble ; des murs en maillages (`LOT-151`) où la
lumière d'une lanterne qui traverse devient une faute visible ; un quartier entier de maillages
dont la passe d'ombres pèse. La réponse sera alors une **carte de lumière du sol** par carte —
les sources fixes accumulées dans une texture, occultées par les murs —, calculée au chargement,
pas un fichier : à seize texels par case, Arenarea tient dans 384 × 208 texels.

### Ce que l'éditeur doit savoir faire

| Besoin de l'auteur | Réponse |
|---|---|
| Voir la carte de nuit sans lancer le jeu | case **Lighting** et curseur d'heure dans la barre d'outils : le canevas est déjà le rendu du jeu (`LOT-1002`), il suffit de lui donner l'heure |
| Un lampadaire éclaire sans qu'on y pense | le champ `light` du manifeste : toute carte qui pose la pièce a sa lumière |
| Une lumière qu'aucune pièce ne porte — une fenêtre, un feu | la famille d'entité `light`, posée comme un coffre : aucune ligne de code d'éditeur, la table des familles suffit |
| Un sous-sol toujours dans sa nuit | la propriété de carte `hour`, dans *Map properties…* |
| Rejouer tout cela sans fenêtre | `--apply` (gestes `entity` et `mapProperties`), `--render --hour`, `--check` |

Le curseur d'heure était prévu à la `0.0.5` (`LOT-169`) : il arrive ici sous sa forme la plus
simple, parce qu'on ne règle pas une lumière qu'on ne voit pas. Le `LOT-169` y ajoutera les
horaires des PNJ.

## Conception

Quatre niveaux, livrés ensemble ; chacun se coupe par son réglage.

| Niveau | Ce qu'on voit | Où |
|---|---|---|
| 1 — Teinte | l'image entière vire au bleu la nuit, à l'or au couchant | `sprite.frag`, `core::DayLight::tint` |
| 2 — Soleil | les faces des personnages et des maillages s'éclairent selon l'heure | `mesh.frag`, `core::DayLight::ambient`, `sun`, `toSun` |
| 3 — Ombres portées | personnages et décor jettent une ombre qui tourne et s'allonge | `hmi::MeshBatch::recordShadow`, `hmi::WorldShadowBox` |
| 4 — Lumières de nuit | lampadaires, lanternes, braseros éclairent autour d'eux | `core::LightSource`, `hmi::buildSceneLighting` |

**Tout se calcule dans le repère de la vue.** `hmi::IsoView` fait de la caméra isométrique une
rotation de l'espace du lieu : un sommet d'image porte déjà sa position (x, y, profondeur), celle
d'un maillage vient de sa pose. Le soleil, la verticale et les sources y sont amenés une fois par
image (`hmi::buildSceneLighting`), et les shaders n'ont rien à inverser. Le bloc uniforme
`Lighting` est le même pour les deux pipelines (`hmi::LightingBlock`).

**Un rendu sans éclairage réglé est celui d'avant le lot, au pixel.** Le bloc neutre et l'état
d'usine d'un quad (`hmi::SpriteShading`) rendent les shaders à ce qu'ils faisaient : la galerie
des assets, les vignettes, `--render` sans `--hour` et les tests de rendu d'avant ne changent pas.

### Les décisions de réalisation (3 octobre 2026, par délégation — D-45)

| Sujet | Décision | Pourquoi |
|---|---|---|
| Allure de l'horloge | **une heure du monde par minute réelle**, un jour en vingt-quatre minutes ; une partie neuve à 10 h | assez lent pour qu'une lumière ne bouge pas à l'œil, assez rapide pour qu'une séance voie passer le soir |
| Où est le soleil de midi | à **gauche de l'écran, au-dessus** (azimut 0°, élévation 48°) | c'est la lumière que porte le décor peint : sa face de gauche éclairée, sa face de droite à l'ombre. Le dessus, la face de gauche et la face de droite d'un bloc reçoivent alors 1 / 0,81 / 0,52, contre 1 / 0,74 / 0,54 pour les blocs de maquette |
| Course du soleil | du devant le matin, à gauche à midi, derrière le lieu le soir | les ombres du matin partent vers le fond, celles du soir s'allongent vers la caméra : le crépuscule se lit d'un coup d'œil |
| La nuit | une **lune** : la lumière dirigée ne s'éteint pas, elle change d'astre ; noire aux deux bascules (05:30 et 20:30), qui ne se voient donc pas | une nuit sans lumière dirigée aplatit les personnages |
| Raccord maillage / image | `ambient + sun × (sin(élévation) + 0,3) / 1,3 = tint`, tenu par la table | un dallage modelé et un dallage peint côte à côte ont la même lumière à toute heure |
| Espace de couleur | la lumière se calcule sur les valeurs sRGB, sans linéarisation | une image à midi doit être la même au pixel ; linéariser déplacerait tous les tons du décor validé |
| Ombres du décor en images | la **boîte** de l'emprise, haute de ce que l'image porte au-dessus de son ancre, large comme elle ; **resserrée** vers le haut pour le mobilier, le végétal et les pièces maîtresses (familles 06, 07, 08) | une fontaine en boîte pleine jetait au crépuscule un rectangle de treize mètres |
| Qui reçoit l'ombre | les maillages et le **sol** ; pas une image dressée | le plan d'une image dressée coupe sa propre boîte en biais : l'ombre y dessinerait une diagonale sur la façade |
| Les lampes et la lumière du jour | elles **comblent** ce qui manque à la lumière pour arriver au plus clair (1,08), elles ne s'y ajoutent pas | un sol ivoire sous deux lanternes brûlait au blanc |
| Les effets | ils **gardent leur éclat** ; la fiche les voulait teintés | un soin ou un éclair bleuis par la nuit se lisent comme une erreur |
| Nombre de lumières | **seize** par image, les plus proches du centre ; le critère en demandait huit | la place la plus éclairée livrée a cinq lampadaires (Martpart) ; huit lumières coûtent 0,06 ms |
| Heure de l'horloge du `LOT-171` | le `LOT-171` (`0.0.5`) n'a plus à livrer « l'horloge du monde » : elle est ici ; il lui reste le voyage et le calendrier | les deux fiches livraient la même chose |

### Ce que les kits déclarent

Quatorze pièces du kit de la Capitale portent une lumière : lampadaires (7,5 m de portée, 9 m
pour le double), lanternes murales, braseros — ceux-ci tremblent et gardent une part de leur éclat
(`glow`). Les variantes éteintes (`-off`) n'en portent pas. Le manifeste est suivi par Git ; les
images ne changent pas, aucun kit n'est republié.

Le sous-sol de l'arène (`undercroft`) déclare son heure fixe (22:00) et trois torches, posées par
l'éditeur sans fenêtre.

## Mesures

Voir « Faut-il précalculer » pour les durées. La mémoire graphique : une carte d'ombres de 2048
texels pèse 16 Mio, celle de 4096 texels 64 Mio ; le bloc uniforme, 656 octets.

**Ce qui n'est pas mesuré.** Le coût du MSAA et du suréchantillonnage combinés à l'éclairage ; la
carte d'ombres sous le rendu logiciel de la CI (WARP), où les tests n'en dessinent qu'une de
2048 texels sur la carte d'essai.

## Risques et questions ouvertes

- **Le jugement de l'auteur reste à rendre** (critère 1) : le cycle a été vu en captures aux
  quatre heures sur les quatre cartes livrées, pas en mouvement par lui. La nuit se règle dans
  `daylight.json`, sans recompiler.
- **La lumière d'une lanterne traverse les murs.** Invisible sur les places ouvertes livrées ;
  à traiter avec le kit en maillages (`LOT-151`), par la carte de lumière du sol décrite plus
  haut.
- **Une pièce allumée et sa variante éteinte sont deux images.** Rien n'échange `prop-brazier-lit`
  contre `prop-brazier-off` au matin : un brasero « allumé » flambe à midi, sans éclairer. C'est
  une règle de présence selon l'heure, hors périmètre.
- **Les ombres des étals** (famille 09, bâtiments) sont des boîtes droites, celles des fontaines
  des pointes : ni l'une ni l'autre n'est la silhouette de la pièce. Elles partent avec les
  images.
- **Aucun écran ne montre l'heure au joueur.**
