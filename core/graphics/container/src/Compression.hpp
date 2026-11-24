#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Graphics::Container {
void CompressDigest(std::array<std::uint32_t, 4>& state, std::span<const std::byte, 64> block);
}
