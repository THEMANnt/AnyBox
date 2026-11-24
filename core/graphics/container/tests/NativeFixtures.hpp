#pragma once

#include "Fixtures.hpp"

namespace Fixtures {
std::vector<std::byte> MakeNativeProgram();
std::vector<std::byte> MakeComputeProgram();
std::vector<std::byte> PackNativeProgram(std::span<const std::byte> data);
std::vector<std::byte> MakeNativeContainer(std::span<const std::byte> first, std::span<const std::byte> second);
}
