// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_place_camera.cpp
 * @brief Tests unitaires de la caméra 2D (conversions monde ↔ écran, projection).
 */

#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

#include <DirectXMath.h>
#include <gtest/gtest.h>

#include "Core/Math/Vector2.h"
#include "Core/Combat/IsoProjection.h"
#include "HMI/Graphics/IsoView.h"
#include "HMI/Graphics/PlaceCamera.h"

namespace {
constexpr float TOLERANCE = 1e-3f;
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;
}  // namespace

/**
 * @brief Le centre de la caméra se projette au centre de l'écran.
 * \castest{<b>Le centre de la caméra se projette au centre de l'écran.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Le centre de la caméra se projette au centre de l'écran.
 * }
 */
TEST(PlaceCameraTest, CentreAuMilieuDeLEcran) {
    hmi::PlaceCamera camera(WIDTH, HEIGHT);
    camera.setCenter(core::Vector2{10.0f, 5.0f});

    const core::Vector2 screen = camera.worldToScreen(core::Vector2{10.0f, 5.0f});
    EXPECT_NEAR(screen.x, WIDTH * 0.5f, TOLERANCE);
    EXPECT_NEAR(screen.y, HEIGHT * 0.5f, TOLERANCE);
}

/**
 * @brief `center()`/`zoom()` renvoient exactement ce que `setCenter()`/`setZoom()` ont posé —
 * nécessaire au pan/zoom manuel de l'éditeur, qui repart du cadrage courant au premier geste.
 * \castest{<b>center() et zoom() renvoient exactement les valeurs posées.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * }
 */
TEST(PlaceCameraTest, CenterEtZoomRenvoientLesValeursPosees) {
    hmi::PlaceCamera camera(WIDTH, HEIGHT);
    camera.setCenter(core::Vector2{3.0f, 4.0f});
    camera.setZoom(2.5f);

    EXPECT_FLOAT_EQ(camera.center().x, 3.0f);
    EXPECT_FLOAT_EQ(camera.center().y, 4.0f);
    EXPECT_FLOAT_EQ(camera.zoom(), 2.5f);
}

/**
 * @brief Une unité monde vaut 16 pixels ; l'axe Y va vers le bas.
 * \castest{<b>Une unité monde vaut 16 pixels ; l'axe Y va vers le bas.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Une unité monde vaut 16 pixels ; l'axe Y va vers le bas.
 * }
 */
TEST(PlaceCameraTest, EchelleEtAxeY) {
    hmi::PlaceCamera camera(WIDTH, HEIGHT);  // centre (0,0), zoom 1 -> 16 px/unité

    const core::Vector2 right = camera.worldToScreen(core::Vector2{1.0f, 0.0f});
    EXPECT_NEAR(right.x, WIDTH * 0.5f + 16.0f, TOLERANCE);
    EXPECT_NEAR(right.y, HEIGHT * 0.5f, TOLERANCE);

    // Un Y monde positif descend à l'écran (Y-bas).
    const core::Vector2 down = camera.worldToScreen(core::Vector2{0.0f, 1.0f});
    EXPECT_NEAR(down.y, HEIGHT * 0.5f + 16.0f, TOLERANCE);
}

/**
 * @brief Le zoom multiplie l'échelle en pixels.
 * \castest{<b>Le zoom multiplie l'échelle en pixels.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Le zoom multiplie l'échelle en pixels.
 * }
 */
TEST(PlaceCameraTest, Zoom) {
    hmi::PlaceCamera camera(WIDTH, HEIGHT);
    camera.setZoom(2.0f);  // 32 px/unité

    const core::Vector2 right = camera.worldToScreen(core::Vector2{1.0f, 0.0f});
    EXPECT_NEAR(right.x, WIDTH * 0.5f + 32.0f, TOLERANCE);
}

/**
 * @brief `screenToWorld` est la réciproque de `worldToScreen`.
 * \castest{<b>`screenToWorld` est la réciproque de `worldToScreen`.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu `screenToWorld` est la réciproque de `worldToScreen`.
 * }
 */
TEST(PlaceCameraTest, ConversionsReciproques) {
    hmi::PlaceCamera camera(WIDTH, HEIGHT);
    camera.setCenter(core::Vector2{-3.0f, 7.5f});
    camera.setZoom(3.0f);

    const core::Vector2 world{4.25f, -2.5f};
    const core::Vector2 roundTrip = camera.screenToWorld(camera.worldToScreen(world));
    EXPECT_NEAR(roundTrip.x, world.x, TOLERANCE);
    EXPECT_NEAR(roundTrip.y, world.y, TOLERANCE);
}

/**
 * @brief La matrice de projection envoie le centre de la caméra à l'origine du clip space.
 * \castest{<b>La matrice de projection envoie le centre de la caméra à l'origine du clip
 * space.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu La matrice de projection envoie le centre de la caméra à l'origine du clip space.
 * }
 */
TEST(PlaceCameraTest, ProjectionCentreVersOrigineClip) {
    hmi::PlaceCamera camera(WIDTH, HEIGHT);
    camera.setCenter(core::Vector2{12.0f, -8.0f});

    const DirectX::XMFLOAT4X4 projection = camera.projectionMatrix();
    const DirectX::XMMATRIX matrix = DirectX::XMLoadFloat4x4(&projection);
    const DirectX::XMVECTOR worldCenter = DirectX::XMVectorSet(12.0f, -8.0f, 0.0f, 1.0f);
    const DirectX::XMVECTOR clip = DirectX::XMVector4Transform(worldCenter, matrix);

    DirectX::XMFLOAT4 result;
    DirectX::XMStoreFloat4(&result, clip);
    EXPECT_NEAR(result.x, 0.0f, TOLERANCE);
    EXPECT_NEAR(result.y, 0.0f, TOLERANCE);
    EXPECT_NEAR(result.w, 1.0f, TOLERANCE);
}

/**
 * @brief Un coin de l'écran correspond à un bord du clip space (±1).
 * \castest{<b>Un coin de l'écran correspond à un bord du clip space (±1).</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Un coin de l'écran correspond à un bord du clip space (±1).
 * }
 */
TEST(PlaceCameraTest, BordEcranVersBordClip) {
    hmi::PlaceCamera camera(WIDTH, HEIGHT);  // centre (0,0)

    // Bord droit du monde visible : x = (WIDTH/2)/16 unités.
    const float rightWorldX = (WIDTH * 0.5f) / hmi::PlaceCamera::PIXELS_PER_UNIT;
    const DirectX::XMFLOAT4X4 projection = camera.projectionMatrix();
    const DirectX::XMMATRIX matrix = DirectX::XMLoadFloat4x4(&projection);
    const DirectX::XMVECTOR edge = DirectX::XMVectorSet(rightWorldX, 0.0f, 0.0f, 1.0f);

    DirectX::XMFLOAT4 result;
    DirectX::XMStoreFloat4(&result, DirectX::XMVector4Transform(edge, matrix));
    EXPECT_NEAR(result.x, 1.0f, TOLERANCE);
}

/**
 * @brief fitZoom remplit la surface sans arrondir, même au-dessus de 1 (`LOT-103`).
 * \castest{<b>fitZoom remplit la surface sans arrondi a l'entier.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Cadrer un niveau de 14 x 8 unites dans une fenetre de 1280 x 720, marge 0,85.<br/>
 * \tattendu Le facteur vaut exactement 0,85 fois le plus petit rapport, sans arrondi : la grille
 * du pixel art n'est plus a proteger (EX-ARCH-022).
 * }
 */
TEST(PlaceCameraTest, FitZoomRemplitSansArrondi) {
    // 14x8 unites, 16 px/unite -> 224x128 px. Fenetre 1280x720 : min(5,714 ; 5,625) = 5,625.
    const float zoom = hmi::PlaceCamera::fitZoom(1280.0f, 720.0f, 14.0f, 8.0f, 0.85f);
    EXPECT_FLOAT_EQ(zoom, 5.625f * 0.85f);
}

/**
 * @brief fitZoom devient fractionnaire pour un niveau plus grand que la surface disponible.
 * \castest{<b>fitZoom devient fractionnaire pour un niveau plus grand que la surface
 * disponible.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu fitZoom devient fractionnaire pour un niveau plus grand que la surface disponible.
 * }
 */
TEST(PlaceCameraTest, FitZoomFractionnairePourGrandNiveau) {
    // 100x100 cases, 16 px/unite -> 1600x1600 px. Fenetre 1280x720 : le facteur brut est < 1.
    const float zoom = hmi::PlaceCamera::fitZoom(1280.0f, 720.0f, 100.0f, 100.0f, 1.0f);
    EXPECT_GT(zoom, 0.0f);
    EXPECT_LT(zoom, 1.0f);
    // Le niveau entier doit tenir dans la surface disponible a ce zoom.
    EXPECT_LE(100.0f * hmi::PlaceCamera::PIXELS_PER_UNIT * zoom, 1280.0f + TOLERANCE);
    EXPECT_LE(100.0f * hmi::PlaceCamera::PIXELS_PER_UNIT * zoom, 720.0f + TOLERANCE);
}

/**
 * @brief fitZoom applique la marge telle quelle.
 * \castest{<b>fitZoom applique la marge telle quelle.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Cadrer 16 x 16 unites dans 1280 x 1280 pixels, sans marge puis a 0,85.<br/>
 * \tattendu 5 sans marge, 4,25 avec : la marge multiplie le facteur, rien ne l'arrondit.
 * }
 */
TEST(PlaceCameraTest, FitZoomAppliqueLaMarge) {
    // Facteur brut exact = 5 (1280 / (16*16)).
    const float zoomSansMarge = hmi::PlaceCamera::fitZoom(1280.0f, 1280.0f, 16.0f, 16.0f, 1.0f);
    const float zoomAvecMarge = hmi::PlaceCamera::fitZoom(1280.0f, 1280.0f, 16.0f, 16.0f, 0.85f);
    EXPECT_FLOAT_EQ(zoomSansMarge, 5.0f);
    EXPECT_FLOAT_EQ(zoomAvecMarge, 4.25f);
}

/**
 * @brief Sans étendue de profondeur fixée, la matrice est celle de la caméra 2D d'avant le lot :
 *        le troisième axe laisse la profondeur telle quelle.
 * \castest{<b>Sans etendue de profondeur, la matrice est celle de la camera 2D.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Construire une caméra, la centrer, la zoomer, sans toucher à sa profondeur.<br/>
 *          2. Lire les seize coefficients de sa matrice.<br/>
 * \tattendu Les coefficients du plan de l'image sont ceux de la caméra 2D ; la ligne de la
 *           profondeur vaut (0, 0, 1, 0) et sa translation est un zéro positif : une scène sans
 *           volume se projette exactement comme avant.
 * }
 */
TEST(PlaceCameraTest, SansProfondeurLaMatriceEstCelleDeLaCamera2D) {
    hmi::PlaceCamera camera(WIDTH, HEIGHT);
    camera.setCenter(core::Vector2{12.0f, -8.0f});
    camera.setZoom(2.5f);
    const DirectX::XMFLOAT4X4 m = camera.projectionMatrix();

    const float scaleX = 16.0f * 2.5f * 2.0f / WIDTH;
    const float scaleY = 16.0f * 2.5f * 2.0f / HEIGHT;
    EXPECT_EQ(m(0, 0), scaleX);
    EXPECT_EQ(m(1, 1), -scaleY);
    EXPECT_EQ(m(3, 0), -12.0f * scaleX);
    EXPECT_EQ(m(3, 1), -8.0f * scaleY);
    EXPECT_EQ(m(2, 2), 1.0f);
    EXPECT_EQ(m(3, 2), 0.0f);
    EXPECT_FALSE(std::signbit(m(3, 2)));
    EXPECT_EQ(m(3, 3), 1.0f);
    for (const auto [row, column] : {std::pair{0, 1}, std::pair{0, 2}, std::pair{0, 3},
                                     std::pair{1, 0}, std::pair{1, 2}, std::pair{1, 3},
                                     std::pair{2, 0}, std::pair{2, 1}, std::pair{2, 3}}) {
        EXPECT_EQ(m(static_cast<std::size_t>(row), static_cast<std::size_t>(column)), 0.0f);
    }
}

/**
 * @brief L'étendue de profondeur se ramène entre les deux plans de la caméra sans toucher au
 *        plan de l'image.
 * \castest{<b>L'etendue de profondeur se ramene entre les deux plans de la camera.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Fixer l'étendue de profondeur d'une grille.<br/>
 *          2. Projeter un point à la profondeur la plus proche, la plus lointaine, puis au
 *             milieu.<br/>
 *          3. Fixer une étendue vide.<br/>
 * \tattendu Le plus proche tombe à -1, le plus lointain à 1, le milieu à 0 ; les coordonnées du
 *           plan de l'image sont celles de la caméra sans profondeur ; une étendue vide laisse la
 *           profondeur telle quelle au lieu de diviser par zéro.
 * }
 */
TEST(PlaceCameraTest, LEtendueDeProfondeurSeRameneEntreLesDeuxPlans) {
    const hmi::IsoView view(core::IsoProjection{12, 9});
    const hmi::DepthRange range = view.depthRange();
    hmi::PlaceCamera flat(WIDTH, HEIGHT);
    flat.setCenter(core::Vector2{20.0f, 10.0f});
    hmi::PlaceCamera camera = flat;
    camera.setDepthRange(range);
    EXPECT_EQ(camera.depthRange(), range);

    const auto project = [](const hmi::PlaceCamera& from, float x, float y, float z) {
        const DirectX::XMFLOAT4X4 projection = from.projectionMatrix();
        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, DirectX::XMVector4Transform(
                                            DirectX::XMVectorSet(x, y, z, 1.0f),
                                            DirectX::XMLoadFloat4x4(&projection)));
        return result;
    };
    EXPECT_NEAR(project(camera, 3.0f, 4.0f, range.nearest).z, -1.0f, TOLERANCE);
    EXPECT_NEAR(project(camera, 3.0f, 4.0f, range.farthest).z, 1.0f, TOLERANCE);
    EXPECT_NEAR(project(camera, 3.0f, 4.0f, (range.nearest + range.farthest) / 2.0f).z, 0.0f,
                TOLERANCE);
    EXPECT_EQ(project(camera, 3.0f, 4.0f, 7.0f).x, project(flat, 3.0f, 4.0f, 7.0f).x);
    EXPECT_EQ(project(camera, 3.0f, 4.0f, 7.0f).y, project(flat, 3.0f, 4.0f, 7.0f).y);

    camera.setDepthRange(hmi::DepthRange{.nearest = 5.0f, .farthest = 5.0f});
    EXPECT_EQ(camera.projectionMatrix()(2, 2), 1.0f);
    EXPECT_EQ(camera.projectionMatrix()(3, 2), 0.0f);
}

/**
 * @brief La matrice d'un maillage posé est la projection composée avec sa pose : son sol tombe à
 *        l'écran où la caméra met le point du sol.
 * \castest{<b>La matrice d'un maillage pose compose la pose et la projection.</b><br/>
 * \tcat Unitaire · PlaceCamera<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser un maillage au centre d'une case, sous une caméra cadrée et à l'étendue de
 *             profondeur de la grille.<br/>
 *          2. Projeter des points du maillage par sa matrice, puis par la pose suivie de la
 *             projection.<br/>
 * \tattendu Les deux chemins donnent le même point de clip ; l'origine du maillage tombe où la
 *           caméra met le centre de la case, et sa profondeur est entre les deux plans.
 * }
 */
TEST(PlaceCameraTest, LaMatriceDUnMaillageComposeLaPoseEtLaProjection) {
    const core::IsoProjection grid{12, 9};
    const hmi::IsoView view(grid);
    hmi::PlaceCamera camera(WIDTH, HEIGHT);
    camera.setCenter(grid.gridToWorld({6.0f, 4.0f}));
    camera.setZoom(1.5f);
    camera.setDepthRange(view.depthRange());

    const hmi::ViewTransform pose = view.meshTransform({4.5f, 2.5f}, 0.0f);
    const DirectX::XMFLOAT4X4 mesh = camera.meshMatrix(pose);
    const DirectX::XMFLOAT4X4 projection = camera.projectionMatrix();
    const auto transform = [](const DirectX::XMFLOAT4X4& matrix, float x, float y, float z) {
        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, DirectX::XMVector4Transform(
                                            DirectX::XMVectorSet(x, y, z, 1.0f),
                                            DirectX::XMLoadFloat4x4(&matrix)));
        return result;
    };
    for (const auto& point : {std::array<float, 3>{0.0f, 0.0f, 0.0f},
                              std::array<float, 3>{0.75f, 2.37f, -0.75f},
                              std::array<float, 3>{-2.0f, 0.5f, 3.0f}}) {
        const std::array<float, 3> inView =
            hmi::IsoView::apply(pose, point[0], point[1], point[2]);
        const DirectX::XMFLOAT4 direct = transform(mesh, point[0], point[1], point[2]);
        const DirectX::XMFLOAT4 chained = transform(projection, inView[0], inView[1], inView[2]);
        EXPECT_NEAR(direct.x, chained.x, 1e-4f);
        EXPECT_NEAR(direct.y, chained.y, 1e-4f);
        EXPECT_NEAR(direct.z, chained.z, 1e-4f);
        EXPECT_NEAR(direct.w, 1.0f, TOLERANCE);
        EXPECT_GT(direct.z, -1.0f);
        EXPECT_LT(direct.z, 1.0f);
    }
    // L'origine du maillage : le centre de la case (4, 2), à l'écran.
    const DirectX::XMFLOAT4 origin = transform(mesh, 0.0f, 0.0f, 0.0f);
    const core::Vector2 screen = camera.worldToScreen(grid.tileToWorld({.column = 4, .row = 2}));
    EXPECT_NEAR((origin.x + 1.0f) * WIDTH * 0.5f, screen.x, 1e-2f);
    EXPECT_NEAR((1.0f - origin.y) * HEIGHT * 0.5f, screen.y, 1e-2f);
}