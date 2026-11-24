#pragma once

#include <anybox/relinker/Image.hpp>

namespace AnyBox::Relinker {
inline std::uint32_t CheckedRva(std::uint64_t value) {
    if (value > UINT32_MAX) throw std::runtime_error("PE output address or size overflows");
    return static_cast<std::uint32_t>(value);
}

inline std::uint32_t Align(std::uint64_t value, std::uint32_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) throw std::runtime_error("PE output alignment is invalid");
    return CheckedRva((value + alignment - 1) & ~static_cast<std::uint64_t>(alignment - 1));
}

struct HelperSection {
    std::vector<std::byte> bytes;
    Directory exceptions;
    std::vector<std::array<std::uint32_t, 3>> calls;
};

HelperSection BuildHelpers(std::span<const LoweringSite> sites, std::span<const std::byte> functions, std::uint32_t sectionAddress);
}
