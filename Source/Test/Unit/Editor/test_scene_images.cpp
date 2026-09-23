// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_scene_images.cpp
 * @brief Le cache d'images du canevas : partagé, borné, lu une fois (`LOT-125`).
 *
 * Une pièce HD pèse seize fois une pièce de l'ancienne planche. Ces tests prouvent ce qui tient la
 * mémoire de l'éditeur sous la borne de son README : une seule instance par dossier d'assets pour
 * tous les onglets, un budget de pixels tenu par éviction, une image évincée qui se relit à
 * l'identique, et un manifeste lu une fois pour toutes les pièces de son dossier. Les images sont
 * celles de la maquette HD (`Fixtures/HdMockup`), à la définition du standard.
 */

#include <QImage>
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Editor/Ui/SceneImages.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "Test/Support/HdMockupScene.h"

namespace {

[[nodiscard]] std::filesystem::path mockupDirectory() {
    return std::filesystem::path(JADG_HD_MOCKUP_DIR);
}

/// Les images de la maquette, telles que la composition les demande.
[[nodiscard]] std::vector<std::string> mockupPaths() {
    const nlohmann::json scene = test_support::readHdMockupJson(mockupDirectory() / "scene.json");
    EXPECT_FALSE(scene.is_discarded());
    return hmi::worldTexturePaths(test_support::hdMockupSnapshot(scene, mockupDirectory()));
}

[[nodiscard]] std::size_t bytesOf(const QImage& image) {
    return static_cast<std::size_t>(image.sizeInBytes());
}

}  // namespace

/**
 * @brief Tous les onglets d'un même dossier d'assets partagent un seul cache.
 * \castest{<b>Le cache d'images est partage entre onglets.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander trois fois le cache partage du dossier de la maquette, comme trois
 *             onglets.<br/>
 *          2. Relacher les onglets un a un.<br/>
 * \tattendu Les trois demandes rendent la meme instance ; elle vit tant qu'un onglet la tient.
 * }
 */
TEST(SceneImagesTest, LesOngletsPartagentUnSeulCache) {
    std::shared_ptr<hmi::SceneImages> first = hmi::SceneImages::shared(mockupDirectory());
    std::shared_ptr<hmi::SceneImages> second = hmi::SceneImages::shared(mockupDirectory());
    std::shared_ptr<hmi::SceneImages> third =
        hmi::SceneImages::shared(mockupDirectory() / "Scene" / "..");
    EXPECT_EQ(first.get(), second.get());
    EXPECT_EQ(first.get(), third.get());
    EXPECT_EQ(first.use_count(), 3);

    first->ensure(mockupPaths());
    EXPECT_GT(first->residentBytes(), 0U);
    const std::weak_ptr<hmi::SceneImages> watched = first;
    first.reset();
    second.reset();
    EXPECT_FALSE(watched.expired());  // le troisième onglet le tient encore
    third.reset();
    EXPECT_TRUE(watched.expired());
}

/**
 * @brief Le cache tient son budget : au-delà, la pièce la moins récemment peinte est évincée, et
 *        se relit à l'identique à la peinture suivante.
 * \castest{<b>Le cache d'images tient son budget.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Creer un cache dont le budget tient la plus grosse piece de la maquette, pas
 *             toutes.<br/>
 *          2. Charger toutes les pieces, puis peindre (lire) chacune.<br/>
 *          3. Relire la premiere.<br/>
 * \tattendu Apres chaque lecture, les octets tenus restent sous le budget (la piece lue
 *           exceptee) ; la premiere piece, evincee, se relit pixel pour pixel.
 * }
 */
TEST(SceneImagesTest, LeBudgetEstTenuEtUneImageEvinceeSeRelit) {
    const std::vector<std::string> paths = mockupPaths();
    ASSERT_GE(paths.size(), 3U);

    std::size_t largest = 0;
    QImage firstPixels;
    {
        hmi::SceneImages unbounded(mockupDirectory());
        unbounded.ensure(paths);
        for (const std::string& path : paths) {
            hmi::SceneImage* const image = unbounded.image(path);
            ASSERT_NE(image, nullptr) << path;
            largest = std::max(largest, bytesOf(image->level(0)));
        }
        firstPixels = unbounded.image(paths.front())->level(0);
    }

    // Deux fois la plus grosse pièce, et ses niveaux réduits : jamais toute la maquette.
    hmi::SceneImages images(mockupDirectory(), 2 * largest);
    images.ensure(paths);
    for (const std::string& path : paths) {
        hmi::SceneImage* const image = images.image(path);
        ASSERT_NE(image, nullptr);
        const QImage pixels = image->level(0);
        EXPECT_LE(images.residentBytes(), images.budgetBytes() + bytesOf(pixels)) << path;
    }
    hmi::SceneImage* const first = images.image(paths.front());
    EXPECT_EQ(first->width(), firstPixels.width());  // les dimensions survivent à l'éviction
    EXPECT_EQ(first->level(0), firstPixels);
}

/**
 * @brief Les niveaux réduits d'une pièce peinte se calculent à la demande ; une image engendrée
 *        n'en a pas.
 * \castest{<b>Une piece peinte a ses niveaux reduits.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger une piece de la maquette et lire son niveau 1 et son dernier niveau.<br/>
 *          2. Lire un marqueur d'entite.<br/>
 * \tattendu Le niveau 1 fait la moitie de la piece, le dernier un pixel de cote au plus ; la
 *           piece est lissee, le marqueur ne l'est pas et n'a qu'un niveau.
 * }
 */
TEST(SceneImagesTest, UnePiecePeinteASesNiveauxReduits) {
    hmi::SceneImages images(mockupDirectory());
    const std::vector<std::string> paths = mockupPaths();
    ASSERT_FALSE(paths.empty());
    hmi::SceneImage* const piece = images.image(paths.front());
    ASSERT_NE(piece, nullptr);
    EXPECT_TRUE(piece->smooth());
    const QImage half = piece->level(1);
    EXPECT_EQ(half.width(), std::max(1, piece->width() / 2));
    EXPECT_EQ(half.height(), std::max(1, piece->height() / 2));
    const QImage last = piece->level(piece->levelCount() - 1);
    EXPECT_LE(std::min(last.width(), last.height()), 1);

    const hmi::SceneImage* const marker = images.marker("marker/porte");
    ASSERT_NE(marker, nullptr);
    EXPECT_FALSE(marker->smooth());
    EXPECT_EQ(marker->levelCount(), 1);
}

/**
 * @brief Le manifeste d'un dossier se lit une fois pour toutes ses pièces (`LOT-125`, constat H6
 *        de l'audit de l'éditeur).
 * \castest{<b>Un manifeste se lit une fois.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger toutes les pieces de la maquette, rangees dans un seul dossier.<br/>
 * \tattendu Autant de dossiers lus que de dossiers distincts sur le chemin des pieces, et non un
 *           par piece.
 * }
 */
TEST(SceneImagesTest, UnManifesteSeLitUneFois) {
    hmi::SceneImages images(mockupDirectory());
    const std::vector<std::string> paths = mockupPaths();
    ASSERT_GE(paths.size(), 3U);
    images.ensure(paths);
    // Le dossier des pièces, et ceux que la remontée des figurines visite : jamais un par pièce.
    EXPECT_LE(images.manifestReads(), 3U);
    EXPECT_LT(images.manifestReads(), paths.size());
}
