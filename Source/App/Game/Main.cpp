// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file App/Game/Main.cpp
 * @brief Point d'entrée du **jeu** (`JustAnotherRpgGame`) — Qt Quick, sans un seul widget.
 *
 * `QGuiApplication` et non `QApplication` : la cible ne lie pas `Qt6::Widgets`, et ce n'est pas un
 * détail de dépendance mais la garantie qui porte tout le `LOT-86`. Un widget ne peut pas
 * réapparaître dans le jeu par inadvertance — l'édition de liens échouerait.
 *
 * L'éditeur de niveaux est un binaire séparé (`App/Editor/Main.cpp`), lui en Qt Widgets : c'est un
 * outil d'auteur, où docks et arbres sont le bon outil.
 */

#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QSurfaceFormat>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QVariantMap>
#include <filesystem>
#include <optional>
#include <string>

#include "App/Common/Bootstrap.h"
#include "Core/BuildConfig.h"
#include "Core/Core.h"
#include "Core/Diagnostics/MemoryLogSink.h"
#include "Core/Levels/GridPosition.h"
#include "HMI/Audio/AudioEngine.h"
#include "HMI/Game/LaunchOptions.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/CityBlockImageProvider.h"
#include "HMI/Runtime/OptionsModel.h"
#include "HMI/Runtime/ScreenRouter.h"
#include "HMI/Runtime/WorldModel.h"

namespace {

/**
 * @brief Enregistre les polices embarquées de l'identité auprès de Qt.
 *
 * Les fichiers sont déposés à côté de l'exécutable (`Assets/Fonts/`). Le QML ne les charge pas
 * lui-même : il les désigne par **nom de famille** (`Tokens.qml`), et Qt Design Studio les prend de
 * son côté via `FontFiles` du `.qmlproject`. Les deux voient donc les mêmes noms, et changer de
 * police reste une modification de `Tokens.qml` — jamais de C++.
 *
 * Un fichier absent n'est pas fatal : Qt retombe sur une famille générique et le journal le dit
 * (`EX-NFR-040`).
 *
 * Charte v2 (`LOT-87`) : `Cinzel` et `IM Fell English` remplacent la charte v1 (`Pixelify Sans`,
 * `Press Start 2P`) — retirées au T5.2, la phase 3 ayant transcrit les quatorze écrans.
 */
void registerIdentityFonts() {
    const std::filesystem::path fonts = hmi::executableDirectory() / "Assets" / "Fonts";
    for (const char* file :
         {"Cinzel-Regular.ttf", "Cinzel-SemiBold.ttf", "Cinzel-Bold.ttf",
          "IMFellEnglish-Regular.ttf", "IMFellEnglish-Italic.ttf", "PinyonScript-Regular.ttf"}) {
        const std::filesystem::path path = fonts / file;
        const int id = QFontDatabase::addApplicationFont(QString::fromStdString(path.string()));
        if (id < 0) {
            HMI_LOG_WARNING(std::string("Police d'identite non enregistree (") + file +
                            ") : famille generique.");
        }
    }
}

/**
 * @brief Traducteur du JEU, partagé par l'installation au lancement et le changement de langue.
 *
 * Duree de vie statique : QCoreApplication ne possede pas le traducteur, et un objet local
 * serait detruit a la sortie de cette portee. La traduction disparaitrait alors sans erreur,
 * et l'interface reviendrait au francais sans que rien ne le dise.
 */
QTranslator& gameTranslator() {
    static QTranslator translator;
    return translator;
}

/**
 * @brief Synchronisation verticale : elle se pose sur le FORMAT DE SURFACE, donc avant la creation
 * de la fenetre.
 *
 * C'est pour cela qu'elle s'applique au prochain lancement et que l'ecran des options le dit. La
 * changer en cours de route recreerait la surface de rendu sous les yeux du joueur, pour un
 * reglage qu'on modifie une fois.
 */
void applyVsyncSetting() {
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(QSettings().value(QStringLiteral("vsync"), true).toBool() ? 1 : 0);
    QSurfaceFormat::setDefaultFormat(format);
}

/**
 * @brief Traductions du JEU pour @p language.
 *
 * Le francais est la langue SOURCE des ecrans -- leurs textes s'ecrivent en francais dans le QML,
 * pour que la conception les lise dans Qt Design Studio -- et n'a donc pas de catalogue : sans
 * traducteur installe, `qsTr` rend sa source.
 */
void installGameTranslation(const QString& language) {
    if (language == QLatin1String("fr")) {
        return;
    }
    if (gameTranslator().load(QStringLiteral(":/i18n/jadg_") + language)) {
        QCoreApplication::installTranslator(&gameTranslator());
    } else {
        HMI_LOG_WARNING("Catalogue de traduction du jeu absent pour '" + language.toStdString() +
                        "' : l'interface restera en francais.");
    }
}

/// @brief Verse les erreurs du moteur QML et ses échecs de chargement dans le journal de session.
void connectEngineDiagnostics(QQmlApplicationEngine& engine, QGuiApplication& application) {
    // Les erreurs de l'engine vont, par defaut, sur la sortie d'erreur de Qt -- que personne ne
    // lit apres coup, et qui n'existe pas dans un binaire livre. On les verse dans le journal de
    // session : un ecran QML casse doit se diagnostiquer depuis Logs/, comme tout le reste.
    QObject::connect(&engine, &QQmlEngine::warnings, &application,
                     [](const QList<QQmlError>& warnings) {
                         for (const QQmlError& warning : warnings) {
                             HMI_LOG_ERROR("QML : " + warning.toString().toStdString());
                         }
                     });
    // Un échec de chargement QML ne lève pas : sans cette garde, le programme rendrait 0 avec une
    // fenêtre absente — la panne muette que le châssis Qt documente déjà pour d'autres raisons.
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
        []() {
            HMI_LOG_ERROR("Chargement de l'interface QML impossible : arret.");
            QCoreApplication::exit(1);
        },
        Qt::QueuedConnection);
}

/// @brief Planifie la capture de la fenetre racine @p root dans @p path, puis la sortie.
void scheduleWindowCapture(QObject* root, const QString& path) {
    auto* window = qobject_cast<QQuickWindow*>(root);
    if (window == nullptr) {
        HMI_LOG_ERROR("Capture impossible : la racine QML n'est pas une fenetre.");
        QCoreApplication::exit(1);
        return;
    }
    HMI_LOG_INFO("Capture : interface chargee, image dans 1,2 s.");
    QTimer::singleShot(1200, window, [window, path]() {
        const bool saved = window->grabWindow().save(path);
        HMI_LOG_INFO((saved ? "Capture ecrite : " : "Echec de la capture : ") + path.toStdString());
        QCoreApplication::exit(saved ? 0 : 1);
    });
}

/// @brief Sortie de secours du mode capture, quand aucune image n'a ete produite a temps.
void abortScreenshotOnTimeout() {
    HMI_LOG_ERROR(
        "Capture : delai depasse, aucune image produite (voir si l'interface a ete "
        "chargee, ligne precedente).");
    QCoreApplication::exit(2);
}

/**
 * @brief Capture d'ecran non interactive (--screenshot=<chemin>), armée si l'option est présente.
 *
 * Une fenetre Qt Quick est rendue par le GPU : les API de capture de Windows en tirent une image
 * NOIRE, seul Qt sait relire son propre graphe de scene. C'est ce qui rend la verification
 * visuelle des ecrans reproductible, au lieu de dependre d'un oeil devant l'ecran au bon moment.
 *
 * Declenchee par minuterie sur le fil graphique, et non depuis `frameSwapped` : ce signal est
 * emis par le FIL DE RENDU, et `grabWindow` -- qui attend ce meme fil -- n'y rendait jamais la
 * main. Le programme restait ouvert sans rien ecrire.
 */
void armScreenshot(int argc, char** argv, QQmlApplicationEngine& engine,
                   QGuiApplication& application) {
    const std::optional<std::string_view> shot =
        app::commandLineOption(argc, argv, "--screenshot=");
    if (!shot) {
        return;
    }
    const QString path = QString::fromUtf8(shot->data(), static_cast<qsizetype>(shot->size()));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &application,
        [path](QObject* root, const QUrl&) { scheduleWindowCapture(root, path); },
        Qt::SingleShotConnection);

    // Filet de securite : en mode capture, le programme ne doit JAMAIS rester ouvert. Sans
    // cette sortie, un echec de chargement de la fenetre laisserait un processus vivant qu'il
    // faudrait tuer a la main -- et, en integration continue, un travail suspendu.
    //
    // 45 s et non 15 : le build Debug, sur un runner charge, a depasse 15 s environ une fois sur
    // dix (Release du 21, du 23 et du 24 septembre 2026), sans rien de casse -- une reussite y
    // prend moins de 10 s. Le script de fumee tue le processus a 90 s ; ce filet reste en dessous.
    QTimer::singleShot(45000, &application, &abortScreenshotOnTimeout);
}

/**
 * @brief Taille de fenetre imposee (--window-size=<L>x<H>), ajoutée à @p initialProperties.
 *
 * Pour capturer un ecran a 1920 x 1080 et a 1280 x 720 cote a cote avec sa maquette (LOT-87,
 * phase 3). Passer par le plein ecran aurait ecrit le reglage du joueur, et donne la taille de SON
 * moniteur, pas celle qu'on verifie.
 */
void insertWindowSize(std::string_view size, QVariantMap& initialProperties) {
    const QStringList parts =
        QString::fromUtf8(size.data(), static_cast<qsizetype>(size.size())).split(QLatin1Char('x'));
    bool widthOk = false;
    bool heightOk = false;
    const int width = parts.size() == 2 ? parts[0].toInt(&widthOk) : 0;
    const int height = parts.size() == 2 ? parts[1].toInt(&heightOk) : 0;
    if (widthOk && heightOk && width > 0 && height > 0) {
        initialProperties.insert(QStringLiteral("width"), width);
        initialProperties.insert(QStringLiteral("height"), height);
    } else {
        HMI_LOG_WARNING("--window-size= attend <largeur>x<hauteur> : taille par defaut.");
    }
}

/**
 * @brief Propriétés initiales de la racine QML, lues sur la ligne de commande.
 *
 * `setInitialProperties` pose la propriété AVANT que la racine ne soit construite : l'affecter
 * après aurait fait afficher l'écran par défaut le temps d'une image, puis le bon -- un clignement
 * visible sur une capture.
 */
QVariantMap initialWindowProperties(int argc, char** argv) {
    QVariantMap initialProperties;
    // Écran d'ouverture (--screen=<Nom>).
    if (const std::optional<std::string_view> screen =
            app::commandLineOption(argc, argv, "--screen=")) {
        initialProperties.insert(
            QStringLiteral("startScreen"),
            QString::fromUtf8(screen->data(), static_cast<qsizetype>(screen->size())));
    }
    if (const std::optional<std::string_view> size =
            app::commandLineOption(argc, argv, "--window-size=")) {
        insertWindowSize(*size, initialProperties);
    }
    return initialProperties;
}

/**
 * @brief Brancher les reglages sur ce qu'ils atteignent.
 *
 * La vue-modele persiste et previent ; c'est ICI que chaque signal rejoint le moteur -- la
 * presentation ne connait ni le son, ni les traducteurs, ni la fenetre.
 */
void connectOptions(QQmlApplicationEngine& engine, hmi::AudioEngine& audio,
                    core::MemoryLogSink* sessionLog) {
    auto* const options =
        engine.singletonInstance<hmi::OptionsModel*>("Jadg.Runtime", "OptionsModel");
    if (options == nullptr) {
        HMI_LOG_ERROR("Reglages introuvables : le volume et la langue ne seront pas appliques.");
        return;
    }
    options->setSessionLog(sessionLog);
    audio.setVolume(static_cast<float>(options->volume()) / 100.0F);
    QObject::connect(options, &hmi::OptionsModel::volumeChanged, options, [options, &audio]() {
        audio.setVolume(static_cast<float>(options->volume()) / 100.0F);
    });
    // Changement de langue A CHAUD : le traducteur est remplace, puis `retranslate()` fait
    // reevaluer toutes les liaisons `qsTr` du QML. Sans ce second appel, la nouvelle langue
    // n'apparaitrait qu'aux ecrans construits ensuite -- la moitie de l'interface changerait.
    QObject::connect(options, &hmi::OptionsModel::languageChanged, &engine, [options, &engine]() {
        QCoreApplication::removeTranslator(&gameTranslator());
        if (options->language() != QLatin1String("fr") &&
            gameTranslator().load(QStringLiteral(":/i18n/jadg_") + options->language())) {
            QCoreApplication::installTranslator(&gameTranslator());
        }
        engine.retranslate();
    });
}

/// @return La valeur d'une option de ligne de commande, en `QString`.
[[nodiscard]] QString toQString(std::string_view value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

/**
 * @brief L'endroit et l'état de la partie imposés par la ligne de commande (`LOT-EDITOR-10`).
 *
 * `--at=<colonne>,<ligne>` pose le héros sur la case voulue, `--flags=<a>,<b>` marque des faits
 * acquis : c'est la carte **après** une quête, sans avoir à la jouer. Une valeur illisible est
 * ignorée et signalée, jamais fatale (`EX-NFR-040`).
 */
void applyStartState(int argc, char** argv, hmi::WorldModel& world) {
    if (const std::optional<std::string_view> at = app::commandLineOption(argc, argv, "--at=")) {
        if (const std::optional<core::GridPosition> cell = hmi::parseStartCell(*at)) {
            world.setStartCell(*cell);
        } else {
            HMI_LOG_WARNING("--at= attend <colonne>,<ligne> : le heros partira de l'entree.");
        }
    }
    if (const std::optional<std::string_view> flags =
            app::commandLineOption(argc, argv, "--flags=")) {
        QStringList poses;
        for (const std::string& flag : hmi::parseWorldFlags(*flags)) {
            poses.push_back(QString::fromStdString(flag));
        }
        world.setStartFlags(poses);
        HMI_LOG_INFO("Drapeaux de monde poses au lancement : " + std::to_string(poses.size()) +
                     ".");
    }
}

/**
 * @brief Carte d'ouverture imposée (--map=<carte>[@<arrivée>]), dans un build de développement.
 *
 * Pour voir ou capturer une carte sans y marcher depuis la porte de départ (`LOT-96`), et pour
 * l'essai complet que lance l'éditeur (`LOT-EDITOR-10`) : `--levels=` sert alors les brouillons
 * avant les cartes du binaire, `--at=` et `--flags=` disent où et dans quel état. Un binaire livré
 * ignore tout cela : « Nouvelle partie » y ouvre toujours la porte de départ.
 *
 * Le jeu s'**ouvre sur la carte**, sans passer par le menu : une carte imposée n'a de sens que si
 * on y entre, et l'essai de l'éditeur ne doit demander aucun clic. L'écran est celui que le
 * routeur désigne — pas un écran forcé (`--screen=`) —, si bien que dialogue, pause et Colisée
 * s'ouvrent ensuite normalement.
 */
void applyStartMap(int argc, char** argv, QQmlApplicationEngine& engine) {
    // `if constexpr` avec sa branche `else` : un retour anticipe laisserait en Release un code
    // inatteignable, que /W4 /WX refuse (C4702).
    if constexpr (core::DEVELOPER_BUILD) {
        const std::optional<std::string_view> option = app::commandLineOption(argc, argv, "--map=");
        if (!option) {
            return;
        }
        auto* const world =
            engine.singletonInstance<hmi::WorldModel*>("Jadg.Runtime", "WorldModel");
        if (world == nullptr) {
            HMI_LOG_WARNING("--map= : le modele du monde est introuvable.");
            return;
        }
        // AVANT toute autre chose : le chargeur des cartes refait la session, et emporterait les
        // drapeaux poses ou la figurine choisie.
        if (const std::optional<std::string_view> levels =
                app::commandLineOption(argc, argv, "--levels=")) {
            world->setLevelDirectories(hmi::parseLevelDirectories(*levels));
        }
        const QStringList parts = toQString(*option).split(QLatin1Char('@'));
        world->setStartOverride(parts.value(0), parts.value(1));
        applyStartState(argc, argv, *world);
        if (const auto figure = app::commandLineOption(argc, argv, "--hero-figure=")) {
            world->setHeroFigure(toQString(*figure));
        }
        if (auto* const router =
                engine.singletonInstance<hmi::ScreenRouter*>("Jadg.Runtime", "ScreenRouter")) {
            router->openGame();
        }
        HMI_LOG_INFO("Carte d'ouverture imposee : " + parts.value(0).toStdString());
    } else {
        static_cast<void>(argc);
        static_cast<void>(argv);
        static_cast<void>(engine);
    }
}

}  // namespace

/**
 * @brief Point d'entrée du programme.
 * @return Code de sortie du processus (0 en cas de succès).
 */
int main(int argc, char** argv) {
    core::MemoryLogSink* const sessionLog = app::installLogging(argc, argv, "JustAnotherRpgGame");

    // Identite de l'application AVANT toute lecture de reglage : c'est elle qui designe la portee
    // des QSettings. Lire la synchronisation verticale avant de la poser aurait interroge une
    // portee vide -- le reglage aurait paru absent, et sa valeur par defaut se serait appliquee a
    // chaque lancement sans que rien ne le signale.
    QCoreApplication::setOrganizationName(QStringLiteral("JustAnotherRpgGame"));
    QCoreApplication::setApplicationName(QStringLiteral("Game"));
    // Lue par le menu principal (`Qt.application.version`) : le numero reste celui du `project()`
    // racine, sans type C++ de plus a exposer ni doublure a tenir pour l'atelier.
    QCoreApplication::setApplicationVersion(QString::fromStdString(core::Engine::version()));

    applyVsyncSetting();

    QGuiApplication application(argc, argv);

    // La racine de contenu imposee (--data=<racine>, LOT-118) : le jeu lit cartes, assets, monde,
    // dialogues et rencontres la ou l'editeur les ouvre (`LevelEditor --data`), et l'on joue une
    // racine d'essai sans rien copier a cote de l'executable. AVANT tout modele : ils lisent
    // `hmi::dataDirectory()` a leur construction. Un binaire livre l'ignore.
    if constexpr (core::DEVELOPER_BUILD) {
        if (const std::optional<std::string_view> data =
                app::commandLineOption(argc, argv, "--data=")) {
            const std::filesystem::path racine = std::filesystem::absolute(*data);
            if (std::filesystem::is_directory(racine)) {
                hmi::setDataDirectory(racine);
                HMI_LOG_INFO("Contenu lu depuis " + racine.string());
            } else {
                HMI_LOG_WARNING("--data= : dossier introuvable, le contenu reste celui du binaire.");
            }
        }
    }

    const QString language =
        QSettings().value(QStringLiteral("language"), QStringLiteral("fr")).toString();
    app::installQtTranslations(language.toStdString());
    installGameTranslation(language);

    registerIdentityFonts();

    // Style des controles Qt Quick : « Basic », impose et non deduit.
    //
    // Sous Windows, Qt choisit « FluentWinUI3 » par defaut. Ce style peint ses controles avec les
    // couleurs du systeme et ignore largement la palette de l'application : les interrupteurs et le
    // curseur de volume ressortaient en BLEU au milieu du parchemin, et aucune retouche de
    // `Tokens.qml` n'y pouvait rien -- la conception aurait cherche longtemps.
    //
    // « Basic » est l'inverse : tout ce qu'il peint vient de la palette, que `Main.qml` derive des
    // jetons. C'est ce qui rend l'apparence des controles modifiable sans code, comme le reste
    // (EX-IHM-100). Il est de surcroit le seul style identique sur toutes les plateformes.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Moteur audio : le volume des options doit atteindre QUELQUE CHOSE, sans quoi le reglage
    // serait un mensonge (EX-IHM-083). Il vit ici, dans l'application, et non dans la vue-modele
    // des options -- ce n'est pas a un ecran de reglages de posseder le son du jeu.
    hmi::AudioEngine audio;

    QQmlApplicationEngine engine;

    // Éditer un écran sans rien reconstruire (EX-IHM-100). Par défaut, les .qml sont lus dans la
    // ressource embarquée -- c'est ce qu'il faut pour un binaire livré. Avec
    // JADG_QML_FROM_SOURCE=1, on place en tête des chemins d'import un module dont le qmldir
    // désigne les fichiers SOURCES : le programme lit alors Source/Ui directement, et relancer
    // suffit à voir la retouche. Seul `Jadg.Ui` -- les formulaires, le territoire de la
    // conception -- se relit ainsi ; `Jadg.App` (le câblage) et `Jadg.Runtime` (le C++) restent
    // ceux du binaire, et c'est voulu : ce qu'un artiste change ne demande jamais de les toucher.
    //
    // `addImportPath` insère en tête : le module sur disque l'emporte donc sur celui de la
    // ressource, sans qu'il faille retirer ce dernier.
    if (qEnvironmentVariable("JADG_QML_FROM_SOURCE") == QLatin1String("1")) {
        engine.addImportPath(QStringLiteral(JADG_QML_DEV_IMPORT_PATH));
        HMI_LOG_INFO("Interface lue depuis les sources : " JADG_QML_DEV_IMPORT_PATH);
    }
    connectEngineDiagnostics(engine, application);
    // Les ilots du plan (LOT-96) : dessines a la demande, le moteur prend possession du
    // fournisseur.
    engine.addImageProvider(QStringLiteral("cityblock"),
                            new hmi::CityBlockImageProvider(hmi::dataDirectory()));
    armScreenshot(argc, argv, engine, application);

    const QVariantMap initialProperties = initialWindowProperties(argc, argv);
    if (!initialProperties.isEmpty()) {
        engine.setInitialProperties(initialProperties);
    }

    // AVANT `loadFromModule`, et ce n'est pas un detail de style : le chargement construit la
    // fenetre ET ses ecrans dans la foulee. Brancher apres, c'est laisser les liaisons de ces
    // ecrans s'evaluer sur un modele pas encore renseigne -- et `logsAvailable` etant CONSTANT,
    // elle ne se serait jamais reevaluee : le bouton d'export des journaux serait reste grise
    // pour toujours, dans un build ou les journaux existent pourtant.
    connectOptions(engine, audio, sessionLog);
    applyStartMap(argc, argv, engine);

    // En dernier : la fenetre et tous ses ecrans naissent ici, et doivent trouver un modele deja
    // branche.
    engine.loadFromModule("Jadg.App", "Main");

    const int code = QGuiApplication::exec();
    HMI_LOG_INFO("Arret de JustAnotherRpgGame (code " + std::to_string(code) + ").");
    return code;
}
