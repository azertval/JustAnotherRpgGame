// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/Dice.h"

#include <charconv>
#include <cstddef>

namespace core {

namespace {

// Bornes de bon sens. Elles ne sont pas la pour brider le jeu mais pour qu'une notation aberrante
// -- un `999d999` issu d'une extraction ratee -- soit refusee a l'analyse plutot que de faire
// tourner une boucle d'un million d'iterations au milieu d'un tour de combat.
constexpr int MAX_COUNT = 100;
constexpr int MAX_FACES = 100;
constexpr int MAX_MODIFIER = 999;

// Lit un entier decimal a partir de `position`, et avance. `nullopt` si aucun chiffre.
[[nodiscard]] std::optional<int> lireEntier(std::string_view texte, std::size_t& position) {
    const std::size_t debut = position;
    while (position < texte.size() && texte[position] >= '0' && texte[position] <= '9') {
        ++position;
    }
    if (position == debut) {
        return std::nullopt;
    }
    int valeur = 0;
    const auto* premier = texte.data() + debut;
    const auto* dernier = texte.data() + position;
    if (std::from_chars(premier, dernier, valeur).ec != std::errc{}) {
        return std::nullopt;  // debordement : la notation est aberrante, pas ambigue.
    }
    return valeur;
}

// Le modificateur signe qui suit un de (`+2`, `-1`), ecrit dans @p dice. Rend false si la
// notation est invalide a cet endroit.
[[nodiscard]] bool lireModificateur(std::string_view notation, std::size_t& position, Dice& dice) {
    const char signe = notation[position];
    if (signe != '+' && signe != '-') {
        return false;
    }
    ++position;
    const std::optional<int> modificateur = lireEntier(notation, position);
    if (!modificateur.has_value() || *modificateur > MAX_MODIFIER) {
        return false;
    }
    dice.modifier = signe == '-' ? -*modificateur : *modificateur;
    return true;
}

}  // namespace

std::optional<Dice> parseDice(std::string_view notation) {
    if (notation.empty()) {
        return std::nullopt;
    }

    std::size_t position = 0;
    const std::optional<int> premier = lireEntier(notation, position);
    if (!premier.has_value()) {
        return std::nullopt;
    }

    Dice dice;
    if (position < notation.size() && (notation[position] == 'd' || notation[position] == 'D')) {
        ++position;
        const std::optional<int> faces = lireEntier(notation, position);
        if (!faces.has_value() || *faces < 1 || *faces > MAX_FACES) {
            return std::nullopt;
        }
        if (*premier < 1 || *premier > MAX_COUNT) {
            return std::nullopt;
        }
        dice.count = *premier;
        dice.faces = *faces;
    } else {
        // Valeur fixe : « 4 » est une expression legitime, et le corpus en contient (des degats
        // qui ne se lancent pas). Elle se distingue d'un dé par l'absence de `d`, pas par sa
        // valeur.
        if (*premier > MAX_MODIFIER) {
            return std::nullopt;
        }
        dice.modifier = *premier;
        return position == notation.size() ? std::optional<Dice>{dice} : std::nullopt;
    }

    if (position < notation.size() && !lireModificateur(notation, position, dice)) {
        return std::nullopt;
    }

    // Un residu apres le modificateur -- « 1d6+2x » -- n'est pas une notation a moitie valide :
    // c'est une donnee dont on ne sait pas ce qu'elle voulait dire.
    return position == notation.size() ? std::optional<Dice>{dice} : std::nullopt;
}

std::string formatDice(const Dice& dice) {
    std::string texte;
    if (dice.count > 0) {
        texte = std::to_string(dice.count) + 'd' + std::to_string(dice.faces);
        if (dice.modifier > 0) {
            texte += '+' + std::to_string(dice.modifier);
        } else if (dice.modifier < 0) {
            texte += '-' + std::to_string(-dice.modifier);
        }
        return texte;
    }
    return std::to_string(dice.modifier);
}

DiceRoll rollDice(const Dice& dice, DeterministicRandom& random) {
    DiceRoll roll;
    roll.dice = dice;
    roll.faces.reserve(static_cast<std::size_t>(dice.count));
    roll.total = dice.modifier;
    for (int i = 0; i < dice.count; ++i) {
        const int face = random.nextInt(1, dice.faces);
        roll.faces.push_back(face);
        roll.total += face;
    }
    return roll;
}

std::string DiceRoll::describe() const {
    std::string texte = formatDice(dice) + " : ";
    bool premier = true;
    for (const int face : faces) {
        if (!premier) {
            texte += " + ";
        }
        texte += std::to_string(face);
        premier = false;
    }
    if (dice.modifier != 0) {
        if (!premier) {
            texte += dice.modifier > 0 ? " + " : " - ";
            texte += std::to_string(dice.modifier > 0 ? dice.modifier : -dice.modifier);
        } else {
            texte += std::to_string(dice.modifier);
        }
    }
    texte += " = " + std::to_string(total);
    return texte;
}

}  // namespace core
