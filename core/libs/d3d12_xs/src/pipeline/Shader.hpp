#pragma once

#include "../device/Device.hpp"
#include <span>

namespace AnyBox::Graphics {
void ValidateComputeShader(Device& device, std::span<const std::byte> code);
}
