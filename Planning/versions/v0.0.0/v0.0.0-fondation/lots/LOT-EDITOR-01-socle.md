+++
id = "LOT-EDITOR-01"
titre = "Le socle du module"
version = "0.0.0"
filiere = "editeur"
statut = "livre"
taille = "M"
resume = "L'éditeur vit dans son propre module, hors de la charte et de la traduction, et l'on y travaille longtemps sans risque : reprise après plantage, garde du fichier sur disque, historique plafonné."
prerequis = []
livrables = [
  "`Source/Editor/{Logic,Ui}` et `Source/Test/Unit/Editor` : bibliothèque `EditorLogic`, exécutable `LevelEditor` (`Source/Editor/CMakeLists.txt`), `HmiLib` libérée de l'éditeur.",
  "L'outil interne : style Fusion, icônes standard, anglais en dur, widgets construits en code ; retrait des jetons, du thème, des icônes tracées, du catalogue d'actions, des sept `.ui`, de `theme-editor.qss`, d'Inter et des 189 clés.",
  "`Autosave` et `DiskGuard` (logique pure, testée) : sauvegarde automatique, reprise au démarrage, garde du fichier par empreinte, question à la fermeture.",
  "`core::LevelDraft` : `revision`, plafond `UNDO_HISTORY_LIMIT` (200 pas), `toJson`.",
  "`--crash-test` différé (`app::CrashTest::Deferred`) : l'éditeur plante après sa première sauvegarde automatique.",
  "`EX-EDIT-056`, `EX-EDIT-057`, `EX-EDIT-058` ; la documentation du module (feuille de route, `editeur-niveaux.md`, `interface-ihm.md`, guides et README).",
]
criteres = [
  "Les fonctions actuelles marchent à l'identique.",
  "`ctest` vert.",
  "Un plantage provoqué est suivi d'une proposition de reprise qui rend le brouillon.",
  "Modifier le JSON d'une carte ouverte et modifiée fait proposer « recharger » ou « garder », sans rien perdre de l'un ni de l'autre.",
  "Historique plafonné.",
]
+++

## Pourquoi

L'éditeur sort dans son propre module, débarrassé de la charte, et gagne de quoi travailler
longtemps sans risque : un brouillon ne se perd plus sur un plantage, une carte changée sur disque
n'est plus écrasée en silence, et l'historique d'annulation a un plafond.

Feuille de route : [éditeur](@ref roadmap-editeur). Le lot alimente `LOT-EDITOR-02`,
`LOT-EDITOR-12` et `LOT-EDITOR-10`.

## Ce que le dépôt contenait à l'ouverture (18 septembre 2026)

- **L'éditeur vivait dans `HMI`** : `Source/HMI/Editor` (canevas, panneaux, logique pure),
  `Source/HMI/Interface` (fenêtre, actions, jetons, thème, icônes tracées), `Source/Ui/Editor`
  (sept formulaires `.ui` et une ressource), la feuille `Source/Elements/Themes/theme-editor.qss`.
  La logique pure de l'éditeur était compilée dans `HmiLib`, la bibliothèque que le jeu lie.
- **Tous ses textes passaient par `hmi::Localization`** : 189 clés dans `fr.lang` et `en.lang`, et
  des tests qui vérifiaient leur présence dans les deux catalogues.
- **« Modifié » était un booléen posé à la main** par chaque geste du canevas (`_dirty = true`), et
  remis à faux à l'ouverture et à l'enregistrement. Défaire jusqu'à l'état enregistré laissait la
  carte « modifiée ».
- **La carte de départ se relisait dans `createResources`**, que `QRhiWidget` rappelle à chaque
  changement d'interface de rendu (fenêtre passée sur un autre écran, dock détaché) : le brouillon en
  cours était alors remplacé par le Colisée du disque, sans question.
- **L'historique d'annulation n'avait pas de plafond** (constat A10) : un instantané complet par
  pas, et un pas par case survolée pendant un coup de pinceau, même sur une case déjà du bon type.
- **Aucune sauvegarde automatique, aucune garde du fichier** ; fermer la fenêtre avec des
  modifications les perdait sans rien demander.

## Périmètre

### Livraison

| Commit | Contenu |
|---|---|
| 1 — déménagement | `git mv` vers `Source/Editor/{Logic,Ui}` et `Source/Test/Unit/Editor`, cible `LevelEditor` et bibliothèque `EditorLogic` dans `Source/Editor/CMakeLists.txt`, `HmiLib` libérée de l'éditeur ; aucun comportement changé |
| 2 — outil interne | Fusion, icônes standard, anglais en dur, widgets construits en code ; retrait des jetons, du thème, des icônes tracées, du catalogue d'actions, des sept `.ui`, de `theme-editor.qss`, d'Inter et des 189 clés ; retrait des tests de charte et de traduction de l'éditeur |
| 3 — sécurité | `Autosave`, `DiskGuard` (logique pure, testée), révision et plafond de `LevelDraft`, `LevelDraft::toJson`, reprise au démarrage, garde du fichier, question à la fermeture, `--crash-test` différé |
| 4 — documentation | le dossier du lot, la feuille de route, `editeur-niveaux.md`, `interface-ihm.md`, les guides et les README |

### Ce qui reste hors du lot, nommément

- **Deux éditeurs ouverts en même temps** se verraient chacun proposer la reprise des brouillons de
  l'autre au démarrage (le dossier de reprise est par poste, pas par processus). Rare pour un outil
  à un seul auteur ; un verrou de session le réglerait.
- **Un coup de pinceau reste un pas d'annulation par case** : « un geste = un pas » est aux outils
  du `LOT-EDITOR-04`.
- **Le rendu `QRhiWidget` du canevas** n'a pas été poli : il est réécrit au `LOT-EDITOR-02`.

## Conception

- **Tests dans `UnitTests`, sources dans `Source/Test/Unit/Editor`.** La décision D5 veut des
  tests « à lui » ; ils le sont par leur dossier et par la bibliothèque qu'ils lient
  (`EditorLogic`), pas par un exécutable de plus : la CI, la couverture, ASan et la nuit lancent
  `UnitTests.exe` par son nom, et un quatrième exécutable aurait demandé de retoucher les quatre.
- **L'espace de noms reste `hmi`.** Renommer 60 fichiers en `editor::` n'apporte rien au lot et
  gonflerait chaque diff des lots suivants ; à reprendre si un lot touche de toute façon tout le
  module.
- **Une famille d'entité et une propriété se nomment par leur identifiant du format** (`npc`,
  `dialogue`, `targetMap`) : c'est ce que l'auteur lit dans le fichier, et une table de libellés
  anglais aurait refait un catalogue. L'inspecteur typé du `LOT-EDITOR-05` tranchera l'affichage.
- **Le menu Thème part avec `EX-IHM-054`** : Fusion suit le schéma clair/sombre du système.
- **La police Inter est retirée** : seule la charte de l'éditeur la chargeait.
- **« Modifié » compare des révisions** (`core::LevelDraft::revision`) : chaque mutation donne une
  révision neuve, `undo` et `redo` rendent celle de l'état qu'ils restaurent. L'éditeur est modifié
  quand la révision n'est plus celle de l'ouverture ou de l'enregistrement. Repeindre une case du
  même type ne fait plus rien (ni pas d'historique, ni révision). Le bogue relevé au
  [LOT-11](@ref lot-11) n'a pas été reproduit sur la version du lot ; la nouvelle règle le rend
  impossible sans mutation réelle.
- **La carte de départ s'ouvre à la construction du canevas**, plus dans `createResources`.
- **Historique plafonné à 200 pas** (`LevelDraft::UNDO_HISTORY_LIMIT`). Le stockage des pièces par
  indice, que A10 évoque, attend le format v4 du `LOT-EDITOR-12`.
- **Sauvegarde automatique** dans `%LOCALAPPDATA%\JustAnotherRpgGame\Editor\autosave`, deux
  secondes après le dernier geste, par fichier temporaire renommé. Le fichier part à
  l'enregistrement et à toute fermeture voulue ; **un fichier présent au démarrage est la trace
  d'une session interrompue**, et l'éditeur propose « Recover » ou « Discard ». « Discard » ne
  supprime rien : le brouillon va dans `autosave\conflicts\`.
- **`--crash-test` plante l'éditeur après sa première sauvegarde automatique**, et non au
  démarrage comme le jeu (`app::CrashTest::Deferred`) : un plantage au démarrage n'a pas de
  brouillon à reprendre. Le test de fumée de la release lance le jeu seul, rien n'y change.
- **Garde du fichier par empreinte de contenu** (taille et FNV-1a 64 bits), comparée quand le
  fichier est signalé changé (`QFileSystemWatcher`), quand la fenêtre reprend la main, et avant
  chaque enregistrement. Brouillon intact : relu en silence. Brouillon modifié : « Reload from
  disk » ou « Keep my version », et la version écartée va dans `autosave\conflicts\` avant tout.
  Fichier supprimé : le brouillon reste, l'enregistrement le recrée.
- **Fermer avec des modifications demande** « Save », « Discard » ou « Cancel ».

## Vérification

- **Les fonctions actuelles marchent à l'identique.** ✔ en automatique pour la logique (tests de
  l'éditeur inchangés hormis les textes) ; ✔ à l'écran pour l'ouverture (capture du lancement).
  **À vérifier à la main** : peindre, rectangle, sélection, copier/coller, couches, entités, essai
  `P`, renommer, redimensionner, raccourcis remappés.
- **`ctest` vert.** ✔
- **Un plantage provoqué est suivi d'une proposition de reprise qui rend le brouillon.** ✔ pour la
  mécanique (`AutosaveTest`, `LevelDraftTest.ToJsonRendUnBrouillonIncomplet`) ; **à vérifier à la
  main** : `LevelEditor --crash-test`, peindre une case, attendre le plantage, relancer, « Recover ».
- **Modifier le JSON d'une carte ouverte et modifiée** : ✔ pour la mécanique (`DiskGuardTest`,
  `AutosaveTest.UneVersionEcarteeEstGardeeDeCote`) ; **à vérifier à la main** : ouvrir Martpart,
  peindre, modifier `martpart.json` dans un éditeur de texte, revenir à la fenêtre, essayer les deux
  réponses et retrouver l'autre version sous `autosave\conflicts\`.
- **Historique plafonné.** ✔ `LevelDraftTest.LHistoriqueEstPlafonne`.

## Bilan

**Livré le 18 septembre 2026** (ouvert le même jour), sur la branche
`worktree-lot-editor-01-socle`, en quatre commits : le déménagement à l'identique, la sortie de la
charte et de la traduction, la sauvegarde automatique et les gardes, puis la documentation.
Vérification automatisée : construction `/W4 /WX` sans avertissement, `ctest` vert à chaque
commit. Relu à l'écran : `LevelEditor` lancé et capturé (style Fusion, textes anglais, docks,
Colisée ouvert, non modifié). Reste la vérification manuelle des scénarios d'acceptation à la
souris (voir « Vérification »).

Exigences : `EX-EDIT-056`, `EX-EDIT-057`, `EX-EDIT-058` (nouvelles) ; `EX-IHM-010`, `020`, `054`,
`055`, `060`, `061`, `074` retirées (décision D7) ; `EX-IHM-050` à `053` et `082` précisées.
