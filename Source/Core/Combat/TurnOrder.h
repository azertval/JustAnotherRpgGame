// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/TurnOrder.h
 * @brief L'ordre d'initiative : qui joue avant qui, et comment se départagent les égalités
 *        (`LOT-20`, `EX-CBT-010`).
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Core/Combat/BattleGrid.h"

namespace core {

/**
 * @brief Le camp d'un combattant.
 *
 * Deux camps, et **aucun héros** : un allié est un allié, qu'il soit le personnage du joueur, un
 * compagnon ou une invocation hors groupe (`EX-CBT-010`). Rien dans l'ordre ni dans la machine à
 * états ne distingue « le » joueur — c'est ce qui fera du groupe de quatre (`LOT-29`) un ajout et
 * non une refonte.
 */
enum class CombatSide : std::uint8_t {
    Allies,
    Enemies,
};

/**
 * @brief La place d'un combattant dans l'ordre : son jet, et ce qui départage une égalité.
 *
 * Tout ce qui sert au départage est **recopié** ici, au moment du jet : l'ordre reste stable
 * jusqu'à la fin du combat (`EX-CBT-010`), même si la Dextérité d'un combattant change en cours
 * de route.
 */
struct InitiativeEntry {
    CombatantId combatant{};
    /// Le total du test de Dextérité : le dé retenu plus le modificateur — ou la valeur fixe d'un
    /// combattant qui entre à une initiative imposée (renfort à 0).
    int total = 0;
    /// Le modificateur d'initiative, premier critère de départage.
    int modifier = 0;
    /// La valeur de Dextérité, second critère.
    int dexterity = 10;
    CombatSide side = CombatSide::Allies;
};

/**
 * @brief Vrai si @p first joue avant @p second.
 *
 * ## La règle de départage
 *
 * Manuel des Joueurs, « Initiative » (PDF p. 191) : les combattants se classent du plus haut
 * résultat au plus bas, et « en cas d'égalité », c'est le MD qui tranche entre ses créatures, les
 * joueurs entre leurs personnages, et le MD encore entre un monstre et un personnage. Le jeu n'a
 * pas de MD : la règle doit être **écrite**, et elle ne peut pas dépendre de l'ordre dans lequel
 * les combattants ont été rangés en mémoire (`EX-CBT-010`). Dans l'ordre :
 *
 * 1. le **total** le plus haut ;
 * 2. le **modificateur d'initiative** le plus haut — la variante que le livre laisse au MD
 *    (« relancer un d20 ») est écartée : un nouveau tirage consommerait la suite aléatoire et
 *    ferait dépendre l'ordre de *combien* d'égalités il y a eu ;
 * 3. la **Dextérité** la plus haute ;
 * 4. les **alliés** avant les ennemis — c'est la part du MD « entre un monstre et un personnage »,
 *    tranchée en faveur du joueur, qui n'a personne à qui en appeler ;
 * 5. l'**identifiant** le plus petit. Il n'est pas un ordre d'insertion : le montage de la
 *    rencontre les attribue dans l'ordre de la **donnée** — le groupe, puis les combattants dans
 *    l'ordre où la rencontre les écrit (`core::mountEncounter`).
 */
[[nodiscard]] bool actsBefore(const InitiativeEntry& first, const InitiativeEntry& second) noexcept;

/**
 * @brief Un repère d'initiative **fixe**, qui n'est pas un combattant.
 *
 * Les actions de repaire se jouent « au rang 20 de l'initiative, en perdant les égalités », les
 * renforts entrent au rang 0 : le livre place des événements dans l'ordre, pas seulement des
 * créatures. Un repère **perd toujours** l'égalité contre un combattant ; deux repères de même rang
 * se rangent par nom.
 */
struct InitiativeMarker {
    int count = 0;
    /// Le nom sous lequel le crochet `core::CombatHook::InitiativeCount` l'annonce.
    std::string name;
};

/**
 * @brief Une place de l'ordre : un combattant, ou un repère.
 *
 * Elle porte **tout** ce qui la range, et pas seulement un identifiant : une place reste
 * comparable après que son combattant a quitté l'ordre, ce qui permet de calculer la suivante
 * quand le combattant actif vient de fuir.
 */
struct TurnSlot {
    /// Renseignée pour un combattant.
    std::optional<InitiativeEntry> entry;
    /// Le repère, quand `entry` est vide.
    InitiativeMarker marker;

    /// @brief Le combattant de cette place, ou `std::nullopt` si la place est un repère.
    [[nodiscard]] std::optional<CombatantId> combatant() const {
        return entry.has_value() ? std::optional<CombatantId>(entry->combatant) : std::nullopt;
    }
    /// @brief Le rang : le total du combattant, ou celui du repère.
    [[nodiscard]] int count() const {
        return entry.has_value() ? entry->total : marker.count;
    }
};

/// @brief Vrai si la place @p first vient avant @p second dans le round.
[[nodiscard]] bool slotBefore(const TurnSlot& first, const TurnSlot& second);

/**
 * @brief L'ordre d'initiative d'un combat : combattants et repères, du premier au dernier.
 *
 * ## Une place, pas un indice
 *
 * Un combattant qui entre en cours de combat (un renfort) ou qui en sort (une fuite, une mort)
 * change la **longueur** de l'ordre. Un curseur tenu par indice sauterait alors un tour ou en
 * rejouerait un. Le tour courant se désigne donc par sa **place** (`TurnSlot`), et la suivante se
 * calcule par la règle d'ordre : un renfort rangé après la place courante joue ce round-ci, rangé
 * avant, au round suivant — ce que dit le livre, sans cas particulier.
 */
class TurnOrder {
public:
    /// @brief Range un combattant. Faux, et rien ne change, s'il y figure déjà.
    bool add(const InitiativeEntry& entry);
    /// @brief Retire un combattant. Faux s'il n'y figurait pas.
    bool remove(CombatantId combatant);
    /// @brief Range un repère d'initiative fixe. Faux, et rien ne change, si un repère de même rang
    /// et de même nom existe déjà : deux places identiques ne se distingueraient plus.
    bool addMarker(InitiativeMarker marker);

    /// @brief Vrai si @p combatant a une place dans l'ordre d'initiative.
    [[nodiscard]] bool contains(CombatantId combatant) const;
    /// @brief La place d'un combattant, ou `nullptr`.
    [[nodiscard]] const InitiativeEntry* find(CombatantId combatant) const;
    /// @brief Les combattants, du premier au dernier.
    [[nodiscard]] const std::vector<InitiativeEntry>& entries() const noexcept {
        return _entries;
    }
    /// @brief Les repères, du premier au dernier.
    [[nodiscard]] const std::vector<InitiativeMarker>& markers() const noexcept {
        return _markers;
    }

    /// @brief La première place d'un round, ou `std::nullopt` si l'ordre est vide.
    [[nodiscard]] std::optional<TurnSlot> firstSlot() const;
    /**
     * @brief La place qui suit @p slot dans le round, ou `std::nullopt` si le round est fini.
     *
     * @p slot n'a pas besoin de figurer encore dans l'ordre : le combattant qui vient de fuir
     * pendant son tour laisse une place dont la suivante se calcule quand même.
     */
    [[nodiscard]] std::optional<TurnSlot> slotAfter(const TurnSlot& slot) const;

private:
    /// Triés à chaque ajout : l'ordre se lit, il ne se recalcule pas.
    std::vector<InitiativeEntry> _entries;
    std::vector<InitiativeMarker> _markers;
};

}  // namespace core
