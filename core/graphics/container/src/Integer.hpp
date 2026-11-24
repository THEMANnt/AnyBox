#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace AnyBox::Graphics::Container {
inline std::uint32_t ReadInteger(std::span<const std::byte> data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4) throw std::invalid_argument("DXBC integer is outside the container");
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < 4; ++index) value |= std::to_integer<std::uint32_t>(data[offset + index]) << (index * 8);
    return value;
}

inline void WriteInteger(std::span<std::byte> data, std::size_t offset, std::uint32_t value) {
    if (offset > data.size() || data.size() - offset < 4) throw std::invalid_argument("DXBC integer output is outside the container");
    for (std::size_t index = 0; index < 4; ++index) data[offset + index] = static_cast<std::byte>(value >> (index * 8));
}
}
