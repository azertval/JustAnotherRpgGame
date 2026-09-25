// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQmlIntegration>
#include <string>
#include <vector>

/**
 * @file HMI/Runtime/DebugConsoleModel.h
 * @brief La console de debug du jeu (`F9`) : les options du binaire, rejouées dans la partie.
 */

namespace hmi {

/**
 * @brief La vue-modèle de la console de debug (`Logic/DebugConsole.qml`).
 *
 * Tout ce que le jeu accepte sur sa ligne de commande — `--map=`, `--at=`, `--flags=`,
 * `--screen=`, `--window-size=`, `--screenshot=`… — se tape ici, dans la partie, et s'applique
 * **à chaud** : éprouver une fonctionnalité ne demande plus de quitter, retaper la ligne et
 * relancer. Ce qui ne se lit qu'au lancement (`--data=`, `--crash-test`) est dit tel, et
 * `relaunch()` redémarre le jeu avec la ligne tapée.
 *
 * Le catalogue et l'analyse des mots sont `hmi::debugOptionCatalog` et `hmi::splitCommandLine`
 * (`HMI/Game/DebugCommands.h`, sans Qt) ; ce modèle applique. Ce qu'il applique passe par les
 * **mêmes fonctions** que `App/Game/Main.cpp` (`hmi::WorldModel::setStartOverride`,
 * `setStartCell`, `setStartFlags`, `setLevelDirectories`, `setHeroFigure`) : la console ne
 * connaît aucun chemin que la ligne de commande n'aurait pas.
 *
 * Ce qui touche à la fenêtre — un écran forcé, une taille, une capture — est **demandé** par
 * signal : le modèle ne connaît pas la fenêtre, et c'est le QML (`Main.qml`) qui la tient.
 *
 * Sans effet dans un binaire livré (`core::DEVELOPER_BUILD`), comme le sélecteur d'écrans.
 */
class DebugConsoleModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /// Ce que la console a dit, dans l'ordre : les lignes tapées (précédées de `> `) et leurs
    /// réponses.
    Q_PROPERTY(QStringList transcript READ transcript NOTIFY transcriptChanged)
    /// Les lignes tapées, la plus récente en dernier : pour les flèches haut et bas.
    Q_PROPERTY(QStringList history READ history NOTIFY transcriptChanged)
    /// Le catalogue des options : `{name, syntax, description, live}`.
    Q_PROPERTY(QVariantList options READ options CONSTANT)

public:
    explicit DebugConsoleModel(QObject* parent = nullptr);

    [[nodiscard]] QStringList transcript() const {
        return _transcript;
    }
    [[nodiscard]] QStringList history() const {
        return _history;
    }
    [[nodiscard]] static QVariantList options();

    /// @brief Exécute @p line : une suite d'options, comme sur la ligne de commande, ou `aide`.
    Q_INVOKABLE void run(const QString& line);

    /**
     * @brief Redémarre le jeu avec @p line pour ligne de commande, et quitte celui-ci.
     * @return Faux si le nouveau processus n'a pas pu partir ; le jeu reste alors ouvert.
     */
    Q_INVOKABLE bool relaunch(const QString& line);

    /// @brief Ajoute une ligne au compte rendu — ce que le QML a fait d'une demande (une capture
    ///        écrite, une taille refusée).
    Q_INVOKABLE void say(const QString& line);

    /// @brief Efface le compte rendu ; l'historique reste.
    Q_INVOKABLE void clear();

signals:
    void transcriptChanged();
    /// `--screen=<name>` : la pile d'écrans épingle cet écran.
    void screenRequested(const QString& name);
    /// `--window-size=` : la fenêtre prend cette taille.
    void windowSizeRequested(int width, int height);
    /// `--screenshot=` : la fenêtre se capture dans ce fichier.
    void screenshotRequested(const QString& path);
    /// Une carte s'est ouverte : la vue de jeu doit paraître (`ScreenRouter.jumpToGame`).
    void gameRequested();

private:
    /// @brief Applique les options de @p words, dans l'ordre que le lancement suivrait.
    void apply(const std::vector<std::string>& words);
    void printHelp();

    QStringList _transcript;
    QStringList _history;
};

}  // namespace hmi
