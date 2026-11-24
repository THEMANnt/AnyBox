#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

inline void SetXmaBit(std::span<std::byte> output, std::uint32_t offset, bool value) {
    auto mask = static_cast<std::byte>(1u << (7 - offset % 8));
    if (value) output[offset / 8] |= mask;
    else output[offset / 8] &= ~mask;
}

inline std::array<std::byte, 2048> MakeXmaSilence(std::uint32_t channels, std::uint32_t count) {
    std::vector<bool> bits(15, false);
    bits.push_back(true);
    bits.push_back(false);
    if (channels == 2) bits.push_back(false);
    bits.insert(bits.end(), 8, false);
    bits.insert(bits.end(), 3, false);
    if (channels == 2) {
        bits.push_back(false);
        bits.push_back(true);
        bits.push_back(false);
    }
    bits.insert(bits.end(), channels + 2, false);
    auto size = static_cast<std::uint32_t>(bits.size());
    for (std::uint32_t bit = 0; bit < 15; ++bit) bits[bit] = ((size >> (14 - bit)) & 1) != 0;
    std::array<std::byte, 2048> output{};
    output[0] = static_cast<std::byte>(count << 2);
    for (std::uint32_t frame = 0; frame < count; ++frame) {
        for (std::uint32_t bit = 0; bit < size; ++bit) SetXmaBit(output, 32 + frame * size + bit, bits[bit]);
        SetXmaBit(output, 32 + (frame + 1) * size - 1, frame + 1 != count);
    }
    return output;
}
