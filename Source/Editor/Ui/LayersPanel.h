// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QString>
#include <QWidget>
#include <cstddef>
#include <memory>
#include <vector>

#include "Core/Levels/TileLayer.h"
#include "Editor/Logic/LayerView.h"

/**
 * @file Editor/Ui/LayersPanel.h
 * @brief Panneau « Couches » : couche active, visibilité, opacité, ajout, retrait, ordre et nom
 *        (`LOT-11`).
 */

class QListWidgetItem;

namespace core {
class LevelDraft;
}

namespace hmi {

/**
 * @brief Vue des couches du brouillon courant.
 *
 * **Vue, pas état** — même patron que `hmi::LinkPanel` : `refresh` reconstruit la liste depuis le
 * brouillon et les réglages du viewport, et chaque geste est une **demande** que le viewport, seul
 * propriétaire du brouillon, applique. Ajout, retrait, ordre et nom passent donc par l'historique ;
 * visibilité et opacité, aides d'édition, n'y passent pas.
 */
class LayersPanel : public QWidget {
    Q_OBJECT

public:
    explicit LayersPanel(QWidget* parent = nullptr);
    ~LayersPanel() override;

    /// Reconstruit la liste. Sans effet visible si rien n'a changé : la sélection et l'édition en
    /// cours d'un nom ne sautent pas à chaque coup de pinceau.
    void refresh(const core::LevelDraft& draft, LayerSlot active, const LayerViewState& view);

signals:
    void activeLayerRequested(hmi::LayerSlot slot);
    void visibilityRequested(hmi::LayerSlot slot, bool visible);
    void opacityRequested(hmi::LayerSlot slot, float opacity);
    /// Griser ou verrouiller une couche (LOT-EDITOR-02, phase 3) : aides d'édition.
    void dimRequested(hmi::LayerSlot slot, bool dimmed);
    void lockRequested(hmi::LayerSlot slot, bool locked);
    void addRequested(core::LayerKind kind);
    void removeRequested(std::size_t index);
    void moveRequested(std::size_t index, bool forward);
    void renameRequested(std::size_t index, const QString& name);

private:
    /// Ce que la liste affiche, pour ne la reconstruire que s'il a changé.
    struct Snapshot {
        std::vector<LayerRow> rows;
        std::vector<LayerDisplay> displays;
        LayerSlot active;
        bool operator==(const Snapshot&) const = default;
    };

    void rebuild();
    void updateButtons();
    [[nodiscard]] static QString rowLabel(const LayerRow& row);
    /// Une case ou un nom de la liste a changé : demande la visibilité ou le renommage.
    /// @param item La ligne modifiée.
    void onItemChanged(QListWidgetItem* item);
    [[nodiscard]] static LayerSlot slotOf(const QListWidgetItem* item);

    /// Les widgets du panneau, construits en code (`LayersPanel.cpp`).
    struct Widgets;
    std::unique_ptr<Widgets> _ui;
    Snapshot _snapshot;
    bool _hasVisualLayers = false;
    /// Vrai pendant une reconstruction : les signaux des widgets n'y sont pas des gestes.
    bool _rebuilding = false;
};

}  // namespace hmi
