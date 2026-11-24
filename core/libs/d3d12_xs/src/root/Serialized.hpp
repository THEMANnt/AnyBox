#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Graphics {
std::size_t LocateSerializedRootFlags(std::span<const std::byte> data);
std::uint32_t ReadSerializedRootFlags(std::span<const std::byte> data);
}
