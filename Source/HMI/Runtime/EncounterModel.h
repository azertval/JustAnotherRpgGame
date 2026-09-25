// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <QtQmlIntegration>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/MapEncounter.h"
#include "HMI/Game/CombatCues.h"
#include "HMI/Game/FigureResolver.h"
#include "HMI/Runtime/CombatModel.h"

/**
 * @file HMI/Runtime/EncounterModel.h
 * @brief Le combat **sur la carte** (`LOT-118`) : une rencontre engagée pendant l'exploration se
 *        joue sur place — la carte se fige, la grille paraît, le combat se joue, l'exploration
 *        reprend.
 */

namespace hmi {

class WorldModel;

/**
 * @brief La vue-modèle du combat sur la carte.
 *
 * ## Ce qu'elle tient
 *
 * La partie commune du combat (`hmi::CombatModel`), plus ce que la carte y ajoute :
 *
 * - la **rencontre** montée sur la zone de combat de la carte courante
 *   (`core::prepareMapEncounter`), dont les cases sont celles de la grille tactique ; l'écran les
 *   ramène sur la carte par `zoneColumn` et `zoneRow` ;
 * - la **file des mouvements** (`hmi::CombatCueTrack`) : ce que la session décide se rejoue à la
 *   vitesse du monde — les pas, les coups, les chutes —, et tant qu'elle joue, `busy` est vrai et
 *   les gestes attendent. Les tours de l'IA se jouent **un par un**, chacun après que le précédent
 *   s'est vu ;
 * - les **figurines** des combattants, publiées à chaque pas dans `hmi::WorldModel`
 *   (`setCombatFigures`) : la surface de rendu de l'exploration les dessine sur la carte gelée,
 *   par le même pipeline. Un combattant sans figurine prend son mannequin (`LOT-145`).
 *
 * ## Un singleton, comme la partie
 *
 * L'écran de combat (`CombatHud.qml`) se construit et se détruit avec la pile d'écrans ; la
 * rencontre, elle, est un état de la **partie** (`hmi::WorldModel`), et survit à l'ouverture
 * d'un autre écran par-dessus.
 *
 * ## Les issues
 *
 * `outcome` se publie quand le combat a une issue **et** que la file a fini de la montrer :
 * `victory`, `defeat` ou `flight`. `leave()` rend alors l'exploration : le drapeau d'une victoire
 * est posé (`core::endEncounter`), le héros reste où le combat l'a laissé, la carte dégèle.
 */
class EncounterModel : public CombatModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    /// Vrai d'un `begin` réussi jusqu'à `leave`.
    Q_PROPERTY(bool active READ active NOTIFY changed)
    /// Vrai tant qu'un mouvement se joue : les gestes attendent, l'écran le montre.
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    /// `victory`, `defeat`, `flight` ; vide tant que le combat dure ou que la file joue.
    Q_PROPERTY(QString outcome READ outcome NOTIFY changed)
    Q_PROPERTY(QString encounterName READ encounterName NOTIFY changed)
    /// L'origine de la grille tactique sur la carte : ce que l'écran ajoute à chaque case.
    Q_PROPERTY(int zoneColumn READ zoneColumn NOTIFY changed)
    Q_PROPERTY(int zoneRow READ zoneRow NOTIFY changed)
    /// Le héros : ses points de vie en clair, et leur part.
    Q_PROPERTY(QString heroName READ heroName NOTIFY changed)
    Q_PROPERTY(QString heroHitPoints READ heroHitPoints NOTIFY changed)
    Q_PROPERTY(qreal heroHitPointsRatio READ heroHitPointsRatio NOTIFY changed)
    /// Le combattant sous le curseur : `name`, `side`, `hitPoints`, `hitPointsRatio`,
    /// `armorClass`, `speed`, `conditions` ; vide si la case est libre.
    Q_PROPERTY(QVariantMap target READ target NOTIFY cursorChanged)
    /// La graine du prochain combat ; 0 : tirée à l'horloge. Le journal dit celle qui a servi.
    Q_PROPERTY(int seed READ seed WRITE setSeed NOTIFY changed)

public:
    /// Pas fixe de la file des mouvements, en millisecondes : celui de l'exploration.
    static constexpr int STEP_MILLISECONDS = 16;

    explicit EncounterModel(QObject* parent = nullptr);
    ~EncounterModel() override;

    /// @return Le combat sur la carte en cours, s'il y en a un (le dernier construit).
    [[nodiscard]] static EncounterModel* current() noexcept;

    /**
     * @brief Engage la rencontre @p encounterId sur la carte courante de la partie
     *        (`hmi::WorldModel::current`), au point de la dernière interaction du héros.
     * @return Vrai si le combat est monté ; sinon `status` dit pourquoi, et l'exploration
     *         continue comme si de rien n'était (`EX-NFR-040`).
     */
    Q_INVOKABLE bool begin(const QString& encounterId);
    /// @brief Le combat a une issue : rend l'exploration, pose ce qu'une victoire acquiert.
    Q_INVOKABLE void leave();
    /// @brief Joue instantanément les mouvements en attente : l'image rejoint la grille.
    Q_INVOKABLE void skipAnimations();

    /**
     * @brief Avance la file des mouvements de @p seconds, joue le tour de l'IA qui attend, publie
     *        les figurines. L'horloge l'appelle à chaque pas ; un test l'appelle lui-même.
     */
    void tick(float seconds);

    /**
     * @brief Remplace la racine du **contenu** — le bestiaire et les rencontres (`Rpg/creatures`,
     *        `Rpg/encounters`) : celle de l'exécutable par défaut, une racine d'essai dans les
     *        tests. Les profils de l'IA (`Rpg/rules`) se lisent toujours à côté de l'exécutable.
     */
    void setContentRoot(std::filesystem::path root);

    [[nodiscard]] bool active() const noexcept {
        return _inCombat;
    }
    [[nodiscard]] bool busy() const noexcept {
        return _cues.busy();
    }
    [[nodiscard]] QString outcome() const {
        return _outcome;
    }
    [[nodiscard]] QString encounterName() const;
    [[nodiscard]] int zoneColumn() const noexcept;
    [[nodiscard]] int zoneRow() const noexcept;
    [[nodiscard]] QString heroName() const;
    [[nodiscard]] QString heroHitPoints() const;
    [[nodiscard]] qreal heroHitPointsRatio() const;
    [[nodiscard]] QVariantMap target() const;
    [[nodiscard]] int seed() const noexcept {
        return _seed;
    }
    void setSeed(int seed);

    /// @return Le montage en cours, pour qui vérifie sans fenêtre.
    [[nodiscard]] const core::MapEncounterSetup* setup() const noexcept {
        return _setup.has_value() ? &*_setup : nullptr;
    }
    /// @return La file des mouvements, en lecture.
    [[nodiscard]] const CombatCueTrack& cues() const noexcept {
        return _cues;
    }

signals:
    /// Le combat est quitté (`leave`), avec son issue : `victory`, `defeat` ou `flight`.
    void finished(const QString& outcome);

protected:
    [[nodiscard]] const core::BehaviorCatalog* behaviors() const override;
    /// Sur la carte, l'IA ne joue pas d'un bloc : `tick` joue un tour quand la file est vide.
    void playAiTurns() override;
    [[nodiscard]] bool acceptsInput() const override {
        return !_cues.busy();
    }

private:
    struct Catalogs;
    /// Ce qu'un combattant dessine.
    struct Binding {
        std::string directory;
        bool oriented = false;
        bool hero = false;
    };

    /// Lit les catalogues à la première rencontre ; @return faux s'il manque l'essentiel.
    bool ensureCatalogs();
    /// Ouvre la session sur la grille de la zone, y monte le héros et les créatures de la
    /// rencontre.
    /// @return Le montage : les camps tels que la session les a numérotés.
    core::ArenaMount mountBout();
    /// Associe à chaque combattant monté la figurine qu'il dessine.
    /// @param world Le modèle du monde, qui résout les figurines.
    /// @param mount Le montage de la session.
    void bindFigures(WorldModel& world, const core::ArenaMount& mount);
    /// Retient le héros du montage, ou défait la session quand un camp est vide.
    /// @param mount Le montage de la session.
    /// @return Faux si un camp est vide : rien à engager.
    bool keepMount(const core::ArenaMount& mount);
    /// Pose la file des mouvements sur les cases de départ des combattants.
    void placeCues();
    /// Branche la file des mouvements sur la session montée.
    void subscribeCues();
    /// Publie les figurines des combattants dans le monde.
    void publishFigures();
    /// Publie l'issue quand le combat en a une et que la file l'a montrée.
    void settleOutcome();
    void step();
    /// Défait tout : la session, la file, les figurines publiées, le gel de la carte.
    void teardown();

    std::filesystem::path _contentRoot;
    std::unique_ptr<Catalogs> _catalogs;
    std::optional<core::MapEncounterSetup> _setup;
    std::string _encounterName;
    std::map<core::CombatantId, Binding> _bindings;
    std::optional<core::CombatantId> _hero;
    CombatCueTrack _cues;
    QTimer _clock;
    QString _outcome;
    int _seed = 0;
};

}  // namespace hmi
