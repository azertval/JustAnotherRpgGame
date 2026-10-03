// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <QColor>
#include <QImage>
#include <QString>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <numbers>
#include <string>

#include <gtest/gtest.h>

#include "Core/Resources/SkeletonFile.h"
#include "Editor/Logic/CharacterPreview.h"
#include "HMI/Graphics/OffscreenRender.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

/**
 * @file Unit/Editor/test_character_preview.cpp
 * @brief L'aperçu de l'atelier des assets (`LOT-1008`, `EX-EDIT-102`) : la scène qu'il remet au
 *        rendu du jeu, l'instant de ses clips, et ce que le rendu en dessine, hors écran.
 */

namespace {

const QColor FOND(34, 36, 42);
const QSize TAILLE(360, 480);
constexpr const char* MODELE = "Mannequins/humanoid/humanoid.glb";

std::filesystem::path personnages() {
    return std::filesystem::path(JADG_CHARACTER_FIXTURE_DIR) / "Assets" / "Common" / "Characters";
}

/// Le nombre de pixels de la moitié haute de @p image qui diffèrent du damier seul (@p vide) : ce
/// que le personnage y occupe.
std::size_t dessines(const QImage& image, const QImage& vide) {
    std::size_t compte = 0;
    for (int y = 0; y < image.height() / 2; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            compte += image.pixel(x, y) != vide.pixel(x, y) ? 1U : 0U;
        }
    }
    return compte;
}

/// La première ligne de @p image, depuis le haut, où le personnage paraît ; sa hauteur sinon.
int sommet(const QImage& image, const QImage& vide) {
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixel(x, y) != vide.pixel(x, y)) {
                return y;
            }
        }
    }
    return image.height();
}

}  // namespace

/**
 * @brief La scène de l'aperçu : un damier sans pièce, le personnage au centre, tourné de ses
 *        quarts de tour.
 * \castest{<b>L'apercu de l'atelier est une scene du rendu du jeu.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer l'aperçu d'un modèle, à 0,4 s de son attaque, tourné d'un quart de
 *             tour.<br/>
 *          2. Composer l'aperçu sans modèle.<br/>
 * \tattendu La scène fait trois cases sur trois, sans pièce, en terre battue. Elle porte une
 *           figurine, au centre, qui nomme son modèle, son clip et son instant ; son cap est
 *           celui de la vue de face plus un quart de tour. Sans modèle, la scène n'a pas de
 *           figurine.
 * }
 */
TEST(CharacterPreviewTest, LaSceneEstUnDamierEtUnPersonnageAuCentre) {
    const hmi::WorldSceneSnapshot scene = hmi::characterPreviewScene(
        {.model = MODELE, .clip = "attack", .seconds = 0.4F, .quarterTurns = 1});
    EXPECT_EQ(scene.columns, 3);
    EXPECT_EQ(scene.rows, 3);
    ASSERT_EQ(scene.types.size(), 9U);
    EXPECT_EQ(scene.typeAt({1, 1}), core::TileType::Dirt);
    EXPECT_TRUE(scene.floorAt({1, 1}).empty());
    ASSERT_EQ(scene.figures.size(), 1U);
    const hmi::WorldFigureSnapshot& figure = scene.figures.front();
    EXPECT_EQ(figure.model, MODELE);
    EXPECT_EQ(figure.clip, "attack");
    EXPECT_FLOAT_EQ(figure.seconds, 0.4F);
    EXPECT_FLOAT_EQ(figure.point.x, 1.5F);
    EXPECT_FLOAT_EQ(figure.point.y, 1.5F);
    EXPECT_FLOAT_EQ(figure.heading, hmi::FIGURE_HEADING_FRONT + (std::numbers::pi_v<float> / 2.0F));

    EXPECT_TRUE(hmi::characterPreviewScene({}).figures.empty());
}

/**
 * @brief Un clip en boucle tourne ; un clip joué une fois va à sa fin, y reste, puis reprend.
 * \castest{<b>L'apercu tient la derniere pose d'un clip joue une fois.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander l'instant d'une marche de 0,5 s après 1,2 s d'aperçu.<br/>
 *          2. Demander l'instant d'une mort de 1,2 s après 0,5 s, 1,5 s, puis 2,1 s.<br/>
 *          3. Demander l'instant d'un clip que le squelette ne déclare pas.<br/>
 * \tattendu La marche est à 0,2 s. La mort est à 0,5 s, puis tenue à 1,2 s — sa dernière pose —,
 *           juste avant sa fin, pour qu'un rendu qui boucle ne revienne pas au début —, puis
 *           repartie à 0,1 s. Un clip inconnu boucle sur une seconde.
 * }
 */
TEST(CharacterPreviewTest, UnClipJoueUneFoisTientSaDernierePose) {
    const core::SkeletonClip marche{.name = "walk", .duration = 0.5F, .loop = true, .key = {}};
    EXPECT_NEAR(hmi::characterPreviewSeconds(&marche, 1.2F), 0.2F, 1e-5F);
    const core::SkeletonClip mort{.name = "death", .duration = 1.2F, .loop = false, .key = {}};
    EXPECT_NEAR(hmi::characterPreviewSeconds(&mort, 0.5F), 0.5F, 1e-5F);
    EXPECT_NEAR(hmi::characterPreviewSeconds(&mort, 1.5F), 1.2F, 2e-3F);
    EXPECT_LT(hmi::characterPreviewSeconds(&mort, 1.5F), 1.2F) << "tenu avant la fin, pas dessus";
    EXPECT_NEAR(hmi::characterPreviewSeconds(&mort, 1.2F + hmi::CHARACTER_PREVIEW_HOLD + 0.1F),
                0.1F, 1e-4F);
    EXPECT_NEAR(hmi::characterPreviewSeconds(nullptr, 2.25F), 0.25F, 1e-5F);
    EXPECT_FLOAT_EQ(hmi::characterPreviewSeconds(&mort, -3.0F), 0.0F);
}

/**
 * @brief Le rendu du jeu dessine l'aperçu : le personnage tient dans le cadre, debout, et son
 *        quart de tour et sa chute changent l'image (`EX-EDIT-102`).
 * \castest{<b>Le rendu du jeu dessine le personnage de l'apercu, cadre et anime.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre hors écran le damier seul, puis l'aperçu du mannequin d'essai, au repos.<br/>
 *          2. Le rendre tourné d'un quart de tour, puis à la fin de sa chute.<br/>
 * \tattendu Au repos, le personnage occupe la moitié haute de l'image et son sommet est entre le
 *           vingtième et le tiers de sa hauteur : il tient dans le cadre et l'emplit. Tourné d'un
 *           quart de tour, l'image diffère. À la fin de sa chute, son sommet est plus bas d'un
 *           huitième de l'image au moins : il est couché.
 * }
 */
TEST(CharacterPreviewRenderTest, LeRenduDuJeuDessineLePersonnage) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(personnages());
    const auto rendre = [&](const hmi::CharacterPreviewView& vue) {
        renderer.setSnapshot(hmi::characterPreviewScene(vue));
        const QImage image = offscreen->render(renderer, TAILLE,
                                               hmi::characterPreviewFraming(TAILLE.height()), FOND);
        EXPECT_EQ(image.size(), TAILLE);
        return image;
    };
    const QImage vide = rendre({});
    const QImage repos = rendre({.model = MODELE, .clip = "idle"});
    const std::filesystem::path captures(JADG_RENDER_CAPTURES_DIR);
    std::filesystem::create_directories(captures);
    EXPECT_TRUE(repos.save(QString::fromStdWString((captures / "atelier-apercu.png").wstring())));
    EXPECT_GT(dessines(repos, vide), 2000U) << "le personnage est debout dans la moitie haute";
    const int sommetDebout = sommet(repos, vide);
    EXPECT_GT(sommetDebout, TAILLE.height() / 20) << "le personnage touche le haut du cadre";
    EXPECT_LT(sommetDebout, TAILLE.height() / 3) << "le personnage n'emplit pas le cadre";
    const QImage tourne = rendre({.model = MODELE, .clip = "idle", .quarterTurns = 1});
    EXPECT_NE(tourne, repos);
    // La pose que l'aperçu tient à la fin de la chute, pour un modèle sans fiche à côté de lui.
    const core::SkeletonClip chute{.name = "death", .duration = 1.2F, .loop = false, .key = {}};
    const QImage couche = rendre(
        {.model = MODELE, .clip = "death", .seconds = hmi::characterPreviewSeconds(&chute, 1.5F)});
    EXPECT_GT(sommet(couche, vide), sommetDebout + (TAILLE.height() / 8))
        << "couche, son sommet est plus bas";
}
