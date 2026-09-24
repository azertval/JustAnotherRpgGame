// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/CityMapView.h"

#include <QEvent>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QToolTip>
#include <utility>

namespace hmi {

namespace {

/// Hauteur réservée au nom d'un quartier, sous son cadre.
constexpr int LABEL_HEIGHT = 16;

/// Taille proposée de la vue : le plan tient sans écraser les cadres.
constexpr int PREFERRED_WIDTH = 420;
constexpr int PREFERRED_HEIGHT = 320;

}  // namespace

CityMapView::CityMapView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(240, 180);
}

void CityMapView::setCity(CityView city, std::filesystem::path dataRoot) {
    _city = std::move(city);
    _dataRoot = std::move(dataRoot);
    _plan = QPixmap();
    if (!_city.image.empty()) {
        // Le plan peint par l'auteur (LOT-94) ; absent, la vue se contente de ses cadres.
        _plan.load(QString::fromStdString((_dataRoot / "Assets" / "Maps" / _city.image).string()));
    }
    _hovered.reset();
    update();
}

QSize CityMapView::sizeHint() const {
    return {PREFERRED_WIDTH, PREFERRED_HEIGHT};
}

QRectF CityMapView::planRect() const {
    const QRectF area = rect().adjusted(4, 4, -4, -4);
    if (_plan.isNull() || _plan.width() <= 0 || _plan.height() <= 0) {
        return area;
    }
    const double ratio = static_cast<double>(_plan.width()) / _plan.height();
    double width = area.width();
    double height = width / ratio;
    if (height > area.height()) {
        height = area.height();
        width = height * ratio;
    }
    return {area.left() + ((area.width() - width) / 2.0),
            area.top() + ((area.height() - height) / 2.0), width, height};
}

MapPoint CityMapView::toPlan(QPointF widgetPoint) const {
    const QRectF plan = planRect();
    if (plan.width() <= 0.0 || plan.height() <= 0.0) {
        return {};
    }
    return MapPoint{.x = (widgetPoint.x() - plan.left()) / plan.width(),
                    .y = (widgetPoint.y() - plan.top()) / plan.height()};
}

std::optional<std::size_t> CityMapView::districtUnder(QPointF widgetPoint) const {
    return districtAt(_city, toPlan(widgetPoint));
}

void CityMapView::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().color(QPalette::Base));

    if (!_city.ok()) {
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap,
                         QString::fromStdString(_city.error));
        return;
    }
    const QRectF plan = planRect();
    if (!_plan.isNull()) {
        painter.drawPixmap(plan, _plan, QRectF(_plan.rect()));
    } else {
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(plan, Qt::AlignCenter, QStringLiteral("No painted plan for this city."));
    }

    for (std::size_t index = 0; index < _city.districts.size(); ++index) {
        const CityDistrictView& district = _city.districts[index];
        if (!district.framed) {
            continue;
        }
        const QRectF frame(plan.left() + (district.frame.x * plan.width()),
                           plan.top() + (district.frame.y * plan.height()),
                           district.frame.width * plan.width(),
                           district.frame.height * plan.height());
        const bool walkable = district.mapExists;
        QPen pen(walkable ? palette().color(QPalette::Highlight)
                          : palette().color(QPalette::PlaceholderText));
        pen.setWidthF(_hovered == index ? 3.0 : 2.0);
        pen.setStyle(walkable ? Qt::SolidLine : Qt::DashLine);
        painter.setPen(pen);
        painter.drawRect(frame);
        painter.setPen(walkable ? palette().color(QPalette::Text)
                                : palette().color(QPalette::PlaceholderText));
        painter.drawText(QRectF(frame.left(), frame.bottom(), frame.width(), LABEL_HEIGHT),
                         Qt::AlignHCenter | Qt::AlignTop, QString::fromStdString(district.name));
    }
}

void CityMapView::mouseMoveEvent(QMouseEvent* event) {
    const std::optional<std::size_t> hovered = districtUnder(event->position());
    if (hovered != _hovered) {
        _hovered = hovered;
        const bool openable = hovered && _city.districts[*hovered].mapExists;
        setCursor(openable ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void CityMapView::leaveEvent(QEvent* event) {
    _hovered.reset();
    update();
    QWidget::leaveEvent(event);
}

void CityMapView::mouseDoubleClickEvent(QMouseEvent* event) {
    const std::optional<std::size_t> district = districtUnder(event->position());
    if (district && event->button() == Qt::LeftButton) {
        const CityDistrictView& chosen = _city.districts[*district];
        if (chosen.mapExists) {
            emit levelOpenRequested(
                QString::fromStdString((_dataRoot / "Levels" / (chosen.mapId + ".json")).string()));
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool CityMapView::event(QEvent* event) {
    if (event->type() != QEvent::ToolTip) {
        return QWidget::event(event);
    }
    // QEvent::ToolTip garantit le type dynamique QHelpEvent, verifie juste au-dessus (idiome Qt
    // courant).
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto* const help = static_cast<QHelpEvent*>(event);
    const std::optional<std::size_t> district = districtUnder(help->pos());
    if (!district) {
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    const CityDistrictView& chosen = _city.districts[*district];
    QString text = QString::fromStdString(chosen.name) + QStringLiteral("\n") +
                   QString::fromStdString(chosen.id);
    if (chosen.mapExists) {
        text += QStringLiteral("\nMap: %1 — double-click to open")
                    .arg(QString::fromStdString(chosen.mapId));
    } else if (!chosen.mapId.empty()) {
        text += QStringLiteral("\nMap %1 is missing").arg(QString::fromStdString(chosen.mapId));
    } else {
        text += QStringLiteral("\nNo map: a guarded gate on %1")
                    .arg(QString::fromStdString(chosen.guardMapId));
    }
    QToolTip::showText(help->globalPos(), text, this);
    return true;
}

}  // namespace hmi
