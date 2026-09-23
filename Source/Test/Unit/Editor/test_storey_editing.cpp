// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_storey_editing.cpp
 * @brief Les étages dans l'éditeur (`LOT-129`) : une couche d'étage se peint, se montre ou se cache
 *        comme les autres, et un préfabriqué garde l'étage de ses couches.
 */

#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/TileLayer.h"
#include "Editor/Logic/CanvasScene.h"
#include "Editor/Logic/LayerView.h"
#include "Editor/Logic/PieceCatalog.h"
#include "Editor/Logic/Stamps.h"
#include "HMI/Graphics/ComposedScene.h"

namespace {

/// Un brouillon de 3 × 3 : un sol, un décor au rez, un décor à l'étage 1 (« toit »).
struct Building {
    core::LevelDraft draft = core::LevelDraft::empty("batiment", 3, 3);
    std::size_t ground = 0;
    std::size_t rez = 0;
    std::size_t roof = 0;

    Building() {
        ground = *draft.addLayer(core::LayerKind::Ground, "sol");
        rez = *draft.addLayer(core::LayerKind::Decor, "rez");
        roof = *draft.addLayer(core::LayerKind::Decor, "toit");
        EXPECT_TRUE(draft.setLayerFloor(roof, 1));
    }
};

}  // namespace

/**
 * @brief Une pièce vise la couche d'étage qu'on peint ; à défaut, le décor du rez.
 * \castest{<b>Une piece se peint sur l'etage actif.</b><br/>
 * \tcat Unitaire · Editeur · Etages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Viser une piece de relief, la couche d'etage active.<br/>
 *          2. La meme, sans couche active, puis le sol active.<br/>
 * \tattendu La couche d'etage ; puis le decor du rez, meme declare apres l'etage.
 * }
 */
TEST(StoreyEditingTest, UnePieceSePeintSurLEtageActif) {
    Building building;
    ASSERT_TRUE(building.draft.moveLayer(building.roof, /*forward=*/false));
    const std::vector<core::TileLayer>& layers = building.draft.layers();
    std::optional<std::size_t> roof;
    std::optional<std::size_t> rez;
    for (std::size_t index = 0; index < layers.size(); ++index) {
        if (layers[index].name == "toit") {
            roof = index;
        } else if (layers[index].name == "rez") {
            rez = index;
        }
    }
    ASSERT_TRUE(roof && rez);
    ASSERT_LT(*roof, *rez);  // l'étage est déclaré AVANT le rez

    EXPECT_EQ(hmi::pieceTargetLayer(layers, false, roof), roof);
    EXPECT_EQ(hmi::pieceTargetLayer(layers, false, std::nullopt), rez);
    EXPECT_EQ(hmi::pieceTargetLayer(layers, false, building.ground), rez);
}

/**
 * @brief Cacher la couche d'un étage cache cet étage, et lui seul.
 * \castest{<b>Cacher une couche d'etage cache son etage.</b><br/>
 * \tcat Unitaire · Editeur · Etages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Cacher la couche de l'etage 1.<br/>
 *          2. Lire l'opacite d'une piece de l'etage 1, du rez.<br/>
 * \tattendu L'etage 1 a une opacite nulle ; le rez et l'etage 2 restent entiers.
 * }
 */
TEST(StoreyEditingTest, CacherUneCoucheDEtageCacheSonEtage) {
    Building building;
    hmi::LayerViewState view;
    view.setVisible(building.roof, false);
    const hmi::IsoBandOpacity bands =
        hmi::isoBandOpacity(building.draft.layers(), view, building.rez, false);

    hmi::ComposedQuad storey;
    storey.layer = hmi::RenderLayer::Object;
    storey.storey = 1;
    hmi::ComposedQuad ground = storey;
    ground.storey = 0;
    hmi::ComposedQuad second = storey;
    second.storey = 2;
    EXPECT_FLOAT_EQ(hmi::bandOpacity(bands, storey), 0.0F);
    EXPECT_FLOAT_EQ(hmi::bandOpacity(bands, ground), 1.0F);
    EXPECT_FLOAT_EQ(hmi::bandOpacity(bands, second), 1.0F);
}

/**
 * @brief Un préfabriqué garde l'étage de ses couches, et se pose à l'étage : un toit ne tombe
 *        jamais au rez.
 * \castest{<b>Un prefabrique garde ses etages.</b><br/>
 * \tcat Unitaire · Editeur · Etages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser un toit a l'etage 1, decouper le batiment, l'ecrire puis le relire.<br/>
 *          2. Le poser sur un autre batiment dont les couches portent d'autres noms.<br/>
 *          3. Relire un prefabrique dont la couche de sol pretend etre a l'etage 1.<br/>
 * \tattendu La couche relue est a l'etage 1 ; le toit se pose sur la couche d'etage, pas au rez ;
 *           le prefabrique faux est refuse.
 * }
 */
TEST(StoreyEditingTest, UnPrefabriqueGardeSesEtages) {
    Building source;
    ASSERT_TRUE(source.draft.placePiece(source.roof, {.column = 1, .row = 1}, "roof",
                                        core::TileType::Wall));
    const hmi::Stamp cut =
        hmi::cutStamp(source.draft, {.column = 0, .row = 0}, {.column = 2, .row = 2});
    std::string error;
    const std::optional<hmi::Stamp> read = hmi::stampFromJson(hmi::stampToJson(cut), error);
    ASSERT_TRUE(read.has_value()) << error;
    ASSERT_EQ(*read, cut);
    const auto stampedRoof = std::ranges::find(read->layers, 1, &hmi::StampLayer::floor);
    ASSERT_NE(stampedRoof, read->layers.end());

    Building target;
    ASSERT_TRUE(target.draft.renameLayer(target.rez, "bas"));
    ASSERT_TRUE(target.draft.renameLayer(target.roof, "haut"));
    const hmi::StampPasteResult pasted =
        hmi::pasteStamp(target.draft, *read, {.column = 0, .row = 0}, hmi::LayerViewState{});
    ASSERT_TRUE(pasted.refusal.empty()) << pasted.refusal;
    EXPECT_EQ(target.draft.layers()[target.roof].pieceAt(1, 1), "roof");
    EXPECT_TRUE(target.draft.layers()[target.rez].pieceAt(1, 1).empty());

    nlohmann::json forged = hmi::stampToJson(cut);
    for (nlohmann::json& layer : forged["layers"]) {
        if (layer["kind"] == "ground") {
            layer["floor"] = 1;
        }
    }
    EXPECT_FALSE(hmi::stampFromJson(forged, error).has_value());
}
