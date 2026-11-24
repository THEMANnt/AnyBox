#pragma once

#include <cstdint>
#include <span>

namespace AnyBox::Shader {
void ValidateSpirv(std::span<const std::uint32_t> code, std::uint32_t vulkanVersion);
}
