// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/CityBlockImageProvider.h"

#include <QStringList>
#include <QUrl>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/CityBlock.h"
#include "HMI/Graphics/CityBlockRender.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/HmiLog.h"

namespace hmi {

namespace {

constexpr qsizetype CHAMPS = 5;

[[nodiscard]] std::string texteDe(const core::MapEntity& entite, std::string_view cle) {
    const auto trouvee = entite.properties.find(std::string{cle});
    if (trouvee == entite.properties.end()) {
        return {};
    }
    const std::string* texte = std::get_if<std::string>(&trouvee->second);
    return texte != nullptr ? *texte : std::string{};
}

}  // namespace

CityBlockImageProvider::CityBlockImageProvider(std::filesystem::path dataDirectory)
    : QQuickImageProvider(QQuickImageProvider::Image), _data(std::move(dataDirectory)) {}

QImage CityBlockImageProvider::requestImage(const QString& id, QSize* size,
                                            const QSize& requestedSize) {
    const QStringList champs = QUrl::fromPercentEncoding(id.toLatin1()).split(QLatin1Char('|'));
    if (champs.size() != CHAMPS) {
        return {};
    }
    const std::string carteId = champs[0].toStdString();
    const core::LevelLoadResult lu =
        core::LevelLoader::loadFromFile(_data / "Levels" / (carteId + ".json"));
    if (!lu.ok() || !lu.level.has_value()) {
        HMI_LOG_WARNING("Plan : la carte " + carteId + " ne se lit pas.");
        return {};
    }
    const core::Level& carte = *lu.level;

    const std::vector<core::CityBlock> ilots = core::cityBlocksOf(carte);
    const std::string ilotId = champs[1].toStdString();
    const auto ilot = std::ranges::find(ilots, ilotId, &core::CityBlock::name);
    if (ilot == ilots.end()) {
        return {};
    }

    const PlaceAppearanceResult table =
        PlaceAppearance::loadForPlace(_data / "Assets", scenePlaceOf(carte));

    // Le lieu tel qu'on le parcourt : ses PNJ, puis le heros s'il est dans ce quartier.
    std::vector<WorldFigureSnapshot> figurines;
    for (const core::MapEntity& objet : carte.entities()) {
        std::string figurine = texteDe(objet, core::NPC_FIGURE_PROPERTY);
        if (objet.type != core::NPC_ENTITY_TYPE || figurine.empty()) {
            continue;
        }
        figurines.push_back(
            WorldFigureSnapshot{.figure = std::move(figurine),
                                .clip = "idle",
                                .point = {static_cast<float>(objet.position.column) + 0.5F,
                                          static_cast<float>(objet.position.row) + 0.5F},
                                .frame = 0});
    }
    if (!champs[2].isEmpty()) {
        figurines.push_back(WorldFigureSnapshot{.figure = champs[2].toStdString(),
                                                .clip = "idle",
                                                .point = {champs[3].toFloat(), champs[4].toFloat()},
                                                .frame = 0});
    }

    QImage image = renderCityBlock(
        _data / "Assets", snapshotWorldScene(carte, table.appearance, std::move(figurines)), *ilot);
    if (!image.isNull() && requestedSize.isValid()) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (size != nullptr) {
        *size = image.size();
    }
    return image;
}

}  // namespace hmi
