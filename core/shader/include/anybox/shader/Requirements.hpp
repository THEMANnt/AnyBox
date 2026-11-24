#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace AnyBox::Shader {
std::optional<std::uint32_t> GetComputeWaveSize(std::span<const std::byte> code);
}
