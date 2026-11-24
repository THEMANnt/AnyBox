#pragma once

#include <RdnaDecoder/RdnaProgram.hpp>
#include <ControlFlow/ControlFlowGraph.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace AnyBox::Shader {
class NativeFrontend {
public:
    explicit NativeFrontend(std::span<const std::byte> program);
    NativeFrontend(const NativeFrontend&) = delete;
    NativeFrontend& operator=(const NativeFrontend&) = delete;
    NativeFrontend(NativeFrontend&&) noexcept = default;
    NativeFrontend& operator=(NativeFrontend&&) noexcept = default;
    std::span<const std::uint32_t> GetCode() const noexcept { return code; }
    const ShaderRecompiler::RdnaProgram& GetDecoded() const noexcept { return decoded; }
    const ShaderRecompiler::ControlFlowGraph& GetGraph() const noexcept { return graph; }
    std::uint32_t GetWaveSize() const noexcept { return waveSize; }

private:
    std::vector<std::uint32_t> code;
    ShaderRecompiler::RdnaProgram decoded;
    ShaderRecompiler::ControlFlowGraph graph;
    std::uint32_t waveSize;
};
}
