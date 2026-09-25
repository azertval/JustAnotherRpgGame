# Bilan de la version 0.0.1 — démo basique

Écrit à la recette ([LOT-122](lots/LOT-122-recette-et-version-0-0-1.md)), le 25 septembre 2026,
comme la [cinquième règle de la trajectoire](../../../vision/trajectoire.md) le demande : ce qui a
coûté plus que prévu, et ce que la `0.0.2` doit en retenir. Les chiffres viennent des fiches de lot,
du `CHANGELOG.md` et de l'historique Git ; les jugements sont ceux de l'auteur, relus à la recette.

## En une page

| | |
|---|---|
| Ouverte | 20 septembre 2026 (`LOT-100`, PR #94) |
| Livrée | 25 septembre 2026, tag `v0.0.1` — **six jours** |
| Lots livrés | **24**, du `LOT-100` au `LOT-129` et `LOT-146`, dont 4 S, 12 M, 7 L, 1 XL |
| Lots sortis de la version | 7 vers la `0.0.3` par [D-25](../../../vision/decisions.md) (`LOT-106`, `107`, `110`, `111`, `113`, `114`, `115`) ; 1 vers la `0.0.2` par D-26 (`LOT-145`) |
| Décisions de planification prises en route | D-17 à D-27 (onze) ; Q-01, Q-03, Q-04, Q-09, Q-12, Q-13 tranchées |
| Critères de sortie | 4 sur 4 tenus (voir [ci-dessous](#les-critères-de-sortie)) |
| Ce qui reste dû | les noms des trois PNJ du marché (Q-05) ; les mannequins complets (`LOT-145`, `0.0.2`) ; les zones de la démo au standard final (`0.0.3`) |

La version a tenu son objet : **une démo qui se joue de bout en bout**, par trois fins, sur trois
cartes, avec un moteur de quête complet et un standard 2D HD écrit et outillé. Elle ne l'a tenu
qu'en **renonçant à produire ses zones** : c'est le fait central de ce bilan.

## Les critères de sortie

| Critère | Tenu par |
|---|---|
| Un joueur qui ne connaît pas le projet termine la démo par chacune de ses trois issues sans aide | L'auteur a joué les trois fins à l'écran ; `SystemGameTests` (étiquette `systeme`) rejoue chacune de « Nouvelle partie » à l'écran de fin, une graine par issue ; `IntegrationTests` joue la chaîne sans fenêtre. La lecture par un joueur **extérieur** n'a pas eu lieu : la recette « trois parties par trois personnes » du `LOT-122` s'est faite à une personne et trois tests — voir [ce que la `0.0.2` retient](#ce-que-la-002-doit-en-retenir). |
| Aucun asset pixel art dans `Source/Elements/Assets/` | `LOT-102` (table rase) ; `check_hd_assets.py` en CI ; la galerie de débug montre les kits HD |
| Les trois cartes de principe s'enchaînent, passent `--check`, se parcourent sans case inatteignable | `LOT-146` : `LevelEditor --check` vert, 0 avertissement ; l'atteignabilité suit les portails et le transfert du parvis |
| CI verte, installeur sur un poste vierge, tag `v0.0.1` | La CI est un contrôle requis de chaque PR ; `release.yml` décompresse et lance chaque archive avant de publier (`smoke_test_release.ps1`) ; le tag se pose sur le commit de fusion de la PR de recette |

## Ce qui a coûté plus que prévu

### 1. Les zones — le poste qui a fait plier la version

La `0.0.1` du 20 septembre voulait **trois lieux au standard du jeu final** : assets, cartes et PNJ
de Martpart, d'Arenarea et de l'Arena of Fate, soit huit lots de *world building* (`LOT-106` à
`LOT-111`, `LOT-113` à `LOT-115`). Un seul quartier a été produit, Arenarea :

- `LOT-108` (assets), taillé **XL**, a livré 1 491 pièces et a fait sortir les images de Git en
  fin de lot (kits publiés, `kits.lock.json`, `fetch_assets.py`) — un chantier d'infrastructure
  que rien n'annonçait dans la fiche ;
- `LOT-109` (carte), taillé **M**, a demandé un [audit de l'affichage d'un lieu](../../../standards/audit-affichage-lieu.md)
  et une scène composée une fois (`hmi::StaticWorldScene`) : la carte de 128 × 88 mettait 82 s à
  paraître en Debug ;
- **validés le 24, jugés à refaire le 25** ([R-13](../../../vision/risques.md)) : loin du standard
  voulu pour le jeu final. Ils restent livrés, la démo n'en dépend plus, la reprise est le
  `LOT-147` de la `0.0.3`.

La décision **D-25** en a tiré la conséquence : la démo se joue sur des **cartes de principe**
(`LOT-146`, un seul lot M, dessiné par `--apply` en une journée), et les zones partent à la `0.0.3`
avec les six autres quartiers. Ce que coûte une zone au standard final **reste inconnu** — c'est le
chiffre dont toute la trajectoire dépend, et Arenarea n'en donne qu'une borne basse (un lot XL et
un lot M pour un résultat à refaire).

### 2. Le moteur : quatre lots L là où l'on en attendait moins

| Lot | Taille | Ce qui a grossi |
|---|---|---|
| `LOT-118` combat sur la carte | L | la **file des mouvements** (`hmi::CombatCueTrack`) : une session instantanée montrait des combattants qui se téléportent, il a fallu rejouer chaque fait à la vitesse du monde ; six bandes d'animation préchargées ; le mannequin SE-v1 installé pour que l'adversaire se voie |
| `LOT-129` étages et toits | L | prévu comme un lot de moteur, il a produit **598 modules de toiture** (droits, L, T, X) et deux scripts de projection — sept commits, le plus gros lot de la version en volume |
| `LOT-116` quêtes et drapeaux | L | la position d'une erreur de sens dans un JSON (`core::positionOfPointer`), la présence en trois propriétés plates, la révision des drapeaux sans signal |
| `LOT-124` arborescence par niveaux | L | les **figurines par niveau** et les préfabriqués par niveau, demandés en cours de lot |

Les lots **S** et la plupart des **M** ont tenu leur taille (`LOT-117`, `LOT-119`, `LOT-121`,
`LOT-127` ; `LOT-103`, `LOT-104`, `LOT-120`, `LOT-146`). Le `LOT-123` a fait trois cartes d'essai au
lieu de deux, et le `LOT-146` a redessiné Martpart : la maquette du `LOT-127` était partie à la
table rase.

### 3. Ce qui a été fait deux fois

- **Arenarea** : produit (`LOT-108`, `LOT-109`), puis remplacé par sa carte de principe, puis à
  reprendre (`LOT-147`).
- **Le plan de la Capitale** : supprimé à la table rase (`LOT-102`), provisoire à deux quartiers
  au `LOT-120`, complet au `LOT-121`.
- **La carte maquette de Martpart** : dessinée pour la recette de l'éditeur (`LOT-127`), perdue à
  la table rase, redessinée par gestes (`LOT-146`).

### 4. Le numéro de version

`CMakeLists.txt` portait `0.1.0` — le **référentiel** de l'Empire central, pas la version en
cours. Le `LOT-122` le ramène à `0.0.1`. La leçon est écrite dans `CONTRIBUTING.md` : le numéro se
bumpe à la recette, et il désigne la version que l'on tague, jamais le jalon qu'elle sert.

### 5. La planification elle-même

Onze décisions (D-17 à D-27) et six questions tranchées en six jours : la moitié des décisions
de la refonte ont été prises **pendant** la version. Trois d'entre elles ont changé son périmètre
(D-21 : un niveau est une carte ; D-22 : une carte se maquette avant de s'habiller ; D-25 : les
cartes de principe). Ce n'est pas une dérive — chacune est écrite, datée, motivée — mais la
version a été **replanifiée le cinquième jour**. Le garde-fou R-10 a tenu : les fiches sont
courtes, le lint est vert, le site suit.

## Ce qui a marché, et se garde

- **La maquette avant l'habillage** (D-22, `LOT-128`) : la démo entière s'est jouée et testée sur
  des cartes sans une pièce, pendant que le standard et les assets se produisaient à côté. C'est
  ce qui a permis D-25 sans perdre la version.
- **La recette par les tests système** : les trois fins de la démo sont trois `SystemGameTests`,
  graine fixée, de « Nouvelle partie » à l'écran de fin. Une régression de la démo est rouge en
  CI, pas découverte à la souris.
- **L'équilibrage par simulation** : cent graines pour le combat (le héros l'emporte 60 à 70
  fois), la Persuasion à DD 15 mesurée à une chance sur quatre. Les valeurs sont dans les tests, pas
  dans une opinion.
- **« Livré » exige la main** ([définition](../../../standards/definition-de-livre.md)) : la dette
  de huit lots d'éditeur « vérification à la souris due » s'est réglée une fois, au `LOT-127`, 48
  gestes, 0 anomalie.
- **Les kits d'assets hors de Git**, verrouillés et publiés : le dépôt reste léger, la CI les
  installe, D-23 (pas de budget de poids) devient tenable.
- **Le menu de développement F9** et `--data=` : ouvrir un dialogue, une fin, une carte à une case
  sans rejouer la démo — c'est ce qui a rendu la recette rapide.

## Ce que la 0.0.2 doit en retenir

1. **Ne pas produire de zone dans une version de système.** La `0.0.2` se joue sur les trois
   cartes de la démo (`LOT-142`, périmètre) : cette règle est déjà écrite, elle se tient. Le coût
   d'une zone se mesure à la `0.0.3`, et nulle part avant.
2. **Les mannequins d'abord** (`LOT-145`, repris par D-26) : le combat de groupe et les
   rencontres d'arène mettront sur la carte des créatures sans figurine ; sans les trois
   silhouettes dans les quatre orientations, chaque lot de combat rejouera le problème du
   `LOT-118`. C'est le premier lot de la `0.0.2` après le socle des fiches.
3. **Une taille L n'est pas une taille XL manquée.** Les quatre lots L du moteur ont grossi sur
   un poste chacun, identifiable à la fiche (animation, production, diagnostic, demande en cours
   de lot). Écrire dans la fiche le poste qui **peut** grossir — et s'y tenir ou redécouper — coûte
   moins qu'un lot qui double.
4. **Une demande en cours de lot devient un lot ou une ligne de fiche**, jamais un ajout tacite :
   les figurines par niveau (`LOT-124`), les jonctions de toits (`LOT-129`), la sortie des images
   de Git (`LOT-108`) ont chacune ajouté une journée sans que le planning le voie venir.
5. **La recette a besoin d'un joueur extérieur.** La `0.0.1` s'est recettée à une personne et
   trois tests ; le critère « un joueur qui ne connaît pas le projet » n'a donc été tenu que par
   l'auteur. Le `LOT-142` doit prévoir la séance, et le lieu (une archive `debug-latest` suffit).
6. **Le `CHANGELOG` se ferme par version, et une section tient sous 125 000 caractères** : la
   limite des notes de release GitHub. La section `0.0.1` en fait 62 000 pour six jours — la
   `0.0.2` doit y penser avant que le tag échoue, en gardant les entrées au niveau du lot et non
   du commit.
7. **Q-05 se tranche tôt** : les trois PNJ du marché n'ont toujours pas de nom, et leurs textes
   les désignent par leur rôle. Un lot de la `0.0.2` qui touche les dialogues (le jet de groupe,
   `LOT-138`) les nommera au passage.
8. **La `CombatCueTrack` est faite pour un héros** : le combat de groupe (`LOT-139`) rejouera les
   mouvements de quatre personnages joués et de plusieurs adversaires ; la file, ses attentes et
   `Entrée` qui saute l'animation sont à relire à quatre, pas à étendre à l'aveugle.

## Ce que l'audit du code a trouvé

La recette a relu `Source/Core`, `Source/HMI` + `App` + `Ui`, l'outillage Python et la
documentation, à la recherche du code mort, du code sans documentation et des résidus du Colisée et
du pixel art. `Core` est sain : aucun `TODO`, aucun `#if 0`, aucun code commenté, `clang++ -Wall
-Wextra -Wunused-*` muet ; sept symboles sans appelant, six includes inutiles et six exemples de
chemins périmés ont été corrigés, et la convention « Doxygen dans le `.h`, `//` dans le `.cpp` »
est rétablie dans 27 `.cpp`. Côté `HMI`, le retrait de l'écran du Colisée (25 septembre) avait laissé
sa chaîne de rendu — `ArenaSceneRenderer`, `ArenaSceneComposer`, `ArenaAppearanceCatalog`,
`ArenaAnimationDriver`, 2 000 lignes compilées dans les deux exécutables, exercées par leurs
seuls tests —, 22 traductions orphelines, une branche de la galerie qui lisait un dossier disparu,
un alias vers une carte supprimée, des signaux et des propriétés que plus aucun écran ne lisait ; le
tout est retiré ou réécrit dans la PR de recette. L'outillage Python n'a aucun script orphelin ;
quatre symboles morts, deux chemins cassants (le cahier des assets d'interface déplacé au planning,
un écran de capture retiré), huit PNG de sortie de test commités à la racine et une dizaine de
commentaires d'un autre âge sont corrigés. Q-09 est tranchée par D-27 : le lint de l'ancienne
feuille de route était déjà parti, les archives restent.

Ce que l'audit **laisse à l'auteur**, parce que la réponse est un choix de produit et non un
constat :

| Constat | Ce que ça touche | Options |
|---|---|---|
| `hmi::CombatModel::preview` et ses 37 chaînes traduites sont calculés pour personne : le HUD de combat ne montre plus la prévisualisation d'un geste (coût, jet à atteindre) que le Colisée affichait | l'interface de combat de la `0.0.2` (`LOT-140`) | la réafficher dans le HUD, ou retirer la propriété et ses chaînes |
| `WorldModel` émet `portalLocked`, `portalBroken`, `portalSealed` et rien ne les écoute : un portail fermé ne dit rien au joueur | l'exploration ; la démo a deux portails condamnés | un message dans le HUD, ou retirer les signaux |
| `ScreenRouter::nextRpgScreen` / `previousRpgScreen` (`EX-IHM-090`, le cycle des écrans aux gâchettes) ne sont plus appelés : LB/RB font autre chose dans la carte et le combat | l'exigence `EX-IHM-090` | recâbler, ou réécrire l'exigence et retirer les invocables |
| La table `hmi::RpgScreens` (515 lignes, `pausesGame`, `EX-IHM-091`) n'est plus lue que par ses tests ; le gel réel se fait dans `GameView.qml` | l'architecture des écrans, héritée de l'ère Widgets | réduire à l'énumération, ou brancher `pausesGame` sur le gel |
| L'API clavier/souris d'`hmi::InputState` n'est plus alimentée par aucune fenêtre depuis Qt Quick ; seule la manette passe encore par elle | les entrées | l'amincir au pad |
| `core::Arena`, `ArenaCatalog`, `loadArenas` chargent `World/arena/`, un dossier que la table rase a supprimé : seules les racines d'essai en ont | le combat d'arène de la `0.0.2` (six rencontres, `LOT-142`) | la donnée revient avec les rencontres d'arène, ou le catalogue part |
| Quatre modules de `Core` ne sont exercés que par les tests : `AreaOfEffect` (sorts), `Multiclassing`, `ScopedLogLevel`, le composant `RpgActor` ; et une centaine de fonctions publiques (drapeaux de dégâts, ECS, atlas des régions, montée de niveau) n'ont pas d'appelant hors tests | des briques de règles livrées en avance par la fondation | les garder pour la `0.0.2` (sorts, groupe) et la `0.3.0`, comme prévu ; rien ne presse |
| Six dialogues du Colisée (`heraut-colisee`, `portier-colisee`, `parieuse-tribunes`, `medecin-vestiaire`, `vieux-gladiateur`, `sentinelle-ironhand`) n'ont plus de carte qui les porte ; deux servent de données de test | le contenu ; l'Arena of Fate définitive de la `0.0.3` en réemploiera peut-être | les garder comme réserve d'écriture, ou les retirer avec leurs textes |

## Ce que la version laisse aux suivantes

| À | Quoi | Où c'est écrit |
|---|---|---|
| `0.0.2` | Les mannequins (`LOT-145`) ; les noms des PNJ du marché (Q-05) | D-26 ; `LOT-138` |
| `0.0.3` | Les trois lieux de la démo au standard final, Arenarea repris (`LOT-147`), les PNJ en figurines, la sauvegarde des drapeaux | D-25 ; `versions.toml` |
| `0.0.2` → `0.0.3` | La décision de stockage des assets (Q-08) : 191 Mio publiés en `@1` pour un quartier et les communs | `LOT-156` |
| `0.2.0` | L'audio et les effets du `LOT-28` | `LOT-122`, périmètre |
