// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/ScreenRouter.h"

#include <optional>

#include "Core/BuildConfig.h"
#include "Core/Rpg/Dialogue.h"
#include "HMI/Runtime/RuleLabels.h"

namespace hmi {

bool ScreenRouter::developerBuild() noexcept {
    return core::DEVELOPER_BUILD;
}

namespace {

// Correspondance entre l'état de la table et celui que le QML lit.
[[nodiscard]] ScreenRouter::Screen toRouterScreen(ScreenId screen) noexcept {
    switch (screen) {
        case ScreenId::Menu:
            return ScreenRouter::Screen::Menu;
        case ScreenId::Game:
            return ScreenRouter::Screen::Game;
        case ScreenId::Options:
            return ScreenRouter::Screen::Options;
        case ScreenId::Pause:
            return ScreenRouter::Screen::Pause;
        case ScreenId::Credits:
            return ScreenRouter::Screen::Credits;
        case ScreenId::RpgScreen:
            return ScreenRouter::Screen::RpgScreen;
        case ScreenId::Death:
            return ScreenRouter::Screen::Death;
        case ScreenId::DemoEnd:
            return ScreenRouter::Screen::DemoEnd;
    }
    return ScreenRouter::Screen::Menu;  // inatteignable : switch exhaustif sur ScreenId.
}

// Les deux énumérations décrivent les mêmes huit écrans, dans le même ordre : la conversion est
// donc un simple changement de type. C'est une hypothèse, et une hypothèse tacite se casse en
// silence — le jour où quelqu'un insère un écran au milieu d'une des deux, la fiche s'ouvrirait
// à la place de l'inventaire, sans la moindre erreur. Ces vérifications la rendent explicite, et
// leur échec se lit à la compilation.
static_assert(static_cast<int>(ScreenRouter::RpgScreen::CharacterSheet) ==
              static_cast<int>(RpgScreenId::CharacterSheet));
static_assert(static_cast<int>(ScreenRouter::RpgScreen::Skills) ==
              static_cast<int>(RpgScreenId::Skills));
static_assert(static_cast<int>(ScreenRouter::RpgScreen::Inventory) ==
              static_cast<int>(RpgScreenId::Inventory));
static_assert(static_cast<int>(ScreenRouter::RpgScreen::Dialogue) ==
              static_cast<int>(RpgScreenId::Dialogue));
static_assert(static_cast<int>(ScreenRouter::RpgScreen::CombatHud) ==
              static_cast<int>(RpgScreenId::CombatHud));
static_assert(static_cast<int>(ScreenRouter::RpgScreen::Company) ==
              static_cast<int>(RpgScreenId::Company));

[[nodiscard]] RpgScreenId toRpgScreenId(ScreenRouter::RpgScreen screen) noexcept {
    return static_cast<RpgScreenId>(screen);
}

[[nodiscard]] ScreenRouter::RpgScreen toRouterRpgScreen(RpgScreenId screen) noexcept {
    return static_cast<ScreenRouter::RpgScreen>(screen);
}

}  // namespace

ScreenRouter::ScreenRouter(QObject* parent) : QObject(parent) {}

ScreenRouter::Screen ScreenRouter::currentScreen() const noexcept {
    return toRouterScreen(_state.screen);
}

ScreenRouter::RpgScreen ScreenRouter::currentRpgScreen() const noexcept {
    return toRouterRpgScreen(_rpgScreen);
}

bool ScreenRouter::apply(ScreenEvent event) {
    // Toute la règle est dans la table. Une transition non déclarée rend `nullopt`, et l'état ne
    // bouge pas : c'est ce qui empêche un geste illégitime de produire un état d'où l'on ne sait
    // pas revenir (EX-GP-041).
    const std::optional<ScreenState> next = resolveTransition(_state, event);
    if (!next.has_value()) {
        return false;
    }
    _state = *next;
    emit changed();
    return true;
}

void ScreenRouter::openMenu() {
    static_cast<void>(apply(ScreenEvent::OpenMenu));
}
void ScreenRouter::openGame() {
    static_cast<void>(apply(ScreenEvent::OpenGame));
}
void ScreenRouter::openOptions() {
    static_cast<void>(apply(ScreenEvent::OpenOptions));
}
void ScreenRouter::closeOptions() {
    static_cast<void>(apply(ScreenEvent::CloseOptions));
}
void ScreenRouter::openPause() {
    static_cast<void>(apply(ScreenEvent::OpenPause));
}
void ScreenRouter::resume() {
    static_cast<void>(apply(ScreenEvent::ResumePause));
}
void ScreenRouter::quitToMenu() {
    static_cast<void>(apply(ScreenEvent::QuitPauseToMenu));
}
void ScreenRouter::openCredits() {
    static_cast<void>(apply(ScreenEvent::OpenCredits));
}
void ScreenRouter::closeCredits() {
    static_cast<void>(apply(ScreenEvent::CloseCredits));
}

void ScreenRouter::jumpToGame() {
    // `if constexpr` avec sa branche `else` : un retour anticipe laisserait en Release un code
    // inatteignable, que /W4 /WX refuse (C4702).
    if constexpr (core::DEVELOPER_BUILD) {
        if (_state.screen == ScreenId::Game) {
            return;
        }
        _state = ScreenState{.screen = ScreenId::Game, .optionsReturnTo = ScreenId::Menu};
        emit changed();
    }
}

void ScreenRouter::openDeath() {
    static_cast<void>(apply(ScreenEvent::OpenDeath));
}

void ScreenRouter::openDemoEnd(const QString& ending) {
    // La voie d'abord, l'ecran ensuite -- meme raison que `openDialogue`.
    _ending = ending;
    static_cast<void>(apply(ScreenEvent::OpenDemoEnd));
}

QString ScreenRouter::endingText() const {
    if (_ending.isEmpty()) {
        return {};
    }
    return QString::fromStdString(
        ruleLabel(core::demoEndingKey(_ending.toStdString()), activeLanguage()));
}

void ScreenRouter::openDialogue(const QString& dialogueId) {
    // Le dialogue d'abord, l'ecran ensuite : l'ecran lit `dialogueId` a sa construction, et le
    // poser apres l'ouverture lui ferait jouer la conversation precedente le temps d'une image.
    _dialogueId = dialogueId;
    openRpgScreen(RpgScreen::Dialogue);
}

void ScreenRouter::openRpgScreen(RpgScreen screen) {
    // L'écran demandé est retenu MÊME si la transition échoue déjà parce qu'on y est : ouvrir la
    // fiche puis l'inventaire depuis le châssis ne repasse pas par un changement d'état global.
    _rpgScreen = toRpgScreenId(screen);
    if (!apply(ScreenEvent::OpenRpgScreen)) {
        emit changed();  // même écran global, écran du RPG différent : la vue doit suivre.
    }
}

void ScreenRouter::closeRpgScreen() {
    static_cast<void>(apply(ScreenEvent::CloseRpgScreen));
}

}  // namespace hmi
