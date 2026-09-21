+++
id = "LOT-15"
titre = "PNJ et dialogues"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "L"
resume = "Parler à un PNJ par un arbre de dialogue scripté, avec choix, conditions, jets de compétence et refus faute de langue commune."
prerequis = ["LOT-05", "LOT-10", "LOT-12"]
livrables = [
  "`Source/Core/Rpg/Dialogue.{h,cpp}` : `core::DialogueGraph`, `core::readDialogue`, `core::validateDialogueReferences`, `core::loadDialogues`.",
  "`core::DialogueRunner`, machine à états pure, `core::DialogueListener` et `core::CharacterListener`.",
  "`hmi::DialogueMode`, le troisième mode de jeu, qui gèle le monde.",
  "L'écran branché : `hmi::dialogueScreenValues`, `hmi::DialogueModel` et le jumeau `Dialogue.qml`.",
  "`Source/Elements/World/dialogues/heraut-colisee.json` (quatorze nœuds), `dialogue.schema.json`, la famille `dialogues` de `check_rpg_data.py`, les textes dans `fr.lang` et `en.lang`.",
  "`core::loadDifficultyScale`, `core::CharacterSheet::languages` et la famille `npc` des interactifs.",
]
criteres = [
  "Un dialogue de dix nœuds, deux conditions et un jet de Persuasion se parcourt en headless.",
  "Un graphe mal formé est rejeté au chargement avec un message exploitable.",
  "L'exploration est gelée pendant le dialogue.",
  "Traduction fr/en complète, aucun texte en dur.",
  "`ctest` : 1107/1107 (1090 avant, plus les dix-sept cas de ce lot).",
]
+++

## Pourquoi

Parler à un PNJ par un arbre de dialogue scripté, avec choix, conditions et jets de compétence.

## Périmètre

- **Le graphe et son chargeur** (`Source/Core/Rpg/Dialogue.{h,cpp}`) : `core::DialogueGraph`,
  cinq natures de nœud — réplique, condition sur drapeau, action, jet de compétence, fin —,
  `core::readDialogue` qui **valide** et `core::validateDialogueReferences` qui confronte le graphe
  aux catalogues ; `core::loadDialogues` pour un dossier.
- **Le runner** : `core::DialogueRunner`, machine à états pure, et `core::DialogueListener`, ce
  que le runner sait de celui qui parle — implémenté par `core::CharacterListener` sur une fiche.
- **Le troisième mode de jeu** : `hmi::DialogueMode` (`Source/HMI/Game/`), à côté
  d'`hmi::ExplorationMode` et d'`hmi::CombatMode`.
- **L'écran branché** : `hmi::dialogueScreenValues` (`Source/HMI/Presentation/DialogueScreen`),
  logique pure de ce qui s'affiche ; `hmi::DialogueModel` (`Source/HMI/Runtime/`), qui remplace
  `PendingData` dans le jumeau `Dialogue.qml` ; sa doublure pour l'atelier.
- **La donnée** : `Source/Elements/World/dialogues/heraut-colisee.json`, quatorze nœuds, son
  schéma `dialogue.schema.json`, la famille `dialogues` de `check_rpg_data.py`, et ses textes dans
  `fr.lang` et `en.lang`.
- **Ce qui manquait autour** : l'échelle des degrés de difficulté se lit enfin
  (`core::loadDifficultyScale`, `Core/Rpg/Check`), la fiche porte ses **langues**
  (`core::CharacterSheet::languages`), et la table des interactifs connaît la famille `npc`.

Il ne livre **pas** la conversation ouverte depuis la carte : l'interaction du [LOT-10](LOT-10-entites-de-carte.md)
ne tourne pas dans `hmi::GameSession`, et la surface de rendu Qt Quick n'affiche aucune scène d'où
parler à quelqu'un. `core::dialogueTriggerFor` lit le dialogue d'un PNJ posé sur une carte et
`hmi::DialogueMode` gèle le monde, mais aucune session ne bascule encore de l'un à l'autre ; l'écran
de dialogue ouvre le héraut du Colisée, écrit dans son jumeau. C'est la boucle du *vertical slice*
(`LOT-27`) qui en a besoin, et sa section le dit maintenant.

### Ce qui reste hors du lot, nommément

- **Ouvrir la conversation depuis la carte** : interaction → `core::dialogueTriggerFor` →
  `hmi::DialogueMode` et l'écran. Le `LOT-27` en a besoin et le dit.
- **Les quêtes elles-mêmes** et leur journal (`LOT-16`) : ce lot pose le drapeau de départ.
- **La persistance** des drapeaux et de ce qu'un PNJ donne (`LOT-17`).
- **Les portraits** des interlocuteurs : les 172 jetons détourés du corpus attendent leur clé
  d'asset.
- **L'attitude qui change par le jeu** (une réussite qui rend un PNJ amical *durablement*) : une
  réplique déclare l'attitude qu'elle montre ; la retenir est un drapeau, et la règle d'attitude du
  *Guide du Maître* (qui module les degrés de difficulté) est à écrire avec le peuplement civil
  (`LOT-82`).
- **L'éditeur de dialogues** : le panneau de propriétés du [LOT-11](LOT-11-editeur-multicouches.md) proposera les
  dialogues du catalogue.

## Conception

### Le graphe ne porte aucun texte

Chaque réplique, chaque réponse et le nom de l'interlocuteur ont une clé de traduction
**fabriquée** depuis les identifiants : `dialogue.<dialogue>.<nœud>` pour une réplique,
`dialogue.<dialogue>.<nœud>.<réponse>` pour une réponse, `dialogue.<dialogue>.speaker` pour le nom.
Un texte écrit dans le JSON serait du français en dur ; une clé écrite à la main pourrait être
fausse et ne se verrait qu'à l'écran. Fabriquée, elle existe ou manque — et
`core::dialogueTextKeys` liste toutes celles qu'un graphe réclame, qu'un test cherche dans les deux
catalogues. C'est ce test qui tient le critère « traduction fr/en complète ».

Les clés vivent dans `fr.lang` et `en.lang`, le catalogue que lit `hmi::ruleLabel`, et non dans le
`.ts` de Qt : `qsTr` exige une chaîne littérale, et ces clés sont calculées.

### Ce que le chargement refuse

Un graphe mal formé n'est pas un graphe : `core::DialogueLoad::graph` reste vide dès la première
erreur, et **toutes** les erreurs sont listées d'un coup, chacune nommant son fichier et son nœud.
Un test en éprouve quatorze :

| Faute | Pourquoi elle ne passe pas |
|---|---|
| **Nœud cible inconnu** (entrée, réponse, suite, branche) | la conversation s'arrêterait au milieu d'une phrase |
| **Choix vide** : `choices: []`, réponse sans identifiant | le joueur resterait devant une réplique sans rien à répondre |
| **Toutes les réponses conditionnelles** | des drapeaux qui les masqueraient toutes produiraient le choix vide *en jeu*, dans l'état de monde précis qui le déclenche |
| **Cycle non intentionnel** | voir ci-dessous |
| **Orphelin** | un nœud que rien n'atteint est presque toujours une cible mal orthographiée ailleurs |
| **Impasse** | un nœud d'où aucune fin n'est atteignable enferme le joueur |
| **Difficulté chiffrée** | `EX-REG-021` : un contenu écrit « moyenne », jamais « 15 » |
| Nœud en double, nature inconnue, graphe sans fin, PNJ sans langue | formes invalides |

**Qu'est-ce qu'un cycle non intentionnel.** Les dialogues bouclent, et c'est voulu : « Autre
chose ? » ramène au menu des questions. La règle retenue : **une boucle doit passer par une
réplique à réponses**, le seul nœud où le joueur décide. Une boucle de conditions et d'actions
tournerait sans fin dans un seul appel ; une boucle de répliques sans réponse enfermerait le joueur
dans un monologue. Un test charge un « hub » et le joue trois fois, un autre refuse les deux boucles.

Le schéma JSON dit la **forme** — un test à la main lui a fait refuser une réplique à la fois à
réponses et à suite, une difficulté chiffrée, une nature inconnue, un choix vide, une action mal
formée —, et ce qu'il ne peut pas dire (les cibles, les boucles, les impasses) est l'affaire du
chargeur. Les **références** — compétence, degré de difficulté, objet, langue — sont vérifiées à
part (`validateDialogueReferences`), pour qu'un test de graphe n'ait pas à charger les deux cents
objets du jeu ; la vue-modèle écarte un dialogue dont une référence manque, en le journalisant.

### Le runner : pur, et il ne s'arrête que sur une réplique

`core::DialogueRunner` consomme un graphe, des drapeaux (`core::WorldFlags`), un interlocuteur et
une suite aléatoire ; il produit une réplique courante et ses réponses. Conditions, actions et jets
s'enchaînent **dans le même appel** jusqu'à la prochaine réplique ou la fin : l'écran ne voit jamais
un nœud automatique, et une conversation n'a que deux états observables — une réplique qui attend,
ou une fin (plus le refus).

- **Une réplique sans réponses** a une suite (`next`) et propose une réponse implicite, `continue`,
  traduite une fois pour tous les dialogues (`dialogue.continue`).
- **La condition d'une réponse est réévaluée au geste**, pas seulement à l'affichage : l'écran ne
  peut pas faire passer une réponse que la donnée n'offre plus. Le défaut inverse a été réintroduit
  à la main : le test de l'échec de Persuasion échoue, et lui seul.
- **Démarrer une quête** pose `core::questStartedFlag(id)` : les quêtes lisent des drapeaux
  (`LOT-16`, note de conception), et le runner n'a besoin d'aucun objet quête.
- **Le jet** passe par `core::rollCheck` contre le degré lu dans `rules/difficulty.json` ; ses
  modificateurs viennent de l'interlocuteur, **détaillés** — Charisme, puis maîtrise — pour que
  `CheckResult::describe` restitue d'où vient le total. Un degré inconnu (graphe construit à la main)
  prend la branche d'échec et le journal le dit : un jet contre 0 réussirait toujours.
- **Rejouable.** La suite aléatoire est fournie, jamais créée : à graine égale, mêmes réponses,
  mêmes jets, même journal — vérifié. Le journal nomme chaque nœud traversé, chaque effet, chaque
  jet détaillé.

### Refusé faute de langue commune

`EX-RPG-042` : un dialogue doit pouvoir être refusé, sans quoi treize espèces parlent partout la
même langue. Un dialogue déclare les langues de son PNJ (`speaker.languages`, au moins une) ;
`start` refuse la conversation si l'interlocuteur n'en parle aucune — **avant** tout nœud, pour
qu'un PNJ qu'on ne comprend pas ne pose aucun drapeau. L'écran montre alors le refus et une seule
réponse, « Quitter ».

La fiche n'avait pas de langues. `core::CharacterSheet::languages` recopie celles de l'espèce à la
construction et y ajoute celles que la fiche **choisit** — un historique en accorde un nombre, pas
une liste ; le schéma de fiche gagne `languages`, et le personnage de démonstration, demi-elfe,
parle le commun et l'elfique de son espèce, et le nain qu'il a choisi.

### Le monde gelé

`hmi::DialogueMode` gèle les mêmes passes que le combat : ni déplacement libre — l'intention choisit
une réponse, elle ne déplace personne —, ni mécanismes, ni événements, ni issue de niveau. La scène
derrière le parchemin continue de respirer (particules, animations, caméra). Un test avance le mode
de six cents pas avec une intention qui pousse en diagonale et presse « interagir » : chaque pas
joue exactement l'ordre annoncé, et aucune passe d'exploration n'est appelée.

Deux modes aux passes égales plutôt qu'un seul partagé : un combat avancera au tour, une
conversation voudra cadrer son interlocuteur, et séparer ensuite un mode partagé coûterait chaque
`if` ajouté entre-temps.

### L'écran

La mise en page du [LOT-68](LOT-68-chassis-ecrans-rpg.md), restylée au [LOT-87](LOT-87-charte-v2.md), est conservée. Deux
ajouts au formulaire, faute desquels il ne pouvait pas servir : `replyChosen(rowId)`, émis par un
clic sur une réponse, et `checkOutcome`, la restitution du jet au-dessus de la réplique qui en
découle. `LedgerList` gagne `interactive` et `rowActivated` — faux par défaut, les autres registres
ne changent pas.

Ce qui s'affiche se décide dans `hmi::dialogueScreenValues`, testé sans fenêtre : une réponse qui
mène à un jet l'**annonce** (« [Persuasion] »), comme une table l'annonce avant qu'on choisisse ; le
jet se restitue sur la réplique suivante seulement (« Persuasion : 17 contre 15 — réussite », gabarit
traduit) ; la conversation terminée, l'écran se referme. `1` à `9` choisissent la réponse de ce
rang, `Échap` quitte.

Deux échafaudages, écrits comme tels dans `hmi::DialogueModel` : les **drapeaux** vivent le temps du
processus — sans partie ni sauvegarde (`LOT-17`), c'est la seule façon de voir le héraut se
souvenir de vous en rouvrant l'écran — et l'interlocuteur est le personnage de démonstration,
rechargé à chaque ouverture : un objet donné ne survit pas à la fermeture. Le **portrait** reste
vide : aucun jeton du corpus n'est encore une clé d'asset servie.

### Le dialogue de démonstration

Le héraut du Colisée, provisoire et marqué comme tel (`status.provisoire`, retrait au `LOT-27`), est
à l'Arène du Futur parce que c'est la seule carte jouable. Quatorze nœuds : une condition de
première rencontre (qui oriente vers une présentation ou un « Encore vous »), une explication de la
Marque Héroïque qui revient au menu, une condition « déjà inscrit », une demande d'inscription dont
la réponse « convaincre » est conditionnelle, un **jet de Persuasion** de difficulté moyenne, et
deux suites — l'une démarre la quête `champion-du-colisee` et rend l'attitude amicale, l'autre pose
un drapeau d'échec qui retire la réponse « convaincre ». Ses répliques ne sont pas tirées du
Sourcebook.

## Relevé en chemin

- **`rules/difficulty.json` n'avait pas de lecteur.** `rollCheck` exigeait depuis le `LOT-12` qu'un
  seuil ne soit jamais un littéral, et personne ne lisait l'échelle : le dialogue est le premier
  contenu à jeter un d20 hors combat.
- **Les clés d'invite d'interaction** (`interaction.chest`, `interaction.sign`, et désormais
  `interaction.npc`) ne sont dans aucun catalogue : rien ne les affiche encore. Elles le seront avec
  l'interaction dans la session de jeu.
- **Avertissements de l'atelier** : `qmllint` signale, pour `LedgerList` et `DialogueForm`, des
  appels de signal dans un gestionnaire (`QtDesignStudio.FunctionsNotSupportedInQmlUi`), comme il
  le fait déjà pour `ArenaForm`. `check_qml_designer_compat.py` est vert.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔ Le héraut, quatorze nœuds dont deux conditions, une réponse conditionnelle et un jet de Persuasion : réussite puis seconde conversation orientée par les drapeaux, douze nœuds distincts traversés (`DialogueTest.LeDialogueDuHerautSeParcourtEnHeadless`) ; échec et réponse retirée (`UnEchecMeneALAutreSuiteEtFermeLaReponseConditionnelle`).
2. ✔ Quatorze fautes, chacune refusée avec son fichier, son nœud et son motif (`UnGrapheMalFormeEstRejeteAuChargement`).
3. ✔ Six cents pas en mode dialogue, aucune passe d'exploration (`ModeDeJeuTest.LeModeDialogueGeleLExploration`). Le mode n'est pas encore basculé depuis une session de jeu (voir plus haut).
4. ✔ Chaque clé fabriquée par chaque dialogue livré, et celles de l'écran, existent dans les deux catalogues (`LesDialoguesSontTraduitsEnFrancaisEtEnAnglais`) ; le graphe ne porte aucun texte.
5. ✔

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1107/1107 en Debug, `clang-format`, les lints, les schémas et le cahier de test verts ; capture de l'écran relue.

Exigences couvertes : `EX-VIS-003` (dialoguer avec un PNJ, à choix et conditions), `EX-RPG-042` (un dialogue refusé faute de langue commune), et `EX-REN-033` pour tout ce qui s'y lit. Aucune exigence ajoutée.
