#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace AnyBox::Shader {
std::size_t ReduceWaveMinMax(std::vector<std::uint32_t>& code);
}
