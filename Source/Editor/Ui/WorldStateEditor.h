// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QWidget>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Core/Gameplay/Quest.h"

/**
 * @file Editor/Ui/WorldStateEditor.h
 * @brief Le **sélecteur d'état de partie** (`LOT-126`) : une valeur par drapeau qu'une quête
 *        déclare, les faits acquis à cocher, et ceux qu'on écrit à la main.
 *
 * Il sert deux fenêtres : « Run in game… » (l'état de l'essai complet) et « World state… » (celui
 * sous lequel le canevas montre la carte, et que l'essai immédiat reprend). L'état qu'il rend est
 * celui de `Editor/Logic/WorldState.h`.
 */

class QComboBox;
class QLineEdit;
class QListWidget;

namespace hmi {

class WorldStateEditor : public QWidget {
    Q_OBJECT

public:
    /**
     * @param knownFlags Les drapeaux que posent dialogues, quêtes et zones.
     * @param declared   Les drapeaux à valeurs des quêtes : un choix de valeur chacun.
     * @param entries    L'état de départ (`fait`, `drapeau=valeur`).
     * @param parent     Le widget parent.
     */
    WorldStateEditor(const std::vector<std::string>& knownFlags,
                     const std::vector<core::QuestFlag>& declared,
                     const std::vector<std::string>& entries, QWidget* parent = nullptr);

    /// @return L'état choisi : les valeurs qui diffèrent de l'initiale, puis les faits cochés, puis
    ///         ceux écrits à la main.
    [[nodiscard]] std::vector<std::string> entries() const;

signals:
    void changed();

private:
    std::vector<std::pair<core::QuestFlag, QComboBox*>> _values;
    QListWidget* _facts = nullptr;
    QLineEdit* _extra = nullptr;
};

/// @brief L'état de partie du canevas, et s'il s'y montre.
struct WorldStateChoice {
    std::vector<std::string> entries;
    /// Vrai si le canevas grise ce que l'état rend absent.
    bool preview = false;

    [[nodiscard]] bool operator==(const WorldStateChoice&) const = default;
};

/// @brief Le dialogue « World state… » ; `std::nullopt` si l'auteur renonce.
[[nodiscard]] std::optional<WorldStateChoice> askWorldState(
    QWidget* parent, const std::vector<std::string>& knownFlags,
    const std::vector<core::QuestFlag>& declared, const WorldStateChoice& current);

}  // namespace hmi
