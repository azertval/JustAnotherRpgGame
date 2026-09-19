// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QWidget>
#include <vector>

#include "Editor/Logic/MapFormat.h"

class QCheckBox;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * @file Editor/Ui/ProblemsPanel.h
 * @brief Le panneau « Problems » : les constats du contrôle, sur **toutes** les cartes du projet
 *        (`LOT-EDITOR-07`).
 */

namespace hmi {

/**
 * @brief La liste des constats de `hmi::checkAllMaps` — ceux que `LevelEditor --check` écrit en CI
 * — et le chemin vers chacun.
 *
 * Le panneau ne contrôle rien lui-même : il montre un bilan qu'on lui donne, et demande un nouveau
 * contrôle (« Check all maps »). Le contrôle lit les **fichiers** : une carte ouverte se contrôle
 * telle qu'elle a été enregistrée, ce que dit la ligne de bilan. Un double-clic demande d'aller au
 * constat : ouvrir sa carte, sélectionner son entité, cerner sa case.
 */
class ProblemsPanel : public QWidget {
    Q_OBJECT

public:
    explicit ProblemsPanel(QWidget* parent = nullptr);

    /// @brief Montre le bilan @p report, erreurs d'abord.
    void setReport(const MapCheckReport& report);

    /// @return Le bilan affiché.
    [[nodiscard]] const MapCheckReport& report() const noexcept {
        return _report;
    }

signals:
    /// Le bouton « Check all maps ».
    void checkRequested();
    /// Un constat double-cliqué (ou validé au clavier).
    void findingActivated(const hmi::MapCheckFinding& finding);

private:
    void rebuild();
    void onActivated(QTreeWidgetItem* item);

    MapCheckReport _report;
    QLabel* _summary = nullptr;
    QCheckBox* _errorsOnly = nullptr;
    QTreeWidget* _tree = nullptr;
};

}  // namespace hmi
