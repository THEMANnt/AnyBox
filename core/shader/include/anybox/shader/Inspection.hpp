#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace AnyBox::Shader {
std::string InspectDxil(std::span<const std::byte> code);
}
