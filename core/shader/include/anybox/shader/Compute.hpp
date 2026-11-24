#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace AnyBox::Shader {
struct ComputeInfo {
    std::string entryPoint;
    std::array<std::uint32_t, 3> threads;
    std::uint64_t flags = 0;
    std::uint32_t shaderModelMinor = 0;
    std::uint32_t minimumWaveSize = 0;
    std::uint32_t maximumWaveSize = 0;
    std::uint32_t preferredWaveSize = 0;
};

ComputeInfo ReadDxilComputeInfo(std::span<const std::byte> code);
}
