// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_world_light.cpp
 * @brief Tests de l'heure du monde, de la table de lumière et des sources de lumière (LOT-1007).
 */

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "Core/Levels/MapEntity.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Core/World/DayLight.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/LightSource.h"
#include "Core/World/WorldClock.h"

namespace {

float length(const std::array<float, 3>& vector) {
    return std::sqrt((vector[0] * vector[0]) + (vector[1] * vector[1]) + (vector[2] * vector[2]));
}

}  // namespace

/**
 * @brief L'horloge avance d'une heure du monde par minute réelle, passe minuit en comptant un
 *        jour, et ne bouge pas quand elle est figée.
 * \castest{<b>L'heure du monde avance, passe minuit et se fige.</b><br/>
 * \tcat Unitaire · Monde · Heure<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire l'heure d'une horloge neuve.<br/>
 *          2. Faire passer soixante secondes, puis quatorze minutes réelles.<br/>
 *          3. Lui donner une durée négative, puis non finie.<br/>
 *          4. La figer, faire passer une heure réelle, la relancer.<br/>
 * \tattendu Elle part de 10:00 au jour 0 ; soixante secondes l'amènent à 11:00 ; quatorze minutes
 *           de plus à 01:00 du jour 1 ; une durée négative ou non finie ne change rien ; figée,
 *           elle ne bouge pas.
 * }
 */
TEST(WorldClockTest, LHeureAvancePasseMinuitEtSeFige) {
    core::WorldClock clock;
    EXPECT_FLOAT_EQ(clock.minutes(), 600.0F);
    EXPECT_EQ(clock.day(), 0);

    clock.advance(60.0F);
    EXPECT_FLOAT_EQ(clock.hours(), 11.0F);

    clock.advance(14.0F * 60.0F);
    EXPECT_FLOAT_EQ(clock.hours(), 1.0F);
    EXPECT_EQ(clock.day(), 1);

    clock.advance(-30.0F);
    clock.advance(std::nanf(""));
    EXPECT_FLOAT_EQ(clock.hours(), 1.0F);

    clock.setRunning(false);
    clock.advance(3600.0F);
    EXPECT_FLOAT_EQ(clock.hours(), 1.0F);
    clock.setRunning(true);
    clock.advance(60.0F);
    EXPECT_FLOAT_EQ(clock.hours(), 2.0F);
}

/**
 * @brief Une heure s'écrit et se relit `HH:MM` ; un réglage hors du jour y est ramené.
 * \castest{<b>Une heure s'ecrit et se relit HH:MM.</b><br/>
 * \tcat Unitaire · Monde · Heure<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire `21:30`, `7:05`, `9`, puis `24:00`, `12:60`, `12:5`, `midi` et un texte
 * vide.<br/>
 *          2. Écrire 1290 minutes, puis 1445 et −30.<br/>
 *          3. Régler une horloge à 1500 minutes.<br/>
 * \tattendu Les trois premières valent 1290, 425 et 540 minutes ; les autres ne sont pas des
 *           heures ; 1290 s'écrit `21:30`, 1445 `00:05` et −30 `23:30` ; l'horloge réglée à 1500
 *           est à 01:00, au même jour.
 * }
 */
TEST(WorldClockTest, UneHeureSEcritEtSeRelit) {
    EXPECT_EQ(core::parseClockTime("21:30"), std::optional<float>{1290.0F});
    EXPECT_EQ(core::parseClockTime("7:05"), std::optional<float>{425.0F});
    EXPECT_EQ(core::parseClockTime("9"), std::optional<float>{540.0F});
    for (const char* const wrong : {"24:00", "12:60", "12:5", "midi", "", ":30", "-1:00"}) {
        EXPECT_FALSE(core::parseClockTime(wrong).has_value()) << wrong;
    }
    EXPECT_EQ(core::formatClockTime(1290.0F), "21:30");
    EXPECT_EQ(core::formatClockTime(1445.0F), "00:05");
    EXPECT_EQ(core::formatClockTime(-30.0F), "23:30");

    core::WorldClock clock;
    clock.setMinutes(1500.0F);
    EXPECT_FLOAT_EQ(clock.hours(), 1.0F);
    EXPECT_EQ(clock.day(), 0);
}

/**
 * @brief La table interpole entre deux clés, rejoint le lendemain passé minuit, et rend à midi la
 *        lumière qui laisse les images telles que peintes.
 * \castest{<b>La table de lumiere interpole entre ses cles et passe minuit.</b><br/>
 * \tcat Unitaire · Monde · Lumière du jour<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Bâtir une table de deux clés : 06:00 (teinte noire, lampes à 1) et 18:00 (teinte
 *             blanche, lampes à 0).<br/>
 *          2. La lire à 06:00, 12:00, 18:00, puis à 00:00.<br/>
 *          3. Lire la table d'usine à midi et à minuit.<br/>
 *          4. Lire une table vide.<br/>
 * \tattendu À 06:00 et 18:00 la table rend ses clés ; à 12:00 leur milieu ; à 00:00, le milieu
 *           de 18:00 et de 06:00 du lendemain. La table d'usine a une teinte blanche et des
 *           lampes éteintes à midi, une teinte plus sombre et des lampes allumées à minuit ; son
 *           vecteur vers le soleil est unitaire et monte. Une table vide rend la lumière neutre.
 * }
 */
TEST(DayLightTest, LaTableInterpoleEtPasseMinuit) {
    const core::DayLightTable table{{
        core::DayLightKey{.minutes = 1080.0F,
                          .tint = {1.0F, 1.0F, 1.0F},
                          .ambient = {},
                          .sun = {},
                          .azimuth = 45.0F,
                          .elevation = 60.0F,
                          .shadow = 0.4F,
                          .lamps = 0.0F},
        core::DayLightKey{.minutes = 360.0F,
                          .tint = {0.0F, 0.0F, 0.0F},
                          .ambient = {},
                          .sun = {},
                          .azimuth = 45.0F,
                          .elevation = 20.0F,
                          .shadow = 0.0F,
                          .lamps = 1.0F},
    }};
    ASSERT_EQ(table.keys().size(), 2U);
    EXPECT_FLOAT_EQ(table.keys().front().minutes, 360.0F) << "les clés sont triées par heure";

    EXPECT_FLOAT_EQ(table.sample(360.0F).tint.r, 0.0F);
    EXPECT_FLOAT_EQ(table.sample(1080.0F).tint.r, 1.0F);
    EXPECT_NEAR(table.sample(720.0F).tint.r, 0.5F, 1e-5F);
    EXPECT_NEAR(table.sample(720.0F).lamps, 0.5F, 1e-5F);
    EXPECT_NEAR(table.sample(720.0F).shadow, 0.2F, 1e-5F);
    // Minuit : six heures après 18:00, six heures avant 06:00.
    EXPECT_NEAR(table.sample(0.0F).tint.r, 0.5F, 1e-5F);
    EXPECT_NEAR(table.sample(1440.0F).tint.r, 0.5F, 1e-5F);

    const core::DayLight noon = core::DayLightTable::factory().sample(720.0F);
    EXPECT_EQ(noon.tint, (core::LightColor{.r = 1.0F, .g = 1.0F, .b = 1.0F}));
    EXPECT_FLOAT_EQ(noon.lamps, 0.0F);
    EXPECT_NEAR(length(noon.toSun), 1.0F, 1e-5F);
    EXPECT_GT(noon.toSun[1], 0.5F) << "le soleil de midi est haut";
    EXPECT_LT(noon.toSun[0], 0.0F) << "et vient du haut à gauche de l'écran : des petites colonnes";

    const core::DayLight midnight = core::DayLightTable::factory().sample(0.0F);
    EXPECT_LT(midnight.tint.r, 0.6F);
    EXPECT_GT(midnight.tint.b, midnight.tint.r) << "la nuit tire sur le bleu";
    EXPECT_FLOAT_EQ(midnight.lamps, 1.0F);

    EXPECT_EQ(core::DayLightTable{}.sample(300.0F), core::DayLight{});
}

/**
 * @brief La lumière ne saute pas : d'une minute à la suivante, sur tout le jour, la teinte, les
 *        lampes et la lumière que reçoit une face ne changent que d'un souffle.
 * \castest{<b>La lumiere du jour ne saute pas d'une minute a l'autre.</b><br/>
 * \tcat Unitaire · Monde · Lumière du jour<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire la table d'usine à chaque minute du jour, et à la minute suivante.<br/>
 *          2. Comparer la teinte, l'ambiance, l'allumage des lampes et la lumière dirigée que
 *             reçoit une face tournée vers le haut.<br/>
 * \tattendu Aucun écart ne dépasse quatre centièmes : ni au lever, ni au coucher, ni aux deux
 *           bascules du soleil à la lune, où la lumière dirigée est noire.
 * }
 */
TEST(DayLightTest, LaLumiereNeSautePas) {
    const core::DayLightTable& table = core::DayLightTable::factory();
    // Ce qu'une face horizontale reçoit de la lumière dirigée : sa couleur par sa hauteur.
    const auto upward = [](const core::DayLight& light) { return light.sun.r * light.toSun[1]; };
    for (int minute = 0; minute < 1440; ++minute) {
        const core::DayLight now = table.sample(static_cast<float>(minute));
        const core::DayLight next = table.sample(static_cast<float>(minute + 1));
        EXPECT_NEAR(now.tint.r, next.tint.r, 0.04F) << minute;
        EXPECT_NEAR(now.tint.b, next.tint.b, 0.04F) << minute;
        EXPECT_NEAR(now.ambient.g, next.ambient.g, 0.04F) << minute;
        EXPECT_NEAR(now.lamps, next.lamps, 0.04F) << minute;
        EXPECT_NEAR(upward(now), upward(next), 0.04F) << minute;
    }
}

/**
 * @brief Le fichier livré est la table d'usine, et une table mal écrite dit pourquoi.
 * \castest{<b>La table livree se lit, et une table fausse est refusee.</b><br/>
 * \tcat Unitaire · Monde · Lumière du jour<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire `Assets/Common/Lighting/daylight.json`.<br/>
 *          2. Lire un fichier absent, une table sans clé, une clé sans heure, une couleur fausse,
 *             une élévation nulle, une ombre hors de [0, 1], deux clés à la même heure.<br/>
 * \tattendu Le fichier livré donne les mêmes clés que la table d'usine, aux arrondis de
 *           l'écriture près. Chaque table fausse est refusée, avec un message.
 * }
 */
TEST(DayLightTest, LaTableLivreeSeLitEtUneFausseEstRefusee) {
    const core::DayLightTableResult read = core::readDayLightTableFile(
        std::filesystem::path(JADG_LEVELS_DIR).parent_path() / "Assets" / core::DAYLIGHT_FILE);
    ASSERT_TRUE(read.ok()) << read.message;
    const std::vector<core::DayLightKey>& expected = core::DayLightTable::factory().keys();
    ASSERT_EQ(read.table.keys().size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const core::DayLightKey& one = read.table.keys()[index];
        EXPECT_FLOAT_EQ(one.minutes, expected[index].minutes) << index;
        EXPECT_NEAR(one.tint.r, expected[index].tint.r, 1e-5F) << index;
        EXPECT_NEAR(one.ambient.g, expected[index].ambient.g, 1e-5F) << index;
        EXPECT_NEAR(one.sun.b, expected[index].sun.b, 1e-5F) << index;
        EXPECT_FLOAT_EQ(one.azimuth, expected[index].azimuth) << index;
        EXPECT_FLOAT_EQ(one.elevation, expected[index].elevation) << index;
        EXPECT_NEAR(one.shadow, expected[index].shadow, 1e-5F) << index;
        EXPECT_NEAR(one.lamps, expected[index].lamps, 1e-5F) << index;
    }

    EXPECT_FALSE(core::readDayLightTableFile("absent/daylight.json").ok());
    const std::string good = R"("tint":"#ffffff","ambient":"#999999","sun":"#808080",)"
                             R"("azimuth":45,"elevation":58,"shadow":0.3,"lamps":0)";
    const auto table = [](const std::string& keys) {
        return core::readDayLightTable(R"({"version":1,"keys":[)" + keys + "]}");
    };
    EXPECT_TRUE(table(R"({"time":"12:00",)" + good + "}").ok());
    EXPECT_FALSE(core::readDayLightTable(R"({"version":1})").ok());
    EXPECT_FALSE(table("").ok());
    EXPECT_FALSE(table("{" + good + "}").ok()) << "une clé sans heure";
    EXPECT_FALSE(table(R"({"time":"12:00","tint":"blanc","ambient":"#999999","sun":"#808080",)"
                       R"("azimuth":45,"elevation":58,"shadow":0.3,"lamps":0})")
                     .ok());
    EXPECT_FALSE(table(R"({"time":"12:00","tint":"#ffffff","ambient":"#999999","sun":"#808080",)"
                       R"("azimuth":45,"elevation":0,"shadow":0.3,"lamps":0})")
                     .ok());
    EXPECT_FALSE(table(R"({"time":"12:00","tint":"#ffffff","ambient":"#999999","sun":"#808080",)"
                       R"("azimuth":45,"elevation":58,"shadow":1.5,"lamps":0})")
                     .ok());
    EXPECT_FALSE(table(R"({"time":"12:00",)" + good + R"(},{"time":"12:00",)" + good + "}").ok());
    EXPECT_FALSE(core::readDayLightTable(R"({"version":99,"keys":[]})").ok());
}

/**
 * @brief Une entité `light` donne une source en mètres, et la famille est connue de l'éditeur.
 * \castest{<b>Une entite light donne une source de lumiere.</b><br/>
 * \tcat Unitaire · Monde · Sources de lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire la famille `light` de la table des familles d'entités.<br/>
 *          2. Poser une lumière en (4, 2) : couleur `#80c0ff`, portée 6 cases, hauteur 30 dm,
 *             intensité 150 %, tremblante, toujours allumée.<br/>
 *          3. En poser une sans propriété, puis une aux valeurs hors bornes et mal typées.<br/>
 *          4. Demander la source d'un coffre.<br/>
 * \tattendu La famille existe, ponctuelle, avec ses six propriétés ; la première source est au
 *           centre de sa case (4,5 ; 2,5), à 9 m de portée, 3 m de haut, intensité 1,5 ; la
 *           deuxième vaut une lanterne (4 cases, 2,2 m, intensité 1) ; la troisième est ramenée
 *           dans ses bornes et garde la couleur d'usine ; un coffre ne donne aucune source.
 * }
 */
TEST(LightSourceTest, UneEntiteLightDonneUneSource) {
    const core::EntityKind* const kind = core::findEntityKind(core::LIGHT_ENTITY_TYPE);
    ASSERT_NE(kind, nullptr);
    EXPECT_EQ(kind->shape, core::EntityShape::Point);
    for (const std::string_view key : {core::LIGHT_COLOR_PROPERTY, core::LIGHT_RADIUS_PROPERTY,
                                       core::LIGHT_HEIGHT_PROPERTY, core::LIGHT_INTENSITY_PROPERTY,
                                       core::LIGHT_FLICKER_PROPERTY, core::LIGHT_ALWAYS_PROPERTY}) {
        EXPECT_NE(kind->find(key), nullptr) << key;
    }

    const core::MapEntity lamp{
        .type = std::string{core::LIGHT_ENTITY_TYPE},
        .position = {.column = 4, .row = 2},
        .properties = {{std::string{core::LIGHT_COLOR_PROPERTY}, std::string{"#80c0ff"}},
                       {std::string{core::LIGHT_RADIUS_PROPERTY}, std::int64_t{6}},
                       {std::string{core::LIGHT_HEIGHT_PROPERTY}, std::int64_t{30}},
                       {std::string{core::LIGHT_INTENSITY_PROPERTY}, std::int64_t{150}},
                       {std::string{core::LIGHT_FLICKER_PROPERTY}, true},
                       {std::string{core::LIGHT_ALWAYS_PROPERTY}, true}}};
    const std::optional<core::LightSource> source = core::lightSourceOf(lamp);
    ASSERT_TRUE(source.has_value());
    EXPECT_FLOAT_EQ(source->column, 4.5F);
    EXPECT_FLOAT_EQ(source->row, 2.5F);
    EXPECT_FLOAT_EQ(source->emission.radius, 9.0F);
    EXPECT_FLOAT_EQ(source->emission.height, 3.0F);
    EXPECT_FLOAT_EQ(source->emission.intensity, 1.5F);
    EXPECT_NEAR(source->emission.color.r, 128.0F / 255.0F, 1e-5F);
    EXPECT_FLOAT_EQ(source->emission.color.b, 1.0F);
    EXPECT_TRUE(source->emission.flicker);
    EXPECT_TRUE(source->emission.always);

    const std::optional<core::LightSource> bare = core::lightSourceOf(
        core::MapEntity{.type = std::string{core::LIGHT_ENTITY_TYPE}, .position = {}});
    ASSERT_TRUE(bare.has_value());
    EXPECT_FLOAT_EQ(bare->emission.radius, 6.0F);
    EXPECT_FLOAT_EQ(bare->emission.height, 2.2F);
    EXPECT_FLOAT_EQ(bare->emission.intensity, 1.0F);
    EXPECT_FALSE(bare->emission.flicker);
    EXPECT_FALSE(bare->emission.always);

    const std::optional<core::LightSource> wrong = core::lightSourceOf(core::MapEntity{
        .type = std::string{core::LIGHT_ENTITY_TYPE},
        .position = {},
        .properties = {{std::string{core::LIGHT_COLOR_PROPERTY}, std::string{"rouge"}},
                       {std::string{core::LIGHT_RADIUS_PROPERTY}, std::int64_t{900}},
                       {std::string{core::LIGHT_HEIGHT_PROPERTY}, std::string{"haut"}},
                       {std::string{core::LIGHT_INTENSITY_PROPERTY}, std::int64_t{-5}}}});
    ASSERT_TRUE(wrong.has_value());
    EXPECT_EQ(wrong->emission.color, core::LightEmission::DEFAULT_COLOR);
    EXPECT_FLOAT_EQ(wrong->emission.radius, 24.0F);
    EXPECT_FLOAT_EQ(wrong->emission.height, 2.2F);
    EXPECT_FLOAT_EQ(wrong->emission.intensity, 0.1F);

    EXPECT_FALSE(core::lightSourceOf(core::MapEntity{.type = "chest", .position = {}}).has_value());
}

/**
 * @brief Une pièce du manifeste déclare sa lumière et son éclat ; une pièce qui n'en dit rien
 *        n'éclaire pas.
 * \castest{<b>Une piece du manifeste declare sa lumiere et son eclat.</b><br/>
 * \tcat Unitaire · Monde · Sources de lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire un manifeste de trois pièces : un lampadaire (`light` complet), un brasero
 *             (`light` vide, `glow` à 0,8), un mur (ni l'un ni l'autre).<br/>
 *          2. Lire un lampadaire dont la portée dépasse la borne et dont la couleur est
 * fausse.<br/>
 * \tattendu Le lampadaire émet sa couleur à 7,5 m, depuis 3,2 m, sans trembler ; le brasero vaut
 *           une lanterne d'usine et garde 0,8 de son éclat ; le mur n'émet rien et ne garde
 *           rien. La portée trop grande est ramenée à 24 m, la couleur fausse à celle d'usine.
 * }
 */
TEST(LightSourceTest, UnePieceDuManifesteDeclareSaLumiere) {
    const core::ScenePieceManifestResult read = core::ScenePieceManifest::loadFromString(R"({
        "version": 1, "disposition": "essai", "tile": [256, 159],
        "textures": {
            "scene/essai/prop-lamppost": { "file": "prop-lamppost.png", "class": "tall",
                "light": { "color": "#ffd090", "radius": 7.5, "height": 3.2 } },
            "scene/essai/prop-brazier-lit": { "file": "prop-brazier-lit.png", "class": "tall",
                "light": {}, "glow": 0.8 },
            "scene/essai/wall": { "file": "wall.png", "class": "tall" },
            "scene/essai/prop-beacon": { "file": "prop-beacon.png", "class": "tall",
                "light": { "color": "jaune", "radius": 500, "flicker": true, "always": true },
                "glow": 7 }
        } })");
    ASSERT_TRUE(read.ok()) << read.message;

    const core::ScenePiece* const lamppost = read.manifest.find("prop-lamppost");
    ASSERT_NE(lamppost, nullptr);
    ASSERT_TRUE(lamppost->light.has_value());
    EXPECT_FLOAT_EQ(lamppost->light->radius, 7.5F);
    EXPECT_FLOAT_EQ(lamppost->light->height, 3.2F);
    EXPECT_NEAR(lamppost->light->color.g, 208.0F / 255.0F, 1e-5F);
    EXPECT_FALSE(lamppost->light->flicker);
    EXPECT_FLOAT_EQ(lamppost->glow, 0.0F);

    const core::ScenePiece* const brazier = read.manifest.find("prop-brazier-lit");
    ASSERT_NE(brazier, nullptr);
    ASSERT_TRUE(brazier->light.has_value());
    EXPECT_EQ(*brazier->light, core::LightEmission{});
    EXPECT_FLOAT_EQ(brazier->glow, 0.8F);

    const core::ScenePiece* const wall = read.manifest.find("wall");
    ASSERT_NE(wall, nullptr);
    EXPECT_FALSE(wall->light.has_value());
    EXPECT_FLOAT_EQ(wall->glow, 0.0F);

    const core::ScenePiece* const beacon = read.manifest.find("prop-beacon");
    ASSERT_NE(beacon, nullptr);
    ASSERT_TRUE(beacon->light.has_value());
    EXPECT_FLOAT_EQ(beacon->light->radius, 24.0F);
    EXPECT_EQ(beacon->light->color, core::LightEmission::DEFAULT_COLOR);
    EXPECT_TRUE(beacon->light->flicker);
    EXPECT_TRUE(beacon->light->always);
    EXPECT_FLOAT_EQ(beacon->glow, 1.0F);
}
