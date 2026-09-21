+++
id = "LOT-67"
titre = "Menus et vocabulaire d'un RPG"
version = "0.0.0"
filiere = "interface"
statut = "livre"
taille = "M"
resume = "Le jeu cesse de décrire un jeu de plateforme : la notion de niveau discret quitte les écrans, le code, le vocabulaire et les exigences, et le menu s'ouvre sur la carte du monde."
prerequis = [
  "LOT-66",
  "LOT-76",
]
livrables = [
  "Le menu principal à cinq entrées sur la **carte du monde de Tanares** (JPEG à 96 ppp, région déclarée, manifeste recoupé en CI) et l'écran de pause à trois entrées.",
  "Le retrait de `LevelSelectScreen`, `LevelCompleteScreen`, `hmi::Progression`, `core::LevelSequence` et `hmi::LevelRunStats` (environ 1 300 lignes).",
  "Douze exigences retirées (ancres conservées), trois refondues (`EX-GP-040`, `EX-IHM-003`, `EX-IHM-004`), une ajoutée (`EX-IHM-076`).",
  "Les deux catalogues de traduction passés de « niveau »/« tableau » à « carte » (`level.*` → `map.*`), 376 clés synchrones.",
  "L'événement sonore `LevelCompleted` renommé `ExitReached` (`sortie_atteinte`), `SequenceCompleted` supprimé.",
]
criteres = [
  "**Plus aucune clé de traduction n'emploie « niveau » ou « tableau »** au sens du jeu de plateforme.",
  "**Les deux catalogues restent synchrones** — 376 clés de part et d'autre, vérifié par test.",
  "**Chaque entrée de menu mène à un écran qui existe** (`EX-IHM-072`) : cinq entrées, cinq destinations.",
  "**Aucune exigence en vigueur ne suppose plus une séquence ordonnée de tableaux** : douze retirées, trois refondues, aucune ancre supprimée.",
  "`Documentation/Doxyfile` n'annonce plus « Jeu 2D de plateforme/puzzle ».",
  "`ctest` : **1007/1007** (1032 avant, moins les 25 tests des mécanismes retirés).",
]
+++

## Pourquoi

Le jeu décrivait un autre jeu. `fr.lang` proposait « Choisir un niveau », l'écran de pause
« Recommencer le niveau », et un avertissement de sortie parlait de « la progression du **tableau**
en cours ». Un bac à sable ouvert n'a ni niveau à choisir, ni tableau à recommencer.

Ce lot retire la **notion de niveau discret** — des écrans, du code, du vocabulaire et des
exigences — et remplace le décor du menu principal par la **carte du monde de Tanares**.

## Conception

### Ce que le joueur voit

| | Avant | Après |
|---|---|---|
| Menu principal | Continuer · Nouvelle partie · Choisir un niveau · Mode Édition · Options · Crédits · Quitter, sur un paysage de plateforme tracé en pavés | Nouvelle partie · Mode Édition · Options · Crédits · Quitter, sur la **carte du monde** |
| Titre du menu | un bandeau « Just Another RPG Game » | aucun : le fond dit déjà où l'on est |
| Écran de pause | Reprendre · Recommencer le niveau · Options · Quitter | Reprendre · Options · Quitter |
| Fin de niveau | un écran de bilan (temps, morts, sauts) | il n'y en a plus |

**Deux entrées de menu disparaissent parce qu'elles ne menaient plus nulle part.** « Continuer »
reposait sur une progression au tableau, « Choisir un niveau » sur une séquence : les deux sont
retirées. Les rétablir en les grisant aurait coûté plus de confiance qu'elles n'apportaient
d'information (`EX-IHM-072`). « Continuer » revient avec la sauvegarde du `LOT-17`, quand il y aura
quelque chose à reprendre ; les entrées RPG de la pause — fiche, inventaire, journal, carte — avec
les écrans du `LOT-68`, quand elles auront où mener.

### La carte du monde en fond, et la frontière qu'elle trace

Le [LOT-76](@ref lot-76) venait de conclure que l'habillage se **trace** et ne se livre pas en
image (`EX-IHM-075`). Ce lot livre une image. Ce n'est pas un revirement : c'est la même frontière,
prise de l'autre côté, et `EX-IHM-076` l'écrit.

Un **ornement** se trace parce qu'il doit se redimensionner et suivre les jetons de la palette. Une
**illustration** ne le peut pas : une carte du monde peinte ne se trace pas, elle se prend ou elle
n'existe pas. Les mêmes garde-fous s'appliquent — région déclarée, rendu de région (`EX-CNT-022`),
manifeste recoupé en intégration continue avec les fichiers **et** avec le code qui les nomme,
repli si l'image est absente (`EX-NFR-040`).

Trois décisions valaient d'être tranchées plutôt que découvertes :

- **Du JPEG**, seul du dépôt. Le PNG de cette carte pèse 4,4 Mo, son JPEG 0,7, pour une différence
  que personne ne voit — surtout sous un voile. Le poids du dépôt est un sujet du corpus depuis le
  début (`EX-CNT-023`) ; tenir ce discours et livrer 4,4 Mo aurait été le contraire.
- **96 ppp, et pas plus.** C'est un fond, jamais lu : ses toponymes sont de la décoration. Le jour
  où la carte du monde devient un écran qu'on consulte (`LOT-42`), ce lot-là extraira la sienne.
- **Le filigrane d'achat est recadré, pas effacé.** L'effacer demanderait de repeindre ce qu'il
  recouvre, c'est-à-dire d'inventer des pixels. La région déclarée s'arrête avant lui.

Et le voile de lisibilité change de nature : le décor tracé se voilait par **bandes superposées**,
ce qui est correct sur des aplats. Sur une carte peinte, chaque palier se lit comme une bande
claire en travers du relief — le procédé se voit, et c'est tout ce qu'on voit. Un **dégradé
continu** le remplace, et seulement à gauche, là où vivent les entrées : assombrir l'image entière
rendrait le texte lisible en effaçant ce qu'on vient de mettre derrière.

### Ce qui a été retiré du code

**Deux écrans** (`LevelSelectScreen`, `LevelCompleteScreen`, leurs `.ui` et leurs deux maquettes),
**deux modèles** (`hmi::Progression`, `core::LevelSequence`) et **un bilan de partie**
(`hmi::LevelRunStats`, qui comptait le temps, les morts et les sauts). Environ **1 300 lignes**.

Les retirer était le seul choix cohérent : ce sont les mises en œuvre des exigences retirées. Les
garder aurait laissé du code que plus aucune exigence ne justifie — et personne, dans six mois,
n'aurait su dire s'il attendait un consommateur ou s'il n'en avait plus.

La machine à états y perd deux écrans et six événements ; le viewport de jeu perd sa **liste** de
niveaux au profit d'une **carte** unique.

### Les seize exigences : retirées pour douze, refondues pour trois

La feuille de route demandait de retirer seize exigences. Trois d'entre elles avaient un objet
**au-delà** du niveau discret, et les retirer aurait laissé leur mise en œuvre orpheline — le
défaut symétrique de celui qu'on corrigeait :

| Exigence | Sort | Pourquoi |
|---|---|---|
| `EX-GP-040` | refondue | Le jeu a toujours des états. Elle en perd deux, `NiveauTermine` et `LevelSelect`. |
| `EX-IHM-003` | refondue | L'ATH reste dû ; c'est la **liste** de son contenu qui était celle d'un platformer. Elle garde son critère — *ce dont le joueur a besoin pour décider* — et cesse d'énumérer. |
| `EX-IHM-004` | refondue | L'écran de **pause** reste ; seule la moitié « écran de fin de niveau » tombe. |

Les douze autres sont **retirées, pas supprimées** : leurs ancres restent, dans une section
« Exigences retirées » de leur spécification, avec le texte d'origine et le motif du retrait. C'est
le patron déjà suivi pour les décors-sprites, et il a une raison mécanique autant que morale —
une vingtaine de lots hérités s'y réfèrent, et supprimer une ancre casserait leurs renvois sans
rien apprendre à personne.

### Le vocabulaire : « niveau » devient « carte », partout

Les deux catalogues passent de « niveau »/« tableau » à « carte » — y compris côté **éditeur**, où
la famille de clés `level.*` devient `map.*`. L'éditeur n'édite pas des niveaux : il édite les
cartes du monde, et il n'y a pas deux vocabulaires.

Les deux catalogues restent **synchrones**, 376 clés de chaque côté — un test les compare dans les
deux sens, une clé oubliée d'un seul côté ne se verrait sinon qu'en changeant de langue.

L'événement sonore suit : `LevelCompleted` devient `ExitReached`, `victoire_tableau` devient
`sortie_atteinte`, et `SequenceCompleted` disparaît avec son bruitage. Un identifiant qui nomme une
mécanique retirée est un piège pour le prochain lecteur.

## Ce que le lot ne fait pas

**Il ne rend pas le jeu jouable.** « Nouvelle partie » ouvre `demo-deplacement.json`, et ce fichier
**n'existe pas** : le [LOT-01](@ref lot-01) a purgé les niveaux du jeu de plateforme et aucun lot
n'en a livré depuis. Le constat est antérieur à ce lot — la « Nouvelle partie » d'avant chargeait
une séquence tout aussi absente — mais il devient visible, et il vaut mieux l'écrire que le laisser
découvrir : la carte de départ arrivera avec le contenu du `LOT-27`, sa destination avec le graphe
du `LOT-09`.

**Il ne remplace pas l'ATH.** `hmi::GameHud` affiche encore des budgets de sauts et de dashs. C'est
le contenu que `EX-IHM-003` n'énumère plus, et son remplacement appartient à l'IHM de combat du
`LOT-24`.

**Il ne touche pas au format de carte.** La tuile de sortie reste dans le format et dans les
cartes ; seul son **effet** change — elle ne termine plus rien, elle ramène au menu, en attendant
la transition du `LOT-09`.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1007/1007, `clang-format`, les sept lints, cahier de test et Doxygen verts ; tous les critères d'acceptation sont cochés dans l'epic d'origine.

Exigence ajoutée : [`EX-IHM-076`](@ref EX-IHM-076) (illustrations extraites du corpus). Exigences **refondues** : [`EX-GP-040`](@ref EX-GP-040) (états de jeu), [`EX-IHM-003`](@ref EX-IHM-003) (affichage tête haute), [`EX-IHM-004`](@ref EX-IHM-004) (écran de pause). Exigences **retirées** : [`EX-GP-030`](@ref EX-GP-030), [`EX-GP-031`](@ref EX-GP-031), [`EX-GP-032`](@ref EX-GP-032), [`EX-LVL-010`](@ref EX-LVL-010) → [`EX-LVL-015`](@ref EX-LVL-015), [`EX-IHM-005`](@ref EX-IHM-005), [`EX-NFR-021`](@ref EX-NFR-021).
