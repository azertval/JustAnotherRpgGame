# Journalisation et assertions

Cette page explique à quoi sert un système de journalisation (« logging ») dans un jeu, et comment
celui de ce moteur est construit. Tout vit dans `Source/Core/Diagnostics`, plus un petit en-tête de
macros par module (`Source/Core/CoreLog.h`, `Source/HMI/HmiLog.h`, `Source/HMI/Graphics/
GraphicsLog.h`, …).

## Pourquoi journaliser dans un jeu vidéo

Un jeu compilé en **Release**, lancé par un joueur, n'a **pas** de console attachée ni de
débogueur : impossible d'y poser un point d'arrêt ou d'inspecter une variable en direct. Si un
comportement anormal survient — un niveau ne se charge pas, une ressource graphique échoue à se
créer — la seule trace disponible après coup est ce que le programme a **explicitement enregistré**
pendant son exécution. La **journalisation** consiste précisément à écrire, au fil de l'exécution,
des messages textuels horodatés décrivant les événements significatifs (démarrage, chargement d'un
niveau, erreur récupérable…), pour reconstituer après coup ce qui s'est passé sans avoir dû le
prévoir en amont dans un débogueur.

C'est un outil **différent** d'une assertion (voir la dernière section) : le journal enregistre des
faits pour un usage **ultérieur** et humain (diagnostic après coup), sans jamais interrompre le
programme ; une assertion vérifie un invariant **immédiatement** et signale un **bug** s'il est
violé.

## Les niveaux de gravité : `core::LogLevel`

Tous les messages ne se valent pas : un message peut être un détail de mise au point sans intérêt
en usage normal, ou au contraire signaler une erreur qu'il faut voir absolument. `core::LogLevel`
distingue quatre niveaux, en **gravité croissante** :

| Niveau | Usage |
|--------|-------|
| `Trace` | Détails fins de débogage — verbeux, utile en cours de développement d'une fonctionnalité précise. |
| `Info` | Fonctionnement normal — démarrage, niveau chargé, ressource créée. |
| `Warning` | Anomalie **non bloquante** — le jeu continue, mais quelque chose méritait attention. |
| `Error` | Erreur nécessitant attention — un échec qui affecte le déroulement. |

Le fait que ces niveaux soient **ordonnés** permet de **filtrer** : en développement, on veut
généralement tout voir (`Trace` et au-dessus) ; pour une session de test plus ciblée, ne garder que
`Warning` et `Error` évite de noyer les messages importants dans le bruit des détails `Trace`/`Info`.
C'est le rôle du `Logger`, ci-dessous.

## `core::Logger` : filtrer puis diffuser

`core::Logger` a exactement deux responsabilités, séparées volontairement de toute écriture réelle
sur un support (fichier, console, etc.) :

- **filtrer** : `setMinimumLevel(niveau)` fixe un seuil ; un message dont le niveau est **inférieur**
  à ce seuil est ignoré avant même d'être formaté (`isEnabled(level)` permet de vérifier ce filtre
  **avant** de construire un message coûteux, voir plus bas) ;
- **diffuser** : un message qui passe le filtre est transmis à **tous** les sinks enregistrés
  (`addSink`).

Le `Logger` **ne fait aucune entrée-sortie lui-même** — il ne sait pas écrire sur un fichier, une
console, ou ailleurs. C'est une décision de conception délibérée : séparer « décider si un message
doit être vu » (le `Logger`) de « où et comment l'écrire réellement » (les sinks) permet de changer
librement les destinations sans toucher au code qui produit les messages, et de tester le filtrage
sans jamais toucher un vrai fichier ou une vraie console (voir `MemoryLogSink` plus bas).

Une instance **unique**, globale, est accessible via `core::defaultLogger()` — c'est elle que les
macros de journalisation (voir plus bas) utilisent implicitement, pour éviter d'avoir à faire
transiter une référence de `Logger` à travers tout le code applicatif.

## Les sinks : où finissent les messages

Un **sink** (« puits », au sens d'un point de destination) implémente l'interface `core::ILogSink`
— une seule méthode, `write(level, message)`. Trois implémentations existent :

- `core::ConsoleLogSink` : écrit sur la sortie standard et, sous Windows, vers le débogueur (visible
  par exemple dans la fenêtre de sortie de Visual Studio même sans console ouverte) ;
- `core::FileLogSink` : écrit chaque ligne dans un fichier sur disque (dossier parent créé au
  besoin) ;
- `core::MemoryLogSink` : ne fait **aucune** entrée-sortie — il **mémorise** les messages reçus dans
  un vecteur, consultable via `entries()`. Deux usages distincts en pratique : dans les **tests**
  automatisés, vérifier qu'un message précis a bien été journalisé sans dépendre d'un fichier ou
  d'une console réels ; en exécution normale, alimenter un **bouton d'enregistrement des logs** de
  l'interface (un joueur qui rencontre un problème peut exporter la session sans avoir eu besoin
  d'ouvrir une console au préalable).

Comme le `Logger` **possède** ses sinks (`std::unique_ptr`, RAII), ajouter un nouveau type de
destination revient à écrire une nouvelle classe implémentant `ILogSink`, sans toucher au `Logger`
ni au code qui journalise.

## Les macros de journalisation, par catégorie

Écrire `core::defaultLogger().log(core::LogLevel::Info, "...")` à chaque site d'appel serait
verbeux et n'indiquerait pas d'où vient le message. `Core/Diagnostics/Log.h` définit une macro
générique, `JADG_LOG(catégorie, niveau, message)`, et ses quatre raccourcis
`JADG_LOG_TRACE/INFO/WARNING/ERROR(catégorie, message)`.

**Pourquoi une macro plutôt qu'une fonction.** Deux raisons concrètes :

- `__FILE__`/`__LINE__` (fichier et ligne source du site d'appel) ne peuvent être capturés
  automatiquement que par une macro préprocesseur — une fonction ordinaire ne verrait que le fichier
  où **elle-même** est définie, jamais celui de l'appelant ;
- la macro teste `isEnabled(level)` **avant** de construire la chaîne de message. Un appel comme
  `JADG_LOG_TRACE("Core", "Position : " + std::to_string(x) + ...)` construirait une
  concaténation de chaînes à **chaque** appel si c'était une fonction ordinaire, même quand le
  niveau `Trace` est filtré — un coût inutile, potentiellement significatif si l'appel est fréquent.
  En macro, si le niveau est désactivé, le message n'est **jamais évalué**.

### Chaque module a sa propre catégorie

Plutôt que de répéter une chaîne littérale `"HMI"` ou `"Core"` à chaque site d'appel (source
d'erreurs de frappe et de récriture répétitive), chaque module définit son propre petit en-tête de
raccourcis — le **modèle** à dupliquer pour un nouveau module est `HMI/HmiLog.h` :

```cpp
// HMI/HmiLog.h
#define HMI_LOG_TRACE(message)   JADG_LOG_TRACE("HMI", message)
#define HMI_LOG_INFO(message)    JADG_LOG_INFO("HMI", message)
#define HMI_LOG_WARNING(message) JADG_LOG_WARNING("HMI", message)
#define HMI_LOG_ERROR(message)   JADG_LOG_ERROR("HMI", message)
```

`Core/CoreLog.h` (catégorie `"Core"`), `Core/Ecs/EcsLog.h` (`"Ecs"`), `Core/Levels/LevelsLog.h`
(`"Levels"`), `HMI/Graphics/GraphicsLog.h` (`"Graphics"`) et `HMI/Audio/AudioLog.h` (`"Audio"`)
suivent exactement le même modèle. La **catégorie** apparaît ensuite dans chaque ligne journalisée
(voir le format ci-dessous), ce qui permet, en lisant un journal, de savoir immédiatement quel sous-système a
émis un message donné — utile dès qu'un jeu grandit au-delà de quelques fichiers, et pour filtrer un
journal verbeux en ne gardant que la catégorie qui intéresse un diagnostic précis (par exemple
`grep "\[Levels\]"` pour ne voir que les chargements de cartes).

### Une règle de performance à respecter

Le commentaire de `Core/CoreLog.h` le rappelle explicitement : ces macros sont **à réserver aux
événements de cycle de vie** (démarrage, chargement d'un niveau, création d'une ressource) — **jamais
dans un chemin exécuté à chaque frame ou à chaque pas fixe** ([Boucle de jeu et pas de temps fixe](guide-boucle.md)). Un jeu tourne à
60 pas par seconde ; journaliser à cette fréquence, même avec un niveau filtré, resterait coûteux
(construction de chaînes, appel de fonction, éventuelle écriture) et pourrait à lui seul dégrader le
*framerate* — le symptôme inverse de ce que la journalisation est censée aider à diagnostiquer.

## Le format d'une ligne : `core::formatLogLine`

`core::formatLogLine(timestamp, level, category, file, line, message)` compose une ligne de la
forme :

```
[14:32:07][INFO][HMI][main.cpp:148] Fenetre creee (1280x720)
```

Deux détails valent d'être notés :

- `fileName(path)` réduit un chemin complet (tel que fourni par `__FILE__`, qui peut être un chemin
  absolu très long selon la configuration de build) à son seul nom de fichier — la ligne complète
  resterait lisible sans encombrer chaque message du chemin entier du projet ;
- l'horodatage (`currentTimestamp()`) est calculé **séparément** puis **injecté** en paramètre de
  `formatLogLine`, plutôt que lu directement à l'intérieur de la fonction de formatage. Cela rend
  `formatLogLine` **pure** et testable : un test peut lui passer un horodatage fixe et vérifier la
  ligne produite **exactement**, sans dépendre de l'heure réelle à laquelle le test s'exécute — un
  cas particulier du principe déjà rencontré en [Boucle de jeu et pas de temps fixe](guide-boucle.md) (isoler ce qui dépend du temps
  réel derrière un paramètre injecté, pas une lecture directe de l'horloge système).

## Configurer le niveau minimal au lancement

Le niveau minimal du `Logger` n'est pas figé dans le code : `core::parseLogLevel(text)`
(`Core/Diagnostics/LogLevelParse.h`) convertit une chaîne (« trace », « info », « warning »/« warn »,
« error », insensible à la casse) en `LogLevel`, ce qui permet de le régler **au lancement** sans
recompiler. Dans ce moteur (`app::installLogging`, `Source/App/Common/Bootstrap.cpp`, partagé par le jeu et
l'éditeur), deux sources sont acceptées, avec priorité à la
seconde si les deux sont présentes :

1. la variable d'environnement `JADG_LOG_LEVEL` ;
2. l'argument de ligne de commande `--log-level=<niveau>`.

Une valeur non reconnue par `parseLogLevel` (renvoyant `std::nullopt`) est **ignorée** plutôt que de
faire échouer le démarrage — journaliser un `Warning` à ce sujet est préférable à interrompre le
jeu pour une simple faute de frappe dans un paramètre de diagnostic.

### Bootstrap réel : sinks différents en développement et en Release

`app::installLogging` illustre bien pourquoi séparer `Logger` (filtrage) et sinks (destination) est
utile en pratique : en build de **développement** (`core::DEVELOPER_BUILD`, dérivé de `NDEBUG`), un
`ConsoleLogSink` **et** un `MemoryLogSink` sont enregistrés (le second alimentant le bouton
d'enregistrement de session évoqué plus haut) ; dans **les deux** configurations, un
`FileLogSink` écrit un fichier horodaté par lancement (`Logs/run_<date>_<heure>.log`, à côté de
l'exécutable) — un crash n'écrase jamais les preuves du lancement précédent. L'exécutable Release
n'a pas de console (sous-système Windows pur) et il serait inutile d'y faire grandir un tampon
mémoire. Le code qui journalise, lui, reste **identique** dans les deux configurations :
`HMI_LOG_INFO("Demarrage...")` s'exécute pareil des deux côtés — c'est le nombre de sinks
enregistrés, décidé une fois au démarrage, qui change ce qu'il en advient.

Le `MemoryLogSink` (`Core`) collecte ces entrées en mémoire en build développement ; les écrire sur
disque relève de la couche présentation. L'écran **Options** du jeu expose un bouton **«
Enregistrer les journaux »** : `hmi::OptionsModel::saveLogs` sérialise `MemoryLogSink::entries()`,
niveau en tête de chaque ligne, dans un fichier horodaté (`Logs/session_<date>_<heure>.log`), sans
toucher à ce que `Core` a déjà collecté. En Release, aucun sink mémoire n'est enregistré : le bouton
signale simplement des journaux indisponibles.

## Assertions : `JADG_ASSERT`, un outil différent

Une **assertion** vérifie qu'une condition, censée être **toujours vraie** si le code est correct
(une précondition, un invariant), l'est effectivement à un point précis de l'exécution — sa
violation signale un **bug** dans le programme lui-même, pas un événement à consigner pour
information. C'est déjà ce que `ComponentPool`/`World` utilisent abondamment ([ECS : entités, composants, systèmes](guide-ecs.md)) :
`JADG_ASSERT(has(entity), "...")` avant d'accéder à un composant, par exemple.

Deux différences fondamentales avec la journalisation :

- **active seulement en Debug** : en Release (`NDEBUG` défini), `JADG_ASSERT(condition,
  message)` ne produit **aucune instruction** et **n'évalue même pas** `condition` — coût nul en
  production, contrairement à un message de log qui peut rester actif (filtré, mais présent) des
  deux côtés. Le raisonnement : une assertion protège le **développement** contre des bugs
  introduits par erreur ; en Release, on présuppose le code déjà correct plutôt que de payer un
  coût de vérification permanent pour des invariants qu'on a déjà validés ;
- **gestionnaire remplaçable** : `core::setAssertionHandler(handler)` permet de substituer le
  comportement par défaut (qui interromprait normalement le programme) par un gestionnaire
  personnalisé — utile en test, pour vérifier qu'une fonction **déclenche bien** une assertion sur
  une entrée invalide, sans faire planter la suite de tests elle-même.

En résumé : **journaliser** un événement (« la carte du Colisée a été chargée ») documente un fait pour un
humain qui lira le journal plus tard ; **asserter** une condition (« cette entité doit être vivante
ici ») protège contre un bug du code, et n'a de sens qu'en développement.

## Quand le journal s'arrête net : le rapport de plantage

Un journal documente ce qui est arrivé **jusqu'au** plantage. Il ne dit jamais *où* le programme
est mort : la dernière ligne écrite est celle d'avant, et elle est le plus souvent anodine. C'est ce
manque que comble le **minidump** (`Source/HMI/Platform/CrashDump.h`) : il garde la pile de chaque
thread et le contexte de l'exception, et, ouvert dans Visual Studio avec les symboles de la **même
version**, montre la ligne fautive.

Rien n'est envoyé nulle part. Le fichier reste dans `Crashes/`, à côté de l'exécutable, comme
`Logs/`.

### `hmi::installCrashDumpWriter`

À appeler **une fois, au plus tôt dans `main`**, et — c'est le détail qui compte — **après**
l'installation du journal : le chemin du dump est consigné dans le journal avant la sortie, ce qui
est la seule façon pour l'utilisateur de savoir qu'un fichier l'attend.

Elle couvre plus que l'exception structurée non attrapée (violation d'accès, division par zéro).
Trois autres fins anormales, qui ne sont **pas** des exceptions structurées, sont converties en une
exception maison (`kFatalErrorExceptionCode`, « JADG ») que le même filtre sait écrire :

| Fin anormale | Cas typique |
|---|---|
| `std::terminate` | une exception C++ non attrapée |
| appel d'une fonction **virtuelle pure** | un objet utilisé pendant sa construction ou sa destruction |
| **paramètre invalide** passé à la CRT | un indice hors bornes sur une fonction de la bibliothèque C |

Le processus se termine ensuite **sans** la boîte de dialogue du rapport d'erreurs Windows — qui,
sur une machine de CI, n'attend qu'un clic qui ne viendra jamais.

### `hmi::writeMiniDump` : quatre tentatives, de la plus riche à la plus pauvre

Écrire un dump est une opération qui peut **échouer dans un processus déjà mourant**. La fonction
tente donc quatre écritures, de la plus complète à la plus réduite :

1. le dump **riche**, avec la mémoire référencée par les piles ;
2. et 3. des dumps réduits, qui gardent piles et contexte mais abandonnent l'état étendu du
   processeur ;
4. le **seul thread du plantage**.

La dernière tentative mérite son explication, parce qu'elle vient d'une panne réellement
diagnostiquée en CI. Une pile illisible — un thread quelconque, sans rapport avec le plantage —
fait échouer **tout** le dump : l'API ne saute pas le thread fautif, elle abandonne. Se replier sur
le seul thread du plantage contourne exactement ce cas. Et cette dernière tentative est en outre
**réessayée**, parce que l'échec observé s'est révélé être un aléa et non un refus stable.

`GetLastError()` ne rend que la raison de la **dernière** tentative, ce qui rend une panne de ce
genre indéchiffrable. D'où `hmi::lastMiniDumpAttemptErrors`, qui rend le relevé **tentative par
tentative** ; c'est un outil de diagnostic pur, que le test de minidump affiche quand il échoue. La
règle qui en découle, apprise à ses dépens : devant un test de minidump rouge, **lire le relevé**,
jamais relancer le job.

### `hmi::routeCrtReportsToStderr`

En Debug, une assertion de la bibliothèque standard (`_STL_VERIFY`, `assert`) ouvre une boîte
modale « Microsoft Visual C++ Runtime Library » et **attend un clic**. Un programme sans fenêtre —
`LevelEditor --check`, `UnitTests.exe`, un job de CI — s'arrête alors pour toujours, sans un mot
dans son journal : le symptôme est un *timeout*, et rien n'indique la cause.

Routées vers `stderr`, les mêmes assertions nomment leur **fichier et leur ligne**, puis la CRT
poursuit sa route habituelle, que `installCrashDumpWriter` sait conclure par un minidump. La
fonction est appelée par `installCrashDumpWriter` ; les exécutables de test, qui n'ont pas de
bootstrap d'application, l'appellent eux-mêmes (`Source/Test/Support/CrtReports.cpp`).

### Le nom du fichier, et pourquoi la version y figure

`hmi::crashDumpFileName` compose `<application>_<version>_<AAAAMMJJ_HHMMSS>.dmp`. La version n'est
pas décorative : un minidump ne se lit qu'avec les symboles **du binaire exact** qui l'a produit, et
c'est elle qui permet de retrouver la bonne archive `<nom>-symbols.zip` sans ouvrir le fichier. Tout
caractère hors `[A-Za-z0-9.-]` devient `_`, pour qu'une version comme `0.1.0+dev` reste un nom de
fichier sûr.

### Prouver que la chaîne marche : `--crash-test`

Un dispositif de plantage qui n'a jamais planté n'est pas un dispositif : c'est une intention.
`hmi::triggerCrashForTest` déclenche volontairement une violation d'accès, et l'option
`--crash-test` du jeu l'expose. Le test de fumée de la release la lance sur l'**archive livrée** —
pas sur un build local — pour prouver que c'est bien le zip téléchargé qui écrit son dump.

## Voir aussi
- `core::Logger`, `core::LogLevel`, `core::ILogSink`, `core::ConsoleLogSink`, `core::MemoryLogSink`.
- `hmi::installCrashDumpWriter`, `hmi::writeMiniDump`, `hmi::crashDumpFileName`,
  `hmi::routeCrtReportsToStderr`.
- `core::formatLogLine`, `core::parseLogLevel`, `core::defaultLogger`.
- `JADG_ASSERT`, `core::setAssertionHandler`.
- [Boucle de jeu et pas de temps fixe](guide-boucle.md) — la règle « jamais de log dans le chemin exécuté à chaque pas fixe ».
- [ECS : entités, composants, systèmes](guide-ecs.md) — usage concret des assertions pour les préconditions du `World`/`ComponentPool`.
