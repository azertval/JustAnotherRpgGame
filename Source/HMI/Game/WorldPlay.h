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

#include <cstdint>
#include <filesystem>
#include <memory>
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
    /// La **carte** a changé : on est entré ailleurs, ou un drapeau fait paraître ou disparaître
    /// un PNJ, une porte (`LOT-116`, `LOT-126`). Elle est à recomposer.
    bool sceneChanged = false;
    /// Les **figurines** ont changé sans que le héros bouge : sa bande a basculé (il part, il
    /// s'arrête), ou il s'est tourné. La carte, elle, reste composée.
    bool figuresChanged = false;
};

class WorldPlay {
public:
    /// Figurine du héros tant que l'appelant n'en nomme pas d'autre : le Brawler pré-tiré, héros
    /// de la démo (`LOT-112`).
    static constexpr std::string_view DEFAULT_HERO_FIGURE = "Common/Characters/Heroes/brawler";

    /**
     * @param loader          Chargeur des cartes (`core::WorldTravel::directoryLoader` en jeu ; en
     *                        essai, un chargeur qui sert d'abord le brouillon de l'éditeur).
     * @param assetsDirectory Dossier des assets : la table et les pièces du lieu s'y lisent, à
     *                        chacun de ses niveaux (`hmi::PlaceAppearance::loadForPlace`).
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
    void setHeroFigure(std::string figure);

    /// @return L'orientation du héros ; `None` si sa figurine n'a pas de bandes orientées.
    [[nodiscard]] FigureFacing heroFacing() const noexcept {
        return _heroOriented ? _heroFacing : FigureFacing::None;
    }

    /// @return Les figurines de la carte courante : les PNJ, puis le héros, qui passe devant.
    [[nodiscard]] std::vector<WorldFigureSnapshot> figures() const;

    /**
     * @brief La **carte** que `hmi::WorldSceneRenderer` dessine, partagée ; vide hors carte.
     *
     * Refaite seulement quand elle change — entrée sur une carte, drapeau qui fait paraître ou
     * disparaître quelque chose, figurine du héros —, jamais à un pas du héros : c'est ce qui
     * rendait chaque pas aussi cher que la carte est grande (audit de l'affichage, A3). Ses
     * figurines sont celles de l'instant où elle a été faite ; celles de l'image sont `figures()`.
     */
    [[nodiscard]] std::shared_ptr<const WorldSceneSnapshot> scene() const;

    /// @return La carte **et** les figurines de l'instant, en une valeur : pour qui compose tout
    ///         d'un coup (un test, une capture).
    [[nodiscard]] WorldSceneSnapshot snapshot() const;
    [[nodiscard]] float diamondRatio() const noexcept {
        return _appearance.diamondRatio();
    }

private:
    /// @brief Relit la table d'apparence du lieu de la carte courante.
    void reloadAppearance();
    /// @brief La carte est à refaire à la prochaine demande.
    void invalidateScene() noexcept {
        _scene.reset();
    }

    core::ExplorationSession _session;
    std::filesystem::path _assetsDirectory;
    PlaceAppearance _appearance;
    std::string _heroFigure{DEFAULT_HERO_FIGURE};
    /// La figurine du héros a ses quatre orientations (`idle-se.png`…) : relu quand elle change,
    /// pas à chaque image.
    bool _heroOriented = false;
    /// Dernière orientation du héros : il la garde à l'arrêt.
    FigureFacing _heroFacing = FigureFacing::SouthEast;
    /// Temps écoulé sur la carte : l'image des bandes de figurine en dépend.
    float _elapsed = 0.0F;
    /// Le héros marche : sa bande est `walk`, sinon `idle`.
    bool _walking = false;
    /// Révision des drapeaux de la dernière scène annoncée : les PNJ conditionnés en dépendent.
    std::uint64_t _drawnFlags = 0;
    /// La carte en valeurs, faite à la première demande après un changement ; nulle d'ici là.
    mutable std::shared_ptr<const WorldSceneSnapshot> _scene;
};

}  // namespace hmi
