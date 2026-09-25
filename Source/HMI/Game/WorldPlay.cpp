// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/WorldPlay.h"

#include <cstddef>
#include <utility>
#include <variant>

#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityPresence.h"
#include "HMI/HmiLog.h"

namespace hmi {

namespace {

// Durée d'une image des bandes de figurine qui ne disent pas la leur, en secondes.
constexpr float FIGURE_FRAME_SECONDS = 0.15F;
// Le décalage de respiration entre deux PNJ, en secondes : ni nul, ni un multiple de la bande.
constexpr float NPC_BREATH_OFFSET_SECONDS = 0.37F;

// Les entités de la carte courante que les drapeaux laissent paraître (`LOT-116`) : une copie,
// que la carte, lue d'un fichier qui ignore la partie, ne peut pas être.
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
    : _session(std::move(loader)),
      _assetsDirectory(std::move(assetsDirectory)),
      _figures(_assetsDirectory) {
    setHeroFigure(std::string{DEFAULT_HERO_FIGURE});
}

void WorldPlay::setHeroFigure(std::string figure) {
    _heroFigure = std::move(figure);
    // La figurine du heros, ou son mannequin si elle n'est pas installee (LOT-145) ; orientee si
    // sa bande de repos vers le sud-est existe (`scripts/checks/check_hd_assets.py` exige les
    // quatre des qu'il y en a une).
    _hero = _figures.resolve(_heroFigure, {}, _appearance);
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
    // Le lieu change : ce que le resolveur savait des figurines ne vaut plus, le heros compris.
    _figures.clear();
    const core::Level* const map = _session.map();
    if (map == nullptr) {
        _appearance = PlaceAppearance{};
        _hero = _figures.resolve(_heroFigure, {}, _appearance);
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
    } else if (PlaceAppearanceResult read = PlaceAppearance::loadForPlace(_assetsDirectory, place);
               !read.ok()) {
        HMI_LOG_WARNING("Monde : table d'apparence du lieu " + place + " illisible, " +
                        read.message);
        _appearance = PlaceAppearance{};
    } else {
        _appearance = std::move(read.appearance);
    }
    _hero = _figures.resolve(_heroFigure, {}, _appearance);
}

std::vector<WorldFigureSnapshot> WorldPlay::figures() const {
    const core::Level* const map = _session.map();
    if (map == nullptr) {
        return {};
    }
    const int frame = static_cast<int>(_elapsed / FIGURE_FRAME_SECONDS);

    // Les PNJ d'abord, le héros ensuite : à égalité de profondeur, c'est lui qui passe devant.
    // Un PNJ sans figurine prend son mannequin (LOT-145) ; chacun respire a son rythme -- un
    // decalage par PNJ, pour qu'une place ne respire pas d'un seul souffle.
    const std::vector<core::MapEntity> presentes = entitesPresentes(_session, *map);
    std::vector<WorldFigureSnapshot> figures = npcFigures(presentes, frame, /*placeholders=*/true);
    std::size_t rang = 0;
    for (const core::MapEntity& entite : presentes) {
        if (entite.type != core::NPC_ENTITY_TYPE || rang >= figures.size()) {
            continue;
        }
        WorldFigureSnapshot& figure = figures[rang++];
        const auto silhouette = entite.properties.find(std::string{SILHOUETTE_PROPERTY});
        const std::string* nom = silhouette != entite.properties.end()
                                     ? std::get_if<std::string>(&silhouette->second)
                                     : nullptr;
        const ResolvedFigure& resolue =
            _figures.resolve(figure.figure, nom != nullptr ? *nom : std::string{}, _appearance);
        figure.figure = resolue.directory;
        figure.facing = resolue.oriented ? FigureFacing::SouthEast : FigureFacing::None;
        figure.seconds = _elapsed + (static_cast<float>(rang) * NPC_BREATH_OFFSET_SECONDS);
    }
    figures.push_back(
        WorldFigureSnapshot{.figure = _hero.directory,
                            .clip = std::string{_walking ? figure_clips::WALK : figure_clips::IDLE},
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
