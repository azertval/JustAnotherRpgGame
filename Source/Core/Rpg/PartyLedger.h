// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/PartyLedger.h
 * @brief Ce que le combat **laisse aux fiches** du groupe (`LOT-139`) : les points de vie qui
 *        restent, les lancers de sorts dépensés, un membre mort.
 *
 * Les fiches pré-tirées (`Rpg/characters/`) sont des **fichiers**, relus à chaque rencontre, et
 * il n'existe pas encore de sauvegarde. Entre deux combats d'une même partie, quelque chose doit
 * pourtant retenir qu'Helga a perdu six points de vie et que Faelar a lancé son *projectile
 * magique* : c'est ce registre, une valeur par membre, que la partie tient (`hmi::WorldModel`)
 * et que le combat lit à l'entrée (`applyRecord`) et écrit à la sortie. Un membre sans
 * enregistrement est **plein** : la fiche telle qu'elle est écrite.
 *
 * Pas de règle de repos ici : un repos long (`core::longRest`) viendra avec l'auberge, et
 * effacera le registre.
 */

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "Core/Rpg/CharacterSheet.h"

namespace core {

/// @brief L'état d'un membre après un combat, par rapport à sa fiche écrite.
struct MemberRecord {
    /// Les points de vie courants ; absent : ceux de la fiche.
    std::optional<int> hitPoints;
    /// Les lancers restants de chaque sort connu (identifiant du sort → lancers) ; un sort absent
    /// garde ceux de la fiche.
    std::map<std::string, int> spellUses;
};

/**
 * @brief Le registre : un enregistrement par membre qui a combattu.
 */
class PartyLedger {
public:
    /// @return L'enregistrement de @p characterId, ou rien s'il n'a pas combattu.
    [[nodiscard]] const MemberRecord* record(std::string_view characterId) const;

    /// @brief Retient @p record pour @p characterId, à la place du précédent.
    void write(std::string characterId, MemberRecord record);

    /// @brief Oublie @p characterId : un membre mort, ou reposé.
    void erase(std::string_view characterId);

    /// @brief Oublie tout : une partie neuve, un repos long.
    void clear() noexcept {
        _records.clear();
    }

    [[nodiscard]] bool empty() const noexcept {
        return _records.empty();
    }

private:
    std::map<std::string, MemberRecord, std::less<>> _records;
};

/**
 * @brief Applique @p record à @p sheet : ses points de vie courants, bornés à `[0, maximum]`, et
 *        les lancers restants des sorts qu'il nomme, bornés à `[0, perDay]`.
 *
 * Un sort de l'enregistrement que la fiche ne connaît pas est ignoré : la fiche fait foi sur ce
 * que le personnage sait ; le registre ne dit que ce qu'il en reste.
 */
void applyRecord(CharacterSheet& sheet, const MemberRecord& record);

}  // namespace core
