#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace AnyBox::Graphics::Container {
std::array<std::byte, 16> ComputeDxbcChecksum(std::span<const std::byte> data);
}
