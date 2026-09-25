+++
id = "LOT-117"
titre = "Un jet de compétence dans un dialogue"
version = "0.0.1"
filiere = "moteur"
statut = "livré"
taille = "S"
resume = "Une réponse de dialogue peut demander un jet : le joueur voit la compétence, le DD, le dé, le résultat."
prerequis = ["LOT-100"]
livrables = [
  "Le graphe de dialogue accepte une réponse **à jet** : compétence, DD, branche de réussite, branche d'échec.",
  "L'interface montre le jet : « Persuasion · DD 18 », le d20 lancé, le total, l'issue.",
  "Le jet passe par le d20 du `LOT-12` : graine, journal, rejeu.",
]
criteres = [
  "À graine fixée, le même dialogue donne la même issue.",
  "Une réponse déjà tentée et ratée ne se propose plus (drapeau).",
  "Le contrôle des dialogues refuse une réponse à jet sans branche d'échec.",
]
+++

## Périmètre

Un jet **simple** : pas d'avantage circonstanciel, pas d'aide d'un allié, pas de jet de groupe.

## Décisions de réalisation

### D1 — Pas de nouveau format : la réponse à jet est une réponse qui mène à un nœud `check`

Le graphe du `LOT-15` avait déjà le nœud de jet (compétence, degré **nommé**, suites de réussite
et d'échec), et l'écran annonçait déjà la compétence de la réponse qui y mène. Une réponse
portant son jet en ligne aurait fait deux écritures pour une même chose. Le lot a donc complété
ce qui existait : la réponse annonce aussi le **seuil** (`core::AvailableChoice::checkDc`, lu dans
`rules/difficulty.json`), et le jet joué se montre par morceaux.

### D2 — Le jet se montre : « Persuasion · DD 15 », le d20, le calcul, l'issue

La réponse porte « [Persuasion · DD 15] » avant d'être choisie. Une fois jouée, la réplique qui
en découle porte un losange avec le d20 tiré — filet d'or si le jet réussit, grenat s'il échoue —,
ce qui était jeté, le calcul (« 12 + 4 = 16 ») et l'issue. `hmi::dialogueScreenValues` compose
ces morceaux (`checkTitle`, `checkDie`, `checkDetail`, `checkVerdict`, `checkSucceeded`) ; le
formulaire ne fait que les poser. Le seuil est **montré** au joueur, comme une table l'annonce ;
le contenu, lui, continue de le nommer (`EX-REG-021`).

### D3 — Un jet raté pose un drapeau fabriqué, et ne se relance jamais

`core::dialogueCheckFailedFlag` fabrique `dialogue/<dialogue>/<jet>/failed`, que le runner pose à
l'échec. Trois conséquences :

- une réponse qui mène à ce jet **disparaît** — dans cette conversation comme dans la suivante,
  puisque c'est un drapeau de monde et non une mémoire du runner ; demandée quand même, elle est
  refusée (`Unavailable`) ;
- le même jet atteint par **un autre chemin** (une réplique sans choix, une condition) échoue sans
  tirer de dé : la suite aléatoire reste intacte, et l'écran dit « déjà tenté » ;
- le drapeau compte parmi ceux que le récit pose (`core::flagsWrittenBy`) : une quête peut le
  lire sans que `--check` le dise « lu sans être posé ».

Le héraut du Colisée gardait son propre drapeau d'échec, écrit à la main ; il reste valable, et
devient superflu.

### D4 — Ce que le chargement refuse

- un jet **sans branche d'échec** : `failure` absent, ou menant où mène `success` — le jet ne
  déciderait rien ; le message le nomme ainsi (« jet sans branche d'echec ») ;
- une réplique que des jets ratés pourraient **vider** : une réponse qui mène à un jet compte
  comme conditionnelle, et il faut une réponse toujours proposée, sans condition et sans jet.

Le contrôle de l'éditeur (`LevelEditor --check`, `hmi::checkStoryContent`) liste désormais les
dialogues refusés au chargement : il ne les taisait pas moins que le jeu, qui les écarte en
journalisant.

### D5 — La graine

Le jet passe par `core::rollCheck` et la suite déterministe fournie au runner (`LOT-12`) ; le
journal détaille le dé, les modificateurs et leur origine. À graine égale, la même conversation
donne la même issue (`DialogueTest.UnDialogueSeRejoueAGraineFixee`). Dans le jeu, la graine d'une
conversation vient toujours d'un compteur (`DialogueModel`) : elle sera celle de la partie quand
la sauvegarde (`0.0.3`) en portera une.

## Tenue des critères

| Critère | Où |
|---|---|
| À graine fixée, la même issue | `DialogueTest.UnDialogueSeRejoueAGraineFixee` |
| Une réponse tentée et ratée ne se propose plus | `DialogueTest.UneReponseAJetRateeNeSeProposePlus`, `DialogueScreenTest.UnEchecSeMontreEtNeSeRetentePas` |
| Le contrôle refuse une réponse à jet sans branche d'échec | `DialogueTest.UnGrapheMalFormeEstRejeteAuChargement`, `ContentCheckTest.LeControleDuRecitRefuseUnJetSansBrancheDEchec` |

## Livraison

PR #134 (avec le lot jumeau). La fiche passe à `livre` à la fusion.
