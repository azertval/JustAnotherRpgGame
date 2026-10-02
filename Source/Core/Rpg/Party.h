// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Core/Rpg/Party.h
 * @brief Le groupe du joueur : jusqu'à quatre personnages, un meneur, un ordre de marche
 *        (`LOT-138`).
 *
 * ## Un ordre, et rien d'autre
 *
 * Le groupe est une **liste ordonnée** d'identifiants de personnage (`Rpg/characters/<id>.json`) :
 * le premier est le **meneur** — celui qu'on déplace, qui parle et qui jette les dés du dialogue —,
 * les suivants marchent derrière lui dans cet ordre. Il n'y a pas de champ « meneur » à côté de la
 * liste : deux mémoires de la même chose finissent par se contredire, et c'est le rang qui fait
 * foi.
 *
 * Le type ne connaît ni les fiches ni les figurines : il dit **qui**, pas **quoi**. Ce qu'un membre
 * sait faire se lit dans sa fiche, chargée par qui en a besoin.
 */

namespace core {

/// @brief Ce qu'une retouche du groupe a donné.
enum class PartyChange {
    /// Le groupe a changé.
    Done,
    /// Le groupe compte déjà `Party::MAX_MEMBERS` personnages.
    Full,
    /// Le personnage est déjà du groupe.
    AlreadyMember,
    /// Le personnage n'est pas du groupe.
    NotMember,
    /// On ne retire pas le dernier : un groupe vide n'a personne à mener.
    LastMember,
};

/**
 * @brief Le groupe du joueur : de un à quatre personnages, dans l'ordre de marche.
 *
 * Un groupe tout juste construit est vide ; tout ce qui en retire refuse d'en ôter le dernier
 * membre. L'identifiant est celui de la fiche (`heros-brawler`), jamais un nom affiché.
 */
class Party {
public:
    /**
     * Quatre, le groupe de la table de jeu (`Player's Guide to Tanares`, les quatre fiches
     * pré-tirées) ; l'Empire interdit d'ailleurs toute bande armée de plus de six (`Tanares
     * Sourcebook`, p. 48-49), et quatre reste en deçà.
     */
    static constexpr std::size_t MAX_MEMBERS = 4;

    Party() = default;

    /// @brief Un groupe de @p members, dans cet ordre ; les doublons et l'excédent sont ignorés.
    explicit Party(const std::vector<std::string>& members);

    /// @brief Ajoute @p characterId en queue de marche.
    PartyChange add(std::string characterId);

    /// @brief Retire @p characterId ; le suivant prend sa place, et le meneur retiré laisse la tête
    ///        au deuxième.
    PartyChange remove(std::string_view characterId);

    /// @brief Fait de @p characterId le meneur ; les autres gardent leur ordre relatif.
    PartyChange setLeader(std::string_view characterId);

    /**
     * @brief Passe la tête au suivant : le meneur va en queue, les autres avancent d'un rang.
     *
     * C'est le geste du clavier : répété, il fait le tour du groupe et revient au
     * premier, ce qu'un simple échange des deux premiers ne ferait pas.
     */
    PartyChange rotateLeader();

    /// @brief Échange les rangs @p first et @p second de l'ordre de marche.
    PartyChange swap(std::size_t first, std::size_t second);

    /// @return Les membres, meneur en tête.
    [[nodiscard]] const std::vector<std::string>& members() const noexcept {
        return _members;
    }

    /// @return L'identifiant du meneur, vide si le groupe est vide.
    [[nodiscard]] std::string_view leader() const noexcept {
        return _members.empty() ? std::string_view{} : std::string_view{_members.front()};
    }

    /// @return Vrai si @p characterId est du groupe.
    [[nodiscard]] bool contains(std::string_view characterId) const noexcept;

    [[nodiscard]] std::size_t size() const noexcept {
        return _members.size();
    }
    [[nodiscard]] bool empty() const noexcept {
        return _members.empty();
    }

    [[nodiscard]] bool operator==(const Party&) const = default;

private:
    std::vector<std::string> _members;
};

/// @brief Un personnage qu'on peut prendre dans le groupe : ce que l'écran de composition liste.
struct PartyCandidate {
    /// L'identifiant de la fiche, qui est aussi le nom du fichier sans `.json`.
    std::string id;
    /// Le nom du personnage, tel que sa fiche le porte.
    std::string name;
    /// Sa classe : c'est elle qui range sa figurine (`Characters/Heroes/<classe>`).
    std::string classId;
    /// Le fichier de la fiche, que `core::loadCharacterSheet` lira.
    std::filesystem::path file;

    [[nodiscard]] bool operator==(const PartyCandidate&) const = default;
};

/// @brief Les personnages d'un dossier de fiches, et ce qui n'a pas pu être lu.
struct PartyCandidates {
    /// Dans l'ordre des identifiants.
    std::vector<PartyCandidate> candidates;
    /// Un fichier illisible, ou sans identifiant, nommé (`EX-CNT-010`) ; il n'est pas proposé.
    std::vector<std::string> errors;
};

/**
 * @brief Lit les fiches de @p directory (`Rpg/characters/`) sans les construire : identifiant,
 *        nom et classe.
 *
 * Construire une fiche demande tous les catalogues de règles ; lister qui l'on peut prendre ne
 * demande que trois champs. Un dossier absent donne une liste vide et une erreur.
 */
[[nodiscard]] PartyCandidates loadPartyCandidates(const std::filesystem::path& directory);

/**
 * @brief Le groupe d'une partie neuve : d'abord ceux de @p order présents parmi @p candidates,
 *        dans cet ordre, puis les autres candidats dans le leur, jusqu'à
 *        @ref Party::MAX_MEMBERS.
 *
 * Le jeu impose le groupe préformé (`hmi::WorldModel::STARTING_PARTY`) : Brawler, Priest,
 * Scoundrel, Mage — décision de l'auteur, `LOT-138`. Un identifiant de @p order absent du dossier
 * est sauté.
 */
[[nodiscard]] Party defaultParty(const std::vector<PartyCandidate>& candidates,
                                 const std::vector<std::string>& order = {});

}  // namespace core
