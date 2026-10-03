// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/Sha256.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace hmi {

namespace {

// Les constantes de ronde de SHA-256 (FIPS 180-4, §4.2.2).
constexpr std::array<std::uint32_t, 64> ROUNDS = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void compress(std::array<std::uint32_t, 8>& state, const unsigned char* block) {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t index = 0; index < 16; ++index) {
        words[index] = (static_cast<std::uint32_t>(block[index * 4]) << 24U) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 1]) << 16U) |
                       (static_cast<std::uint32_t>(block[(index * 4) + 2]) << 8U) |
                       static_cast<std::uint32_t>(block[(index * 4) + 3]);
    }
    for (std::size_t index = 16; index < 64; ++index) {
        const std::uint32_t low = std::rotr(words[index - 15], 7) ^
                                  std::rotr(words[index - 15], 18) ^ (words[index - 15] >> 3U);
        const std::uint32_t high = std::rotr(words[index - 2], 17) ^
                                   std::rotr(words[index - 2], 19) ^ (words[index - 2] >> 10U);
        words[index] = words[index - 16] + low + words[index - 7] + high;
    }
    std::array<std::uint32_t, 8> v = state;
    for (std::size_t index = 0; index < 64; ++index) {
        const std::uint32_t sum1 = std::rotr(v[4], 6) ^ std::rotr(v[4], 11) ^ std::rotr(v[4], 25);
        const std::uint32_t choice = (v[4] & v[5]) ^ (~v[4] & v[6]);
        const std::uint32_t first = v[7] + sum1 + choice + ROUNDS[index] + words[index];
        const std::uint32_t sum0 = std::rotr(v[0], 2) ^ std::rotr(v[0], 13) ^ std::rotr(v[0], 22);
        const std::uint32_t majority = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
        const std::uint32_t second = sum0 + majority;
        v = {first + second, v[0], v[1], v[2], v[3] + first, v[4], v[5], v[6]};
    }
    for (std::size_t index = 0; index < 8; ++index) {
        state[index] += v[index];
    }
}

}  // namespace

std::string sha256Hex(std::string_view bytes) {
    std::array<std::uint32_t, 8> state = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    const auto* const data = reinterpret_cast<const unsigned char*>(bytes.data());
    const std::size_t whole = bytes.size() / 64;
    for (std::size_t block = 0; block < whole; ++block) {
        compress(state, data + (block * 64));
    }
    // Le reste, le bit de fin, puis la longueur en bits sur les huit derniers octets.
    std::array<unsigned char, 128> tail{};
    const std::size_t rest = bytes.size() - (whole * 64);
    for (std::size_t index = 0; index < rest; ++index) {
        tail[index] = data[(whole * 64) + index];
    }
    tail[rest] = 0x80;
    const std::size_t padded = rest < 56 ? 64 : 128;
    const std::uint64_t bits = static_cast<std::uint64_t>(bytes.size()) * 8U;
    for (std::size_t index = 0; index < 8; ++index) {
        tail[padded - 1 - index] = static_cast<unsigned char>((bits >> (index * 8U)) & 0xFFU);
    }
    compress(state, tail.data());
    if (padded == 128) {
        compress(state, tail.data() + 64);
    }
    static constexpr std::string_view DIGITS = "0123456789abcdef";
    std::string hex;
    hex.reserve(64);
    for (const std::uint32_t word : state) {
        for (int shift = 28; shift >= 0; shift -= 4) {
            hex.push_back(DIGITS[(word >> static_cast<unsigned>(shift)) & 0xFU]);
        }
    }
    return hex;
}

}  // namespace hmi
