// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/WorldPlay.h"

#include <utility>
#include <variant>

#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Rpg/Dialogue.h"
#include "HMI/HmiLog.h"

namespace hmi {

namespace {

/// Durée d'une image des bandes de figurine, en secondes (`idle.anim.json`, `frameDuration`).
constexpr float FIGURE_FRAME_SECONDS = 0.15F;

}  // namespace

WorldPlay::WorldPlay(core::WorldTravel::MapLoader loader, std::filesystem::path assetsDirectory)
    : _session(std::move(loader)), _assetsDirectory(std::move(assetsDirectory)) {}

bool WorldPlay::enter(std::string_view mapId, std::string_view arrival) {
    if (!_session.start(mapId, arrival)) {
        return false;
    }
    _elapsed = 0.0F;
    _walking = false;
    reloadAppearance();
    return true;
}

WorldPlayStep WorldPlay::step(const core::ExplorationIntent& intent, float seconds) {
    WorldPlayStep result;
    if (_session.map() == nullptr) {
        return result;
    }
    const core::CellPoint before = _session.heroPoint();
    result.events = _session.update(intent, seconds);
    _elapsed += seconds;

    const bool walking = !_session.frozen() && (intent.move.x != 0.0F || intent.move.y != 0.0F);
    if (walking != _walking) {
        _walking = walking;
        result.sceneChanged = true;
    }
    // Un héros qui pousse contre un mur ne change pas de case, mais sa bande continue de tourner :
    // la scène doit se redessiner autant que s'il avait bougé.
    result.heroMoved = _session.heroPoint() != before || walking;
    for (const core::ExplorationEvent& event : result.events) {
        if (event.kind == core::ExplorationEventKind::MapEntered) {
            _elapsed = 0.0F;
            reloadAppearance();
            result.sceneChanged = true;
            result.heroMoved = true;
        }
    }
    return result;
}

void WorldPlay::reloadAppearance() {
    const core::Level* const map = _session.map();
    if (map == nullptr) {
        _appearance = PlaceAppearance{};
        return;
    }
    const std::string place = scenePlaceOf(*map);
    if (place.empty()) {
        // Une carte qui ne nomme aucun lieu se joue en MAQUETTE (LOT-128) : la composition dessine
        // ses types en couleurs, ses murs en blocs et ses entites en jetons. Ce n'est plus un
        // manque a signaler, c'est l'etat de depart normal d'une carte.
        _appearance = PlaceAppearance{};
        HMI_LOG_INFO("Monde : la carte " + _session.mapId() +
                     " ne declare aucun lieu ; elle se joue en maquette.");
        return;
    }
    PlaceAppearanceResult read =
        PlaceAppearance::loadFromFile(_assetsDirectory / "Scene" / place / "appearance.json");
    if (!read.ok()) {
        HMI_LOG_WARNING("Monde : table d'apparence du lieu " + place + " illisible, " +
                        read.message);
        _appearance = PlaceAppearance{};
        return;
    }
    _appearance = std::move(read.appearance);
}

std::vector<WorldFigureSnapshot> WorldPlay::figures() const {
    const core::Level* const map = _session.map();
    if (map == nullptr) {
        return {};
    }
    const int frame = static_cast<int>(_elapsed / FIGURE_FRAME_SECONDS);

    // Les PNJ d'abord, le héros ensuite : à égalité de profondeur, c'est lui qui passe devant.
    std::vector<WorldFigureSnapshot> figures = npcFigures(map->entities(), frame);
    figures.push_back(
        WorldFigureSnapshot{.figure = _heroFigure,
                            .clip = _walking ? "walk" : "idle",
                            .point = {_session.heroPoint().column, _session.heroPoint().row},
                            .frame = frame});
    return figures;
}

WorldSceneSnapshot WorldPlay::snapshot() const {
    const core::Level* const map = _session.map();
    if (map == nullptr) {
        return WorldSceneSnapshot{};
    }
    return snapshotWorldScene(*map, _appearance, figures());
}

}  // namespace hmi
