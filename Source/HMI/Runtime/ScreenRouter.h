// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

#include "HMI/Presentation/RpgScreens.h"
#include "HMI/Presentation/ScreenFlow.h"

/**
 * @file HMI/Runtime/ScreenRouter.h
 * @brief La navigation entre écrans, telle que le QML la pilote (`LOT-86`).
 */

namespace hmi {

/**
 * @brief Pilote la machine à états des écrans, et publie l'écran courant.
 *
 * ## Ce qu'il n'est pas
 *
 * Il ne décide **rien**. Toute la règle vit dans `hmi::resolveTransition` — table pure, sans Qt,
 * couverte par ses tests — et ce routeur ne fait que l'appeler et diffuser le résultat. C'est ce
 * qui permet de vérifier la navigation sans ouvrir une fenêtre, et qui l'a déjà été.
 *
 * **Une transition non déclarée est refusée**, jamais silencieusement acceptée :
 * `resolveTransition` rend alors `std::nullopt` et l'état ne bouge pas. Sans cette discipline, un
 * `openOptions()` appelé depuis un écran d'où les options ne s'ouvrent pas produirait un état que
 * la table ne décrit pas, et dont personne ne saurait comment revenir.
 *
 * ## Pourquoi une énumération et non un nom de fichier
 *
 * Le routeur publie un **état**, pas un chemin. Faire transiter « `Screens/MainMenu.qml` » ferait
 * connaître à la présentation le nom des fichiers de la conception — que l'artiste doit pouvoir
 * renommer sans toucher au C++. La correspondance entre état et fichier vit donc en QML, dans
 * `Logic/ScreenStack.qml`, du côté développeur mais du bon côté de la frontière.
 */
class ScreenRouter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Screen currentScreen READ currentScreen NOTIFY changed)
    Q_PROPERTY(RpgScreen currentRpgScreen READ currentRpgScreen NOTIFY changed)

    /// La voie par laquelle la démo s'est terminée (`LOT-119`), que l'écran de fin dit. Le routeur
    /// la **transporte**, comme `dialogueId` : c'est le dialogue qui la nomme. Vide sinon.
    Q_PROPERTY(QString ending READ ending NOTIFY changed)
    /// La même voie, dite dans la langue active (clé `ending.arene` pour `arene`), vide sans voie. Une lecture du
    /// catalogue, pas une décision : l'écran de fin n'a pas d'autre modèle à qui la demander.
    Q_PROPERTY(QString endingText READ endingText NOTIFY changed)

    /// Le dialogue que l'écran de dialogue doit jouer (`LOT-09`). Le routeur le **transporte** :
    /// c'est la carte qui le nomme, en ouvrant la conversation du PNJ visé, et l'écran n'a plus de
    /// dialogue écrit en dur. Vide avant la première conversation.
    Q_PROPERTY(QString dialogueId READ dialogueId NOTIFY changed)

    /// Vrai dans un binaire de developpement, faux dans un binaire livre. Ce qui s'y adosse est un
    /// outil de verification -- le selecteur d'ecrans de `Logic/ScreenProbe.qml` --, et un outil de
    /// verification ne doit pas pouvoir partir avec le jeu. Une liaison QML sur cette propriete le
    /// garantit a la construction, la ou une consigne de relecture ne garantit rien.
    Q_PROPERTY(bool developerBuild READ developerBuild CONSTANT)

public:
    /// Écran affiché. Reprend `hmi::ScreenId`.
    enum class Screen {
        Menu,
        Game,
        Options,
        Pause,
        Credits,
        RpgScreen,
        /// Le Colisée (`LOT-50`).
        Arena,
        /// L'écran de mort (`LOT-119`).
        Death,
        /// L'écran « Fin de la démo » (`LOT-119`).
        DemoEnd,
    };
    Q_ENUM(Screen)

    /// Lequel des huit écrans du RPG est ouvert, quand `currentScreen` vaut `RpgScreen`.
    enum class RpgScreen {
        CharacterSheet,
        Skills,
        Inventory,
        QuestJournal,
        WorldMap,
        Dialogue,
        Merchant,
        Company,
        CombatHud,
    };
    Q_ENUM(RpgScreen)

    explicit ScreenRouter(QObject* parent = nullptr);

    [[nodiscard]] Screen currentScreen() const noexcept;
    [[nodiscard]] RpgScreen currentRpgScreen() const noexcept;

    [[nodiscard]] static bool developerBuild() noexcept;

    Q_INVOKABLE void openMenu();
    Q_INVOKABLE void openGame();
    Q_INVOKABLE void openOptions();
    Q_INVOKABLE void closeOptions();
    Q_INVOKABLE void openPause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void quitToMenu();
    Q_INVOKABLE void openCredits();
    Q_INVOKABLE void closeCredits();

    /// Ouvre le Colisée depuis le menu, et en revient. Un mode du jeu, pas un écran du RPG.
    Q_INVOKABLE void openArena();
    Q_INVOKABLE void closeArena();

    /// @brief Ouvre l'écran de dialogue **sur** @p dialogueId : le dialogue du PNJ à qui l'on
    ///        parle, jamais un identifiant écrit dans l'écran (`LOT-09`).
    Q_INVOKABLE void openDialogue(const QString& dialogueId);

    [[nodiscard]] QString dialogueId() const {
        return _dialogueId;
    }

    /// Le héros est tombé : l'écran de mort (`LOT-119`).
    Q_INVOKABLE void openDeath();
    /// @brief La démo est bouclée par la voie @p ending : l'écran de fin (`LOT-119`).
    Q_INVOKABLE void openDemoEnd(const QString& ending);

    [[nodiscard]] QString ending() const {
        return _ending;
    }
    [[nodiscard]] QString endingText() const;

    /// Ouvre un écran du RPG. L'écran d'où l'on vient est retenu par la table : refermer y revient,
    /// qu'on soit venu du menu, du jeu ou de la pause.
    Q_INVOKABLE void openRpgScreen(RpgScreen screen);
    Q_INVOKABLE void closeRpgScreen();

    /// Passe à l'écran suivant/précédent du RPG **sans repasser par le menu** (`EX-IHM-090`).
    /// C'est le geste des gâchettes de la manette.
    Q_INVOKABLE void nextRpgScreen();
    Q_INVOKABLE void previousRpgScreen();

signals:
    /// Émis quand l'écran courant change. Rien n'est émis si la transition a été **refusée** :
    /// l'interface ne doit pas se rafraîchir pour un geste que la table n'autorise pas.
    void changed();

private:
    /// Applique @p event si la table l'autorise. @return `true` si l'état a changé.
    bool apply(ScreenEvent event);

    ScreenState _state{};
    RpgScreenId _rpgScreen = RpgScreenId::CharacterSheet;
    QString _dialogueId;
    QString _ending;
};

}  // namespace hmi
