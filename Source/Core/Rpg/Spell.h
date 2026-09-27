// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/Spell.h
 * @brief Le catalogue des sorts (`spell.schema.json`), et ce que le moteur sait en jouer
 *        (`LOT-131`, `EX-RPG-050`, `EX-RPG-051`).
 *
 * Un sort est une **combinaison déclarée de mécanismes**, jamais une fonction. Le socle
 * (`LOT-131`) jouait le sort à **jet d'attaque** ; le Mage (`LOT-133`) ajoute les **projectiles**
 * (un jet ou des dés par rayon), le sort qui **touche sans jet**, le sort à **jet de sauvegarde**
 * — sur une cible ou dans une **sphère** —, et l'**effet qui dure** posé sur une créature (le vol,
 * l'invisibilité), sous concentration. `core::spellMechanism` dit, sort par sort, lequel le moteur
 * sait jouer : aucun ne tombe dans un cas par défaut, et un sort sans mécanisme est dit, pas tu
 * (`EX-RPG-051`).
 */

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

/// @brief Qui un sort peut viser (`LOT-133`).
enum class SpellTarget : std::uint8_t {
    /// Une créature hostile : les sorts qui blessent.
    Enemy,
    /// Une créature de son camp, soi compris : les sorts qui aident (*vol*, *invisibilité*).
    Ally,
    /// Le lanceur seul.
    Self,
};

/// @brief Ce qu'une sauvegarde réussie fait des dégâts d'un sort.
enum class SaveEffect : std::uint8_t {
    /// Rien : la cible qui réussit ne subit rien (*flamme sacrée*).
    Negates,
    /// La moitié, arrondie à l'inférieur (*boule de feu*).
    Half,
};

/// @brief Les effets qui durent qu'un sort pose sur une créature, et que le moteur sait jouer.
enum class SpellEffectKind : std::uint8_t {
    /// La créature vole, à la vitesse que l'effet donne (*vol*).
    Fly,
    /// La créature est invisible : attaquée avec désavantage, elle attaque avec avantage ; l'effet
    /// cesse quand elle attaque ou lance un sort (*invisibilité*).
    Invisible,
    /// La créature ajoute des dés à ses jets d'attaque et de sauvegarde (*bénédiction*,
    /// `LOT-134`).
    Bless,
    /// Une arme invoquée par le lanceur : tant qu'elle dure, relancer le sort la fait frapper
    /// sans dépenser de lancer (*arme spirituelle*, `LOT-134`).
    SpiritualWeapon,
};

/// @brief Nom de donnée d'un genre d'effet de sort (`fly`…), celui du schéma.
[[nodiscard]] std::string_view spellEffectKindName(SpellEffectKind kind) noexcept;

/// @brief Un effet qui dure, posé par un sort.
struct SpellEffect {
    SpellEffectKind kind = SpellEffectKind::Fly;
    /// `Fly` : la vitesse de vol, en mètres.
    float meters = 0.0F;
    /// `Bless` : les dés ajoutés aux jets.
    std::optional<Dice> dice;
    /// La durée en rounds ; 0 : tout le combat (une minute en fait dix, une heure dépasse
    /// toujours un combat).
    int durationRounds = 0;
};

/// @brief Un sort, tel que `spell.schema.json` l'écrit.
struct Spell {
    std::string id;
    std::string name;
    std::string source;
    /// 0 : sort mineur (*cantrip*), à volonté.
    int level = 0;
    std::string school;
    std::string castingTime;
    /// Le texte du livre (« 36 mètres »).
    std::string range;
    /// La portée en **mètres**, si le sort vise à distance ; 0 sinon.
    float rangeMeters = 0.0F;
    std::string duration;
    bool concentration = false;
    bool ritual = false;
    /// Vrai pour un sort qui demande un jet d'attaque de sort contre la CA.
    bool attackRoll = false;
    /// Vrai pour un sort qui touche **sans jet** (*projectile magique*).
    bool autoHit = false;
    /// Le nombre de projectiles ou de rayons, chacun ses dés — et son jet s'il en demande un.
    int projectiles = 1;
    /// Vrai pour un sort mineur dont les dés montent aux niveaux 5, 11 et 17 du lanceur (Manuel,
    /// « Tours de magie »).
    bool cantripScaling = false;
    /// Une attaque de sort **au corps à corps** (*arme spirituelle*) : la portée sert d'allonge.
    bool meleeAttack = false;
    /// Le modificateur d'incantation s'ajoute aux dégâts ou au soin.
    bool addsAbilityModifier = false;
    /// Les dés de soin (*soin des blessures*, `LOT-134`).
    std::optional<Dice> healing;
    /// Le sort se lance par une action bonus.
    bool bonusAction = false;
    /// Les créatures qu'un sort qui aide peut viser (*bénédiction* : 3).
    int maxTargets = 1;
    /// Ce qu'une sauvegarde réussie fait des dégâts.
    SaveEffect saveEffect = SaveEffect::Negates;
    /// Le rayon de la **sphère** que le sort remplit, en mètres ; 0 : une seule cible.
    float areaRadiusMeters = 0.0F;
    /// Une forme de zone que le moteur ne sait pas poser (cône, ligne…) : le sort n'est pas joué.
    std::string unsupportedArea;
    SpellTarget target = SpellTarget::Enemy;
    /// L'effet qui dure, si le sort en pose un que le moteur sait jouer.
    std::optional<SpellEffect> effect;
    std::optional<Dice> damage;
    std::optional<DamageType> damageType;
    std::optional<Ability> savingThrow;
    std::string appliesCondition;
    std::string text;
    /// Déclaré sans mécanisme joué (`EX-RPG-051`).
    bool narrative = false;
    std::vector<std::string> requiredMechanisms;
};

/// @brief Le catalogue des sorts, et ce qui n'a pas pu l'être.
struct SpellCatalog {
    std::vector<Spell> spells;
    std::vector<std::string> errors;

    /// @brief Le sort d'identifiant @p id, ou `nullptr` s'il est inconnu.
    [[nodiscard]] const Spell* find(std::string_view id) const;
};

/**
 * @brief Charge les sorts depuis leur dossier (`Source/Elements/Rpg/spells`).
 *
 * Balaye le dossier, jamais une liste de noms. Un type de dégâts inconnu refuse le sort
 * (`EX-CBT-032`) ; des dés illisibles aussi. Ne lève jamais (`EX-NFR-040`).
 */
[[nodiscard]] SpellCatalog loadSpells(const std::filesystem::path& spellsDir);

/**
 * @brief Vrai si le moteur sait jouer ce sort comme une **attaque** : jet d'attaque, dés et type
 *        de dégâts.
 */
[[nodiscard]] bool isAttackSpell(const Spell& spell) noexcept;

/// @brief Les mécanismes de sort que le moteur sait jouer en combat.
enum class SpellMechanism : std::uint8_t {
    /// Un jet d'attaque de sort par projectile (*trait de feu*, *rayon ardent*).
    AttackRoll,
    /// Des dés par projectile, sans jet (*projectile magique*).
    AutoHit,
    /// Un jet de sauvegarde de chaque cible, sur une créature ou dans une sphère (*boule de feu*).
    SavingThrow,
    /// Un effet qui dure, posé sur une créature (*vol*, *invisibilité*, *bénédiction*).
    Effect,
    /// Des points de vie rendus à une créature de son camp (*soin des blessures*, `LOT-134`).
    Healing,
};

/**
 * @brief Le mécanisme par lequel le moteur joue ce sort, ou `std::nullopt` s'il n'en a aucun :
 *        un sort narratif, un sort qui exige un mécanisme absent, une zone qui n'est pas une
 *        sphère.
 */
[[nodiscard]] std::optional<SpellMechanism> spellMechanism(const Spell& spell) noexcept;

/**
 * @brief Les dés d'un sort au niveau @p casterLevel du lanceur : un dé de plus aux niveaux 5, 11
 *        et 17 pour un sort mineur qui le déclare (`cantripScaling`), les dés du sort sinon.
 */
[[nodiscard]] std::optional<Dice> spellDamageAt(const Spell& spell, int casterLevel) noexcept;

}  // namespace core
