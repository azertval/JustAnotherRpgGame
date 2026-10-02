// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QFile>
#include <QMatrix4x4>
#include <QString>
#include <stdexcept>
#include <string>

#include <DirectXMath.h>
#include <rhi/qrhi.h>

/**
 * @file HMI/Graphics/RhiShaders.h
 * @brief Ce que les deux pipelines du rendu — les quads (`hmi::SpriteBatch`) et les maillages
 *        (`hmi::MeshBatch`, `LOT-1003`) — font de la même façon : lire un shader précompilé, et
 *        passer une matrice de la caméra à l'espace de clip du backend.
 */

namespace hmi {

/**
 * @brief Charge un shader précompilé (`.qsb`) depuis les ressources de l'exécutable.
 *
 * Un shader absent est une erreur de **build**, pas un état d'exécution récupérable : la ressource
 * est embarquée par `qt6_add_shaders`, elle ne peut manquer que si la compilation n'a pas eu lieu.
 * @throws std::runtime_error si la ressource manque ou ne se lit pas.
 */
[[nodiscard]] inline QShader loadShader(const char* resourcePath) {
    QFile file(QString::fromLatin1(resourcePath));
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(std::string("Shader introuvable dans les ressources : ") +
                                 resourcePath);
    }
    const QShader shader = QShader::fromSerialized(file.readAll());
    if (!shader.isValid()) {
        throw std::runtime_error(std::string("Shader illisible : ") + resourcePath);
    }
    return shader;
}

/**
 * @brief Convertit la matrice ligne-major de DirectXMath (convention `position * matrice`) en
 *        `QMatrix4x4` (convention `matrice * position`, comme GLSL) : c'est exactement sa
 *        transposée.
 *
 * La correction d'espace de clip de QRhi est appliquée par-dessus — le shader écrit en convention
 * OpenGL, QRhi la ramène à celle du backend (Direct3D 11 sous Windows).
 */
[[nodiscard]] inline QMatrix4x4 toClipMatrix(QRhi* rhi, const DirectX::XMFLOAT4X4& projection) {
    const QMatrix4x4 columnMajor(
        projection(0, 0), projection(1, 0), projection(2, 0), projection(3, 0),  //
        projection(0, 1), projection(1, 1), projection(2, 1), projection(3, 1),  //
        projection(0, 2), projection(1, 2), projection(2, 2), projection(3, 2),  //
        projection(0, 3), projection(1, 3), projection(2, 3), projection(3, 3));
    return rhi->clipSpaceCorrMatrix() * columnMajor;
}

}  // namespace hmi
