// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/WorldPlay.h"

#include <system_error>
#include <utility>
#include <variant>

#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityPresence.h"
#include "HMI/HmiLog.h"

namespace hmi {

namespace {

/// Durée d'une image des bandes de figurine qui ne disent pas la leur, en secondes.
constexpr float FIGURE_FRAME_SECONDS = 0.15F;

/// Les entités de la carte courante que les drapeaux laissent paraître (`LOT-116`) : une copie,
/// que la carte, lue d'un fichier qui ignore la partie, ne peut pas être.
[[nodiscard]] std::vector<core::MapEntity> entitesPresentes(const core::ExplorationSession& session,
                                                            const core::Level& map) {
    std::vector<core::MapEntity> presentes;
    presentes.reserve(map.entities().size());
    for (const core::MapEntity& entite : map.entities()) {
        if (session.isPresent(entite)) {
            presentes.push_back(entite);
        }
    }
    return presentes;
}

}  // namespace

WorldPlay::WorldPlay(core::WorldTravel::MapLoader loader, std::filesystem::path assetsDirectory)
    : _session(std::move(loader)), _assetsDirectory(std::move(assetsDirectory)) {
    setHeroFigure(std::string{DEFAULT_HERO_FIGURE});
}

void WorldPlay::setHeroFigure(std::string figure) {
    _heroFigure = std::move(figure);
    // Une figurine est orientee si sa bande de repos vers le sud-est existe : c'est la premiere que
    // l'atelier produit, et une figurine a moitie orientee se verrait plus mal qu'une qui ne l'est
    // pas (`scripts/checks/check_hd_assets.py` exige les quatre).
    std::error_code erreur;
    _heroOriented = std::filesystem::is_regular_file(
        _assetsDirectory / figureStripPath(_heroFigure, "idle", FigureFacing::SouthEast), erreur);
    // Le dossier de la figurine du heros se lit dans la carte en valeurs.
    invalidateScene();
}

bool WorldPlay::enter(std::string_view mapId, std::string_view arrival) {
    if (!_session.start(mapId, arrival)) {
        return false;
    }
    _elapsed = 0.0F;
    _walking = false;
    _drawnFlags = _session.flags().revision();
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
        result.figuresChanged = true;
    }
    if (walking) {
        const FigureFacing facing = figureFacingFor(intent.move, _heroFacing);
        if (facing != _heroFacing) {
            _heroFacing = facing;
            result.figuresChanged = true;
        }
    }
    // Un héros qui pousse contre un mur ne change pas de case, mais sa bande continue de tourner :
    // la scène doit se redessiner autant que s'il avait bougé.
    result.heroMoved = _session.heroPoint() != before || walking;
    // Un drapeau change -- un dialogue, une quete qui avance : un PNJ parait ou disparait, et la
    // scene se recompose sans que la carte soit relue (`LOT-116`).
    if (_session.flags().revision() != _drawnFlags) {
        _drawnFlags = _session.flags().revision();
        invalidateScene();
        result.sceneChanged = true;
    }
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
    invalidateScene();
    const core::Level* const map = _session.map();
    if (map == nullptr) {
        _appearance = PlaceAppearance{};
        return;
    }
    const std::string place = scenePlaceOf(*map);
    if (place.empty()) {
        // Une carte qui ne nomme aucun lieu se joue en MAQUETTE (LOT-128) : la composition dessine
        // ses types en couleurs, ses murs en blocs et ses entites en jetons. Ce n'est plus un
        // manque a signaler, c'est l'etat de depart normal d'une carte. Ses PNJ prennent les
        // figurines du monde (LOT-124).
        _appearance = PlaceAppearance::loadForPlace(_assetsDirectory, {}).appearance;
        HMI_LOG_INFO("Monde : la carte " + _session.mapId() +
                     " ne declare aucun lieu ; elle se joue en maquette.");
        return;
    }
    PlaceAppearanceResult read = PlaceAppearance::loadForPlace(_assetsDirectory, place);
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
    std::vector<WorldFigureSnapshot> figures = npcFigures(entitesPresentes(_session, *map), frame);
    figures.push_back(
        WorldFigureSnapshot{.figure = _heroFigure,
                            .clip = _walking ? "walk" : "idle",
                            .point = {_session.heroPoint().column, _session.heroPoint().row},
                            .frame = frame,
                            .facing = heroFacing(),
                            .seconds = _elapsed,
                            .hero = true});
    return figures;
}

std::shared_ptr<const WorldSceneSnapshot> WorldPlay::scene() const {
    if (_scene != nullptr) {
        return _scene;
    }
    const core::Level* const map = _session.map();
    if (map == nullptr) {
        _scene = std::make_shared<const WorldSceneSnapshot>();
        return _scene;
    }
    const std::vector<core::MapEntity> presentes = entitesPresentes(_session, *map);
    const WorldSceneSource source{
        .root = map->tileMap(), .layers = map->layers(), .entities = presentes};
    // Les figurines de l'instant y sont : c'est d'elles que la carte tire le dossier de chacune.
    _scene = std::make_shared<const WorldSceneSnapshot>(
        snapshotWorldScene(source, _appearance, figures()));
    return _scene;
}

WorldSceneSnapshot WorldPlay::snapshot() const {
    WorldSceneSnapshot snapshot = *scene();
    snapshot.figures = figures();
    return snapshot;
}

}  // namespace hmi
