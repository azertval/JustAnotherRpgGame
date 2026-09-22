// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_camera2d.cpp
 * @brief Tests unitaires de la caméra 2D (conversions monde ↔ écran, projection).
 */

#include <cmath>

#include <DirectXMath.h>
#include <gtest/gtest.h>

#include "Core/Math/Vector2.h"
#include "HMI/Graphics/Camera2D.h"

namespace {
constexpr float TOLERANCE = 1e-3f;
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;
}  // namespace

/**
 * @brief Le centre de la caméra se projette au centre de l'écran.
 * \castest{<b>Le centre de la caméra se projette au centre de l'écran.</b><br/>
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Le centre de la caméra se projette au centre de l'écran.
 * }
 */
TEST(Camera2DTest, CentreAuMilieuDeLEcran) {
    hmi::Camera2D camera(WIDTH, HEIGHT);
    camera.setCenter(core::Vector2{10.0f, 5.0f});

    const core::Vector2 screen = camera.worldToScreen(core::Vector2{10.0f, 5.0f});
    EXPECT_NEAR(screen.x, WIDTH * 0.5f, TOLERANCE);
    EXPECT_NEAR(screen.y, HEIGHT * 0.5f, TOLERANCE);
}

/**
 * @brief `center()`/`zoom()` renvoient exactement ce que `setCenter()`/`setZoom()` ont posé —
 * nécessaire au pan/zoom manuel de l'éditeur, qui repart du cadrage courant au premier geste.
 * \castest{<b>center() et zoom() renvoient exactement les valeurs posées.</b><br/>
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * }
 */
TEST(Camera2DTest, CenterEtZoomRenvoientLesValeursPosees) {
    hmi::Camera2D camera(WIDTH, HEIGHT);
    camera.setCenter(core::Vector2{3.0f, 4.0f});
    camera.setZoom(2.5f);

    EXPECT_FLOAT_EQ(camera.center().x, 3.0f);
    EXPECT_FLOAT_EQ(camera.center().y, 4.0f);
    EXPECT_FLOAT_EQ(camera.zoom(), 2.5f);
}

/**
 * @brief Une unité monde vaut 16 pixels ; l'axe Y va vers le bas.
 * \castest{<b>Une unité monde vaut 16 pixels ; l'axe Y va vers le bas.</b><br/>
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Une unité monde vaut 16 pixels ; l'axe Y va vers le bas.
 * }
 */
TEST(Camera2DTest, EchelleEtAxeY) {
    hmi::Camera2D camera(WIDTH, HEIGHT);  // centre (0,0), zoom 1 -> 16 px/unité

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
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Le zoom multiplie l'échelle en pixels.
 * }
 */
TEST(Camera2DTest, Zoom) {
    hmi::Camera2D camera(WIDTH, HEIGHT);
    camera.setZoom(2.0f);  // 32 px/unité

    const core::Vector2 right = camera.worldToScreen(core::Vector2{1.0f, 0.0f});
    EXPECT_NEAR(right.x, WIDTH * 0.5f + 32.0f, TOLERANCE);
}

/**
 * @brief `screenToWorld` est la réciproque de `worldToScreen`.
 * \castest{<b>`screenToWorld` est la réciproque de `worldToScreen`.</b><br/>
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu `screenToWorld` est la réciproque de `worldToScreen`.
 * }
 */
TEST(Camera2DTest, ConversionsReciproques) {
    hmi::Camera2D camera(WIDTH, HEIGHT);
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
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu La matrice de projection envoie le centre de la caméra à l'origine du clip space.
 * }
 */
TEST(Camera2DTest, ProjectionCentreVersOrigineClip) {
    hmi::Camera2D camera(WIDTH, HEIGHT);
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
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Un coin de l'écran correspond à un bord du clip space (±1).
 * }
 */
TEST(Camera2DTest, BordEcranVersBordClip) {
    hmi::Camera2D camera(WIDTH, HEIGHT);  // centre (0,0)

    // Bord droit du monde visible : x = (WIDTH/2)/16 unités.
    const float rightWorldX = (WIDTH * 0.5f) / hmi::Camera2D::PIXELS_PER_UNIT;
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
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Cadrer un niveau de 14 x 8 unites dans une fenetre de 1280 x 720, marge 0,85.<br/>
 * \tattendu Le facteur vaut exactement 0,85 fois le plus petit rapport, sans arrondi : la grille
 * du pixel art n'est plus a proteger (EX-ARCH-022).
 * }
 */
TEST(Camera2DTest, FitZoomRemplitSansArrondi) {
    // 14x8 unites, 16 px/unite -> 224x128 px. Fenetre 1280x720 : min(5,714 ; 5,625) = 5,625.
    const float zoom = hmi::Camera2D::fitZoom(1280.0f, 720.0f, 14.0f, 8.0f, 0.85f);
    EXPECT_FLOAT_EQ(zoom, 5.625f * 0.85f);
}

/**
 * @brief fitZoom devient fractionnaire pour un niveau plus grand que la surface disponible.
 * \castest{<b>fitZoom devient fractionnaire pour un niveau plus grand que la surface
 * disponible.</b><br/>
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu fitZoom devient fractionnaire pour un niveau plus grand que la surface disponible.
 * }
 */
TEST(Camera2DTest, FitZoomFractionnairePourGrandNiveau) {
    // 100x100 cases, 16 px/unite -> 1600x1600 px. Fenetre 1280x720 : le facteur brut est < 1.
    const float zoom = hmi::Camera2D::fitZoom(1280.0f, 720.0f, 100.0f, 100.0f, 1.0f);
    EXPECT_GT(zoom, 0.0f);
    EXPECT_LT(zoom, 1.0f);
    // Le niveau entier doit tenir dans la surface disponible a ce zoom.
    EXPECT_LE(100.0f * hmi::Camera2D::PIXELS_PER_UNIT * zoom, 1280.0f + TOLERANCE);
    EXPECT_LE(100.0f * hmi::Camera2D::PIXELS_PER_UNIT * zoom, 720.0f + TOLERANCE);
}

/**
 * @brief fitZoom applique la marge telle quelle.
 * \castest{<b>fitZoom applique la marge telle quelle.</b><br/>
 * \tcat Unitaire · Camera2 D<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Cadrer 16 x 16 unites dans 1280 x 1280 pixels, sans marge puis a 0,85.<br/>
 * \tattendu 5 sans marge, 4,25 avec : la marge multiplie le facteur, rien ne l'arrondit.
 * }
 */
TEST(Camera2DTest, FitZoomAppliqueLaMarge) {
    // Facteur brut exact = 5 (1280 / (16*16)).
    const float zoomSansMarge = hmi::Camera2D::fitZoom(1280.0f, 1280.0f, 16.0f, 16.0f, 1.0f);
    const float zoomAvecMarge = hmi::Camera2D::fitZoom(1280.0f, 1280.0f, 16.0f, 16.0f, 0.85f);
    EXPECT_FLOAT_EQ(zoomSansMarge, 5.0f);
    EXPECT_FLOAT_EQ(zoomAvecMarge, 4.25f);
}
