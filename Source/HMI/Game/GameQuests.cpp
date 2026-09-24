// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/GameQuests.h"

#include "Core/Rpg/Dialogue.h"

namespace hmi {

GameQuests loadGameQuests(const std::filesystem::path& root) {
    GameQuests lues;
    lues.catalog = core::loadQuests(root / "World" / "quests");
    lues.errors = lues.catalog.errors;
    // Les erreurs propres aux dialogues sont dites par qui les charge pour les jouer
    // (`hmi::DialogueModel`) ; seuls les usages de drapeaux sont relevés ici.
    const core::DialogueCatalog dialogues = core::loadDialogues(root / "World" / "dialogues");
    const std::vector<std::string> usages = core::validateFlagUses(lues.catalog, dialogues);
    lues.errors.insert(lues.errors.end(), usages.begin(), usages.end());
    return lues;
}

}  // namespace hmi
