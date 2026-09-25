// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQmlIntegration>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core/Combat/Arena.h"
#include "Core/Rpg/Bestiary.h"
#include "HMI/Runtime/CombatModel.h"

/**
 * @file HMI/Runtime/ArenaModel.h
 * @brief Le Colisée, tel que l'écran de mise en place le voit et le commande (`LOT-50`).
 */

namespace hmi {

/**
 * @brief La vue-modèle de l'arène : composer deux camps, lancer, jouer, rejouer.
 *
 * ## Ce qu'elle tient, et ce qu'elle ne décide pas
 *
 * Elle tient les catalogues dont l'écran compose son affrontement : le bestiaire (`LOT-33`), le
 * héros de la démo (`LOT-112`), les arènes et les Marques Héroïques. Le combat lui-même — le
 * curseur, les actions, les gestes, les tours de l'IA — est celui de `hmi::CombatModel`, commun au
 * combat sur la carte (`LOT-118`) : ici ne vit que ce qui est propre au banc d'essai du Colisée.
 *
 * ## Signal et notification de changement
 *
 * `changed` couvre tout ce que l'interface affiche : la composition, la grille, l'ordre, le journal
 * et l'issue changent ensemble, et les distinguer n'épargnerait aucun rafraîchissement à ces
 * propriétés, toutes relues d'un coup.
 *
 * La surface de rendu (`hmi::ArenaViewportItem`, `LOT-86` Phase 5+) n'a besoin de reprendre un
 * instantané que lorsque la grille de combat elle-même a pu changer — pas à chaque geste de
 * composition (enrôler, retirer, marquer, régler la graine ou l'IA), qui ne touche encore à aucune
 * session montée. `combatSceneChanged`, émis en plus de `changed` aux seuls gestes qui mutent la
 * grille (`core::ArenaSession::mount`, un déplacement, une attaque, un retrait, la fin du tour, un
 * rejeu), lui épargne ces recompositions inutiles.
 */
class ArenaModel : public CombatModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString arenaName READ arenaName NOTIFY changed)
    /// Tout ce qui peut s'enrôler : le personnage, puis le bestiaire.
    Q_PROPERTY(QVariantList roster READ roster NOTIFY changed)
    Q_PROPERTY(QVariantList allies READ allies NOTIFY changed)
    Q_PROPERTY(QVariantList enemies READ enemies NOTIFY changed)
    Q_PROPERTY(QStringList marks READ marks NOTIFY changed)
    Q_PROPERTY(int seed READ seed WRITE setSeed NOTIFY changed)
    /// Les ennemis sont-ils joués par l'IA tactique (`LOT-23`) ? Sinon à la main.
    Q_PROPERTY(bool enemyAi READ enemyAi WRITE setEnemyAi NOTIFY changed)

public:
    /**
     * @brief Construit la vue-modèle et lit ses catalogues.
     * @param parent      Parent Qt.
     * @param contentRoot Racine du **contenu** — les arènes (`World/arena`) et leurs cartes
     *                    (`Levels/`). Vide : à côté de l'exécutable, comme le jeu. Les tests y
     *                    mettent la racine d'essai, faute de contenu livré depuis le `LOT-102`.
     */
    explicit ArenaModel(QObject* parent = nullptr, std::filesystem::path contentRoot = {});
    ~ArenaModel() override;

    [[nodiscard]] QString arenaName() const;
    [[nodiscard]] QVariantList roster() const;
    [[nodiscard]] QVariantList allies() const;
    [[nodiscard]] QVariantList enemies() const;
    [[nodiscard]] QStringList marks() const;
    [[nodiscard]] int seed() const noexcept;
    void setSeed(int seed);
    [[nodiscard]] bool enemyAi() const noexcept;
    void setEnemyAi(bool enabled);

    Q_INVOKABLE void addAlly(const QString& id);
    Q_INVOKABLE void addEnemy(const QString& id);
    Q_INVOKABLE void removeAlly(int index);
    Q_INVOKABLE void removeEnemy(int index);
    Q_INVOKABLE void assignMark(bool ally, int index, const QString& markId);
    /// @brief Monte les deux camps et ouvre le combat.
    Q_INVOKABLE void launch();
    /// @brief Rejoue le même affrontement à la même graine (`EX-CBT-050`).
    Q_INVOKABLE void replay();
    /// @brief Revient à la mise en place, session neuve.
    Q_INVOKABLE void backToSetup();

protected:
    [[nodiscard]] const core::BehaviorCatalog* behaviors() const override;

private:
    struct Fighter;
    struct Catalogs;

    void loadCatalogs();
    [[nodiscard]] bool loadPlayableLevel();
    [[nodiscard]] std::optional<Fighter> fighterFor(const QString& id, core::CombatSide side) const;
    [[nodiscard]] core::ArenaBout composeBout() const;
    void resetSession();

    std::filesystem::path _contentRoot;
    std::unique_ptr<Catalogs> _catalogs;
    std::vector<Fighter> _allies;
    std::vector<Fighter> _enemies;
    int _seed = 2026;
    bool _enemyAi = true;
};

}  // namespace hmi
