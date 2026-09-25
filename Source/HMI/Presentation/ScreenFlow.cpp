// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Presentation/ScreenFlow.h"

namespace hmi {

std::optional<ScreenState> resolveTransition(const ScreenState& current,
                                             ScreenEvent event) noexcept {
    switch (current.screen) {
        case ScreenId::Menu:
            switch (event) {
                case ScreenEvent::OpenMenu:
                    return ScreenState{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenGame:
                    return ScreenState{.screen = ScreenId::Game, .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenOptions:
                    return ScreenState{.screen = ScreenId::Options,
                                       .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenCredits:
                    return ScreenState{.screen = ScreenId::Credits,
                                       .optionsReturnTo = ScreenId::Menu};
                // Depuis le menu, les écrans du RPG s'ouvrent en ECHAFAUDAGE (LOT-68) : « Nouvelle
                // partie » n'a pas de carte à charger tant que le LOT-27 n'en livre pas une, et
                // huit écrans qu'on ne peut pas atteindre ne se valident pas.
                case ScreenEvent::OpenRpgScreen:
                    return ScreenState{.screen = ScreenId::RpgScreen,
                                       .optionsReturnTo = ScreenId::Menu,
                                       .rpgReturnTo = ScreenId::Menu};
                // Le Colisée (LOT-50) s'ouvre depuis le menu — « Nouvelle partie », tant que
                // c'est la seule carte jouable — et seulement de là : c'est un mode du jeu, pas
                // un écran qu'on consulte pendant une partie. Le jour où l'arène
                // s'ouvrira depuis le monde comme une carte ordinaire (LOT-42), ce sera par
                // `Game`, pas par cet événement.
                case ScreenEvent::OpenArena:
                    return ScreenState{.screen = ScreenId::Arena,
                                       .optionsReturnTo = ScreenId::Menu,
                                       .rpgReturnTo = ScreenId::Menu,
                                       .arenaReturnTo = ScreenId::Menu};
                default:
                    return std::nullopt;
            }
        case ScreenId::Arena:
            switch (event) {
                // On revient d'où l'on vient : au menu si le Colisée s'est ouvert de là, sur la
                // CARTE si le héraut a envoyé sur le sable (LOT-09) -- et la carte est restée ce
                // qu'elle était, personnage au même endroit.
                case ScreenEvent::CloseArena:
                    return ScreenState{.screen = current.arenaReturnTo,
                                       .optionsReturnTo = ScreenId::Menu,
                                       .rpgReturnTo = current.arenaReturnTo,
                                       .arenaReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenMenu:
                    return ScreenState{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
                default:
                    return std::nullopt;
            }
        case ScreenId::Credits:
            switch (event) {
                case ScreenEvent::CloseCredits:
                    return ScreenState{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
                default:
                    return std::nullopt;
            }
        // Les deux ecrans de fin (LOT-119) ferment la partie : ni pause, ni retour sur la carte.
        // La mort recommence ou rend le menu ; la fin de la demo mene aux credits ou au menu.
        case ScreenId::Death:
            switch (event) {
                case ScreenEvent::OpenGame:
                    return ScreenState{.screen = ScreenId::Game, .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenMenu:
                    return ScreenState{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
                default:
                    return std::nullopt;
            }
        case ScreenId::DemoEnd:
            switch (event) {
                case ScreenEvent::OpenCredits:
                    return ScreenState{.screen = ScreenId::Credits,
                                       .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenMenu:
                    return ScreenState{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
                default:
                    return std::nullopt;
            }
        case ScreenId::Game:
            switch (event) {
                case ScreenEvent::OpenMenu:
                    return ScreenState{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
                // Le héraut envoie sur le sable (LOT-09) : la zone de combat de la carte se joue
                // dans le Colisée, et l'on revient ici -- `arenaReturnTo` le retient.
                case ScreenEvent::OpenArena:
                    return ScreenState{.screen = ScreenId::Arena,
                                       .optionsReturnTo = ScreenId::Menu,
                                       .rpgReturnTo = ScreenId::Menu,
                                       .arenaReturnTo = ScreenId::Game};
                case ScreenEvent::OpenPause:
                    return ScreenState{.screen = ScreenId::Pause,
                                       .optionsReturnTo = ScreenId::Menu};
                // Depuis le jeu : la fiche, l'inventaire ou la carte s'ouvrent et se referment sur
                // la partie en cours. C'est `hmi::pausesGame` qui dit lequel suspend la simulation
                // (EX-IHM-091), pas cette table -- elle ne connaît pas les huit écrans.
                case ScreenEvent::OpenRpgScreen:
                    return ScreenState{.screen = ScreenId::RpgScreen,
                                       .optionsReturnTo = ScreenId::Menu,
                                       .rpgReturnTo = ScreenId::Game};
                case ScreenEvent::OpenDeath:
                    return ScreenState{.screen = ScreenId::Death,
                                       .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenDemoEnd:
                    return ScreenState{.screen = ScreenId::DemoEnd,
                                       .optionsReturnTo = ScreenId::Menu};
                default:
                    return std::nullopt;
            }
        case ScreenId::Options:
            switch (event) {
                case ScreenEvent::CloseOptions:
                    return ScreenState{.screen = current.optionsReturnTo,
                                       .optionsReturnTo = ScreenId::Menu};
                default:
                    return std::nullopt;
            }
        case ScreenId::Pause:
            switch (event) {
                case ScreenEvent::ResumePause:
                    return ScreenState{.screen = ScreenId::Game, .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::QuitPauseToMenu:
                    return ScreenState{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenOptions:
                    return ScreenState{.screen = ScreenId::Options,
                                       .optionsReturnTo = ScreenId::Pause};
                case ScreenEvent::OpenRpgScreen:
                    return ScreenState{.screen = ScreenId::RpgScreen,
                                       .optionsReturnTo = ScreenId::Menu,
                                       .rpgReturnTo = ScreenId::Pause};
                default:
                    return std::nullopt;
            }
        case ScreenId::RpgScreen:
            switch (event) {
                // Un seul retour, vers l'écran d'où l'on vient. Le PASSAGE d'un écran du RPG à un
                // autre n'est pas une transition de cette machine : les huit vivent sur une seule
                // page (`hmi::RpgScreenHost`), et c'est ce qui permet d'aller de la fiche au
                // journal sans repasser par le menu (EX-IHM-090).
                case ScreenEvent::CloseRpgScreen:
                    return ScreenState{.screen = current.rpgReturnTo,
                                       .optionsReturnTo = ScreenId::Menu,
                                       .rpgReturnTo = ScreenId::Menu};
                // Le heros tombe sur le HUD de combat, la demo se clot dans un dialogue : l'ecran
                // de fin prend la place, sans repasser par la carte (LOT-119).
                case ScreenEvent::OpenDeath:
                    return ScreenState{.screen = ScreenId::Death,
                                       .optionsReturnTo = ScreenId::Menu};
                case ScreenEvent::OpenDemoEnd:
                    return ScreenState{.screen = ScreenId::DemoEnd,
                                       .optionsReturnTo = ScreenId::Menu};
                default:
                    return std::nullopt;
            }
    }
    return std::nullopt;
}

}  // namespace hmi
