// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>

/**
 * @file HMI/Presentation/ScreenFlow.h
 * @brief Machine à états des écrans de la fenêtre principale (`EX-GP-041`).
 *
 * Logique **pure** (aucune dépendance Qt), testable hors instance d'application (`EX-NFR-010`) —
 * même patron que `Editor/Logic/PanelFocus.h`. `hmi::ScreenRouter`
 * ne fait que suivre cette table : toute navigation passe par `resolveTransition()` -- une
 * transition non déclarée ici est **refusée**, jamais silencieusement acceptée.
 */

namespace hmi {

/// Écran actuellement affiché par la fenêtre principale.
enum class ScreenId {
    Menu,
    Game,
    Options,
    Pause,
    Credits,
    /// **Un** écran du RPG est ouvert (`LOT-68`, `EX-IHM-090`). Lequel des huit n'est pas la
    /// question de cette table : c'est celle de `hmi::RpgScreenHost`, qui les héberge tous sur une
    /// seule page. Les y déclarer un par un aurait multiplié par huit les transitions à écrire
    /// pour n'exprimer, huit fois, que la même règle.
    RpgScreen,
    /// L'écran de mort (`LOT-119`) : le combat sur la carte est létal, la partie s'y termine. On
    /// n'en sort que par « Recommencer » (`OpenGame`) ou « Menu » (`OpenMenu`) — pas de retour à
    /// la partie où l'on vient de mourir.
    Death,
    /// L'écran « Fin de la démo » (`LOT-119`) : la quête est bouclée. On en sort par les crédits
    /// ou le menu.
    DemoEnd,
};

/// Événement pouvant déclencher une transition d'écran. Un seul événement `OpenOptions`/
/// `CloseOptions` sert Menu et Pause : c'est `ScreenState::optionsReturnTo` (pas l'événement) qui
/// porte la différence, cf. plus bas.
enum class ScreenEvent {
    OpenMenu,
    OpenGame,
    OpenOptions,
    CloseOptions,
    OpenPause,
    ResumePause,
    QuitPauseToMenu,
    OpenCredits,
    CloseCredits,
    /// Ouvre un écran du RPG. Un seul événement pour les huit, et depuis trois écrans (Menu, Game,
    /// Pause) : c'est `ScreenState::rpgReturnTo` qui porte la différence, comme
    /// `optionsReturnTo` le fait pour Options.
    OpenRpgScreen,
    CloseRpgScreen,
    /// Le héros est tombé (`LOT-119`) : depuis le combat (`RpgScreen`) ou la carte (`Game`).
    OpenDeath,
    /// Un dialogue a clos la démo (`LOT-119`) : depuis la conversation ou la carte.
    OpenDemoEnd,
};

/// État complet de la machine. `optionsReturnTo` n'est pertinent que lorsque `screen ==
/// ScreenId::Options` : l'écran vers lequel `CloseOptions` revient (`Menu` ou `Pause`, selon
/// l'origine) -- porté ici plutôt que par une variable « écran précédent » posée à côté.
struct ScreenState {
    ScreenId screen = ScreenId::Menu;
    ScreenId optionsReturnTo = ScreenId::Menu;
    /// Écran vers lequel `CloseRpgScreen` revient (`Menu`, `Game` ou `Pause`, selon l'origine).
    /// Même patron, et même raison, qu'`optionsReturnTo` : la provenance est un attribut de
    /// l'état, jamais une variable « écran précédent » posée à côté de la machine.
    ScreenId rpgReturnTo = ScreenId::Menu;

    friend bool operator==(const ScreenState&, const ScreenState&) = default;
};

/// Résout une transition d'écran à partir de l'état courant et d'un événement.
/// @return Le nouvel état, ou `std::nullopt` si @p event est interdit depuis `current.screen` --
///         l'appelant garde alors l'état courant inchangé, jamais de bascule silencieuse
///         (`EX-GP-041`).
[[nodiscard]] std::optional<ScreenState> resolveTransition(const ScreenState& current,
                                                           ScreenEvent event) noexcept;

}  // namespace hmi
