+++
id = "LOT-18"
titre = "Bascule exploration ↔ combat"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "Une rencontre se déclenche depuis l'exploration, gèle le monde, monte ses combattants et rend l'exploration sans que le joueur perde quoi que ce soit."
prerequis = ["LOT-05", "LOT-10", "LOT-13"]
livrables = [
  "`core::ExplorationSnapshot` : position, orientation et caméra mises de côté.",
  "`core::EncounterRun`, `core::beginEncounter` et `core::endEncounter` : l'aller-retour pur, sans widget.",
  "`hmi::CombatMode`, qui ordonne les passes et gèle déplacement libre, mécanismes et issue de niveau.",
  "`core::WorldFlags` et `core::keyForEntity` : l'ennemi vaincu est un drapeau de monde ; `encounterAlreadyCleared`.",
  "`core::placeCombatants` : positions relatives au déclencheur.",
  "`Rpg/encounters/nuee-de-rats.json`, rencontre provisoire de démonstration.",
]
criteres = [
  "Aller-retour exploration → combat → exploration restituant l'état, PV mis à jour.",
  "Un ennemi vaincu ne réapparaît pas, y compris après avoir quitté et rechargé la carte — c'est un drapeau de monde, qui survit à la destruction de l'entité.",
  "Une fuite ramène à l'exploration sans que l'ennemi soit marqué vaincu.",
  "Testable headless : montage et démontage ne demandent ni fenêtre ni GPU.",
  "`ctest` : 1047/1047 (1034 avant, plus les treize cas de ce lot).",
]
+++

## Pourquoi

Déclencher une rencontre depuis l'exploration, geler le monde, monter les combattants, et en
revenir — **sans que le joueur perde quoi que ce soit au passage**.

## Périmètre

Il livre la **bascule**, pas le combat. Ni initiative, ni tour actif, ni résolution d'action : ce
sont les [LOT-19](LOT-19-grille-tactique.md) et [LOT-20](LOT-20-initiative-tour-par-tour.md). Séparer les deux n'est pas un découpage
administratif — un aller-retour qui perd la position du personnage est un défaut qu'on ne voit plus
une fois qu'il y a des tours à jouer par-dessus, et qu'on n'aurait jamais isolé.

## Conception

### L'aller-retour est PUR, et c'est ce qui le rend vérifiable

Le critère d'acceptation demandait que le montage et le démontage d'une rencontre se testent
**headless**. Ils le sont, parce que rien de ce qui décide n'est dans un widget :

- `core::ExplorationSnapshot` — ce qu'on met de côté ;
- `core::EncounterRun` — la rencontre engagée, sans widget, sans entité, sans pointeur de monde ;
- `core::beginEncounter` / `core::endEncounter` — deux fonctions, et toute la règle.

`hmi::CombatMode`, lui, ne fait qu'**ordonner des passes**. Il ne retient rien de la rencontre : la
tentation était de lui faire porter l'état, et l'aller-retour serait alors devenu invérifiable sans
fenêtre.

### Ce que l'instantané porte, et ce qu'il ne porte pas

Il porte la position du personnage, son **orientation** et la caméra. Sans mémoire, le personnage
reviendrait de son combat ailleurs qu'il ne l'avait quitté — un pas de côté à chaque rencontre,
invisible une fois, gênant au dixième — et regardant une direction par défaut (`EX-EXP-004`).

Il ne porte **pas les points de vie**, et c'est délibéré. Le critère dit « restituer exactement
l'état d'exploration, *aux PV près* » : les points de vie sont précisément ce que le combat a
changé, et les remettre à leur valeur d'avant annulerait le combat. Les inclure « pour être
complet » aurait été le bogue, pas la prudence.

Il ne porte pas non plus les ennemis vaincus : cela ne se restitue pas, cela s'**acquiert**.

### Un ennemi vaincu est un drapeau de monde, pas un booléen

C'est le piège que le [LOT-10](LOT-10-entites-de-carte.md) avait déjà nommé pour les coffres, et il se pose
identiquement ici : l'entité de l'ennemi est détruite et recréée depuis la couche `objects` au
rechargement de la carte, et un booléen porté par elle disparaîtrait avec elle. L'ennemi
réapparaîtrait à chaque passage — un défaut qui ne casse rien, ne lève aucune alerte, et se confond
avec une carte peuplée.

`core::WorldFlags` porte donc le fait, sous une clé **fabriquée** par `core::keyForEntity` — jamais
écrite à la main, sinon deux ennemis finiraient par la partager. Le test le vérifie dans les deux
sens : deux cases d'une même carte, et la même case sur deux cartes.

#### Seule une victoire l'acquiert

Une fuite ramène à l'exploration **sans** marquer l'ennemi vaincu, et une défaite non plus. Poser le
drapeau à la sortie, quelle qu'elle soit, aurait fait de la fuite un moyen de nettoyer une carte —
et le défaut ne se serait pas vu : la carte se serait vidée, ce qui ressemble à une progression.

### Deux natures de déclencheur, distinguées par la donnée

Une entité de type `encounter` nomme sa rencontre (`encounterId`). Ce qui la distingue n'est pas son
type mais une propriété :

- un **ennemi posé** — il se combat une fois, et sa clé le fait disparaître pour de bon ;
- une **zone** (`respawns: true`) — elle se redéclenche, et n'a donc pas de clé.

Confondre les deux se paierait dans les deux sens : un ennemi sans clé réapparaîtrait à chaque
passage, et une zone avec clé s'éteindrait au premier combat gagné. C'est pourquoi
`encounterAlreadyCleared` répond **non** pour une clé vide : l'inverse aurait désactivé toutes les
zones du jeu dès le premier combat.

### Une rencontre dit QUI, jamais OÙ

Les positions des combattants sont **relatives au déclencheur**. Une rencontre est écrite une fois
et jouée partout : trois rats « un pas devant » gardent leur formation quel que soit l'endroit,
alors que des coordonnées absolues les feraient apparaître au même endroit à chaque fois — ou hors
de la carte.

`core::placeCombatants` rend la liste des cases **voulues**, sans consulter ni carte ni collision :
c'est au montage de refuser celles qui tombent dans un mur, et il ne peut le faire que s'il les
reçoit toutes.

### Ce que le mode combat gèle, et pourquoi

Trois passes de l'exploration disparaissent, chacune pour une raison précise :

| Passe retirée | Pourquoi |
|---|---|
| `moveCharacter` | le déplacement suit le **budget du tour** ([LOT-19](LOT-19-grille-tactique.md)), pas l'intention libre du joueur ; la laisser donnerait un combat où l'on marche pendant le tour d'un autre |
| `updateMechanisms` | une plaque de pression qui s'enfoncerait au milieu d'un tour ferait dépendre le combat d'une règle qu'aucun livre ne décrit |
| `evaluateOutcome` | tomber à zéro point de vie est une issue du **combat**, pas du niveau ; l'évaluer ici rechargerait le niveau au lieu d'ouvrir l'agonie ([LOT-72](../../../../vision/archives/feuille-de-route-jeu.md#lot-72)) |

Ce qui reste tourne parce que le combat demeure une **scène** : particules et secousse d'écran
finissent ce qu'elles ont commencé, les animations continuent — un combattant immobile respire — et
la caméra suit. Un test compare la séquence réellement appelée à celle que le mode annonce, et
vérifie qu'aucune passe gelée n'y figure.

### Le personnage de démonstration

`Rpg/encounters/nuee-de-rats.json` : trois bêtes du SRD ([LOT-33](LOT-33-bestiaire-de-base.md)), seules créatures
livrées à ce jour. Déclarée **provisoire** avec son critère de retrait — elle disparaît quand le
contenu du [LOT-27](../../../../vision/archives/feuille-de-route-jeu.md#lot-27) fournira ses propres rencontres.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔
2. ✔
3. ✔ (la défaite non plus)
4. ✔
5. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1047/1047, `clang-format`, les huit lints, cahier de test et Doxygen verts.

Aucune exigence ajoutée : `EX-CBT-001` couvre ce lot, et c'est son premier vrai emploi.
