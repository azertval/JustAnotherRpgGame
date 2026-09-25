// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/GamepadNavigator.h"

#include <chrono>
#include <utility>

namespace hmi {

namespace {

// Soixante sondages par seconde : la frequence d'affichage, rien de plus fin n'est perceptible.
constexpr int POLL_PERIOD_MS = 16;

}  // namespace

GamepadNavigator::GamepadNavigator(QObject* parent) : QObject(parent) {
    _timer.setInterval(POLL_PERIOD_MS);
    connect(&_timer, &QTimer::timeout, this, &GamepadNavigator::poll);
}

void GamepadNavigator::setActive(bool active) {
    if (active == _timer.isActive()) {
        return;
    }
    if (active) {
        _timer.start();
    } else {
        _timer.stop();
        _input.releaseAll();
    }
    emit activeChanged();
}

void GamepadNavigator::poll() {
    _input.beginFrame();
    _poller.poll(_input);
    if (_input.gamepadConnected() != _connected) {
        _connected = _input.gamepadConnected();
        emit connectedChanged();
    }

    const auto now = std::chrono::steady_clock::now();
    constexpr std::array<std::pair<GamepadButton, const char*>, 4> DIRECTIONS{{
        {GamepadButton::Up, "up"},
        {GamepadButton::Down, "down"},
        {GamepadButton::Left, "left"},
        {GamepadButton::Right, "right"},
    }};
    for (std::size_t i = 0; i < DIRECTIONS.size(); ++i) {
        if (_directions[i].update(_input.gamepadButtonDown(DIRECTIONS[i].first), now)) {
            emit pressed(QString::fromLatin1(DIRECTIONS[i].second));
        }
    }
    constexpr std::array<std::pair<GamepadButton, const char*>, 6> BUTTONS{{
        {GamepadButton::A, "a"},
        {GamepadButton::B, "b"},
        {GamepadButton::X, "x"},
        {GamepadButton::Y, "y"},
        {GamepadButton::LeftShoulder, "lb"},
        {GamepadButton::RightShoulder, "rb"},
    }};
    for (const auto& [button, name] : BUTTONS) {
        if (_input.gamepadButtonPressed(button)) {
            emit pressed(QString::fromLatin1(name));
        }
    }
}

}  // namespace hmi
