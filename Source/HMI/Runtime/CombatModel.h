// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core/Combat/Arena.h"
#include "Core/Combat/EnemyAi.h"
#include "HMI/Game/CombatContestants.h"

/**
 * @file HMI/Runtime/CombatModel.h
 * @brief Un combat tel qu'un écran le joue (`LOT-24`) : la partie commune, que le combat sur la
 *        carte dérive (`hmi::EncounterModel`, `LOT-118`) ; l'écran du Colisée est retiré.
 */

namespace hmi {

/**
 * @brief La vue-modèle d'un combat monté : le curseur, les actions du tour, ce que la grille
 *        montre, les gestes du joueur, les tours de l'IA.
 *
 * ## Ce qu'elle tient, et ce qu'elle ne décide pas
 *
 * Elle tient une `core::ArenaSession` — la machine à états du combat — et le curseur de ciblage ;
 * chaque geste de l'écran devient un appel à la session, et ce que l'écran affiche est relu de la
 * session après chaque geste. Elle ne décide **rien** du combat.
 *
 * Elle ne sait pas d'où vient le combat : le Colisée compose deux camps et une carte d'arène,
 * la carte d'exploration monte une rencontre sur sa zone de combat. Les deux dérivent d'elle et
 * n'ajoutent que cela — sans quoi l'interface de combat aurait été écrite deux fois, et les deux
 * copies auraient divergé au premier réglage (le critère du `LOT-118` : aucune régression du
 * Colisée).
 *
 * ## Signaux
 *
 * `changed` couvre tout ce que l'interface affiche ; `cursorChanged` ce que le curseur montre
 * (il suit aussi `changed`) ; `combatSceneChanged` n'est émis qu'aux gestes qui mutent la grille,
 * pour que la surface de rendu ne reprenne un instantané que lorsqu'il le faut.
 */
class CombatModel : public QObject {
    Q_OBJECT
    /// Ce que l'écran dit du dernier geste : un jet, un refus, l'issue.
    Q_PROPERTY(QString status READ status NOTIFY changed)
    /// Vrai entre le montage et le retour à la mise en place ou à l'exploration.
    Q_PROPERTY(bool inCombat READ inCombat NOTIFY changed)
    /// Vrai quand le combat a une issue.
    Q_PROPERTY(bool ended READ ended NOTIFY changed)
    Q_PROPERTY(int gridColumns READ gridColumns NOTIFY changed)
    Q_PROPERTY(int gridRows READ gridRows NOTIFY changed)
    /// Les combattants sur la grille : case, emprise, camp, tour, à terre, points de vie.
    Q_PROPERTY(QVariantList fighters READ fighters NOTIFY changed)
    /// Les cases où le combattant actif peut finir son déplacement.
    Q_PROPERTY(QVariantList reachableCells READ reachableCells NOTIFY changed)
    Q_PROPERTY(int cursorColumn READ cursorColumn NOTIFY cursorChanged)
    Q_PROPERTY(int cursorRow READ cursorRow NOTIFY cursorChanged)
    /// Le chemin que le déplacement vers le curseur emprunterait.
    Q_PROPERTY(QVariantList pathCells READ pathCells NOTIFY cursorChanged)
    /// Les actions du tour du joueur : attaques, puis celles du Manuel, puis la réaction.
    Q_PROPERTY(QVariantList turnActions READ turnActions NOTIFY cursorChanged)
    Q_PROPERTY(QVariantList turnOrder READ turnOrder NOTIFY changed)
    Q_PROPERTY(QString activeName READ activeName NOTIFY changed)
    Q_PROPERTY(QString activeResources READ activeResources NOTIFY changed)
    Q_PROPERTY(QStringList journal READ journal NOTIFY changed)

public:
    explicit CombatModel(QObject* parent = nullptr);
    ~CombatModel() override;

    [[nodiscard]] QString status() const {
        return _status;
    }
    [[nodiscard]] bool inCombat() const noexcept {
        return _inCombat;
    }
    [[nodiscard]] bool ended() const;
    [[nodiscard]] int gridColumns() const;
    [[nodiscard]] int gridRows() const;
    [[nodiscard]] QVariantList fighters() const;
    [[nodiscard]] QVariantList reachableCells() const;
    [[nodiscard]] int cursorColumn() const noexcept {
        return _cursor.column;
    }
    [[nodiscard]] int cursorRow() const noexcept {
        return _cursor.row;
    }
    [[nodiscard]] QVariantList pathCells() const;
    [[nodiscard]] QVariantList turnActions() const;
    [[nodiscard]] QVariantList turnOrder() const;
    [[nodiscard]] QString activeName() const;
    [[nodiscard]] QString activeResources() const;
    [[nodiscard]] QStringList journal() const;

    /**
     * @brief La session, en lecture seule, pour la surface de rendu.
     *
     * Pas une propriété QML : le QML n'a rien à lire d'une session. Le rendu ne la lit que dans
     * `synchronize()`, fil graphique bloqué, et n'en garde qu'un instantané en valeurs.
     * @return La session, `nullptr` tant que rien n'est monté.
     */
    [[nodiscard]] const core::ArenaSession* session() const noexcept {
        return _session.get();
    }

    /// @brief Le clic ou la confirmation sur une case : attaquer l'ennemi qui l'occupe, sinon s'y
    ///        déplacer (`LOT-24`).
    Q_INVOKABLE void tapCell(int column, int row);
    /// @brief Déplace le curseur de ciblage de @p columns et @p rows cases (clavier, manette).
    Q_INVOKABLE void moveCursor(int columns, int rows);
    /// @brief Pose le curseur sur une case (survol de la souris) ; hors grille, rien.
    Q_INVOKABLE void pointCursor(int column, int row);
    /// @brief Ramène le curseur sur le combattant actif.
    Q_INVOKABLE void centerCursor();
    /// @brief Passe à l'ennemi debout suivant (@p step > 0) ou précédent, du plus proche au plus
    ///        loin.
    Q_INVOKABLE void cycleTarget(int step);
    /// @brief Choisit l'action de rang @p index dans la barre.
    Q_INVOKABLE void selectAction(int index);
    /// @brief Passe à l'action suivante ou précédente de la barre, en boucle.
    Q_INVOKABLE void cycleAction(int step);
    /// @brief Confirme l'action choisie sur la case du curseur.
    Q_INVOKABLE void confirm();
    /// @brief Esquive : dépense l'action du tour du joueur (`core::ArenaSession::dodge`) ; si elle
    ///        l'est déjà, `status` le dit.
    Q_INVOKABLE void dodge();
    /// @brief Désengagement : dépense l'action du tour ; si elle l'est déjà, `status` le dit.
    Q_INVOKABLE void disengage();
    /// @brief Sprint : dépense l'action du tour ; si elle l'est déjà, `status` le dit.
    Q_INVOKABLE void dash();
    /// @brief Termine le tour du joueur ; l'IA joue ensuite les siens.
    Q_INVOKABLE void endTurn();
    /// @brief Le joueur se retire, si la rencontre le permet.
    Q_INVOKABLE void withdraw();

signals:
    /// L'état du combat a changé en bloc : combattants, ordre de tour, journal, statut.
    void changed();
    /// Le curseur de ciblage a bougé : sa case, le chemin et les actions qui en dépendent.
    void cursorChanged();
    /// Les figurines du combat sont à recomposer : la surface de rendu se redessine.
    void combatSceneChanged();

protected:
    /// @return Les profils de l'IA que `core::playTurn` lit ; `nullptr` : l'IA ne joue pas.
    [[nodiscard]] virtual const core::BehaviorCatalog* behaviors() const = 0;
    /**
     * @brief Joue les tours de l'IA jusqu'au prochain tour du joueur, ou la fin.
     *
     * Le Colisée les joue d'un bloc ; le combat sur la carte les joue **un par un**, au rythme
     * de la file des mouvements — c'est ce que sa surcharge fait.
     */
    virtual void playAiTurns();
    /// @return Vrai si un geste du joueur est reçu maintenant ; faux pendant qu'une animation joue.
    [[nodiscard]] virtual bool acceptsInput() const {
        return true;
    }
    /// @brief Joue **un** tour de l'IA. @return Vrai s'il y en avait un à jouer.
    bool playOneAiTurn();
    /// @brief Émet `changed` et `combatSceneChanged`.
    void emitSceneChanged();
    /// @brief Écrit dans `status` les refus d'un montage.
    void refreshMessage(const core::ArenaMount& mount);
    /// @brief Le curseur suit le combattant actif quand il change.
    void followActive();
    /// @return Le combattant actif s'il est **au joueur** : debout, sans profil d'IA, en tour.
    [[nodiscard]] std::optional<core::CombatantId> playerTurn() const;
    void attackAt(core::CombatantId target, std::optional<std::size_t> index);
    void moveTo(core::GridPosition cell);
    /// @brief Le héros de la démo comme source de combattant ; `std::nullopt` sans fiche, et
    ///        @p problems dit pourquoi.
    [[nodiscard]] static std::optional<HeroContestantSource> loadHeroSource(
        std::vector<std::string>& problems);

    std::unique_ptr<core::ArenaSession> _session;
    core::GridPosition _cursor{};
    int _selectedAction = 0;
    /// Le combattant que le curseur suit : quand l'actif change, le curseur va sur lui.
    std::optional<core::CombatantId> _followed;
    bool _inCombat = false;
    QString _status;
};

}  // namespace hmi
