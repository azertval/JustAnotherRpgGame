// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/ScenePainter.h"

#include <QPainter>
#include <QPolygonF>
#include <cmath>
#include <numbers>

#include "Editor/Ui/SceneImages.h"
#include "HMI/Graphics/Camera2D.h"

namespace hmi {

namespace {

[[nodiscard]] bool intersects(const core::Rect& a, const core::Rect& b) noexcept {
    return a.position.x < b.position.x + b.size.x && b.position.x < a.position.x + a.size.x &&
           a.position.y < b.position.y + b.size.y && b.position.y < a.position.y + a.size.y;
}

[[nodiscard]] bool isSolid(const QImage* image) noexcept {
    // L'aplat : une image 1 × 1 blanche, celle que `SceneImages::solid` désigne.
    return image != nullptr && image->width() == 1 && image->height() == 1 &&
           image->pixel(0, 0) == 0xFFFFFFFFU;
}

[[nodiscard]] QColor tintOf(float r, float g, float b, float a) {
    return QColor::fromRgbF(r, g, b, a);
}

void paintSprite(QPainter& painter, const SpriteQuad& quad, const QImage* image, float opacity) {
    const QRectF target(quad.x, quad.y, quad.width, quad.height);
    const auto width = static_cast<float>(image->width());
    const auto height = static_cast<float>(image->height());
    const QRectF source(quad.u0 * width, quad.v0 * height, (quad.u1 - quad.u0) * width,
                        (quad.v1 - quad.v0) * height);
    if (source.width() == 0.0 || source.height() == 0.0) {
        return;  // une région d'image vide ne dessine rien, comme sur le GPU.
    }
    const bool rotated = quad.rotation != 0.0F;
    if (rotated) {
        painter.save();
        painter.translate(target.center());
        painter.rotate(static_cast<double>(quad.rotation) * 180.0 / std::numbers::pi);
        painter.translate(-target.center());
    }
    if (isSolid(image)) {
        painter.fillRect(target, tintOf(quad.r, quad.g, quad.b, quad.a * opacity));
    } else {
        // Un remplissage texturé plutôt que `drawImage` : le moteur raster échantillonne alors au
        // centre de chaque pixel couvert, comme le GPU, là où l'agrandissement de `drawImage`
        // décale d'un demi-pixel et ouvre des jours entre les losanges du sol.
        QTransform mapping;
        mapping.translate(target.x(), target.y());
        mapping.scale(target.width() / source.width(), target.height() / source.height());
        mapping.translate(-source.x(), -source.y());
        QBrush brush(*image);
        brush.setTransform(mapping);
        painter.setOpacity(static_cast<double>(quad.a * opacity));
        painter.fillRect(target, brush);
        painter.setOpacity(1.0);
    }
    if (rotated) {
        painter.restore();
    }
}

void paintLine(QPainter& painter, const LineQuad& quad, float opacity) {
    const float dx = quad.bx - quad.ax;
    const float dy = quad.by - quad.ay;
    const float length = std::sqrt((dx * dx) + (dy * dy));
    if (length <= 0.0F) {
        return;
    }
    const float nx = -dy / length * quad.thickness / 2.0F;
    const float ny = dx / length * quad.thickness / 2.0F;
    const QPolygonF band{{quad.ax + nx, quad.ay + ny},
                         {quad.bx + nx, quad.by + ny},
                         {quad.bx - nx, quad.by - ny},
                         {quad.ax - nx, quad.ay - ny}};
    painter.setPen(Qt::NoPen);
    painter.setBrush(tintOf(quad.r, quad.g, quad.b, quad.a * opacity));
    painter.drawPolygon(band);
}

void paintPoly(QPainter& painter, const PolyQuad& quad, float opacity) {
    const QPolygonF shape{{quad.x[0], quad.y[0]},
                          {quad.x[1], quad.y[1]},
                          {quad.x[2], quad.y[2]},
                          {quad.x[3], quad.y[3]}};
    painter.setPen(Qt::NoPen);
    painter.setBrush(tintOf(quad.r, quad.g, quad.b, quad.a * opacity));
    painter.drawPolygon(shape);
}

}  // namespace

void paintComposedScene(QPainter& painter, const ComposedScene& scene,
                        const std::optional<core::Rect>& visible, const QuadOpacity& opacity) {
    // Au plus proche, comme le sampler du jeu (`hmi::SpriteBatch`) : lisser brouillerait l'art.
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (const ComposedQuad& quad : scene.quads()) {
        const QImage* const image = sceneImageOf(quad.texture);
        if (image == nullptr || image->isNull()) {
            continue;  // primitive sans texture liée : rien à dessiner, comme en jeu.
        }
        const float extra = opacity ? opacity(quad) : 1.0F;
        if (extra <= 0.0F) {
            continue;
        }
        switch (quad.kind) {
            case QuadKind::Sprite:
                if (visible && !intersects(*visible, spriteQuadBounds(quad.sprite))) {
                    continue;
                }
                paintSprite(painter, quad.sprite, image, extra);
                break;
            case QuadKind::Line:
                if (visible && !intersects(*visible, lineQuadBounds(quad.line))) {
                    continue;
                }
                paintLine(painter, quad.line, extra);
                break;
            case QuadKind::Poly:
                if (visible && !intersects(*visible, polyQuadBounds(quad.poly))) {
                    continue;
                }
                paintPoly(painter, quad.poly, extra);
                break;
        }
    }
}

QTransform cameraTransform(const Camera2D& camera) {
    // pixel = (monde - centre) × échelle + demi-surface : la matrice du jeu, sans le passage au
    // clip.
    const core::Vector2 origin = camera.worldToScreen({0.0F, 0.0F});
    const auto scale = static_cast<double>(camera.worldToScreen({1.0F, 0.0F}).x - origin.x);
    return {scale, 0.0, 0.0, scale, static_cast<double>(origin.x), static_cast<double>(origin.y)};
}

QImage renderComposedScene(const ComposedScene& scene, const Camera2D& camera, int width,
                           int height, const QColor& clear) {
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(clear);
    QPainter painter(&image);
    painter.setTransform(cameraTransform(camera));
    paintComposedScene(painter, scene, camera.visibleBounds());
    painter.end();
    return image;
}

}  // namespace hmi
