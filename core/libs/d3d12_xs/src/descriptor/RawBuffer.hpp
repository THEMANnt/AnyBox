#pragma once

#include <array>
#include <cstdint>

namespace AnyBox::Graphics {
std::array<std::uint32_t, 8> EncodeRawBuffer(std::uint64_t address, std::uint64_t width, std::uint64_t firstElement, std::uint32_t elementCount, std::uint32_t componentMapping);
}
