// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QString>
#include <QWidget>
#include <cstddef>
#include <filesystem>
#include <optional>

#include "Core/World/WorldGraph.h"
#include "Editor/Logic/WorldGraphLayout.h"

/**
 * @file Editor/Ui/WorldGraphView.h
 * @brief Vue « graphe du monde » du navigateur de cartes (`LOT-11`) : les cartes du dossier des
 *        niveaux et les portails qui les relient, peints au `QPainter`.
 */

class QPainter;

namespace hmi {

/**
 * @brief Peint un `core::WorldGraph` disposé par `hmi::layoutWorldGraph`.
 *
 * Une carte est un disque (nom et identifiant dessous) ; une carte illisible a un contour
 * pointillé de la couleur d'erreur ; une carte absente (fantôme) n'est qu'un contour tireté
 * atténué. Une flèche par paire ordonnée de cartes, tiretée en couleur d'erreur si un de ses
 * portails est cassé, avec une pastille de compte quand elle en regroupe plusieurs. Survoler une
 * carte ou une flèche en donne le détail en infobulle ; double-cliquer une carte lisible émet
 * `levelOpenRequested`, le même signal que la liste du navigateur.
 *
 * Les couleurs sont celles de la palette du widget (style Fusion), relues à chaque
 * peinture : la vue suit une bascule de thème à chaud.
 */
class WorldGraphView : public QWidget {
    Q_OBJECT

public:
    explicit WorldGraphView(QWidget* parent = nullptr);

    /// Remplace le graphe affiché ; @p levelsDir sert à rebâtir le chemin d'une carte à ouvrir.
    void setGraph(core::WorldGraph graph, std::filesystem::path levelsDir);

    /// @return La disposition peinte (pour les tests et le diagnostic).
    [[nodiscard]] const WorldGraphLayout& graphLayout() const noexcept {
        return _layout;
    }

    [[nodiscard]] QSize sizeHint() const override;

signals:
    /// Émis au double-clic sur une carte lisible (chemin absolu du fichier).
    void levelOpenRequested(const QString& path);

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    /// @return Le point de disposition sous le point @p widgetPoint du widget.
    [[nodiscard]] core::Vector2 toLayout(QPointF widgetPoint) const;
    /// @return L'échelle appliquée à la disposition pour la faire tenir dans le widget.
    [[nodiscard]] qreal scale() const;
    /// @return Libellé court d'un nœud (nom, ou identifiant, ou « sans cible »).
    [[nodiscard]] QString nodeLabel(std::size_t node) const;
    [[nodiscard]] static QString statusText(core::PortalLinkStatus status);
    [[nodiscard]] QString nodeToolTip(std::size_t node) const;
    [[nodiscard]] QString edgeToolTip(std::size_t edge) const;

    void paintEdges(QPainter& painter) const;
    void paintNodes(QPainter& painter) const;
    /**
     * @brief Seconde ligne sous un nœud.
     * @param node Nœud dessiné.
     * @return L'état d'une carte à problème, ou rien : le nœud porte déjà l'identifiant.
     */
    [[nodiscard]] static QString nodeDetail(const WorldGraphLayoutNode& node);
    void paintLegend(QPainter& painter) const;

    core::WorldGraph _graph;
    WorldGraphLayout _layout;
    std::filesystem::path _dir;
    std::optional<std::size_t> _hoveredNode;
    std::optional<std::size_t> _hoveredEdge;
};

}  // namespace hmi
