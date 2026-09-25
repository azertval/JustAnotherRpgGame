// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQmlIntegration>
#include <filesystem>
#include <vector>

/**
 * @file HMI/Runtime/MapLauncherModel.h
 * @brief Le lanceur de cartes : un **outil de debug** qui liste les cartes et en ouvre une dans
 *        la partie, avec les mêmes réglages que la ligne de commande.
 */

namespace hmi {

/**
 * @brief La vue-modèle du lanceur de cartes (`Tools/MapLauncher.qml`, `--screen=MapLauncher`).
 *
 * Il remplace l'écran du Colisée (`LOT-50`), retiré le 25 septembre 2026 : depuis que le combat
 * se joue sur la carte (`LOT-118`), ce qu'il faut pour éprouver une fonctionnalité n'est plus un
 * banc d'essai à part, mais **ouvrir la carte qu'on vient de dessiner**, au bon endroit, dans le
 * bon état. C'est ce que `--map=`, `--at=`, `--flags=`, `--hero-figure=` et `--levels=` font au
 * lancement ; ce modèle le fait depuis le jeu, sans relancer.
 *
 * Les cartes viennent du `Levels/` du contenu (`hmi::dataDirectory`), précédé des dossiers que
 * l'utilisateur ajoute — les brouillons de l'éditeur, comme `--levels=`. Lancer passe par
 * `hmi::WorldModel` exactement comme `Main.cpp` : mêmes réglages, mêmes fonctions, donc ce qui
 * marche ici marche sur la ligne de commande, et inversement.
 *
 * Pas un écran du jeu : aucun formulaire dans `Jadg.Ui`, aucune transition dans
 * `hmi::ScreenFlow`. Il s'ouvre par `--screen=`, par le sélecteur de développement ou par la
 * console de debug (`F9`).
 */
class MapLauncherModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// Les cartes trouvées, dans l'ordre des identifiants : `{id, name, columns, rows, directory,
    /// file, error}`. `error` non vide : la carte ne se charge pas, et dit pourquoi.
    Q_PROPERTY(QVariantList maps READ maps NOTIFY changed)
    /// Les dossiers ajoutés par l'utilisateur, lus **avant** celui du contenu.
    Q_PROPERTY(QStringList directories READ directories NOTIFY changed)
    /// Le `Levels/` du contenu, toujours lu en dernier.
    Q_PROPERTY(QString levelsRoot READ levelsRoot CONSTANT)
    /// Ce que le dernier geste a donné : carte ouverte, dossier absent, case refusée…
    Q_PROPERTY(QString status READ status NOTIFY changed)

public:
    explicit MapLauncherModel(QObject* parent = nullptr);

    [[nodiscard]] QVariantList maps() const {
        return _maps;
    }
    [[nodiscard]] QStringList directories() const;
    [[nodiscard]] static QString levelsRoot();
    [[nodiscard]] QString status() const {
        return _status;
    }

    /// @brief Relit les dossiers et recharge la liste.
    Q_INVOKABLE void refresh();

    /// @brief Ajoute un dossier de cartes devant les autres. @return Faux s'il n'existe pas.
    Q_INVOKABLE bool addDirectory(const QString& path);
    /// @brief Retire le dossier de rang @p index.
    Q_INVOKABLE void removeDirectory(int index);

    /**
     * @brief Ouvre @p mapId dans la partie, comme `--map=<mapId>@<arrival> --at=<at>
     *        --flags=<flags> --hero-figure=<heroFigure>` l'aurait fait au lancement.
     *
     * Les champs vides ne posent rien. Une case illisible ou hors carte ne fait pas échouer le
     * lancement : le héros part de l'entrée et `status` le dit (`EX-NFR-040`).
     * @return Vrai si la carte s'est ouverte ; l'appelant bascule alors sur la vue de jeu.
     */
    Q_INVOKABLE bool launch(const QString& mapId, const QString& arrival, const QString& at,
                            const QString& flags, const QString& heroFigure);

signals:
    void changed();

private:
    /// Les dossiers lus, dans l'ordre : ceux de l'utilisateur, puis le `Levels/` du contenu.
    [[nodiscard]] std::vector<std::filesystem::path> allDirectories() const;
    void setStatus(QString status);

    std::vector<std::filesystem::path> _directories;
    QVariantList _maps;
    QString _status;
};

}  // namespace hmi
