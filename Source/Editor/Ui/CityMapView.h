// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QPixmap>
#include <QString>
#include <QWidget>
#include <cstddef>
#include <filesystem>
#include <optional>

#include "Editor/Logic/CityView.h"

/**
 * @file Editor/Ui/CityMapView.h
 * @brief La **vue de ville** du navigateur de cartes (`LOT-EDITOR-09`, `EX-EDIT-090`) : le plan
 *        peint de la ville, les cadres de ses quartiers, et la carte qu'un clic ouvre.
 */

class QPainter;

namespace hmi {

/**
 * @brief Peint une `hmi::CityView` sur le plan de sa ville.
 *
 * Chaque quartier cadré se dessine sur le plan, son nom dessous ; un quartier dont la carte
 * manque est tireté et atténué — c'est le tableau de bord d'une ville en cours de construction.
 * Double-cliquer un quartier qui a sa carte émet `levelOpenRequested`, le même signal que la
 * liste et que le graphe ; un quartier sans carte le dit dans l'infobulle, sans rien ouvrir.
 *
 * La vue ne récrit ni la ville ni `world-maps.json` (décision de l'auteur) : on la regarde.
 */
class CityMapView : public QWidget {
    Q_OBJECT

public:
    explicit CityMapView(QWidget* parent = nullptr);

    /// Montre @p city, dont le plan est cherché dans `<dataRoot>/Assets/Maps`.
    void setCity(CityView city, std::filesystem::path dataRoot);

    /// @return La ville montrée (pour les tests et le diagnostic).
    [[nodiscard]] const CityView& city() const noexcept {
        return _city;
    }

    [[nodiscard]] QSize sizeHint() const override;

signals:
    /// Émis au double-clic sur un quartier qui a sa carte (chemin absolu du fichier).
    void levelOpenRequested(const QString& path);

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    /// @return Le rectangle du plan dans le widget : l'image entière, proportions gardées.
    [[nodiscard]] QRectF planRect() const;
    /// @return Le point du plan, en fractions, sous @p widgetPoint.
    [[nodiscard]] MapPoint toPlan(QPointF widgetPoint) const;
    /// @return Le quartier sous @p widgetPoint.
    [[nodiscard]] std::optional<std::size_t> districtUnder(QPointF widgetPoint) const;

    CityView _city;
    std::filesystem::path _dataRoot;
    QPixmap _plan;
    std::optional<std::size_t> _hovered;
};

}  // namespace hmi
