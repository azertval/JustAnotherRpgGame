// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>

namespace hmi {

/** Clavier de l'editeur, identifie par les codes virtuels Win32. */
enum class Key : std::uint16_t {
    Backspace = 0x08,
    Tab = 0x09,
    Enter = 0x0D,
    Shift = 0x10,
    Control = 0x11,
    Escape = 0x1B,
    Space = 0x20,
    Left = 0x25,
    Up = 0x26,
    Right = 0x27,
    Down = 0x28,
    D0 = 0x30,
    A = 0x41,
    C = 0x43,
    D = 0x44,
    E = 0x45,
    P = 0x50,
    Q = 0x51,
    R = 0x52,
    S = 0x53,
    T = 0x54,
    V = 0x56,
    W = 0x57,
    Y = 0x59,
    Z = 0x5A,
    F1 = 0x70,
    F2 = 0x71,
    F10 = 0x79,
};

}  // namespace hmi
