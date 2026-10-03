// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Core/Levels/LevelProperties.h"

/**
 * @file Core/World/WorldClock.h
 * @brief L'**heure du monde** (`LOT-1007`, `EX-EXP-015`) : une heure du jour qui avance avec le
 *        temps de jeu.
 *
 * Elle n'a qu'un lecteur dans cette version : la lumière (`core::DayLightTable`). Rien du jeu n'en
 * dépend — ni présence d'un PNJ, ni boutique, ni rencontre : ce sont des règles, elles viendront
 * avec les zones.
 *
 * ## Ce qui la fait avancer
 *
 * L'exploration (`core::ExplorationSession::update`), et elle seule : un combat, un dialogue, un
 * menu ne font pas tourner la session, donc pas l'horloge. Un combat lancé au crépuscule se joue
 * tout entier au crépuscule — la lumière ne change pas pendant qu'on vise.
 *
 * ## Son allure
 *
 * Une heure du monde par **minute** réelle (`GAME_MINUTES_PER_SECOND`), soit un jour en
 * vingt-quatre minutes : assez lent pour qu'une lumière ne bouge pas à l'œil, assez rapide pour
 * qu'une séance voie passer le soir. Décision du 3 octobre 2026 (D-45).
 *
 * Logique pure, sans Qt.
 */

namespace core {

/// @brief L'heure du monde, en minutes depuis minuit, et le compte des jours écoulés.
class WorldClock {
public:
    /// Minutes d'un jour.
    static constexpr float MINUTES_PER_DAY = 1440.0F;
    /// Minutes du monde par seconde réelle : une heure du monde par minute.
    static constexpr float GAME_MINUTES_PER_SECOND = 1.0F;
    /// L'heure d'une partie neuve : dix heures du matin, le plein jour.
    static constexpr float START_MINUTES = 600.0F;

    /// @return L'heure du jour, en minutes depuis minuit, dans [0, 1440[.
    [[nodiscard]] float minutes() const noexcept {
        return _minutes;
    }

    /// @return L'heure du jour, en heures décimales, dans [0, 24[.
    [[nodiscard]] float hours() const noexcept {
        return _minutes / 60.0F;
    }

    /// @return Les jours entiers écoulés depuis le début de la partie.
    [[nodiscard]] int day() const noexcept {
        return _day;
    }

    /// @brief Règle l'heure du jour ; une valeur hors du jour est ramenée dedans. Le jour ne
    ///        change pas : c'est un réglage, pas du temps qui passe.
    void setMinutes(float minutes) noexcept;

    /// @brief Fait passer @p seconds de temps réel. Une durée négative ou non finie est ignorée.
    void advance(float seconds) noexcept;

    /// @return Vrai si l'horloge avance (`advance`) ; fausse, elle est figée par l'appelant.
    [[nodiscard]] bool running() const noexcept {
        return _running;
    }

    /// @brief Fige ou relance l'horloge : figée, `advance` ne fait rien. Pour une commande de
    ///        débug, une capture, un essai de l'éditeur à heure choisie.
    void setRunning(bool running) noexcept {
        _running = running;
    }

    [[nodiscard]] bool operator==(const WorldClock&) const = default;

private:
    float _minutes = START_MINUTES;
    int _day = 0;
    bool _running = true;
};

/// @return Les minutes depuis minuit de l'heure écrite `HH:MM` (ou `H:MM`, ou `HH`), dans
///         [0, 1440[ ; rien si le texte n'est pas une heure.
[[nodiscard]] std::optional<float> parseClockTime(std::string_view text) noexcept;

/// @return L'heure @p minutes écrite `HH:MM`, la minute tronquée.
[[nodiscard]] std::string formatClockTime(float minutes);

/**
 * @brief Clé de l'**heure fixe** d'une carte (`LOT-1007`), écrite `HH:MM` : une propriété de
 *        carte, pas un champ du format.
 *
 * Une carte qui la porte se montre toujours à cette heure, quelle que soit celle du monde : un
 * sous-sol reste dans la nuit de ses braseros, un sanctuaire dans son plein jour. L'horloge, elle,
 * continue d'avancer — on ressort à l'heure qu'il est.
 */
inline constexpr std::string_view MAP_HOUR_PROPERTY = "hour";

/// @return L'heure fixe que déclarent les propriétés de carte @p properties, en minutes depuis
///         minuit ; rien si la carte n'en déclare pas, ou si elle est mal écrite.
[[nodiscard]] std::optional<float> mapFixedMinutes(const PropertyMap& properties);

}  // namespace core
