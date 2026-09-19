// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/WorldPlay.h
 * @brief Une carte qu'on parcourt, et ce qu'il faut pour la dessiner — sans Qt.
 *
 * Le jeu (`hmi::WorldModel`) et l'essai immédiat de l'éditeur (`hmi::EditorViewport`) jouent la
 * même exploration : une `core::ExplorationSession`, la table d'apparence du lieu de la carte, et
 * les figurines qu'on pose dessus. Deux copies de cette mise en scène divergeraient au premier
 * réglage — l'essai montrerait alors une carte que le jeu ne montre pas, ce qui est exactement ce
 * qu'un essai ne doit jamais faire (`EX-EDIT-055`). D'où ce type, sans QObject ni horloge : chaque
 * appelant garde sa cadence et ses signaux, et partage le reste.
 */

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Core/World/ExplorationSession.h"
#include "Core/World/WorldTravel.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

/// @brief Ce qu'un pas a changé à l'image, en plus des événements de la session.
struct WorldPlayStep {
    std::vector<core::ExplorationEvent> events;
    /// Le héros a bougé, ou marche contre un mur : la caméra suit, la scène se redessine.
    bool heroMoved = false;
    /// La scène doit être recomposée : carte changée, ou bande du héros basculée.
    bool sceneChanged = false;
};

class WorldPlay {
public:
    /// Figurine du héros tant que l'appelant n'en nomme pas d'autre.
    static constexpr std::string_view DEFAULT_HERO_FIGURE = "jade";

    /**
     * @param loader          Chargeur des cartes (`core::WorldTravel::directoryLoader` en jeu ; en
     *                        essai, un chargeur qui sert d'abord le brouillon de l'éditeur).
     * @param assetsDirectory Dossier des assets : les tables d'apparence s'y lisent sous
     *                        `Scene/<lieu>/appearance.json`.
     */
    WorldPlay(core::WorldTravel::MapLoader loader, std::filesystem::path assetsDirectory);

    /**
     * @brief Entre sur @p mapId au point d'arrivée @p arrival (vide : l'entrée de la carte).
     * @return Vrai si la carte s'ouvre ; la session dit sinon pourquoi.
     */
    bool enter(std::string_view mapId, std::string_view arrival);

    /// @brief Avance d'un pas de @p seconds avec l'intention @p intent.
    WorldPlayStep step(const core::ExplorationIntent& intent, float seconds);

    [[nodiscard]] core::ExplorationSession& session() noexcept {
        return _session;
    }
    [[nodiscard]] const core::ExplorationSession& session() const noexcept {
        return _session;
    }

    [[nodiscard]] const std::string& heroFigure() const noexcept {
        return _heroFigure;
    }
    void setHeroFigure(std::string figure) {
        _heroFigure = std::move(figure);
    }

    /// @return Les figurines de la carte courante : les PNJ, puis le héros, qui passe devant.
    [[nodiscard]] std::vector<WorldFigureSnapshot> figures() const;

    /// @return L'instantané que `hmi::WorldSceneRenderer` dessine ; vide hors carte.
    [[nodiscard]] WorldSceneSnapshot snapshot() const;
    [[nodiscard]] float diamondRatio() const noexcept {
        return _appearance.diamondRatio();
    }

private:
    /// @brief Relit la table d'apparence du lieu de la carte courante.
    void reloadAppearance();

    core::ExplorationSession _session;
    std::filesystem::path _assetsDirectory;
    PlaceAppearance _appearance;
    std::string _heroFigure{DEFAULT_HERO_FIGURE};
    /// Temps écoulé sur la carte : l'image des bandes de figurine en dépend.
    float _elapsed = 0.0F;
    /// Le héros marche : sa bande est `walk`, sinon `idle`.
    bool _walking = false;
};

}  // namespace hmi
