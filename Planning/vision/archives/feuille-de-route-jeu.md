# Feuille de route — le bac à sable de Tanares

> **Page figée le 20 septembre 2026.** Elle n'est plus la source des lots à venir : la planification
> vit désormais dans le dossier `Planning/` du dépôt, publié sur le
> [site de planification](https://azertval.github.io/JustAnotherRpgGame/planning/). Le jeu quitte
> le pixel art pour la 2D HD, le référentiel `0.1.0` devient l'Empire central seul, et la `0.0.1` une
> démo basique. Les lots que cette page annonçait sont repris sous de nouveaux numéros, à partir de
> `LOT-100` : la table de correspondance est dans `Planning/vision/correspondance-ancienne-roadmap.md`.
> Cette page reste pour l'histoire des lots `LOT-01` à `LOT-96`, et parce que la documentation
> renvoie à ses ancres ; son tableau d'avancement ne se met plus à jour.

Le programme complet du jeu, et **l'unique source de vérité** des lots à venir : les lots `LOT-09`
à `LOT-96`, ce que le corpus `Documentation/SourceBook/` permet d'en tirer, et les audits qui ont
confronté le tout à l'état réel du dépôt et aux deux livres de Tanares.

> **Cette page s'appelait « Feuille de route 0.1.0 ».** Ce nom disait le périmètre d'un *vertical
> slice* — la version que le dépôt porte déjà dans son `CMakeLists.txt` — alors que la cible, fixée
> par les livres, est un **bac à sable dans l'univers complet de Tanares** : treize régions, treize
> espèces, seize classes, le Colisée, la compagnie, le plan pénombral — et **c'est ce contenu
> complet qui est la `0.1.0`**. Renommée au second audit, le 14 septembre 2026, avec les jalons
> intermédiaires `0.0.x` ci-dessous. L'ancre Doxygen a suivi (`@ref roadmap`).

> **L'éditeur de cartes a sa propre feuille de route** depuis le 18 septembre 2026 :
> [`Documentation/Editeur/feuille-de-route.md`](feuille-de-route-editeur.md), lots `LOT-EDITOR-01` à
> `LOT-EDITOR-14`. Outil interne, il avance à part ; aucun lot de cette page n'en dépend, et le lint
> du graphe ne le lit pas.

Les lots **livrés** gardent leur dossier et leur `epic.md` : ils portent ce que leur réalisation a
tranché, qui est de l'histoire, pas du programme. Tout le reste vit ici, et un dossier se crée
**au démarrage** d'un lot.

**Par où commencer.** L'[état d'avancement](#roadmap-avancement) ci-dessous donne, en un
tableau, ce qui reste à faire et dans quel ordre — c'est la réponse à « et maintenant ? ». Le reste
de la page dit pourquoi.

`Documentation/SourceBook/` rassemble le **matériel de référence** ayant servi à construire le monde
et les règles : huit PDF et neuf ressources de table virtuelle, environ 1 400 pages, 785 Mo.
Les sections 1 à 4 disent ce qu'ils
contiennent et ce que l'extraction a appris ; la section 5 découpe le travail et la section 6
donne la **règle d'ordre** qui produit le tableau d'avancement, le chemin critique et le graphe ;
la section 7 dessine l'arborescence cible et la section 8 récapitule ce qui est tranché ; les
sections 9 et 10 rapportent l'audit et ce qu'il faut anticiper ; la section 11 porte les lots
`LOT-09` à `LOT-29`.

---

## État d'avancement {#roadmap-avancement}

**Quarante-cinq lots livrés, quarante-trois restants.** Le [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md) est livré : le
dépôt ne garde plus rien du jeu d'origine — ni son runtime, que seul l'essai de l'éditeur faisait
encore tourner, ni ses mécanismes, ni son habillage, ni ses assets, ni les spécifications qui le
décrivaient ; l'essai de l'éditeur joue désormais l'exploration du jeu. Il a absorbé le `LOT-69`
(le retrait de l'atelier pixel art). Le prochain calculé est le [LOT-16](#lot-16), les quêtes
de la Capitale.

Livrés : `LOT-01` à `LOT-08` (le socle : fork et purge, `HmiLib`, `LevelData`, format de carte
version 3, modes de jeu, déplacement top-down, tri par profondeur, tuiles RPG), `LOT-77`, `LOT-78`
et `LOT-79` (les trois préconditions), puis le [LOT-30](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-30-chaine-extraction-corpus.md) (la chaîne d'extraction du
corpus et le lexique bilingue), le [LOT-32](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-32-schemas-donnees-rpg.md) (les schémas de données RPG) et le
[LOT-43](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-43-options-de-personnage.md) (compétences, langues, dons, multiclassage) — les trois premiers lots de la
filière contenu —, le [LOT-12](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-12-des-caracteristiques-jets.md) (dés, caractéristiques, jet de d20), premier lot du
noyau RPG, le [LOT-33](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-33-bestiaire-de-base.md) (les 94 bêtes du SRD), premier catalogue rempli, et le
[LOT-36](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-36-especes-historiques-classes.md) (22 espèces, 13 historiques, 4 classes provisoires), enfin le
[LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md) (la fiche de personnage, qui les assemble) et le [LOT-10](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-10-entites-de-carte.md)
(entités de carte et interaction), le [LOT-34](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-34-equipement.md) (armes, armures et équipement), le
[LOT-66](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-66-charte-visuelle.md) (charte visuelle), le [LOT-37](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-37-atlas-des-regions.md) (l'atlas des treize régions et
de leurs 94 lieux) et le [LOT-76](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-76-habillage-interface.md) (les vingt et une planches d'habillage extraites des
feuilles de personnage) et le [LOT-67](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-67-menus-vocabulaire-rpg.md) (les menus et le vocabulaire d'un RPG, qui
retire du programme la notion de niveau discret) et le [LOT-68](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-68-chassis-ecrans-rpg.md) (le châssis des huit
écrans du RPG, décrits par une table plutôt qu'écrits un par un) et le
[LOT-38](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-38-fiche-de-personnage.md) (la fiche de personnage, calquée sur la planche du corpus) et le
[LOT-14](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-14-inventaire-et-equipement.md) (inventaire, équipement et statistiques dérivées) et le
[LOT-18](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-18-bascule-exploration-combat.md) (la bascule exploration ↔ combat) et le [LOT-39](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-39-cles-assets.md) (la plomberie
des clés d'assets), puis les deux lots de refonte de l'interface, inscrits **hors** de cette page
parce que décidés après elle : le [LOT-86](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-86-refonte-hmi-quick.md) (l'IHM du jeu en Qt Quick, dans son propre
exécutable, modifiable par un artiste sans compilateur) et le [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) (la **charte
v2**, tirée des dix maquettes du pack UI, et les treize écrans transcrits dessus), enfin le
[LOT-19](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-19-grille-tactique.md) (la grille tactique : occupation par emprise, cases atteignables au budget,
chemin déterministe), premier lot du combat proprement dit, le [LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md)
(l'initiative, le tour par tour et ses crochets, les trois fins d'un combat), le
[LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md) (le Colisée : la première carte, l'arène qui tient le combat et le rejoue à
graine fixée, personne n'y meurt), et le [LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md) (les PNJ et les dialogues : un
graphe refusé au chargement s'il est mal formé, joué sans fenêtre, refusé faute de langue commune),
et le [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md) (les attaques et les dégâts du Manuel : un jet qui s'amende avant d'être
figé, un pipeline de dégâts à étapes nommées, l'attaque d'opportunité, un journal qui dit tout),
et le [LOT-22](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-22-portee-ligne-de-vue.md) (la portée, la ligne de vue symétrique par construction, l'abri qui ne
s'additionne pas, les cinq zones d'effet du Manuel, les portées des armes enfin structurées),
et le [LOT-23](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-23-ia-tactique.md) (l'IA tactique, tirée du *Guide du Maître* : elle ne lit que ce que la
table voit, choisit par l'espérance de dégâts, ne se suicide pas, ne se bloque pas, et se rejoue à
l'identique ; la prise en tenaille au Colisée), et le [LOT-24](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-24-ihm-combat.md) (l'IHM de combat au
Colisée : un curseur de ciblage, une prévisualisation qui est le jet, la manette dans le jeu, un
combat entier sans souris — la vérification manuelle reste à faire), et le [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md)
(l'éditeur retargé sur le RPG : trois couches, entités et portails posés sans JSON, graphe du monde,
avertissement de terrain tactique, essai immédiat avec entités — la vérification manuelle reste à
faire), et le [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) (l'atelier des textures : une maquette de style approuvée, un
style écrit, une planche commandée par lieu depuis sa fiche d'atlas et découpée par script — le
Colisée, rendu dans l'arène, et Martpart ; la scène en pixel art et l'interface à la charte v2
écrites dans la spécification), et le [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) (les images du corpus hors du dépôt,
le fond de menu produit, et l'écran « Carte » à trois niveaux — monde, région, ville — sur les
seize cartes peintes par l'auteur ; il absorbe le `LOT-95`), et le [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) (le
Colisée, première carte qu'on parcourt), le [LOT-96](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-96-quartiers-capitale.md) (Martpart et Arenarea, le graphe
des quartiers) et le [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md) (le retrait de l'héritage : code, assets et
spécifications ; il absorbe le `LOT-69`).
Chacun garde son dossier et son `epic.md`. Le `LOT-85` (outillage Qt Designer) a été **abandonné**
au profit du `LOT-86` et son numéro n'est pas réattribué.

**Les jalons de version.** La `0.1.0` est le **contenu final** : le bac à sable complet. Tout ce
qui la précède est un état intermédiaire **jouable**, numéroté `0.0.x`, et chaque lot restant en
sert exactement un. Le `CMakeLists.txt` porte encore `0.1.0` : c'est un contresens dans ce programme, et le `LOT-28`
le ramène à `0.0.1` en taguant le slice.

| Version | Ce qu'elle rend jouable | Lots |
|---|---|---|
| `0.0.1` | Le *vertical slice* **dans la Capitale** : le Colisée en version finale comme première carte qu'on parcourt, deux quartiers de la capitale impériale et leur bas-fond, une quête tirée du livre, un combat tactique complet sur la carte, le verdict de l'Arène du Destin, une sauvegarde ; un style de scène propre au jeu et plus aucune image du corpus ; des spécifications qui décrivent ce jeu | `LOT-93`, `LOT-09`, `LOT-96`, `LOT-16`, `LOT-17`, `LOT-27`, `LOT-28`, `LOT-88` |
| `0.0.2` | Le bac à sable des **treize régions** : terrain généré, peuplement déduit des statistiques régionales, voyage, calendrier lunaire, économie, Guilde, groupe de quatre | `LOT-80`, `LOT-81`, `LOT-40`, `LOT-44`, `LOT-46`, `LOT-89`, `LOT-26`, `LOT-41`, `LOT-70`, `LOT-82`, `LOT-35`, `LOT-42`, `LOT-45`, `LOT-74`, `LOT-75`, `LOT-72`, `LOT-25`, `LOT-29`, `LOT-49` |
| `0.0.3` | Les **seize classes** du *Player's Guide* et leurs sous-classes, le Colisée comme institution, la compagnie et son quartier général | `LOT-84`, `LOT-47`, `LOT-51` → `LOT-65`, `LOT-83` |
| `0.0.4` | Le **plan pénombral**, second monde du Sourcebook | `LOT-90` |
| **`0.1.0`** | **Le bac à sable de Tanares, complet** : la somme des quatre, relue et équilibrée d'un bloc — la version de contenu final | aucun lot propre : c'est le tag posé quand le dernier lot de la `0.0.4` est livré |

Les jalons **priment** dans le calcul de l'ordre (§6) : un lot d'un jalon ultérieur ne démarre pas
tant que le jalon courant a un lot prêt, et `scripts/lint_lots.py` refuse un lot restant qui ne
figure dans aucun jalon (règle 14).

**Quelle date pour la `0.0.1` ?** Aucune n'est annoncée ici, et ce n'est pas une prudence de
principe : les quarante-deux lots livrés l'ont été entre le 3 et le 17 septembre 2026, soit une
cadence observée qui, prise au pied de la lettre, placerait la version dans trois semaines. Cette
extrapolation est fausse, et il vaut mieux l'écrire que la laisser deviner : les lots livrés sont
des lots de **socle**, de **catalogues** déjà extraits et d'**écrans** dessinés sur des données en
attente. Ceux qui restent sont des lots de **moteur** — les quêtes, la sauvegarde, le graphe de
cartes — dont seuls la grille tactique du `LOT-19`, le tour du `LOT-20`, l'arène du `LOT-50`, les
dialogues du `LOT-15`, les attaques du `LOT-21`, la ligne de vue du `LOT-22`, livrés le 14
septembre, puis l'IA du `LOT-23` et l'IHM de combat du `LOT-24`, livrées le 15, et l'éditeur du
`LOT-11` et l'atelier du [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md), livrés le 16, donnent une première mesure de cadence, puis les
catalogues restants (82 créatures de Tanares, 31 tables de progression) et le peuplement de dix
régions. Ce tableau donne le **reste à faire**, pas une date.

L'ordre ci-dessous n'est pas arbitré : il est **calculé** depuis le graphe de dépendances, par la
règle de la **section 6** — *à chaque pas, parmi les lots dont tous les prérequis sont faits,
celui du jalon de version le plus proche ; à jalon égal, celui qui en débloque le plus* — et
vérifié en intégration continue par `scripts/lint_lots.py`.
Le détail de chaque lot est en section 5 (filière contenu) et en section 11 (lots absorbés) ; ce
tableau n'en porte volontairement aucun.

| # | Lot | Objet | Débloque | Statut |
|---|---|---|---|---|
| 1 | `LOT-16` | Les affaires de la Capitale : quêtes et drapeaux de monde | 12 | **prochain** |
| 2 | `LOT-17` | Reprendre sa partie dans la Capitale : sauvegarde riche | 10 | en attente |
| 3 | `LOT-27` | La Capitale : contenu du *vertical slice* | 9 | en attente |
| 4 | `LOT-28` | Audio, effets et version `0.0.1` | 0 | en attente |
| 5 | `LOT-81` | Descripteurs de terrain et règles de zone des treize régions | 8 | prêt |
| 6 | `LOT-40` | Générateur de terrain | 7 | en attente |
| 7 | `LOT-44` | Noms, tables aléatoires et contenu d'ambiance | 7 | prêt |
| 8 | `LOT-46` | Créatures de Tanares | 7 | prêt |
| 9 | `LOT-89` | Dons, objets magiques et consommables de Tanares | 7 | prêt |
| 10 | `LOT-26` | Butin, marchands, économie | 6 | en attente |
| 11 | `LOT-41` | Peuplement : rencontres et créatures | 6 | en attente |
| 12 | `LOT-80` | Factions, panthéon et organisations | 6 | prêt |
| 13 | `LOT-70` | Horloge, calendrier et lune | 5 | prêt |
| 14 | `LOT-82` | Peuplement civil : PNJ, marchands et quêtes | 5 | en attente |
| 15 | `LOT-35` | Sorts et états | 2 | prêt |
| 16 | `LOT-42` | Voyage et carte du monde | 2 | en attente |
| 17 | `LOT-29` | Groupe de quatre personnages | 1 | en attente |
| 18 | `LOT-45` | Guilde des Aventuriers : rangs et contrats | 1 | en attente |
| 19 | `LOT-74` | Expérience et progression | 1 | en attente |
| 20 | `LOT-25` | Sorts et capacités de classe | 0 | en attente |
| 21 | `LOT-49` | Contrôle de cohérence du contenu | 0 | prêt |
| 22 | `LOT-72` | Conditions, agonie et mort | 0 | en attente |
| 23 | `LOT-75` | Campement et repos dans le monde | 0 | en attente |
| 24 | `LOT-84` | Les 31 tables de progression de classe | 2 | prêt |
| 25 | `LOT-47` | Socle de classe, et le guerrier comme preuve | 1 | en attente |
| 26 | `LOT-83` | Compagnie : rangs de Guilde, niveau d'équipe et quartier général | 0 | en attente |
| 27 | `LOT-90` | Le plan pénombral | 0 | en attente |
| 28 | `LOT-51` → `LOT-65` | une classe par lot | 0 | en attente |

**Débloque** — combien de lots restants dépendent de celui-ci, directement ou en cascade. C'est le
critère de priorité, et il se relit sur la ligne.

- **prochain** — le lot à démarrer.
- **prêt** — tous ses prérequis sont livrés ; il pourrait démarrer aujourd'hui, la règle lui
  préfère seulement un lot qui débloque davantage.
- **en attente** — il attend au moins un lot non livré.

---

## 1. Le corpus n'est pas versionné

`Documentation/SourceBook/` est exclu **en entier** par le `.gitignore`. La raison est désormais la
taille : **785 Mo** de binaires, dont deux images de plus de 100 Mo, que git versionne mal —
chaque clone les traîne, et aucune de leurs révisions ne se compresse. (280 Mo jusqu'au 7 septembre
2026 ; les neuf ressources de table virtuelle du §2 ont presque triplé le volume, ce qui ne change
pas la règle mais en renforce le motif.)

> **Cette phrase a été fausse jusqu'au [LOT-30](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-30-chaine-extraction-corpus.md).** La règle d'exclusion n'était pas
> dans le `.gitignore` du dépôt : elle vivait comme modification locale non commitée, sur un seul
> poste. Sur un clone neuf, un `git add -A` embarquait les 280 Mo. `scripts/check_glossary.py`
> vérifie désormais l'exclusion en intégration continue — une affirmation que rien ne vérifie finit
> par devenir fausse, et celle-ci l'était depuis le début.

Deux conséquences pratiques, à retenir avant d'écrire quoi que ce soit dans ce dossier :

- **Rien de ce qu'on y dépose ne sera versionné**, pas même un fichier texte. Le manifeste du
  `LOT-30` vit donc dans `scripts/sourcebook/corpus.toml`, avec l'outil qui le lit, et non à côté
  des PDF qu'il décrit.
- Le corpus texte intermédiaire produit par l'extraction n'est pas versionné non plus. Il se
  régénère à la demande depuis les PDF locaux ; ce sont les **données finales**, elles, qui entrent
  dans le dépôt.

---

## 2. Inventaire du corpus

| Document | Nature | Pages PDF | Langue | Texte | Images |
|---|---|---|---|---|---|
| `Tanares_Sourcebook.pdf` | Univers de Tanares : monde, factions, panthéon, régions, organisations, histoire, bestiaire, objets magiques | 179 | EN | natif, **double page** | **2 029** (1 806 ≥ 512²) |
| `Players_Guide_to_Tanares_Version_20231218.pdf` | Espèces, classes, sous-classes, historiques, dons, sorts, règles optionnelles | 165 | EN | natif, **double page** | **1 710** (1 476 ≥ 512²) |
| `Manuel-Des-Monstres.pdf` | *Monster Manual* D&D 5 en français | 354 | FR | **scan OCR bruité** | 1 424 (633 ≥ 512²) |
| `Manuel-Des-Joueurs.pdf` | *Player's Handbook* D&D 5 en français — **5 races et 8 classes** absentes des *Basic Rules* | 320 | FR | scan OCR, **tables récupérables en `-table`** | 631 (453 ≥ 512²) |
| `Basic-Rules-FR.pdf` | Règles de base D&D 5 en français (aidedd.org) | 137 | FR | natif **propre** | 175, surtout des fonds de page |
| `Animaux.pdf` | Bêtes du SRD traduites (aidedd.org) | 32 | FR | natif **très propre** | 38, uniquement des fonds |
| `Glossaire.pdf` | Lexique de traduction anglais → français (aidedd.org) | 22 | EN/FR | natif, 2 colonnes | 5 pictogrammes |
| `Character_Sheets_Tanares.pdf` | Feuilles de personnage | 5 | EN | **aucun** (vectoriel) | 6, jusqu'à 2 668 × 3 418 |

### Les ressources de table virtuelle (ajoutées le 7 septembre 2026)

Neuf ressources se sont ajoutées au corpus **après** les huit PDF ci-dessus. Elles ne viennent pas
d'un autre livre : ce sont les fichiers distribués avec Tanares pour **jouer sur table virtuelle**.
Leur intérêt tient exactement à ce que les PDF de règles n'ont pas — des cartes vues **de dessus**,
des jetons **déjà détourés**, une carte du monde sans double page ni filigrane, et une feuille de
personnage **vierge**, c'est-à-dire une maquette et non un exemple rempli.

| Ressource | Nature | Volume | Texte | Ce qu'elle alimente |
|---|---|---|---|---|
| `VTT/Character Compendium - Low.pdf` | PNJ et créatures de Tanares : bloc de statistiques complet et portrait pleine page | 169 pages | **natif, 608 000 car.** | `LOT-46`, `LOT-82` |
| `VTT/Character Compendium - High.pdf` | le même, à haute définition (83 Mo) | 169 pages | identique | extraction d'illustrations |
| `VTT/Adventure Map Grids.pdf` | **31 cartes tactiques peintes, vues de dessus, alignées sur une grille** | 31 planches | aucun | `LOT-19`, `LOT-27`, `LOT-40` |
| `VTT/Adventure Hunt for Azymor.pdf` | module d'aventure complet : intrigue, PNJ nommés, lieux, rencontres | 15 pages, **double** | natif, 97 000 car. | `LOT-27`, `LOT-16` |
| `VTT/Map - World.jpg` | la carte du monde, **9 933 × 7 016**, sans filigrane — **référence seule** : le jeu ne l'affiche plus ([LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md)) ; l'écran « Carte » montre les cartes **peintes par l'auteur**, et cette planche n'a servi qu'à relever, à l'œil, la position des lieux | 1 planche | aucun | [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) |
| `VTT/Map - Capital.jpg` | plan de la **Capitale impériale**, rue par rue, 9 933 × 7 016 — les douze quartiers du Sourcebook y sont nommés, avec les trois arènes, les portes et les bâtiments que la prose décrit | 1 planche | aucun | **référence seule**, lue sur le poste : le plan du jeu est celui que l'auteur a peint ([LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md)), et les cartes du slice se tracent depuis celui-là (`LOT-96`, [LOT-27](#lot-27)) |
| `VTT/Blank Sheets RPG (1).pdf` | les 5 feuilles de personnage **vierges** | 5 planches | aucun (vectoriel) | [LOT-38](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-38-fiche-de-personnage.md) |
| `VTT/RPG Sheets - BW print (2).pdf` | les mêmes en noir et blanc | 5 planches | aucun | [LOT-38](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-38-fiche-de-personnage.md) |
| `VTT/Tokens VTT PNG/` | **172 jetons ronds détourés**, 354 × 354, canal alpha | 172 fichiers | — | [LOT-38](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-38-fiche-de-personnage.md), `LOT-15`, `LOT-24`, `LOT-82` |

Trois d'entre elles changent un arbitrage déjà pris, et il vaut mieux l'écrire ici que le laisser
découvrir au lot concerné :

- **Le compendium de personnages est un second gisement de blocs de statistiques**, en texte
  **natif** et non en scan. Le §8 écartait les 416 blocs du *Manuel des Monstres* parce que son OCR
  bruité rendait chaque valeur suspecte ; cet argument ne vaut pas ici. Ce que le `LOT-46` en
  retiendra reste à arbitrer **dans ce lot** — le total de 176 créatures était un choix de
  périmètre, pas une limite de matière.
- **Les cartes tactiques sont la seule matière du corpus qui montre le jeu sous l'angle où il se
  joue.** Tout le reste est en vue de côté, en portrait ou en carte régionale ; ces 31 planches sont
  vues de dessus et alignées sur une grille, comme les cartes du `LOT-04`.
- **Les jetons épargnent un détourage.** 172 portraits ronds à fond transparent, prêts à poser dans
  une fiche, une réplique de dialogue ou une piste d'initiative — un travail d'image que le projet
  n'aurait pas fait à la main.

Comme les huit premiers, ces fichiers **ne sont pas versionnés** (§1) : ils sont déclarés dans
`scripts/sourcebook/corpus.toml`, avec leur empreinte, et c'est ce manifeste qui est versionné. Il
sait depuis leur arrivée décrire autre chose qu'un PDF — une **planche** seule, ou une
**collection** de fichiers, dont l'empreinte porte sur la liste triée de ses membres : un jeton
retiré, ajouté ou retouché la change, et 172 lignes de manifeste n'ont pas eu à être écrites pour un
jeu qu'on prend ou qu'on laisse entier.

### Ce que chaque document apporte, chiffré

- **`Animaux.pdf`** — **94 blocs de statistiques** complets (CA, PV, vitesse, six caractéristiques,
  compétences, sens, langues, facteur de puissance, traits, actions), au format le plus régulier de
  tout le corpus. C'est le gisement le plus rentable : traduction française déjà faite, mise en page
  mécanique. Aucune illustration de créature à en tirer en revanche — ses 38 images sont les fonds
  de parchemin des pages.
- **`Basic-Rules-FR.pdf`** — le socle jouable complet : 4 races, 4 classes, 6 historiques, et
  surtout **les tables d'équipement** (armes p. 48, armures p. 50, matériel d'aventurier p. 51,
  outils p. 55, montures et véhicules p. 56, marchandises p. 56, dépenses p. 57, babioles p. 59),
  les sorts, et l'appendice des états. C'est aussi la **référence terminologique française** du
  projet.
- **`Glossaire.pdf`** — **2 084 paires `anglais = français ; catégorie`**, catégorie
  comprise (`; classe`, `; sort`, `; état`, `; type de dégâts`, `; capacité (paladin)`…). C'est une
  table d'autorité de traduction directement exploitable, et la réponse à « comment dit-on *saving
  throw* partout pareil dans le jeu ».
- **`Tanares_Sourcebook.pdf`** — **13 régions** à encart `Regional Statistics` (plus Darkall,
  l'Undertanares et le Wasteland, décrits sans encart), **5 factions**, un **plan pénombral en
  sept couches et 32 lieux**, **18 divinités**, **13 organisations** à fiche (une trentaine
  nommées), une chronologie de 90 événements datés, un calendrier complet, une économie de
  minerais rares, puis **17 familles de créatures pour 82 blocs de statistiques** (ch. 9) et
  **29 objets magiques** (ch. 10). *(Comptes relus sur le livre le 14 septembre 2026, §4bis.)* Chaque région y a ses lieux, ses factions présentes, ses PNJ notables : c'est
  un monde de jeu prêt à l'emploi, pas seulement un exemple de structure. Et **2 029 images**, dont
  les cartes de régions.
- **`Players_Guide_to_Tanares.pdf`** — 13 espèces, 4 classes inédites, 4 classes simplifiées, une
  vingtaine de sous-classes (**31 tables de progression** détectées), 7 historiques, plus **1 710
  images** — portraits d'espèces, illustrations de classes, objets.
- **`Manuel-Des-Monstres.pdf`** — **416 blocs de statistiques** en français, le plus gros bestiaire
  du corpus. **Hors périmètre** (§8) : son OCR bruité en fait aussi le plus coûteux, et les 176
  créatures des deux autres gisements suffisent à peupler treize régions. Il reste disponible si le
  besoin s'en fait sentir plus tard.
- **`Manuel-Des-Joueurs.pdf`** — **la source française des races et classes manquantes**, et à ce
  titre indispensable. Les *Basic Rules* ne portent que 4 races et 4 classes ; ce manuel apporte les
  **5 races** restantes (drakéide, gnome, demi-elfe, demi-orc, tieffelin) et les **8 classes**
  restantes (barbare, barde, druide, moine, paladin, rôdeur, ensorceleur, sorcier) — toutes
  vérifiées présentes. Sans lui, les sous-classes Tanares pour barbare, barde, druide, moine,
  paladin, rôdeur et ensorceleur n'auraient **aucune classe de base sur laquelle se greffer**. Son
  OCR est bruité mais gérable : voir §4.
- **`Character_Sheets_Tanares.pdf`** — 5 feuilles, **zéro caractère de texte** hors filigrane. Les
  planches sont en revanche disponibles en image à 2 668 × 3 418, soit environ 300 ppp : de quoi
  servir de maquette précise pour l'écran de fiche — ce que le [LOT-38](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-38-fiche-de-personnage.md) a fait, avant
  que la charte v2 du [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) ne lui substitue la maquette 03 du pack UI.

---

## 3. Licences : contrainte en sommeil

Le projet est **privé, sans diffusion ni acte commercial**, et le dépôt est passé en privé. Les
licences du corpus ne contraignent donc **pas** l'usage : noms propres, textes, blocs de
statistiques et illustrations sont utilisables tels quels dans le jeu.

Ce paragraphe existe quand même, parce que la contrainte n'est pas supprimée mais **endormie** : elle
se réveille intégralement le jour où le projet serait publié, diffusé ou vendu. Autant savoir dès
maintenant ce qu'il faudrait alors reprendre, plutôt que de le découvrir à ce moment-là.

| Source | Statut | Ce qu'il faudrait faire en cas de publication |
|---|---|---|
| `Basic-Rules-FR`, `Animaux`, `Glossaire` | **OGL 1.0a**, diffusion libre (aidedd.org) | Rien, hors ajout de la notice OGL et de l'attribution SRD 5.1 à `THIRD-PARTY-NOTICES.md` |
| `Tanares_Sourcebook`, `Players_Guide` | Mécanique SRD ouverte ; lore, noms et art **réservés** (*Product Identity* : « Tanares », « Penumbral Plane », « madwalker », « taii'maku », « Golgöggoth », « Isendden », « emogum ») | Renommer le monde et ses entités, réécrire les textes, remplacer les illustrations |
| `Manuel-Des-Joueurs` (dans le périmètre), `Manuel-Des-Monstres` (hors périmètre) | Copyright plein (Wizards of the Coast / Black Book Éditions) | Retirer les données marquées `phb-fr` — soit 5 races et 8 classes, à remplacer par des créations propres |

En pratique, cela suggère une seule discipline, peu coûteuse et qui garde la porte ouverte :
**tracer la provenance**. Chaque donnée produite porte un champ `"source"` (`srd`, `tanares`,
`phb-fr`, `original`). Un jour de publication, la question « qu'est-ce qui doit sauter ? » se répond
par une requête plutôt que par une relecture de tout le catalogue — et le `LOT-32` peut l'imposer par
schéma sans effort supplémentaire.

Cela ne change rien au cadrage acté avant le [LOT-01](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-01-fork-purge.md) — **règles d20 maison,
compatibles SRD dans leur structure, sans en dépendre** : le moteur reste indépendant du SRD, ce
sont les *données* qui viennent du corpus.

---

## 4. Ce que l'extraction a appris

Six constats techniques, tous vérifiés sur le corpus, qui déterminent la faisabilité des lots.

**PyMuPDF fait tout, et mieux que l'outillage en ligne de commande.** Le poste dispose de
`pdftotext` version Xpdf 4.00, mais ni `pdfimages` ni `pdftoppm`. **PyMuPDF 1.28.2** (installable
par `pip`, testé) couvre en une seule bibliothèque le texte, les coordonnées de mots, les images et
le rendu de page — c'est la dépendance à retenir pour le `LOT-30`, plutôt qu'un assemblage
d'exécutables partiellement présents.

**Les livres Tanares sont paginés en double page.** 179 pages PDF pour un livre qui numérote jusqu'à
358 ; une page PDF porte deux pages du livre côte à côte, ce qu'un rendu confirme visuellement.
Toute correspondance « page du sommaire → page PDF » doit intégrer ce facteur 2, sinon on cible
systématiquement le mauvais chapitre. C'est la première chose que le manifeste du `LOT-30` doit
enregistrer.

**`pdftotext -layout` mélange les colonnes des tableaux.** Sur la table des armes des *Basic Rules*,
il produit des lignes où le poids et le prix appartiennent à l'arme d'en dessous — une donnée
fausse, et fausse **silencieusement**, ce qui est pire qu'une extraction qui échoue. Le mode
`-table` d'Xpdf les restitue correctement (`Bâton | 1d6 contondant | 2 kg | 2 pa | Polyvalente
(1d8)`), et PyMuPDF permet mieux encore en regroupant les mots par coordonnée x. Dans tous les cas :
**jamais `-layout` sur un tableau**.

**Les blocs de statistiques d'`Animaux.pdf` suivent un gabarit strict.** Toujours le même ordre,
toujours les mêmes libellés :

```
AIGLE
Bête de taille P, sans alignement
Classe d'armure 12
Points de vie 3 (1d6)
Vitesse 3 m, vol 18 m
   FOR DEX CON INT SAG CHA
  6 (-2) 15 (+2) 10 (+0) 2 (-4) 14 (+2) 7 (-2)
Compétences Perception +4
Sens Perception passive 14
Langues -
Puissance 0 (10 PX)
Vue aiguisée. …
ACTIONS
Serres. Attaque au corps à corps avec une arme : +4 au toucher, allonge 1,50 m, une cible.
Touché : 4 (1d4 + 2) dégâts tranchants.
```

Un automate suffit. Les 94 entrées sont accessibles sans jugement humain, hors relecture de
contrôle. Les 82 blocs du Sourcebook suivent un gabarit analogue en anglais.

**Les deux manuels français sont des OCR bruités — mais moins gravement qu'il n'y paraît.** Leurs
pages de crédits donnent « Cordeil » pour « Cordell », « )on Schindehette », « Chefde projet »,
« W'àyne Reynolds ». Il serait tentant d'en conclure que ces fichiers sont inexploitables ; ce
serait généraliser depuis la page la plus bruitée du livre, et ce serait faux. Le bruit se répartit
en **trois niveaux, de gravité très inégale** :

- **La structure des lignes** — le plus dangereux, et le plus facile à corriger. En `-layout`, la
  table du barbare désynchronise ses colonnes : le niveau 5 y reçoit « Amélioration de
  caractéristiques » au lieu d'« Attaque supplémentaire », toute la table décalée d'un cran. En
  `-table`, elle est **exacte**, niveau par niveau. Même règle qu'au paragraphe précédent, cette
  fois démontrée sur un scan : **jamais `-layout` sur un tableau**.
- **Le bruit au niveau du mot** — « Voire primitive » pour « Voie primitive », « sup plémentaire »,
  « unjet ». Visible, non numérique, corrigé par une relecture ordinaire. Désagréable, pas
  dangereux.
- **Les valeurs numériques** — le risque résiduel, celui qu'aucune relecture rapide n'attrape : un
  `1d8` devenu `ld8`, un `+3` devenu `+8`. C'est précisément la raison d'être du `LOT-49`.
- **Les valeurs numériques *absentes*** — pire que le précédent, et découvert en préparant le
  `LOT-43`. Sur la table du multiclassage (`Manuel-Des-Joueurs`, p. 166), l'extraction **supprime
  purement et simplement toute cellule valant `1`** : le chiffre est trop fin pour que l'OCR le
  retienne, et il ne laisse rien derrière lui. **Onze lignes sur vingt** sont amputées d'une à
  quatre cellules, et un magicien de niveau 20 y perd ses deux emplacements de niveau 8 et 9.

  Ce défaut n'est pas de la même famille que les précédents : une valeur *fausse* finit par se
  voir, une valeur *absente* ressemble à une case vide légitime — et cette table en contient de
  vraies. Aucune relecture du texte extrait ne peut le détecter, parce qu'il n'y a rien à relire.

  Ce qui l'a révélé n'est pas une relecture mais un **recoupement** : la table du multiclassage est
  identique à la table de progression du magicien, laquelle figure en **texte natif propre** dans
  les *Basic Rules* (p. 31). Les vingt lignes comparées cellule à cellule donnent onze divergences,
  et **toutes** sont un `1` manquant côté OCR.

  La règle qui en découle vaut pour toute la filière : *une table numérique tirée d'un scan doit
  être recoupée contre une seconde source, ou contre un invariant.* Sans quoi le `LOT-49` cherchera
  des valeurs fausses, et ne trouvera pas les valeurs manquantes.

Conclusion : le `Manuel-Des-Joueurs.pdf` est **exploitable** avec le bon outil et une passe de
contrôle — et il est indispensable, puisqu'il est la seule source française des 5 races et 8 classes
absentes des *Basic Rules*. Le `Manuel-Des-Monstres.pdf` reste hors périmètre, non par impossibilité
mais par arbitrage (§8) : 416 blocs à contrôler pour un bestiaire déjà suffisant.

**L'extraction du flux brut des images est corrompue ; il faut passer par le rendu.** Tirer un objet
image par son `xref` produit sur ce corpus des zones de bruit vert et cyan — décodage raté d'un flux
JPX ou d'un masque alpha. En revanche, **rendre la page** (ou une région clippée de la page) donne
un résultat parfait, parce que le rendu passe par la composition complète. Avec une réserve qui
compte : rendre une région composite **tout ce qui y est dessiné, texte compris**. Sur ces livres où
l'art court sous les colonnes, l'extraction d'illustrations propres est donc **semi-automatique** —
l'outil propose les régions candidates, l'humain garde et recadre. Les éléments d'habillage
autonomes (panneaux de parchemin, cadres, bordures) sortent en revanche parfaitement seuls, et ce
sont eux qui ont alimenté l'interface au [LOT-76](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-76-habillage-interface.md) — jusqu'à ce que la charte v2 du
[LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) remplace ces relevés par des images **produites** depuis un cahier des
assets, le corpus ne servant plus que de référence de cotes et de couleurs.

---

## 4bis. Ce que les deux livres imposent au moteur — relecture du 14 septembre 2026

Les entrants réels du projet ne sont pas les spécifications : ce sont le *Player's Guide to
Tanares* et le *Tanares Sourcebook*. Ils ont été relus **en entier** (2,65 millions de caractères,
neuf lecteurs en parallèle) avant de démarrer les lots de mécanique de cœur, pour que le
[LOT-19](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-19-grille-tactique.md) et ses suivants ne construisent pas un moteur que le contenu de Tanares ne
saura pas traverser. Le résultat tient dans une règle et une matrice.

**La règle.** Le moteur ne cherche pas à implémenter chaque capacité des livres — il y en a
plusieurs centaines, et une bonne part est arbitrée par un maître de jeu. Il doit en revanche
**offrir les crochets** que ces capacités composent (`EX-RPG-050`), et laisser la donnée déclarer ce
qu'elle exige (`EX-CNT-030`) pour que le moteur refuse en le disant ce qu'il ne sait pas honorer
(`EX-CNT-031`). Un crochet manquant au socle se paie en refonte de tous les lots de classe ; un
crochet posé tôt ne coûte qu'une interface. La matrice ci-dessous dit, domaine par domaine, ce
que le socle doit **prévoir** (une interface, un champ, un événement) et ce qu'il peut **ignorer**.

| Domaine | Ce que les livres exigent, chiffres en main | Ce que le socle doit prévoir | Lot porteur |
|---|---|---|---|
| **Tour et initiative** | Actions légendaires (réserve de 3, dépensées en fin de tour d'autrui) ; actions de repaire à l'initiative 20 ; renforts entrant à l'initiative 0 ; un acteur **flottant** qui joue avant n'importe quel tour (*Law of Time*) ; une fenêtre de réaction **avant le premier tour** (*Natural Strategist*) ; octroi d'une réaction supplémentaire à un allié ; fenêtre « une créature *déclare* une attaque » (postures du moine) ; une **troisième économie d'action** (Heroic Action) ; compteurs « une fois par rencontre » et immunités 24 h par couple (créature, source) | La machine à états du tour expose des **points d'insertion** nommés (début de round, avant le premier tour, fin de tour de X, initiative fixe N, à la déclaration d'une attaque) et des compteurs à portée (tour, rencontre, jour) ; l'économie d'action est une **liste** de ressources par tour, pas trois booléens — **posé au [LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md)** : `core::CombatHook` (neuf crochets), repères d'initiative fixe qui perdent les égalités, acteur flottant (`interject`), `core::ActionEconomy` (`declare`, `grant`), `core::ScopedCounters` (tour, round, rencontre, jour) et `core::ImmunityLedger` par couple (créature, source) | [LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md) |
| **Le dé** | d20 brut exposé (*Omen*, *Augurs*, plancher à 10) ; relance **avant** résolution ; modificateur ajouté **après** avoir vu le résultat (*Future Guard*) ; résultats de d20 **stockés** puis substitués ; « si les deux d20 de l'avantage touchent » ; super-avantage à 3d20 | `core::Check` (livré) reste le seul mécanisme, mais son résultat est un **objet** (dés bruts, modificateurs avec origine, seuil, issue, `EX-REG-003`) que des crochets peuvent lire et amender avant que l'issue ne soit figée | [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md) |
| **Dégâts et points de vie** | Dégâts typés avec **drapeaux de source** (magique, adamantium, sort) ; conversion de type (*Shadowcaster*, zone *Maelstrom*) ; « ignore résistances et PV temporaires » ; dégâts redirigés (*Life Link*), différés, par case parcourue ; échange et transfert de PV ; réserves ablatives (*Exoskeleton*, 80 PV) ; PV **mis en commun** (monture) ; PV temporaires sans plafond ; **PV de structures** ; phases par seuil (*Battle Fury* sous 50 %) ; déclencheurs à la mort (spawn, explosion) ; seuil de jets de mort **variable** (Cultist : 4 échecs) ; régénération conditionnelle | Un **pipeline** de dégâts à étapes nommées (source → conversion → résistances → réserves → PV) où chaque étape est un point d'insertion, et un événement « seuil franchi » / « mort » ; les PV sont une pile de réserves, pas un entier | [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md), [LOT-72](#lot-72) |
| **Conditions et afflictions** | Marqueurs **empilables** (poison ×4) ; saignement ; « couvert de lave », « mouillé », « chargé » ; malédictions à **tick quotidien ou hebdomadaire**, levées seulement par *remove curse* ou *wish* ; épuisement à niveaux utilisé comme **coût** ; cécité permanente, mutisme ; malus par paliers (Insanity −1/−2/−3) ; folies aléatoires | Une condition est une **source datée** avec durée en tours **ou en temps de jeu**, empilable et recalculée (`EX-REG-040`) ; une couche « affliction persistante » qui survit au combat et se réveille à l'horloge | [LOT-72](#lot-72), `LOT-70` |
| **La grille** | Zones à **règle locale** sur la carte (zone du rituel des Marques ; combat interdit ; aucun soin ; type de dégâts aléatoire) ; objets **destructibles** posés sur la grille (toiles CA 10 / 10 PV) ; terrain difficile créé en combat ; téléportation, échange de positions, déplacement forcé avec dégâts de collision ; **vol et vol stationnaire** partout (dragons, drakes, ailes) sur un jeu vu de dessus ; requête « une case où l'on peut se cacher » | La grille porte des **propriétés de zone** déclarées par la carte (`EX-LVL-018`) que le combat lit ; les objets de grille sont des entités à PV ; **l'altitude est un attribut**, jamais une géométrie — décision **écrite au [LOT-19](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-19-grille-tactique.md)** : un volant survole les obstacles de sol (eau profonde, falaise) et ignore le terrain difficile, mais pas les murs, et reste ciblable à portée | [LOT-19](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-19-grille-tactique.md), [LOT-22](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-22-portee-ligne-de-vue.md) |
| **L'adversaire** | Tactiques de meute et bonus d'adjacence (+1 par drake, max +5) ; lanceurs de sorts ; boss de repaire (actions de repaire, effets régionaux, ×3 rencontres à 6 miles) ; formes alternatives (*Change Shape* → second bloc) | Les profils de comportement en JSON (`LOT-23`) savent lire les traits de groupe ; un bloc peut **référencer** un autre bloc | [LOT-23](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-23-ia-tactique.md), `LOT-46` |
| **Ressources de classe** | Au moins **six modèles** : réserve dépensée (ki, Courage) ; **jauge montante** à paliers de malus et transformation forcée (Insanity) ; automate positionnel à six cases (Cycle of Redemption) ; postures avec interruptions déclaratives (Way of the Animals) ; points à **gain événementiel** (Soul Points, +1 par mort à 15 ft) ; cadence « un pouvoir par round » + accumulation de marques (Dragonblade) ; tampon « prochain surge » + journal FIFO de deux éléments (Elementalist) ; second système d'emplacements (pacte) ; « N usages par sort » (classes simplifiées) | `EX-RPG-021` reste vraie — une ressource est une donnée — mais la « ressource générique » est une **famille** de six modèles, chacun avec sa cadence, et non un compteur ; le socle en livre l'interface et le guerrier n'en prouve qu'un | `LOT-47` |
| **Substitution de profil** | Forme de dragon (4 profils), *Penumbral Shape* (30), forme sauvage, *Change Shape*, *Machine Infestation* (prend le bloc d'un construct), **deux fiches alternées** (cirrus) | La substitution est un mécanisme du **socle** (PV séparés, report de l'excédent, ce qui est conservé déclaré par la donnée), pas une particularité du druide | `LOT-47`, puis `LOT-61` |
| **Espèces et personnage** | Espèce **composable** (soulborn hérite taille/vitesse/vision d'une autre) ; scores effectifs conditionnels (taii'maku *Equilibrium*) ; sous-classe au niveau **1, 2 ou 3** selon la classe ; artisanat racial à péremption (poison gloomfolk, gadgets) ; compagnons hors quota (gardien-plante, jusqu'à 20 morts-vivants, invocations paramétriques) | Le niveau de greffe est un champ de la classe ; les compagnons sont des combattants **hors groupe** dans l'initiative (`EX-CBT-010`) | `LOT-47`, [LOT-29](#lot-29) |
| **Étiquettes de créature** | « naturel / non naturel », origine planaire, « possède un lancement de sorts », type, FP exposé — lus par le Redeemer, le Dark Hunter, l'Arcane Guardian, *Empathy* | Le schéma de créature porte ces **tags** (`creature.schema.json`, livré) ; le `LOT-46` les remplit | `LOT-46` |
| **Temps** | Calendrier de **12 mois × 30 jours**, 7 jours nommés, an 1298, saisons, solstices, **4 fêtes à bonus mécanique**, **phase lunaire** et périgée (*Bauron's Vigil* 1d4→1d12, lycanthropes, Vandanamalika) ; *Tamera's Light* — les créatures pénombrales n'errent que **la nuit** ; recharges de 7, 30 et 1001 jours ; vieillissement ; échéances mensuelles (prêts, entretien des constructs) ; respawns différés (1d6 jours, 1d10 semaines) | `core::GameClock` est un **calendrier**, pas un compteur jour/nuit, avec une **phase de lune** et des **événements datés** consommables par les rencontres, les malédictions, l'économie | `LOT-70` |
| **Monde et régions** | 13 régions à encart **+ Darkall, Undertanares, Wasteland** ; statistiques à **portée** (nord/sud, surface/souterrain — `EX-CNT-061`, livré) ; **règles de zone** régionales ; légalité de la magie par région ; réputations multiples (Guilde, Conseil draconique à 7 rangs et primes, Relic Hunters notés sur 10, code d'honneur, foi) ; **Undertanares** comme couche sous les régions ; plan pénombral en 7 couches avec ses règles de survie | Un descripteur de région porte ses **règles de zone** et sa **légalité** ; la réputation est un **modèle par organisation**, la Guilde n'en est qu'une instance ; le plan pénombral est un lot à part | `LOT-81`, `LOT-45`, `LOT-90` |
| **Économie et objets** | 5 pièces + électrum standard à Yama ; lettres de crédit, prêts à 5 %/mois, coffres, assurances ; grille de prix des trois minerais ; marché noir et contrebande ; consommables à **durée** (boissons, mets, Estelindea, peintures de guerre) ; **29 objets magiques** à charges, malédictions, harmonisation forcée, variantes par type de dragon ; bénédictions achetées en or ; corrosion d'arme, vol d'objet | Un objet déclare rareté, harmonisation, charges et recharge, malédiction ; l'inventaire (livré) accepte durabilité et vol ; la monnaie est régionale | [LOT-26](#lot-26), `LOT-89` |
| **Voyage** | Table de 19 montures ; portails à coût en bauronite ; disques nains ; failles pénombrales comme raccourcis ; Tritors reliés ; **mer** (Seashores est presque entièrement marine, navires statés) | Une route a un **type** (terre, mer, portail, faille) et un coût ; pas de combat naval | `LOT-42` |
| **Arène** | *Law of the Arena* = justice civile (litiges, champions à louer) ; arènes **régionales** (Future non létale, Feargus létale, Braves débutants, duel de baguettes) ; **paris** avec combat simulé hors écran ; Marques Héroïques (8) ; Tritors ; *Arena Day* | Le Colisée est **un mode du jeu** — banc d'essai des mécaniques d'abord, institution ensuite — et ses variantes régionales sont des **données** de lieu — **posé au [LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md)** : `core::ArenaSession` tient le combat et le rejoue à graine fixée, `Source/Elements/World/arena/` porte trois arènes (`lethal`, `heroicMark`, `map`), les huit Marques sont une règle (`heroic-marks.json`), la *Heroic Action* est déclarée à chaque combattant marqué, et la Marque relève tout le monde à l'issue ; paris, Tritors et *Arena Day* restent à écrire | [LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md) |
| **Compagnie** | Système d'**équipe** complet : niveau 1-20 par *Career Points*, QG à 12 structures × 6 niveaux, 12 employés statés, 12 dons d'équipe, récompenses légendaires — c'est la maquette 09 que le [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) a déjà transcrite (`CompanyForm`) | Deux échelles de progression de groupe coexistent (rangs de Guilde, niveau d'équipe) ; elles se tiennent chacune dans son lot | `LOT-45`, `LOT-83` |

**Ce que la relecture écarte, et pourquoi** — inscrit au §8 pour ne pas être redécouvert :
les montures volantes et leurs *Flying Points* (n'agissent que monté contre monté, sans sens vu de
dessus) ; le combat naval ; les deux fiches alternées du cirrus (une seule face, choisie à la
création) ; les tests « liés aux dragons » ou « mathématiques » (étiquetage sémantique des tests) ;
les sorts arbitrés par le maître de jeu et ceux qui **rejouent un tour** ou **réécrivent un PNJ**
(déclarés narratifs, `EX-RPG-051`) ; les zones **mobiles** (oasis migrante, Deep Freeze qui
s'étend) ; les plans autres que le pénombral (Éthéré, Astral, Yrthak, Elemental Grounds) ; la
Malédiction malrokienne comme compteur global.

**Ce que la relecture a compté, et qui corrige la page** : 13 régions et non 10 ; 82 blocs de
créatures confirmés (66 + 16), plus **Etoraax** (FP 29), 30 formes pénombrales, 5 gardiens-plantes,
3 invocations, 3 montures, 12 employés, 13 créatures orientales de Yama et 3 blocs de Sindile —
soit une soixantaine de blocs **hors** des 82 ; 7 historiques (les 13 du `LOT-36` les incluent) ;
**20 dons de Tanares** absents des 42 dons livrés au `LOT-43` ; 76 sorts du *Player's Guide* et
**191 sorts d'élémentaliste**, chacun avec un second effet (*Elemental Flux*) ; 46 pouvoirs
draconiques, 48 mutations, 16 pouvoirs de mort, 50 bénédictions, 6 postures ; 29 objets magiques ;
15 sous-classes de Tanares pour les classes du SRD, plus l'Amazone ; les 4 classes simplifiées sont
bien les 4 provisoires du `LOT-36`.

---

## 5. Programme de lots proposé — la filière contenu

Les lots [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md) (fiche de personnage), [LOT-14](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-14-inventaire-et-equipement.md) (inventaire),
[LOT-25](#lot-25) (sorts) et [LOT-26](#lot-26) (économie) exigent tous un « catalogue en
**JSON** », conformément à [`EX-VIS-007`](../../../Documentation/Specification/vision.md#EX-VIS-007). Aucun ne disait **d'où sortent ces
JSON**. C'est exactement le trou que ce corpus comble, et c'est le périmètre de cette filière — les
deux premiers sont livrés depuis, sur les catalogues qu'elle a produits.

Trente-cinq lots, `LOT-35` à `LOT-96`, restent à livrer dans cette filière — la plage court
jusqu'au [LOT-96](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-96-quartiers-capitale.md), la ville scindée du `LOT-09`, livré le 18 septembre 2026 — dont
deux ajoutés par la relecture des livres (§4bis) : `LOT-89`, `LOT-90` — le troisième,
[LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md), est livré. Des cinq nés de la relecture du plan d'intégration de la
Capitale, le 16 septembre 2026 (§8), aucun ne reste : le [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md), le
[LOT-93](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-93-atelier-monstres.md) (l'atelier des monstres), le [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) et le
[LOT-96](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-96-quartiers-capitale.md) sont livrés, et le `LOT-95` a été absorbé par le [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md). Sept numéros ont été
**retirés** (`LOT-31`, `LOT-48`, `LOT-69`, `LOT-71`, `LOT-73`, `LOT-95` par fusion, `LOT-85` par abandon : voir
l'encart en fin de section),
et vingt et un lots de la même plage sont **livrés** — les trois préconditions [LOT-77](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-77-specification-rpg.md),
[LOT-78](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-78-numeros-herites.md) et [LOT-79](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-79-socle-chargement-donnees.md), puis `LOT-30`, `LOT-32` à `LOT-34`, `LOT-36` à
`LOT-39`, `LOT-43`, `LOT-66` à `LOT-68`, `LOT-76`, [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md), [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md),
[LOT-93](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-93-atelier-monstres.md), [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) et [LOT-96](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-96-quartiers-capitale.md) — qui ont donc quitté cette page pour leur
dossier, comme tout lot livré. Les numéros sont, comme toujours, des identifiants stables : ils
viennent après [LOT-29](#lot-29) dans la numérotation, mais plusieurs s'exécutent **avant**
les lots qui les consomment (voir §6). La famille d'exigences `EX-CNT-*` les couvre, dans
`Documentation/Specification/contenu.md` ([LOT-77](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-77-specification-rpg.md)).

> **État de l'audit.** Cette page a été confrontée à l'état réel du dépôt, puis à elle-même. Six
> lots dépassaient l'étalon mesuré sur les **huit lots livrés** — `LOT-01` à `LOT-08`, soit 16 à 34
> fichiers et 450 à 1340 lignes : le `LOT-04` marque la borne haute à 19 fichiers pour 1337 lignes,
> le `LOT-08` la borne basse à 16 fichiers pour 453 lignes. Ils ont été **découpés** : `LOT-37`/`LOT-80`, `LOT-40`/`LOT-81`, `LOT-41`/`LOT-82`,
> `LOT-45`/`LOT-83`, `LOT-47`/`LOT-84`, et le `LOT-69` réduit à la suppression qu'il est réellement.
> Le principe de coupe est le même partout : **le code d'un côté, la donnée de l'autre**, chaque
> moitié ayant son consommateur et son critère d'acceptation.
>
> Les epics réels font 33 à 89 lignes ; les sections ci-dessous en font environ 27, donc
> **sous-spécifiées comme epics** — elles seront reprises au démarrage de chaque lot. Aucune ne
> portait de rubrique « Exigences couvertes », faute de familles `EX-*` existantes : le `LOT-77`
> les a écrites.
>
> **Second audit, le 14 septembre 2026** (§9.6) : trente et un lots livrés, dont deux refontes
> d'interface (`LOT-86`, `LOT-87`) décidées hors de cette page. Il a corrigé ce qui avait dérivé —
> un `LOT-27` que le lint croyait prêt, des sections de lots qui décrivaient des classes retirées,
> une charte remplacée — sans changer un seul périmètre.

Trois formes de sortie, et le choix entre elles n'est pas cosmétique. **JSON** pour tout ce qui est
structuré et imbriqué — une créature a des actions, une classe a une progression par niveau — et
parce que le projet lit déjà du JSON (`skins.json`, `sounds.json`, `palettes.json`). **CSV** pour la
seule donnée réellement tabulaire et plate du corpus, le lexique du `LOT-30` : 1 200 lignes de trois
colonnes, qu'on veut pouvoir trier, comparer et corriger dans un tableur sans passer par un éditeur
de code. **PNG** enfin, pour les images de l'interface — non plus des textures **extraites** du
corpus, comme le `LOT-39` le prévoyait, mais des images **produites** à 1080p depuis le cahier des
assets du [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) ; le `LOT-39` a livré la plomberie des clés et le marqueur généré
qui tient lieu de toute image absente.

### Importer tout, honorer progressivement

Le périmètre est **le monde complet** : les 13 régions et leurs lieux, les 13 espèces, les 8 classes
et leurs sous-classes, les historiques, les factions, le panthéon, le bestiaire entier. Pas de
socle réduit, pas de tri préalable.

C'est le bon choix, et il est peu coûteux : extraire treize régions ne demande pas dix fois le travail
d'en extraire une, puisque c'est le **parseur** qui coûte, pas les données qu'il avale. Mais il
introduit un écart qu'il faut nommer tout de suite, sous peine de le découvrir en jeu :

> **Une donnée importée n'est pas une mécanique implémentée.** L'élémentaliste a sa propre liste de
> sorts, le madwalker ses *trails*, le redeemer ses *blessings*. Importer ces classes prend une
> après-midi ; faire que le moteur les joue correctement est un programme entier.

La règle qui évite le piège est celle que le [LOT-25](#lot-25) pose déjà pour les sorts, étendue
à toute la filière : **chaque donnée déclare les mécanismes dont elle a besoin**, et le moteur
**refuse en le disant** ce qu'il ne sait pas honorer. Une classe dont le moteur ignore la ressource
propre se signale au chargement, elle ne se joue pas en silence comme une classe ordinaire amputée
de ce qui la définit.

Ainsi le catalogue peut être complet longtemps avant le moteur, sans jamais mentir sur ce qui est
jouable — et l'écart entre les deux devient une liste consultable plutôt qu'une surprise.

### La cible : un bac à sable dans l'univers entier

Le jeu visé dans un premier temps est un **bac à sable** — le personnage parcourt les treize régions
librement, sans intrigue directrice. Ce cadrage n'ajoute pas une contrainte à la filière, il en
change la nature, et pour le mieux.

Un bac à sable se nourrit de contenu **systémique** : les rencontres naissent d'une table et d'un
lieu, pas d'un script ; le stock d'un marchand se déduit de sa région ; une quête se compose à
partir des objectifs d'une faction et de l'état du monde. Rien de tout cela ne s'écrit à la main
région par région — et c'est précisément ce que le corpus permet d'éviter, parce que les
**`Regional Statistics` sont déjà des réglages de bac à sable**. `Monster Presence`, `Magic Access`,
`Economic Prosperity`, `Crime and Violence` : quatre axes notés, treize régions, et le monde se
différencie mécaniquement sans qu'une seule valeur soit inventée.

D'où trois lots supplémentaires, `LOT-40` à `LOT-42`, qui transforment l'atlas en monde parcourable :
le terrain, son peuplement, et le voyage entre les régions.

Cela ne remplace pas le [LOT-27](#lot-27), qui garde sa fonction : prouver que la boucle
complète tourne — explorer, parler, déclencher, combattre, gagner — sur **deux** quartiers de la
Capitale et leur bas-fond (§8, décision du 16 septembre 2026). Un bac à sable est cette boucle
répétée sur cent lieux ; la construire sur cent lieux avant
de l'avoir validée sur un seul multiplierait simplement par cent le coût de chaque correction.

### `LOT-35` — Sorts et états {#lot-35}

*Prérequis : `LOT-32`. Alimente [LOT-25](#lot-25).*

*Exigences couvertes : `EX-RPG-050`, `EX-RPG-051`.*

Les sorts et l'appendice des états des *Basic Rules* vers `spells/` et `conditions/`, complétés par
les sorts propres au *Player's Guide* : **76 sorts** du chapitre 4 (dont cinq cités par les blocs
du bestiaire) et **191 sorts d'élémentaliste** cloisonnés par élément, chacun portant un second
effet (*Elemental Flux*) et 36 sorts de fusion — le catalogue de sorts de ce jeu est donc plus
grand que celui du SRD, et sa moitié Tanares se code en **deux champs par sort** (§4bis).

La règle qui compte est celle que le [LOT-25](#lot-25) énonce déjà : le C++ ne porte que des
*mécanismes* (dégâts de zone, jet de sauvegarde, condition appliquée, durée), la donnée les compose.
Un sort dont l'effet n'entre pas dans ces mécanismes se déclare **explicitement**
`"effet": "narratif"` — un sort tu, qu'on croit implémenté et qui ne fait rien, coûte bien plus cher
à diagnostiquer qu'un sort déclaré non joué.

*Acceptation* — chaque sort porte école, niveau, portée, durée, composantes, effet ; aucun sort ne
tombe silencieusement dans un cas par défaut, un test énumère le catalogue et l'exige.

### `LOT-40` — Générateur de terrain {#lot-40}

*Prérequis : `LOT-81`, [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md). Alimente `LOT-41`.*

> **Découpé à l'audit.** Ce lot mêlait un **générateur** (du C++ déterministe, testable sans aucune
> donnée de région) et les **descripteurs des treize régions** (de la donnée extraite et relue). Deux
> métiers, deux critères d'acceptation : les descripteurs partent au `LOT-81`. Ne reste ici que le
> moteur, qui se valide sur un descripteur de test.

Treize régions, une dizaine de lieux nommés chacune : environ **cent cartes**. Dessinées à la main dans
l'éditeur, à raison de quelques heures pièce, c'est une année de travail pour un développeur seul —
et la certitude que le monde ne sera jamais fini. Elles doivent donc se **générer**.

La génération n'est pas un pis-aller ici, parce que la matière existe déjà. Le `LOT-81` livre pour
chaque région un descripteur de terrain dérivé de sa section `Geography`, et le vocabulaire de
tuiles est posé depuis le [LOT-08](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-08-tuiles-rpg.md) : `Grass`, `Dirt`, `Sand`, `Water`, `DeepWater`,
`Wall`, `Cliff`, `Bridge`, `Stairs`. Le Central Empire annonce « vallées fluviales, vastes
prairies, forêt du Bak, marais, hauts plateaux du nord » ; le Freelands « landes et zones humides,
forêts tempérées, chaînes glacées, deux mers ». Ce sont des recettes de terrain, pas de la prose
d'ambiance.

Le lot produit un générateur qui, d'un descripteur de région et d'une **graine dérivée de l'identité
du lieu**, rend une carte au format du [LOT-04](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-04-format-v3-multicouches.md) : couches, collision, portails. La
graine dérive du lieu et non de l'horloge — même raison qu'au [LOT-26](#lot-26) pour les coffres :
une carte qui se re-tire différemment à chaque chargement n'est pas un monde, c'est un kaléidoscope.

Le rapport à l'éditeur est le point à ne pas manquer : la génération **produit un niveau ordinaire**,
que le [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) ouvre, corrige et enregistre comme n'importe quel autre. Un générateur
dont la sortie n'est pas éditable oblige à choisir entre tout générer et tout dessiner ; celui-ci
permet de générer les cent cartes et d'en finir dix à la main — les seules que le joueur regardera
de près.

Les **31 cartes tactiques** de `VTT/Adventure Map Grids.pdf` (§2) donnent au générateur ce qui lui
manquait : un **étalon**. Elles sont peintes vues de dessus et alignées sur une grille, c'est-à-dire
sous l'angle exact où ce lot produit — ce qu'aucune autre matière du corpus ne montre. Une carte
générée qui ne soutient pas la comparaison avec l'une d'elles n'est pas une carte finie.

*Acceptation* — une même graine et un même descripteur rendent deux fois la carte **identique** ;
toute carte générée est traversable de son entrée à sa sortie, vérifié par un parcours automatique ;
une carte générée s'ouvre dans l'éditeur, se modifie et se recharge sans perte. Le générateur se
teste sur un descripteur **de fixture**, sans dépendre des treize régions réelles.

### `LOT-41` — Peuplement : rencontres et créatures {#lot-41}

*Prérequis : `LOT-40`, `LOT-33`, `LOT-46`, `LOT-44`, [LOT-27](#lot-27).
Alimente `LOT-82`.*

> **Découpé à l'audit.** Ce lot dérivait **deux peuplements sans rapport** des mêmes statistiques :
> le danger (créatures, rencontres, embuscades) et le civil (PNJ, marchands, prix, quêtes). Deux
> dérivations indépendantes, deux jeux de données d'entrée, deux mesures d'acceptation. Le civil
> part au `LOT-82`.

Une carte vide n'est pas un lieu. Ce lot y met ce qui est **hostile**, et il le fait **par déduction
depuis les données de région**, jamais par placement manuel — c'est ce qui rend treize régions tenables.

Trois des sept `Regional Statistics` pilotent ce versant :

| Statistique | Ce qu'elle pilote |
|---|---|
| `Monster Presence` | Densité des rencontres, dangerosité des tables de la région |
| `Crime and Violence` | Embuscades sur les routes, fréquence des rencontres hostiles |
| `Political Stability` | Présence de gardes, donc de rencontres qui tournent mal |

Le bestiaire des 176 créatures y est filtré par région : une table de rencontre se compose de ce qui
vit là, à la dangerosité que la région annonce, et non d'un tirage uniforme dans le catalogue.

*Contraintes du corpus (§4bis).* Les créatures pénombrales n'errent que **la nuit** (*Tamera's
Light* : 10 dégâts radiants par tour au soleil) — la table lit l'horloge du `LOT-70`. Un repaire
de dragon **triple** la fréquence des rencontres à six miles : un lieu peut porter un
modificateur. L'Undertanares est une **couche** sous les régions, avec ses propres tables.

*Acceptation* — deux régions aux statistiques opposées produisent des **densités et des tables de
rencontre mesurablement différentes**, vérifié par un test ; le peuplement est reproductible à
graine égale ; aucune rencontre n'est placée en dur dans le code ; aucune créature n'apparaît dans
une région dont le biome ne la porte pas.

### `LOT-42` — Voyage et carte du monde {#lot-42}

*Prérequis : `LOT-82`, [LOT-68](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-68-chassis-ecrans-rpg.md), `LOT-70`, [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md), [LOT-17](#lot-17),
[LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) (l'écran « Carte » à trois niveaux et les cartes de l'auteur, sur lesquels ce
lot bâtit).*

Un univers vaste ne vaut que si l'on peut le parcourir. La **carte** existe : le
[LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) a livré l'écran, les seize cartes peintes par l'auteur et les lieux de
l'atlas placés dessus. Ce lot y ajoute ce qui en fait un voyage — les lieux **découverts**, la
position de la compagnie — et le déplacement entre régions : routes,
voyage rapide vers un lieu déjà visité, et le coût que ce voyage représente (temps, ravitaillement,
risque de rencontre selon la `Crime and Violence` traversée).

*Contraintes du corpus (§4bis).* Une route a un **type** — terre, mer (les Seashores sont
presque entièrement marines), portail (trois gros tameranium à bâtir, une bauronite par usage),
disque nain, faille pénombrale (Yama → Freelands en six heures, au prix de ce qu'elle attire),
Tritor — et une monture parmi les **19** de la table du ch. 8 change son coût. Pas de combat
naval : la mer est une route, pas un champ de bataille.

Il porte aussi la **découverte** : un lieu se révèle en y arrivant ou en l'apprenant d'un PNJ, et
cet état de découverte entre dans la sauvegarde du [LOT-17](#lot-17). Sans cela, un monde de
cent lieux s'ouvre entièrement dès la première seconde et n'a plus rien à offrir.

**L'écran est là ; ce lot n'a plus de carte à produire.** `WorldMapForm.ui.qml`
([LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md), maquette 08) posait la carte du monde **extraite du corpus** ; le
[LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) l'a retirée, parce qu'une œuvre du corpus n'est pas un asset du jeu (§8,
décision du 16 septembre 2026), puis a rebâti l'écran sur les cartes **peintes par l'auteur**
(décision du 17) : trois niveaux — monde, région, ville —, les positions dans
`Source/Elements/Maps/world-maps.json`, à part de l'atlas, et le modèle `hmi::WorldMapModel`. Ce
lot **bâtit dessus** : il n'affiche que les lieux découverts, pose la position de la compagnie,
et fait de « ouvrir un lieu » le choix d'une destination — et **on ne s'y déplace pas** : la carte
sert à choisir et à s'orienter, le voyage la quitte pour une carte de niveau à l'arrivée. Il livre
la **donnée** du voyage (routes, types, coûts) et le modèle qui la porte. Un lieu que le livre ne
situe pas reste sans marqueur : le jeu n'invente pas de position (`EX-IHM-107`).

*Acceptation* — le joueur atteint les treize régions par le seul jeu, sans commande de débogage ;
l'écran « Carte » du [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) n'affiche que ce qui est découvert ; l'état de découverte survit à une sauvegarde et
à un rechargement.

### `LOT-44` — Noms, tables aléatoires et contenu d'ambiance {#lot-44}

*Prérequis : `LOT-36`, `LOT-37`. Prérequis de `LOT-41`.*

Un bac à sable peuple des centaines de PNJ. Il lui faut donc des **noms**, et le corpus en fournit
par espèce et par culture : les *Basic Rules* listent noms masculins, féminins et de famille pour
chaque race, le *Player's Guide* fait de même pour les siennes (`Male Names: A'Kole, B'Abku…` chez
les taii'maku). Croisés avec la répartition d'espèces d'une région, ils donnent des habitants
plausibles plutôt qu'une suite de « Villageois 1 ».

S'y ajoutent les **tables aléatoires** déjà écrites : les traits, idéaux, liens et défauts en `d6`
de chaque historique — de quoi donner une personnalité à un PNJ sans en écrire une —, la table de
**babioles** (p. 59), et le contenu d'ambiance du chapitre 8 du Sourcebook : animaux de compagnie,
boissons, fêtes, nourriture, mesure du temps, et les **jeux** — quatre jeux de taverne à DD, le
Dragon Gateway de Yama, les jeux de Mirare, la Greatwolf Race — qui sont des mini-jeux à
tables, pas des mécaniques : ce lot livre leurs tables, pas leurs écrans.

C'est le lot le moins spectaculaire de la filière et l'un des plus rentables : c'est lui qui fait la
différence entre un monde peuplé et une grille d'entités.

*Acceptation* — mille PNJ générés dans une région donnent une distribution d'espèces conforme à
celle déclarée par la région, et aucun doublon de nom complet en deçà d'un seuil annoncé ; la
génération est reproductible à graine égale.

### `LOT-45` — Guilde des Aventuriers : rangs et contrats {#lot-45}

*Prérequis : `LOT-82`, [LOT-68](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-68-chassis-ecrans-rpg.md), [LOT-16](#lot-16), [LOT-26](#lot-26).
Alimente `LOT-83`.*

> **Découpé à l'audit.** Ce lot portait à la fois le **catalogue** (rangs, bandes de facteur de
> puissance, gabarits de contrat, tableau d'affichage) et la **boucle de progression** (monter de
> rang, ce que cela change, ce que cela débloque). Le premier est de la donnée et un écran ; le
> second est une mécanique qui suppose l'expérience du `LOT-74`. La boucle part au `LOT-83`.

C'est la **colonne vertébrale du bac à sable**, et elle n'est pas à inventer : le Sourcebook la
décrit dans la République des Freelands. La Guilde enregistre les aventuriers, affiche les quêtes
déposées par les citoyens, et applique un **classement du fer au diamant** dont l'objet explicite est
d'« empêcher les aventuriers inexpérimentés de prendre des tâches au-dessus de leurs moyens ».

Chaque rang porte sa correspondance chiffrée :

| Rang | Niveau de personnage | FP des PNJ |
|---|---|---|
| Fer | 1 à 2 | ⅛ à ¼ |
| Cuivre | 3 à 4 | ½ à 1 |
| Bronze | 5 à 6 | 2 à 4 |
| Argent | 7 à 10 | 5 à 7 |
| Or et au-delà | 11 à 16 | 8 à 13 |

C'est exactement ce qui manque à un monde ouvert : un **tableau de quêtes** diégétique où déposer le
contenu généré par le `LOT-41`, une **échelle de difficulté** qui dit quelle créature opposer à quel
niveau, une **raison de progresser**, et un garde-fou qui évite au joueur de niveau 2 de se faire
étriller dans une région d'Argent sans avertissement.

Les halls de guilde étant « bâtis jusqu'au-delà des Freelands », le système s'étend naturellement aux
treize régions et donne au `LOT-42` des points d'ancrage pour le voyage.

*Contraintes du corpus (§4bis).* La Guilde n'est pas la seule échelle : le **Conseil draconique**
a sept rangs et paie des primes sur les créatures pénombrales, les **Relic Hunters** notent sur
dix, Kolbjörn a un code d'honneur, le Kepesh une réputation religieuse, l'Empire un régime de
délation indexé sur `Citizen Freedom`. Ce lot livre donc un **modèle de réputation par
organisation** dont la Guilde est la première instance, pas une mécanique de guilde.

**L'écran existe déjà.** Le tableau de la Guilde du [LOT-68](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-68-chassis-ecrans-rpg.md) a été absorbé par
`CompanyForm.ui.qml` ([LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md), maquette 09) — quatre onglets : Équipe, Recrutement,
Contrats, Réserve — dont **26 clés `company.*`** attendent ce lot et le `LOT-83`. Ce lot alimente
l'onglet Contrats ; il ne redessine rien.

*Acceptation* — un contrat n'est proposé qu'au rang correspondant ; la difficulté d'un contrat
généré respecte la bande de FP de son rang, vérifié sur les cinq rangs ; le tableau de quêtes
affiche ce que le `LOT-82` a composé, sans qu'aucun contrat ne soit écrit à la main.

### `LOT-46` — Créatures de Tanares {#lot-46}

*Prérequis : `LOT-33`, `LOT-30`, [LOT-93](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-93-atelier-monstres.md) (l'atelier des monstres, qui dessine ces blocs depuis leur
texte seul).*

Les **82 blocs de Tanares** (ch. 9, 17 familles : akhu, emoguns, gloomfolk, ironhands, kemets,
kepesh, kikoku, dragons et drakes pénombraux, ninjas spectraux, constructs taii'makian…). Gabarit
analogue aux 94 du `LOT-33`, mais **en anglais** : c'est l'étape de traduction, via le lexique du
`LOT-30`, qui justifie un lot séparé plutôt qu'un pipeline supplémentaire dans le précédent.

Avec les 94 du SRD, cela porte le bestiaire à **176 créatures** — et c'est le total retenu. Les 416
blocs du *Manuel des Monstres* sont **écartés du périmètre** (voir §8) : 176 profils suffisent
largement à peupler treize régions, et ils épargnent le seul gisement du corpus dont chaque valeur
numérique aurait dû être relue.

> **À réexaminer dans ce lot** : le `VTT/Character Compendium` (§2), arrivé après cet arbitrage,
> porte 169 pages de PNJ et de créatures en texte **natif** — l'argument de l'OCR bruité qui
> écartait le *Manuel des Monstres* ne s'y applique pas. Le total de 176 était un choix de
> périmètre, pas une limite de matière : c'est ici qu'il se rediscute, chiffres en main.

Ce lot est donc **borné et achevable**, contrairement à ce qu'il était quand il portait le *Manuel
des Monstres* : 17 familles, une fin, un critère de fin.

*Contraintes du corpus (§4bis).* Les 82 blocs sont confirmés (66 au ch. 9 après l'index, 16
avant), et la relecture en trouve une **soixantaine d'autres** hors du ch. 9 — Etoraax (FP 29),
30 formes pénombrales du madwalker, 5 gardiens-plantes, 13 créatures orientales de Yama,
invocations, montures, employés — qui sont des **fiches jouables** au même gabarit : ce lot les
prend, ou dit lesquelles il laisse aux lots de classe. Chaque bloc porte les **étiquettes** que
les classes lisent (naturel / non naturel, origine planaire, lanceur de sorts, type, FP) et
**déclare les mécanismes qu'il exige** (`EX-CNT-030`) : actions légendaires et de repaire,
effets régionaux, phases par seuil, déclencheur à la mort, forme alternative référençant un
autre bloc, aura, régénération conditionnelle — quatorze blocs en dépendent. Le moteur les
liste au chargement (`EX-CNT-031`) ; le `LOT-20` et le `LOT-21` les honorent un à un. Les
coquilles du livre sont à corriger à l'extraction (seuils d'aura de l'Ancient Bauronite copiés
de l'adulte, INT 17 (−3), CR 6 pour 5 000 XP).

*Acceptation* — les 82 profils chargent et **déclarent** ce qu'ils exigent ; ceux dont le moteur
honore tous les mécanismes sont jouables en combat, les autres sont listés, jamais joués amputés ;
chaque terme de règle traduit est conforme au lexique du `LOT-30` ; dix profils sont vérifiés à la
main contre le PDF.

### `LOT-47` — Socle de classe, et le guerrier comme preuve {#lot-47}

*Prérequis : `LOT-36`, `LOT-43`, `LOT-84`.*

*Exigences couvertes : `EX-RPG-021`, `EX-RPG-022`, `EX-RPG-023`, `EX-RPG-052`.*

> **Découpé à l'audit, et allégé d'une dépendance.** Ce lot portait aussi l'**extraction des 31
> tables de progression** — de la donnée, relue ligne à ligne, qui appartient à la filière contenu
> et n'a aucune raison d'attendre le socle : elle part au `LOT-84`. Et il déclarait le `LOT-49` en
> prérequis, ce qui faisait attendre au socle de classe un contrôleur de cohérence numérique qui
> attend lui-même le catalogue d'objets. Le lien est passé en « alimente » : le `LOT-49` contrôle
> les tables quand elles arrivent, il ne les précède pas.

Non pas les seize classes, mais **ce qui leur est commun** — et une seule classe pour le démontrer.

Grouper seize classes dans un lot serait cacher une phase entière derrière un numéro. Une classe
n'apporte pas des données, elle apporte une **mécanique** : la rage n'est pas la magie de pacte, la
forme sauvage n'est pas l'attaque sournoise. Chacune se code, se teste et se règle séparément. D'où
le découpage en `LOT-51` à `LOT-65`, **une classe par lot**.

Ce lot-ci pose donc le contrat commun : ce qu'une classe **déclare** (dé de vie, maîtrises,
caractéristiques de sauvegarde, table de progression sur 20 niveaux, niveau de choix de sous-classe
— **1, 2 ou 3** selon la classe, le clerc au premier, le druide, le madwalker et le redeemer au
second —, ressources propres, présence ou non d'un *Spellcasting* pour le multiclassage), comment
une **ressource de classe** se décrit — et la relecture des livres (§4bis) dit qu'il y a **six
modèles**, pas un : réserve dépensée, jauge montante à paliers, automate positionnel, postures à
interruptions, points à gain événementiel, cadence par round avec marques ; chacun a sa cadence
de récupération et ce qu'il alimente —, comment une **sous-classe** se greffe sans dupliquer sa
classe mère, et comment un personnage **substitue son profil** (forme de dragon, forme
pénombrale, forme sauvage : PV séparés, excédent reporté, ce qui est conservé déclaré par la
donnée). Ce dernier point n'est pas une particularité du druide : quatre classes en dépendent.

Il le prouve sur le **guerrier**, la classe la plus simple : pas d'incantation, deux ressources
seulement (Second souffle, Fougue), et l'Attaque supplémentaire qui suffit à valider la boucle
d'attaque du [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md). Si le socle ne tient pas pour le guerrier, il ne tiendra pour
personne.

Les **31 tables de progression** que le `LOT-84` a extraites sont ici **consommées** : le socle les
lit, il ne les produit pas.

*Acceptation* — le guerrier est jouable du niveau 1 au niveau 5, ses deux ressources se consomment
et se récupèrent au bon repos ; ajouter une classe ne demande de toucher à **aucun** fichier C++
existant hors l'ajout de sa mécanique propre.

### `LOT-49` — Contrôle de cohérence du contenu {#lot-49}

*Prérequis : `LOT-33`, `LOT-34`.*

*Exigences couvertes : `EX-CNT-050`.*

Le `LOT-32` valide la **structure** : un fichier bien formé, des champs présents, des énumérations
connues. Il ne dit rien de la **plausibilité**. Un loup à CA 47, une épée à 3 pièces d'or au lieu de
30, une créature de FP ⅛ avec 90 points de vie : tout cela franchit un schéma sans broncher.

Écarter le *Manuel des Monstres* retire le pire des risques — l'OCR bruité — mais **pas le risque
lui-même**, qui vient des tableaux et qui est démontré au §4 : sur la table des armes, `-layout`
attribue le poids et le prix à l'arme de la ligne suivante. Les 31 tables de progression du `LOT-84`
et les tables d'équipement du `LOT-34` courent exactement ce danger, et une valeur décalée d'une
ligne ne lève aucune alerte.

Le lot ajoute donc un contrôle **statistique** plutôt que syntaxique : pour chaque famille, les
bornes attendues d'une valeur au regard des autres champs — PV cohérents avec le dé de vie et la
Constitution, bonus d'attaque cohérent avec la caractéristique et le facteur de puissance, prix
cohérent avec la rareté. Ce qui sort des bornes est **signalé, pas rejeté** : une créature
volontairement hors norme existe, un OCR raté aussi, et seul un humain les distingue.

*Acceptation* — une valeur sciemment corrompue dans un profil de test est détectée ; le rapport
distingue l'anomalie confirmée de l'anomalie acceptée, et cette acceptation est **enregistrée dans
la donnée** pour ne pas être re-signalée à chaque exécution.
### `LOT-51` à `LOT-65` — une classe par lot {#lot-51}

*Prérequis de chacun : `LOT-47`, `LOT-50`, `LOT-70`.*

Quinze lots sur le même patron, un par classe restante. Chacun livre : la classe complète du niveau
1 au niveau 20, **sa mécanique propre implémentée dans le moteur**, au moins une sous-classe, et ses
tests.

Le critère d'acceptation est commun et tient en une phrase : **la classe se joue dans le Colisée
contre un adversaire de son niveau, et sa mécanique propre s'y observe** — la rage réduit
effectivement les dégâts subis, le châtiment divin consomme bien un emplacement, la forme sauvage
change réellement le profil. Une classe dont la mécanique ne se voit pas en combat n'est pas livrée.

| Lot | Classe | Mécanique propre | Ce qu'elle exige du moteur |
|---|---|---|---|
| `LOT-51` | Barbare | Rage, défense sans armure | Ressource par repos long, résistance conditionnelle, CA calculée autrement |
| `LOT-52` | Roublard | Attaque sournoise, Ruse | Condition d'avantage ou d'allié adjacent, action bonus de déplacement |
| `LOT-53` | Clerc | Conduit divin, sorts préparés | Emplacements, distinction préparés / connus, canal à usage limité |
| `LOT-54` | Magicien | Grimoire, restauration arcanique | Préparation depuis un grimoire, récupération partielle d'emplacements |
| `LOT-55` | Barde | Inspiration bardique | Dé confié à un **autre** personnage, dépensé plus tard, à distance |
| `LOT-56` | Moine | Ki, arts martiaux | Ressource par repos **court**, progression des attaques à mains nues |
| `LOT-57` | Paladin | Châtiment divin, auras | Conversion d'emplacement en dégâts, effet de zone permanent autour du porteur |
| `LOT-58` | Rôdeur | Ennemi juré, explorateur | Bonus conditionnés au **type** de créature et au terrain |
| `LOT-59` | Ensorceleur | Points de sorcellerie, métamagie | Conversion ressource ↔ emplacement, altération d'un sort à l'incantation |
| `LOT-60` | Sorcier | Magie de pacte, manifestations occultes | **Un second système d'emplacements** : peu nombreux, toujours au niveau maximal, récupérés au repos court |
| `LOT-61` | Druide | Forme sauvage | **Substitution complète du profil de créature** — le plus lourd du lot |
| `LOT-62` | Élémentaliste | Éléments primordiaux, *Elemental Flux*, sorts de fusion | **191 sorts** cloisonnés par élément, un tampon « prochain surge » écrasé par chaque sort, un journal FIFO des deux derniers éléments, un cantrip qui consomme des emplacements |
| `LOT-63` | Dragonblade | Pouvoirs draconiques, âmes | Martial à l'**Intelligence** ; cadence « un pouvoir par round » + accumulation de trois marques ; réaction **avant le premier tour** ; 46 pouvoirs typés Claw/Scale/Wing ; 4 formes de dragon ; sous-classe au niveau 2 ; pas de *Spellcasting* |
| `LOT-64` | Madwalker | Mutations, *Insanity*, formes pénombrales | Jauge **montante** à paliers de malus et transformation forcée ; 48 mutations, plusieurs actives, une activation par round ; **30 formes** de substitution ; *trail* au niveau 2 ; pas de *Spellcasting* |
| `LOT-65` | Redeemer | Rayons, Cycle, bénédictions | Rayons à 60 ft qui **véhiculent** un sort ; automate à six positions ; dégâts qui ignorent les résistances des créatures **non naturelles** (étiquette) ; 50 bénédictions achetées en or et harmonisées ; sous-classe au niveau 2 |

Chaque classe du SRD reçoit en outre **sa sous-classe de Tanares** (quinze dans le *Player's
Guide*, plus l'Amazone pour le guerrier) : la Voie des Animaux du moine avec ses postures, le
domaine du Chaos qui prépare sur la liste d'une autre classe, le Chevalier de la Mort et ses
points d'âme à gain événementiel, le Cultiste qui meurt au quatrième échec — c'est là que les six
modèles de ressource du `LOT-47` se vérifient.

Deux d'entre eux méritent d'être vus venir. Le **druide** (`LOT-61`) remplace le profil entier du
personnage par celui d'une bête pendant sa forme sauvage : c'est le seul cas où une classe touche à
l'agrégat que le [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md) suppose stable, et il vaut mieux l'aborder tard, une fois le
reste éprouvé. Le **sorcier** (`LOT-60`) introduit un système d'emplacements parallèle : si le
`LOT-25` a codé « les emplacements » au singulier, il faudra y revenir — autant le savoir avant.

L'ordre du tableau n'est pas alphabétique : il va du plus simple au plus intrusif, pour que chaque
lot bénéficie du précédent.

**Le dernier lot livré retire l'échafaudage** : les quatre classes provisoires du `LOT-36` sont
supprimées, et rien ne doit s'en apercevoir.

### `LOT-70` — Horloge, calendrier et lune {#lot-70}

*Prérequis : [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md). **Débloque `LOT-51`→`LOT-65`, [LOT-25](#lot-25),
`LOT-75` et `LOT-42`.***

*Exigences couvertes : `EX-REG-030`, `EX-REG-031`, `EX-REG-032`.*

> **Élargi au second audit.** « Horloge et cycle jour/nuit » ne suffit pas : les livres datent
> tout. Un **calendrier** de 12 mois × 30 jours et 7 jours nommés (an 1298), des saisons et des
> solstices, quatre fêtes à bonus mécanique, une **phase de lune** et son périgée (*Bauron's
> Vigil* : 1d4 → 1d12 PV temporaires selon la phase ; lycanthropes ; Vandanamalika), des
> recharges de 7, 30 et 1001 jours, des malédictions à tick quotidien, des échéances mensuelles
> (prêts, entretien des constructs), des respawns différés. Et surtout *Tamera's Light* : les
> créatures pénombrales ne sortent que **la nuit** — le premier consommateur de l'horloge n'est pas
> le repos, c'est la table de rencontres du `LOT-41`. `core::GameClock` expose donc une **date**,
> une **phase de lune** et des **événements datés** que les autres lots consomment.

> **Fusionné à l'audit.** Ce lot a absorbé l'ancien `LOT-71` (repos court et long). L'horloge n'a
> **aucun consommateur** hors du repos et du campement : livrée seule, elle ne produit rien
> d'observable, et le §6 les traitait déjà comme une paire indissociable. Le numéro `LOT-71` est
> retiré.

`core::GameClock` : un temps **de jeu**, distinct du temps réel, qui avance en exploration et se
**gèle en combat** — un combat se compte en tours, pas en minutes. Cycle jour/nuit, calendrier
simple.

**La fenêtre du §10 est fermée.** Le [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md) a été livré sans que la fiche déclare
ses ressources ni leur cadence : `CharacterSheet` ne connaît ni repos ni ressource. Ce lot les
rétro-adapte — c'est le « coût si on attend » que le tableau du §10 annonçait, et il est dû.

**Le piège.** Le temps de jeu ne doit jamais dériver de l'horloge système. Le [LOT-26](#lot-26)
interdit déjà les graines liées à l'horloge, et pour la même raison : une sauvegarde rechargée
décalerait tout. Le temps avance par pas de simulation, et par rien d'autre.

**Et ce qu'un repos restaure.** Un repos court (1 h de jeu) et un repos long (8 h) : points de vie
via les dés de vie, ressources de classe selon **leur** cadence, emplacements de sorts. Interruption
par une rencontre.

**Le second piège.** Chaque ressource déclare sa propre cadence — repos court, repos long, à
volonté — et `Rest` ne connaît **aucune** classe. Sans cela, chacun des quinze lots de classes à
venir modifierait le code du repos, et la quinzième modification casserait la première.

*Acceptation* — 24 h de jeu s'écoulent en un nombre déterministe de pas ; une ressource « repos
court » se restaure au repos court et pas avant ; un repos interrompu ne restaure rien ; les quatre
classes provisoires du `LOT-36` récupèrent correctement sans que `Rest` les connaisse ; l'horloge ne
bouge pas
pendant un combat ; elle survit à une sauvegarde et à un rechargement.

### `LOT-72` — Conditions, agonie et mort {#lot-72}

*Prérequis : [LOT-12](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-12-des-caracteristiques-jets.md), [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md), `LOT-35`.*

*Exigences couvertes : `EX-REG-040`, `EX-REG-041`, `EX-CBT-040`, `EX-CBT-041`, `EX-CBT-042`.*

> **Fusionné à l'audit.** Ce lot a absorbé l'ancien `LOT-73` (agonie et mort). Les jets de
> sauvegarde contre la mort sont une **application** du système de conditions — inconscient,
> stabilisé — et les traiter à part faisait rouvrir par le second les fichiers du premier. Le numéro
> `LOT-73` est retiré.

Le moteur d'application : poser, empiler, expirer — en tours ou en temps de jeu — et l'effet sur les
jets : avantage, désavantage, incapacité d'agir.

*Contraintes du corpus (§4bis).* Les états de Tanares débordent l'appendice du SRD : marqueurs
**empilables** (quatre marques de poison), saignement, « couvert de lave », « mouillé »,
« chargé », malus par paliers, cécité **permanente**, mutisme, malédictions à tick quotidien ou
hebdomadaire levées seulement par *remove curse* — et l'**épuisement à niveaux**, que dix sorts
et capacités emploient comme coût. Une condition est donc une **source datée** avec une durée en
tours **ou** en temps de jeu (`LOT-70`), et la couche « affliction persistante » survit au combat.
Le seuil de jets contre la mort est **par personnage** (le Cultiste meurt au quatrième échec). Les
**maladies** du Sourcebook (la peste de Goldraft : DD 17, −1d6 PV maximum par aube ; les brumes de
Mistvale) sont des afflictions persistantes de cette couche, déclenchées par une zone du `LOT-81`.

*État au 14 septembre 2026.* Un combattant à 0 PV est `core::CombatantStatus::Down`, et ses tours
sont **passés** par `core::CombatState` ([LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md)) : ce lot lui rend un tour de jet
contre la mort, et y greffe la surprise — ni déplacement ni action au premier tour, pas de réaction
avant sa fin. Le [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md) lui laisse ce que le chapitre 9 du Manuel demande :
`core::CombatHook::CombatantDowned` à la chute et `DamageTaken` à chaque perte — y compris **à 0
PV**, où elle vaut un échec —, avec l'**excédent** au-delà de 0 (`CombatEvent::overflow`, la mort
instantanée quand il atteint le maximum) et le **critique** (`CombatEvent::critical`, deux échecs).
Une cible à terre ne peut pas encore être attaquée (`declareAttack` le refuse) : l'avantage contre
un inconscient et le critique au contact sont à ouvrir ici, avec l'état. Les points de vie
temporaires absorbent déjà à 0 PV sans relever (`core::CombatState::grantReserve`).

**Le piège.** Une condition n'est pas un booléen sur la fiche. Deux sources peuvent poser
« empoisonné » avec deux durées différentes ; retirer l'une ne doit pas retirer l'autre. C'est le
piège de la classe d'armure du [LOT-14](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-14-inventaire-et-equipement.md), transposé : on **recalcule depuis les
sources**, on n'accumule jamais.

**Et l'agonie.** Points de vie à 0, inconscience, jets de sauvegarde contre la mort (trois succès
ou trois échecs), stabilisation, critique à 0 PV, dégâts massifs. Et la mort **hors combat**,
aujourd'hui absente de tout document.

`EX-GP-030`/`031`/`032`, qui décrivaient une mort sans règles, ont été retirées par le
[LOT-67](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-67-menus-vocabulaire-rpg.md) avec la notion de niveau discret. Ce lot ne les rouvre pas : la mort, en
combat comme hors combat, se spécifie dans les familles `EX-REG` et `EX-CBT` qu'il couvre.

*Acceptation* — deux sources de la même condition, retrait de l'une, l'autre tient ; une condition
expire au bon tour ; chaque état du catalogue `LOT-35` a un effet observable, ou est explicitement
déclaré narratif ; trois échecs tuent, trois succès stabilisent, un soin au-dessus de 0 réinitialise
le compteur ; mourir en exploration a un effet défini, et ce n'est pas « redémarrer le niveau ».

### `LOT-74` — Expérience et progression {#lot-74}

*Prérequis : [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md), [LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md), [LOT-16](#lot-16).*

*Exigences couvertes : `EX-RPG-030`, `EX-RPG-031`, `EX-RPG-032`, `EX-REG-050` (le facteur de
puissance, échelle unique du dosage, des rangs et de l'expérience).*

**Les sources** d'expérience, qui manquent entièrement : victoire au combat selon le facteur de
puissance des adversaires, achèvement de quête, découverte de lieu — cette dernière propre au bac à
sable. Puis les seuils, la montée de niveau, et la répartition dans un groupe
([LOT-29](#lot-29)).

**Le piège.** Trois lots consomment l'expérience et aucun n'en produit : le `LOT-13` fait monter de
niveau, le `LOT-28` règle des seuils, le `LOT-29` la répartit, mais rien n'en attribue jamais. Les
**seuils** existent déjà en donnée (`Rpg/rules/experience.json`, [LOT-43](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-43-options-de-personnage.md)) ; la
progression **par tableau franchi** (`EX-LVL-014`) a été retirée par le [LOT-67](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-67-menus-vocabulaire-rpg.md). Il
ne manque donc que la **production** de l'expérience, et c'est tout ce lot.

*Acceptation* — tuer une créature de facteur de puissance connu donne l'expérience attendue ;
franchir un seuil monte d'un niveau, un dépassement multiple monte de plusieurs ; aucune progression
liée au franchissement d'un tableau ne subsiste.

### `LOT-75` — Campement et repos dans le monde {#lot-75}

*Prérequis : `LOT-70`, `LOT-42`, `LOT-41`.*

Où et quand on peut se reposer : le campement comme action en monde ouvert, le risque de rencontre
nocturne, l'auberge comme lieu sûr — que le [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) cite déjà comme nœud de graphe
sans lui donner de fonction.

**Le piège.** Un repos long disponible partout et sans risque annule toute gestion de ressources —
or c'est elle qui rend le repos intéressant. La contrainte doit venir des **données de région**
(`Crime and Violence`, `Monster Presence` du `LOT-41`), jamais d'une règle codée : c'est ainsi que
treize régions donnent dix rapports au repos sans qu'une ligne de C++ les distingue.

*Acceptation* — se reposer en zone dangereuse déclenche des rencontres à une fréquence dérivée de la
région ; une auberge garantit un repos non interrompu ; l'horloge avance du montant attendu.

### `LOT-80` — Factions, panthéon et organisations {#lot-80}

*Prérequis : `LOT-37`. Alimente `LOT-82`.*

Détaché du `LOT-37` à l'audit : les **5 factions** (avec leurs 15 entreprises et 23 secrets, qui
sont des gabarits de quête tout faits), **18 divinités** avec domaines, **13 organisations** à
fiche (une trentaine nommées, dont les 8 ordres de l'Église et les 7 rangs du Conseil draconique),
la chronologie (90 événements datés, 12 royaumes disparus) et la **légalité de la magie par
région** (réservée à la noblesse dans l'Empire, certifiée et payante à la Magocratie, débattue en
République — `Magic Access` seul ne l'exprime pas), vers `Source/Elements/World/factions/`,
`pantheon/` et `organizations/`. Le plan pénombral — sept couches, 32 lieux, ses règles de survie
— n'est plus « 7 lieux » ici : c'est le `LOT-90`.

Ce sont les entités **transverses aux régions**, et elles servent un autre consommateur que l'atlas :
une faction porte des objectifs qui se traduisent en quêtes, un panthéon porte des domaines qui se
traduisent en capacités. C'est ce qui rend les quêtes **par gabarit** du `LOT-82` écrivables plutôt
qu'improvisées. Le [LOT-16](#lot-16) n'en dépend pas : le slice (`0.0.1`) n'a qu'une quête,
écrite à la main — ce lot est de la `0.0.2`, et le lien qui le plaçait avant le slice a été retiré
le 14 septembre 2026.

*Acceptation* — toute faction, divinité ou organisation citée par une région du `LOT-37` existe dans
le catalogue ; chaque faction déclare au moins un objectif exploitable comme gabarit de quête ;
aucune entité orpheline, vérifié par `check_world_graph.py`.

### `LOT-81` — Descripteurs de terrain et règles de zone des treize régions {#lot-81}

*Prérequis : `LOT-37`. Prérequis de `LOT-40`.*

Détaché du `LOT-40` à l'audit, pour séparer la donnée du moteur. Chaque région déclare son
**descripteur de terrain** — proportions de biomes, présence d'eau, de relief, de bâti — dérivé de sa
section `Geography`.

La matière est déjà écrite : le Central Empire annonce « vallées fluviales, vastes prairies, forêt
du Bak, marais, hauts plateaux du nord » ; le Freelands « landes et zones humides, forêts tempérées,
chaînes glacées, deux mers ». Ce sont des recettes de terrain, pas de la prose d'ambiance. Le
travail est de les **typer** dans le vocabulaire de tuiles du [LOT-08](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-08-tuiles-rpg.md), et de les
relire.

*Contraintes du corpus (§4bis).* Une région porte aussi ses **règles de zone** : le Wasteland
interdit tout regain de points de vie en son centre et halve les soins en bordure ; le centre de
Darkall change au hasard le type de tout dégât non physique ; l'île d'Uncle Joe et les Temples de
Paix interdisent le combat ; les Night Mansions sanctionnent une attaque de 5d6 de froid. Ce sont
des **propriétés de zone** que la carte déclare (`EX-LVL-018`) et que le combat lit — le
[LOT-19](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-19-grille-tactique.md) en a posé le crochet (`core::BattleGrid::zonesAt`), qui ne lit lui-même que
`difficultTerrain`. **L'Undertanares** est une couche sous les régions, Darkall et le Wasteland
sont décrits sans encart : trois espaces jouables de plus que les treize, à descripteur propre.

*Acceptation* — les treize régions, l'Undertanares, Darkall et le Wasteland portent un descripteur
validé par schéma ; chaque biome cité correspond à un type de tuile existant ; deux régions
distinctes ne portent pas le même descripteur ; chaque règle de zone du livre est déclarée, et
un test la lit.

### `LOT-82` — Peuplement civil : PNJ, marchands et quêtes {#lot-82}

*Prérequis : `LOT-41`, `LOT-34`, `LOT-44`, `LOT-80`, [LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md), [LOT-26](#lot-26).
Alimente `LOT-45`, `LOT-42`.*

*Exigences couvertes : `EX-INV-031`.*

Détaché du `LOT-41` à l'audit : le versant **non hostile** du peuplement, dérivé des quatre autres
`Regional Statistics`.

| Statistique | Ce qu'elle pilote |
|---|---|
| `Magic Access` | Présence d'objets magiques chez les marchands, PNJ lanceurs de sorts |
| `Economic Prosperity` | Prix pratiqués, richesse des étals, valeur du butin |
| `Government Corruption` | Disponibilité des marchés noirs et des contrats douteux |
| `Citizen Freedom` | Ton des dialogues génériques, ce que les PNJ osent dire |

S'y ajoute la **répartition des espèces** de la région — le Freelands est à 70 % humain, 11 % elfe
d'automne, 8 % elfe d'hiver : les PNJ générés suivent cette distribution, avec les noms du `LOT-44`.

Les quêtes se composent sur le même principe : un objectif de faction venu du `LOT-80`, une cible
dans la région, une récompense tirée de sa prospérité. Le [LOT-16](#lot-16) fournit les drapeaux,
ce lot fournit les gabarits qui s'en servent.

*Acceptation* — deux régions aux statistiques opposées produisent des **prix moyens et des stocks
mesurablement différents**, vérifié par un test ; mille PNJ générés suivent la distribution
d'espèces déclarée par leur région ; aucun PNJ ni aucun marchand n'est placé en dur dans le code.

### `LOT-83` — Compagnie : rangs de Guilde, niveau d'équipe et quartier général {#lot-83}

*Prérequis : `LOT-45`, `LOT-74`, [LOT-29](#lot-29).*

Détaché du `LOT-45` à l'audit : le catalogue des rangs et des contrats est une donnée et un écran,
la **progression** est une mécanique — et elle suppose l'expérience du `LOT-74`, que le `LOT-45`
n'exigeait pas.

> **Élargi au second audit.** Le *Player's Guide* (ch. 5) porte un système d'**équipe** complet
> que la maquette 09 du [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) a déjà transcrite en `CompanyForm` : un **niveau
> d'équipe** de 1 à 20 gagné en *Career Points* (une échelle de contrats en sept paliers), un
> **quartier général** à douze structures de six niveaux chacune (hall, bibliothèque, écurie,
> atelier, taverne, infirmerie, observatoire, forteresse, ferme, musée, quai, jardin), douze
> employés statés, des effets économiques (remises, profits de taverne, rations, voyage raccourci),
> **douze dons d'équipe** et des récompenses légendaires. L'écran a ses onglets Équipe,
> Recrutement, Contrats et Réserve ; ce lot les remplit. La Guilde (rangs, `LOT-45`) et la
> compagnie (niveau, QG) sont **deux échelles**, et les deux restent : l'une borne ce qu'on a le
> droit de prendre, l'autre ce qu'on est capable de faire. Le [LOT-29](#lot-29) est prérequis
> : une équipe se joue à plusieurs.

Monter de rang : ce qui le déclenche (contrats accomplis, niveau atteint), ce que cela change dans
l'offre affichée, ce que cela ouvre — accès aux régions de rang supérieur, tarifs, réputation auprès
des factions du `LOT-80`. Et le garde-fou que la fiction impose déjà : « empêcher les aventuriers
inexpérimentés de prendre des tâches au-dessus de leurs moyens » — un joueur de niveau 2 doit être
**averti** avant d'entrer dans une région d'Argent, pas seulement puni.

*Acceptation* — accomplir les contrats d'un rang fait monter au suivant ; monter de rang change
l'offre de façon observable ; entrer dans une zone au-dessus de son rang produit un avertissement
diégétique, et non un écran de mort ; le niveau d'équipe monte par *Career Points*, une structure
de QG produit son effet, un don d'équipe s'observe en combat.

### `LOT-84` — Les 31 tables de progression de classe {#lot-84}

*Prérequis : `LOT-32`. Prérequis de `LOT-47`.*

*Exigences couvertes : `EX-RPG-020`.*

Détaché du `LOT-47` à l'audit : c'est de la **donnée**, extraite et relue, et elle appartient à la
filière contenu aux côtés des `LOT-36` et `LOT-43`. Rien ne justifiait qu'elle attende le socle de
classe ; l'inverse, si.

Les **31 tables de progression** sont extraites pour les seize classes et leurs sous-classes, même
celles dont la mécanique viendra bien plus tard. Le §4 le démontre sur la table du barbare :
illisible en `-layout`, où le niveau 5 reçoit « Amélioration de caractéristiques » au lieu
d'« Attaque supplémentaire », **exacte** en `-table`. C'est le lot où le §4 se paie une seconde fois.

La donnée peut précéder le code — c'est même souhaitable, cela rend le travail restant visible : les
quinze lots de classes à venir lisent une table qui existe déjà, au lieu de l'extraire chacun pour
soi.

Le `LOT-49` **contrôle** ces tables comme il contrôle les autres catalogues — il ne les précède
pas et n'en dépend pas : c'est un filet, pas un maillon.

*Acceptation* — les 31 tables sont extraites et validées par schéma ; trois d'entre elles sont
comparées **ligne à ligne** au PDF ; le contrôle statistique du `LOT-49` ne signale aucune anomalie
non acceptée.

### `LOT-89` — Dons, objets magiques et consommables de Tanares {#lot-89}

*Prérequis : `LOT-34`, `LOT-43`. Alimente [LOT-26](#lot-26).*

> **Ajouté au second audit.** Trois matières du corpus n'avaient aucun lot : les **20 dons** du
> *Player's Guide* (les 42 livrés au `LOT-43` sont ceux du SRD), les **29 objets magiques** du
> Sourcebook (27 au ch. 10, deux dans le bestiaire) et les **consommables à durée** du chapitre 8
> — boissons, mets rares à +1 de caractéristique pendant deux heures, Estelindea, peintures de
> guerre, Aemomium — et les **herbes à récolter** (trois des Monts de Cristal, avec leur DD de
> récolte), qui sont le premier artisanat du jeu. De la donnée, comme le `LOT-34`, avec les mêmes
> outils.

Les objets magiques imposent au schéma d'objet ce que le `LOT-34` n'avait pas à prévoir : rareté,
**harmonisation** avec restriction de classe (`EX-INV-041`), réserve de **charges** et règle de
recharge (à l'aube ; un bâton se réduit en cendres sur un 1), coût d'activation, **malédiction**
et harmonisation forcée (la Couronne de Pénombre éjecte un autre objet), jet de casse, paramètre
« type de dragon » ; onze objets ont un effet de combat, dix-huit relèvent d'enquête, de voyage ou
de messagerie et déclarent le mécanisme qu'ils exigent (`EX-CNT-030`). Les dons déclarent leurs
conditions d'accès et leurs effets en mécanismes (`EX-RPG-040`) — cinq dépendent d'une espèce.

*Acceptation* — 20 dons, 29 objets et les consommables du ch. 8 validés par schéma, avec `source`
et le mécanisme qu'ils exigent ; un objet à charges se recharge à l'heure déclarée par le
`LOT-70` ; un objet maudit ne se retire pas sans le mécanisme qui le permet ; le `LOT-49` ne
signale aucune anomalie non acceptée.

### `LOT-90` — Le plan pénombral {#lot-90}

*Prérequis : `LOT-42`, `LOT-41`, `LOT-46`.*

> **Ajouté au second audit.** Le plan pénombral n'est pas « 7 lieux » du `LOT-80` : c'est le
> **second monde** du Sourcebook — une surface et six couches, 32 lieux, un maître par couche, une
> table de cauchemars par couche — et le cœur de tout ce que Tanares raconte : les emoguns naissent
> des péchés, le madwalker s'y mute, les failles y mènent, *Tamera's Light* en chasse les créatures
> le jour. Un bac à sable dans l'univers complet ne peut pas l'ignorer ; un *vertical slice* ne
> doit pas l'attendre. D'où un lot à part, **après** le voyage et le peuplement, et différable sans
> rien casser.

Ce qu'il porte : les **règles de survie** du plan (pas de repos long hors des zones de stabilité,
jet de Sagesse toutes les huit heures ou niveau d'épuisement, lumière plafonnée, divination
faussée) comme règles de zone du `LOT-81` ; les **failles** — ouvertes par rituel ou sur un lieu
de massacre, bidirectionnelles, menant toujours au lieu **miroir** — comme routes du `LOT-42` ; les
lieux miroirs (marché, taudis, bois, cour) comme cartes du `LOT-40` ; les emoguns comme rencontres
du `LOT-41`, filtrés par couche ; le péage de sang, le Ghost Market mensuel, les trains toutes les
deux heures. La Malédiction malrokienne comme compteur global est **écartée** (§8) ; ce lot garde
la règle locale — un lieu de massacre ouvre une faille.

*Acceptation* — le personnage entre dans le plan par une faille et en revient au lieu miroir ;
un repos long y échoue hors zone de stabilité ; une créature pénombrale ramenée sur le plan
matériel brûle au soleil ; les sept couches sont jouables, chacune avec sa table.

---

> **Six numéros retirés.** Les `LOT-48`, `LOT-69`, `LOT-71`, `LOT-73` et `LOT-95` ont été absorbés par fusion, le
> `LOT-85` a été abandonné, et aucun **ne sera réattribué** : un identifiant de lot est stable, y compris quand il cesse de
> désigner du travail. Le tableau ci-dessous dit où leur contenu est parti.
>
> | Numéro retiré | Contenu | Absorbé par |
> |---|---|---|
> | `LOT-48` | Portraits et illustrations | Dissous dans chaque lot de catalogue |
> | `LOT-69` | Retrait de l'atelier pixel art | [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md), qui a retiré l'atelier avec le reste de l'habillage mort de l'éditeur |
> | `LOT-71` | Repos court et long | `LOT-70` |
> | `LOT-73` | Agonie et mort | `LOT-72` |
> | `LOT-85` | Outillage Qt Designer pour les `.ui` | Abandonné ; remplacé par le [LOT-86](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-86-refonte-hmi-quick.md) |
> | `LOT-95` | Le plan de la Capitale, en sous-zones, avec un zoom | [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) (le plan de la ville, peint par l'auteur, et l'écran de zoom) ; `capital.json`, les niveaux quartier et îlot et la position du héros au `LOT-96` |
>
> Le `LOT-31` (lexique bilingue) a lui aussi été retiré par fusion, dans le
> [LOT-30](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-30-chaine-extraction-corpus.md) — mais ce dernier étant livré, la plage de cette page commence désormais au
> `LOT-32`, et le numéro `LOT-31` n'y manque plus. Son sort est écrit dans l'epic du `LOT-30`.


---
## 6. Ordre d'exécution

**Les numéros ne sont pas un ordre.** Ils datent de l'écriture de cette page, où la filière
contenu a été numérotée après les lots de moteur. Les suivre tels quels construirait le graphe de
cartes du [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) avant l'atlas du `LOT-37` qui lui donne de vrais nœuds à relier,
et la fiche de personnage du [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md) avant les espèces qu'elle affiche. Mais les
ignorer ne vaut pas mieux : l'ordre retombe alors sur un arbitrage, et un arbitrage se refait à
chaque lot — sans jamais donner deux fois la même réponse.

Cette section donne donc **une suite unique**, et la règle qui la produit.

### La règle

> **À chaque pas, on prend, parmi les lots dont tous les prérequis sont faits, celui du jalon de
> version le plus proche ; à jalon égal, celui qui en débloque le plus.** À égalité, le plus petit
> numéro.

Le jalon prime, et c'est un ajout du 14 septembre 2026 : sans lui, le calcul plaçait les factions
du bac à sable (`0.0.2`) au rang 7, devant le contenu du slice (`0.0.1`), parce qu'elles
débloquaient davantage — un ordre exact pour le graphe, faux pour le programme. Le jalon est la
seule priorité que le graphe ne dit pas, et la seule que l'auteur fixe à la main, dans le tableau
en tête de page.

« Débloque » se compte : c'est le nombre de lots restants qui dépendent de celui-ci, directement
ou en cascade. Le `LOT-30` en débloque **quarante-neuf** sur soixante-neuf, le `LOT-10` vingt-cinq,
le `LOT-49` aucun. « Outillage et contrats ; le plus tôt est le mieux » cesse ainsi d'être un avis
éditorial : c'est ce que le graphe dit, et le chiffre est dans le tableau.

Le critère a d'abord été le **plus petit numéro**. Il donnait une suite déterministe, mais bête :
elle plaçait le `LOT-10` et le `LOT-12` devant le `LOT-30`, c'est-à-dire deux lots de moteur devant
la chaîne qui leur fournira leurs catalogues. Le numéro ne dit rien de l'utilité d'un lot ; il ne
sert plus qu'à départager, et il faut un départage — sans lui, deux lots de même portée sortiraient
dans un ordre qui changerait d'une exécution à l'autre.

Quatre propriétés en découlent, et ce sont elles qui justifient de préférer une règle à un choix :

- elle est **déterministe** — deux lecteurs retrouvent la même suite, et personne n'a à trancher ;
- elle est **calculée, jamais tenue à jour à la main**. `scripts/lint_lots.py` la recalcule depuis
  les lignes « Prérequis » des sections 5 et 11, et refuse le tableau d'avancement s'il en diverge
  (règle 13). Un tableau d'ordre écrit à la main est faux dès le premier lot livré, et il l'est en
  silence ;
- elle **n'invente aucune dépendance** : ce qu'aucune ligne ne déclare ne contraint rien. Un ordre
  qui semble mauvais se corrige donc en corrigeant un prérequis, pas en réécrivant un tableau ;
- elle **ne promet pas d'être optimale**. Elle ne raccourcit pas le programme et ne minimise aucune
  durée — rien ici ne mesure la durée d'un lot. Elle maximise, à chaque pas, le nombre de lots qui
  deviennent démarrables : elle repousse le plus tard possible le moment où il ne reste qu'un seul
  chemin.

Un lien se déclare **des deux côtés** : soit un lot cite ce qu'il attend, soit un lot amont
déclare ce qu'il alimente. Les deux comptent. C'est ce qui place le [LOT-27](#lot-27) au rang
20, et non parmi les tout premiers : il ne déclare **aucun** prérequis — son texte est repris tel
quel de son epic d'origine, qui ignorait la filière — alors que cinq lots de contenu déclarent
l'alimenter. Le *vertical slice* se jouerait sinon sur des catalogues vides.

### Où lire la suite

La suite calculée par cette règle est le **tableau d'avancement en tête de page**
([État d'avancement](#roadmap-avancement)) : c'est la première chose qu'on lit en ouvrant ce
document, et c'est là qu'elle sert. Elle n'est pas recopiée ici — deux tableaux décrivant le
même ordre divergeraient, et cette page a déjà payé ce prix une fois.

Ce qui reste dans cette section explique **pourquoi** la suite est celle-là : le regroupement
d'intention dont elle est issue, ce que le calcul a révélé, le chemin critique qu'il faut tenir
court, et le graphe dont tout est tiré.

### Le regroupement d'intention

Le tableau ci-dessous **ne donne pas l'ordre** — celui d'en-tête le donne. Il donne la *raison* de
chaque placement, et c'est la seule chose qu'un calcul ne saura jamais produire : un graphe dit
qu'un lot en attend un autre, il ne dit pas pourquoi on a voulu ce lien. Les deux tableaux ne se
contredisent donc pas, ils ne répondent pas à la même question — et chacun a son garde-fou : le
lint vérifie qu'aucun lot de la filière ne manque à celui-ci (règle 6), comme il vérifie que celui
d'en-tête est bien la suite calculée (règle 13).

Ces trente-cinq lots ne forment **pas une phase** qui suivrait le [LOT-29](#lot-29) : ils
s'entrelacent avec les phases B à E, parce que chacun sert un lot existant qui, sans lui, se
construirait sur un catalogue fictif — et un catalogue fictif finit toujours par se figer en
valeurs codées en dur, exactement ce que [`EX-VIS-007`](../../../Documentation/Specification/vision.md#EX-VIS-007) interdit. D'où
« filière » plutôt que « phase ».


| Quand | Lots | Pourquoi là |
|---|---|---|
| **Livré le 17 septembre 2026** | [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) | La première carte, le Colisée en version finale, dans le style que le [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) (livré) a fixé : l'exploration dans le jeu et le sable comme zone de combat déclarée — tout le chemin critique restant passe par lui ; les images du corpus sont parties et l'écran « Carte » est revenu sur les cartes de l'auteur ([LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md), livré) — l'éditeur du [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) est livré et trace déjà la première carte |
| Après [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) (livré) | [LOT-93](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-93-atelier-monstres.md) (livré) | L'atelier des monstres hérite du style |
| Après [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) et [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) (livré) | `LOT-96` | Les quartiers se tracent depuis le plan de la Capitale peint par l'auteur, sur un moteur éprouvé au Colisée ; `capital.json` et les niveaux quartier et îlot du plan y naissent, le `LOT-95` ayant été absorbé |
| Après `LOT-96` | [LOT-16](#lot-16) | La quête se vérifie en parlant à Myr sur la carte de Martpart, pas dans un test seul — c'est ce qui la rend vérifiable |
| Avec [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) | `LOT-80` | L'atlas du `LOT-37` est livré : le graphe de cartes a ses nœuds, il lui manque les entités transverses |
| Après `LOT-34` (livré) | `LOT-49` | Catalogue réel, puis contrôle de ses valeurs — un filet, pas un maillon |
| Avant [LOT-25](#lot-25) | `LOT-35` | Les sorts sont des données avant d'être un système ; l'écran des sorts existe déjà (`LOT-87`) |
| Avant [LOT-25](#lot-25) et les lots de classes | `LOT-70` | Un petit lot qui en débloque quinze — et qui rétro-adapte la fiche du `LOT-13`, livrée sans cadence de ressource |
| Après [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) (livré) | `LOT-81`, puis `LOT-40` | Les descripteurs, puis le générateur qui les consomme |
| Après [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md) (livré) | `LOT-72` | Conditions, agonie et mort appartiennent au combat ; le pipeline rapporte déjà l'excédent de dégâts et le critique |
| Après [LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md) (livré) | `LOT-74` | L'expérience se gagne à la fin d'un combat (`core::CombatHook::CombatEnded`) |
| Avant `LOT-47` | `LOT-84` | Les 31 tables sont de la donnée : elles précèdent le socle qui les lit |
| Après [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md) (livré) | `LOT-47` | Le socle de classe, éprouvé dans l'arène |
| Après `LOT-34` et `LOT-43` (livrés) | `LOT-89` | De la donnée : dons, objets magiques, consommables |
| Après [LOT-27](#lot-27) | `LOT-90` | Le second monde, différable sans rien casser |
| Avant [LOT-27](#lot-27), en filière | `LOT-44`, `LOT-46` | Noms et créatures de Tanares : de la donnée, sans prérequis de moteur, qui donne au slice de vrais habitants |
| Après [LOT-27](#lot-27) | `LOT-41`, `LOT-82`, `LOT-42`, `LOT-45`, `LOT-83`, `LOT-75` | Le bac à sable généralise une boucle ; elle se valide une fois avant d'être appliquée cent fois |

### Ce que l'ordre a révélé

Quatre points méritent qu'on s'y arrête.

**Ce que les deux premiers calculs avaient montré, en septembre 2026.** Le `LOT-09`, plus petit
numéro de la page, tombait au rang 33 parce que l'atlas du `LOT-37` déclarait l'alimenter — un
graphe de cartes sans régions relierait des nœuds inventés. Et le `LOT-30` sortait premier, pour
une raison chiffrée : il débloquait quarante-neuf des soixante-neuf lots restants, une fois les
trois préconditions livrées ([LOT-77](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-77-specification-rpg.md) : les cinq familles `EX-*` ; [LOT-78](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-78-numeros-herites.md) : 201 renvois de spécification désambiguïsés ; [LOT-79](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-79-socle-chargement-donnees.md) : six lecteurs JSON factorisés).
Les deux sont livrés ; la trace reste ici parce qu'elle dit pourquoi les numéros ne sont pas un
ordre.

**Le `LOT-27` passait pour prêt, et il ne l'était pas.** Sa ligne « Prérequis : phases B, C et D
complètes » ne nommait aucun lot. Le lint n'y lisait rien, et le calcul plaçait le *vertical slice*
au rang 5, **prêt**, alors qu'il attend encore le combat entier, les dialogues, les quêtes, la
sauvegarde, l'éditeur et le graphe de cartes. L'audit du 14 septembre 2026 a écrit ces onze lots en
numéros ; le slice retombe à sa place réelle — voir le tableau d'avancement — et le **chemin
critique** ci-dessous en est la conséquence directe. Une dépendance que seul un humain sait lire
n'existe pas pour la règle qui calcule l'ordre.

**Les lots du bac à sable restent résolument après le [LOT-27](#lot-27).** Peupler treize régions
revient à appliquer cent fois la même recette ; si la recette est mauvaise — rencontres mal dosées,
marchands inutiles, quêtes vides — on la découvre cent fois. Le `LOT-27` coûte deux quartiers et
un bas-fond de la Capitale, et c'est le prix pour ne pas payer cette erreur au centuple. Leur ordre interne compte
aussi, et la règle le produit sans qu'on ait à l'imposer : `LOT-44` (les noms) avant `LOT-41` (le
peuplement), sinon on peuple avec des « Villageois 1 » ; `LOT-45` (la Guilde) après le `LOT-82`,
parce qu'elle a besoin de quêtes à afficher.

**Le volume d'images n'est plus un lot.** 6 000 images ne se traitent pas d'un bloc, et rien n'y
oblige — le [LOT-39](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-39-cles-assets.md) affiche un marqueur pour tout ce qui n'a pas encore d'image, et
chaque illustration livrée en remplace un. Ce travail était le `LOT-48`, « volume long, sans
jalon » ; il est désormais **réparti dans chaque lot de catalogue**, qui livre ses propres
illustrations. Un lot sans date de fin est un lot qu'on ne finit pas — et en garder un pendant
qu'on écrit cette phrase était le plus visible des angles morts de cette page. Le
[LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) a appliqué la même règle à l'interface : ses **214 images** de cadres, plaques
et fonds sont décrites dans un cahier des assets, aucune n'est produite, et chaque brique dessine un
aplat de repli en attendant — le jeu tourne complet sans elles.

### Le chemin critique jusqu'au *vertical slice*

C'est la contrainte que cette feuille de route a le plus de mal à tenir, parce qu'elle se dégrade
sans que personne ne décide rien : chaque lot qu'on déclare « avant le [LOT-27](#lot-27) »
repousse d'autant le seul jalon qui prouve que le jeu **est un jeu**.

Le premier audit avait trouvé le `LOT-69` placé « avec `LOT-50`, avant le `LOT-27` » — soit les
seize classes, le Colisée et le mode édition **devant** le slice — et l'avait réduit à une
suppression. Puis le chemin passait par les catalogues : `LOT-30` → `LOT-32` → `LOT-43` →
`LOT-36` → [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md) → [LOT-38](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-38-fiche-de-personnage.md) → `LOT-39` → [LOT-27](#lot-27), huit
lots, dont la fiche de personnage et sa maquette. **Ces sept-là sont livrés.**

Le chemin critique du **combat** est parcouru : de la bascule du [LOT-18](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-18-bascule-exploration-combat.md) à l'IHM de
combat du [LOT-24](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-24-ihm-combat.md), livrée le 15 septembre 2026, le combat tactique se joue au Colisée,
contre l'IA, au clavier et à la manette. Le chemin restant est celui de la **partie**, et depuis le
16 septembre 2026 c'est **une seule chaîne** :

[LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) (livré) → [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) → `LOT-96` → [LOT-16](#lot-16) → [LOT-17](#lot-17) →
[LOT-27](#lot-27)

avec, en parallèle, l'éditeur du [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) (livré, qui trace les cartes),
[LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) (livré) → `LOT-96` (le plan de la Capitale peint par l'auteur, source du
tracé) et `LOT-93` → `LOT-27` (les monstres) ; les images du corpus sont parties avant la version
([LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) → [LOT-28](#lot-28)). La chaîne du
matin du 16 septembre était `LOT-09 → LOT-16 → LOT-17 → LOT-27` ; la relecture du soir, par
l'auteur, a scindé le `LOT-09` et mis le style avant la carte — cinq lots de plus, ramenés à quatre
le 17 quand le `LOT-94` a absorbé le `LOT-95`, pour un grain
où chaque lot livre une chose qu'on voit. La chaîne
était un éventail : `LOT-16` et `LOT-09` démarrables ensemble, `LOT-17` les attendant tous deux,
`LOT-27` ramassant tout. C'était exact pour le graphe et faux pour le travail : ces quatre lots
avaient été rédigés avant que le jeu ait une carte, ils ne nommaient **aucun lieu, aucun PNJ,
aucune quête**, et chacun ne se vérifiait qu'en test — un graphe de cartes sans carte à ouvrir,
« une quête à trois étapes » sans dire laquelle, une sauvegarde sans rien à sauvegarder. Un lot
qu'on ne peut pas voir dans le jeu n'est jamais tout à fait fini, et le contenu qui devait les
relier était renvoyé au dernier lot, celui qu'on n'atteint pas.

La relecture du 16 septembre 2026 les **rattache à un lieu** : la Capitale du Central Empire, où se
trouve le Colisée du [LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md) — le seul contenu jouable du jeu — et que le Sourcebook
décrit plus longuement qu'aucun autre lieu (§8). Chaque lot livre désormais une pièce **visible**
de cette ville, et le suivant s'appuie dessus. Le `LOT-09` porte l'exploration dans le jeu
Qt Quick, que personne ne portait, et l'ouvre au **Colisée**, première carte en version finale ;
le `LOT-96` pose la ville sur ce moteur ; le `LOT-16` l'attend, parce que sa quête se joue sur ses
cartes. Le `LOT-27` hérite du `LOT-24` une tâche que personne d'autre ne porte : **poser le combat
sur la carte d'exploration**. Tout le reste — classes, peuplement, voyage, guilde — vient
**après**. La règle à tenir tient en une phrase : *un lot n'entre dans ce chemin que si le slice ne
peut pas se jouer sans lui.*

### Le plan d'intégration : la Capitale, lot après lot {#roadmap-capitale}

Ce tableau dit ce qu'on **voit dans le jeu** à la fin de chaque lot du chemin, et comment on le
vérifie — un geste de joueur et un test. C'est le contrat de « vérifiable » que les quatre lots
réécrits doivent tenir ; le détail de chacun est en section 11.

| Lot | Ce qu'on voit dans le jeu, à la fin | Comment on le vérifie |
|---|---|---|
| [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) (livré) | L'éditeur pose PNJ, portails et points d'arrivée sur les trois couches, et avertit d'une zone impraticable au combat | essai immédiat depuis l'éditeur ; le Colisée final s'y trace |
| [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) (livré) | L'arène du Colisée dessinée avec les textures de l'atelier ; une **maquette de style** approuvée, les planches du Colisée et de Martpart découpées, un atelier qui se commande depuis une fiche d'atlas | la maquette signée par l'auteur ; `check_assets_brief.py` ; `extract_texture_sheet.py --check` ; Martpart commandé sans toucher au style |
| [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) (livré) | Le menu principal sur un **fond produit** ; plus une image du corpus dans le dépôt ; l'écran « **Carte** » à trois niveaux — monde, région, ville — sur les seize cartes peintes par l'auteur | lints de provenance et des cartes en CI (`check_ui_assets.py`, `check_map_assets.py`) ; une capture de référence par niveau, et celles du menu et des crédits |
| [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) | « Nouvelle partie » ouvre le **Colisée** à sa porte ; on parcourt hall, couloirs, vestiaires et tribunes ; le héraut lance le combat sur le sable et l'on revient sur la carte ; plus aucun contenu provisoire | parcours headless sur cinq fixtures ; capture de référence ; le geste au clavier et à la manette ; `Source/Elements/` sans fichier de test |
| [LOT-93](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-93-atelier-monstres.md) (livré) | Le lion, le loup et le soldat Ironhand dessinés et animés dans la galerie des assets | deux gabarits (Moyen, Grand) produits de bout en bout ; un humanoïde par la même chaîne ; manifeste et clés |
| [LOT-96](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-96-quartiers-capitale.md) (livré) | « Nouvelle partie » ouvre **Martpart** ; on passe à **Arenarea** et on revient au bon point ; dix portes gardées ; le plan montre où l'on est, et descend au quartier puis à l'îlot | aller-retour headless ; captures de Martpart et d'Arenarea ; le geste au clavier et à la manette |
| [LOT-16](#lot-16) | **Myr** confie « Les enfants de Martpart » ; le journal la suit ; la porte d'Arenarea s'ouvre au laissez-passer | la quête jouée par drapeaux en headless ; à l'écran, du marché au journal |
| [LOT-17](#lot-17) | « Continuer » et « Charger » reprennent la partie où on l'a laissée — quartier, case, journal | aller-retour à l'identique à chaque étape de la quête ; quitter et reprendre |
| [LOT-27](#lot-27) | La ville **habillée** — textures, figurines de Myr et Galender, champions du bestiaire, le repaire —, le combat sur la carte, le verdict de l'Arène du Destin | test système de la porte de Martpart au verdict ; `check_world_graph.py` ; aucun marqueur sur le chemin de la quête |
| [LOT-28](#lot-28) | Musique, bruitages, secousse sur critique ; aucune image du corpus ; la version `0.0.1` | cahier de test régénéré ; lint de provenance ; tag |

Un chantier court **à côté** de la chaîne, sans l'attendre : les **figurines** de la série
« Capitale » à l'atelier du `LOT-91` (Myr et Galender au *Character Compendium*, huit rôles
rédigés depuis la prose). Les **planches de textures**, qui étaient l'autre chantier, sont
devenues un lot, le [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md), parce que le style qu'elles fixent précède la première carte ; leur
production continue ensuite au fil de l'eau, lieu par lieu, et les monstres suivent la même
règle (`LOT-93`). Lancer les figurines avec le [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) fait que le `LOT-27` habille au lieu de
dessiner.

**Ce chemin portait une décision, pas seulement du code, et elle est prise.** Le contenu du slice
« se produit dans l'éditeur », et le [LOT-86](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-86-refonte-hmi-quick.md) avait fait de l'éditeur un second
exécutable en Qt Widgets, là où le §10 voulait l'édition **dans la scène** depuis le jeu. L'auteur a
tranché le 16 septembre 2026 : retarger `LevelEditor`, ce que le [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) a livré.

### Le graphe des dépendances

Les liens ci-dessous sont ceux que déclarent les lignes « Prérequis » de la section 5. Ils sont
**vérifiés en CI** par `scripts/lint_lots.py`, qui refuse un cycle, un lien déclaré d'un seul côté,
un lot absent du tableau d'ordre ci-dessus ou une section de lot sans ancre Doxygen.

Le graphe ci-dessous est donné en **source Graphviz**, et non en diagramme rendu : la chaîne
Doxygen du projet tourne sans `HAVE_DOT`, et ajouter Graphviz au runner pour une seule
illustration coûterait plus que de coller ces vingt lignes dans un visualiseur.

```dot
digraph filiere {
  rankdir=LR;
  node [shape=box, style=rounded, fontsize=10];
  subgraph cluster_pre { label="Préconditions"; style=dashed;
    L77 [label="LOT-77\nSpécification RPG\n(livré)", style="rounded,filled", fillcolor=grey90];
    L78 [label="LOT-78\nNuméros hérités\n(livré)", style="rounded,filled", fillcolor=grey90];
    L79 [label="LOT-79\nChargement JSON\n(livré)", style="rounded,filled", fillcolor=grey90];
  }
  subgraph cluster_slice { label="Chemin critique du vertical slice"; style=dashed;
    L30 [label="LOT-30\nExtraction + lexique\n(livré)", style="rounded,filled", fillcolor=grey90];
    L32 [label="LOT-32\nSchémas\n(livré)", style="rounded,filled", fillcolor=grey90];
    L33 [label="LOT-33\nBestiaire\n(livré)", style="rounded,filled", fillcolor=grey90];
    L34 [label="LOT-34\nÉquipement\n(livré)", style="rounded,filled", fillcolor=grey90];
    L43 [label="LOT-43\nOptions perso\n(livré)", style="rounded,filled", fillcolor=grey90];
    L36 [label="LOT-36\nEspèces\n(livré)", style="rounded,filled", fillcolor=grey90];
    L37 [label="LOT-37\nAtlas régions\n(livré)", style="rounded,filled", fillcolor=grey90];
    L27 [label="LOT-27\nLa Capitale", shape=box, style="rounded,bold"];
  }
  L84 [label="LOT-84\n31 tables"]; L47 [label="LOT-47\nSocle de classe"];
  L50 [label="LOT-50\nColisée\n(livré)", style="rounded,filled", fillcolor=grey90]; L51 [label="LOT-51→65\nune classe par lot"];
  L32 -> L84 -> L47 -> L51; L50 -> L51;
  L43 -> L47; L70 [label="LOT-70\nCalendrier et repos"] -> L51;
  L80 [label="LOT-80\nFactions"]; L81 [label="LOT-81\nDescripteurs"];
  L40 [label="LOT-40\nGénérateur"]; L41 [label="LOT-41\nRencontres"];
  L82 [label="LOT-82\nPeuplement civil"]; L45 [label="LOT-45\nGuilde"];
  L83 [label="LOT-83\nProgression guilde"]; L42 [label="LOT-42\nVoyage"];
  L37 -> L80; L37 -> L81 -> L40 -> L41 -> L82 -> L45 -> L83;
  L80 -> L82; L82 -> L42;
  L27 -> L41 [style=dotted, label="valide la recette"];
  subgraph cluster_combat { label="Chemin critique restant : le combat"; style=dashed;
    L18 [label="LOT-18\nBascule\n(livré)", style="rounded,filled", fillcolor=grey90];
    L19 [label="LOT-19\nGrille tactique\n(livré)", style="rounded,filled", fillcolor=grey90];
    L20 [label="LOT-20\nInitiative\n(livré)", style="rounded,filled", fillcolor=grey90];
    L21 [label="LOT-21\nAttaques\n(livré)", style="rounded,filled", fillcolor=grey90]; L22 [label="LOT-22\nLigne de vue\n(livré)", style="rounded,filled", fillcolor=grey90];
    L23 [label="LOT-23\nIA tactique\n(livré)", style="rounded,filled", fillcolor=grey90]; L24 [label="LOT-24\nIHM de combat\n(livré)", style="rounded,filled", fillcolor=grey90];
    L24 -> L27;
  }
  L15 [label="LOT-15\nDialogues\n(livré)", style="rounded,filled", fillcolor=grey90]; L16 [label="LOT-16\nQuêtes"]; L17 [label="LOT-17\nSauvegarde"];
  L09 [label="LOT-09\nLe Colisée\npremière carte\n(livré)", style="rounded,filled", fillcolor=grey90]; L11 [label="LOT-11\nÉditeur\n(livré)", style="rounded,filled", fillcolor=grey90];
  L92 [label="LOT-92\nAtelier textures\n(livré)", style="rounded,filled", fillcolor=grey90]; L93 [label="LOT-93\nAtelier monstres\n(livré)", style="rounded,filled", fillcolor=grey90];
  L94 [label="LOT-94\nCartes de l'auteur\n(livré)", style="rounded,filled", fillcolor=grey90];
  L96 [label="LOT-96\nMartpart, Arenarea\n(livré)", style="rounded,filled", fillcolor=grey90]; L28 [label="LOT-28\nv0.0.1"];
  L15 -> L16 -> L17 -> L27; L09 -> L17; L11 -> L27;
  L96 -> L16; L96 -> L17; L96 -> L27;
  L93 -> L27; L94 -> L28; L27 -> L28; L94 -> L42;
}
```

### Récapitulatif : qui dépend de qui

Ce tableau est **généré depuis les lignes « Prérequis » ci-dessus** et vérifié en CI par
`scripts/lint_lots.py` : s'il diverge du texte, c'est le lint qui échoue, pas le lecteur qui s'en
aperçoit trois lots plus tard. La colonne « Alimente » est l'inverse calculé de la colonne
« Prérequis », augmentée des dépendances que les lots livrés et absorbés ne peuvent pas déclarer
eux-mêmes.

| Lot | Objet | Prérequis | Alimente |
|---|---|---|---|
| `LOT-35` | Sorts et états | `LOT-32` | `LOT-25`, `LOT-72` |
| `LOT-40` | Générateur de terrain | `LOT-11`, `LOT-81` | `LOT-41` |
| `LOT-41` | Peuplement : rencontres et créatures | `LOT-27`, `LOT-33`, `LOT-40`, `LOT-44`, `LOT-46` | `LOT-75`, `LOT-82`, `LOT-90` |
| `LOT-42` | Voyage et carte du monde | `LOT-09`, `LOT-17`, `LOT-68`, `LOT-70`, `LOT-82`, `LOT-94` | `LOT-75`, `LOT-90` |
| `LOT-44` | Noms, tables aléatoires et contenu d'ambiance | `LOT-36`, `LOT-37` | `LOT-41`, `LOT-82` |
| `LOT-45` | Guilde des Aventuriers : rangs et contrats | `LOT-16`, `LOT-26`, `LOT-68`, `LOT-82` | `LOT-83` |
| `LOT-46` | Créatures de Tanares | `LOT-30`, `LOT-33`, `LOT-93` | `LOT-41`, `LOT-90` |
| `LOT-47` | Socle de classe, et le guerrier comme preuve | `LOT-36`, `LOT-43`, `LOT-84` | `LOT-51` |
| `LOT-49` | Contrôle de cohérence du contenu | `LOT-33`, `LOT-34` | — |
| `LOT-51` | une classe par lot | `LOT-47`, `LOT-50`, `LOT-70` | — |
| `LOT-70` | Horloge, calendrier et lune | `LOT-13` | `LOT-25`, `LOT-42`, `LOT-51`, `LOT-65`, `LOT-75` |
| `LOT-72` | Conditions, agonie et mort | `LOT-12`, `LOT-21`, `LOT-35` | — |
| `LOT-74` | Expérience et progression | `LOT-13`, `LOT-16`, `LOT-20` | `LOT-83` |
| `LOT-75` | Campement et repos dans le monde | `LOT-41`, `LOT-42`, `LOT-70` | — |
| `LOT-80` | Factions, panthéon et organisations | `LOT-37` | `LOT-82` |
| `LOT-81` | Descripteurs de terrain et règles de zone des treize régions | `LOT-37` | `LOT-40` |
| `LOT-82` | Peuplement civil : PNJ, marchands et quêtes | `LOT-15`, `LOT-26`, `LOT-34`, `LOT-41`, `LOT-44`, `LOT-80` | `LOT-42`, `LOT-45` |
| `LOT-83` | Compagnie : rangs de Guilde, niveau d'équipe et quartier général | `LOT-29`, `LOT-45`, `LOT-74` | — |
| `LOT-84` | Les 31 tables de progression de classe | `LOT-32` | `LOT-47` |
| `LOT-89` | Dons, objets magiques et consommables de Tanares | `LOT-34`, `LOT-43` | `LOT-26` |
| `LOT-90` | Le plan pénombral | `LOT-41`, `LOT-42`, `LOT-46` | — |


---

## 7. Arborescence cible

```
Documentation/SourceBook/   ← les PDF et les ressources VTT, exclus du dépôt (taille)

scripts/sourcebook/
  corpus.toml               ← manifeste : empreintes, pagination, provenance (LOT-30, livré)
  *.py                      ← chaîne d'extraction, sur PyMuPDF (LOT-30, livré)
scripts/check_rpg_data.py, check_glossary.py, check_asset_keys.py   ← livrés
scripts/lint_lots.py        ← contrôle du graphe de lots de cette page (LOT-78, livré)
scripts/check_assets_brief.py, receive_ui_assets.py   ← cahier et réception des images (LOT-87)

Source/Elements/Rpg/
  schema/*.schema.json      ← contrats, 23 schémas (LOT-32, livré)
  creatures/*.json          ← 94 bêtes (LOT-33, livré), puis 82 de Tanares (LOT-46)
  items/, weapons/, armors/ ← équipement (LOT-34, livré)
  spells/, conditions/      ← sorts et états (LOT-35) — leurs schémas existent déjà
  species/, backgrounds/    ← espèces et historiques (LOT-36, livré)
  classes/*.json            ← 4 provisoires (LOT-36, livré), 31 tables (LOT-84), socle (LOT-47),
                              puis une classe par lot (LOT-51 à LOT-65)
  feats/, skills/, languages/, rules/   ← options de personnage, seuils d'expérience (LOT-43, livré)
  characters/, encounters/  ← personnage et rencontre de démonstration (LOT-13, LOT-18, livrés ;
                              provisoires, retirés par le contenu du LOT-27)
  tables/*.json             ← noms, personnalité, babioles (LOT-44)
  guild/*.json              ← rangs et gabarits de contrat (LOT-45), compagnie et QG (LOT-83)
  magic-items/, consumables/, feats/ (20 de Tanares)   ← (LOT-89)

Source/Elements/World/
  regions/*.json            ← 13 régions : statistiques, espèces, factions (LOT-37, livré)
                              + descripteurs de terrain (LOT-81)
  locations/*.json          ← 107 lieux nommés, avec leurs effets mécaniques (LOT-37, livré)
  factions/, pantheon/, organizations/            ← (LOT-80)
  arena/*.json              ← Colisée : les arènes, une par variante régionale (LOT-50, livré) ;
                              les huit Marques Héroïques sont une règle, Rpg/rules/heroic-marks.json
  populate/*.json           ← gabarits de rencontre (LOT-41), civils et quêtes (LOT-82)
  penumbra/*.json           ← sept couches, lieux miroirs, failles (LOT-90)
Source/Elements/Maps/world-maps.json   ← ancres des régions, lieux de l'atlas et quartiers placés sur
                              les cartes, à part de l'atlas (LOT-94, livré)
Source/Elements/Assets/
  Entities/families.json, UI/illustrations.json   ← clés d'assets et marqueurs (LOT-39, livré)
  UI/background/            ← le fond du menu principal, produit (LOT-94, livré) ; plus aucune image du corpus
  Npc/<slug>/, Monsters/<slug>/   ← figurines de l'atelier des PNJ (LOT-91) et des monstres (LOT-93)
  Textures/<lieu>/          ← planches découpées de l'atelier des textures (LOT-92), le Colisée d'abord
  Maps/                     ← les seize cartes peintes par l'auteur — monde, treize régions, deux villes — et
                              leur manifest.json (LOT-94, livré) ; niveaux quartier et îlot de la Capitale au LOT-96
  UI/                       ← les 214 images 9-patch du cahier des assets (LOT-87 : à produire)
Source/Elements/Localization/rpg.glossary.csv   ← lexique (LOT-30, livré)

Source/Ui/                  ← les écrans du jeu en QML : formulaires, contrôles, jetons (LOT-86, LOT-87)
Source/HMI/Runtime/         ← les vues-modèles C++ exposées au QML ; `PendingData` y tient lieu
                              de toute donnée que son lot n'a pas encore livrée
Source/Core/Data/
  JsonDocument.{h,cpp}      ← brique de lecture unique, position ligne/colonne (LOT-79, livré)
Source/Core/Combat/
  Encounter, CombatTransition        ← la bascule (LOT-18, livré)
  BattleGrid, Pathfinding            ← la grille et le déplacement (LOT-19, livré)
  TurnOrder, CombatState, Attack, Damage, LineOfSight, Flanking, EnemyAi
                            ← le combat lui-même (LOT-20 à LOT-23, livrés ; les poids de l'IA
                              dans Rpg/rules/behaviors.json)
Source/Core/World/
  Atlas.{h,cpp}             ← régions et lieux (LOT-37, livré)
  WorldGraph.{h,cpp}        ← cartes, portails, points d'arrivée nommés (LOT-09)
  TerrainGenerator.{h,cpp}  ← génération pilotée par descripteur de région (LOT-40)
  RegionPopulator.{h,cpp}   ← peuplement systémique (LOT-41, LOT-82)
  WorldMap.{h,cpp}          ← voyage et découverte (LOT-42)
Source/Core/Time/
  GameClock.{h,cpp}, Calendar, Rest  ← date, phase de lune, repos court et long (LOT-70)

Documentation/Specification/
  contenu.md, regles-d20.md, combat.md, rpg.md, inventaire.md   ← (LOT-77, livré)
Documentation/Lot/LOT-87-charte-v2/
  assets-brief.{md,json}, references/   ← le cahier des 214 images et les dix maquettes (LOT-87)
```

---

## 8. Décisions prises et questions restantes

### Tranché

- **Périmètre : l'univers complet.** Les 13 régions et leurs lieux, les 13 espèces, les 8 classes et
  leurs sous-classes, les historiques, les factions, le panthéon, le bestiaire entier. Pas de socle
  réduit.
- **Les 13 régions sont jouables**, pas seulement importées — d'où la génération de terrain du
  `LOT-40`, qui seule rend une centaine de cartes atteignable.
- **Le jeu est un bac à sable** dans un premier temps : contenu systémique déduit des données de
  région, pas d'intrigue directrice.
- **Licences en sommeil** (§3) : projet privé, dépôt privé, aucune contrainte d'usage.

- **Bestiaire arrêté à 176 créatures** : les 94 bêtes du SRD (`LOT-33`) et les 82 de Tanares
  (`LOT-46`). Le *Manuel des Monstres* est **hors périmètre** — 416 blocs dont chaque valeur
  numérique demanderait un contrôle, pour un bestiaire déjà largement suffisant à un monde ouvert
  de treize régions. Rien n'est fermé : le `LOT-30` sait le lire, il pourra être repris plus tard.
- **Le *Manuel des Joueurs* est dans le périmètre**, lui, et il est indispensable : il porte les
  5 races et 8 classes que les *Basic Rules* n'ont pas, donc les classes de base sur lesquelles se
  greffent les sous-classes de Tanares. Son OCR se traite (§4).
- **Les 4 classes simplifiées de Tanares sont un échafaudage.** Elles servent de socle au premier
  modèle de combat (`LOT-36`) et **seront retirées** au profit des seize classes complètes livrées
  par le `LOT-47` puis du `LOT-51` au `LOT-65`. Elles portent pour cela un `"statut": "provisoire"` en donnée, et le
  **dernier lot de classe livré** ne se clôt que sur leur suppression effective.
- **Une classe par lot, pas un lot pour toutes.** Une classe apporte une mécanique — rage, forme
  sauvage, magie de pacte — qui se code, se teste et se règle séparément. Le `LOT-47` ne porte donc
  que le socle commun et le guerrier ; les quinze autres classes ont chacune leur lot.
- **Le Colisée est le bac à sable de combat** ([LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md), livré), et il n'est pas un outil jetable : les
  Arènes sont une institution centrale de Tanares, leurs combats sont non létaux par la fiction
  même, et la zone sert telle quelle dans le jeu final.
- **La charte IHM a été refondue deux fois, et la seconde remplace la première.** Le
  [LOT-66](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-66-charte-visuelle.md) a retiré l'identité **pixel art** d'`EX-IHM-070` au profit du parchemin de Tanares relevé sur les feuilles de personnage ; les
  [LOT-76](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-76-habillage-interface.md), [LOT-67](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-67-menus-vocabulaire-rpg.md) et [LOT-68](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-68-chassis-ecrans-rpg.md) ont habillé, renommé et
  dessiné les écrans dessus — un monde ouvert n'a ni « niveau à choisir » ni « tableau à
  recommencer ». Puis le [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) (décision du 11 septembre 2026) a adopté les dix
  maquettes du pack UI comme **charte v2** : panneaux sombres cerclés d'or et parchemin, `Cinzel`
  en titres, `IM Fell English` en corps, ornements livrés en **images 9-patch produites à 1080p**
  depuis un cahier des assets, jamais découpées des maquettes. Les treize écrans du jeu sont
  transcrits ; les 214 images du cahier restent **à produire**, et chaque brique dessine un aplat
  de repli d'ici là. Le parchemin du `LOT-66` survit dans les rôles de couleur qu'il a mesurés.
- **L'IHM du jeu est en Qt Quick, l'éditeur en Qt Widgets, dans deux exécutables**
  ([LOT-86](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-86-refonte-hmi-quick.md)). Le jeu ne lie pas `Qt6::Widgets` (`EX-IHM-102`) ; un artiste modifie
  les écrans dans Qt Design Studio et les voit sans compilateur (`EX-IHM-100`). Chaque écran du RPG
  existe en QML, sur `PendingData` là où son lot n'a pas encore livré la donnée : les lots de
  moteur qui restent **branchent** des écrans, ils n'en dessinent plus.
- **L'arène est un mode du jeu principal** ([LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md), livré le jour même) — clarification de l'auteur, 14 septembre
  2026. C'est ce que l'ancienne appellation « mode édition » désignait : un bac à sable de
  **débogage** où l'on pose librement combattants, objets et décors, joue et rejoue à graine fixée,
  réutilisé tel quel dans le jeu final comme Colisée. Il arrive dès la grille et le tour
  ([LOT-19](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-19-grille-tactique.md), [LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md)), pas après les classes. **L'éditeur d'auteur
  reste `LevelEditor`**, l'exécutable Qt Widgets du [LOT-86](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-86-refonte-hmi-quick.md) — décision de l'auteur,
  16 septembre 2026 : le [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) l'a retargé aux trois couches et aux entités, et a
  refondu `EX-EDIT-030` (« intégré à l'application ») en distinguant l'outil d'auteur, séparé, du
  mode intégré, l'arène.
- **L'atelier pixel art est retiré**, faute d'objet une fois l'identité pixel art abandonnée —
  au [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md), qui a absorbé le `LOT-69`.
- **Les livres sont les entrants, et le moteur leur offre des crochets** (§4bis). Le socle du
  combat (`LOT-20`, `LOT-21`) expose des points d'insertion nommés et un pipeline de dégâts à
  étapes ; le socle de classe (`LOT-47`) livre six modèles de ressource et la substitution de
  profil ; l'horloge (`LOT-70`) est un calendrier lunaire ; l'altitude est un attribut, jamais
  une géométrie (`LOT-19`). Ce qui est écarté l'est nommément, dans la liste ci-dessous.
- **La `0.1.0` est le contenu final, les jalons sont des `0.0.x`** : `0.0.1` le slice, `0.0.2` les
  treize régions, `0.0.3` les seize classes, `0.0.4` le plan pénombral, `0.1.0` la somme (tableau
  en tête de page). La page s'appelle désormais « Feuille de route » ; le `LOT-28` tague `v0.0.1`
  et ramène le `CMakeLists.txt` du `0.1.0` hérité à ce numéro.
- **Treize régions**, pas dix : le livre en porte treize à encart, et trois espaces de plus
  (Darkall, Undertanares, Wasteland) que le `LOT-81` décrit sans encart.
- **Deux échelles de progression de groupe coexistent** : les rangs de la Guilde (`LOT-45`) et le
  niveau d'équipe avec quartier général (`LOT-83`), que la maquette 09 montre déjà.
- **Les images du corpus sont des références, jamais des assets** — décision de l'auteur, 16
  septembre 2026. Le §3 laisse les licences en sommeil pour la donnée et le vocabulaire ; les
  **images** — plan de la Capitale, carte du monde, planches — sont des œuvres que le jeu
  n'affiche pas et que ses cartes ne recopient pas. Les deux extractions commises sont parties
  ([LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md)), le fond du menu principal est produit. Le corpus reste ce qu'il est :
  lu sur le poste, hors dépôt (`EX-CNT-023`).
- **On ne se déplace jamais sur une carte du monde** — même décision. Le déplacement se fait sur
  des **cartes de niveau** ; l'écran « Carte » sert à s'orienter et à choisir, pas à marcher
  (`EX-IHM-106`), et le voyage du `LOT-42` s'y choisit sans s'y jouer.
- **L'écran « Carte » montre les cartes peintes par l'auteur** — décision de l'auteur, 17
  septembre 2026, qui remplace « la carte du monde est supprimée pour le moment ». L'auteur a
  livré seize cartes peintes de sa main, sans lettrage : le monde, les treize régions de l'atlas,
  deux plans de ville (la Capitale impériale, Fisherman's Wharf). Le [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) rebâtit
  l'écran dessus, à **trois niveaux** — vue d'ensemble, région, ville —, et absorbe le `LOT-95` :
  le plan de la Capitale n'est plus à régénérer. Les noms ne sont jamais peints, les positions
  vivent dans un fichier à part de l'atlas, et un lieu que le livre ne situe pas reste sans
  marqueur — le jeu n'invente pas de position (`EX-IHM-107`). Le `LOT-42` n'a plus de carte du
  monde à produire ; `capital.json`, les niveaux quartier et îlot et la position du héros vont au
  `LOT-96`.
- **La première carte est le Colisée, en version finale** ([LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md)) — même
  décision. Un lieu avec ses zones neutres (entrée, couloirs, vestiaires, tribunes) et le sable
  comme zone de combat déclarée, pas un dégrossi ; il pose le style des textures et emporte, en
  sortie de lot, tout le contenu provisoire accumulé depuis le socle — sauf les quatre classes
  provisoires, qui sont un échafaudage de règles et tombent au dernier lot de classe. Les
  quartiers de la Capitale suivent (`LOT-96`).
- **Deux identités visuelles, une par couche.** La **scène** — figurines, monstres, textures — est
  en pixel art, style fixé par la maquette du [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) sur les figurines du `LOT-91` ;
  l'**interface** reste à la charte v2 du `LOT-87`. Le `LOT-66` avait retiré le pixel art de
  l'interface, pas du jeu ; les deux décisions tiennent ensemble.
- **Les ateliers d'assets sont des lots quand le slice attend leur méthode** : textures
  ([LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md)) et monstres (`LOT-93`) entrent dans la feuille de route avec un critère
  d'acceptation — la méthode et les premières pièces ; leur production continue ensuite au fil
  de l'eau, comme celle du `LOT-91`, qui reste hors page parce qu'il n'a pas de fin.

### Écarté à la relecture des livres

- **Montures volantes et *Flying Points*** : n'agissent que monté contre monté, sans sens sur une
  grille vue de dessus. Les montures restent une ligne de la table de voyage (`LOT-42`).
- **Combat naval** : la mer est un type de route (`LOT-42`), pas un champ de bataille.
- **Les deux fiches alternées du cirrus** : une seule face, choisie à la création ; le don
  *Soul Connection* tombe avec.
- **Les tests étiquetés** (« lié aux dragons », « mathématique ») : un étiquetage sémantique des
  tests que rien ne sait produire ; les traits qui en dépendent se déclarent narratifs.
- **Les sorts arbitrés par le maître de jeu**, ceux qui **rejouent un tour** (*Echoing*) ou
  **réécrivent un PNJ** (*Mind Reshaping*) : narratifs, `EX-RPG-051`.
- **Les zones mobiles** (oasis migrante, Deep Freeze qui s'étend, Illus qui marchent) : les cartes
  du `LOT-40` sont fixes.
- **Les plans autres que le pénombral** (Éthéré, Astral, Yrthak, Elemental Grounds).
- **La Malédiction malrokienne comme compteur global** ; sa règle locale (un massacre ouvre une
  faille) reste au `LOT-90`.
- **Les classes simplifiées** (Brawler, Mage, Priest, Scoundrel) comme classes du jeu final : ce
  sont les quatre provisoires du `LOT-36`, retirées au dernier lot de classe — décision inchangée.
- **Région de départ : la Capitale du Central Empire** — décision du 16 septembre 2026, qui
  **renverse** celle du 14. La République des Freelands avait été retenue pour ses statistiques :
  `Monster Presence` haute, économie de mercenaires, villes de frontière, siège de la Guilde. Ce
  raisonnement choisissait une région pour ce qu'elle *justifie* ; il ne donnait au slice aucun
  lieu que le livre décrive, et « le village à construire » restait à inventer. La Capitale est le
  lieu le plus **écrit** du Sourcebook (pages 94 à 103 du livre) : douze quartiers nommés et
  caractérisés, trois arènes, une intrigue ouverte — les enfants disparus —, sept personnages avec
  leurs commanditaires, et le plan `VTT/Map - Capital.jpg` qui la dessine rue par rue. Surtout, le
  Colisée du [LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md) s'y trouve : le seul contenu jouable du jeu est déjà dans cette
  ville. `Monster Presence : Low` n'est pas un obstacle — le slice ne combat pas des monstres
  errants mais des hommes de main et des champions d'arène, ce que la ville fournit sans rien
  inventer. La Guilde et les Freelands restent la porte du bac à sable (`LOT-45`, `LOT-42`).

- **Trois préconditions sont devenues des lots, et les trois sont livrées.** Écrire la moitié RPG
  de la spécification ([LOT-77](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-77-specification-rpg.md)), désambiguïser les numéros hérités
  ([LOT-78](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-78-numeros-herites.md)) et factoriser le chargement de données ([LOT-79](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-79-socle-chargement-donnees.md)) étaient
  signalés comme préalables et portés par personne. Un préalable sans porteur n'est pas un
  préalable, c'est une dette.
- **Le découpage suit une règle unique : le code d'un côté, la donnée de l'autre.** C'est ce qui a
  scindé `LOT-37`/`LOT-80`, `LOT-40`/`LOT-81`, `LOT-41`/`LOT-82`, `LOT-45`/`LOT-83` et
  `LOT-47`/`LOT-84`. Les deux moitiés n'ont ni le même métier, ni le même critère d'acceptation, ni
  le même moment.
- **Le `LOT-69` n'était plus une refonte de l'éditeur**, seulement le retrait de l'atelier pixel
  art — fait au [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md). Ce qu'il portait de refonte — le placement libre depuis le
  jeu — est l'arène du `LOT-50`.

### À trancher

- **Le plan pénombral, quand.** Le `LOT-90` est écrit et placé après le voyage ; il peut être
  différé au-delà de la première version du bac à sable sans rien casser. La décision est une
  décision de périmètre, pas d'architecture — les crochets qu'il exige (règles de zone, routes
  typées, horloge) sont pris par les lots qui le précèdent.
- **La portée identité du châssis d'édition.** `hmi::identityTokens()` et `hmi::applyFont()`
  chargent encore `Pixelify Sans` et `Press Start 2P`, que plus aucun écran du jeu n'emploie
  depuis le `LOT-87`, qui a explicitement laissé ce point ouvert. Suivre la charte v2, ou décider
  qu'un outil de développeur reste en pixel. Le [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md), qui a retiré l'atelier
  pixel art, a laissé la question ouverte : les polices sont de la charte, pas de l'héritage.
- **La production des 214 images du cahier des assets.** Le `LOT-87` a écrit le cahier et la
  chaîne de réception (`receive_ui_assets.py`), et n'a jamais lancé la production (T2.6) : par
  quel générateur, quand, et qui relit. Sans elles, chaque écran est un aplat ; avec elles, aucune
  ligne de QML ne change. Ce n'est pas un lot — c'est une commande à passer.

- **Profondeur du bac à sable.** Le `LOT-41` compose des quêtes par gabarit et le `LOT-45` les
  distribue par rang. Faut-il en rester à ce contenu déduit, ou écrire par-dessus quelques quêtes à
  la main dans les lieux notables ? La réponse change le poids du [LOT-16](#lot-16), pas
  l'architecture.
*(Le sort du *Manuel des Monstres* est tranché : voir ci-dessus.)*

---

## 9. Ce que l'audit a révélé sur l'état du dépôt

Cette page a été confrontée à l'état réel du code et des spécifications. Cinq constats la
dépassent et conditionnent son exécution.

### 9.1 Cinq familles d'exigences sont fantômes — le lint le dit désormais

Le dépôt compte **13 familles réelles pour 269 exigences** (`EX-ARCH`, `EX-BUILD`, `EX-CTRL`,
`EX-DEC`, `EX-EDIT`, `EX-EXP`, `EX-GP`, `EX-IA`, `EX-IHM`, `EX-LVL`, `EX-NFR`, `EX-REN`, `EX-VIS`).
Cinq autres sont référencées par une vingtaine d'epics et **n'existent pas** : `EX-REG-*`,
`EX-RPG-*`, `EX-INV-*`, `EX-CBT-*` et `EX-CNT-*`. Les documents censés les porter — `regles-d20.md`,
`combat.md`, `rpg.md`, `contenu.md` — sont absents.

**Le garde-fou, lui, est réparé.** Il ne l'était pas : `scripts/lint_exigences.py` ne filtrait que
sur `EX-[A-Z]+-[0-9]+`, si bien qu'un `EX-REG-*` n'était **ni** une déclaration **ni** une
référence, et que la CI passait au vert sur cinq familles inexistantes. Une seconde expression,
`FAMILY_REF_RE = re.compile(r'EX-([A-Z]+)-\*')`, capte désormais les références de famille entière
et le lint **échoue** sur les cinq.

Cet échec était **voulu**, et c'était le seul mécanisme empêchant la dette de grossir en silence.
Il est **éteint** : le [LOT-77](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-77-specification-rpg.md) a écrit `regles-d20.md`, `rpg.md`, `combat.md`,
`inventaire.md` et `contenu.md`, soit **69 exigences** qui portent les cinq familles, et a posé la
rubrique « Exigences couvertes » sur les 25 lots qui les implémentent. Le lint comptait alors
**339 exigences déclarées et 339 référencées**, sans aucune entrée ajoutée à la liste des
exceptions ; il en compte 352 le 14 septembre 2026, les `LOT-86` et `LOT-87` ayant ajouté les
leurs (`EX-IHM-100` à `EX-IHM-105`, la charte v2).

Écrire la moitié RPG de la spécification était le vrai chantier de fond, celui dont dépendait tout
le reste — d'où un lot dédié plutôt qu'une ligne dans « à trancher ».

### 9.2 Six mécaniques étaient consommées sans être produites

Le repos était **invoqué par six lots et livré par aucun**. Le temps de jeu et le campement
n'existaient nulle part. Les conditions, l'agonie et l'expérience étaient mentionnées dans un
périmètre sans être ni spécifiées ni testées — et **aucune source d'expérience** n'était définie,
alors que trois lots en consomment.

D'où les `LOT-70` à `LOT-75`. Les rattacher au périmètre de lots existants aurait reproduit
exactement la cause du trou.

### 9.3 L'allègement est le geste à plus fort levier du programme

Les décisions prises rendent mort un volume de code supérieur à ce que les cinq lots suivants
ajouteront :

| Élément | Poids | Retiré par |
|---|---|---|
| Atelier de dessin pixel art (`Source/HMI/Editor/Pixel*`) | ~2 975 lignes | [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md) ✔ |
| Widgets pixel art (`Source/HMI/Interface/Pixel*`) | ~640 lignes | `LOT-66` ✔ |
| Écrans de plateforme (`LevelSelectScreen`, `LevelCompleteScreen`) | 622 lignes | `LOT-67` ✔ |
| Maquettes `SelectionNiveau`, `FinDeNiveau` + clés `level.*` | — | `LOT-67` ✔ |
| Couche Qt Widgets des écrans du jeu (`MainWindow` à 2 472 lignes, pile d'écrans, `.design-mockups/`, outillage du `LOT-85`) | −3 879 lignes | `LOT-86` ✔ |
| Charte v1 des écrans QML (treize contrôles, neuf jetons, polices pixel du jeu) | — | `LOT-87` ✔ |
| Runtime de l'ancien jeu, mécanismes, habillage de l'éditeur, assets et bruitages, archive `Documentation/Heritage/` | ~48 000 lignes | [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md) ✔ |

Le premier audit comptait **plus de 4 200 lignes** ; les deux refontes d'interface en ont retiré
davantage encore. Chaque suppression est portée par le lot qui la rend possible, **jamais
reportée** à un lot de ménage : du code mort qu'on garde « pour plus tard » se remet à coûter dès
la première refactorisation qui le traverse. Le [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md) a retiré le reste.

### 9.4 Le socle technique tient, mais trois préconditions manquent

Ce qui est sain : **927 tests verts** sur 129 fichiers au premier audit — **1 046 sur 140** le
14 septembre 2026 —, une CI durcie (`/W4 /WX`, `clang-tidy` et
Doxygen épinglés), un cœur déjà purgé de 63 000 lignes au [LOT-01](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-01-fork-purge.md) et déjà réorienté
vers le RPG top-down par les `LOT-06`, `LOT-07` et `LOT-08`. Le format v3 **porte déjà les
entités**
(`MapEntity`), et `GridDistanceField` — un parcours en largeur sur la grille — a été explicitement
sauvé de la purge comme « le calcul de portée de déplacement du futur combat tactique ».

Ce qui manque avant d'ajouter des dizaines de catalogues :

- ~~**Aucune brique de chargement JSON partagée.**~~ Six réimplémentations identiques de
  `loadFromFile` (`SkinCatalog`, `AnimationCatalog`, `SoundCatalog`, `PixelPalette`, `LevelLoader`,
  `LevelSequence`), validation écrite à la main champ par champ. **Corrigé au
  [LOT-79](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-79-socle-chargement-donnees.md)** : les six passent par `core::JsonDocument`, et un échec nomme désormais
  le fichier **et la ligne**.
- ~~**Aucun test paramétré**~~ dans tout `Source/Test/` — ni `TEST_P`, ni parcours de dossier de
  fixtures. **Créé au [LOT-79](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-79-socle-chargement-donnees.md)**, avec `Source/Test/Fixtures/Json/`.

  Ces deux manques étaient attribués au `LOT-32` par le tableau du §10, mais **absents de son
  périmètre**, qui ne livre que des schémas, un script Python et un test d'énumérations : personne
  ne portait le travail C++. C'est l'audit qui l'a trouvé.
- **`Source/Elements/Levels/` est vide.** Le jeu n'a aucune carte à charger ; `loadDeliveredLevel`
  (`test_render_budget.cpp`) est du code mort et la fixture `rejeu-test-deplacement.json` est
  orpheline depuis la purge. **Vrai jusqu'au 14 septembre 2026** — le [LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md) y
  a posé la première carte, celle de l'arène, que son écran dessine lui-même : le [LOT-86](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-86-refonte-hmi-quick.md)
  l'a reconstaté — le viewport n'affiche aucune scène, « Nouvelle partie » ouvre la fiche — et les
  treize écrans se vérifient par un sélecteur de développement faute d'un niveau pour y mener. La
  première carte d'**exploration** — Martpart, un quartier de la Capitale — arrive avec le
  [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md), qui donne au jeu Qt Quick sa scène ; la ville habillée avec le
  [LOT-27](#lot-27).

### 9.5 La collision de numéros est bloquante, et bien plus large qu'estimé

Le programme hérité allait jusqu'à `LOT-74`. En atteignant `LOT-84`, la filière **recouvre
entièrement** cette plage. Or les spécifications citent encore des lots hérités dans le corps de
leurs exigences — `gameplay.md` (« Complété en `LOT-65` »), `niveaux.md` (« `LOT-25`, étendu en
`LOT-65` »), `editeur-niveaux.md` (« `LOT-54` introduit un éditeur de texture »), les titres de
sections d'`interface-ihm.md`, et jusqu'à `architecture.md` (« Concrétisé en `LOT-33` »).

**Le décompte a été fait, et il corrige l'estimation initiale.** Les spécifications portent
**222 renvois `LOT-NN`, dont 208 ambigus** — tout numéro au-delà de `LOT-07`, puisque seuls les sept
premiers désignent à coup sûr un lot livré de ce dépôt. Ils se répartissent sur **douze fichiers**,
et non quatre :

| Fichier | Renvois ambigus |
|---|---|
| `editeur-niveaux.md` | 42 |
| `rendu-technique.md` | 34 |
| `interface-ihm.md` | 26 |
| `decors.md` | 21 |
| `gameplay.md` | 20 |
| `exigences-non-fonctionnelles.md` | 17 |
| `niveaux.md` | 15 |
| `controles.md` | 13 |
| `architecture.md` | 9 |
| `exploration.md` | 6 |
| `conventions.md` | 4 |
| `vision.md` | 1 |

Ces renvois étaient **ambigus sans être cassés** : ni le lint ni Doxygen ne les signalaient. Et ce
n'était pas « une recherche-remplacement dans six specs » comme l'estimait le §10 : chaque référence
devait être **classée** — lot hérité ou lot courant — avant d'être préfixée ou laissée telle quelle.
D'où un lot à part entière.

**C'est fait** : le [LOT-78](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-78-numeros-herites.md) a préfixé **201 renvois** dans dix fichiers,
laissé les 49 qui désignent de vrais lots courants, écrit la convention en tête de
`specifications.md`, et ajouté à `scripts/lint_lots.py` la **règle 12**, qui refuse tout `LOT-NN` de
spécification ne désignant pas un lot existant de ce programme. La classification s'est faite par
couple (fichier, numéro) et non par fichier. Le [LOT-88](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-88-retrait-heritage.md) a depuis retiré ces renvois
avec le programme qu'ils désignaient, et la règle 12 refuse l'ancienne notation.

### 9.6 Second audit, 14 septembre 2026 : ce qui avait dérivé en dix jours

Cette page a été relue contre `origin/main` après la livraison du [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md). Le lint
était vert ; il ne l'aurait pas été si cette dérive lui avait été visible, et c'est ce qui vaut
d'être consigné : ce qu'il ne vérifie pas vieillit en silence.

- **Un prérequis illisible par la règle.** Le `LOT-27` déclarait « phases B, C et D complètes » ;
  le calcul d'ordre n'y lisait aucun lot et plaçait le *vertical slice* au rang 5, « prêt ». Il
  déclare désormais ses onze prérequis en numéros, et le chemin critique du §6 en découle. Règle
  pour la suite : **une dépendance s'écrit `LOT-NN`, ou elle n'existe pas.**
- **Deux lots hors de la page.** Les `LOT-85` (abandonné), `LOT-86` et `LOT-87` ont été décidés
  et livrés sans passer par ici ; le compte de tête les ignorait, comme l'index `lots.md`
  ignorait quatre lots livrés (`LOT-14`, `LOT-18`, `LOT-39`, `LOT-86`). Corrigé.
- **Des sections écrites contre des classes retirées.** `core::LevelSequence`, `hmi::Progression`
  (retirés au `LOT-67`), `PixelFrameWidget` (au `LOT-66`), `hmi::ScreenFlow` comme navigation du
  jeu (c'est désormais celle du châssis d'édition ; le jeu navigue par `hmi::ScreenRouter`) : les
  `LOT-09`, `LOT-15`, `LOT-16`, `LOT-17` et `LOT-24` décrivaient un dépôt qui n'existe plus. Leurs
  « problèmes » sont réécrits contre l'état réel ; leurs périmètres ne changent pas.
- **Des écrans à dessiner qui existent.** Dialogue, journal, marchand, carte du monde, compagnie
  (guilde et équipe fondues), compétences et sorts, HUD de combat : tous existent en QML sur
  `PendingData`. Les lots `LOT-15`, `LOT-16`, `LOT-24`, `LOT-25`, `LOT-26`, `LOT-29`, `LOT-42` et
  `LOT-45` **branchent** un écran, ils n'en dessinent plus — chaque section le dit maintenant.
- **Une décision de la page contredite par un lot livré.** « L'éditeur devient un mode de l'arène,
  sans application séparée » (§8) et « le `LOT-11` vise l'édition dans la scène » (§10) contre les
  deux exécutables du `LOT-86`. Le point est remis en « à trancher », avec une recommandation, et
  le `LOT-11` ne démarre pas sans lui. **Tranché le 16 septembre 2026** en faveur de la
  recommandation, et livré au [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md).
- **Une charte décrite au passé.** Le §8 tenait le parchemin du `LOT-66` pour la charte ; le
  `LOT-87` l'a remplacée par la charte v2 des maquettes. Trois choses restent ouvertes après lui,
  toutes en §8 : la portée identité de l'éditeur, la production des 214 images, et la refonte
  d'`EX-EDIT-030` — celle-ci faite au [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md).
- **Des chiffres.** 29 lots livrés au lieu de 31, 927 tests au lieu de 1 046, 280 Mo au lieu de
  785 dans l'introduction, « huit lots livrés sur quatre-vingts » au §10, une fenêtre du §10
  (déclarer les ressources de la fiche avant le `LOT-13`) présentée comme ouverte alors qu'elle
  s'est refermée sans être saisie. Tous repris.

Ce que l'audit n'a **pas** fait : découper, fusionner ou réordonner un lot à la main. L'ordre
reste calculé ; il a seulement reçu une dépendance qu'il ne voyait pas.

**Seconde passe, le même jour : les entrants.** À la demande de l'auteur, les deux livres de
Tanares ont été relus en entier et confrontés à chaque lot de moteur (§4bis), et les
spécifications relues comme des entrants. Ce qu'elle a changé :

- l'**arène** est reclassée mode du jeu et banc d'essai, prérequise par le seul tour (`LOT-50`
  passe de la fin du programme à juste après le `LOT-20`) ; l'éditeur d'auteur reste séparé ;
- **treize régions** partout ; trois espaces sans encart ajoutés au `LOT-81` ;
- `LOT-70` devient un **calendrier lunaire**, `LOT-83` prend le système d'équipe, `LOT-46` et
  `LOT-47` reçoivent leurs contraintes chiffrées, `LOT-19` à `LOT-24` et `LOT-72` leurs crochets ;
- trois lots ajoutés là où une matière n'avait aucun porteur : `LOT-88` (les spécifications
  décrivent encore le jeu d'origine), `LOT-89` (20 dons, 29 objets magiques, consommables),
  `LOT-90` (le plan pénombral) ; `LOT-85` inscrit parmi les numéros retirés ;
- une liste de ce qui est **écarté**, nommément, au §8 ;
- sept exigences sans lot rattachées (`EX-VIS-002/003/004`, `EX-REG-031/050`, `EX-IHM-003`,
  `EX-RPG-042`), et `EX-INV-030` (la monnaie) rendue au `LOT-26`.

---

## 10. Ce qu'il faut anticiper, tant que rien n'est construit

**Trente et un lots sont livrés sur les quatre-vingt-dix que compte le programme** (huit au
premier audit). La fiche de personnage, l'inventaire, la bascule de combat et tous les écrans
existent désormais ; le combat lui-même, l'éditeur multi-couches, les dialogues et le graphe de
cartes, non. L'avantage était **temporaire**, et le tableau ci-dessous le montre : chaque décision
prise à temps a coûté une écriture ; celle qui ne l'a pas été coûte maintenant une rétro-adaptation.

Huit choses méritaient d'être décidées avant, et non après ; voici où chacune en est.

| À anticiper | Coût aujourd'hui | Coût si on attend |
|---|---|---|
| ~~**Retirer les exigences de plateforme**~~ (`EX-GP-030/031/032`, `EX-LVL-010`→`015`, `EX-IHM-005`) — fait au [LOT-67](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-67-menus-vocabulaire-rpg.md) | Une passe de rédaction, faite avant que les `LOT-09`→`17` ne se bâtissent dessus | — |
| ~~**La charte visuelle**~~ (`LOT-66`) — faite, puis **remplacée** par la charte v2 du [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) | Les écrans livrés entre les deux (`LOT-68`, `LOT-38`, `LOT-86`) ont été transcrits une fois : c'est le coût de droite, payé, pour treize écrans | — |
| ~~**Viser directement l'édition dans la scène**~~ au `LOT-11` au lieu de la refondre au `LOT-69` — **tranché** : retarger `LevelEditor`, livré au [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) | Le [LOT-86](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-86-refonte-hmi-quick.md) avait séparé l'éditeur dans son propre exécutable ; l'éditeur a été retargé une fois, sans refonte à venir | — |
| ~~**La brique de chargement JSON**~~ — fait au [LOT-79](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-79-socle-chargement-donnees.md) | Six lecteurs factorisés, et un échec qui nomme le fichier et la ligne | Vingt et plus, chacun avec sa validation manuscrite |
| ~~**Les tests paramétrés**~~ — fait au [LOT-79](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-79-socle-chargement-donnees.md) | Capacité créée sur une suite encore petite | À créer quand même, mais avec des dizaines de tests déjà écrits autrement |
| **L'horloge et le repos** (`LOT-70`) — **fenêtre fermée** | Le [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md) est livré sans que la fiche déclare ressources ni cadence | Rétro-adapter la fiche au `LOT-70`, puis tenir la règle pour les quinze classes : c'est ce coût-ci qui est dû |
| ~~**Les champs `"source"` et `"statut"` au schéma**~~ — fait au [LOT-32](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-32-schemas-donnees-rpg.md) et au [LOT-36](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-36-especes-historiques-classes.md) | `source` est requis par `common.schema.json` ; les quatre classes provisoires portent leur `statut` | — |
| ~~**Désambiguïser les numéros hérités**~~ — fait au [LOT-78](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-78-numeros-herites.md) | 201 renvois désambiguïsés dans dix specs, plus la règle de lint qui l'empêche de revenir | Ambiguïté silencieuse, invisible au lint comme à Doxygen |

Deux d'entre elles sont plus que des économies.

**Le `LOT-11` ne doit pas être construit puis refondu.** Le `LOT-69` était écrit comme une refonte
parce qu'il a été conçu après ; l'éditeur multi-couches n'étant pas commencé, la page concluait que
le `LOT-11` devait **viser d'emblée** l'édition dans la scène, et le `LOT-69` a été réduit au
retrait de l'atelier de dessin. La moitié « suppression » tient toujours ; la moitié « dans la
scène » a été rattrapée par le [LOT-86](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-86-refonte-hmi-quick.md), qui a fait de l'éditeur un exécutable à part.
Elle a été tranchée **avant** le `LOT-11`, comme cette ligne le demandait : l'auteur a retenu
`LevelEditor`, et le [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md) l'a retargé une fois pour toutes — aucune refonte n'est
prévue derrière lui.

~~**`MapEntity` existe déjà et n'alimente rien.**~~ Le format v3 portait `{ type, position,
properties }` sans consommateur hors des tests. **Fait au [LOT-10](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-10-entites-de-carte.md)** :
`core::MapEntitySpawner` en fait des entités, `core::WorldFlags` retient ce qui leur est arrivé, et
les PNJ du [LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md) s'y sont ajoutés ainsi (famille `npc`, qui nomme son dialogue),
et les portails du `LOT-09` feront de même plutôt que d'inventer un second conteneur.

---

## 11. Les lots `LOT-09` à `LOT-29`

Ces vingt et un lots avaient chacun leur dossier `LOT-NN-nom/`. Ils sont absorbés ici pour que
la feuille de route ait **une seule source de vérité** : deux documents décrivant le même
programme divergent, et l'audit a montré qu'ils avaient déjà commencé à le faire.

> Les `LOT-08`, `LOT-10`, `LOT-12`, `LOT-13`, `LOT-14`, `LOT-18` et `LOT-19` ont été **livrés**
> entre-temps.
> Ils ne sont donc plus ici : comme les sept premiers lots, chacun garde son dossier et son
> `epic.md`, qui porte ce que sa réalisation a tranché — de l'histoire, pas du programme. Restent
> quatorze lots, `LOT-11`, `LOT-15` à `LOT-17` et `LOT-20` à `LOT-29`.

Leurs **ancres Doxygen sont conservées** (`{#lot-09}`, `{#lot-13}`…), si bien que tous les
renvois `@ref lot-NN` des spécifications continuent de résoudre. Seuls les `@subpage` de
`lots.md` disparaissent, faute de pages séparées.

> **Contenu d'origine, corrections signalées.** Le texte de chaque epic est repris tel quel.
> Là où l'audit l'a contredit — l'éditeur du `LOT-11`, la mort du `LOT-21`, le repos du
> `LOT-25` — la correction figure dans les sections 5, 9 et 10 ci-dessus, qui font foi. Le second
> audit (§9.6) a en outre réécrit, **dans** les sections, ce qui décrivait des classes retirées ou
> des écrans déjà dessinés : ces passages sont marqués « *État au 14 septembre 2026* ».

### LOT-16 — Les affaires de la Capitale : quêtes et drapeaux de monde {#lot-16}

> Statut : **à faire**.
> Prérequis : [LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md) (les dialogues déclenchent et font avancer les quêtes),
> `LOT-96` (la quête se joue sur les cartes de Martpart et d'Arenarea, pas dans un test seul).

> **Réécrit le 16 septembre 2026.** La version précédente demandait « une quête à trois étapes »
> sans dire laquelle, ni où, ni avec qui. Ce lot écrit **la** quête du slice, tirée du livre, et
> l'appuie sur la ville du `LOT-96` : on la vérifie en parlant à Myr sur la place du
> marché, et le journal la suit à l'écran.

#### Objectif

Suivre l'avancement du joueur par des **drapeaux persistants**, en donner une lecture dans le
journal de quêtes, et le prouver sur une quête réelle de la Capitale : **« Les enfants de
Martpart »**.

#### La quête, telle que le livre la tend

Trois pages du Sourcebook s'emboîtent sans qu'on ait rien à inventer. Page 98 : « les disparitions
inexpliquées d'enfants, surtout dans les quartiers hors les murs, inquiètent la population ; les
autorités n'ont rien trouvé ». Page 101 : **Myr**, cheffe des Marchands de Mort, ancienne orpheline,
« enquête sur les enlèvements d'enfants qu'elle soupçonne liés aux mages impériaux » et « emploie
discrètement des mercenaires et des aventuriers ». Page 100 : les Tigres Jumeaux ont été « trouvés
sur le terrain d'expérimentation du haut mage impérial Thidexius ». Et page 91 : les conflits sans
coupable avéré « se règlent dans l'arène ».

D'où la quête, en six étapes :

| # | Étape | Où | Ce qui la fait avancer |
|---|---|---|---|
| 1 | Myr engage le personnage | Martpart, le marché | dialogue (`startQuest`) — pose aussi le **laissez-passer** pour Arenarea |
| 2 | Deux indices : un parent, un receleur du Troisième Œil | Martpart | deux drapeaux, dont un **jet de compétence** (Persuasion ou Intimidation) dans le dialogue |
| 3 | Le repaire trouvé dans les ruelles | Martpart | la porte du repaire lit les deux drapeaux et s'ouvre |
| 4 | Les enfants libérés | le repaire | le combat scénarisé du [LOT-27](#lot-27) pose le drapeau ; jusqu'à lui, un dialogue **provisoire**, marqué comme le héraut l'était |
| 5 | L'acheteur, un noble d'Arenarea, exige l'Arène du Destin : pas de preuve, donc le jugement du sable | Arenarea | dialogue du héraut de l'Arène |
| 6 | Le verdict | l'Arène du Destin | la fin du combat d'arène (`LOT-27`) pose le drapeau ; provisoire jusque-là |

La récompense est déclarée en JSON — un objet d'inventaire ([LOT-14](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-14-inventaire-et-equipement.md)) et un drapeau ;
l'or attend la monnaie du [LOT-26](#lot-26), l'expérience le [LOT-74](#lot-74). La piste
vers Thidexius reste ouverte : c'est le bac à sable qui la reprendra (`LOT-82`).

#### Périmètre

- *État au 14 septembre 2026.* `core::WorldFlags` (`Source/Core/Gameplay/`) **existe depuis le
  [LOT-10](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-10-entites-de-carte.md)** : ensemble de drapeaux nommés, à clés libres précisément pour que ce
  lot y écrive les siens sans toucher au fichier. Le coffre ouvert (`LOT-10`) et l'ennemi vaincu
  ([LOT-18](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-18-bascule-exploration-combat.md)) y sont déjà. Ce lot n'a pas de substrat à créer.
- *État au 14 septembre 2026.* **Les dialogues démarrent déjà une quête** ([LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md)) :
  l'action `startQuest` d'un nœud pose `core::questStartedFlag(id)` — `quest/<id>/started` —, et
  les conditions d'un dialogue lisent n'importe quel drapeau. Une étape qui avance par dialogue
  n'est qu'une action `setFlag` de plus : aucun appel du runner vers les quêtes n'est à écrire.
- `Quest.{h,cpp}` : étapes, conditions d'avancement (des drapeaux), récompenses. Définie en
  **JSON**, `Source/Elements/World/quests/enfants-de-martpart.json`, avec son schéma.
- **Les deux sens du lien drapeau ↔ carte**, sur la ville : un dialogue pose le laissez-passer et
  la porte d'Arenarea (`requiresFlag`, [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md), posé au `LOT-96`) s'ouvre ; la porte du repaire, une
  entité, lit deux drapeaux ; le coffre du repaire, ouvert, pose un drapeau que Myr lit.
- **Les PNJ de la quête, posés sur les cartes** : Myr, le parent, le receleur, le héraut de
  l'Arène du Destin (un second héraut : celui du Colisée reste à sa grille depuis le
  [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md), qui a déjà cessé de l'ouvrir en dur). Dessinés par le marqueur du [LOT-39](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-39-cles-assets.md) tant
  que leurs figurines n'arrivent pas (`LOT-27`).
- Journal de quêtes : **l'écran existe** (`JournalForm.ui.qml`, [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md)) sur
  `PendingData` ; ce lot lui donne sa vue-modèle — quêtes en cours, étape courante et son texte,
  quêtes terminées.
- L'état d'une quête est **sérialisable** ; le [LOT-17](#lot-17) le persiste.

#### Note de conception

Les drapeaux de monde et les quêtes sont deux niveaux du même mécanisme : une quête **lit** des
drapeaux pour décider de son avancement, elle n'a pas d'état propre au-delà de son étape courante.
Cette séparation évite le piège classique où l'état du monde existe en double — une fois dans les
entités, une fois dans les quêtes — et diverge. Les deux étapes que le combat pose (4 et 6) le
montrent : le jour où le [LOT-27](#lot-27) remplace le dialogue provisoire par une fin de
combat, la quête ne change pas d'une ligne.

#### Exigences couvertes

`EX-VIS-003` (dialogue à choix, avec un jet de compétence), `EX-RPG-*` (quêtes, drapeaux).

#### Critères d'acceptation

- **À l'écran** : on parle à Myr sur Martpart, la quête apparaît au journal ; la porte d'Arenarea,
  fermée avant, s'ouvre ; chaque étape franchie se lit au journal.
- **Headless** : la quête se déclenche, avance par ses six étapes et se termine, par drapeaux
  seulement ; un drapeau posé par un dialogue est lu par une entité de carte, et inversement.
- Le jet de compétence de l'étape 2 est joué par le runner du [LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md), à graine
  fixée (`EX-VIS-005`).
- `check_rpg_data.py` valide le fichier de quête ; une étape qui cite un drapeau que rien ne pose
  est une erreur au chargement.

### LOT-17 — Reprendre sa partie dans la Capitale : sauvegarde riche {#lot-17}

> Statut : **à faire**.
> Prérequis : [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md), `LOT-96` (le quartier courant et les quartiers visités),
> [LOT-10](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-10-entites-de-carte.md), [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md), [LOT-16](#lot-16).

> **Réécrit le 16 septembre 2026.** Le périmètre ne change pas — un état de partie complet et
> versionné — mais il a maintenant un état à sauvegarder : un quartier, une case, une quête à une
> étape, une porte ouverte. Les critères le disent en gestes de joueur, pas seulement en
> aller-retour de fichier.

#### Objectif

Donner au jeu un état de partie complet et versionné, qu'on sauvegarde et qu'on reprend : quitter
Arenarea au milieu de « Les enfants de Martpart », relancer le jeu, appuyer sur « Continuer », et
se retrouver à la même case, avec le même journal.

#### Le problème

*État au 14 septembre 2026.* La progression héritée (`hmi::Progression`, un tableau atteint) a
été retirée par le [LOT-67](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-67-menus-vocabulaire-rpg.md) avec l'entrée « Continuer » du menu, qui n'avait plus
rien à reprendre. **Rien ne persiste** aujourd'hui hors les options ; le menu du
[LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md) réserve à ce lot ses entrées « Continuer » et « Charger », grisées jusqu'à
lui.

#### Périmètre

`Source/Core/Rpg/SaveGame.{h,cpp}` : JSON **versionné**, portant

- la carte courante — un quartier de la Capitale, ou le repaire — et la position exacte du
  personnage ;
- la **liste** des personnages (décision de cadrage : un héros au départ, quatre à terme — c'est
  une liste dès maintenant, pas un champ unique qu'on pluraliserait plus tard) ;
- l'inventaire et l'équipement ;
- les drapeaux de monde et les entités consommées (le coffre du repaire, la porte d'Arenarea
  ouverte, les ennemis vaincus) ;
- les quêtes et leur étape ;
- les quartiers **visités** — l'état de découverte, que le `LOT-42` étendra au monde.

Et ce qui l'entoure :

- **« Continuer »** reprend la dernière sauvegarde ; **« Charger »** les liste, avec en en-tête le
  quartier, l'étape de quête et la date — ce qu'un joueur lit pour choisir.
- Une **sauvegarde automatique** au passage de chaque portail, en plus de la sauvegarde manuelle :
  décision de ce lot, parce qu'une ville se traverse par portes et qu'on y perd sinon un quartier
  entier.
- **Pas de sauvegarde pendant un combat** : la session de combat ne se sérialise pas dans ce lot ;
  l'entrée est grisée tant qu'un combat est engagé, et le dire vaut mieux qu'un format qui
  promettrait de le faire.
- Emplacement : le dossier de données utilisateur de la plateforme (`QStandardPaths`), un fichier
  par emplacement.

#### Règles de format

Mêmes règles que le format de carte, pour les mêmes raisons : **versionné**, **tolérant aux champs
inconnus**, migration ascendante. Une sauvegarde est la donnée que le joueur ne peut pas
reconstruire — un format qui casse lui fait perdre sa partie.

#### Exigences couvertes

`EX-RPG-*` (sauvegarde, chargement, versionnement, tolérance).

#### Critères d'acceptation

- **À l'écran** : sauvegarder sur Arenarea à l'étape 5 de la quête, quitter, « Continuer » — même
  quartier, même case, même journal, porte d'Arenarea toujours ouverte.
- **Headless** : aller-retour sauvegarde → chargement **à l'identique** sur un état riche — la
  quête à **chacune** de ses étapes, inventaire garni, deux quartiers visités, coffre pris.
- Une sauvegarde d'une version antérieure se charge avec des valeurs par défaut sensées.
- Un champ inconnu est ignoré **et préservé** à la réécriture.
- L'entrée « Sauvegarder » est indisponible pendant un combat, et un test le vérifie.

### LOT-25 — Sorts et capacités de classe {#lot-25}

> Statut : **à faire**.
> Prérequis : [LOT-13](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-13-fiche-de-personnage.md), [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md), [LOT-22](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-22-portee-ligne-de-vue.md).

#### Objectif

Ajouter les sorts — emplacements, incantation, concentration — et les capacités propres aux
classes.

#### Périmètre

- `Source/Core/Rpg/Spell.{h,cpp}` : école, niveau, portée, durée, composantes, effet.
- `SpellSlots` : emplacements par niveau, consommés à l'incantation, restaurés au repos.
- `Concentration` : un seul sort concentré à la fois ; il tombe si le lanceur subit des dégâts
  (jet de sauvegarde) ou en incante un autre.
- Catalogue de sorts en **JSON** (`EX-VIS-007`), extrait par le `LOT-35`.
- *État au 14 septembre 2026.* **L'écran existe** : `SkillsForm.ui.qml` ([LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md),
  maquette 10, compétences et sorts) sur `PendingData` ; ce lot et le `LOT-35` l'alimentent.
- Branchement sur les gabarits d'effet de zone du [LOT-22](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-22-portee-ligne-de-vue.md) — aucune géométrie
  nouvelle : `core::AreaOfEffect` et ses cinq formes, `core::areaTilesFromMeters` pour les tailles
  du catalogue, `core::combatantsInArea` pour les cibles. L'abri contre l'origine de la zone
  s'ajoute aux sauvegardes de Dextérité (`core::coverFromPoint`, `core::coverBonus`), et
  « l'origine derrière un obstacle qu'on ne voit pas » reste à écrire ici.
- Effets **hors combat** aussi : un sort d'utilité en exploration passe par le même catalogue.

#### Note de conception

C'est le lot où la tentation d'écrire des règles en dur est la plus forte, parce que chaque sort a
sa particularité. Y céder rend l'équilibrage impossible : un sort qui se règle en recompilant ne se
règle pas. Un sort est une **donnée** ; le C++ ne porte que les *mécanismes* qu'elle compose
(dégâts de zone, jet de sauvegarde, condition appliquée, durée).

#### Exigences couvertes

`EX-REG-*`.

#### Critères d'acceptation

- Un emplacement consommé est indisponible jusqu'au repos.
- La concentration tombe au bon moment (dégâts avec échec de sauvegarde, second sort concentré),
  et **pas** aux mauvais.
- Un sort de zone touche exactement les cases du gabarit du `LOT-22`.
- Aucune règle de sort codée en dur dans le C++.

### LOT-26 — Butin, marchands, économie {#lot-26}

> Statut : **à faire**.
> Prérequis : [LOT-14](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-14-inventaire-et-equipement.md), [LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md).
> Exigences couvertes : `EX-INV-040`, `EX-INV-030` (la monnaie, à taux fixes, sans perte par arrondi
> — le peuplement civil ne fait ensuite que la moduler par région).

#### Objectif

Boucler la boucle économique : gagner du butin, le vendre, acheter mieux.

#### Périmètre

- `Source/Core/Rpg/LootTable.{h,cpp}` : tables de butin en **JSON**, tirées avec le hasard
  déterministe du `LOT-12` — un coffre rouvert après rechargement de sauvegarde doit donner le
  **même** contenu, sinon le joueur peut relancer jusqu'au bon tirage.
- Or et valeur des objets — *contraintes du corpus (§4bis)* : cinq pièces et l'électrum comme
  monnaie standard de Yama, lettres de crédit de la Banque d'Hajal, prêts à 5 % par mois et
  coffres, grille de prix des trois minerais, légalité et marché noir selon la région (`LOT-80`).
  Les objets magiques et les consommables à durée viennent du `LOT-89`.
- `Shop.{h,cpp}` : achat, vente, marge du marchand, stock.
- *État au 14 septembre 2026.* **L'écran existe** : `MerchantForm.ui.qml` ([LOT-68](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-68-chassis-ecrans-rpg.md),
  restylé au [LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md)) sur `PendingData` ; ce lot lui donne sa vue-modèle.

#### Le piège du tirage

Un butin tiré au moment de l'ouverture, avec une graine liée à l'instant, se re-tire différemment à
chaque chargement de sauvegarde. La graine doit dériver de l'**identité du coffre** et de l'état de
la partie, pas de l'horloge — `deriveSeed(baseSeed, step, entityId)` existe pour cela.

#### Exigences couvertes

`EX-RPG-*`, `EX-INV-*`.

#### Critères d'acceptation

- Un coffre donne le **même** butin après sauvegarde et rechargement.
- Vendre puis racheter un objet ne crée ni ne détruit de valeur au-delà de la marge annoncée.
- Le stock d'un marchand se comporte de façon définie quand il est épuisé.

### LOT-27 — La Capitale : contenu du *vertical slice* {#lot-27}

> Statut : **à faire**.
> Prérequis : [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md), [LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md), [LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md),
> [LOT-16](#lot-16), [LOT-17](#lot-17), [LOT-19](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-19-grille-tactique.md), [LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md),
> [LOT-21](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-21-attaques-degats-etats.md), [LOT-22](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-22-portee-ligne-de-vue.md), [LOT-23](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-23-ia-tactique.md), [LOT-24](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-24-ihm-combat.md),
> [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) (les planches de la ville), `LOT-93` (les champions du bestiaire), [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) (le plan
> de la Capitale, source du tracé), `LOT-96` (les deux quartiers à habiller).

> **Réécrit le 16 septembre 2026.** « Un village, un donjon, trois PNJ » ne nommait rien que le
> livre décrive, et un contenu qu'aucun texte ne porte s'invente au dernier moment — mal. Le
> slice se joue désormais dans la **Capitale** (§8), le lieu le plus écrit du Sourcebook et celui
> où le Colisée du [LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md) se trouve déjà. Ce lot livre la ville **habillée** :
> ses cartes, ses habitants, ses textures, et le combat posé sur la carte.

> **Hérité du [LOT-24](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-24-ihm-combat.md), le 15 septembre 2026.** L'IHM de combat a été livrée sur le
> Colisée, parce que le jeu Qt Quick n'avait ni exploration ni combat hors de l'arène. Le
> [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) donne l'exploration ; reste ici ce que personne d'autre ne porte : la
> rencontre engagée (`core::beginEncounter`) monte une session de combat sur la carte **gelée**,
> et `CombatHudForm` lit la vue-modèle et la prévisualisation du `LOT-24` (`core::previewAttack`,
> `core::previewMove`, les gestes du curseur) au lieu de `PendingData`.

#### Objectif

Produire le contenu jouable qui démontre la boucle entière : *« un personnage explore Martpart,
parle à Myr, suit la piste des enfants disparus, libère le repaire dans un combat tactique sur la
carte, et gagne le verdict de l'Arène du Destin »* — dans une ville qui ressemble à la Capitale.

#### Périmètre

##### Les cartes — quatre, tracées depuis le plan

Dessinées dans l'éditeur ([LOT-11](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-11-editeur-multicouches.md)), pas en JSON à la main : c'est le test grandeur
nature de l'outil, et la preuve qu'un non-développeur pourrait le faire (`EX-VIS-006`). L'éditeur
en question est celui que refait la [feuille de route de l'éditeur](feuille-de-route-editeur.md) : ses
lots `LOT-EDITOR-01` à `LOT-EDITOR-06` passent **avant** ces cartes — un ordre, pas un prérequis
déclaré, puisque l'éditeur a sa piste à part — et le repaire ne reçoit pas de script d'atelier. La source
est le **plan de la Capitale peint par l'auteur** ([LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md)), ses douze quartiers
placés d'après l'atlas ; le plan du corpus n'est qu'une référence lue sur le poste (§8).

| Carte | Rôle dans la boucle | Ce que le plan y nomme |
|---|---|---|
| **Martpart** (extérieur) | le « village » : le point de départ, le marché, Myr, les témoins, les ruelles | le Marché municipal, l'arène Illu Die, les tavernes du quartier sans sommeil |
| **Le repaire du Troisième Œil** (intérieur) | le « donjon » : trois salles, deux rencontres, un coffre gardé — le registre des acheteurs | sous les ruelles de Martpart ; la prose : « un monde souterrain de trafics et de complots » |
| **Arenarea** (extérieur) | le noble accusé, le héraut, le parvis de l'Arène | l'Arène du Destin, le casino du Calice d'Or, l'Hippodrome, le Crépuscule de la Justice, le bazar de l'Anse, l'avenue Herofate |
| **L'Arène du Destin** | le verdict : un combat d'arène à composition fixée | la carte du Colisée du [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md), sa zone de combat et une variante de planche (loge impériale, bannières) ; `arena-of-fate.json` entre au catalogue des arènes, à Marque Héroïque — personne n'y meurt |

Martpart et Arenarea sont les cartes du `LOT-96`, **habillées** ; le repaire est
nouveau. Les dix autres quartiers gardent leur sentinelle : ils s'ouvriront avec le peuplement
civil du `LOT-82`, un par un.

##### Les habitants — les figurines de l'atelier

Les personnages sont des figurines 48 × 64 de l'atelier du `LOT-91`, série **« Capitale »**,
produites par la méthode du PoC ; elle n'attend rien du moteur et **démarre en parallèle du
[LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md)**. Deux ont leur fiche au *Character Compendium* : **Myr** (page 91) et
**Galender, le maître d'armes** (page 48), mentor à l'Arène du Destin, qui « respecte le courage
des aventuriers et les forme ». Les autres se rédigent depuis la prose du Sourcebook, sans fiche :
la sentinelle et la patrouille Ironhand, le spadassin et l'archer du Troisième Œil, le marchand du
marché, le parent d'un enfant disparu, un enfant, le héraut de l'Arène, le noble des Blood Bound.
Dix figurines ; le marqueur du [LOT-39](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-39-cles-assets.md) remplace chacune tant qu'elle n'est pas
livrée, et le contenu se joue sans attendre l'art. La sentinelle et la patrouille Ironhand sont
**déjà dessinées** : c'est le soldat Ironhand de l'atelier des monstres ([LOT-93](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-93-atelier-monstres.md)),
que portent les sentinelles de Martpart et d'Arenarea. Les **champions loués** du noble ne sont pas
des PNJ mais des blocs du bestiaire, dessinés par le même atelier.

Le **marchand** parle, il ne vend pas : la boutique est au [LOT-26](#lot-26) (`0.0.2`), et
`MerchantForm` attend sa vue-modèle. Le slice ne le cache pas.

##### Les textures — trois jeux, une planche chacun

Par l'atelier du [LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md) : une **planche de production** par lieu, commandée depuis le descriptif
d'atlas du lieu (bloc B), découpée par le script de l'atelier depuis sa disposition JSON, chaque
cellule au cahier des assets (`check_assets_brief.py`).

| Jeu | Pour | Ce que la prose impose |
|---|---|---|
| **Martpart** | l'extérieur du marché | « rues pavées, étals éclairés de lanternes, architecture aux styles mêlés » : pavés, étals, auvents, lanternes, tonneaux et caisses, façades mêlées, une fontaine |
| **Arenarea** | l'extérieur noble | « grands hôtels à colonnes de marbre, larges jardins de façade, fontaines de pierre ouvragées » : dalles de marbre, colonnes, grilles, pelouses et haies, fontaines |
| **Bas-fond** | le repaire | caves : pierre, poutres, cages, paillasses, le coffre |

L'Arène du Destin reprend la planche du Colisée ([LOT-92](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-92-atelier-textures.md)) ; ses gradins nobles sont une variante.

##### Le combat sur la carte

- Les deux rencontres du repaire : `core::beginEncounter` gèle la carte, monte
  `core::CombatState` dessus, et `CombatHudForm` joue sur la vue-modèle du [LOT-24](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-24-ihm-combat.md).
  La fin de combat pose le drapeau de l'étape 4 ([LOT-16](#lot-16)) et retire le dialogue
  provisoire.
- Deux types d'**ennemis** avec deux des cinq profils d'IA du [LOT-23](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-23-ia-tactique.md) : le spadassin
  (mêlée agressive), l'archer (distance prudente).
- **Le défi d'arène depuis la ville** : le dialogue du héraut monte une `core::ArenaSession`
  ([LOT-50](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-50-colisee.md)) à composition fixée — le personnage contre les champions loués du noble —
  et sa fin pose le drapeau du verdict. C'est le geste du [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md), le héraut du
  Colisée qui lance le combat sur la zone déclarée, rejoué sur l'Arène du Destin ; le voyage reste
  au `LOT-42`.

##### Retraits

Les personnages et rencontres de démonstration sont partis avec le [LOT-09](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-09-colisee-premiere-carte.md) ; restent
ici les deux dialogues provisoires des étapes 4 et 6, qui disparaissent, et le dernier marqueur
du chemin de la quête.

#### Outillage

`scripts/check_world_graph.py` remplace le `check_demo_sequence.py` hérité, retiré au `LOT-01`.
Il valide, en CI :

- le **graphe de cartes** : aucun portail orphelin, aucune carte inatteignable depuis la porte de
  départ ;
- les **références entre fichiers** : chaque drapeau qu'une quête attend est posé par un dialogue,
  une entité ou une fin de combat ; chaque PNJ posé sur une carte a un dialogue du catalogue ;
  chaque récompense existe — le chargeur de dialogues du [LOT-15](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-15-pnj-dialogues.md) vérifie déjà
  l'intérieur d'un fichier, ce script relie les fichiers **entre** eux ;
- que chaque zone de rencontre est un **terrain tactique valide** (contrainte du `LOT-11`, née de
  la décision « combat sur la carte ») ;
- que **le chemin de la quête n'affiche aucun marqueur** de clé d'asset manquante
  ([LOT-39](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-39-cles-assets.md)) : les textures et figurines du slice sont livrées, pas promises.

#### Exigences couvertes

`EX-RPG-*`, `EX-LVL-*`, et concrétisation de `EX-VIS-001` à `EX-VIS-005`.

#### Critères d'acceptation

- La boucle complète se joue de bout en bout, de la porte de Martpart au verdict.
- Un **test système** la rejoue en headless, du départ à la victoire de l'Arène.
- `check_world_graph.py` vert ; aucun marqueur sur le chemin de la quête.
- Le contenu se recharge après sauvegarde à n'importe quel point de la boucle.
- Les quatre cartes ont leur capture de référence dans les tests QML.
### LOT-28 — Audio, effets et version `0.0.1` {#lot-28}

> Statut : **à faire**.
> Prérequis : [LOT-27](#lot-27), [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) (aucune image du corpus dans une version).

#### Objectif

Donner au jeu son identité sonore et ses retours d'impact, puis clore le programme du *vertical
slice* par une version publiable.

#### Périmètre

##### Audio

`hmi::AudioEngine` et `SoundCatalog` sont hérités et fonctionnels — ce lot les **alimente**, il ne
les réécrit pas :

- musique de carte et thème de combat, avec bascule à l'entrée et à la sortie de rencontre ;
- bruitages : pas, interaction, ouverture de coffre, jet de dé, coup porté, coup critique, échec,
  fin de combat, navigation de menu.

La règle héritée tient : **le jeu reste pleinement jouable en silence**, sans périphérique audio,
et le volume est réglable et persisté.

##### Effets

`ParticleRenderer` et la secousse d'écran sont hérités : particules à l'impact, secousse **sur
critique uniquement** (une secousse à chaque coup rendrait un combat tour par tour épuisant).

##### Équilibrage

Passe de réglage sur les données du slice : PV, CA, dégâts, seuils d'expérience, prix. Tout est en
JSON depuis la phase C — aucune recompilation.

##### Clôture

- Régénération du **cahier de test** (`scripts/generate_cahier_test.py`).
- Le lint de provenance du [LOT-94](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-94-cartes-de-l-auteur.md) est vert : la version ne contient **aucune image du corpus**.
- Mise en cohérence documentaire globale : specs, guides, manuel.
- `project(VERSION 0.0.1)` dans le `CMakeLists.txt` racine — **seul endroit** où la version
  s'écrit — puis tag `v0.0.1`. Dans ce programme, `0.1.0` est le bac à sable complet, et le slice
  est son premier `0.0.x`.

#### Note de méthode

La documentation de chaque domaine est mise à jour **par le lot qui le livre**, pas ici. Ce lot ne
fait que la cohérence d'ensemble et la régénération. Un lot fourre-tout de fin de programme n'est
jamais fini : c'est le risque à éviter.

#### Exigences couvertes

`EX-REN-*` audio et effets, `EX-NFR-*` (budget de rendu mesuré, patron hérité).

#### Critères d'acceptation

- Le jeu est pleinement jouable **sans périphérique audio**.
- Volume réglable et persisté ; bascule musicale exploration ↔ combat sans coupure brutale.
- Secousse d'écran réservée aux critiques.
- Cahier de test régénéré, tous les linters verts, version bumpée et taguée.

### LOT-29 — Groupe de quatre personnages {#lot-29}

> Statut : **à faire**.
> Prérequis : [LOT-27](#lot-27). Vient **après** le *vertical slice*, délibérément.

#### Objectif

Passer d'un héros seul à un groupe de quatre — recrutement, compagnons suiveurs en exploration,
combat tactique à quatre alliés.

#### Pourquoi ce lot est un lot d'**ajout**, pas une refonte

C'est la décision de cadrage n° 4, et tout le programme la prépare : **rien ne doit supposer
l'unicité du personnage**. Concrètement, au moment d'aborder ce lot, ces précautions doivent déjà
être en place :

- `CharacterSheet` est un objet **autonome** (`LOT-13`), jamais un singleton joueur ;
- `TurnOrder` est **multi-alliés** dès le [LOT-20](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-20-initiative-tour-par-tour.md), avec un test qui monte quatre
  alliés — **tenu** : `CombatStateTest.QuatreAlliesSansHerosUnique` ;
- `SaveGame` stocke une **liste** de personnages dès le `LOT-17`, pas un champ unique ;
- les écrans de fiche et d'inventaire sont conçus pour un **sélecteur de personnage**, même quand
  ils n'en affichent qu'un — et `CompanyForm.ui.qml` ([LOT-87](../../versions/v0.0.0/v0.0.0-fondation/lots/LOT-87-charte-v2.md)) montre déjà six
  places d'équipe, dont une seule occupée.

Si l'un de ces points a dérivé en chemin, ce lot redevient une refonte — c'est le signal
d'alarme à surveiller pendant les phases C et D.

#### Périmètre

- **Recrutement** de compagnons (dialogue, quête).
- **Personnages suiveurs** en exploration : ordre de marche, suivi du héros, pathing simple.
- **Sélecteur de personnage** activé dans les écrans de fiche, d'inventaire et d'équipement.
- Quatre alliés dans l'ordre d'initiative ; **ciblage allié** (soins, sorts de soutien).
- Répartition de l'expérience et du butin.
- *Contraintes du corpus (§4bis).* Le groupe n'est pas seul sur la carte : gardien-plante du druide,
  morts-vivants du Chevalier de la Mort (jusqu'à vingt), invocations paramétriques, montures. Ce
  sont des combattants **hors quota** dans l'initiative, qui suivent, obéissent ou non, et que la
  sauvegarde du `LOT-17` retient ; la compagnie du `LOT-83` en fait des recrues.

#### Le point délicat

Le combat à quatre alliés multiplie les tours et allonge la boucle : l'IHM du `LOT-24` doit rester
lisible avec huit combattants au bandeau d'initiative. C'est le seul endroit où ce lot peut exiger
un vrai travail d'interface plutôt qu'un simple ajout.

#### Exigences couvertes

`EX-RPG-*`, `EX-CBT-*`, `EX-IHM-*`.

#### Critères d'acceptation

- Un combat à **4 alliés contre 4 ennemis** se déroule en headless jusqu'à une condition de fin.
- **Aucune régression** du jeu à un personnage : le contenu du `LOT-27` reste jouable tel quel.
- Les suiveurs ne restent pas coincés dans le décor ni ne bloquent le héros dans un passage étroit.
- Le bandeau d'initiative reste lisible à huit combattants.
