#pragma once

#include <cstdint>
#include <optional>

namespace AnyBox::AcpHal {
struct XmaWorkState {
    std::uint64_t signature;
    std::uint64_t generation;
    std::uint32_t context;
    std::uint32_t format;
};

inline constexpr std::uint64_t XmaWorkSignature = 0x31574D5842584E41;
static_assert(sizeof(XmaWorkState) == 24);
std::optional<XmaWorkState> ReadXmaWorkState(std::uint32_t address);
}
