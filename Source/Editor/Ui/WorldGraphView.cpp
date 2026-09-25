// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/WorldGraphView.h"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPolygonF>
#include <QStringList>
#include <QToolTip>
#include <algorithm>
#include <cmath>
#include <utility>

namespace hmi {

namespace {

/// Marge autour du cercle réservée aux étiquettes et aux boucles, en pixels logiques.
constexpr qreal LABEL_MARGIN = 90.0;
/// Tolérance de désignation d'une flèche, en pixels du widget.
constexpr qreal EDGE_PICK_TOLERANCE = 5.0;
/// Pointe de flèche : longueur et demi-ouverture (radians).
constexpr qreal ARROW_LENGTH = 10.0;
constexpr qreal ARROW_ANGLE = 0.45;
/// Légende : hauteur d'une ligne, marge et longueur de l'échantillon.
constexpr int LEGEND_ROW = 18;
constexpr int LEGEND_PADDING = 8;
constexpr int LEGEND_SAMPLE = 26;
constexpr int LEGEND_ROWS = 6;
/// Taille minimale et taille souhaitée du widget, en pixels.
constexpr int MINIMUM_SIDE = 160;
constexpr int PREFERRED_SIDE = 320;
/// Échelle minimale de la vue : en deçà, le graphe n'est plus lisible.
constexpr qreal MIN_SCALE = 0.05;
/// La moitié : le centre d'une étendue.
constexpr qreal HALF = 0.5;
/// Le diamètre vaut deux rayons.
constexpr qreal DIAMETER_FACTOR = 2.0;
/// Épaisseur du trait du lien qu'on tire.
constexpr qreal LINK_PEN_WIDTH = 2.0;
/// Épaisseur d'une flèche, au repos et survolée.
constexpr qreal EDGE_PEN_WIDTH = 1.6;
constexpr qreal EDGE_PEN_WIDTH_HOVERED = 3.0;
/// Épaisseur des traits d'un échantillon de la légende.
constexpr qreal LEGEND_PEN_WIDTH = 1.5;
/// Rayon des coins du badge du nombre de portails.
constexpr qreal BADGE_CORNER_RADIUS = 8.0;
/// Le rouge des erreurs : un canal fort, deux faibles.
constexpr int ERROR_RED = 0xc6;
constexpr int ERROR_OTHER = 0x28;
/// Corps de l'identifiant d'une carte, en proportion de celui de son nom.
constexpr qreal DETAIL_FONT_RATIO = 0.85;

/// Les couleurs du graphe, tirées de la palette du widget (style Fusion) : l'éditeur n'a pas de
/// charte (`LOT-EDITOR-01`).
struct GraphColors {
    QColor background;
    QColor text;
    QColor textMuted;
    QColor accent;
    QColor surface;
    QColor surfaceAlt;
    QColor error;
};

[[nodiscard]] GraphColors graphColors(const QPalette& palette) {
    return GraphColors{.background = palette.color(QPalette::Base),
                       .text = palette.color(QPalette::Text),
                       .textMuted = palette.color(QPalette::PlaceholderText),
                       .accent = palette.color(QPalette::Highlight),
                       .surface = palette.color(QPalette::Button),
                       .surfaceAlt = palette.color(QPalette::AlternateBase),
                       .error = QColor(ERROR_RED, ERROR_OTHER, ERROR_OTHER)};
}

[[nodiscard]] QPointF toPoint(core::Vector2 vector) {
    return {static_cast<qreal>(vector.x), static_cast<qreal>(vector.y)};
}

// Pointe pleine d'une flèche arrivant en @p tip depuis @p from.
[[nodiscard]] QPolygonF arrowHead(QPointF from, QPointF tip) {
    const QPointF direction = tip - from;
    const qreal angle = std::atan2(direction.y(), direction.x());
    const auto wing = [&](qreal side) {
        return tip - QPointF(std::cos(angle + side) * ARROW_LENGTH,
                             std::sin(angle + side) * ARROW_LENGTH);
    };
    return QPolygonF({tip, wing(ARROW_ANGLE), wing(-ARROW_ANGLE)});
}

}  // namespace

WorldGraphView::WorldGraphView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(MINIMUM_SIDE, MINIMUM_SIDE);
}

QSize WorldGraphView::sizeHint() const {
    return {PREFERRED_SIDE, PREFERRED_SIDE};
}

void WorldGraphView::setGraph(core::WorldGraph graph, std::filesystem::path levelsDir) {
    _graph = std::move(graph);
    _dir = std::move(levelsDir);
    _layout = layoutWorldGraph(_graph);
    _hoveredNode.reset();
    _hoveredEdge.reset();
    update();
}

qreal WorldGraphView::scale() const {
    const qreal extent =
        DIAMETER_FACTOR * (static_cast<qreal>(_layout.circleRadius) + LABEL_MARGIN);
    const qreal available = std::min<qreal>(width(), height() - (LEGEND_ROWS * LEGEND_ROW));
    return std::clamp(available / extent, MIN_SCALE, 1.0);
}

core::Vector2 WorldGraphView::toLayout(QPointF widgetPoint) const {
    const QPointF origin(width() / 2.0, (height() - (LEGEND_ROWS * LEGEND_ROW)) / 2.0);
    const QPointF local = (widgetPoint - origin) / scale();
    return {static_cast<float>(local.x()), static_cast<float>(local.y())};
}

QString WorldGraphView::nodeLabel(std::size_t node) const {
    const WorldGraphLayoutNode& n = _layout.nodes.at(node);
    if (n.ghost && n.mapId.empty()) {
        return QStringLiteral("(no target)");
    }
    // L'identifiant : le nom d'une carte est une clé de traduction (LOT-EDITOR-07).
    return QString::fromStdString(n.mapId);
}

QString WorldGraphView::statusText(core::PortalLinkStatus status) {
    switch (status) {
        case core::PortalLinkStatus::Resolved:
            return QStringLiteral("linked");
        case core::PortalLinkStatus::MissingTarget:
            return QStringLiteral("no target map");
        case core::PortalLinkStatus::UnknownMap:
            return QStringLiteral("target map not in the folder");
        case core::PortalLinkStatus::MissingArrival:
            return QStringLiteral("no arrival point");
        case core::PortalLinkStatus::UnknownArrival:
            return QStringLiteral("arrival point unknown on the target map");
        case core::PortalLinkStatus::TargetUnreadable:
            return QStringLiteral("target map unreadable");
        case core::PortalLinkStatus::Sealed:
            return QStringLiteral("sealed");
    }
    return {};
}

QString WorldGraphView::nodeToolTip(std::size_t node) const {
    const WorldGraphLayoutNode& n = _layout.nodes.at(node);
    if (n.ghost) {
        return n.mapId.empty() ? QStringLiteral("Portals without a target map.")
                               : QStringLiteral("Map “%1” is not in the folder; portals name it.")
                                     .arg(QString::fromStdString(n.mapId));
    }
    QStringList lines{
        QStringLiteral("%1 (%2)").arg(nodeLabel(node), QString::fromStdString(n.mapId))};
    if (n.unreadable) {
        lines << QStringLiteral("Unreadable map: %1").arg(QString::fromStdString(n.loadError));
    } else {
        lines << QStringLiteral("Double-click to open the map.");
    }
    return lines.join(QLatin1Char('\n'));
}

QString WorldGraphView::edgeToolTip(std::size_t edge) const {
    const WorldGraphLayoutEdge& e = _layout.edges.at(edge);
    QStringList lines{QStringLiteral("%1 → %2").arg(nodeLabel(e.from), nodeLabel(e.to))};
    for (const std::size_t index : e.portals) {
        const core::WorldPortalLink& portal = _graph.portals.at(index);
        const QString arrival = portal.arrival.empty() ? QStringLiteral("(no arrival point)")
                                                       : QString::fromStdString(portal.arrival);
        lines << QStringLiteral("(%1, %2) → “%3”: %4")
                     .arg(QString::number(portal.position.column),
                          QString::number(portal.position.row), arrival, statusText(portal.status));
    }
    return lines.join(QLatin1Char('\n'));
}

bool WorldGraphView::event(QEvent* event) {
    const auto* help =
        event->type() == QEvent::ToolTip ? dynamic_cast<const QHelpEvent*>(event) : nullptr;
    if (help != nullptr) {
        const core::Vector2 point = toLayout(help->pos());
        if (const auto node = nodeAt(_layout, point, WORLD_GRAPH_NODE_RADIUS)) {
            QToolTip::showText(help->globalPos(), nodeToolTip(*node), this);
        } else if (const auto edge =
                       edgeAt(_layout, point, static_cast<float>(EDGE_PICK_TOLERANCE / scale()))) {
            QToolTip::showText(help->globalPos(), edgeToolTip(*edge), this);
        } else {
            QToolTip::hideText();
            event->ignore();
        }
        return true;
    }
    return QWidget::event(event);
}

namespace {

/// Vrai si @p node est une carte qu'on peut relier : elle existe et se lit.
[[nodiscard]] bool isLinkable(const WorldGraphLayout& layout, std::optional<std::size_t> node) {
    if (!node || *node >= layout.nodes.size()) {
        return false;
    }
    const WorldGraphLayoutNode& candidate = layout.nodes[*node];
    return !candidate.ghost && !candidate.unreadable && !candidate.mapId.empty();
}

}  // namespace

void WorldGraphView::mousePressEvent(QMouseEvent* event) {
    // Tirer d'une carte à une autre crée le lien (LOT-EDITOR-09) ; un fantôme ne se relie pas.
    const std::optional<std::size_t> node =
        nodeAt(_layout, toLayout(event->position()), WORLD_GRAPH_NODE_RADIUS);
    if (event->button() == Qt::LeftButton && isLinkable(_layout, node)) {
        _linkFrom = node;
        _linkPoint = _layout.nodes[*node].center;
        update();
        return;
    }
    QWidget::mousePressEvent(event);
}

void WorldGraphView::mouseReleaseEvent(QMouseEvent* event) {
    if (!_linkFrom) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const std::optional<std::size_t> from = _linkFrom;
    _linkFrom.reset();
    update();
    const std::optional<std::size_t> to =
        nodeAt(_layout, toLayout(event->position()), WORLD_GRAPH_NODE_RADIUS);
    if (isLinkable(_layout, to) && *to != *from) {
        emit linkRequested(QString::fromStdString(_layout.nodes[*from].mapId),
                           QString::fromStdString(_layout.nodes[*to].mapId));
    }
    QWidget::mouseReleaseEvent(event);
}

void WorldGraphView::mouseMoveEvent(QMouseEvent* event) {
    const core::Vector2 point = toLayout(event->position());
    if (_linkFrom) {
        _linkPoint = point;
        update();
    }
    std::optional<std::size_t> node = nodeAt(_layout, point, WORLD_GRAPH_NODE_RADIUS);
    std::optional<std::size_t> edge;
    if (!node) {
        edge = edgeAt(_layout, point, static_cast<float>(EDGE_PICK_TOLERANCE / scale()));
    }
    if (node != _hoveredNode || edge != _hoveredEdge) {
        _hoveredNode = node;
        _hoveredEdge = edge;
        const WorldGraphLayoutNode* hovered = node ? &_layout.nodes[*node] : nullptr;
        const bool openable = hovered != nullptr && !hovered->ghost && !hovered->unreadable;
        setCursor(openable ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void WorldGraphView::leaveEvent(QEvent* event) {
    _hoveredNode.reset();
    _hoveredEdge.reset();
    _linkFrom.reset();  // un lien tiré hors de la vue n'aboutit pas
    update();
    QWidget::leaveEvent(event);
}

void WorldGraphView::mouseDoubleClickEvent(QMouseEvent* event) {
    const auto node = nodeAt(_layout, toLayout(event->position()), WORLD_GRAPH_NODE_RADIUS);
    if (node && event->button() == Qt::LeftButton) {
        const WorldGraphLayoutNode& n = _layout.nodes[*node];
        if (!n.ghost && !n.unreadable) {
            const std::filesystem::path path = _dir / (n.mapId + ".json");
            emit levelOpenRequested(QString::fromStdString(path.string()));
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void WorldGraphView::paintEvent(QPaintEvent* /*event*/) {
    const GraphColors colors = graphColors(palette());
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), colors.background);

    if (_layout.nodes.empty()) {
        painter.setPen(colors.textMuted);
        painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap,
                         QStringLiteral("No map in the levels folder."));
        return;
    }

    painter.save();
    painter.translate(width() * HALF, (height() - (LEGEND_ROWS * LEGEND_ROW)) * HALF);
    painter.scale(scale(), scale());
    paintEdges(painter);
    paintNodes(painter);
    if (_linkFrom && *_linkFrom < _layout.nodes.size()) {
        // Le lien qu'on tire : un trait pointillé de la carte de départ au pointeur.
        const core::Vector2 from = _layout.nodes[*_linkFrom].center;
        QPen pen(colors.text);
        pen.setStyle(Qt::DashLine);
        pen.setWidthF(LINK_PEN_WIDTH);
        painter.setPen(pen);
        painter.drawLine(QPointF(from.x, from.y), QPointF(_linkPoint.x, _linkPoint.y));
    }
    painter.restore();

    paintLegend(painter);
}

void WorldGraphView::paintEdges(QPainter& painter) const {
    const GraphColors colors = graphColors(palette());
    for (std::size_t i = 0; i < _layout.edges.size(); ++i) {
        const WorldGraphLayoutEdge& edge = _layout.edges[i];
        const WorldGraphEdgeGeometry geometry = worldGraphEdgeGeometry(_layout, i);
        const QColor color = edge.broken ? colors.error : colors.textMuted;
        const bool hovered = _hoveredEdge == i;
        // Condamné : voulu, il se montre en pointillé discret, pas en erreur (LOT-126).
        Qt::PenStyle style = Qt::SolidLine;
        if (edge.broken) {
            style = Qt::DashLine;
        } else if (edge.sealed) {
            style = Qt::DotLine;
        }
        QPen pen(color, hovered ? EDGE_PEN_WIDTH_HOVERED : EDGE_PEN_WIDTH, style);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        if (edge.selfLoop) {
            painter.drawEllipse(toPoint(geometry.loopCenter), geometry.loopRadius,
                                geometry.loopRadius);
        } else {
            const QPointF start = toPoint(geometry.start);
            const QPointF end = toPoint(geometry.end);
            painter.drawLine(start, end);
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawPolygon(arrowHead(start, end));
        }

        if (edge.count() > 1) {
            const QString text = QString::number(edge.count());
            const QFontMetrics metrics(painter.font());
            const qreal diameter = std::max<qreal>(16.0, metrics.horizontalAdvance(text) + 8.0);
            const QRectF badge(toPoint(geometry.badge) - QPointF(diameter / 2.0, 8.0),
                               QSizeF(diameter, 16.0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawRoundedRect(badge, BADGE_CORNER_RADIUS, BADGE_CORNER_RADIUS);
            painter.setPen(colors.background);
            painter.drawText(badge, Qt::AlignCenter, text);
        }
    }
}

void WorldGraphView::paintNodes(QPainter& painter) const {
    const GraphColors colors = graphColors(palette());
    const QFont baseFont = painter.font();
    QFont nameFont = baseFont;
    nameFont.setBold(true);
    QFont idFont = baseFont;
    idFont.setPointSizeF(baseFont.pointSizeF() * DETAIL_FONT_RATIO);

    for (std::size_t i = 0; i < _layout.nodes.size(); ++i) {
        const WorldGraphLayoutNode& node = _layout.nodes[i];
        const QPointF center = toPoint(node.center);
        const bool hovered = _hoveredNode == i;
        const qreal penWidth = hovered ? 3.0 : 1.8;

        if (node.ghost) {
            painter.setPen(QPen(colors.textMuted, penWidth, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
        } else if (node.unreadable) {
            painter.setPen(QPen(colors.error, penWidth, Qt::DotLine));
            painter.setBrush(colors.surfaceAlt);
        } else {
            painter.setPen(QPen(colors.accent, penWidth));
            painter.setBrush(colors.surface);
        }
        painter.drawEllipse(center, WORLD_GRAPH_NODE_RADIUS, WORLD_GRAPH_NODE_RADIUS);

        // Marque au cœur du disque : « ? » pour un fantôme, « ! » pour une carte illisible.
        if (node.ghost || node.unreadable) {
            painter.setFont(nameFont);
            painter.setPen(node.ghost ? colors.textMuted : colors.error);
            const QRectF heart(
                center - QPointF(WORLD_GRAPH_NODE_RADIUS, WORLD_GRAPH_NODE_RADIUS),
                QSizeF(2.0 * WORLD_GRAPH_NODE_RADIUS, 2.0 * WORLD_GRAPH_NODE_RADIUS));
            painter.drawText(heart, Qt::AlignCenter,
                             node.ghost ? QStringLiteral("?") : QStringLiteral("!"));
        }

        const qreal labelWidth = static_cast<qreal>(WORLD_GRAPH_NODE_SPACING) - 8.0;
        const QRectF nameRect(center.x() - (labelWidth / 2.0),
                              center.y() + WORLD_GRAPH_NODE_RADIUS + 2.0, labelWidth, 18.0);
        const QRectF idRect = nameRect.translated(0.0, 16.0);

        QFont labelFont = nameFont;
        labelFont.setItalic(node.ghost);
        painter.setFont(labelFont);
        painter.setPen(node.ghost ? colors.textMuted : colors.text);
        const QString name = nodeLabel(i);
        painter.drawText(
            nameRect, Qt::AlignHCenter | Qt::AlignTop,
            QFontMetrics(labelFont).elidedText(name, Qt::ElideRight, static_cast<int>(labelWidth)));

        const QString detail = nodeDetail(node);
        if (!detail.isEmpty()) {
            painter.setFont(idFont);
            painter.setPen(node.unreadable ? colors.error : colors.textMuted);
            painter.drawText(idRect, Qt::AlignHCenter | Qt::AlignTop,
                             QFontMetrics(idFont).elidedText(detail, Qt::ElideRight,
                                                             static_cast<int>(labelWidth)));
        }
    }
    painter.setFont(baseFont);
}

QString WorldGraphView::nodeDetail(const WorldGraphLayoutNode& node) {
    // L'état d'une carte à problème.
    if (node.unreadable) {
        return QStringLiteral("unreadable");
    }
    if (node.ghost) {
        return QStringLiteral("missing");
    }
    return {};
}

void WorldGraphView::paintLegend(QPainter& painter) const {
    const GraphColors colors = graphColors(palette());
    const qreal radius = 6.0;
    int y = height() - (LEGEND_ROWS * LEGEND_ROW) + (LEGEND_ROW / 2) - (LEGEND_PADDING / 2);
    const int x = LEGEND_PADDING;
    const int textX = x + LEGEND_SAMPLE + LEGEND_PADDING;

    const auto label = [&](const QString& text) {
        painter.setPen(colors.textMuted);
        painter.drawText(QRectF(textX, y - (LEGEND_ROW * HALF), width() - textX, LEGEND_ROW),
                         Qt::AlignLeft | Qt::AlignVCenter, text);
        y += LEGEND_ROW;
    };
    const QPointF sample(x + (LEGEND_SAMPLE / 2.0), 0.0);

    painter.setPen(QPen(colors.accent, LEGEND_PEN_WIDTH));
    painter.setBrush(colors.surface);
    painter.drawEllipse(sample + QPointF(0.0, y), radius, radius);
    label(QStringLiteral("Map"));

    painter.setPen(QPen(colors.error, LEGEND_PEN_WIDTH, Qt::DotLine));
    painter.setBrush(colors.surfaceAlt);
    painter.drawEllipse(sample + QPointF(0.0, y), radius, radius);
    label(QStringLiteral("Unreadable map"));

    painter.setPen(QPen(colors.textMuted, LEGEND_PEN_WIDTH, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(sample + QPointF(0.0, y), radius, radius);
    label(QStringLiteral("Missing map"));

    painter.setPen(QPen(colors.textMuted, EDGE_PEN_WIDTH));
    painter.drawLine(QPointF(x, y), QPointF(x + LEGEND_SAMPLE, y));
    label(QStringLiteral("Portal"));

    painter.setPen(QPen(colors.error, EDGE_PEN_WIDTH, Qt::DashLine));
    painter.drawLine(QPointF(x, y), QPointF(x + LEGEND_SAMPLE, y));
    label(QStringLiteral("Broken portal"));

    painter.setPen(QPen(colors.textMuted, EDGE_PEN_WIDTH, Qt::DotLine));
    painter.drawLine(QPointF(x, y), QPointF(x + LEGEND_SAMPLE, y));
    label(QStringLiteral("Sealed portal"));
}

}  // namespace hmi
